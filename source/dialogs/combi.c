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
#include <stdlib.h>
#include <stdio.h>
#include <rofi.h>

extern unsigned int num_switchers;
extern Switcher     **switchers;
/**
 * Combi Switcher
 */
typedef struct _CombiModePrivateData
{
    // List of (combined) entries.
    char         **cmd_list;
    unsigned int cmd_list_length;
    // List to validate where each switcher starts.
    unsigned int *starts;
    unsigned int *lengths;
    // List of switchers to combine.
    unsigned int num_switchers;
    Switcher     **switchers;
} CombiModePrivateData;

static void combi_add_switcher ( Switcher *sw, Switcher *m )
{
    CombiModePrivateData *pd = sw->private_data;
    pd->switchers                        = g_realloc ( pd->switchers, ( pd->num_switchers + 2 ) * sizeof ( Switcher * ) );
    pd->switchers[pd->num_switchers]     = m;
    pd->switchers[pd->num_switchers + 1] = NULL;
    pd->num_switchers++;
}
static void combi_mode_init ( Switcher *sw )
{
    if ( sw->private_data == NULL ) {
        CombiModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
        sw->private_data = (void *) pd;
        for ( unsigned int i = 0; i < num_switchers; i++ ) {
            if ( switchers[i] != sw ) {
                fprintf ( stderr, "Adding: %s\n", switchers[i]->name );
                combi_add_switcher ( sw, switchers[i] );
            }
        }
        pd->starts  = g_malloc0 ( sizeof ( int ) * pd->num_switchers );
        pd->lengths = g_malloc0 ( sizeof ( int ) * pd->num_switchers );
    }
}
static char ** combi_mode_get_data ( unsigned int *length, Switcher *sw )
{
    CombiModePrivateData *pd = sw->private_data;
    if ( pd->cmd_list == NULL ) {
        pd->cmd_list_length = 0;
        for ( unsigned int i = 0; i < pd->num_switchers; i++ ) {
            unsigned int length = 0;
            pd->switchers[i]->get_data ( &length, pd->switchers[i] );
            pd->starts[i]  = pd->cmd_list_length;
            pd->lengths[i] = length;
            pd->cmd_list_length += length;
        }
        // Fill the list.
        pd->cmd_list = g_malloc0 ( ( pd->cmd_list_length + 1 ) * sizeof ( char* ) );
        for ( unsigned i = 0; i < pd->num_switchers; i++ ) {
            unsigned int length = 0;
            char         **retv = pd->switchers[i]->get_data ( &length, pd->switchers[i] );
            for ( unsigned int j = 0; j < length; j++ ) {
                pd->cmd_list[j + pd->starts[i]] = retv[j];
            }
        }
    }
    *length = pd->cmd_list_length;
    return pd->cmd_list;
}
static void combi_mode_destroy ( Switcher *sw )
{
    CombiModePrivateData *pd = (CombiModePrivateData *) sw->private_data;
    if ( pd != NULL ) {
        g_free ( pd->cmd_list );
        g_free ( pd->starts );
        g_free ( pd->lengths );
g_free(pd->switchers);
        g_free ( pd );
        sw->private_data = NULL;
    }
}
static SwitcherMode combi_mode_result ( int mretv, char **input, unsigned int selected_line, Switcher *sw )
{
    CombiModePrivateData *pd = sw->private_data;

    for ( unsigned i = 0; i < pd->num_switchers; i++ ) {
        if ( selected_line >= pd->starts[i] &&
             selected_line < ( pd->starts[i] + pd->lengths[i] ) ) {
            return pd->switchers[i]->result ( mretv,
                                              input,
                                              selected_line - pd->starts[i],
                                              pd->switchers[i] );
        }
    }
    return MODE_EXIT;
}
static int combi_mode_match ( char **tokens, const char *input,
                              int case_sensitive, unsigned int index, Switcher *sw )
{
    CombiModePrivateData *pd = sw->private_data;

    for ( unsigned i = 0; i < pd->num_switchers; i++ ) {
        if ( index >= pd->starts[i] && index < ( pd->starts[i] + pd->lengths[i] ) ) {
            return pd->switchers[i]->token_match ( tokens,
                                                   input,
                                                   case_sensitive,
                                                   index - pd->starts[i],
                                                   pd->switchers[i] );
        }
    }
    abort ();
    return 0;
}

Switcher combi_mode =
{
    .name         = "combi",
    .tb           = NULL,
    .keycfg       = NULL,
    .keystr       = NULL,
    .modmask      = AnyModifier,
    .init         = combi_mode_init,
    .get_data     = combi_mode_get_data,
    .result       = combi_mode_result,
    .destroy      = combi_mode_destroy,
    .token_match  = combi_mode_match,
    .private_data = NULL,
    .free         = NULL
};
