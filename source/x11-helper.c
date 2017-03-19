/**
 * rofi
 *
 * MIT/X11 License
 * Copyright (c) 2012 Sean Pringle <sean.pringle@gmail.com>
 * Modified 2013-2017 Qball Cow <qball@gmpclient.org>
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
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <glib.h>
#include <cairo.h>

#include "settings.h"
#include "helper.h"

#include <rofi.h>
/** Checks if the if x and y is inside rectangle. */
#define INTERSECT( x, y, x1, y1, w1, h1 )    ( ( ( ( x ) >= ( x1 ) ) && ( ( x ) < ( x1 + w1 ) ) ) && ( ( ( y ) >= ( y1 ) ) && ( ( y ) < ( y1 + h1 ) ) ) )
#include "x11-helper.h"
#include "xkb-internal.h"

/** Log domain for this module */
#define LOG_DOMAIN    "X11Helper"

/**
 * Holds for each supported modifier the possible modifier mask.
 * Check x11_mod_masks[MODIFIER]&mask != 0 to see if MODIFIER is activated.
 */
static unsigned int x11_mod_masks[NUM_X11MOD];

static unsigned int x11_find_mod_mask ( xkb_stuff *xkb, ... )
{
    va_list         names;
    const char      *name;
    xkb_mod_index_t i;
    unsigned int    mask = 0;
    va_start ( names, xkb );
    while ( ( name = va_arg ( names, const char * ) ) != NULL ) {
        i = xkb_keymap_mod_get_index ( xkb->keymap, name );
        if ( i != XKB_MOD_INVALID ) {
            mask |= 1 << i;
        }
    }
    va_end ( names );
    return mask;
}

static void x11_figure_out_masks ( xkb_stuff *xkb )
{
    x11_mod_masks[X11MOD_SHIFT]   = x11_find_mod_mask ( xkb, XKB_MOD_NAME_SHIFT, NULL );
    x11_mod_masks[X11MOD_CONTROL] = x11_find_mod_mask ( xkb, XKB_MOD_NAME_CTRL, NULL );
    x11_mod_masks[X11MOD_ALT]     = x11_find_mod_mask ( xkb, XKB_MOD_NAME_ALT, "Alt", "LAlt", "RAlt", "AltGr", "Mod5", "LevelThree", NULL );
    x11_mod_masks[X11MOD_META]    = x11_find_mod_mask ( xkb, "Meta", NULL );
    x11_mod_masks[X11MOD_SUPER]   = x11_find_mod_mask ( xkb, XKB_MOD_NAME_LOGO, "Super", NULL );
    x11_mod_masks[X11MOD_HYPER]   = x11_find_mod_mask ( xkb, "Hyper", NULL );

    gsize i;
    for ( i = 0; i < X11MOD_ANY; ++i ) {
        x11_mod_masks[X11MOD_ANY] |= x11_mod_masks[i];
    }
}

int x11_modifier_active ( unsigned int mask, int key )
{
    return ( x11_mod_masks[key] & mask ) != 0;
}

unsigned int x11_canonalize_mask ( unsigned int mask )
{
    // Bits 13 and 14 of the modifiers together are the group number, and
    // should be ignored when looking up key bindings
    mask &= x11_mod_masks[X11MOD_ANY];

    gsize i;
    for ( i = 0; i < X11MOD_ANY; ++i ) {
        if ( mask & x11_mod_masks[i] ) {
            mask |= x11_mod_masks[i];
        }
    }
    return mask;
}

unsigned int x11_get_current_mask ( xkb_stuff *xkb )
{
    unsigned int mask = 0;
    for ( gsize i = 0; i < xkb_keymap_num_mods ( xkb->keymap ); ++i ) {
        if ( xkb_state_mod_index_is_active ( xkb->state, i, XKB_STATE_MODS_EFFECTIVE ) ) {
            mask |= ( 1 << i );
        }
    }
    return x11_canonalize_mask ( mask );
}

// convert a Mod+key arg to mod mask and keysym
gboolean x11_parse_key ( const char *combo, unsigned int *mod, xkb_keysym_t *key, gboolean *release, GString *str )
{
    char         *input_key = g_strdup ( combo );
    char         *mod_key   = input_key;
    char         *error_msg = NULL;
    unsigned int modmask    = 0;
    xkb_keysym_t sym        = XKB_KEY_NoSymbol;
    // Test if this works on release.
    if ( g_str_has_prefix ( mod_key, "!" ) ) {
        ++mod_key;
        *release = TRUE;
    }

    char **entries = g_strsplit_set ( mod_key, "+-", -1 );
    for ( int i = 0; entries && entries[i]; i++ ) {
        char *entry = entries[i];
        // Remove trailing and leading spaces.
        entry = g_strstrip ( entry );
        // Compare against lowered version.
        char *entry_lowered = g_utf8_strdown ( entry, -1 );
        if ( g_utf8_collate ( entry_lowered, "shift" ) == 0  ) {
            modmask |= x11_mod_masks[X11MOD_SHIFT];
            if ( x11_mod_masks[X11MOD_SHIFT] == 0 ) {
                error_msg = g_strdup ( "X11 configured keyboard has no <b>Shift</b> key.\n" );
            }
        }
        else if  ( g_utf8_collate ( entry_lowered, "control" ) == 0 ) {
            modmask |= x11_mod_masks[X11MOD_CONTROL];
            if  ( x11_mod_masks[X11MOD_CONTROL] == 0 ) {
                error_msg = g_strdup ( "X11 configured keyboard has no <b>Control</b> key.\n" );
            }
        }
        else if  ( g_utf8_collate ( entry_lowered, "alt" ) == 0 ) {
            modmask |= x11_mod_masks[X11MOD_ALT];
            if  ( x11_mod_masks[X11MOD_ALT] == 0 ) {
                error_msg = g_strdup ( "X11 configured keyboard has no <b>Alt</b> key.\n" );
            }
        }
        else if  ( g_utf8_collate ( entry_lowered, "super" ) == 0 ||
                   g_utf8_collate ( entry_lowered, "super_l" ) == 0 ||
                   g_utf8_collate ( entry_lowered, "super_r" ) == 0
                   ) {
            modmask |= x11_mod_masks[X11MOD_SUPER];
            if  ( x11_mod_masks[X11MOD_SUPER] == 0 ) {
                error_msg = g_strdup ( "X11 configured keyboard has no <b>Super</b> key.\n" );
            }
        }
        else if  ( g_utf8_collate ( entry_lowered, "meta" ) == 0 ) {
            modmask |= x11_mod_masks[X11MOD_META];
            if  ( x11_mod_masks[X11MOD_META] == 0 ) {
                error_msg = g_strdup ( "X11 configured keyboard has no <b>Meta</b> key.\n" );
            }
        }
        else if  ( g_utf8_collate ( entry_lowered, "hyper" ) == 0 ) {
            modmask |= x11_mod_masks[X11MOD_HYPER];
            if  ( x11_mod_masks[X11MOD_HYPER] == 0 ) {
                error_msg = g_strdup ( "X11 configured keyboard has no <b>Hyper</b> key.\n" );
            }
        }
        else {
            if ( sym != XKB_KEY_NoSymbol ) {
                error_msg = g_markup_printf_escaped ( "Only one (non modifier) key can be bound per binding: <b>%s</b> is invalid.\n", entry );
            }
            sym = xkb_keysym_from_name ( entry, XKB_KEYSYM_NO_FLAGS );
            if ( sym == XKB_KEY_NoSymbol ) {
                error_msg = g_markup_printf_escaped ( "∙ Key <i>%s</i> is not understood\n", entry );
            }
        }
        g_free ( entry_lowered );
    }
    g_strfreev ( entries );

    g_free ( input_key );

    if ( error_msg ) {
        char *name = g_markup_escape_text ( combo, -1 );
        g_string_append_printf ( str, "Cannot understand the key combination: <i>%s</i>\n", name );
        g_string_append ( str, error_msg );
        g_free ( name );
        g_free ( error_msg );
        return FALSE;
    }
    *key = sym;
    *mod = modmask;
    return TRUE;
}

void x11_setup ( xkb_stuff *xkb )
{
    // determine numlock mask so we can bind on keys with and without it
    x11_figure_out_masks ( xkb );
}


Color color_get ( const char *const name )
{
    char *copy  = g_strdup ( name );
    char *cname = g_strstrip ( copy );

    union
    {
        struct
        {
            uint8_t b;
            uint8_t g;
            uint8_t r;
            uint8_t a;
        }        sep;
        uint32_t pixel;
    } color = {
        .pixel = 0xffffffff,
    };
    // Special format.
    if ( strncmp ( cname, "argb:", 5 ) == 0 ) {
        color.pixel = strtoul ( &cname[5], NULL, 16 );
    }
    else if ( strncmp ( cname, "#", 1 ) == 0 ) {
        unsigned long val    = strtoul ( &cname[1], NULL, 16 );
        ssize_t       length = strlen ( &cname[1] );
        switch ( length )
        {
        case 3:
            color.sep.a = 0xff;
            color.sep.r = 16 * ( ( val & 0xF00 ) >> 8 );
            color.sep.g = 16 * ( ( val & 0x0F0 ) >> 4 );
            color.sep.b = 16 * ( val & 0x00F );
            break;
        case 6:
            color.pixel = val;
            color.sep.a = 0xff;
            break;
        case 8:
            color.pixel = val;
            break;
        default:
            break;
        }
    }
    g_free ( copy );

    Color ret = {
        .red   = color.sep.r / 255.0,
        .green = color.sep.g / 255.0,
        .blue  = color.sep.b / 255.0,
        .alpha = color.sep.a / 255.0,
    };
    return ret;
}
