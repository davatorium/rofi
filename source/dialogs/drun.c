/**
 * rofi
 *
 * MIT/X11 License
 * Copyright 2013-2016 Qball Cow <qball@gmpclient.org>
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

#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <strings.h>
#include <string.h>
#include <errno.h>

#include "rofi.h"
#include "settings.h"
#include "helper.h"
#include "textbox.h"
#include "history.h"
#include "dialogs/drun.h"

#define DRUN_CACHE_FILE    "rofi.druncache"

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
        rofi_view_error_dialog ( msg, FALSE  );
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
 * Store extra information about the entry.
 * Currently the executable and if it should run in terminal.
 */
typedef struct
{
    /* Path to desktop file */
    char         *path;
    /* Executable */
    char         *exec;
    /* Name of the Entry */
    char         *name;
    /* Generic Name */
    char         *generic_name;
    /* Application needs to be launched in terminal. */
    unsigned int terminal;
} DRunModeEntry;

typedef struct
{
    DRunModeEntry *entry_list;
    unsigned int  cmd_list_length;
    unsigned int  history_length;
} DRunModePrivateData;

static void exec_cmd_entry ( DRunModeEntry *e )
{
    // strip % arguments
    gchar *str = g_strdup ( e->exec );
    for ( ssize_t i = 0; str != NULL && str[i] != '\0'; i++ ) {
        if ( str[i] == '%' ) {
            while ( str[i] != ' ' && str[i] != '\0' ) {
                str[i++] = ' ';
            }
            // We might have hit '\0' in prev. loop, break out of for then.
            if ( str[i] == '\0' ) {
                break;
            }
        }
    }
    gchar *fp = rofi_expand_path ( g_strstrip ( str ) );
    if ( execsh ( fp, e->terminal ) ) {
        char *path = g_build_filename ( cache_dir, DRUN_CACHE_FILE, NULL );
        history_set ( path, e->path );
        g_free ( path );
    }
    g_free ( str );
    g_free ( fp );
}
/**
 * This function absorbs/freeś path, so this is no longer available afterwards.
 */
static void read_desktop_file ( DRunModePrivateData *pd, char *path, const char *filename )
{
    for ( unsigned int index = 0; index < pd->history_length; index++ ) {
        if ( g_strcmp0 ( path, pd->entry_list[index].path ) == 0 ) {
            g_free ( path );
            return;
        }
    }
    GKeyFile *kf    = g_key_file_new ();
    GError   *error = NULL;
    g_key_file_load_from_file ( kf, path, 0, &error );
    // If error, skip to next entry
    if ( error != NULL ) {
        g_error_free ( error );
        g_key_file_free ( kf );
        g_free ( path );
        return;
    }
    // Skip hidden entries.
    if ( g_key_file_has_key ( kf, "Desktop Entry", "Hidden", NULL ) ) {
        if ( g_key_file_get_boolean ( kf, "Desktop Entry", "Hidden", NULL ) ) {
            g_key_file_free ( kf );
            g_free ( path );
            return;
        }
    }
    // Skip entries that have NoDisplay set.
    if ( g_key_file_has_key ( kf, "Desktop Entry", "NoDisplay", NULL ) ) {
        if ( g_key_file_get_boolean ( kf, "Desktop Entry", "NoDisplay", NULL ) ) {
            g_key_file_free ( kf );
            g_free ( path );
            return;
        }
    }
    if ( g_key_file_has_key ( kf, "Desktop Entry", "Exec", NULL ) ) {
        size_t nl = ( ( pd->cmd_list_length ) + 1 );
        pd->entry_list                               = g_realloc ( pd->entry_list, nl * sizeof ( *( pd->entry_list ) ) );
        pd->entry_list[pd->cmd_list_length].path     = path;
        pd->entry_list[pd->cmd_list_length].terminal = FALSE;
        if ( g_key_file_has_key ( kf, "Desktop Entry", "Name", NULL ) ) {
            gchar *n  = g_key_file_get_locale_string ( kf, "Desktop Entry", "Name", NULL, NULL );
            gchar *gn = g_key_file_get_locale_string ( kf, "Desktop Entry", "GenericName", NULL, NULL );
            pd->entry_list[pd->cmd_list_length].name         = n;
            pd->entry_list[pd->cmd_list_length].generic_name = gn;
        }
        else {
            pd->entry_list[pd->cmd_list_length].name         = g_filename_display_name ( filename );
            pd->entry_list[pd->cmd_list_length].generic_name = NULL;
        }
        pd->entry_list[pd->cmd_list_length].exec = g_key_file_get_string ( kf, "Desktop Entry", "Exec", NULL );
        if ( g_key_file_has_key ( kf, "Desktop Entry", "Terminal", NULL ) ) {
            pd->entry_list[pd->cmd_list_length].terminal = g_key_file_get_boolean ( kf, "Desktop Entry", "Terminal", NULL );
        }
        ( pd->cmd_list_length )++;
    }

    g_key_file_free ( kf );
}

/**
 * Internal spider used to get list of executables.
 */
static void walk_dir ( DRunModePrivateData *pd, const char *dirname )
{
    DIR *dir;

    dir = opendir ( dirname );
    if ( dir == NULL ) {
        return;
    }

    struct dirent *file;
    gchar         *filename = NULL;
    struct stat   st;
    while ( ( file = readdir ( dir ) ) != NULL ) {
        if ( file->d_name[0] == '.' ) {
            continue;
        }

        switch ( file->d_type )
        {
        case DT_LNK:
        case DT_REG:
        case DT_DIR:
        case DT_UKNOWN:
            filename = g_build_filename ( dirname, file->d_name, NULL );
            break;
        default:
            continue;
        }

        // On a link, or if FS does not support providing this information
        // Fallback to stat method.
        if ( file->d_type == DT_LNK || file->d_type == DT_UNKNOWN ) {
            file->d_type = DT_UNKNOWN;
            if ( stat ( filename, &st ) == 0 ) {
                if ( S_ISDIR ( st.st_mode ) ) {
                    file->d_type = DT_DIR;
                }
                else if ( S_ISREG ( st.st_mode ) ) {
                    file->d_type = DT_REG;
                }
            }
        }

        switch ( file->d_type )
        {
        case DT_REG:
            read_desktop_file ( pd, filename, file->d_name );
            filename = NULL;
            break;
        case DT_DIR:
            walk_dir ( pd, filename );
            break;
        default:
            break;
        }
        g_free ( filename );
    }
    closedir ( dir );
}
/**
 * @param entry The command entry to remove from history
 *
 * Remove command from history.
 */
static void delete_entry_history ( const DRunModeEntry *entry )
{
    char *path = g_build_filename ( cache_dir, DRUN_CACHE_FILE, NULL );

    history_remove ( path, entry->path );

    g_free ( path );
}

static void get_apps_history ( DRunModePrivateData *pd )
{
    unsigned int length = 0;
    gchar        *path  = g_build_filename ( cache_dir, DRUN_CACHE_FILE, NULL );
    gchar        **retv = history_get_list ( path, &length );
    g_free ( path );
    for ( unsigned int index = 0; index < length; index++ ) {
        gchar *name = g_path_get_basename ( retv[index] );
        read_desktop_file ( pd, retv[index], name );
        g_free ( name );
    }
    g_free ( retv );
    pd->history_length = pd->cmd_list_length;
}

static void get_apps ( DRunModePrivateData *pd )
{
    get_apps_history ( pd );

    gchar               *dir;
    const gchar * const * sys = g_get_system_data_dirs ();
    for (; *sys != NULL; ++sys ) {
        dir = g_build_filename ( *sys, "applications", NULL );
        walk_dir ( pd, dir );
        g_free ( dir );
    }
    dir = g_build_filename ( g_get_user_data_dir (), "applications", NULL );
    walk_dir ( pd, dir );
    g_free ( dir );
}

static int drun_mode_init ( Mode *sw )
{
    if ( mode_get_private_data ( sw ) == NULL ) {
        DRunModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
        mode_set_private_data ( sw, (void *) pd );
        get_apps ( pd );
    }
    return TRUE;
}
static void drun_entry_clear ( DRunModeEntry *e )
{
    g_free ( e->path );
    g_free ( e->exec );
    g_free ( e->name );
    g_free ( e->generic_name );
}

static ModeMode drun_mode_result ( Mode *sw, int mretv, char **input, unsigned int selected_line )
{
    DRunModePrivateData *rmpd = (DRunModePrivateData *) mode_get_private_data ( sw );
    ModeMode            retv  = MODE_EXIT;

    gboolean            run_in_term = ( ( mretv & MENU_CUSTOM_ACTION ) == MENU_CUSTOM_ACTION );

    if ( mretv & MENU_NEXT ) {
        retv = NEXT_DIALOG;
    }
    else if ( mretv & MENU_PREVIOUS ) {
        retv = PREVIOUS_DIALOG;
    }
    else if ( mretv & MENU_QUICK_SWITCH ) {
        retv = ( mretv & MENU_LOWER_MASK );
    }
    else if ( ( mretv & MENU_OK )  ) {
        exec_cmd_entry ( &( rmpd->entry_list[selected_line] ) );
    }
    else if ( ( mretv & MENU_CUSTOM_INPUT ) && *input != NULL && *input[0] != '\0' ) {
        exec_cmd ( *input, run_in_term );
    }
    else if ( ( mretv & MENU_ENTRY_DELETE ) && selected_line < rmpd->cmd_list_length ) {
        if ( selected_line < rmpd->history_length ) {
            delete_entry_history ( &( rmpd->entry_list[selected_line] ) );
            drun_entry_clear ( &( rmpd->entry_list[selected_line] ) );
            memmove ( &( rmpd->entry_list[selected_line] ), &rmpd->entry_list[selected_line + 1],
                      sizeof ( DRunModeEntry ) * ( rmpd->cmd_list_length - selected_line - 1 ) );
            rmpd->cmd_list_length--;
        }
        retv = RELOAD_DIALOG;
    }
    return retv;
}
static void drun_mode_destroy ( Mode *sw )
{
    DRunModePrivateData *rmpd = (DRunModePrivateData *) mode_get_private_data ( sw );
    if ( rmpd != NULL ) {
        for ( size_t i = 0; i < rmpd->cmd_list_length; i++ ) {
            drun_entry_clear ( &( rmpd->entry_list[i] ) );
        }
        g_free ( rmpd->entry_list );
        g_free ( rmpd );
        mode_set_private_data ( sw, NULL );
    }
}

static char *_get_display_value ( const Mode *sw, unsigned int selected_line, int *state, int get_entry )
{
    DRunModePrivateData *pd = (DRunModePrivateData *) mode_get_private_data ( sw );
    *state |= MARKUP;
    if ( !get_entry ) {
        return NULL;
    }
    if ( pd->entry_list == NULL ) {
        // Should never get here.
        return g_strdup ( "Failed" );
    }
    /* Free temp storage. */
    DRunModeEntry *dr = &( pd->entry_list[selected_line] );
    if ( dr->generic_name == NULL ) {
        return g_markup_escape_text ( dr->name, -1 );
    }
    else {
        return g_markup_printf_escaped ( "%s <span weight='light' size='small'><i>(%s)</i></span>", dr->name,
                                         dr->generic_name );
    }
}
static char *drun_get_completion ( const Mode *sw, unsigned int index )
{
    DRunModePrivateData *pd = (DRunModePrivateData *) mode_get_private_data ( sw );
    /* Free temp storage. */
    DRunModeEntry       *dr = &( pd->entry_list[index] );
    if ( dr->generic_name == NULL ) {
        return g_strdup ( dr->name );
    }
    else {
        return g_strdup_printf ( "%s", dr->name );
    }
}

static int drun_token_match ( const Mode *data,
                              char **tokens,
                              int not_ascii,
                              int case_sensitive,
                              unsigned int index
                              )
{
    DRunModePrivateData *rmpd = (DRunModePrivateData *) mode_get_private_data ( data );
    int                 match = 1;
    if ( tokens ) {
        for ( int j = 0; match && tokens != NULL && tokens[j] != NULL; j++ ) {
            int  test        = 0;
            char *ftokens[2] = { tokens[j], NULL };
            if ( !test && rmpd->entry_list[index].name &&
                 token_match ( ftokens, rmpd->entry_list[index].name, not_ascii, case_sensitive ) ) {
                test = 1;
            }
            if ( !test && rmpd->entry_list[index].generic_name &&
                 token_match ( ftokens, rmpd->entry_list[index].generic_name, not_ascii, case_sensitive ) ) {
                test = 1;
            }

            if ( !test && token_match ( ftokens, rmpd->entry_list[index].exec, not_ascii, case_sensitive ) ) {
                test = 1;
            }
            if ( test == 0 ) {
                match = 0;
            }
        }
    }
    return match;
}

static unsigned int drun_mode_get_num_entries ( const Mode *sw )
{
    const DRunModePrivateData *pd = (const DRunModePrivateData *) mode_get_private_data ( sw );
    return pd->cmd_list_length;
}
static int drun_is_not_ascii ( const Mode *sw, unsigned int index )
{
    DRunModePrivateData *pd = (DRunModePrivateData *) mode_get_private_data ( sw );
    if ( pd->entry_list[index].generic_name ) {
        return !g_str_is_ascii ( pd->entry_list[index].name ) || !g_str_is_ascii ( pd->entry_list[index].generic_name );
    }
    return !g_str_is_ascii ( pd->entry_list[index].name );
}

#include "mode-private.h"
Mode drun_mode =
{
    .name               = "drun",
    .cfg_name_key       = "display-drun",
    ._init              = drun_mode_init,
    ._get_num_entries   = drun_mode_get_num_entries,
    ._result            = drun_mode_result,
    ._destroy           = drun_mode_destroy,
    ._token_match       = drun_token_match,
    ._get_completion    = drun_get_completion,
    ._get_display_value = _get_display_value,
    ._is_not_ascii      = drun_is_not_ascii,
    .private_data       = NULL,
    .free               = NULL
};
