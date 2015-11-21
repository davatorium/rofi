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
#include <stdlib.h>
#include <stdio.h>
#include <rofi.h>

#include <dialogs/dialogs.h>

/**
 * Combi Switcher
 */
typedef struct _CombiModePrivateData
{
    // List of (combined) entries.
    unsigned int cmd_list_length;
    // List to validate where each switcher starts.
    unsigned int *starts;
    unsigned int *lengths;
    // List of switchers to combine.
    unsigned int num_switchers;
    Switcher     **switchers;
} CombiModePrivateData;

static void combi_mode_parse_switchers ( Switcher *sw )
{
    CombiModePrivateData *pd     = sw->private_data;
    char                 *savept = NULL;
    // Make a copy, as strtok will modify it.
    char                 *switcher_str = g_strdup ( config.combi_modi );
    // Split token on ','. This modifies switcher_str.
    for ( char *token = strtok_r ( switcher_str, ",", &savept ); token != NULL;
          token = strtok_r ( NULL, ",", &savept ) ) {
        // Resize and add entry.
        pd->switchers = (Switcher * *) g_realloc ( pd->switchers,
                                                   sizeof ( Switcher* ) * ( pd->num_switchers + 1 ) );

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
            Switcher *sw = script_switcher_parse_setup ( token );
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

static void combi_mode_init ( Switcher *sw )
{
    if ( sw->private_data == NULL ) {
        CombiModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
        sw->private_data = (void *) pd;
        combi_mode_parse_switchers ( sw );
        pd->starts  = g_malloc0 ( sizeof ( int ) * pd->num_switchers );
        pd->lengths = g_malloc0 ( sizeof ( int ) * pd->num_switchers );
        for ( unsigned int i = 0; i < pd->num_switchers; i++ ) {
            pd->switchers[i]->init ( pd->switchers[i] );
        }
        if ( pd->cmd_list_length == 0 ) {
            pd->cmd_list_length = 0;
            for ( unsigned int i = 0; i < pd->num_switchers; i++ ) {
                unsigned int length = pd->switchers[i]->get_num_entries ( pd->switchers[i] );;
                pd->starts[i]        = pd->cmd_list_length;
                pd->lengths[i]       = length;
                pd->cmd_list_length += length;
            }
        }
    }
}
static unsigned int combi_mode_get_num_entries ( Switcher *sw )
{
    CombiModePrivateData *pd = sw->private_data;
    return pd->cmd_list_length;
}
static void combi_mode_destroy ( Switcher *sw )
{
    CombiModePrivateData *pd = (CombiModePrivateData *) sw->private_data;
    if ( pd != NULL ) {
        g_free ( pd->starts );
        g_free ( pd->lengths );
        // Cleanup switchers.
        for ( unsigned int i = 0; i < pd->num_switchers; i++ ) {
            pd->switchers[i]->destroy ( pd->switchers[i] );
        }
        g_free ( pd->switchers );
        g_free ( pd );
        sw->private_data = NULL;
    }
}
static SwitcherMode combi_mode_result ( int mretv, char **input, unsigned int selected_line,
                                        Switcher *sw )
{
    CombiModePrivateData *pd = sw->private_data;
    if ( *input[0] == '!' ) {
        int switcher = -1;
        for ( unsigned i = 0; switcher == -1 && i < pd->num_switchers; i++ ) {
            if ( ( *input )[1] == pd->switchers[i]->name[0] ) {
                switcher = i;
            }
        }
        if ( switcher >= 0  ) {
            char *n = strchr ( *input, ' ' );
            // skip whitespace
            if ( n != NULL ) {
                n++;
                return pd->switchers[switcher]->result ( mretv, &n,
                                                         selected_line - pd->starts[switcher],
                                                         pd->switchers[switcher] );
            }
            return MODE_EXIT;
        }
    }

    for ( unsigned i = 0; i < pd->num_switchers; i++ ) {
        if ( selected_line >= pd->starts[i] &&
             selected_line < ( pd->starts[i] + pd->lengths[i] ) ) {
            return pd->switchers[i]->result ( mretv, input, selected_line - pd->starts[i],
                                              pd->switchers[i] );
        }
    }
    return MODE_EXIT;
}
static int combi_mode_match ( char **tokens, int not_ascii,
                              int case_sensitive, unsigned int index, Switcher *sw )
{
    CombiModePrivateData *pd = sw->private_data;

    for ( unsigned i = 0; i < pd->num_switchers; i++ ) {
        if ( index >= pd->starts[i] && index < ( pd->starts[i] + pd->lengths[i] ) ) {
            if ( tokens && tokens[0][0] == '!' ) {
                if ( tokens[0][1] == pd->switchers[i]->name[0] ) {
                    return pd->switchers[i]->token_match ( &tokens[1], not_ascii, case_sensitive,
                                                           index - pd->starts[i], pd->switchers[i] );
                }
                return 0;
            }
            else {
                return pd->switchers[i]->token_match ( tokens, not_ascii, case_sensitive,
                                                       index - pd->starts[i], pd->switchers[i] );
            }
        }
    }
    abort ();
    return 0;
}
static char * combi_mgrv ( unsigned int selected_line, Switcher *sw, int *state, int get_entry )
{
    CombiModePrivateData *pd = sw->private_data;
    if ( !get_entry ) {
        for ( unsigned i = 0; i < pd->num_switchers; i++ ) {
            if ( selected_line >= pd->starts[i] && selected_line < ( pd->starts[i] + pd->lengths[i] ) ) {
                pd->switchers[i]->mgrv ( selected_line - pd->starts[i], (void *) pd->switchers[i], state, FALSE );
                return NULL;
            }
        }
        return NULL;
    }
    for ( unsigned i = 0; i < pd->num_switchers; i++ ) {
        if ( selected_line >= pd->starts[i] && selected_line < ( pd->starts[i] + pd->lengths[i] ) ) {
            char * str  = pd->switchers[i]->mgrv ( selected_line - pd->starts[i], (void *) pd->switchers[i], state, TRUE );
            char * retv = g_strdup_printf ( "(%s) %s", pd->switchers[i]->name, str );
            g_free ( str );
            return retv;
        }
    }

    return NULL;
}
static int combi_is_not_ascii ( Switcher *sw, unsigned int index )
{
    CombiModePrivateData *pd = sw->private_data;
    for ( unsigned i = 0; i < pd->num_switchers; i++ ) {
        if ( index >= pd->starts[i] && index < ( pd->starts[i] + pd->lengths[i] ) ) {
            return pd->switchers[i]->is_not_ascii ( pd->switchers[i], index - pd->starts[i] );
        }
    }
    return FALSE;
}

Switcher combi_mode =
{
    .name            = "combi",
    .keycfg          = NULL,
    .keystr          = NULL,
    .modmask         = AnyModifier,
    .init            = combi_mode_init,
    .get_num_entries = combi_mode_get_num_entries,
    .result          = combi_mode_result,
    .destroy         = combi_mode_destroy,
    .token_match     = combi_mode_match,
    .mgrv            = combi_mgrv,
    .is_not_ascii    = combi_is_not_ascii,
    .private_data    = NULL,
    .free            = NULL
};
