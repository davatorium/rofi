#ifndef ROFI_XKB_INTERNAL_H
#define ROFI_XKB_INTERNAL_H

#include <xkbcommon/xkbcommon.h>

struct xkb_stuff {
    xcb_connection_t   *xcb_connection;
    struct xkb_context *context;
    uint8_t            first_event;
    int32_t            device_id;
    struct xkb_keymap  *keymap;
    struct xkb_state   *state;
};

#endif
