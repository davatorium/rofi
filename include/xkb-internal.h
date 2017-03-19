#ifndef ROFI_XKB_INTERNAL_H
#define ROFI_XKB_INTERNAL_H

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>

struct xkb_stuff
{
    /** Keyboard context */
    struct xkb_context *context;
    /** Current keymap */
    struct xkb_keymap  *keymap;
    /** Keyboard state */
    struct xkb_state   *state;
    /** Compose information */
    struct
    {
        /** Compose table */
        struct xkb_compose_table *table;
        /** Compose state */
        struct xkb_compose_state * state;
    } compose;
};

#endif
