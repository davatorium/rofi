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
            modmask |= WIDGET_MODMASK_SHIFT;
        }
        else if  ( g_utf8_collate ( entry_lowered, "control" ) == 0 ) {
            modmask |= WIDGET_MODMASK_CONTROL;
        }
        else if  ( g_utf8_collate ( entry_lowered, "alt" ) == 0 ) {
            modmask |= WIDGET_MODMASK_ALT;
        }
        else if  ( g_utf8_collate ( entry_lowered, "super" ) == 0 ||
                   g_utf8_collate ( entry_lowered, "super_l" ) == 0 ||
                   g_utf8_collate ( entry_lowered, "super_r" ) == 0
                   ) {
            modmask |= WIDGET_MODMASK_SUPER;
        }
        else if  ( g_utf8_collate ( entry_lowered, "meta" ) == 0 ) {
            modmask |= WIDGET_MODMASK_META;
        }
        else if  ( g_utf8_collate ( entry_lowered, "hyper" ) == 0 ) {
            modmask |= WIDGET_MODMASK_HYPER;
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
