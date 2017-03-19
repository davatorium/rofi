#ifndef ROFI_XCB_INTERNAL_H
#define ROFI_XCB_INTERNAL_H
/** Indication we accept that startup notification api is not yet frozen */

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

/**
 * Structure to keep xcb stuff around.
 */
struct _xcb_stuff
{
    xcb_connection_t      *connection;
    xcb_ewmh_connection_t ewmh;
    xcb_screen_t          *screen;
    int                   screen_nbr;
};

#endif
