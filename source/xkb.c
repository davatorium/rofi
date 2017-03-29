/**
 *   MIT/X11 License
 *   Modified  (c) 2017 Quentin “Sardem FF7” Glidic
 *
 *   Permission is hereby granted, free of charge, to any person obtaining
 *   a copy of this software and associated documentation files (the
 *   "Software"), to deal in the Software without restriction, including
 *   without limitation the rights to use, copy, modify, merge, publish,
 *   distribute, sublicense, and/or sell copies of the Software, and to
 *   permit persons to whom the Software is furnished to do so, subject to
 *   the following conditions:
 *
 *   The above copyright notice and this permission notice shall be
 *   included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <locale.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <cairo.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>

#include "xkb.h"

static void xkb_find_mod_mask ( xkb_stuff *xkb, widget_modifier mod, ... )
{
    va_list         names;
    const char      *name;
    xkb_mod_index_t i, *m = xkb->mods[mod];
    va_start ( names, mod );
    while ( ( name = va_arg ( names, const char * ) ) != NULL ) {
        i = xkb_keymap_mod_get_index ( xkb->keymap, name );
        if ( i != XKB_MOD_INVALID ) {
            *m++ = i;
        }
    }
    *m = XKB_MOD_INVALID;
    va_end ( names );
}

void xkb_common_init ( xkb_stuff *xkb )
{
    xkb->compose.table = xkb_compose_table_new_from_locale ( xkb->context, setlocale ( LC_CTYPE, NULL ), 0 );
    if ( xkb->compose.table != NULL ) {
        xkb->compose.state = xkb_compose_state_new ( xkb->compose.table, 0 );
    }

    xkb_find_mod_mask ( xkb, WIDGET_MOD_SHIFT, XKB_MOD_NAME_SHIFT, NULL );
    xkb_find_mod_mask ( xkb, WIDGET_MOD_CONTROL, XKB_MOD_NAME_CTRL, NULL );
    xkb_find_mod_mask ( xkb, WIDGET_MOD_ALT, XKB_MOD_NAME_ALT, "Alt", "LAlt", "RAlt", "AltGr", "Mod5", "LevelThree", NULL );
    xkb_find_mod_mask ( xkb, WIDGET_MOD_META, "Meta", NULL );
    xkb_find_mod_mask ( xkb, WIDGET_MOD_SUPER, XKB_MOD_NAME_LOGO, "Super", NULL );
    xkb_find_mod_mask ( xkb, WIDGET_MOD_HYPER, "Hyper", NULL );
}

widget_modifier_mask
xkb_get_modmask(xkb_stuff *xkb, xkb_keycode_t key)
{
    widget_modifier_mask mask = 0;
    widget_modifier mod;
    xkb_mod_index_t *i;
    for ( mod = 0; mod < NUM_WIDGET_MOD; ++mod ) {
        gboolean found = FALSE;
        for ( i = xkb->mods[mod] ; ! found && *i != XKB_MOD_INVALID ; ++i ) {
            found = ( xkb_state_mod_index_is_active(xkb->state, *i, XKB_STATE_MODS_EFFECTIVE) && xkb_state_mod_index_is_consumed(xkb->state, key, *i) != 1 );
        }
        if ( found )
            mask |= (1 << mod);
    }
    return mask;
}

xkb_keysym_t
xkb_handle_key(xkb_stuff *xkb, xkb_keycode_t key, char **text, int *length)
{
    xkb_keysym_t keysym;
    static char  pad[32] = "";
    int          len = 0;

    *text = NULL;
    *length = 0;

    keysym = xkb_state_key_get_one_sym ( xkb->state, key );

    if ( xkb->compose.state != NULL ) {
        if ( ( keysym != XKB_KEY_NoSymbol ) && ( xkb_compose_state_feed ( xkb->compose.state, keysym ) == XKB_COMPOSE_FEED_ACCEPTED ) ) {
            switch ( xkb_compose_state_get_status ( xkb->compose.state ) )
            {
            case XKB_COMPOSE_CANCELLED:
            /* Eat the keysym that cancelled the compose sequence.
             * This is default behaviour with Xlib */
            case XKB_COMPOSE_COMPOSING:
                keysym = XKB_KEY_NoSymbol;
                break;
            case XKB_COMPOSE_COMPOSED:
                keysym = xkb_compose_state_get_one_sym ( xkb->compose.state );
                len = xkb_compose_state_get_utf8 ( xkb->compose.state, pad, sizeof ( pad ) );
                *text = pad;
                break;
            case XKB_COMPOSE_NOTHING:
                break;
            }
            if ( ( keysym == XKB_KEY_NoSymbol ) && ( len == 0 ) ) {
                return XKB_KEY_NoSymbol;
            }
        }
    }

    if ( len == 0 ) {
        len = xkb_state_key_get_utf8 ( xkb->state, key, pad, sizeof ( pad ) );
        *text = pad;
    }

    *length = len;

    return keysym;
}
