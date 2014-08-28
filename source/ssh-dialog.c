/**
 * rofi
 *
 * MIT/X11 License
 * Copyright 2013-2014 Qball  Cow <qball@gmpclient.org>
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
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "rofi.h"
#include "history.h"
#include "ssh-dialog.h"
#ifdef TIMING
#include <time.h>
#endif

#define SSH_CACHE_FILE    "rofi-2.sshcache"

static inline int execshssh ( const char *host )
{
    /**
     * I am not happy about this code, it causes 7 mallocs and frees
     */
    char **args = g_malloc_n ( 7, sizeof ( char* ) );
    int  i      = 0;
    args[i++] = g_strdup ( config.terminal_emulator );
    if ( config.ssh_set_title ) {
        args[i++] = g_strdup ( "-T" );
        args[i++] = g_strdup_printf ( "ssh %s", host );
    }
    args[i++] = g_strdup ( "-e" );
    args[i++] = g_strdup ( "ssh" );
    args[i++] = g_strdup ( host );
    args[i++] = NULL;

    GError *error = NULL;
    g_spawn_async ( NULL, args, NULL,
                    G_SPAWN_SEARCH_PATH,
                    NULL, NULL, NULL, &error );

    if( error != NULL )
    {
        char *msg = g_strdup_printf("Failed to execute: 'ssh %s'\nError: '%s'", host,
                error->message);
        error_dialog(msg);
        g_free(msg);
        // print error.
        g_error_free(error);
    }
    // Free the args list.
    g_strfreev ( args );

    return 0;
}
// execute sub-process
static pid_t exec_ssh ( const char *cmd )
{
    if ( !cmd || !cmd[0] ) {
        return -1;
    }

    execshssh ( cmd );

    /**
     * This happens in non-critical time (After launching app)
     * It is allowed to be a bit slower.
     */
    char *path = g_strdup_printf ( "%s/%s", cache_dir, SSH_CACHE_FILE );
    history_set ( path, cmd );
    g_free ( path );

    return 0;
}
static void delete_ssh ( const char *cmd )
{
    if ( !cmd || !cmd[0] ) {
        return;
    }
    char *path = NULL;
    path = g_strdup_printf ( "%s/%s", cache_dir, SSH_CACHE_FILE );
    history_remove ( path, cmd );
    g_free ( path );
}
static int ssh_sort_func ( const void *a, const void *b )
{
    const char *astr = *( const char * const * ) a;
    const char *bstr = *( const char * const * ) b;
    return g_utf8_collate ( astr, bstr );
}
static char ** get_ssh ( unsigned int *length )
{
    unsigned int    num_favorites = 0;
    char            *path;
    char            **retv = NULL;
#ifdef TIMING
    struct timespec start, stop;
    clock_gettime ( CLOCK_REALTIME, &start );
#endif

    if ( getenv ( "HOME" ) == NULL ) {
        return NULL;
    }

    path = g_strdup_printf ( "%s/%s", cache_dir, SSH_CACHE_FILE );
    retv = history_get_list ( path, length );
    g_free ( path );
    num_favorites = ( *length );

    FILE       *fd = NULL;
    const char *hd = getenv ( "HOME" );
    path = g_strdup_printf ( "%s/%s", hd, ".ssh/config" );
    fd   = fopen ( path, "r" );

    if ( fd != NULL ) {
        char buffer[1024];
        while ( fgets ( buffer, 1024, fd ) != NULL ) {
            if ( strncasecmp ( buffer, "Host", 4 ) == 0 ) {
                int start = 0, stop = 0;
                buffer[strlen ( buffer ) - 1] = '\0';

                for ( start = 4; isspace ( buffer[start] ); start++ ) {
                    ;
                }

                for ( stop = start; isalnum ( buffer[stop] ) ||
                      buffer[stop] == '_' ||
                      buffer[stop] == '.'; stop++ ) {
                    ;
                }

                int found = 0;

                if ( start == stop ) {
                    continue;
                }

                // This is a nice little penalty, but doable? time will tell.
                // given num_favorites is max 25.
                for ( unsigned int j = 0; found == 0 && j < num_favorites; j++ ) {
                    if ( strncasecmp ( &buffer[start], retv[j], stop - start ) == 0 ) {
                        found = 1;
                    }
                }

                if ( found == 1 ) {
                    continue;
                }

                retv                  = g_realloc ( retv, ( ( *length ) + 2 ) * sizeof ( char* ) );
                retv[( *length )]     = strndup ( &buffer[start], stop - start );
                retv[( *length ) + 1] = NULL;
                ( *length )++;
            }
        }

        fclose ( fd );
    }

    // TODO: check this is still fast enough. (takes 1ms on laptop.)
    if ( ( *length ) > num_favorites ) {
        qsort ( &retv[num_favorites], ( *length ) - num_favorites, sizeof ( char* ), ssh_sort_func );
    }
    g_free ( path );
#ifdef TIMING
    clock_gettime ( CLOCK_REALTIME, &stop );

    if ( stop.tv_sec != start.tv_sec ) {
        stop.tv_nsec += ( stop.tv_sec - start.tv_sec ) * 1e9;
    }

    long diff = stop.tv_nsec - start.tv_nsec;
    printf ( "Time elapsed: %ld us\n", diff / 1000 );
#endif
    return retv;
}

SwitcherMode ssh_switcher_dialog ( char **input, G_GNUC_UNUSED void *data )
{
    SwitcherMode retv = MODE_EXIT;
    // act as a launcher
    unsigned int cmd_list_length = 0;
    char         **cmd_list      = get_ssh ( &cmd_list_length );

    if ( cmd_list == NULL ) {
        cmd_list    = g_malloc_n ( 2, sizeof ( char * ) );
        cmd_list[0] = g_strdup ( "No ssh hosts found" );
        cmd_list[1] = NULL;
    }

    int shift         = 0;
    int selected_line = 0;
    int mretv         = menu ( cmd_list, cmd_list_length, input, "ssh:",
                               NULL, &shift, token_match, NULL, &selected_line, config.levenshtein_sort );

    if ( mretv == MENU_NEXT ) {
        retv = NEXT_DIALOG;
    }
    else if ( mretv == MENU_QUICK_SWITCH ) {
        retv = selected_line;
    }
    else if ( mretv == MENU_OK && cmd_list[selected_line] != NULL ) {
        exec_ssh ( cmd_list[selected_line] );
    }
    else if ( mretv == MENU_CUSTOM_INPUT && *input != NULL && *input[0] != '\0' ) {
        exec_ssh ( *input );
    }
    else if ( mretv == MENU_ENTRY_DELETE && cmd_list[selected_line] ) {
        delete_ssh ( cmd_list[selected_line] );
        // Stay
        retv = RELOAD_DIALOG;
    }

    g_strfreev ( cmd_list );

    return retv;
}
