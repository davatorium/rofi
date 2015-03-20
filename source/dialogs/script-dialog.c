/**
 * rofi
 *
 * MIT/X11 License
 * Copyright 2013-2015 Qball  Cow <qball@gmpclient.org>
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "rofi.h"
#include "dialogs/script-dialog.h"
#include "helper.h"





static char **get_script_output ( char *command, unsigned int *length )
{
    char **retv = NULL;

    *length = 0;
    int fd = execute_generator ( command );
    if ( fd >= 0 ) {
        FILE *inp = fdopen ( fd, "r" );
        if ( inp ) {
            char buffer[1024];
            while ( fgets ( buffer, 1024, inp ) != NULL ) {
                retv                  = g_realloc ( retv, ( ( *length ) + 2 ) * sizeof ( char* ) );
                retv[( *length )]     = g_strdup ( buffer );
                retv[( *length ) + 1] = NULL;

                // Filter out line-end.
                if ( retv[( *length )][strlen ( buffer ) - 1] == '\n' ) {
                    retv[( *length )][strlen ( buffer ) - 1] = '\0';
                }

                ( *length )++;
            }
            fclose ( inp );
        }
    }
    return retv;
}

static char **execute_executor ( ScriptOptions *options, const char *result, unsigned int *length )
{
    char **retv = NULL;

    char *arg     = g_shell_quote ( result );
    char *command = g_strdup_printf ( "%s %s", options->script_path, arg );
    retv = get_script_output ( command, length );
    g_free ( command );
    g_free ( arg );
    return retv;
}

SwitcherMode script_switcher_dialog ( char **input, void *data )
{
    ScriptOptions *options = (ScriptOptions *) data;
    assert ( options != NULL );
    int           selected_line = 0;
    SwitcherMode  retv          = MODE_EXIT;
    unsigned int  length        = 0;
    char          **list        = get_script_output ( options->script_path, &length );
    char          *prompt       = g_strdup_printf ( "%s:", options->name );

    do {
        unsigned int new_length = 0;
        char         **new_list = NULL;
        int          mretv      = menu ( list, length, input, prompt, NULL, NULL,
                                         token_match, NULL, &selected_line, FALSE );

        if ( mretv == MENU_NEXT ) {
            retv = NEXT_DIALOG;
        }
        else if ( mretv == MENU_PREVIOUS ) {
            retv = PREVIOUS_DIALOG;
        }
        else if ( mretv == MENU_QUICK_SWITCH ) {
            retv = selected_line;
        }
        else if ( mretv == MENU_OK && list[selected_line] != NULL ) {
            new_list = execute_executor ( options, list[selected_line], &new_length );
        }
        else if ( mretv == MENU_CUSTOM_INPUT && *input != NULL && *input[0] != '\0' ) {
            new_list = execute_executor ( options, *input, &new_length );
        }

        // Free old list.
        g_strfreev ( list );
        list = NULL;

        // If a new list was generated, use that an loop around.
        if ( new_list != NULL ) {
            list   = new_list;
            length = new_length;
            g_free ( *input );
            *input = NULL;
        }
    } while ( list != NULL );

    g_free ( prompt );
    return retv;
}

void script_switcher_free_options ( void *data )
{
    ScriptOptions *sw = (ScriptOptions *) data;
    if ( sw == NULL ) {
        return;
    }
    g_free ( sw->name );
    g_free ( sw->script_path );
    g_free ( sw );
}


ScriptOptions *script_switcher_parse_setup ( const char *str )
{
    ScriptOptions *sw    = g_malloc0 ( sizeof ( *sw ) );
    char          *endp  = NULL;
    char          *parse = g_strdup ( str );
    unsigned int  index  = 0;
    for ( char *token = strtok_r ( parse, ":", &endp ); token != NULL; token = strtok_r ( NULL, ":", &endp ) ) {
        if ( index == 0 ) {
            sw->name = g_strdup ( token );
        }
        else if ( index == 1 ) {
            sw->script_path = g_strdup ( token );
        }
        index++;
    }
    g_free ( parse );
    if ( index == 2 ) {
        return sw;
    }
    fprintf ( stderr, "The script command '%s' has %u options, but needs 2: <name>:<script>.\n",
              str, index );
    script_switcher_free_options ( sw );
    return NULL;
}

