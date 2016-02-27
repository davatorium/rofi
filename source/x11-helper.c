/**
 * rofi
 *
 * MIT/X11 License
 * Copyright (c) 2012 Sean Pringle <sean.pringle@gmail.com>
 * Modified 2013-2016 Qball Cow <qball@gmpclient.org>
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
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <glib.h>
#include <cairo.h>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/extensions/Xinerama.h>
#include "settings.h"

#include <rofi.h>
#define OVERLAP( a, b, c,                          \
                 d )       ( ( ( a ) == ( c ) &&   \
                               ( b ) == ( d ) ) || \
                             MIN ( ( a ) + ( b ), ( c ) + ( d ) ) - MAX ( ( a ), ( c ) ) > 0 )
#define INTERSECT( x, y, w, h, x1, y1, w1,                   \
                   h1 )    ( OVERLAP ( ( x ), ( w ), ( x1 ), \
                                       ( w1 ) ) && OVERLAP ( ( y ), ( h ), ( y1 ), ( h1 ) ) )
#include "x11-helper.h"
#include "xkb-internal.h"

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

xcb_depth_t         *depth           = NULL;
xcb_visualtype_t    *visual          = NULL;
xcb_colormap_t      map              = XCB_COLORMAP_NONE;
xcb_depth_t         *root_depth      = NULL;
xcb_visualtype_t    *root_visual     = NULL;
Atom                netatoms[NUM_NETATOMS];
const char          *netatom_names[] = { EWMH_ATOMS ( ATOM_CHAR ) };
static unsigned int x11_mod_masks[NUM_X11MOD];
extern xcb_ewmh_connection_t xcb_ewmh;

// retrieve a property of any type from a window
int window_get_prop ( Display *display, Window w, Atom prop, Atom *type, int *items, void *buffer, unsigned int bytes )
{
    int           format;
    unsigned long nitems, nbytes;
    unsigned char *ret = NULL;
    memset ( buffer, 0, bytes );

    if ( XGetWindowProperty ( display, w, prop, 0, bytes / 4, False, AnyPropertyType, type, &format, &nitems, &nbytes, &ret ) == Success &&
         ret && *type != None && format ) {
        if ( format == 8 ) {
            memmove ( buffer, ret, MIN ( bytes, nitems ) );
        }

        if ( format == 16 ) {
            memmove ( buffer, ret, MIN ( bytes, nitems * sizeof ( short ) ) );
        }

        if ( format == 32 ) {
            memmove ( buffer, ret, MIN ( bytes, nitems * sizeof ( long ) ) );
        }

        *items = ( int ) nitems;
        XFree ( ret );
        return 1;
    }

    return 0;
}

// retrieve a text property from a window
// technically we could use window_get_prop(), but this is better for character set support
char* window_get_text_prop ( Display *display, Window w, Atom atom )
{
    XTextProperty prop;
    char          *res   = NULL;
    char          **list = NULL;
    int           count;

    if ( XGetTextProperty ( display, w, &prop, atom ) && prop.value && prop.nitems ) {
        if ( prop.encoding == XA_STRING ) {
            size_t l = strlen ( ( char *) prop.value ) + 1;
            res = g_malloc ( l );
            // make clang-check happy.
            if ( res ) {
                g_strlcpy ( res, ( char * ) prop.value, l );
            }
        }
        else if ( Xutf8TextPropertyToTextList ( display, &prop, &list, &count ) >= Success && count > 0 && *list ) {
            size_t l = strlen ( *list ) + 1;
            res = g_malloc ( l );
            // make clang-check happy.
            if ( res ) {
                g_strlcpy ( res, *list, l );
            }
            XFreeStringList ( list );
        }
    }

    if ( prop.value ) {
        XFree ( prop.value );
    }

    return res;
}
int window_get_atom_prop ( Display *display, Window w, Atom atom, Atom *list, int count )
{
    Atom type;
    int  items;
    return window_get_prop ( display, w, atom, &type, &items, list, count * sizeof ( Atom ) ) && type == XA_ATOM ? items : 0;
}

void window_set_atom_prop ( xcb_connection_t *xcb_connection, Window w, xcb_atom_t prop, xcb_atom_t *atoms, int count )
{
    xcb_change_property ( xcb_connection, XCB_PROP_MODE_REPLACE, w, prop, XCB_ATOM_ATOM, 32, count, atoms);
}

int window_get_cardinal_prop ( Display *display, Window w, Atom atom, unsigned long *list, int count )
{
    Atom type; int items;
    return window_get_prop ( display, w, atom, &type, &items, list, count * sizeof ( unsigned long ) ) && type == XA_CARDINAL ? items : 0;
}
int monitor_get_smallest_size ( Display *display )
{
    int size = MIN ( WidthOfScreen ( DefaultScreenOfDisplay ( display ) ),
                     HeightOfScreen ( DefaultScreenOfDisplay ( display ) ) );
    // locate the current monitor
    if ( XineramaIsActive ( display ) ) {
        int                monitors;
        XineramaScreenInfo *info = XineramaQueryScreens ( display, &monitors );

        if ( info ) {
            for ( int i = 0; i < monitors; i++ ) {
                size = MIN ( info[i].width, size );
                size = MIN ( info[i].height, size );
            }
        }
        XFree ( info );
    }

    return size;
}
int monitor_get_dimension ( Display *display, Screen *screen, int monitor, workarea *mon )
{
    memset ( mon, 0, sizeof ( workarea ) );
    mon->w = WidthOfScreen ( screen );
    mon->h = HeightOfScreen ( screen );
    // locate the current monitor
    if ( XineramaIsActive ( display ) ) {
        int                monitors;
        XineramaScreenInfo *info = XineramaQueryScreens ( display, &monitors );

        if ( info ) {
            if ( monitor >= 0 && monitor < monitors ) {
                mon->x = info[monitor].x_org;
                mon->y = info[monitor].y_org;
                mon->w = info[monitor].width;
                mon->h = info[monitor].height;
                return TRUE;
            }
            XFree ( info );
        }
    }
    return FALSE;
}
// find the dimensions of the monitor displaying point x,y
void monitor_dimensions ( Display *display, Screen *screen, int x, int y, workarea *mon )
{
    memset ( mon, 0, sizeof ( workarea ) );
    mon->w = WidthOfScreen ( screen );
    mon->h = HeightOfScreen ( screen );

    // locate the current monitor
    if ( XineramaIsActive ( display ) ) {
        int                monitors;
        XineramaScreenInfo *info = XineramaQueryScreens ( display, &monitors );

        if ( info ) {
            for ( int i = 0; i < monitors; i++ ) {
                if ( INTERSECT ( x, y, 1, 1, info[i].x_org, info[i].y_org, info[i].width, info[i].height ) ) {
                    mon->x = info[i].x_org;
                    mon->y = info[i].y_org;
                    mon->w = info[i].width;
                    mon->h = info[i].height;
                    break;
                }
            }
        }

        XFree ( info );
    }
}

/**
 * @param x  The x position of the mouse [out]
 * @param y  The y position of the mouse [out]
 *
 * find mouse pointer location
 *
 * @returns 1 when found
 */
static int pointer_get ( Display *display, Window root, int *x, int *y )
{
    *x = 0;
    *y = 0;
    Window       rr, cr;
    int          rxr, ryr, wxr, wyr;
    unsigned int mr;

    if ( XQueryPointer ( display, root, &rr, &cr, &rxr, &ryr, &wxr, &wyr, &mr ) ) {
        *x = rxr;
        *y = ryr;
        return 1;
    }

    return 0;
}

// determine which monitor holds the active window, or failing that the mouse pointer
extern xcb_connection_t *xcb_connection;
void monitor_active ( Display *display, workarea *mon )
{
    Screen *screen = DefaultScreenOfDisplay ( display );
    Window root    = RootWindow ( display, XScreenNumberOfScreen ( screen ) );
    int    x, y;

    if ( config.monitor >= 0 ) {
        if ( monitor_get_dimension ( display, screen, config.monitor, mon ) ) {
            return;
        }
        fprintf ( stderr, "Failed to find selected monitor.\n" );
    }
    // Get the current desktop.
    unsigned int current_desktop = 0;
    if ( config.monitor != -2 && xcb_ewmh_get_current_desktop_reply ( &xcb_ewmh,
                xcb_ewmh_get_current_desktop( &xcb_ewmh, XScreenNumberOfScreen ( screen )), &current_desktop, NULL )) {
            xcb_ewmh_get_desktop_viewport_reply_t vp;
            if ( xcb_ewmh_get_desktop_viewport_reply ( &xcb_ewmh,
                        xcb_ewmh_get_desktop_viewport(&xcb_ewmh, XScreenNumberOfScreen ( screen ) ),
                        &vp, NULL)){
                if ( current_desktop < vp.desktop_viewport_len) {
                    monitor_dimensions ( display, screen, vp.desktop_viewport[current_desktop].x,
                            vp.desktop_viewport[current_desktop].y, mon );
                    return;
                }
            }
    }

    xcb_window_t active_window;
    if ( xcb_ewmh_get_active_window_reply ( &xcb_ewmh,
                xcb_ewmh_get_active_window( &xcb_ewmh, XScreenNumberOfScreen ( screen )), &active_window, NULL )) {
        // get geometry.
        xcb_get_geometry_cookie_t c = xcb_get_geometry ( xcb_connection, active_window);
        xcb_get_geometry_reply_t *r = xcb_get_geometry_reply ( xcb_connection, c, NULL);
        if ( r ) {
            if ( config.monitor == -2 ) {
                xcb_translate_coordinates_cookie_t ct = xcb_translate_coordinates(xcb_connection, active_window, root, r->x, r->y);
                xcb_translate_coordinates_reply_t *t = xcb_translate_coordinates_reply (xcb_connection, ct, NULL);
                if ( t ){
                    // place the menu above the window
                    // if some window is focused, place menu above window, else fall
                    // back to selected monitor.
                    mon->x = t->dst_x;
                    mon->y = t->dst_y;
                    mon->w = r->width;
                    mon->h = r->height;
                    mon->t = r->border_width;
                    mon->b = r->border_width;
                    mon->l = r->border_width;
                    mon->r = r->border_width;
                    free(r);
                    free(t);
                    return;
                }
            }
            monitor_dimensions ( display, screen, r->x, r->y, mon );
            free(r);
            return;
        }
    }
    if ( pointer_get ( display, root, &x, &y ) ) {
        monitor_dimensions ( display, screen, x, y, mon );
        return;
    }

    monitor_dimensions ( display, screen, 0, 0, mon );
}

int window_send_message ( Display *display, Window trg, Window subject, Atom atom, unsigned long protocol, unsigned long mask, Time time )
{
    XEvent e;
    memset ( &e, 0, sizeof ( XEvent ) );
    e.xclient.type         = ClientMessage;
    e.xclient.message_type = atom;
    e.xclient.window       = subject;
    e.xclient.data.l[0]    = protocol;
    e.xclient.data.l[1]    = time;
    e.xclient.send_event   = True;
    e.xclient.format       = 32;
    int r = XSendEvent ( display, trg, False, mask, &e ) ? 1 : 0;
    XFlush ( display );
    return r;
}

int take_keyboard ( Display *display, Window w )
{
    for ( int i = 0; i < 500; i++ ) {
        if ( XGrabKeyboard ( display, w, True, GrabModeAsync, GrabModeAsync,
                             CurrentTime ) == GrabSuccess ) {
            return 1;
        }
        usleep ( 1000 );
    }

    return 0;
}

void release_keyboard ( Display *display )
{
    XUngrabKeyboard ( display, CurrentTime );
}

static unsigned int x11_find_mod_mask ( xkb_stuff *xkb, ... )
{
    va_list         names;
    const char      *name;
    xkb_mod_index_t i;
    unsigned int    mask = 0;
    va_start ( names, xkb );
    while ( ( name = va_arg ( names, const char * ) ) != NULL ) {
        i = xkb_keymap_mod_get_index ( xkb->keymap, name );
        if ( i != XKB_MOD_INVALID ) {
            mask |= 1 << i;
        }
    }
    return mask;
}

static void x11_figure_out_masks ( xkb_stuff *xkb )
{
    x11_mod_masks[X11MOD_SHIFT]   = x11_find_mod_mask ( xkb, XKB_MOD_NAME_SHIFT, NULL );
    x11_mod_masks[X11MOD_CONTROL] = x11_find_mod_mask ( xkb, XKB_MOD_NAME_CTRL, "RControl", "LControl", NULL );
    x11_mod_masks[X11MOD_ALT]     = x11_find_mod_mask ( xkb, XKB_MOD_NAME_ALT, "Alt", "LAlt", "RAlt", "AltGr", "Mod5", "LevelThree", NULL );
    x11_mod_masks[X11MOD_META]    = x11_find_mod_mask ( xkb, "Meta", NULL );
    x11_mod_masks[X11MOD_SUPER]   = x11_find_mod_mask ( xkb, XKB_MOD_NAME_LOGO, "Super", NULL );
    x11_mod_masks[X11MOD_HYPER]   = x11_find_mod_mask ( xkb, "Hyper", NULL );

    gsize i;
    for ( i = 0; i < X11MOD_ANY; ++i ) {
        x11_mod_masks[X11MOD_ANY] |= x11_mod_masks[i];
    }
}

unsigned int x11_canonalize_mask ( unsigned int mask )
{
    // Bits 13 and 14 of the modifiers together are the group number, and
    // should be ignored when looking up key bindings
    mask &= x11_mod_masks[X11MOD_ANY];

    gsize i;
    for ( i = 0; i < X11MOD_ANY; ++i ) {
        if ( mask & x11_mod_masks[i] ) {
            mask |= x11_mod_masks[i];
        }
    }
    return mask;
}

// convert a Mod+key arg to mod mask and keysym
void x11_parse_key ( char *combo, unsigned int *mod, xkb_keysym_t *key )
{
    GString      *str    = g_string_new ( "" );
    unsigned int modmask = 0;

    if ( strcasestr ( combo, "shift" ) ) {
        modmask |= x11_mod_masks[X11MOD_SHIFT];
        if ( x11_mod_masks[X11MOD_SHIFT] == 0 ) {
            g_string_append_printf ( str, "X11 configured keyboard has no <b>Shift</b> key.\n" );
        }
    }
    if ( strcasestr ( combo, "control" ) ) {
        modmask |= x11_mod_masks[X11MOD_CONTROL];
        if ( x11_mod_masks[X11MOD_CONTROL] == 0 ) {
            g_string_append_printf ( str, "X11 configured keyboard has no <b>Control</b> key.\n" );
        }
    }
    if ( strcasestr ( combo, "alt" ) ) {
        modmask |= x11_mod_masks[X11MOD_ALT];
        if ( x11_mod_masks[X11MOD_ALT] == 0 ) {
            g_string_append_printf ( str, "X11 configured keyboard has no <b>Alt</b> key.\n" );
        }
    }
    if ( strcasestr ( combo, "super" ) ) {
        modmask |= x11_mod_masks[X11MOD_SUPER];
        if ( x11_mod_masks[X11MOD_SUPER] == 0 ) {
            g_string_append_printf ( str, "X11 configured keyboard has no <b>Super</b> key.\n" );
        }
    }
    if ( strcasestr ( combo, "meta" ) ) {
        modmask |= x11_mod_masks[X11MOD_META];
        if ( x11_mod_masks[X11MOD_META] == 0 ) {
            g_string_append_printf ( str, "X11 configured keyboard has no <b>Meta</b> key.\n" );
        }
    }
    if ( strcasestr ( combo, "hyper" ) ) {
        modmask |= x11_mod_masks[X11MOD_HYPER];
        if ( x11_mod_masks[X11MOD_HYPER] == 0 ) {
            g_string_append_printf ( str, "X11 configured keyboard has no <b>Hyper</b> key.\n" );
        }
    }
    int seen_mod = FALSE;
    if ( strcasestr ( combo, "Mod" ) ) {
        seen_mod = TRUE;
    }

    *mod = modmask;

    // Skip modifier (if exist) and parse key.
    char i = strlen ( combo );

    while ( i > 0 && !strchr ( "-+", combo[i - 1] ) ) {
        i--;
    }

    xkb_keysym_t sym = xkb_keysym_from_name ( combo + i, XKB_KEYSYM_CASE_INSENSITIVE );

    if ( sym == XKB_KEY_NoSymbol || ( !modmask && ( strchr ( combo, '-' ) || strchr ( combo, '+' ) ) ) ) {
        g_string_append_printf ( str, "Sorry, rofi cannot understand the key combination: <i>%s</i>\n", combo );
        g_string_append ( str, "\nRofi supports the following modifiers:\n\t" );
        g_string_append ( str, "<i>Shift,Control,Alt,AltGR,SuperL,SuperR," );
        g_string_append ( str, "MetaL,MetaR,HyperL,HyperR</i>" );
        if ( seen_mod ) {
            g_string_append ( str, "\n\n<b>Mod1,Mod2,Mod3,Mod4,Mod5 are no longer supported, use one of the above.</b>" );
        }
    }
    if ( str->len > 0 ) {
        show_error_message ( str->str, TRUE );
        g_string_free ( str, TRUE );
        return;
    }
    g_string_free ( str, TRUE );
    *key = sym;
}

void x11_set_window_opacity ( Display *display, Window box, unsigned int opacity )
{
    // Scale 0-100 to 0 - UINT32_MAX.
    unsigned int opacity_set = ( unsigned int ) ( ( opacity / 100.0 ) * UINT32_MAX );
    // Set opacity.
    XChangeProperty ( display, box, netatoms[_NET_WM_WINDOW_OPACITY], XA_CARDINAL, 32, PropModeReplace,
                      ( unsigned char * ) &opacity_set, 1L );
}

/**
 * @param display The connection to the X server.
 *
 * Fill in the list of Atoms.
 */
static void x11_create_frequently_used_atoms ( Display *display )
{
    // X atom values
    for ( int i = 0; i < NUM_NETATOMS; i++ ) {
        netatoms[i] = XInternAtom ( display, netatom_names[i], False );
    }
}

static int ( *xerror )( Display *, XErrorEvent * );
/**
 * @param d  The connection to the X server.
 * @param ee The XErrorEvent
 *
 * X11 Error handler.
 */
static int display_oops ( Display *d, XErrorEvent *ee )
{
    if ( ee->error_code == BadWindow || ( ee->request_code == X_GrabButton && ee->error_code == BadAccess )
         || ( ee->request_code == X_GrabKey && ee->error_code == BadAccess ) ) {
        return 0;
    }

    fprintf ( stderr, "error: request code=%d, error code=%d\n", ee->request_code, ee->error_code );
    return xerror ( d, ee );
}

void x11_setup ( Display *display, xkb_stuff *xkb )
{
    // Set error handle
    XSync ( display, False );
    xerror = XSetErrorHandler ( display_oops );
    XSync ( display, False );

    // determine numlock mask so we can bind on keys with and without it
    x11_figure_out_masks ( xkb );
    x11_create_frequently_used_atoms ( display );
}

void x11_create_visual_and_colormap ( xcb_connection_t *xcb_connection, xcb_screen_t *xcb_screen )
{
    xcb_depth_iterator_t depth_iter;
    for ( depth_iter = xcb_screen_allowed_depths_iterator ( xcb_screen ); depth_iter.rem; xcb_depth_next ( &depth_iter ) ) {
        xcb_depth_t               *d = depth_iter.data;

        xcb_visualtype_iterator_t visual_iter;
        for ( visual_iter = xcb_depth_visuals_iterator ( d ); visual_iter.rem; xcb_visualtype_next ( &visual_iter ) ) {
            xcb_visualtype_t *v = visual_iter.data;
            if ( ( d->depth == 32 ) && ( v->_class == XCB_VISUAL_CLASS_TRUE_COLOR ) ) {
                depth  = d;
                visual = v;
            }
            if ( xcb_screen->root_visual == v->visual_id ) {
                root_depth  = d;
                root_visual = v;
            }
        }
    }
    if ( visual != NULL ) {
        xcb_void_cookie_t   c;
        xcb_generic_error_t *e;
        map = xcb_generate_id ( xcb_connection );
        c   = xcb_create_colormap_checked ( xcb_connection, XCB_COLORMAP_ALLOC_NONE, map, xcb_screen->root, visual->visual_id );
        e   = xcb_request_check ( xcb_connection, c );
        if ( e ) {
            depth  = NULL;
            visual = NULL;
            free ( e );
        }
    }

    if ( visual == NULL ) {
        depth  = root_depth;
        visual = root_visual;
        map    = xcb_screen->default_colormap;
    }
}

Color color_get ( const char *const name )
{
    char *copy  = g_strdup ( name );
    char *cname = g_strstrip ( copy );

    union
    {
        struct
        {
            uint8_t b;
            uint8_t g;
            uint8_t r;
            uint8_t a;
        };
        uint32_t pixel;
    } color = {
        .r = 0xff,
        .g = 0xff,
        .b = 0xff,
        .a = 0xff,
    };
    // Special format.
    if ( strncmp ( cname, "argb:", 5 ) == 0 ) {
        color.pixel = strtoul ( &cname[5], NULL, 16 );
    }
    else if ( strncmp ( cname, "#", 1 ) == 0 ) {
        color.pixel = strtoul ( &cname[1], NULL, 16 );
        color.a     = 0xff;
    }
    g_free ( copy );

    Color ret = {
        .red   = color.r / 255.0,
        .green = color.g / 255.0,
        .blue  = color.b / 255.0,
        .alpha = color.a / 255.0,
    };
    return ret;
}

void x11_helper_set_cairo_rgba ( cairo_t *d, Color col )
{
    cairo_set_source_rgba ( d, col.red, col.green, col.blue, col.alpha );
}
/**
 * Color cache.
 *
 * This stores the current color until
 */
enum
{
    BACKGROUND,
    BORDER,
    SEPARATOR
};
static struct
{
    Color        color;
    unsigned int set;
} color_cache[3];

void color_cache_reset ( void )
{
    color_cache[BACKGROUND].set = FALSE;
    color_cache[BORDER].set     = FALSE;
    color_cache[SEPARATOR].set  = FALSE;
}
void color_background ( cairo_t *d )
{
    if ( !color_cache[BACKGROUND].set ) {
        if ( !config.color_enabled ) {
            color_cache[BACKGROUND].color = color_get ( config.menu_bg );
        }
        else {
            gchar **vals = g_strsplit ( config.color_window, ",", 3 );
            if ( vals != NULL && vals[0] != NULL ) {
                color_cache[BACKGROUND].color = color_get ( vals[0] );
            }
            g_strfreev ( vals );
        }
        color_cache[BACKGROUND].set = TRUE;
    }

    x11_helper_set_cairo_rgba ( d, color_cache[BACKGROUND].color );
}

void color_border ( cairo_t *d  )
{
    if ( !color_cache[BORDER].set ) {
        if ( !config.color_enabled ) {
            color_cache[BORDER].color = color_get ( config.menu_bc );
        }
        else {
            gchar **vals = g_strsplit ( config.color_window, ",", 3 );
            if ( vals != NULL && vals[0] != NULL && vals[1] != NULL ) {
                color_cache[BORDER].color = color_get ( vals[1] );
            }
            g_strfreev ( vals );
        }
        color_cache[BORDER].set = TRUE;
    }
    x11_helper_set_cairo_rgba ( d, color_cache[BORDER].color );
}

void color_separator ( cairo_t *d )
{
    if ( !color_cache[SEPARATOR].set ) {
        if ( !config.color_enabled ) {
            color_cache[SEPARATOR].color = color_get ( config.menu_bc );
        }
        else {
            gchar **vals = g_strsplit ( config.color_window, ",", 3 );
            if ( vals != NULL && vals[0] != NULL && vals[1] != NULL && vals[2] != NULL  ) {
                color_cache[SEPARATOR].color = color_get ( vals[2] );
            }
            else if ( vals != NULL && vals[0] != NULL && vals[1] != NULL ) {
                color_cache[SEPARATOR].color = color_get ( vals[1] );
            }
            g_strfreev ( vals );
        }
        color_cache[SEPARATOR].set = TRUE;
    }
    x11_helper_set_cairo_rgba ( d, color_cache[SEPARATOR].color );
}
