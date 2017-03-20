#ifndef ROFI_XKB_H
#define ROFI_XKB_H

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>

#include "widgets/widget.h"

typedef struct {
    /** Keyboard context */
    struct xkb_context *context;
    /** Current keymap */
    struct xkb_keymap  *keymap;
    /** Keyboard state */
    struct xkb_state   *state;
    /** Modifiers indexes */
    xkb_mod_index_t mods[NUM_WIDGET_MOD][8];
    /** Compose information */
    struct
    {
        /** Compose table */
        struct xkb_compose_table *table;
        /** Compose state */
        struct xkb_compose_state *state;
    } compose;
} xkb_stuff;


void xkb_common_init ( xkb_stuff *xkb );
widget_modifier_mask xkb_get_modmask(xkb_stuff *xkb, xkb_keysym_t key);
xkb_keysym_t xkb_handle_key ( xkb_stuff *xkb, xkb_keycode_t keycode, char **text, int *length);
#endif
