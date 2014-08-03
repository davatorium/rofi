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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include "rofi.h"
#include "dmenu-dialog.h"

char *dmenu_prompt = "dmenu ";

static char **get_dmenu ( unsigned int *length )
{
    char buffer[1024];
    char **retv = NULL;

    *length = 0;

    while ( fgets ( buffer, 1024, stdin ) != NULL ) {
        char **tr = realloc ( retv, ( ( *length ) + 2 ) * sizeof ( char* ) );
        if ( tr == NULL ) {
            return retv;
        }
        retv                  = tr;
        retv[( *length )]     = strdup ( buffer );
        retv[( *length ) + 1] = NULL;

        // Filter out line-end.
        if ( retv[( *length )][strlen ( buffer ) - 1] == '\n' ) {
            retv[( *length )][strlen ( buffer ) - 1] = '\0';
        }

        ( *length )++;
    }

    return retv;
}

SwitcherMode dmenu_switcher_dialog ( char **input, void *data )
{
    int          selected_line = 0;
    SwitcherMode retv          = MODE_EXIT;
    unsigned int length        = 0;
    char         **list        = get_dmenu ( &length );
    int          restart       = FALSE;

    do {
        int shift = 0;
        int mretv = menu ( list, length, input, dmenu_prompt, NULL, &shift,
                           token_match, NULL, &selected_line, FALSE );

        // We normally do not want to restart the loop.
        restart = FALSE;
        if ( mretv == MENU_NEXT ) {
            retv = RELOAD_DIALOG;
        }
        else if ( mretv == MENU_OK && list[selected_line] != NULL ) {
            fputs ( list[selected_line], stdout );
            fputc ( '\n', stdout );
            fflush ( stdout );
            if ( shift ) {
                restart = TRUE;
                // Move to next line.
                selected_line = MIN ( selected_line + 1, length - 1 );
            }
        }
        else if ( mretv == MENU_CUSTOM_INPUT && *input != NULL && *input[0] != '\0' ) {
            fputs ( *input, stdout );
            fputc ( '\n', stdout );
            fflush ( stdout );
            if ( shift ) {
                restart = TRUE;
                // Move to next line.
                selected_line = MIN ( selected_line + 1, length - 1 );
            }
        }
    } while ( restart );
    for ( unsigned int i = 0; i < length; i++ ) {
        free ( list[i] );
    }

    if ( list != NULL ) {
        free ( list );
    }
    return retv;
}

