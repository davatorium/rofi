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
#include <assert.h>
#include <errno.h>
#include "rofi.h"
#include "dialogs/script.h"
#include "helper.h"

static char **get_script_output ( const char *command, unsigned int *length )
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
            if ( fclose ( inp ) != 0 ) {
                fprintf ( stderr, "Failed to close stdout off executor script: '%s'\n",
                          strerror ( errno ) );
            }
        }
    }
    return retv;
}

static char **execute_executor ( Switcher *sw, const char *result, unsigned int *length )
{
    char **retv = NULL;

    char *arg     = g_shell_quote ( result );
    char *command = g_strdup_printf ( "%s %s", (const char *) sw->ed, arg );
    retv = get_script_output ( command, length );
    g_free ( command );
    g_free ( arg );
    return retv;
}

static void script_switcher_free ( Switcher *sw )
{
    if ( sw == NULL ) {
        return;
    }
    g_free ( sw->ed );
    g_free ( sw );
}

typedef struct _ScriptModePrivateData
{
    unsigned int id;
    char         **cmd_list;
    unsigned int cmd_list_length;
} ScriptModePrivateData;

static void script_mode_init ( Switcher *sw )
{
    if ( sw->private_data == NULL ) {
        ScriptModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
        sw->private_data = (void *) pd;
        pd->cmd_list     = get_script_output ( (const char *) sw->ed, &( pd->cmd_list_length ) );
    }
}
static unsigned int script_mode_get_num_entries ( const Switcher *sw )
{
    const ScriptModePrivateData *rmpd = (const ScriptModePrivateData *) sw->private_data;
    return rmpd->cmd_list_length;
}

static SwitcherMode script_mode_result ( Switcher *sw, int mretv, char **input, unsigned int selected_line )
{
    ScriptModePrivateData *rmpd      = (ScriptModePrivateData *) sw->private_data;
    SwitcherMode          retv       = MODE_EXIT;
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
        g_free ( *input );
        *input = NULL;
        retv   = RELOAD_DIALOG;
    }
    return retv;
}

static void script_mode_destroy ( Switcher *sw )
{
    ScriptModePrivateData *rmpd = (ScriptModePrivateData *) sw->private_data;
    if ( rmpd != NULL ) {
        g_strfreev ( rmpd->cmd_list );
        g_free ( rmpd );
        sw->private_data = NULL;
    }
}
static char *mgrv ( const Switcher *sw, unsigned int selected_line, G_GNUC_UNUSED int *state, int get_entry )
{
    ScriptModePrivateData *rmpd = sw->private_data;
    return get_entry ? g_strdup ( rmpd->cmd_list[selected_line] ) : NULL;
}

static int script_token_match ( const Switcher *sw, char **tokens, int not_ascii, int case_sensitive, unsigned int index )
{
    ScriptModePrivateData *rmpd = sw->private_data;
    return token_match ( tokens, rmpd->cmd_list[index], not_ascii, case_sensitive );
}

static int script_is_not_ascii ( const Switcher *sw, unsigned int index )
{
    ScriptModePrivateData *rmpd = sw->private_data;
    return is_not_ascii ( rmpd->cmd_list[index] );
}

Switcher *script_switcher_parse_setup ( const char *str )
{
    Switcher     *sw    = g_malloc0 ( sizeof ( *sw ) );
    char         *endp  = NULL;
    char         *parse = g_strdup ( str );
    unsigned int index  = 0;
    for ( char *token = strtok_r ( parse, ":", &endp ); token != NULL; token = strtok_r ( NULL, ":", &endp ) ) {
        if ( index == 0 ) {
            g_strlcpy ( sw->name, token, 32 );
        }
        else if ( index == 1 ) {
            sw->ed = (void *) g_strdup ( token );
        }
        index++;
    }
    g_free ( parse );
    if ( index == 2 ) {
        sw->free            = script_switcher_free;
        sw->keysym          = None;
        sw->modmask         = AnyModifier;
        sw->init            = script_mode_init;
        sw->get_num_entries = script_mode_get_num_entries;
        sw->result          = script_mode_result;
        sw->destroy         = script_mode_destroy;
        sw->token_match     = script_token_match;
        sw->get_completion  = NULL,
        sw->mgrv            = mgrv;
        sw->is_not_ascii    = script_is_not_ascii;

        return sw;
    }
    fprintf ( stderr, "The script command '%s' has %u options, but needs 2: <name>:<script>.\n", str, index );
    script_switcher_free ( sw );
    return NULL;
}

