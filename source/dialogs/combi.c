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
#include <stdlib.h>
#include <stdio.h>
#include <rofi.h>
#include "settings.h"
#include "textbox.h"

#include <dialogs/dialogs.h>

/**
 * Combi Mode
 */
typedef struct
{
    // List of (combined) entries.
    unsigned int cmd_list_length;
    // List to validate where each switcher starts.
    unsigned int *starts;
    unsigned int *lengths;
    // List of switchers to combine.
    unsigned int num_switchers;
    Mode         **switchers;
    Mode         *current;
} CombiModePrivateData;

static void combi_mode_parse_switchers ( Mode *sw )
{
    CombiModePrivateData *pd     = mode_get_private_data ( sw );
    char                 *savept = NULL;
    // Make a copy, as strtok will modify it.
    char                 *switcher_str = g_strdup ( config.combi_modi );
    const char * const   sep           = ",";
    // Split token on ','. This modifies switcher_str.
    for ( char *token = strtok_r ( switcher_str, sep, &savept ); token != NULL;
          token = strtok_r ( NULL, sep, &savept ) ) {
        // Resize and add entry.
        pd->switchers = (Mode * *) g_realloc ( pd->switchers,
                                               sizeof ( Mode* ) * ( pd->num_switchers + 1 ) );

        // Window switcher.
        #ifdef WINDOW_MODE
        if ( strcasecmp ( token, "window" ) == 0 ) {
            pd->switchers[pd->num_switchers++] = &window_mode;
        }
        else if ( strcasecmp ( token, "windowcd" ) == 0 ) {
            pd->switchers[pd->num_switchers++] = &window_mode_cd;
        }
        else
        #endif // WINDOW_MODE
        // SSh dialog
        if ( strcasecmp ( token, "ssh" ) == 0 ) {
            pd->switchers[pd->num_switchers++] = &ssh_mode;
        }
        // Run dialog
        else if ( strcasecmp ( token, "run" ) == 0 ) {
            pd->switchers[pd->num_switchers++] = &run_mode;
        }
        else if ( strcasecmp ( token, "drun" ) == 0 ) {
            pd->switchers[pd->num_switchers++] = &drun_mode;
        }
        else {
            // If not build in, use custom switchers.
            Mode *sw = script_switcher_parse_setup ( token );
            if ( sw != NULL ) {
                pd->switchers[pd->num_switchers++] = sw;
            }
            else{
                // Report error, don't continue.
                fprintf ( stderr, "Invalid script switcher: %s\n", token );
                token = NULL;
            }
        }
        // Keybinding.
    }
    // Free string that was modified by strtok_r
    g_free ( switcher_str );
}

static int combi_mode_init ( Mode *sw )
{
    if ( mode_get_private_data ( sw ) == NULL ) {
        CombiModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
        mode_set_private_data ( sw, (void *) pd );
        combi_mode_parse_switchers ( sw );
        pd->starts  = g_malloc0 ( sizeof ( int ) * pd->num_switchers );
        pd->lengths = g_malloc0 ( sizeof ( int ) * pd->num_switchers );
        for ( unsigned int i = 0; i < pd->num_switchers; i++ ) {
            if ( !mode_init ( pd->switchers[i] ) ) {
                return FALSE;
            }
        }
        if ( pd->cmd_list_length == 0 ) {
            pd->cmd_list_length = 0;
            for ( unsigned int i = 0; i < pd->num_switchers; i++ ) {
                unsigned int length = mode_get_num_entries ( pd->switchers[i] );
                pd->starts[i]        = pd->cmd_list_length;
                pd->lengths[i]       = length;
                pd->cmd_list_length += length;
            }
        }
    }
    return TRUE;
}
static unsigned int combi_mode_get_num_entries ( const Mode *sw )
{
    const CombiModePrivateData *pd = (const CombiModePrivateData *) mode_get_private_data ( sw );
    return pd->cmd_list_length;
}
static void combi_mode_destroy ( Mode *sw )
{
    CombiModePrivateData *pd = (CombiModePrivateData *) mode_get_private_data ( sw );
    if ( pd != NULL ) {
        g_free ( pd->starts );
        g_free ( pd->lengths );
        // Cleanup switchers.
        for ( unsigned int i = 0; i < pd->num_switchers; i++ ) {
            mode_destroy ( pd->switchers[i] );
        }
        g_free ( pd->switchers );
        g_free ( pd );
        mode_set_private_data ( sw, NULL );
    }
}
static ModeMode combi_mode_result ( Mode *sw, int mretv, char **input, unsigned int selected_line )
{
    CombiModePrivateData *pd = mode_get_private_data ( sw );
    if ( *input[0] == '!' ) {
        int switcher = -1;
        for ( unsigned i = 0; switcher == -1 && i < pd->num_switchers; i++ ) {
            if ( ( *input )[1] == mode_get_name ( pd->switchers[i] )[0] ) {
                switcher = i;
            }
        }
        if ( switcher >= 0  ) {
            char *n = strchr ( *input, ' ' );
            // skip whitespace
            if ( n != NULL ) {
                n++;
                return mode_result ( pd->switchers[switcher], mretv, &n,
                                     selected_line - pd->starts[switcher] );
            }
            return MODE_EXIT;
        }
    }

    for ( unsigned i = 0; i < pd->num_switchers; i++ ) {
        if ( selected_line >= pd->starts[i] &&
             selected_line < ( pd->starts[i] + pd->lengths[i] ) ) {
            return mode_result ( pd->switchers[i], mretv, input, selected_line - pd->starts[i] );
        }
    }
    return MODE_EXIT;
}
static int combi_mode_match ( const Mode *sw, GRegex **tokens, unsigned int index )
{
    CombiModePrivateData *pd = mode_get_private_data ( sw );
    for ( unsigned i = 0; i < pd->num_switchers; i++ ) {
        if ( pd->current != NULL && pd->switchers[i] != pd->current ) {
            continue;
        }
        if ( index >= pd->starts[i] && index < ( pd->starts[i] + pd->lengths[i] ) ) {
            return mode_token_match ( pd->switchers[i], tokens, index - pd->starts[i] );
        }
    }
    return 0;
}
static char * combi_mgrv ( const Mode *sw, unsigned int selected_line, int *state, int get_entry )
{
    CombiModePrivateData *pd = mode_get_private_data ( sw );
    if ( !get_entry ) {
        for ( unsigned i = 0; i < pd->num_switchers; i++ ) {
            if ( selected_line >= pd->starts[i] && selected_line < ( pd->starts[i] + pd->lengths[i] ) ) {
                mode_get_display_value ( pd->switchers[i], selected_line - pd->starts[i], state, FALSE );
                return NULL;
            }
        }
        return NULL;
    }
    for ( unsigned i = 0; i < pd->num_switchers; i++ ) {
        if ( selected_line >= pd->starts[i] && selected_line < ( pd->starts[i] + pd->lengths[i] ) ) {
            char * str = mode_get_display_value ( pd->switchers[i], selected_line - pd->starts[i], state, TRUE );
            if ( !( *state & MARKUP ) ) {
                g_debug ( "ESCAPE" );
                char *tmp = str;
                str = g_markup_escape_text ( tmp, -1 );
                g_free ( tmp );
                *state |= MARKUP;
            }
            char * retv = g_strdup_printf ( "%s %s%s", mode_get_display_name ( pd->switchers[i] ), str, mode_get_display_close ( pd->switchers[i] ) );
            g_free ( str );
            return retv;
        }
    }

    return NULL;
}
static char * combi_get_completion ( const Mode *sw, unsigned int index )
{
    CombiModePrivateData *pd = mode_get_private_data ( sw );
    for ( unsigned i = 0; i < pd->num_switchers; i++ ) {
        if ( index >= pd->starts[i] && index < ( pd->starts[i] + pd->lengths[i] ) ) {
            char *comp  = mode_get_completion ( pd->switchers[i], index - pd->starts[i] );
            char *mcomp = g_strdup_printf ( "!%c %s", mode_get_name ( pd->switchers[i] )[0], comp );
            g_free ( comp );
            return mcomp;
        }
    }
    // Should never get here.
    g_error ( "Failure, could not resolve sub-switcher." );
    return NULL;
}

static char * combi_preprocess_input ( Mode *sw, const char *input )
{
    CombiModePrivateData *pd = mode_get_private_data ( sw );
    pd->current = NULL;
    if ( input != NULL && input[0] == '!' && strlen ( input ) > 1 ) {
        for ( unsigned i = 0; i < pd->num_switchers; i++ ) {
            if ( input[1] == mode_get_name ( pd->switchers[i] )[0] ) {
                pd->current = pd->switchers[i];
                if ( input[2] == '\0' ) {
                    return NULL;
                }
                return g_strdup ( &input[2] );
            }
        }
    }
    return g_strdup ( input );
}

#include "mode-private.h"
Mode combi_mode =
{
    .name               = "combi",
    .cfg_name_key       = "display-combi",
    ._init              = combi_mode_init,
    ._get_num_entries   = combi_mode_get_num_entries,
    ._result            = combi_mode_result,
    ._destroy           = combi_mode_destroy,
    ._token_match       = combi_mode_match,
    ._get_completion    = combi_get_completion,
    ._get_display_value = combi_mgrv,
    ._preprocess_input  = combi_preprocess_input,
    .private_data       = NULL,
    .free               = NULL
};
