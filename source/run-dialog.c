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
#include <errno.h>

#include "simpleswitcher.h"
#include "run-dialog.h"
#ifdef TIMING
#include <time.h>
#endif

extern char *config_terminal_emulator;

static inline int execsh( const char *cmd ,int run_in_term )
{
// use sh for args parsing
    if ( run_in_term )
        return execlp( config_terminal_emulator, config_terminal_emulator, "-e", "sh", "-c", cmd, NULL );

    return execlp( "/bin/sh", "sh", "-c", cmd, NULL );
}
// execute sub-process
static pid_t exec_cmd( const char *cmd, int run_in_term )
{
    if ( !cmd || !cmd[0] ) return -1;

    signal( SIGCHLD, catch_exit );
    pid_t pid = fork();

    if ( !pid ) {
        setsid();
        execsh( cmd, run_in_term );
        exit( EXIT_FAILURE );
    }

    int curr = -1;
    unsigned int index = 0;
    char **retv = NULL;
    const char *hd = getenv( "HOME" );

    if ( hd == NULL ) return pid;

    /**
     * This happens in non-critical time (After launching app)
     * It is allowed to be a bit slower.
     */
    char *path = allocate( strlen( hd ) + strlen( "/.simpleswitcher.cache" )+2 );
    sprintf( path, "%s/%s", hd, ".simpleswitcher.cache" );
    FILE *fd = fopen ( path, "r" );
    char buffer[1024];

    if ( fd != NULL ) {
        while ( fgets( buffer,1024,fd ) != NULL ) {
            retv = realloc( retv, ( index+2 )*sizeof( char* ) );
            buffer[strlen( buffer )-1] = '\0';
            retv[index] = strdup( buffer );
            retv[index+1] = NULL;

            if ( strcasecmp( retv[index], cmd ) == 0 ) {
                curr = index;
            }

            index++;
        }

        fclose( fd );
    }

    /**
     * Write out the last 25 results again.
     */
    fd = fopen ( path, "w" );

    if ( fd ) {
        // Last one goes on top!
        fputs( cmd, fd );
        fputc( '\n', fd );

        for ( int i = 0; i < ( int )index && i < 20; i++ ) {
            if ( i != curr ) {
                fputs( retv[i], fd );
                fputc( '\n', fd );
            }
        }

        fclose( fd );
    }

    for ( int i=0; retv[i] != NULL; i++ ) {
        free( retv[i] );
    }

    free( retv );

    free( path );

    return pid;
}
static int sort_func ( const void *a, const void *b )
{
    const char *astr = *( const char * const * )a;
    const char *bstr = *( const char * const * )b;
    return strcasecmp( astr,bstr );
}
static char ** get_apps ( )
{
    int num_favorites = 0;
    unsigned int index = 0;
    char *path;
    char **retv = NULL;
#ifdef TIMING
    struct timespec start, stop;
    clock_gettime( CLOCK_REALTIME, &start );
#endif

    if ( getenv( "PATH" ) == NULL ) return NULL;

    const char *hd = getenv( "HOME" );

    if ( hd == NULL ) return NULL;

    path = allocate( strlen( hd ) + strlen( "/.simpleswitcher.cache" )+2 );
    sprintf( path, "%s/%s", hd, ".simpleswitcher.cache" );
    FILE *fd = fopen ( path, "r" );
    char buffer[1024];

    if ( fd != NULL ) {
        while ( fgets( buffer,1024,fd ) != NULL ) {
            retv = realloc( retv, ( index+2 )*sizeof( char* ) );
            buffer[strlen( buffer )-1] = '\0';
            retv[index] = strdup( buffer );
            retv[index+1] = NULL;
            index++;
            num_favorites++;
        }

        fclose( fd );
    }

    free( path );


    path = strdup( getenv( "PATH" ) );

    for ( const char *dirname = strtok( path, ":" ); dirname != NULL; dirname = strtok( NULL, ":" ) ) {
        DIR *dir = opendir( dirname );

        if ( dir != NULL ) {
            struct dirent *dent;

            while ( ( dent=readdir( dir ) )!=NULL ) {
                if ( dent->d_type != DT_REG &&
                     dent->d_type != DT_LNK &&
                     dent->d_type != DT_UNKNOWN ) {
                    continue;
                }

                int found = 0;

                // This is a nice little penalty, but doable? time will tell.
                // given num_favorites is max 25.
                for ( int j = 0; found == 0 && j < num_favorites; j++ ) {
                    if ( strcasecmp( dent->d_name, retv[j] ) == 0 ) found = 1;
                }

                if ( found == 1 ) continue;

                retv = realloc( retv, ( index+2 )*sizeof( char* ) );
                retv[index] = strdup( dent->d_name );
                retv[index+1] = NULL;
                index++;
            }

            closedir( dir );
        }
    }

    // TODO: check this is still fast enough. (takes 1ms on laptop.)
    qsort( &retv[num_favorites],index-num_favorites, sizeof( char* ), sort_func );
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

SwitcherMode run_switcher_dialog ( char **input )
{
    SwitcherMode retv = MODE_EXIT;
    // act as a launcher
    char **cmd_list = get_apps( );

    if ( cmd_list == NULL ) {
        cmd_list = allocate( 2*sizeof( char * ) );
        cmd_list[0] = strdup( "No applications found" );
        cmd_list[1] = NULL;
    }

    int shift=0;
    int n = menu( cmd_list, input, "$ ", 0, NULL, &shift,token_match, NULL );

    if ( n == -2 ) {
        retv = SSH_DIALOG;
    } else if ( n >=0 && cmd_list[n] != NULL ) {
        exec_cmd( cmd_list[n], shift );
    } else if ( n == -3 && *input != NULL && *input[0] != '\0' ) {
        exec_cmd( *input, shift );
    }

    for ( int i=0; cmd_list[i] != NULL; i++ ) {
        free( cmd_list[i] );
    }

    free( cmd_list );

    return retv;
}
