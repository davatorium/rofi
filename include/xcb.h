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

#ifndef ROFI_XCB_H
#define ROFI_XCB_H

#include <cairo.h>

/**
 * xcb data structure type declaration.
 */
typedef struct _xcb_stuff   xcb_stuff;

/**
 * Global pointer to xcb_stuff instance.
 */
extern xcb_stuff *xcb;

/**
 * @param xcb the xcb data structure
 *
 * Get the root window.
 *
 * @returns the root window.
 */
xcb_window_t xcb_stuff_get_root_window ( void );
/**
 * @param xcb The xcb data structure.
 *
 * Disconnect and free all xcb connections and references.
 */
void xcb_stuff_wipe ( void );

/**
 * @param w The xcb_window_t to read property from.
 * @param atom The property identifier
 *
 * Get text property defined by atom from window.
 * Support utf8.
 *
 * @returns a newly allocated string with the result or NULL
 */
char* window_get_text_prop ( xcb_window_t w, xcb_atom_t atom );

/**
 * @param w The xcb_window_t to set property on
 * @param prop Atom of the property to change
 * @param atoms List of atoms to change the property too
 * @param count The length of the atoms list.
 *
 * Set property on window.
 */
void window_set_atom_prop ( xcb_window_t w, xcb_atom_t prop, xcb_atom_t *atoms, int count );

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
/** atoms as string */
extern const char *netatom_names[];
/** atoms */
extern xcb_atom_t netatoms[NUM_NETATOMS];

/**
 * Enumerator describing the different modifier keys.
 */
enum
{
    /** Shift key */
    X11MOD_SHIFT,
    /** Control Key */
    X11MOD_CONTROL,
    /** Alt key */
    X11MOD_ALT,
    /** Meta key */
    X11MOD_META,
    /** Super (window) key */
    X11MOD_SUPER,
    /** Hyper key */
    X11MOD_HYPER,
    /** Any modifier */
    X11MOD_ANY,
    /** Number of modifier keys */
    NUM_X11MOD
};

/**
 * Structure describing a workarea/monitor.
 */
typedef struct _workarea
{
    /** numeric monitor id. */
    int              monitor_id;
    /** if monitor is set as primary monitor. */
    int              primary;
    /** Horizontal location (in pixels) of the monitor. */
    int              x;
    /** Vertical location  (in pixels) of the monitor. */
    int              y;
    /** Width of the monitor. */
    int              w;
    /** Height of the monitor */
    int              h;
    int              mw, mh;
    /** Output name of the monitor, e.g. eDP1 or VGA-1 */
    char             *name;
    /** Pointer to next monitor */
    struct _workarea *next;
} workarea;

/**
 * @param mon workarea to be filled in.
 *
 * Fills in #mon with the information about the monitor rofi should show on.
 *
 * @returns TRUE if monitor is found, FALSE if no monitor could be detected.
 */
int monitor_active ( workarea *mon );

/**
 * Release keyboard grab on root window.
 */
void release_keyboard ( void );
/**
 * Release pointer grab on root window.
 */
void release_pointer ( void );

/**
 * @param w       xcb_window_t we want to grab keyboard on.
 * @param iters   Number of retries.
 *
 * Grab keyboard.
 *
 * @return 1 when keyboard is grabbed, 0 not.
 */
int take_keyboard ( xcb_window_t w, int iters );

/**
 * @param w       xcb_window_t we want to grab mouse on.
 * @param iters   Number of retries.
 *
 * Grab mouse.
 *
 * @return 1 when mouse is grabbed, 0 not.
 */
int take_pointer ( xcb_window_t w, int iters );

/**
 * @param main_loop The GMainLoop
 *
 * Setup several items required.
 * * Error handling,
 * * Numlock detection
 * * Cache
 *
 * @returns Whether the setup succeeded or not
 */
gboolean x11_setup ( GMainLoop *main_loop );

/**
 * Depth of visual
 */
extern xcb_depth_t *depth;
/**
 * Visual to use for creating window
 */
extern xcb_visualtype_t *visual;
/**
 * Color map to use for creating window
 */
extern xcb_colormap_t map;

/**
 * This function tries to create a 32bit TrueColor colormap.
 * If this fails, it falls back to the default for the connected display.
 */
void x11_create_visual_and_colormap ( void );

/**
 * Gets a surface containing the background image of the desktop.
 *
 * @returns a cairo surface with the background image of the desktop.
 */
cairo_surface_t * x11_helper_get_bg_surface ( void );
/**
 * Gets a surface for the root window of the desktop.
 *
 * Can be used to take screenshot.
 *
 * @returns a cairo surface for the root window of the desktop.
 */
cairo_surface_t *x11_helper_get_screenshot_surface ( void );

/**
 * Gets a surface from an svg path
 *
 * @returns a cairo surface from an svg path
 */
cairo_surface_t *cairo_image_surface_create_from_svg ( const gchar* file, int height );

/**
 * Creates an internal represenation of the available monitors.
 * Used for positioning rofi.
 */
void x11_build_monitor_layout ( void );

/**
 * Dump the monitor layout to stdout.
 */
void x11_dump_monitor_layout ( void );

/**
 * @param window The X11 window to modify
 *
 * Set the right hints to disable the window decoration.
 * (Set MOTIF_WM_HINTS, decoration field)
 */
void x11_disable_decoration ( xcb_window_t window );

/**
 * List of window managers that need different behaviour to functioning.
 */
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

/**
 * Indicates the current window manager.
 * This is used for work-arounds.
 */
extern WindowManager current_window_manager;

/**
 * discover the window manager.
 */
void x11_helper_discover_window_manager ( void );
#endif
