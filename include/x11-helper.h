#ifndef X11_ROFI_HELPER_H
#define X11_ROFI_HELPER_H
#include <cairo.h>
#include <xcb/xcb.h>

#include "xkb.h"

/**
 * @defgroup X11Helper X11Helper
 * @ingroup HELPERS
 * @{
 */

int window_get_prop ( Display *display, Window w, Atom prop,
                      Atom *type, int *items,
                      void *buffer, unsigned int bytes ) __attribute__ ( ( nonnull ( 4, 5 ) ) );

/**
 * @param display Connection to the X server.
 * @param w The Window to read property from.
 * @param atom The property identifier
 *
 * Get text property defined by atom from window.
 * Support utf8.
 *
 * @returns a newly allocated string with the result or NULL
 */
char* window_get_text_prop ( Display *display, Window w, Atom atom );

int window_get_atom_prop ( Display *display, Window w, Atom atom, Atom *list, int count );
void window_set_atom_prop ( xcb_connection_t *xcb_connection, Window w, xcb_atom_t prop, xcb_atom_t *atoms, int count );
int window_get_cardinal_prop ( Display *display, Window w, Atom atom, unsigned long *list,
                               int count );

/**
 * Window info.
 */
#define ATOM_ENUM( x )    x
#define ATOM_CHAR( x )    # x

// usable space on a monitor
#define EWMH_ATOMS( X )                    \
    X ( _NET_CLIENT_LIST_STACKING ),       \
    X ( _NET_NUMBER_OF_DESKTOPS ),         \
    X ( _NET_CURRENT_DESKTOP ),            \
    X ( _NET_ACTIVE_WINDOW ),              \
    X ( _NET_CLIENT_LIST ),                \
    X ( _NET_WM_NAME ),                    \
    X ( _NET_WM_STATE ),                   \
    X ( _NET_WM_STATE_SKIP_TASKBAR ),      \
    X ( _NET_WM_STATE_SKIP_PAGER ),        \
    X ( _NET_WM_STATE_ABOVE ),             \
    X ( _NET_WM_STATE_DEMANDS_ATTENTION ), \
    X ( _NET_WM_STATE_WITHDRAWN ),         \
    X ( _NET_WM_WINDOW_TYPE ),             \
    X ( _NET_WM_WINDOW_TYPE_DOCK ),        \
    X ( _NET_WM_WINDOW_TYPE_DESKTOP ),     \
    X ( _NET_WM_WINDOW_TYPE_NORMAL ),      \
    X ( _NET_WM_STATE_FULLSCREEN ),        \
    X ( _NET_WM_DESKTOP ),                 \
    X ( _NET_DESKTOP_VIEWPORT ),           \
    X ( CLIPBOARD ),                       \
    X ( UTF8_STRING ),                     \
    X ( _NET_WM_WINDOW_OPACITY )

enum { EWMH_ATOMS ( ATOM_ENUM ), NUM_NETATOMS };

extern const char *netatom_names[];
extern Atom netatoms[NUM_NETATOMS];
typedef struct
{
    int x, y, w, h;
    int l, r, t, b;
} workarea;

void monitor_active ( xcb_connection_t *xcb_connection, workarea *mon );

int window_send_message ( Display *display, Window target, Window subject,
                          Atom atom, unsigned long protocol,
                          unsigned long mask, Time time );

// find the dimensions of the monitor displaying point x,y
void monitor_dimensions ( xcb_connection_t *xcb_connection, xcb_screen_t *screen, int x, int y, workarea *mon );
// Find the dimensions of the monitor specified by user.
int monitor_get_dimension ( xcb_connection_t *xcb_connection, xcb_screen_t *screen, int monitor, workarea *mon );
int monitor_get_smallest_size ( xcb_connection_t *xcb_connection );

/**
 * @param display The display.
 *
 * Release keyboard.
 */
void release_keyboard ( Display *display );

/**
 * @param display The display.
 * @param w       Window we want to grab keyboard on.
 *
 * Grab keyboard and mouse.
 *
 * @return 1 when keyboard is grabbed, 0 not.
 */
int take_keyboard ( Display *display, Window w );

/**
 * @param mask The mask to canonilize
 *
 * @return The canonilized mask
 */
unsigned int x11_canonalize_mask ( unsigned int mask );

/**
 * @param combo String representing the key combo
 * @param mod [out]  The modifier specified (or AnyModifier if not specified)
 * @param key [out]  The key specified
 *
 * Parse key from user input string.
 */
void x11_parse_key ( char *combo, unsigned int *mod, xkb_keysym_t *key );

/**
 * @param display The connection to the X server.
 * @param box     The window to set the opacity on.
 * @param opacity The opacity value. (0-100)
 *
 * Set the opacity of the window and sub-windows.
 */
void x11_set_window_opacity ( Display *display, Window box, unsigned int opacity );

/**
 * Setup several items required.
 * * Error handling,
 * * Numlock detection
 * * Cache
 */
void x11_setup ( Display *display, xkb_stuff *xkb );

extern xcb_depth_t      *depth;
extern xcb_visualtype_t *visual;
extern xcb_colormap_t   map;
extern xcb_depth_t      *root_depth;
extern xcb_visualtype_t *root_visual;
/**
 * This function tries to create a 32bit TrueColor colormap.
 * If this fails, it falls back to the default for the connected display.
 */
void x11_create_visual_and_colormap ( xcb_connection_t *xcb_connection, xcb_screen_t *xcb_screen );

typedef struct
{
    double red, green, blue, alpha;
} Color;

/**
 * @param display Connection to the X server.
 * @param name    String representing the color.
 *
 * Allocate a pixel value for an X named color
 */
Color color_get ( const char *const name );

void color_background ( cairo_t *d );
void color_border ( cairo_t *d  );
void color_separator ( cairo_t *d );
void color_cache_reset ( void );

void x11_helper_set_cairo_rgba ( cairo_t *d, Color col );
/*@}*/
#endif
