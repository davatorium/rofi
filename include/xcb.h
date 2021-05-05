/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2020 Qball Cow <qball@gmpclient.org>
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

#include <xcb/xcb.h>
#include <cairo.h>

/**
 * xcb data structure type declaration.
 */
typedef struct _xcb_stuff xcb_stuff;

/**
 * Global pointer to xcb_stuff instance.
 */
extern xcb_stuff *xcb;

/**
 * Get the root window.
 *
 * @returns the root window.
 */
xcb_window_t xcb_stuff_get_root_window ( void );

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
    X ( WM_TAKE_FOCUS ),          \
    X ( ESETROOT_PMAP_ID )

/** enumeration of the atoms. */
enum { EWMH_ATOMS ( ATOM_ENUM ), NUM_NETATOMS };
/** atoms as string */
extern const char *netatom_names[];
/** atoms */
extern xcb_atom_t netatoms[NUM_NETATOMS];

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
 * @param w rofis window
 *
 * Stores old input focus for reverting and set focus to rofi.
 */
void rofi_xcb_set_input_focus(xcb_window_t w);

/**
 * IF set, revert the focus back to the original applications.
 */
void rofi_xcb_revert_input_focus(void);

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
    WM_EWHM                          = 0,
    /** I3 Window manager */
    WM_DO_NOT_CHANGE_CURRENT_DESKTOP = 1,
    /** PANGO WORKSPACE NAMES */
    WM_PANGO_WORKSPACE_NAMES         = 2,
} WindowManagerQuirk;

/**
 * Indicates the current window manager.
 * This is used for work-arounds.
 */
extern WindowManagerQuirk current_window_manager;

/**
 * @param window the window the screenshot
 * @param size   Size of the thumbnail
 *
 * Creates a thumbnail of the window.
 *
 * @returns NULL if window was not found, or unmapped, otherwise returns a cairo_surface.
 */
cairo_surface_t *x11_helper_get_screenshot_surface_window ( xcb_window_t window, int size );

/**
 * @param surface
 * @param radius
 * @param deviation
 *
 * Blur the content of the surface with radius and deviation.
 */
void cairo_image_surface_blur(cairo_surface_t* surface, double radius, double deviation);
#endif
