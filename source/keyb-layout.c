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

#include "rofi.h"

/**
 * Data structure corresponding to the relationship of the character to the layout
 */
struct kbl_charmap {
    xkb_layout_index_t layout;
    xkb_level_index_t level;
    xkb_keycode_t keycode;
    int keysim;
    gunichar chr;
};

/**
 * Array of relationships of the character to the layout. Sorted by keycode
 */
static GArray *rofi_kbl_charmap;

static void kbl_charmap_key_iter ( struct xkb_keymap *keymap, xkb_keycode_t key, void *data ) {
    const xkb_keysym_t *keysims;
    char buffer[255];
    GArray *char_array = ( GArray* ) data;

    xkb_layout_index_t lt_count = xkb_keymap_num_layouts_for_key ( keymap, key );
    if ( lt_count <= 1 ) {
        return;
    }
    for ( xkb_layout_index_t lt_index = 0; lt_index < lt_count; lt_index ++ ) {
        xkb_level_index_t lvl_count = xkb_keymap_num_levels_for_key ( keymap, key, lt_index );
        for ( xkb_level_index_t lvl_index = 0; lvl_index < lvl_count; lvl_index++ ) {
            int ks_count = xkb_keymap_key_get_syms_by_level ( keymap, key, lt_index, lvl_index, &keysims );
            for ( int ks_index=0; ks_index < ks_count; ks_index++ ) {
                int char_count = xkb_keysym_to_utf8 ( keysims[ks_index], buffer, 255 );
                if ( char_count > 0 ) {
                    gunichar chr = g_utf8_get_char ( buffer );
                    if ( g_unichar_isdefined ( chr ) ) {
                        struct kbl_charmap charmap_item = {
                            .layout = lt_index,
                            .level = lvl_index,
                            .keycode = key,
                            .keysim = ks_index,
                            .chr = g_utf8_get_char ( buffer )
                        };
                        g_array_append_val ( char_array, charmap_item );
                    }
                }
            }
        }
    }
}

void kbl_charmap_load_from_keymap ( struct xkb_keymap *keymap ) {
    rofi_kbl_charmap = NULL;
    xkb_layout_index_t layout_count = xkb_keymap_num_layouts ( keymap );
    if ( layout_count <= 1 ) {
        return;
    }
    rofi_kbl_charmap = g_array_new ( FALSE, FALSE, sizeof ( struct kbl_charmap ) );
    xkb_keymap_key_for_each ( keymap, kbl_charmap_key_iter, rofi_kbl_charmap );
}

void kbl_charmap_cleanup(void) {
    g_array_free ( rofi_kbl_charmap, TRUE );
}

static gint kbl_charmap_find_char_key ( GArray *charmap, gunichar chr, struct kbl_charmap *key, gint next ) {
    for ( guint i = ( guint ) next; i<charmap->len; i++ ) {
        struct kbl_charmap *charmap_item = &g_array_index ( charmap, struct kbl_charmap, i );
        if ( charmap_item->chr == chr ) {
            key->layout = charmap_item->layout;
            key->level = charmap_item->level;
            key->keycode = charmap_item->keycode;
            key->keysim = charmap_item->keysim;
            key->chr = charmap_item->chr;
            return ( gint ) i;
        }
    }
    return -1;
}

gchar *kbl_charmap_for_char ( const gchar *chr ) {
    gint found = 0;
    struct kbl_charmap key_for_char;
    gunichar uchar = g_utf8_get_char ( chr );
    GString * str = g_string_new ( "" );
    if ( rofi_kbl_charmap ) {
        while ( ( found = kbl_charmap_find_char_key ( rofi_kbl_charmap, uchar, &key_for_char, found ) ) >= 0) {
            for ( gint i = found; i >= 0; i-- ) {
                struct kbl_charmap *charmap_item = &g_array_index ( rofi_kbl_charmap, struct kbl_charmap, i );
                if ( key_for_char.level == charmap_item->level &&
                     key_for_char.keycode == charmap_item->keycode &&
                     key_for_char.keysim == charmap_item->keysim )
                {
                    g_string_append_unichar ( str, charmap_item->chr );
                }
                if (key_for_char.keycode != charmap_item->keycode) {
                    break;
                }
            }
            for ( guint i = ( guint ) found+1; i < rofi_kbl_charmap->len; i++ ) {
                struct kbl_charmap *charmap_item = &g_array_index ( rofi_kbl_charmap, struct kbl_charmap, i );
                if ( key_for_char.level == charmap_item->level &&
                     key_for_char.keycode == charmap_item->keycode &&
                     key_for_char.keysim == charmap_item->keysim )
                {
                    g_string_append_unichar ( str, charmap_item->chr );
                }
                if (key_for_char.keycode != charmap_item->keycode) {
                    break;
                }
            }
            found++;
        }
    }
    if ( str->len ==0 ) {
        g_string_append_unichar ( str, uchar );
    }
    gchar *retv = str->str;
    g_string_free ( str, FALSE );
    return retv;
}
