#ifndef ROFI_XCB_INTERNAL_H
#define ROFI_XCB_INTERNAL_H

#define SN_API_NOT_YET_FROZEN
#include <libsn/sn.h>

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
    SnDisplay             *sndisplay;
    SnLauncheeContext     *sncontext;
    struct _workarea      *monitors;
};

#endif
