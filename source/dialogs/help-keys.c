/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2020 Qball Cow <qball@gmpclient.org>
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

#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>
#include <strings.h>
#include <string.h>
#include <errno.h>

#include "rofi.h"
#include "settings.h"
#include "helper.h"
#include "xrmoptions.h"
#include "dialogs/help-keys.h"
#include "widgets/textbox.h"

typedef struct
{
    char         **messages;
    unsigned int messages_length;
} KeysHelpModePrivateData;

static void get_apps ( KeysHelpModePrivateData *pd )
{
    pd->messages = config_parser_return_display_help ( &( pd->messages_length ) );
}

static int help_keys_mode_init ( Mode *sw )
{
    if ( mode_get_private_data ( sw ) == NULL ) {
        KeysHelpModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
        mode_set_private_data ( sw, (void *) pd );
        get_apps ( pd );
    }
    return TRUE;
}

static ModeMode help_keys_mode_result ( G_GNUC_UNUSED Mode *sw,
                                        int mretv,
                                        G_GNUC_UNUSED char **input,
                                        G_GNUC_UNUSED unsigned int selected_line )
{

    if ( mretv & MENU_CUSTOM_COMMAND ) {
        int retv = ( mretv & MENU_LOWER_MASK );
        return retv;
    }
    return MODE_EXIT;
}
static void help_keys_mode_destroy ( Mode *sw )
{
    KeysHelpModePrivateData *rmpd = (KeysHelpModePrivateData *) mode_get_private_data ( sw );
    if ( rmpd != NULL ) {
        g_strfreev ( rmpd->messages );
        g_free ( rmpd );
        mode_set_private_data ( sw, NULL );
    }
}

static char *_get_display_value ( const Mode *sw, unsigned int selected_line, int *state, G_GNUC_UNUSED GList **list, int get_entry )
{
    KeysHelpModePrivateData *pd = (KeysHelpModePrivateData *) mode_get_private_data ( sw );
    *state |= MARKUP;
    if ( !get_entry ) {
        return NULL;
    }
    return g_strdup ( pd->messages[selected_line] );
}
static int help_keys_token_match ( const Mode *data,
                                   rofi_int_matcher **tokens,
                                   unsigned int index
                                   )
{
    KeysHelpModePrivateData *rmpd = (KeysHelpModePrivateData *) mode_get_private_data ( data );
    return helper_token_match ( tokens, rmpd->messages[index] );
}

static unsigned int help_keys_mode_get_num_entries ( const Mode *sw )
{
    const KeysHelpModePrivateData *pd = (const KeysHelpModePrivateData *) mode_get_private_data ( sw );
    return pd->messages_length;
}

#include "mode-private.h"
Mode help_keys_mode =
{
    .name               = "keys",
    .cfg_name_key       = "display-keys",
    ._init              = help_keys_mode_init,
    ._get_num_entries   = help_keys_mode_get_num_entries,
    ._result            = help_keys_mode_result,
    ._destroy           = help_keys_mode_destroy,
    ._token_match       = help_keys_token_match,
    ._get_completion    = NULL,
    ._get_display_value = _get_display_value,
    .private_data       = NULL,
    .free               = NULL
};
