#ifndef ROFI_XKB_INTERNAL_H
#define ROFI_XKB_INTERNAL_H

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>

struct xkb_stuff
{
    xcb_connection_t   *xcb_connection;
    struct xkb_context *context;
    uint8_t            first_event;
    int32_t            device_id;
    struct xkb_keymap  *keymap;
    struct xkb_state   *state;
    struct
    {
        struct xkb_compose_table *table;
        struct xkb_compose_state * state;
    } compose;
};

#endif
