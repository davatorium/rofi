/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2017 Qball Cow <qball@gmpclient.org>
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

#define G_LOG_DOMAIN    "Dialogs.Script"

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include "rofi.h"
#include "dialogs/script.h"
#include "helper.h"

#include "widgets/textbox.h"

#include "mode-private.h"

typedef struct
{
    /** ID of the current script. */
    unsigned int           id;
    /** List of visible items. */
    char                   **cmd_list;
    /** length list of visible items. */
    unsigned int           cmd_list_length;

    /** Urgent list */
    struct rofi_range_pair * urgent_list;
    unsigned int           num_urgent_list;
    /** Active list */
    struct rofi_range_pair * active_list;
    unsigned int           num_active_list;
    /** Configuration settings. */
    char                   *message;
    char                   *prompt;
    gboolean               do_markup;
} ScriptModePrivateData;

static void parse_header_entry ( Mode *sw, char *line, ssize_t length )
{
    ScriptModePrivateData *pd        = (ScriptModePrivateData *) sw->private_data;
    ssize_t               length_key = 0;//strlen ( line );
    while ( line[length_key] != '\x1f' && length_key <= length ) {
        length_key++;
    }

    if ( length_key < length ) {
        line[length_key] = '\0';
        char *value = line + length_key + 1;
        if ( strcasecmp ( line, "message" ) == 0 ) {
            g_free ( pd->message );
            pd->message = g_strdup ( value );
        }
        else if ( strcasecmp ( line, "prompt" ) == 0 ) {
            g_free ( pd->prompt );
            pd->prompt       = g_strdup  ( value );
            sw->display_name = pd->prompt;
        }
        else if ( strcasecmp ( line, "markup-rows" ) == 0 ) {
            pd->do_markup = ( strcasecmp ( value, "true" ) == 0 );
        }
        else if ( strcasecmp ( line, "urgent" ) == 0 ) {
            parse_ranges ( value, &( pd->urgent_list ), &( pd->num_urgent_list ) );
        }
        else if ( strcasecmp ( line, "active" ) == 0 ) {
            parse_ranges ( value, &( pd->active_list ), &( pd->num_active_list ) );
        }
    }
}

static char **get_script_output ( Mode *sw, char *command, char *arg, unsigned int *length )
{
    size_t actual_size = 0;
    int    fd          = -1;
    GError *error      = NULL;
    char   **retv      = NULL;
    char   **argv      = NULL;
    int    argc        = 0;
    *length = 0;
    if ( g_shell_parse_argv ( command, &argc, &argv, &error ) ) {
        argv           = g_realloc ( argv, ( argc + 2 ) * sizeof ( char* ) );
        argv[argc]     = g_strdup ( arg );
        argv[argc + 1] = NULL;
        g_spawn_async_with_pipes ( NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL, &fd, NULL, &error );
    }
    if ( error != NULL ) {
        char *msg = g_strdup_printf ( "Failed to execute: '%s'\nError: '%s'", command, error->message );
        rofi_view_error_dialog ( msg, FALSE );
        g_free ( msg );
        // print error.
        g_error_free ( error );
        fd = -1;
    }
    if ( fd >= 0 ) {
        FILE *inp = fdopen ( fd, "r" );
        if ( inp ) {
            char    *buffer       = NULL;
            size_t  buffer_length = 0;
            ssize_t read_length   = 0;
            while ( ( read_length = getline ( &buffer, &buffer_length, inp ) ) > 0 ) {
                // Filter out line-end.
                if ( buffer[read_length - 1] == '\n' ) {
                    buffer[read_length - 1] = '\0';
                }
                if ( buffer[0] == '\0' ) {
                    parse_header_entry ( sw, &buffer[1], read_length - 1 );
                }
                else {
                    if ( actual_size < ( ( *length ) + 2 ) ) {
                        actual_size += 256;
                        retv         = g_realloc ( retv, ( actual_size ) * sizeof ( char* ) );
                    }
                    retv[( *length )]     = g_strdup ( buffer );
                    retv[( *length ) + 1] = NULL;
                    ( *length )++;
                }
            }
            if ( buffer ) {
                free ( buffer );
            }
            if ( fclose ( inp ) != 0 ) {
                g_warning ( "Failed to close stdout off executor script: '%s'",
                            g_strerror ( errno ) );
            }
        }
    }
    return retv;
}

static char **execute_executor ( Mode *sw, char *result, unsigned int *length )
{
    char **retv = get_script_output ( sw, sw->ed, result, length );
    return retv;
}

static void script_switcher_free ( Mode *sw )
{
    if ( sw == NULL ) {
        return;
    }
    g_free ( sw->name );
    g_free ( sw->ed );
    g_free ( sw );
}

static int script_mode_init ( Mode *sw )
{
    if ( sw->private_data == NULL ) {
        ScriptModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
        sw->private_data = (void *) pd;
        pd->cmd_list     = get_script_output ( sw, (char *) sw->ed, NULL, &( pd->cmd_list_length ) );
    }
    return TRUE;
}
static unsigned int script_mode_get_num_entries ( const Mode *sw )
{
    const ScriptModePrivateData *rmpd = (const ScriptModePrivateData *) sw->private_data;
    return rmpd->cmd_list_length;
}

static ModeMode script_mode_result ( Mode *sw, int mretv, char **input, unsigned int selected_line )
{
    ScriptModePrivateData *rmpd      = (ScriptModePrivateData *) sw->private_data;
    ModeMode              retv       = MODE_EXIT;
    char                  **new_list = NULL;
    unsigned int          new_length = 0;

    if ( ( mretv & MENU_NEXT ) ) {
        retv = NEXT_DIALOG;
    }
    else if ( ( mretv & MENU_PREVIOUS ) ) {
        retv = PREVIOUS_DIALOG;
    }
    else if ( ( mretv & MENU_QUICK_SWITCH ) ) {
        retv = ( mretv & MENU_LOWER_MASK );
    }
    else if ( ( mretv & MENU_OK ) && rmpd->cmd_list[selected_line] != NULL ) {
        new_list = execute_executor ( sw, rmpd->cmd_list[selected_line], &new_length );
    }
    else if ( ( mretv & MENU_CUSTOM_INPUT ) && *input != NULL && *input[0] != '\0' ) {
        new_list = execute_executor ( sw, *input, &new_length );
    }

    // If a new list was generated, use that an loop around.
    if ( new_list != NULL ) {
        g_strfreev ( rmpd->cmd_list );

        rmpd->cmd_list        = new_list;
        rmpd->cmd_list_length = new_length;
        retv                  = RESET_DIALOG;
    }
    return retv;
}

static void script_mode_destroy ( Mode *sw )
{
    ScriptModePrivateData *rmpd = (ScriptModePrivateData *) sw->private_data;
    if ( rmpd != NULL ) {
        g_strfreev ( rmpd->cmd_list );
        g_free ( rmpd->message );
        g_free ( rmpd->prompt );
        g_free ( rmpd->urgent_list );
        g_free ( rmpd->active_list );
        g_free ( rmpd );
        sw->private_data = NULL;
    }
}
static char *_get_display_value ( const Mode *sw, unsigned int selected_line, G_GNUC_UNUSED int *state, G_GNUC_UNUSED GList **list, int get_entry )
{
    ScriptModePrivateData *pd = sw->private_data;
    for ( unsigned int i = 0; i < pd->num_active_list; i++ ) {
        if ( selected_line >= pd->active_list[i].start && selected_line <= pd->active_list[i].stop ) {
            *state |= ACTIVE;
        }
    }
    for ( unsigned int i = 0; i < pd->num_urgent_list; i++ ) {
        if ( selected_line >= pd->urgent_list[i].start && selected_line <= pd->urgent_list[i].stop ) {
            *state |= URGENT;
        }
    }
    if ( pd->do_markup ) {
        *state |= MARKUP;
    }
    return get_entry ? g_strdup ( pd->cmd_list[selected_line] ) : NULL;
}

static int script_token_match ( const Mode *sw, rofi_int_matcher **tokens, unsigned int index )
{
    ScriptModePrivateData *rmpd = sw->private_data;
    return helper_token_match ( tokens, rmpd->cmd_list[index] );
}
static char *script_get_message ( const Mode *sw )
{
    ScriptModePrivateData *pd = sw->private_data;
    return g_strdup ( pd->message );
}

#include "mode-private.h"
Mode *script_switcher_parse_setup ( const char *str )
{
    Mode              *sw    = g_malloc0 ( sizeof ( *sw ) );
    char              *endp  = NULL;
    char              *parse = g_strdup ( str );
    unsigned int      index  = 0;
    const char *const sep    = ":";
    for ( char *token = strtok_r ( parse, sep, &endp ); token != NULL; token = strtok_r ( NULL, sep, &endp ) ) {
        if ( index == 0 ) {
            sw->name = g_strdup ( token );
        }
        else if ( index == 1 ) {
            sw->ed = (void *) rofi_expand_path ( token );
        }
        index++;
    }
    g_free ( parse );
    if ( index == 2 ) {
        sw->free               = script_switcher_free;
        sw->_init              = script_mode_init;
        sw->_get_num_entries   = script_mode_get_num_entries;
        sw->_result            = script_mode_result;
        sw->_destroy           = script_mode_destroy;
        sw->_token_match       = script_token_match;
        sw->_get_message       = script_get_message;
        sw->_get_completion    = NULL,
        sw->_preprocess_input  = NULL,
        sw->_get_display_value = _get_display_value;

        return sw;
    }
    fprintf ( stderr, "The script command '%s' has %u options, but needs 2: <name>:<script>.", str, index );
    script_switcher_free ( sw );
    return NULL;
}

gboolean script_switcher_is_valid ( const char *token )
{
    return strchr ( token, ':' ) != NULL;
}
