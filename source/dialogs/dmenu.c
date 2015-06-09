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

/**
 * @param format The format string used. See below for possible syntax.
 * @param string The selected entry.
 * @param selected_line The selected line index.
 * @param filter The entered filter.
 *
 * Function that outputs the selected line in the user-specified format.
 * Currently the following formats are supported:
 *   * i: Print the index (0-(N-1))
 *   * d: Print the index (1-N)
 *   * s: Print input string.
 *   * q: Print quoted input string.
 *   * f: Print the entered filter.
 *   * F: Print the entered filter, quoted
 *
 * This functions outputs the formatted string to stdout, appends a newline (\n) character and
 * calls flush on the file descriptor.
 */
static void dmenu_output_formatted_line ( const char *format, const char *string, int selected_line, const char *filter )
{
    for ( int i = 0; format && format[i]; i++ ) {
        if ( format[i] == 'i' ) {
            fprintf ( stdout, "%d", selected_line );
        }
        else if ( format[i] == 'd' ) {
            fprintf ( stdout, "%d", ( selected_line + 1 ) );
        }
        else if ( format[i] == 's' ) {
            fputs ( string, stdout );
        }
        else if ( format[i] == 'q' ) {
            char *quote = g_shell_quote ( string );
            fputs ( quote, stdout );
            g_free ( quote );
        }
        else if ( format[i] == 'f' ) {
            fputs ( filter, stdout );
        }
        else if ( format[i] == 'F' ) {
            char *quote = g_shell_quote ( filter );
            fputs ( quote, stdout );
            g_free ( quote );
        }
        else {
            fputc ( format[i], stdout );
        }
    }
    fputc ( '\n', stdout );
    fflush ( stdout );
}

int dmenu_switcher_dialog ( char **input )
{
    char *dmenu_prompt = "dmenu ";
    int  selected_line = -1;
    int  retv          = FALSE;
    int  length        = 0;
    char **list        = get_dmenu ( &length );
    int  restart       = FALSE;
    char *message      = NULL;


    find_arg_str ( "-mesg", &message );

    // By default we print the unescaped line back.
    char *format = "s";
    // This is here for compatibility reason.
    // Use -format 'i' instead.
    if ( find_arg (  "-i" ) >= 0 ) {
        format = "i";
    }
    // Allow user to override the output format.
    find_arg_str ( "-format", &format );
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
    find_arg_str_alloc ( "-filter", input );

    char *select = NULL;
    find_arg_str ( "-select", &select );
    if ( select != NULL ) {
        char **tokens = tokenize ( select, config.case_sensitive );
        int  i        = 0;
        for ( i = 0; i < length; i++ ) {
            if ( token_match ( tokens, list[i], config.case_sensitive, 0, NULL ) ) {
                selected_line = i;
                break;
            }
        }
        g_strfreev ( tokens );
    }

    do {
        int next_pos = selected_line;
        int mretv    = menu ( list, length, input, dmenu_prompt,
                              token_match, NULL, &selected_line, config.levenshtein_sort, get_display_data, list, &next_pos, message );
        // Special behavior.
        if ( only_selected ) {
            /**
             * Select item mode.
             */
            restart = TRUE;
            if ( ( mretv & ( MENU_OK | MENU_QUICK_SWITCH ) ) && list[selected_line] != NULL ) {
                dmenu_output_formatted_line ( format, list[selected_line], selected_line, *input );
                retv = TRUE;
                if ( ( mretv & MENU_QUICK_SWITCH ) ) {
                    retv = 10 + ( mretv & MENU_LOWER_MASK );
                }
                return retv;
            }
            selected_line = next_pos - 1;
            continue;
        }
        // We normally do not want to restart the loop.
        restart = FALSE;
        // Normal mode
        if ( ( mretv & MENU_OK  ) && list[selected_line] != NULL ) {
            dmenu_output_formatted_line ( format, list[selected_line], selected_line, *input );
            if ( ( mretv & MENU_SHIFT ) ) {
                restart = TRUE;
                // Move to next line.
                selected_line = MIN ( next_pos, length - 1 );
            }
            retv = TRUE;
            // Custom input
        }
        else if ( ( mretv & ( MENU_CUSTOM_INPUT ) ) ) {
            dmenu_output_formatted_line ( format, *input, -1, *input );
            retv = TRUE;
        }
        // Quick switch with entry selected.
        else if ( ( mretv & MENU_QUICK_SWITCH ) && selected_line > 0 ) {
            dmenu_output_formatted_line ( format, list[selected_line], selected_line, *input );

            restart = FALSE;
            retv    = 10 + ( mretv & MENU_LOWER_MASK );
        }
        // Quick switch without entry selected.
        else if ( ( mretv & MENU_QUICK_SWITCH ) && selected_line == -1 ) {
            dmenu_output_formatted_line ( format, *input, -1, *input );

            restart = FALSE;
            retv    = 10 + ( mretv & MENU_LOWER_MASK );
        }
    } while ( restart );

    g_strfreev ( list );
    g_free ( urgent_list );
    g_free ( active_list );

    return retv;
}

