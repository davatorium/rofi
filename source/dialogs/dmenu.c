/**
 * rofi
 *
 * MIT/X11 License
 * Copyright 2013-2015 Qball Cow <qball@gmpclient.org>
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
#include "rofi.h"
#include "dialogs/dmenu.h"
#include "helper.h"


static char **get_dmenu ( int *length )
{
    char buffer[1024];
    char **retv = NULL;

    *length = 0;

    while ( fgets_s ( buffer, 1024, stdin, (char) config.separator ) != NULL ) {
        retv                  = g_realloc ( retv, ( ( *length ) + 2 ) * sizeof ( char* ) );
        retv[( *length )]     = g_strdup ( buffer );
        retv[( *length ) + 1] = NULL;

        // Filter out line-end.
        if ( retv[( *length )][strlen ( buffer ) - 1] == '\n' ) {
            retv[( *length )][strlen ( buffer ) - 1] = '\0';
        }

        ( *length )++;
        // Stop when we hit 2³¹ entries.
        if ( ( *length ) == INT_MAX ) {
            return retv;
        }
    }

    return retv;
}

struct range_pair
{
    unsigned int start;
    unsigned int stop;
};

struct range_pair * urgent_list   = NULL;
unsigned int      num_urgent_list = 0;
struct range_pair * active_list   = NULL;
unsigned int      num_active_list = 0;

static void parse_pair ( char  *input, struct range_pair  *item )
{
    int index = 0;
    for ( char *token = strsep ( &input, "-" );
          token != NULL;
          token = strsep ( &input, "-" ) ) {
        if ( index == 0 ) {
            item->start = item->stop = (unsigned int) strtoul ( token, NULL, 10 );
            index++;
        }
        else {
            if ( token[0] == '\0' ) {
                item->stop = 0xFFFFFFFF;
            }
            else{
                item->stop = (unsigned int) strtoul ( token, NULL, 10 );
            }
        }
    }
}

static void parse_ranges ( char *input, struct range_pair **list, unsigned int *length )
{
    char *endp;
    for ( char *token = strtok_r ( input, ",", &endp );
          token != NULL;
          token = strtok_r ( NULL, ",", &endp ) ) {
        // Make space.
        *list = g_realloc ( ( *list ), ( ( *length ) + 1 ) * sizeof ( struct range_pair ) );
        // Parse a single pair.
        parse_pair ( token, &( ( *list )[*length] ) );

        ( *length )++;
    }
}

static const char *get_display_data ( unsigned int index, void *data, G_GNUC_UNUSED int *state )
{
    char **retv = (char * *) data;
    for ( unsigned int i = 0; i < num_active_list; i++ ) {
        if ( index >= active_list[i].start && index <= active_list[i].stop ) {
            *state |= ACTIVE;
        }
    }
    for ( unsigned int i = 0; i < num_urgent_list; i++ ) {
        if ( index >= urgent_list[i].start && index <= urgent_list[i].stop ) {
            *state |= URGENT;
        }
    }
    return retv[index];
}

int dmenu_switcher_dialog ( char **input )
{
    char *dmenu_prompt = "dmenu ";
    int  selected_line = 0;
    int  retv          = FALSE;
    int  length        = 0;
    char **list        = get_dmenu ( &length );
    int  restart       = FALSE;

    int  number_mode = FALSE;
    // Check if the user requested number mode.
    if ( find_arg (  "-i" ) >= 0 ) {
        number_mode = TRUE;
    }
    // Check prompt
    find_arg_str (  "-p", &dmenu_prompt );
    find_arg_int (  "-l", &selected_line );
    // Urgent.
    char *str = NULL;
    find_arg_str (  "-u", &str );
    if ( str != NULL ) {
        parse_ranges ( str, &urgent_list, &num_urgent_list );
    }
    // Active
    str = NULL;
    find_arg_str (  "-a", &str );
    if ( str != NULL ) {
        parse_ranges ( str, &active_list, &num_active_list );
    }

    int only_selected = FALSE;
    if ( find_arg ( "-only-match" ) >= 0 ) {
        only_selected = TRUE;
        if ( length == 0 ) {
            return TRUE;
        }
    }

    do {
        int next_pos = selected_line;
        int mretv    = menu ( list, length, input, dmenu_prompt,
                              token_match, NULL, &selected_line, config.levenshtein_sort, get_display_data, list, &next_pos );
        // Special behavior.
        if ( only_selected ) {
            /**
             * Select item mode.
             */
            restart = TRUE;
            if ( ( mretv & ( MENU_OK | MENU_QUICK_SWITCH ) ) && list[selected_line] != NULL ) {
                if ( number_mode ) {
                    fprintf ( stdout, "%d", selected_line );
                }
                else {
                    fputs ( list[selected_line], stdout );
                }
                retv = TRUE;
                if ( ( mretv & MENU_QUICK_SWITCH ) ) {
                    retv = 10 + ( mretv & MENU_LOWER_MASK );
                }
                fputc ( '\n', stdout );
                fflush ( stdout );
                return retv;
            }
            selected_line = next_pos - 1;
            continue;
        }
        // We normally do not want to restart the loop.
        restart = FALSE;
        if ( ( mretv & ( MENU_OK | MENU_CUSTOM_INPUT ) ) && list[selected_line] != NULL ) {
            if ( number_mode ) {
                fprintf ( stdout, "%d", selected_line );
            }
            else {
                fputs ( list[selected_line], stdout );
            }
            fputc ( '\n', stdout );
            fflush ( stdout );
            if ( ( mretv & MENU_SHIFT ) ) {
                restart = TRUE;
                // Move to next line.
                selected_line = MIN ( next_pos, length - 1 );
            }
            retv = TRUE;
            if ( ( mretv & MENU_QUICK_SWITCH ) ) {
                retv = 10 + ( mretv & MENU_LOWER_MASK );
            }
        }
        else if ( ( mretv & MENU_QUICK_SWITCH ) ) {
            if ( number_mode ) {
                fprintf ( stdout, "%d", selected_line );
            }
            else {
                fputs ( list[selected_line], stdout );
            }
            fputc ( '\n', stdout );
            fflush ( stdout );

            restart = FALSE;
            retv    = 10 + ( mretv & MENU_LOWER_MASK );
        }
    } while ( restart );

    g_strfreev ( list );
    g_free ( urgent_list );
    g_free ( active_list );

    return retv;
}

