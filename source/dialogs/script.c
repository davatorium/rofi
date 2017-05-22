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

#define START_OF_HEADING '\x01'
#define START_OF_TEXT    '\x02'
#define UNIT_SEPARATOR   '\x1F'

typedef enum {
    PARSING_ENTRIES = 0,
    PARSING_HEADER  = 1

} ParserMode;

typedef struct
{
    unsigned int id;

    ParserMode  pmode;
    char         **cmd_list;
    unsigned int cmd_list_length;
    /** Display msg */
    char *message;
    char *prompt;
    char *format;
    gboolean markup_rows;
} ScriptModePrivateData;

static void parse_header ( Mode *sw, ScriptModePrivateData *pd, char *buffer, const ssize_t buffer_length )
{
    if ( *buffer == START_OF_TEXT )
    {
        pd->pmode = PARSING_ENTRIES;
        return;
    }
    char *unit_sep = strchr ( buffer, UNIT_SEPARATOR);
    if ( unit_sep )
    {
        const char *command = buffer;
        const char *value = unit_sep+1;
        *unit_sep = '\0';
        if ( strcmp ( command, "message" ) == 0 ) {
            if ( pd->message ) g_free ( pd->message );
            pd->message = g_strdup(value);    
        } 
        else if ( strcmp ( command, "prompt" ) == 0 ) {
            if ( pd->prompt ) g_free ( pd->prompt );
            pd->prompt = g_strdup(value);    
            sw->display_name = pd->prompt; 
        }
        else if ( strcmp ( command, "markup_rows" ) == 0 ){
            pd->markup_rows = strcasecmp( value, "true") == 0; 
        } 
        else if ( strcmp ( command, "format" ) == 0 ) {
            if ( pd->format ) g_free ( pd->format );
            pd->format = g_strdup(value);    
        }
    }
}

static char **get_script_output ( Mode *sw, ScriptModePrivateData *pd, const char *command, unsigned int *length )
{
    // Default to parsing entries.
    pd->pmode = PARSING_ENTRIES;

    char **retv = NULL;

    *length = 0;
    unsigned int actual_length = 0;
    int fd = execute_generator ( command );
    if ( fd >= 0 ) {
        FILE *inp = fdopen ( fd, "r" );
        if ( inp ) {
            char   *buffer       = NULL;
            size_t buffer_length = 0;
            ssize_t read_length = 0;
            while ( (read_length = getline ( &buffer, &buffer_length, inp )) > 0 ) {
                if ( buffer[read_length-1] == '\n' ){
                    buffer[read_length-1] = '\0';
                   read_length--; 
                }
                if ( buffer[0] == START_OF_HEADING ) {
                    pd->pmode = PARSING_HEADER;
                    parse_header( sw, pd, buffer+1, read_length);
                    continue;
                } 
                if ( pd->pmode == PARSING_HEADER ){
                    parse_header(sw, pd, buffer, read_length);
                    continue; 
                }

                if ( actual_length < ((*length)+2))  {
                    actual_length += 128;     
                    retv           = g_realloc ( retv, ( actual_length ) * sizeof ( char* ) );
                }

                retv[( *length )]     = g_strdup ( buffer );
                retv[( *length ) + 1] = NULL;


                ( *length )++;
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

static char **execute_executor ( Mode *sw, const char *result, unsigned int *length )
{
    ScriptModePrivateData *pd = (ScriptModePrivateData *) sw->private_data;
    char *arg     = g_shell_quote ( result );
    char *command = g_strdup_printf ( "%s %s", (const char *) sw->ed, arg );
    char **retv   = get_script_output ( sw, pd, command, length );
    g_free ( command );
    g_free ( arg );
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
        pd->cmd_list     = get_script_output ( sw,pd, (const char *) sw->ed, &( pd->cmd_list_length ) );
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
        g_free ( rmpd );
        sw->private_data = NULL;
    }
}
static char *_get_display_value ( const Mode *sw, unsigned int selected_line, G_GNUC_UNUSED int *state, G_GNUC_UNUSED GList **list, int get_entry )
{
    ScriptModePrivateData *rmpd = sw->private_data;
    if ( rmpd->markup_rows ) {
            *state |= MARKUP;
    }
    return get_entry ? g_strdup ( rmpd->cmd_list[selected_line] ) : NULL;
}

static int script_token_match ( const Mode *sw, GRegex **tokens, unsigned int index )
{
    ScriptModePrivateData *rmpd = sw->private_data;
    return helper_token_match ( tokens, rmpd->cmd_list[index] );
}
static char *script_get_message ( const Mode *sw )
{
    ScriptModePrivateData *pd = (ScriptModePrivateData *) mode_get_private_data ( sw );
    if ( pd->message ) {
        return g_strdup ( pd->message );
    }
    return NULL;
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
        sw->_get_completion    = NULL;
        sw->_preprocess_input  = NULL;
        sw->_get_display_value = _get_display_value;
        sw->_get_message       = script_get_message;

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
