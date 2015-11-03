/**
 * rofi
 *
 * MIT/X11 License
 * Copyright 2013-2015 Qball Cow <qball@gmpclient.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <X11/X.h>

#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>
#include <strings.h>
#include <string.h>
#include <errno.h>

#include "rofi.h"
#include "helper.h"
#include "dialogs/drun.h"

#define RUN_CACHE_FILE    "rofi-2.runcache"

static inline int execsh ( const char *cmd, int run_in_term )
{
    int  retv   = TRUE;
    char **args = NULL;
    int  argc   = 0;
    if ( run_in_term ) {
        helper_parse_setup ( config.run_shell_command, &args, &argc, "{cmd}", cmd, NULL );
    }
    else {
        helper_parse_setup ( config.run_command, &args, &argc, "{cmd}", cmd, NULL );
    }
    GError *error = NULL;
    g_spawn_async ( NULL, args, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error );
    if ( error != NULL ) {
        char *msg = g_strdup_printf ( "Failed to execute: '%s'\nError: '%s'", cmd, error->message );
        error_dialog ( msg, FALSE  );
        g_free ( msg );
        // print error.
        g_error_free ( error );
        retv = FALSE;
    }

    // Free the args list.
    g_strfreev ( args );
    return retv;
}

// execute sub-process
static void exec_cmd ( const char *cmd, int run_in_term )
{
    if ( !cmd || !cmd[0] ) {
        return;
    }

    execsh ( cmd, run_in_term );
}

typedef struct _DRunModeEntry
{
    char         *exec;
    unsigned int terminal;
} DRunModeEntry;

typedef struct _DRunModePrivateData
{
    unsigned int  id;
    char          **cmd_list;
    DRunModeEntry *entry_list;
    unsigned int  cmd_list_length;
} DRunModePrivateData;

static void exec_cmd_entry ( DRunModeEntry *e )
{
    // strip % arguments
    gchar *str = g_strdup ( e->exec );
    for ( ssize_t i = 0; str && str[i]; i++ ) {
        if ( str[i] == '%' ) {
            while ( str[i] != ' ' && str[i] != '\0' ) {
                str[i++] = ' ';
            }
        }
    }
    execsh ( str, e->terminal );
    g_free ( str );
}
/**
 * Internal spider used to get list of executables.
 */
static void get_apps_dir ( DRunModePrivateData *pd, const char *bp )
{
    DIR *dir = opendir ( bp );

    if ( dir != NULL ) {
        struct dirent *dent;

        while ( ( dent = readdir ( dir ) ) != NULL ) {
            if ( dent->d_type != DT_REG && dent->d_type != DT_LNK && dent->d_type != DT_UNKNOWN ) {
                continue;
            }
            // Skip dot files.
            if ( dent->d_name[0] == '.' ) {
                continue;
            }
            gchar    *path  = g_build_filename ( bp, dent->d_name, NULL );
            GKeyFile *kf    = g_key_file_new ();
            GError   *error = NULL;
            // TODO: check what flags to set;
            g_key_file_load_from_file ( kf, path, 0, NULL );
            if ( error == NULL ) {
                if ( g_key_file_has_key ( kf, "Desktop Entry", "Exec", NULL ) ) {
                    pd->cmd_list   = g_realloc ( pd->cmd_list, ( ( pd->cmd_list_length ) + 2 ) * sizeof ( *( pd->cmd_list ) ) );
                    pd->entry_list = g_realloc ( pd->entry_list, ( pd->cmd_list_length + 2 ) * sizeof ( *( pd->entry_list ) ) );
                    if ( g_key_file_has_key ( kf, "Desktop Entry", "Name", NULL ) ) {
                        gchar *n  = NULL;
                        gchar *gn = NULL;
                        n  = g_key_file_get_string ( kf, "Desktop Entry", "Name", NULL );
                        gn = g_key_file_get_string ( kf, "Desktop Entry", "GenericName", NULL );
                        if ( gn == NULL ) {
                            pd->cmd_list[pd->cmd_list_length] = g_markup_escape_text ( n, -1 );
                        }
                        else {
                            ( pd->cmd_list )[( pd->cmd_list_length )] = g_markup_printf_escaped (
                                "%s <span weight='light' size='small'><i>(%s)</i></span>",
                                n,
                                gn ? gn : "" );
                        }
                        g_free ( n ); g_free ( gn );
                    }
                    else {
                        ( pd->cmd_list )[( pd->cmd_list_length )] = g_strdup ( dent->d_name );
                    }
                    pd->entry_list[pd->cmd_list_length].exec = g_key_file_get_string ( kf, "Desktop Entry", "Exec", NULL );
                    if ( g_key_file_has_key ( kf, "Desktop Entry", "Terminal", NULL ) ) {
                        pd->entry_list[pd->cmd_list_length].terminal = g_key_file_get_boolean ( kf, "Desktop Entry", "Terminal", NULL );
                    }
                    ( pd->cmd_list )[( pd->cmd_list_length ) + 1] = NULL;
                    ( pd->cmd_list_length )++;
                }
            }
            else {
                g_error_free ( error );
            }
            g_key_file_free ( kf );
            g_free ( path );
        }

        closedir ( dir );
    }
}

static void get_apps ( DRunModePrivateData *pd )
{
    const char * const * dr   = g_get_system_data_dirs ();
    const char * const * iter = dr;
    while ( iter != NULL && *iter != NULL && **iter != '\0' ) {
        gboolean skip = FALSE;
        for ( size_t i = 0; !skip && dr[i] != ( *iter ); i++ ) {
            skip = ( g_strcmp0 ( *iter, dr[i] ) == 0 );
        }
        if ( skip ) {
            iter++;
            continue;
        }
        gchar *bp = g_build_filename ( *iter, "applications", NULL );
        get_apps_dir ( pd, bp );
        g_free ( bp );
        iter++;
    }

    const char *d = g_get_user_data_dir ();
    for ( size_t i = 0; dr[i] != NULL; i++ ) {
        if ( g_strcmp0 ( d, dr[i] ) == 0 ) {
            // Done this already, no need to repeat.
            return;
        }
    }
    if ( d ) {
        gchar *bp = g_build_filename ( d, "applications", NULL );
        get_apps_dir ( pd, bp );
        g_free ( bp );
    }
}

static void drun_mode_init ( Switcher *sw )
{
    if ( sw->private_data == NULL ) {
        DRunModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
        sw->private_data = (void *) pd;
    }
}

static char ** drun_mode_get_data ( unsigned int *length, Switcher *sw )
{
    DRunModePrivateData *rmpd = (DRunModePrivateData *) sw->private_data;
    if ( rmpd->cmd_list == NULL ) {
        rmpd->cmd_list_length = 0;
        get_apps ( rmpd );
    }
    if ( length != NULL ) {
        *length = rmpd->cmd_list_length;
    }
    return rmpd->cmd_list;
}

static SwitcherMode drun_mode_result ( int mretv, char **input, unsigned int selected_line,
                                       Switcher *sw )
{
    DRunModePrivateData *rmpd = (DRunModePrivateData *) sw->private_data;
    SwitcherMode        retv  = MODE_EXIT;

    int                 shift = ( ( mretv & MENU_SHIFT ) == MENU_SHIFT );

    if ( mretv & MENU_NEXT ) {
        retv = NEXT_DIALOG;
    }
    else if ( mretv & MENU_PREVIOUS ) {
        retv = PREVIOUS_DIALOG;
    }
    else if ( mretv & MENU_QUICK_SWITCH ) {
        retv = ( mretv & MENU_LOWER_MASK );
    }
    else if ( ( mretv & MENU_OK ) && rmpd->cmd_list[selected_line] != NULL ) {
        exec_cmd_entry ( &( rmpd->entry_list[selected_line] ) );
    }
    else if ( ( mretv & MENU_CUSTOM_INPUT ) && *input != NULL && *input[0] != '\0' ) {
        exec_cmd ( *input, shift );
    }
    return retv;
}

static void drun_mode_destroy ( Switcher *sw )
{
    DRunModePrivateData *rmpd = (DRunModePrivateData *) sw->private_data;
    if ( rmpd != NULL ) {
        g_strfreev ( rmpd->cmd_list );
        for ( size_t i = 0; i < rmpd->cmd_list_length; i++ ) {
            g_free ( rmpd->entry_list[i].exec );
        }
        g_free ( rmpd->entry_list );
        g_free ( rmpd );
        sw->private_data = NULL;
    }
}

static const char *mgrv ( unsigned int selected_line, void *sw, int *state )
{
    *state |= MARKUP;
    return drun_mode_get_data ( NULL, sw )[selected_line];
}

Switcher drun_mode =
{
    .name         = "drun",
    .keycfg       = NULL,
    .keystr       = NULL,
    .modmask      = AnyModifier,
    .init         = drun_mode_init,
    .get_data     = drun_mode_get_data,
    .result       = drun_mode_result,
    .destroy      = drun_mode_destroy,
    .token_match  = token_match,
    .mgrv         = mgrv,
    .private_data = NULL,
    .free         = NULL
};
