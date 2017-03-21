#ifndef ROFI_XCB_H
#define ROFI_XCB_H

#include <glib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xkb.h>
#include <cairo.h>
#include <cairo-xcb.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-x11.h>
#include <libgwater-xcb.h>

#include "xkb.h"

typedef enum
{
    /** Default EWHM compatible window manager */
    WM_EWHM    = 0,
    /** I3 Window manager */
    WM_I3      = 1,
    /** Awesome window manager */
    WM_AWESOME = 2,
    /** Openbox window manager */
    WM_OPENBOX = 4
} WindowManager;

/** For getting the atoms in an enum  */
#define ATOM_ENUM( x )    x
/** Get the atoms as strings. */
#define ATOM_CHAR( x )    # x

/** Atoms we want to pre-load */
#define EWMH_ATOMS( X )           \
    X ( _NET_WM_WINDOW_OPACITY ), \
    X ( I3_SOCKET_PATH ),         \
    X ( UTF8_STRING ),            \
    X ( STRING ),                 \
    X ( CLIPBOARD ),              \
    X ( WM_WINDOW_ROLE ),         \
    X ( _XROOTPMAP_ID ),          \
    X ( _MOTIF_WM_HINTS ),        \
    X ( ESETROOT_PMAP_ID )

/** enumeration of the atoms. */
enum { EWMH_ATOMS ( ATOM_ENUM ), NUM_NETATOMS };

typedef struct {
    GMainLoop *main_loop;
    GWaterXcbSource *main_loop_source;
    xcb_connection_t *connection;
    xcb_ewmh_connection_t ewmh;
    xcb_screen_t          *screen;
    int screen_nbr;
    xkb_stuff xkb;
    uint8_t xkb_first_event;
    int32_t xkb_device_id;
    xcb_atom_t netatoms[NUM_NETATOMS];
    xcb_visualtype_t *root_visual;
    xcb_depth_t      *depth;
    xcb_visualtype_t *visual;
    xcb_colormap_t map;
    WindowManager wm;
    xcb_gc_t gc;
    xcb_window_t main_window;
    gint width, height;
    gboolean mapped;
} xcb_stuff;

extern xcb_stuff *xcb;

char* window_get_text_prop ( xcb_window_t w, xcb_atom_t atom );

#endif
