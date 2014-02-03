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

char *dmenu_prompt = "dmenu ";

static char **get_dmenu ( )
{
    char buffer[1024];
    char **retv = NULL;
    int index = 0;

    while ( fgets( buffer, 1024, stdin ) != NULL ) {
        retv = realloc( retv, ( index+2 )*sizeof( char* ) );
        retv[index] = strdup( buffer );

        if ( retv[index][strlen( buffer )-1] == '\n' )
            retv[index][strlen( buffer )-1] = '\0';

        retv[index+1] = NULL;
        index++;
    }

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

SwitcherMode dmenu_switcher_dialog ( char **input )
{
    int shift=0;
    int selected_line = 0;
    SwitcherMode retv = MODE_EXIT;
    // act as a launcher
    char **list = get_dmenu( );

    int mretv = menu( list, input, dmenu_prompt,NULL, &shift,
            token_match, NULL, &selected_line );

    if ( mretv == MENU_NEXT ) {
        retv = DMENU_DIALOG;
    } else if ( mretv == MENU_OK && list[selected_line] != NULL ) {
        fputs( list[selected_line],stdout );
    } else if ( mretv == MENU_CUSTOM_INPUT && *input != NULL && *input[0] != '\0' ) {
        fputs( *input, stdout );
    }

    for ( unsigned int i=0; list != NULL && list[i] != NULL; i++ ) {
        free( list[i] );
    }

    if ( list != NULL ) free( list );

    return retv;
}

