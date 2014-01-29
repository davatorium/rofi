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

#include <json/json.h>

char *json_input_file = NULL;

static inline int execsh( const char *cmd ,int run_in_term )
{
// use sh for args parsing
    if ( run_in_term )
        return execlp( config.terminal_emulator, config.terminal_emulator, "-e", "sh", "-c", cmd, NULL );

    return execlp( "/bin/sh", "sh", "-c", cmd, NULL );
}
static void exec_json( const char *cmd, int run_in_term)
{
    if ( !cmd || !cmd[0] ) return;

    signal( SIGCHLD, catch_exit );
    pid_t pid = fork();

    if ( !pid ) {
        setsid();
        execsh( cmd, run_in_term );
        exit( EXIT_FAILURE );
    }

}

typedef struct pmenu {
    char *prompt;
    int execute_in_term;
    unsigned int num_entries;
    char **entries;
    char **commands;
} pmenu;

static struct json_object * read_json_file_descr(FILE *fd)
{
    struct json_object *jo = NULL;
    enum json_tokener_error err;
    struct json_tokener *tok = json_tokener_new();
    char buffer[64];
    ssize_t len = 0;

    while( (len = fread(buffer, 1, 64, fd)) > 0) {
        jo = json_tokener_parse_ex(tok, buffer, len);

        if((err = json_tokener_get_error(tok)) != json_tokener_continue)
        {
            if(err != json_tokener_success) {
                fprintf(stderr, "Error parsing json file: %s\n", json_tokener_error_desc(err)); 
                jo = NULL;
            }
            break;
        } 
    }
    // Get reference before we destroy the tokener.
    if(jo) json_object_get(jo);
    json_tokener_free(tok);
    return jo;
}

static pmenu *get_json ( )
{

#ifdef TIMING
    struct timespec start, stop;
    clock_gettime( CLOCK_REALTIME, &start );
#endif

    struct json_object *jo = NULL;
    if(json_input_file != NULL) {
        if (json_input_file[0] == '-' && json_input_file[1] == '\0') {
            jo = read_json_file_descr(stdin);
        }
        else
        {
            FILE *fd = fopen(json_input_file, "r");
            if(fd == NULL) {
                fprintf(stderr, "Failed to open file: %s: %s\n",
                        json_input_file, strerror(errno));
                return NULL;
            }
            jo = read_json_file_descr(fd);
            fclose(fd);

        }
    } 

    if(jo && json_object_get_type(jo) != json_type_object) {
        fprintf(stderr, "Json file has invalid root type.\n");
        json_object_put(jo);
        jo = NULL;
    }
    // Create error and exit.
    if(jo == NULL ) {
        return NULL;
    }
    pmenu *retv = allocate_clear(sizeof(*retv));

    struct json_object *jo2;
    if(json_object_object_get_ex(jo, "prompt", &jo2)) {
        retv->prompt = strdup(json_object_get_string(jo2));
    }
    if(json_object_object_get_ex(jo, "run-in-terminal", &jo2)) {
        retv->execute_in_term = json_object_get_int(jo2);
    }

    if(json_object_object_get_ex(jo, "commands", &jo2)) {
        json_object_object_foreach(jo2, key, child) {
            retv->entries= realloc( retv->entries, ( retv->num_entries+2 )*sizeof( char* ) );
            retv->entries[retv->num_entries] = strdup( key);
            retv->entries[retv->num_entries+1] = NULL;
            retv->commands= realloc( retv->commands, ( retv->num_entries+2 )*sizeof( char* ) );
            retv->commands[retv->num_entries] = strdup( json_object_get_string(child));
            retv->commands[retv->num_entries+1] = NULL;

            retv->num_entries++;
        }
    }
    json_object_put(jo);



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

SwitcherMode json_switcher_dialog ( char **input )
{
    SwitcherMode retv = MODE_EXIT;
    // act as a launcher
    pmenu *list = get_json( );

    if ( list == NULL ) {
        return retv;
    }

    if(list->num_entries > 0) {

        int shift=0;
        int n = menu( list->entries, input, list->prompt, NULL, &shift,token_match, NULL );

        if ( n == -2 ) {
            retv = JSON_DIALOG;
        } else if ( n >=0 && list->commands[n] != NULL ) {
            exec_json( list->commands[n], list->execute_in_term);
        }

    } else {
        fprintf(stderr, "No commands found in json file\n");
    }
    for ( unsigned int i=0; i < list->num_entries; i++ ) {
        free( list->entries[i] );
        free( list->commands[i] );
    }

    if(list->entries)free( list->entries );
    if(list->commands)free( list->commands );
    if(list->prompt)free( list->prompt );

    return retv;
}

