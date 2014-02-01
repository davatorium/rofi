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

#ifdef I3

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <X11/X.h>

#include <unistd.h>
#include <signal.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "simpleswitcher.h"
#include "mark-dialog.h"
#include <errno.h>
#include <linux/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <i3/ipc.h>

#ifdef TIMING
#include <time.h>
#endif

static char * escape_name( const char *mark, ssize_t *length )
{
    // Escape the mark.
    // Max length is twice the old size + trailing \0.
    char *escaped_mark = malloc( sizeof( char )*( strlen( mark )*2+1 ) );
    ssize_t lm = strlen( mark );
    *length = 0;

    for ( ssize_t iter = 0; iter < lm; iter++ ) {
        if ( mark[iter] == '\'' || mark[iter] == '\"' || mark[iter] == '\\' ) {
            escaped_mark[( *length )++] = '\\';
        }

        escaped_mark[( *length )++] = mark[iter];
        escaped_mark[( *length )] = '\0';
    }

    return escaped_mark;
}


static void exec_mark( const char *mark )
{
    int s, t, len;
    struct sockaddr_un remote;

    if ( config.i3_mode == 0 ) {
        fprintf( stderr, "Cannot use marks without i3 running\n" );
        return ;
    }

    if ( strlen( i3_socket_path ) > UNIX_PATH_MAX ) {
        fprintf( stderr, "Socket path is to long. %zd > %d\n", strlen( i3_socket_path ), UNIX_PATH_MAX );
        return;
    }

    if ( ( s = socket( AF_UNIX, SOCK_STREAM, 0 ) ) == -1 ) {
        fprintf( stderr, "Failed to open connection to I3: %s\n", strerror( errno ) );
        return;
    }

    remote.sun_family = AF_UNIX;
    strcpy( remote.sun_path, i3_socket_path );
    len = strlen( remote.sun_path ) + sizeof( remote.sun_family );

    if ( connect( s, ( struct sockaddr * )&remote, len ) == -1 ) {
        fprintf( stderr, "Failed to connect to I3 (%s): %s\n", i3_socket_path,strerror( errno ) );
        close( s );
        return ;
    }


// Formulate command
    ssize_t lem = 0;
    char *escaped_mark = escape_name( mark, &lem );
    char command[lem+20];
    snprintf( command, lem+20, "[con_mark=\"%s\"] focus", escaped_mark );

    // Prepare header.
    i3_ipc_header_t head;
    memcpy( head.magic, I3_IPC_MAGIC, 6 );
    head.size = strlen( command );
    head.type = I3_IPC_MESSAGE_TYPE_COMMAND;
    // Send header.
    send( s, &head, sizeof( head ),0 );
    // Send message
    send( s, command, strlen( command ),0 );


    // Receive result.
    t = recv( s, &head, sizeof( head ),0 );

    if ( t == sizeof( head ) ) {
        t= recv( s, command, head.size, 0 );
        command[t] = '\0';
        printf( "%s\n", command );
    }

    close( s );

    free( escaped_mark );
}

static char ** get_mark ( )
{
    unsigned int retv_index = 0;
    char **retv = NULL;

    if ( config.i3_mode == 0 ) {
        fprintf( stderr, "Cannot use marks without i3 running\n" );
        return retv;
    }

#ifdef TIMING
    struct timespec start, stop;
    clock_gettime( CLOCK_REALTIME, &start );
#endif

    i3_ipc_header_t head;
    int s, t, len;
    struct sockaddr_un remote;

    if ( strlen( i3_socket_path ) > UNIX_PATH_MAX ) {
        fprintf( stderr, "Socket path is to long. %zd > %d\n", strlen( i3_socket_path ), UNIX_PATH_MAX );
        return retv;
    }

    if ( ( s = socket( AF_UNIX, SOCK_STREAM, 0 ) ) == -1 ) {
        fprintf( stderr, "Failed to open connection to I3: %s\n", strerror( errno ) );
        return retv;
    }

    remote.sun_family = AF_UNIX;
    strcpy( remote.sun_path, i3_socket_path );
    len = strlen( remote.sun_path ) + sizeof( remote.sun_family );

    if ( connect( s, ( struct sockaddr * )&remote, len ) == -1 ) {
        fprintf( stderr, "Failed to connect to I3 (%s): %s\n", i3_socket_path,strerror( errno ) );
        close( s );
        return retv;
    }


// Formulate command
    // Prepare header.
    memcpy( head.magic, I3_IPC_MAGIC, 6 );
    head.size = 0;
    head.type = I3_IPC_MESSAGE_TYPE_GET_MARKS;
    // Send header.
    send( s, &head, sizeof( head ),0 );
    // Receive header.
    t = recv( s, &head, sizeof( head ),0 );

    if ( t == sizeof( head ) ) {
        char *result = malloc( sizeof( char )*( head.size+1 ) );
        ssize_t index = 0;
        t = 0;

        // Grab results.
        while ( index < ( ssize_t )head.size ) {
            t= recv( s, &result[index], ( head.size-t ), 0 );

            if ( t < 0 ) break;

            result[index+t] = '\0';
            index+=t;
        }

        for ( int iter_start = 1; iter_start < index-1 ; iter_start++ ) {
            // Skip , and opening "
            if ( result[iter_start] == '"' || result[iter_start] == ',' ) continue;

            int iter_end = iter_start;

            // Find closing tag.. make sure to ignore escaped chars.
            // Copy the un-escaped string into the first part.
            int rindex = 0;

            do {
                result[rindex++] = result[iter_end];
                iter_end++;

                if ( result[iter_end] == '\\' ) iter_end+=1;
            } while ( result[iter_end] != '"' );

            result[rindex] = '\0';

            // Add element to list.
            retv = realloc( retv, ( retv_index+2 )*sizeof( char* ) );
            retv[retv_index] = strndup( result,rindex );
            retv[retv_index+1] = NULL;
            retv_index++;

            iter_start = iter_end;
        }

        free( result );
    }

    close( s );

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

SwitcherMode mark_switcher_dialog ( char **input )
{
    SwitcherMode retv = MODE_EXIT;
    // act as a launcher
    char **cmd_list = get_mark( );

    if ( cmd_list == NULL ) {
        cmd_list = allocate( 2*sizeof( char * ) );
        cmd_list[0] = strdup( "No i3 marks found" );
        cmd_list[1] = NULL;
    }

    int shift=0;
    int selected_line = 0;
    int mretv = menu( cmd_list, input, "mark:", NULL, &shift,token_match,NULL, &selected_line );

    if ( mretv == MENU_NEXT ) {
        retv = NEXT_DIALOG;
    } else if ( mretv == MENU_OK && cmd_list[selected_line] != NULL ) {
        exec_mark( cmd_list[selected_line] );
    } else if ( mretv == MENU_CUSTOM_INPUT && *input != NULL && *input[0] != '\0' ) {
        exec_mark( *input );
    }

    for ( int i=0; cmd_list != NULL &&  cmd_list[i] != NULL; i++ ) {
        free( cmd_list[i] );
    }

    if ( cmd_list ) free( cmd_list );


    return retv;
}

#endif
