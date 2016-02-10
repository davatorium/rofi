/**
 * rofi
 *
 * MIT/X11 License
 * Copyright 2013-2016 Qball Cow <qball@gmpclient.org>
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
#include <stdint.h>
#include <errno.h>
#include "rofi.h"
#include "settings.h"
#include "textbox.h"
#include "dialogs/dmenu.h"
#include "helper.h"
#include "xrmoptions.h"
#include "view.h"
// We limit at 1000000 rows for now.
#define DMENU_MAX_ROWS    1000000

// TODO HACK TO BE REMOVED
extern Display *display;

struct range_pair
{
    unsigned int start;
    unsigned int stop;
};
typedef struct _DmenuModePrivateData
{
    char              *prompt;
    unsigned int      selected_line;
    char              *message;
    char              *format;
    struct range_pair * urgent_list;
    unsigned int      num_urgent_list;
    struct range_pair * active_list;
    unsigned int      num_active_list;
    struct range_pair * selected_list;
    unsigned int      num_selected_list;
    unsigned int      do_markup;
    // List with entries.
    char              **cmd_list;
    unsigned int      cmd_list_length;
} DmenuModePrivateData;

static char **get_dmenu ( FILE *fd, unsigned int *length )
{
    TICK_N ( "Read stdin START" );
    char         **retv   = NULL;
    unsigned int rvlength = 1;

    *length = 0;
    gchar   *data  = NULL;
    size_t  data_l = 0;
    ssize_t l      = 0;
    while ( ( l = getdelim ( &data, &data_l, config.separator, fd ) ) > 0 ) {
        if ( rvlength < ( *length + 2 ) ) {
            rvlength *= 2;
            retv      = g_realloc ( retv, ( rvlength ) * sizeof ( char* ) );
        }
        if ( data[l - 1] == config.separator ) {
            data[l - 1] = '\0';
            l--;
        }
        if (  !g_utf8_validate ( data, l, NULL ) ) {
            fprintf ( stderr, "String: '%s' is not valid utf-8\n", data );
            continue;
        }

        retv[( *length )] = data;
        data              = NULL;
        data_l            = 0;

        ( *length )++;
        // Stop when we hit 2³¹ entries.
        if ( ( *length ) >= DMENU_MAX_ROWS ) {
            break;
        }
    }
    if ( data != NULL ) {
        free ( data );
        data = NULL;
    }
    if ( retv != NULL ) {
        retv               = g_realloc ( retv, ( *length + 1 ) * sizeof ( char* ) );
        retv[( *length ) ] = NULL;
    }
    TICK_N ( "Read stdin STOP" );
    return retv;
}

static unsigned int dmenu_mode_get_num_entries ( const Mode *sw )
{
    const DmenuModePrivateData *rmpd = (const DmenuModePrivateData *) mode_get_private_data ( sw );
    return rmpd->cmd_list_length;
}

static void parse_pair ( char  *input, struct range_pair  *item )
{
    int index = 0;
    for ( char *token = strsep ( &input, "-" ); token != NULL; token = strsep ( &input, "-" ) ) {
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
    if ( input == NULL ) {
        return;
    }
    for ( char *token = strtok_r ( input, ",", &endp ); token != NULL; token = strtok_r ( NULL, ",", &endp ) ) {
        // Make space.
        *list = g_realloc ( ( *list ), ( ( *length ) + 1 ) * sizeof ( struct range_pair ) );
        // Parse a single pair.
        parse_pair ( token, &( ( *list )[*length] ) );

        ( *length )++;
    }
}

static char *get_display_data ( const Mode *data, unsigned int index, int *state, int get_entry )
{
    Mode                 *sw    = (Mode *) data;
    DmenuModePrivateData *pd    = (DmenuModePrivateData *) mode_get_private_data ( sw );
    char                 **retv = (char * *) pd->cmd_list;
    for ( unsigned int i = 0; i < pd->num_active_list; i++ ) {
        if ( index >= pd->active_list[i].start && index <= pd->active_list[i].stop ) {
            *state |= ACTIVE;
        }
    }
    for ( unsigned int i = 0; i < pd->num_urgent_list; i++ ) {
        if ( index >= pd->urgent_list[i].start && index <= pd->urgent_list[i].stop ) {
            *state |= URGENT;
        }
    }
    for ( unsigned int i = 0; i < pd->num_selected_list; i++ ) {
        if ( index >= pd->selected_list[i].start && index <= pd->selected_list[i].stop ) {
            *state |= SELECTED;
        }
    }
    if ( pd->do_markup ) {
        *state |= MARKUP;
    }
    return get_entry ? g_strdup ( retv[index] ) : NULL;
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
static void dmenu_output_formatted_line ( const char *format, const char *string, int selected_line,
                                          const char *filter )
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
static void dmenu_mode_free ( Mode *sw )
{
    if ( mode_get_private_data ( sw ) == NULL ) {
        return;
    }
    DmenuModePrivateData *pd = (DmenuModePrivateData *) mode_get_private_data ( sw );
    if ( pd != NULL ) {
        for ( size_t i = 0; i < pd->cmd_list_length; i++ ) {
            if ( pd->cmd_list[i] ) {
                free ( pd->cmd_list[i] );
            }
        }
        g_free ( pd->cmd_list );
        g_free ( pd->urgent_list );
        g_free ( pd->active_list );
        g_free ( pd->selected_list );

        g_free ( pd );
        mode_set_private_data ( sw, NULL );
    }
}

static int dmenu_mode_init ( Mode *sw )
{
    if ( mode_get_private_data ( sw ) != NULL ) {
        return TRUE;
    }
    mode_set_private_data ( sw, g_malloc0 ( sizeof ( DmenuModePrivateData ) ) );
    DmenuModePrivateData *pd = (DmenuModePrivateData *) mode_get_private_data ( sw );

    pd->prompt        = "dmenu ";
    pd->selected_line = UINT32_MAX;

    find_arg_str ( "-mesg", &( pd->message ) );

    // Check prompt
    find_arg_str (  "-p", &( pd->prompt ) );
    find_arg_uint (  "-selected-row", &( pd->selected_line ) );
    // By default we print the unescaped line back.
    pd->format = "s";

    // Allow user to override the output format.
    find_arg_str ( "-format", &( pd->format ) );
    // Urgent.
    char *str = NULL;
    find_arg_str (  "-u", &str );
    if ( str != NULL ) {
        parse_ranges ( str, &( pd->urgent_list ), &( pd->num_urgent_list ) );
    }
    // Active
    str = NULL;
    find_arg_str (  "-a", &str );
    if ( str != NULL ) {
        parse_ranges ( str, &( pd->active_list ), &( pd->num_active_list ) );
    }

    // DMENU COMPATIBILITY
    find_arg_uint (  "-l", &( config.menu_lines ) );

    /**
     * Dmenu compatibility.
     * `-b` put on bottom.
     */
    if ( find_arg ( "-b" ) >= 0 ) {
        config.location = 6;
    }
    /* -i case insensitive */
    config.case_sensitive = TRUE;
    if ( find_arg ( "-i" ) >= 0 ) {
        config.case_sensitive = FALSE;
    }
    FILE *fd = NULL;
    str = NULL;
    if ( find_arg_str ( "-input", &str ) ) {
        char *estr = rofi_expand_path ( str );
        fd = fopen ( str, "r" );
        if ( fd == NULL ) {
            char *msg = g_markup_printf_escaped ( "Failed to open file: <b>%s</b>:\n\t<i>%s</i>", estr, strerror ( errno ) );
            rofi_view_error_dialog ( msg, TRUE );
            g_free ( msg );
            g_free ( estr );
            return TRUE;
        }
        g_free ( estr );
    }
    pd->cmd_list = get_dmenu ( fd == NULL ? stdin : fd, &( pd->cmd_list_length ) );
    if ( fd != NULL ) {
        fclose ( fd );
    }
    return TRUE;
}

static int dmenu_token_match ( const Mode *sw, char **tokens, int not_ascii, int case_sensitive, unsigned int index )
{
    DmenuModePrivateData *rmpd = (DmenuModePrivateData *) mode_get_private_data ( sw );
    return token_match ( tokens, rmpd->cmd_list[index], not_ascii, case_sensitive );
}

static int dmenu_is_not_ascii ( const Mode *sw, unsigned int index )
{
    DmenuModePrivateData *rmpd = (DmenuModePrivateData *) mode_get_private_data ( sw );
    return !g_str_is_ascii ( rmpd->cmd_list[index] );
}

#include "mode-private.h"
Mode dmenu_mode =
{
    .name               = "dmenu",
    .cfg_name_key       = "display-combi",
    .keycfg             = NULL,
    .keystr             = NULL,
    .modmask            = AnyModifier,
    ._init              = dmenu_mode_init,
    ._get_num_entries   = dmenu_mode_get_num_entries,
    ._result            = NULL,
    ._destroy           = dmenu_mode_free,
    ._token_match       = dmenu_token_match,
    ._get_display_value = get_display_data,
    ._get_completion    = NULL,
    ._is_not_ascii      = dmenu_is_not_ascii,
    .private_data       = NULL,
    .free               = NULL
};

int dmenu_switcher_dialog ( void )
{
    mode_init ( &dmenu_mode );
    MenuFlags            menu_flags      = MENU_NORMAL;
    DmenuModePrivateData *pd             = (DmenuModePrivateData *) dmenu_mode.private_data;
    char                 *input          = NULL;
    int                  retv            = FALSE;
    int                  restart         = FALSE;
    unsigned int         cmd_list_length = pd->cmd_list_length;
    char                 **cmd_list      = pd->cmd_list;

    int                  only_selected = FALSE;
    if ( find_arg ( "-markup-rows" ) >= 0 ) {
        pd->do_markup = TRUE;
    }
    if ( find_arg ( "-only-match" ) >= 0 || find_arg ( "-no-custom" ) >= 0 ) {
        only_selected = TRUE;
        if ( cmd_list_length == 0 ) {
            return TRUE;
        }
    }
    if ( find_arg ( "-password" ) >= 0 ) {
        menu_flags |= MENU_PASSWORD;
    }
    if ( find_arg ( "-normal-window" ) >= 0 ) {
        menu_flags |= MENU_NORMAL_WINDOW;
    }
    /* copy filter string */
    input = g_strdup ( config.filter );

    char *select = NULL;
    find_arg_str ( "-select", &select );
    if ( select != NULL ) {
        char         **tokens = tokenize ( select, config.case_sensitive );
        unsigned int i        = 0;
        for ( i = 0; i < cmd_list_length; i++ ) {
            if ( token_match ( tokens, cmd_list[i], !g_str_is_ascii ( cmd_list[i] ), config.case_sensitive ) ) {
                pd->selected_line = i;
                break;
            }
        }
        g_strfreev ( tokens );
    }
    if ( find_arg ( "-dump" ) >= 0 ) {
        char         **tokens = tokenize ( config.filter ? config.filter : "", config.case_sensitive );
        unsigned int i        = 0;
        for ( i = 0; i < cmd_list_length; i++ ) {
            if ( token_match ( tokens, cmd_list[i], !g_str_is_ascii ( cmd_list[i] ), config.case_sensitive ) ) {
                dmenu_output_formatted_line ( pd->format, cmd_list[i], i, config.filter );
            }
        }
        g_strfreev ( tokens );
        return TRUE;
    }

    RofiViewState *state = rofi_view_create ( &dmenu_mode, input, pd->prompt, pd->message, menu_flags );
    rofi_view_set_selected_line ( state, pd->selected_line );
    while ( XPending ( display ) ) {
        XEvent ev;
        XNextEvent ( display, &ev );
        rofi_view_itterrate ( state, &ev );
    }
    do {
        retv = FALSE;

        rofi_view_set_active ( state );
        // Enter main loop.
        while ( !rofi_view_get_completed ( state )  ) {
            g_main_context_iteration ( NULL, TRUE );
        }
        rofi_view_set_active ( NULL );
        g_free ( input );
        input             = g_strdup ( rofi_view_get_user_input ( state ) );
        pd->selected_line = rofi_view_get_selected_line ( state );;
        MenuReturn   mretv    = rofi_view_get_return_value ( state );
        unsigned int next_pos = rofi_view_get_next_position ( state );

        // Special behavior.
        // TODO clean this up!
        if ( only_selected ) {
            /**
             * Select item mode.
             */
            restart = 1;
            // Skip if no valid item is selected.
            if ( ( mretv & MENU_CANCEL ) == MENU_CANCEL ) {
                // In no custom mode we allow canceling.
                restart = ( find_arg ( "-only-match" ) >= 0 );
            }
            else if ( pd->selected_line != UINT32_MAX ) {
                if ( ( mretv & ( MENU_OK | MENU_QUICK_SWITCH ) ) && cmd_list[pd->selected_line] != NULL ) {
                    dmenu_output_formatted_line ( pd->format, cmd_list[pd->selected_line], pd->selected_line, input );
                    retv = TRUE;
                    if ( ( mretv & MENU_QUICK_SWITCH ) ) {
                        retv = 10 + ( mretv & MENU_LOWER_MASK );
                    }
                    rofi_view_free ( state );
                    g_free ( input );
                    mode_destroy ( &dmenu_mode );
                    return retv;
                }
                pd->selected_line = next_pos - 1;
            }
            // Restart
            rofi_view_restart ( state );
            rofi_view_set_selected_line ( state, pd->selected_line );
            continue;
        }
        // We normally do not want to restart the loop.
        restart = FALSE;
        // Normal mode
        if ( ( mretv & MENU_OK  ) && pd->selected_line != UINT32_MAX && cmd_list[pd->selected_line] != NULL ) {
            dmenu_output_formatted_line ( pd->format, cmd_list[pd->selected_line], pd->selected_line, input );
            if ( ( mretv & MENU_SHIFT ) ) {
                restart = TRUE;
                int seen = FALSE;
                if ( pd->selected_list != NULL ) {
                    if ( pd->selected_list[pd->num_selected_list - 1].stop == ( pd->selected_line - 1 ) ) {
                        pd->selected_list[pd->num_selected_list - 1].stop = pd->selected_line;
                        seen                                              = TRUE;
                    }
                }
                if ( !seen ) {
                    pd->selected_list = g_realloc ( pd->selected_list,
                                                    ( pd->num_selected_list + 1 ) * sizeof ( struct range_pair ) );
                    pd->selected_list[pd->num_selected_list].start = pd->selected_line;
                    pd->selected_list[pd->num_selected_list].stop  = pd->selected_line;
                    ( pd->num_selected_list )++;
                }

                // Move to next line.
                pd->selected_line = MIN ( next_pos, cmd_list_length - 1 );
            }
            retv = TRUE;
        }
        // Custom input
        else if ( ( mretv & ( MENU_CUSTOM_INPUT ) ) ) {
            dmenu_output_formatted_line ( pd->format, input, -1, input );
            if ( ( mretv & MENU_SHIFT ) ) {
                restart = TRUE;
                // Move to next line.
                pd->selected_line = MIN ( next_pos, cmd_list_length - 1 );
            }

            retv = TRUE;
        }
        // Quick switch with entry selected.
        else if ( ( mretv & MENU_QUICK_SWITCH ) && pd->selected_line < UINT32_MAX ) {
            dmenu_output_formatted_line ( pd->format, cmd_list[pd->selected_line], pd->selected_line, input );

            restart = FALSE;
            retv    = 10 + ( mretv & MENU_LOWER_MASK );
        }
        // Quick switch without entry selected.
        else if ( ( mretv & MENU_QUICK_SWITCH ) && pd->selected_line == UINT32_MAX ) {
            dmenu_output_formatted_line ( pd->format, input, -1, input );

            restart = FALSE;
            retv    = 10 + ( mretv & MENU_LOWER_MASK );
        }
        if ( restart ) {
            rofi_view_restart ( state );
            rofi_view_set_selected_line ( state, pd->selected_line );
        }
    } while ( restart );

    rofi_view_free ( state );
    g_free ( input );
    mode_destroy ( &dmenu_mode );
    return retv;
}

void print_dmenu_options ( void )
{
    int is_term = isatty ( fileno ( stdout ) );
    print_help_msg ( "-mesg", "[string]", "Print a small user message under the prompt (uses pango markup)", NULL, is_term );
    print_help_msg ( "-p", "[string]", "Prompt to display left of entry field", NULL, is_term );
    print_help_msg ( "-selected-row", "[integer]", "Select row", NULL, is_term );
    print_help_msg ( "-format", "[string]", "Output format string", "s", is_term );
    print_help_msg ( "-u", "[list]", "List of row indexes to mark urgent", NULL, is_term );
    print_help_msg ( "-a", "[list]", "List of row indexes to mark active", NULL, is_term );
    print_help_msg ( "-l", "[integer] ", "Number of rows to display", NULL, is_term );
    print_help_msg ( "-i", "", "Set filter to be case insensitive", NULL, is_term );
    print_help_msg ( "-only-match", "", "Force selection or custom entry", NULL, is_term );
    print_help_msg ( "-no-custom", "", "Don't accept custom entry", NULL, is_term );
    print_help_msg ( "-select", "[string]", "Select the first row that matches", NULL, is_term );
    print_help_msg ( "-password", "", "Do not show what the user inputs. Show '*' instead.", NULL, is_term );
    print_help_msg ( "-markup-rows", "", "Allow and render pango markup as input data.", NULL, is_term );
}
