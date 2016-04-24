/*
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

/**
 * \ingroup RUNMode
 * @{
 */
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
/**
 * Name of the history file where previously choosen commands are stored.
 */
#define RUN_CACHE_FILE    "rofi-3.runcache"

#include <dialogs/fileb.h>
/**
 * The internal data structure holding the private data of the Run Mode.
 */
typedef enum
{
    SELECT_BINARY,
    SELECT_FILE
}RunModeMode;
typedef struct
{
    /* Mode */
    RunModeMode  mode;
    unsigned int row_selected;
    /** list of available commands. */
    char         **cmd_list;
    /** Length of the #cmd_list. */
    unsigned int cmd_list_length;

    Mode         filec;
    /** Mode 2 */
    char         **file_list;
    unsigned int file_list_length;
} RunModePrivateData;

/**
 * @param cmd The cmd to execute
 * @param run_in_term Indicate if command should be run in a terminal
 *
 * Execute command.
 *
 * @returns FALSE On failure, TRUE on success
 */
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

/**
 * @param cmd The cmd to execute
 * @param run_in_term Indicate if command should be run in a terminal
 *
 * Execute command and add to history.
 */
static void exec_cmd ( const char *cmd, int run_in_term )
{
    GError *error = NULL;
    if ( !cmd || !cmd[0] ) {
        return;
    }
    gsize lf_cmd_size = 0;
    gchar *lf_cmd     = g_locale_from_utf8 ( cmd, -1, NULL, &lf_cmd_size, &error );
    if ( error != NULL ) {
        fprintf ( stderr, "Failed to convert command to locale encoding: %s\n", error->message );
        g_error_free ( error );
        return;
    }

    if (  execsh ( lf_cmd, run_in_term ) ) {
        /**
         * This happens in non-critical time (After launching app)
         * It is allowed to be a bit slower.
         */
        char *path = g_build_filename ( cache_dir, RUN_CACHE_FILE, NULL );

        history_set ( path, cmd );

        g_free ( path );
    }
    g_free ( lf_cmd );
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
 *
 * Function used for sorting.
 *
 * @returns returns less then, equal to and greater than zero is a is less than, is a match or greater than b.
 */
static int sort_func ( const void *a, const void *b, void *data __attribute__( ( unused ) ) )
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
    return g_ascii_strcasecmp ( astr, bstr );
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
                fprintf ( stderr, "Failed to close stdout off executor script: '%s'\n",
                          strerror ( errno ) );
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
        fprintf ( stderr, "Failed to convert homedir to UTF-8: %s\n", error->message );
        g_clear_error ( &error );
        g_free ( homedir );
        return NULL;
    }

    const char *const sep = ":";
    for ( const char *dirname = strtok ( path, sep ); dirname != NULL; dirname = strtok ( NULL, sep ) ) {
        DIR *dir = opendir ( dirname );

        if ( dir != NULL ) {
            struct dirent *dent;
            gsize         dirn_len = 0;
            gchar         *dirn    = g_locale_to_utf8 ( dirname, -1, NULL, &dirn_len, &error );
            if ( error != NULL ) {
                fprintf ( stderr, "Failed to convert directory name to UTF-8: %s\n", error->message );
                g_clear_error ( &error );
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
                    fprintf ( stderr, "Failed to convert filename to UTF-8: %s\n", error->message );
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
        pd->mode         = SELECT_BINARY;
        pd->row_selected = UINT32_MAX;
        pd->cmd_list     = get_apps ( &( pd->cmd_list_length ) );
        pd->filec        = fileb_mode;

        mode_init ( &pd->filec );
    }

    return TRUE;
}
static void run_mode_destroy ( Mode *sw )
{
    RunModePrivateData *rmpd = (RunModePrivateData *) sw->private_data;
    if ( rmpd != NULL ) {
        g_strfreev ( rmpd->file_list );
        g_strfreev ( rmpd->cmd_list );
        g_free ( rmpd );

        mode_destroy ( &rmpd->filec );
        sw->private_data = NULL;
    }
}

static unsigned int run_mode_get_num_entries ( const Mode *sw )
{
    const RunModePrivateData *rmpd = (const RunModePrivateData *) sw->private_data;
    if ( rmpd->mode == SELECT_FILE ) {
        return mode_get_num_entries ( &( rmpd->filec ) );
    }
    return rmpd->cmd_list_length;
}

static ModeMode run_mode_result ( Mode *sw, int mretv, char **input, unsigned int selected_line )
{
    RunModePrivateData *rmpd = (RunModePrivateData *) sw->private_data;
    ModeMode           retv  = MODE_EXIT;

    gboolean           run_in_term = ( ( mretv & MENU_CUSTOM_ACTION ) == MENU_CUSTOM_ACTION );

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
        exec_cmd ( rmpd->cmd_list[selected_line], run_in_term );
    }
    else if ( ( mretv & MENU_CUSTOM_INPUT ) && *input != NULL && *input[0] != '\0' ) {
        exec_cmd ( *input, run_in_term );
    }
    else if ( ( mretv & MENU_ENTRY_DELETE ) && rmpd->cmd_list[selected_line] ) {
        delete_entry ( rmpd->cmd_list[selected_line] );

        // Clear the list.
        retv = RELOAD_DIALOG;
        run_mode_destroy ( sw );
        run_mode_init ( sw );
    }
    return retv;
}

static char *_get_display_value ( const Mode *sw, unsigned int selected_line, G_GNUC_UNUSED int *state, int get_entry )
{
    const RunModePrivateData *rmpd = (const RunModePrivateData *) sw->private_data;
    if ( rmpd->mode == SELECT_FILE ) {
        if ( !get_entry ) {
            return NULL;
        }
        char *rv = mode_get_display_value ( &( rmpd->filec ), selected_line, state, get_entry );
        return rv;
    }
    return get_entry ? g_strdup ( rmpd->cmd_list[selected_line] ) : NULL;
}
static int run_token_match ( const Mode *sw, char **tokens, int not_ascii, int case_sensitive, unsigned int index )
{
    const RunModePrivateData *rmpd = (const RunModePrivateData *) sw->private_data;
    if ( rmpd->mode == SELECT_FILE ) {
        return mode_token_match ( &( rmpd->filec ), &( tokens[1] ), not_ascii, case_sensitive, index );
    }
    return token_match ( tokens, rmpd->cmd_list[index], not_ascii, case_sensitive );
}

static int run_is_not_ascii ( const Mode *sw, unsigned int index )
{
    const RunModePrivateData *rmpd = (const RunModePrivateData *) sw->private_data;
    if ( rmpd->mode == SELECT_FILE ) {
        return mode_is_not_ascii ( &( rmpd->filec ), index );
    }
    return !g_str_is_ascii ( rmpd->cmd_list[index] );
}

static int _update_result ( Mode *sw, const char *input, unsigned int selected )
{
    RunModePrivateData *rmpd = (RunModePrivateData *) sw->private_data;
    if ( rmpd->mode == SELECT_BINARY ) {
        rmpd->row_selected = selected;
        if ( selected == UINT32_MAX ) {
            return FALSE;
        }
        if ( g_strcmp0 ( input, rmpd->cmd_list[selected] ) == 0 ) {
            printf ( "Switch to 'select file'\n" );
            rmpd->mode = SELECT_FILE;
            return TRUE;
        }
    }
    else if ( rmpd->mode == SELECT_FILE ) {
        if ( !g_str_has_prefix ( input, rmpd->cmd_list[rmpd->row_selected] ) ) {
            printf ( "Switch to 'select binary'\n" );
            rmpd->mode = SELECT_BINARY;
            return TRUE;
        }
        ssize_t l = strlen ( rmpd->cmd_list[rmpd->row_selected] );
        if ( l < strlen ( input ) ) {
            return mode_update_result ( &( rmpd->filec ), &input[strlen ( rmpd->cmd_list[rmpd->row_selected] ) + 1], selected );
        }
    }
    return FALSE;
}
static char *_get_completion ( const Mode *sw, unsigned int index )
{
    RunModePrivateData *rmpd = (RunModePrivateData *) mode_get_private_data ( sw );
    if ( rmpd->mode == SELECT_FILE ) {
        char *r    = mode_get_completion ( &( rmpd->filec ), index );
        char *retv = g_strdup_printf ( "%s %s", rmpd->cmd_list[rmpd->row_selected], r );
        g_free ( r );
        return retv;
    }
    return g_strdup ( rmpd->cmd_list[index] );
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
    ._get_completion    = _get_completion,
    ._is_not_ascii      = run_is_not_ascii,
    ._update_result     = _update_result,
    .private_data       = NULL,
    .free               = NULL
};
/*@}*/
