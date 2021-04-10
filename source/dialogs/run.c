/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2020 Qball Cow <qball@gmpclient.org>
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

/**
 * \ingroup RUNMode
 * @{
 */

/** The log domain of this dialog. */
#define G_LOG_DOMAIN    "Dialogs.Run"

#include <config.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>
#include <strings.h>
#include <string.h>
#include <errno.h>

#include "rofi.h"
#include "settings.h"
#include "helper.h"
#include "history.h"
#include "dialogs/run.h"

#include "mode-private.h"

#include "timings.h"
#include "rofi-icon-fetcher.h"
/**
 * Name of the history file where previously chosen commands are stored.
 */
#define RUN_CACHE_FILE    "rofi-3.runcache"

/**
 * The internal data structure holding the private data of the Run Mode.
 */
typedef struct
{
    /** list of available commands. */
    char         **cmd_list;
    /** Length of the #cmd_list. */
    unsigned int cmd_list_length;
} RunModePrivateData;

/**
 * @param cmd The cmd to execute
 * @param run_in_term Indicate if command should be run in a terminal
 *
 * Execute command and add to history.
 */
static gboolean exec_cmd ( const char *cmd, int run_in_term )
{
    GError *error = NULL;
    if ( !cmd || !cmd[0] ) {
        return FALSE;
    }
    gsize lf_cmd_size = 0;
    gchar *lf_cmd     = g_locale_from_utf8 ( cmd, -1, NULL, &lf_cmd_size, &error );
    if ( error != NULL ) {
        g_warning ( "Failed to convert command to locale encoding: %s", error->message );
        g_error_free ( error );
        return FALSE;
    }

    char                     *path   = g_build_filename ( cache_dir, RUN_CACHE_FILE, NULL );
    RofiHelperExecuteContext context = { .name = NULL };
    // FIXME: assume startup notification support for terminals
    if (  helper_execute_command ( NULL, lf_cmd, run_in_term, run_in_term ? &context : NULL ) ) {
        /**
         * This happens in non-critical time (After launching app)
         * It is allowed to be a bit slower.
         */

        history_set ( path, cmd );
        g_free ( path );
        g_free ( lf_cmd );
        return TRUE;
    }
    else {
        history_remove ( path, cmd );
        g_free ( path );
        g_free ( lf_cmd );
        return FALSE;
    }
}

/**
 * @param cmd The command to remove from history
 *
 * Remove command from history.
 */
static void delete_entry ( const char *cmd )
{
    char *path = g_build_filename ( cache_dir, RUN_CACHE_FILE, NULL );

    history_remove ( path, cmd );

    g_free ( path );
}

/**
 * @param a The First key to compare
 * @param b The second key to compare
 * @param data Unused.
 *
 * Function used for sorting.
 *
 * @returns returns less then, equal to and greater than zero is a is less than, is a match or greater than b.
 */
static int sort_func ( const void *a, const void *b, G_GNUC_UNUSED void *data )
{
    const char *astr = *( const char * const * ) a;
    const char *bstr = *( const char * const * ) b;

    if ( astr == NULL && bstr == NULL ) {
        return 0;
    }
    else if ( astr == NULL ) {
        return 1;
    }
    else if ( bstr == NULL ) {
        return -1;
    }
    return g_strcmp0 ( astr, bstr );
}

/**
 * External spider to get list of executables.
 */
static char ** get_apps_external ( char **retv, unsigned int *length, unsigned int num_favorites )
{
    int fd = execute_generator ( config.run_list_command );
    if ( fd >= 0 ) {
        FILE *inp = fdopen ( fd, "r" );
        if ( inp ) {
            char   *buffer       = NULL;
            size_t buffer_length = 0;

            while ( getline ( &buffer, &buffer_length, inp ) > 0 ) {
                int found = 0;
                // Filter out line-end.
                if ( buffer[strlen ( buffer ) - 1] == '\n' ) {
                    buffer[strlen ( buffer ) - 1] = '\0';
                }

                // This is a nice little penalty, but doable? time will tell.
                // given num_favorites is max 25.
                for ( unsigned int j = 0; found == 0 && j < num_favorites; j++ ) {
                    if ( strcasecmp ( buffer, retv[j] ) == 0 ) {
                        found = 1;
                    }
                }

                if ( found == 1 ) {
                    continue;
                }

                // No duplicate, add it.
                retv              = g_realloc ( retv, ( ( *length ) + 2 ) * sizeof ( char* ) );
                retv[( *length )] = g_strdup ( buffer );

                ( *length )++;
            }
            if ( buffer != NULL ) {
                free ( buffer );
            }
            if ( fclose ( inp ) != 0 ) {
                g_warning ( "Failed to close stdout off executor script: '%s'",
                            g_strerror ( errno ) );
            }
        }
    }
    retv[( *length ) ] = NULL;
    return retv;
}

/**
 * Internal spider used to get list of executables.
 */
static char ** get_apps ( unsigned int *length )
{
    GError       *error        = NULL;
    char         **retv        = NULL;
    unsigned int num_favorites = 0;
    char         *path;

    if ( g_getenv ( "PATH" ) == NULL ) {
        return NULL;
    }
    TICK_N ( "start" );
    path = g_build_filename ( cache_dir, RUN_CACHE_FILE, NULL );
    retv = history_get_list ( path, length );
    g_free ( path );
    // Keep track of how many where loaded as favorite.
    num_favorites = ( *length );

    path = g_strdup ( g_getenv ( "PATH" ) );

    gsize l        = 0;
    gchar *homedir = g_locale_to_utf8 (  g_get_home_dir (), -1, NULL, &l, &error );
    if ( error != NULL ) {
        g_debug ( "Failed to convert homedir to UTF-8: %s", error->message );
        g_clear_error ( &error );
        g_free ( homedir );
        return NULL;
    }

    const char *const sep                 = ":";
    char              *strtok_savepointer = NULL;
    for ( const char *dirname = strtok_r ( path, sep, &strtok_savepointer ); dirname != NULL; dirname = strtok_r ( NULL, sep, &strtok_savepointer ) ) {
        char *fpath = rofi_expand_path ( dirname );
        DIR  *dir   = opendir ( fpath );
        g_debug ( "Checking path %s for executable.", fpath );
        g_free ( fpath );

        if ( dir != NULL ) {
            struct dirent *dent;
            gsize         dirn_len = 0;
            gchar         *dirn    = g_locale_to_utf8 ( dirname, -1, NULL, &dirn_len, &error );
            if ( error != NULL ) {
                g_debug ( "Failed to convert directory name to UTF-8: %s", error->message );
                g_clear_error ( &error );
                closedir ( dir );
                continue;
            }
            gboolean is_homedir = g_str_has_prefix ( dirn, homedir );
            g_free ( dirn );

            while ( ( dent = readdir ( dir ) ) != NULL ) {
                if ( dent->d_type != DT_REG && dent->d_type != DT_LNK && dent->d_type != DT_UNKNOWN ) {
                    continue;
                }
                // Skip dot files.
                if ( dent->d_name[0] == '.' ) {
                    continue;
                }
                if ( is_homedir ) {
                    gchar    *fpath = g_build_filename ( dirname, dent->d_name, NULL );
                    gboolean b      = g_file_test ( fpath, G_FILE_TEST_IS_EXECUTABLE );
                    g_free ( fpath );
                    if ( !b ) {
                        continue;
                    }
                }

                gsize name_len;
                gchar *name = g_filename_to_utf8 ( dent->d_name, -1, NULL, &name_len, &error );
                if ( error != NULL ) {
                    g_debug ( "Failed to convert filename to UTF-8: %s", error->message );
                    g_clear_error ( &error );
                    g_free ( name );
                    continue;
                }
                // This is a nice little penalty, but doable? time will tell.
                // given num_favorites is max 25.
                int found = 0;
                for ( unsigned int j = 0; found == 0 && j < num_favorites; j++ ) {
                    if ( g_strcmp0 ( name, retv[j] ) == 0 ) {
                        found = 1;
                    }
                }

                if ( found == 1 ) {
                    g_free ( name );
                    continue;
                }

                retv                  = g_realloc ( retv, ( ( *length ) + 2 ) * sizeof ( char* ) );
                retv[( *length )]     = name;
                retv[( *length ) + 1] = NULL;
                ( *length )++;
            }

            closedir ( dir );
        }
    }
    g_free ( homedir );

    // Get external apps.
    if ( config.run_list_command != NULL && config.run_list_command[0] != '\0' ) {
        retv = get_apps_external ( retv, length, num_favorites );
    }
    // No sorting needed.
    if ( ( *length ) == 0 ) {
        return retv;
    }
    // TODO: check this is still fast enough. (takes 1ms on laptop.)
    if ( ( *length ) > num_favorites ) {
        g_qsort_with_data ( &retv[num_favorites], ( *length ) - num_favorites, sizeof ( char* ), sort_func, NULL );
    }
    g_free ( path );

    unsigned int removed = 0;
    for ( unsigned int index = num_favorites; index < ( ( *length ) - 1 ); index++ ) {
        if ( g_strcmp0 ( retv[index], retv[index + 1] ) == 0 ) {
            g_free ( retv[index] );
            retv[index] = NULL;
            removed++;
        }
    }

    if ( ( *length ) > num_favorites ) {
        g_qsort_with_data ( &retv[num_favorites], ( *length ) - num_favorites, sizeof ( char* ),
                            sort_func,
                            NULL );
    }
    // Reduce array length;
    ( *length ) -= removed;

    TICK_N ( "stop" );
    return retv;
}

static int run_mode_init ( Mode *sw )
{
    if ( sw->private_data == NULL ) {
        RunModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
        sw->private_data = (void *) pd;
        pd->cmd_list     = get_apps ( &( pd->cmd_list_length ) );
    }

    return TRUE;
}
static void run_mode_destroy ( Mode *sw )
{
    RunModePrivateData *rmpd = (RunModePrivateData *) sw->private_data;
    if ( rmpd != NULL ) {
        g_strfreev ( rmpd->cmd_list );
        g_free ( rmpd );
        sw->private_data = NULL;
    }
}

static unsigned int run_mode_get_num_entries ( const Mode *sw )
{
    const RunModePrivateData *rmpd = (const RunModePrivateData *) sw->private_data;
    return rmpd->cmd_list_length;
}

static ModeMode run_mode_result ( Mode *sw, int mretv, char **input, unsigned int selected_line )
{
    RunModePrivateData *rmpd = (RunModePrivateData *) sw->private_data;
    ModeMode           retv  = MODE_EXIT;

    gboolean           run_in_term = ( ( mretv & MENU_CUSTOM_ACTION ) == MENU_CUSTOM_ACTION );

    if ( ( mretv & MENU_OK ) && rmpd->cmd_list[selected_line] != NULL ) {
        if ( !exec_cmd ( rmpd->cmd_list[selected_line], run_in_term ) ) {
            retv = RELOAD_DIALOG;
        }
    }
    else if ( ( mretv & MENU_CUSTOM_INPUT ) && *input != NULL && *input[0] != '\0' ) {
        if ( !exec_cmd ( *input, run_in_term ) ) {
            retv = RELOAD_DIALOG;
        }
    }
    else if ( ( mretv & MENU_ENTRY_DELETE ) && rmpd->cmd_list[selected_line] ) {
        delete_entry ( rmpd->cmd_list[selected_line] );

        // Clear the list.
        retv = RELOAD_DIALOG;
        run_mode_destroy ( sw );
        run_mode_init ( sw );

    } else if ( mretv & MENU_CUSTOM_COMMAND ) {
        retv = ( mretv & MENU_LOWER_MASK );
    }
    return retv;
}

static char *_get_display_value ( const Mode *sw, unsigned int selected_line, G_GNUC_UNUSED int *state, G_GNUC_UNUSED GList **list, int get_entry )
{
    const RunModePrivateData *rmpd = (const RunModePrivateData *) sw->private_data;
    return get_entry ? g_strdup ( rmpd->cmd_list[selected_line] ) : NULL;
}

static int run_token_match ( const Mode *sw, rofi_int_matcher **tokens, unsigned int index )
{
    const RunModePrivateData *rmpd = (const RunModePrivateData *) sw->private_data;
    return helper_token_match ( tokens, rmpd->cmd_list[index] );
}

#include "mode-private.h"
Mode run_mode =
{
    .name               = "run",
    .cfg_name_key       = "display-run",
    ._init              = run_mode_init,
    ._get_num_entries   = run_mode_get_num_entries,
    ._result            = run_mode_result,
    ._destroy           = run_mode_destroy,
    ._token_match       = run_token_match,
    ._get_display_value = _get_display_value,
    ._get_icon          = NULL,
    ._get_completion    = NULL,
    ._preprocess_input  = NULL,
    .private_data       = NULL,
    .free               = NULL
};
/** @}*/
