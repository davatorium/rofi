/**
 * simpleswitcher
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

#ifdef __QC_MODE__
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

#include "simpleswitcher.h"
#include "profile-dialog.h"
#ifdef TIMING
#include <time.h>
#endif

static inline int execshprofile( const char *profile )
{
    return execlp( config.terminal_emulator, config.terminal_emulator, "-e","switch_profile.sh", profile, NULL );
}
// execute sub-process
static pid_t exec_profile( const char *cmd )
{
    if ( !cmd || !cmd[0] ) return -1;

    signal( SIGCHLD, catch_exit );
    pid_t pid = fork();

    if ( !pid ) {
        setsid();
        execshprofile( cmd );
        exit( EXIT_FAILURE );
    }

    return pid;
}
static char ** add_elements( char **retv, char *element, unsigned int *retv_index )
{
    retv = realloc( retv, ( ( *retv_index )+2 )*sizeof( char* ) );
    retv[( *retv_index )] = element;
    retv[( *retv_index )+1] = NULL;
    ( *retv_index )++;
    return retv;
}

static char ** get_profile ( )
{
    unsigned int retv_index = 0;
    char *path;
    char **retv = NULL;
#ifdef TIMING
    struct timespec start, stop;
    clock_gettime( CLOCK_REALTIME, &start );
#endif

    if ( getenv( "HOME" ) == NULL ) return NULL;

    const char *hd = getenv( "HOME" );
    path = allocate( strlen( hd ) + strlen( ".switch_profile.conf" )+3 );
    sprintf( path, "%s/%s", hd, ".switch_profile.conf" );
    FILE *fd = fopen ( path, "r" );

    if ( fd != NULL ) {
        char buffer[1024];
        char *element = NULL;
        int enabled = 0;

        while ( fgets( buffer,1024,fd ) != NULL ) {
            buffer[strlen( buffer )-1] = '\0';
            char *entry = index( buffer, '=' );

            // skip
            if ( entry == NULL ) {
                if ( element != NULL && enabled ) {
                    retv = add_elements( retv, element, &retv_index );
                }

                element = NULL;
                continue;
            }

            *entry = '\0';

            if ( element == NULL ) {
                element = strdup( buffer );
                enabled = strtol( entry+1, NULL, 10 );
            } else {
                if ( strncmp( buffer, element, strlen( element ) ) == 0 )
                    continue;

                // New entry? Store old one
                if ( enabled ) {
                    retv = add_elements( retv, element, &retv_index );
                }

                element = strdup( buffer );
                enabled = strtol( entry+1, NULL, 10 );
            }
        }

        if ( element  != NULL && enabled ) {
            retv = add_elements( retv, element, &retv_index );
        }

        fclose( fd );
    }

    free( path );
#ifdef TIMING
    clock_gettime( CLOCK_REALTIME, &stop );

    if ( stop.tv_sec != start.tv_sec ) {
        stop.tv_nsec += ( stop.tv_sec-start.tv_sec )*1e9;
    }

    long diff = stop.tv_nsec-start.tv_nsec;
    printf( "Time elapsed: %ld us\n", diff/1000 );
#endif
    return retv;
}

static int token_match ( char **tokens, const char *input,
                         __attribute__( ( unused ) )int index,
                         __attribute__( ( unused ) )void *data )
{
    int match = 1;

    // Do a tokenized match.
    if ( tokens ) for ( int j  = 1; match && tokens[j]; j++ ) {
            match = ( strcasestr( input, tokens[j] ) != NULL );
        }

    return match;
}

SwitcherMode profile_switcher_dialog ( char **input )
{
    SwitcherMode retv = MODE_EXIT;
    // act as a launcher
    char **cmd_list = get_profile( );

    if ( cmd_list == NULL ) {
        cmd_list = allocate( 2*sizeof( char * ) );
        cmd_list[0] = strdup( "No profiles found" );
        cmd_list[1] = NULL;
    }

    int shift=0;
    int n = menu( cmd_list, input, "profile ", NULL, &shift,token_match, NULL );

    if ( n == -2 ) {
        retv = NEXT_DIALOG;
    } else if ( n >=0 && cmd_list[n] != NULL ) {
        exec_profile( cmd_list[n] );
    } else if ( n == -3 && *input != NULL && *input[0] != '\0' ) {
        exec_profile( *input );
    }

    for ( int i=0; cmd_list[i] != NULL; i++ ) {
        free( cmd_list[i] );
    }

    free( cmd_list );

    return retv;
}
#endif //__QC_MODE__
