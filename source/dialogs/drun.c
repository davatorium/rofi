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
/**
 * Internal spider used to get list of executables.
 */
static char** get_apps ( unsigned int *length )
{
    char **retv        = NULL;

    const char * const * dr = g_get_system_data_dirs();
    while(dr != NULL && *dr != NULL ) {
        gchar *bp = g_build_filename(*dr, "applications", NULL);
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
                gchar *path = g_build_filename(bp, dent->d_name, NULL);
                GKeyFile *kf = g_key_file_new();
                GError *error = NULL;
                // TODO: check what flags to set;
                g_key_file_load_from_file(kf, path, 0, NULL);
                if ( error == NULL ) {
                    retv                  = g_realloc ( retv, ( ( *length ) + 2 ) * sizeof ( *retv ) );
                    if( g_key_file_has_key(kf, "Desktop Entry", "Name", NULL) ) {
                        gchar *n = NULL;
                        gchar *gn = NULL;
                        n = g_key_file_get_string(kf, "Desktop Entry", "Name", NULL); 
                        gn = g_key_file_get_string(kf, "Desktop Entry", "GenericName", NULL); 
                        retv[( *length )]     = g_strdup_printf("%-30s\t%s", n, gn?gn:""); 
                        g_free(n); g_free(gn);
                    } else { 
                        retv[(*length)] = g_strdup(dent->d_name);
                    }
                    retv[( *length ) + 1] = NULL;
                    ( *length )++;
                } else {
                    g_error_free(error);
                }
                g_key_file_free(kf);
                g_free(path);
            }

            closedir ( dir );
        }
        dr++;
        g_free(bp);
    }

    // No sorting needed.
    if ( ( *length ) == 0 ) {
        return retv;
    }

    return retv;
}

typedef struct _DRunModePrivateData
{
    unsigned int id;
    char **cmd_list;
    unsigned int cmd_list_length;
} DRunModePrivateData;

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
        rmpd->cmd_list        = get_apps ( &( rmpd->cmd_list_length ) );
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
    SwitcherMode       retv  = MODE_EXIT;

    int                shift = ( ( mretv & MENU_SHIFT ) == MENU_SHIFT );

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
        exec_cmd ( rmpd->cmd_list[selected_line], shift );
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
        g_strfreev(rmpd->cmd_list); 
        g_free ( rmpd );
        sw->private_data = NULL;
    }
}

static const char *mgrv ( unsigned int selected_line, void *sw, G_GNUC_UNUSED int *state )
{
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
