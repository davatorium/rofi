#ifndef X11_ROFI_HELPER_H
#define X11_ROFI_HELPER_H
#include <glib.h>
#include <cairo.h>
#include <xcb/xcb.h>

#include "xkb.h"

/**
 * @defgroup X11Helper X11Helper
 * @ingroup HELPERS
 * @{
 */

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

void window_set_atom_prop ( xcb_window_t w, xcb_atom_t prop, xcb_atom_t *atoms, int count );

/**
 * xcb_window_t info.
 */
#define ATOM_ENUM( x )    x
#define ATOM_CHAR( x )    # x

// usable space on a monitor
#define EWMH_ATOMS( X )           \
    X ( _NET_WM_WINDOW_OPACITY ), \
    X ( I3_SOCKET_PATH ),         \
    X ( UTF8_STRING ),            \
    X ( STRING ),                 \
    X ( WM_WINDOW_ROLE ),         \
    X ( _XROOTPMAP_ID ),          \
    X ( _MOTIF_WM_HINTS ),        \
    X ( ESETROOT_PMAP_ID )

enum { EWMH_ATOMS ( ATOM_ENUM ), NUM_NETATOMS };

extern const char *netatom_names[];
extern xcb_atom_t netatoms[NUM_NETATOMS];

enum
{
    X11MOD_SHIFT,
    X11MOD_CONTROL,
    X11MOD_ALT,
    X11MOD_META,
    X11MOD_SUPER,
    X11MOD_HYPER,
    X11MOD_ANY,
    NUM_X11MOD
};

typedef struct _workarea
{
    int              monitor_id;
    int              primary;
    int              x, y, w, h;
    char             *name;
    struct _workarea *next;
} workarea;
int monitor_active ( workarea *mon );

// find the dimensions of the monitor displaying point x,y
void monitor_dimensions ( int x, int y, workarea *mon );
// Find the dimensions of the monitor specified by user.
int monitor_get_dimension ( int monitor_id, workarea *mon );
int monitor_get_smallest_size ( void );

/**
 * Release keyboard.
 */
void release_keyboard ( void );
void release_pointer ( void );

/**
 * @param w       xcb_window_t we want to grab keyboard on.
 *
 * Grab keyboard and mouse.
 *
 * @return 1 when keyboard is grabbed, 0 not.
 */
int take_keyboard ( xcb_window_t w );
int take_pointer ( xcb_window_t w );

/**
 * @param mask The mask to canonilize
 *
 * @return The canonilized mask
 */
unsigned int x11_canonalize_mask ( unsigned int mask );

unsigned int x11_get_current_mask ( xkb_stuff *xkb );

/**
 * @param combo String representing the key combo
 * @param mod [out]  The modifier specified (or AnyModifier if not specified)
 * @param key [out]  The key specified
 * @param release [out] If it should react on key-release, not key-press
 *
 * Parse key from user input string.
 */
gboolean x11_parse_key ( char *combo, unsigned int *mod, xkb_keysym_t *key, gboolean *release );

/**
 * @param box     The window to set the opacity on.
 * @param opacity The opacity value. (0-100)
 *
 * Set the opacity of the window and sub-windows.
 */
void x11_set_window_opacity ( xcb_window_t box, unsigned int opacity );

/**
 * Setup several items required.
 * * Error handling,
 * * Numlock detection
 * * Cache
 */
void x11_setup ( xkb_stuff *xkb );

extern xcb_depth_t      *depth;
extern xcb_visualtype_t *visual;
extern xcb_colormap_t   map;
extern xcb_depth_t      *root_depth;
extern xcb_visualtype_t *root_visual;
/**
 * This function tries to create a 32bit TrueColor colormap.
 * If this fails, it falls back to the default for the connected display.
 */
void x11_create_visual_and_colormap ( void );

typedef struct
{
    double red, green, blue, alpha;
} Color;

/**
 * @param name    String representing the color.
 *
 * Allocate a pixel value for an X named color
 */
Color color_get ( const char *const name );

void color_background ( cairo_t *d );
void color_border ( cairo_t *d  );
void color_separator ( cairo_t *d );

void x11_helper_set_cairo_rgba ( cairo_t *d, Color col );

/**
 * Gets a surface containing the background image of the desktop.
 *
 * @returns a cairo surface with the background image of the desktop.
 */
cairo_surface_t * x11_helper_get_bg_surface ( void );

/**
 * Creates an internal represenation of the available monitors.
 * Used for positioning rofi.
 */
void x11_build_monitor_layout ( void );
void x11_dump_monitor_layout ( void );
int x11_modifier_active ( unsigned int mask, int key );

/**
 * @param window The X11 window to modify
 *
 * Set the right hints to disable the window decoration.
 * (Set MOTIF_WM_HINTS, decoration field)
 */
void x11_disable_decoration ( xcb_window_t window );
/*@}*/
#endif
