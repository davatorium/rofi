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

#define _GNU_SOURCE
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
    char **args = malloc ( sizeof ( char* ) * 7 );
    int  i      = 0;
    args[i++] = config.terminal_emulator;
    if ( config.ssh_set_title ) {
        char *buffer = NULL;
        if ( asprintf ( &buffer, "ssh %s", host ) > 0 ) {
            args[i++] = strdup ( "-T" );
            args[i++] = buffer;
        }
    }
    args[i++] = strdup ( "-e" );
    args[i++] = strdup ( "ssh" );
    args[i++] = strdup ( host );
    args[i++] = NULL;
    int retv = execvp ( config.terminal_emulator, (char * const *) args );

    // Free the args list.
    for ( i = 0; i < 7; i++ ) {
        if ( args[i] != NULL ) {
            free ( args[i] );
        }
    }
    free ( args );
    return retv;
}
// execute sub-process
static pid_t exec_ssh ( const char *cmd )
{
    if ( !cmd || !cmd[0] ) {
        return -1;
    }

    signal ( SIGCHLD, catch_exit );
    pid_t pid = fork ();

    if ( !pid ) {
        setsid ();
        execshssh ( cmd );
        exit ( EXIT_FAILURE );
    }

    /**
     * This happens in non-critical time (After launching app)
     * It is allowed to be a bit slower.
     */
    char *path = NULL;
    if ( asprintf ( &path, "%s/%s", cache_dir, SSH_CACHE_FILE ) > 0 ) {
        history_set ( path, cmd );
        free ( path );
    }
    return pid;
}
static void delete_ssh ( const char *cmd )
{
    if ( !cmd || !cmd[0] ) {
        return;
    }
    char *path = NULL;
    if ( asprintf ( &path, "%s/%s", cache_dir, SSH_CACHE_FILE ) > 0 ) {
        history_remove ( path, cmd );
        free ( path );
    }
}
static int sort_func ( const void *a, const void *b )
{
    const char *astr = *( const char * const * ) a;
    const char *bstr = *( const char * const * ) b;
    return strcasecmp ( astr, bstr );
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

    if ( asprintf ( &path, "%s/%s", cache_dir, SSH_CACHE_FILE ) > 0 ) {
        retv = history_get_list ( path, length );
        free ( path );
        num_favorites = ( *length );
    }


    FILE       *fd = NULL;
    const char *hd = getenv ( "HOME" );
    if ( asprintf ( &path, "%s/%s", hd, ".ssh/config" ) >= 0 ) {
        fd = fopen ( path, "r" );
    }

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

                char **tr = realloc ( retv, ( ( *length ) + 2 ) * sizeof ( char* ) );
                if ( tr != NULL ) {
                    retv                  = tr;
                    retv[( *length )]     = strndup ( &buffer[start], stop - start );
                    retv[( *length ) + 1] = NULL;
                    ( *length )++;
                }
            }
        }

        fclose ( fd );
    }

    // TODO: check this is still fast enough. (takes 1ms on laptop.)
    if ( ( *length ) > num_favorites ) {
        qsort ( &retv[num_favorites], ( *length ) - num_favorites, sizeof ( char* ), sort_func );
    }
    free ( path );
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

SwitcherMode ssh_switcher_dialog ( char **input, void *data )
{
    SwitcherMode retv = MODE_EXIT;
    // act as a launcher
    unsigned int cmd_list_length = 0;
    char         **cmd_list      = get_ssh ( &cmd_list_length );

    if ( cmd_list == NULL ) {
        cmd_list    = malloc ( 2 * sizeof ( char * ) );
        cmd_list[0] = strdup ( "No ssh hosts found" );
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

    for ( int i = 0; cmd_list[i] != NULL; i++ ) {
        free ( cmd_list[i] );
    }

    if ( cmd_list != NULL ) {
        free ( cmd_list );
    }

    return retv;
}
