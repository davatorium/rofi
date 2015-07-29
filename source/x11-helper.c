/**
 * rofi
 *
 * MIT/X11 License
 * Copyright (c) 2012 Sean Pringle <sean.pringle@gmail.com>
 * Modified 2013-2015 Qball Cow <qball@gmpclient.org>
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

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/extensions/Xinerama.h>

#include <rofi.h>
#define OVERLAP( a, b, c, d )                      ( ( ( a ) == ( c ) && ( b ) == ( d ) ) || MIN ( ( a ) + ( b ), ( c ) + ( d ) ) - MAX ( ( a ), ( c ) ) > 0 )
#define INTERSECT( x, y, w, h, x1, y1, w1, h1 )    ( OVERLAP ( ( x ), ( w ), ( x1 ), ( w1 ) ) && OVERLAP ( ( y ), ( h ), ( y1 ), ( h1 ) ) )
#include "x11-helper.h"

Atom         netatoms[NUM_NETATOMS];
const char   *netatom_names[] = { EWMH_ATOMS ( ATOM_CHAR ) };
// Mask indicating num-lock.
unsigned int NumlockMask = 0;

// retrieve a property of any type from a window
int window_get_prop ( Display *display, Window w, Atom prop,
                      Atom *type, int *items,
                      void *buffer, unsigned int bytes )
{
    int           format;
    unsigned long nitems, nbytes;
    unsigned char *ret = NULL;
    memset ( buffer, 0, bytes );

    if ( XGetWindowProperty ( display, w, prop, 0, bytes / 4, False, AnyPropertyType, type,
                              &format, &nitems, &nbytes, &ret ) == Success && ret && *type != None && format ) {
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

void window_set_atom_prop ( Display *display, Window w, Atom prop, Atom *atoms, int count )
{
    XChangeProperty ( display, w, prop, XA_ATOM, 32, PropModeReplace, ( unsigned char * ) atoms, count );
}

int window_get_cardinal_prop ( Display *display, Window w, Atom atom, unsigned long *list, int count )
{
    Atom type; int items;
    return window_get_prop ( display, w, atom, &type, &items, list, count * sizeof ( unsigned long ) ) && type == XA_CARDINAL ? items : 0;
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
void monitor_active ( Display *display, workarea *mon )
{
    Screen *screen = DefaultScreenOfDisplay ( display );
    Window root    = RootWindow ( display, XScreenNumberOfScreen ( screen ) );
    int    x, y;

    Window id;
    Atom   type;
    int    count;
    if ( window_get_prop ( display, root, netatoms[_NET_ACTIVE_WINDOW], &type, &count, &id, sizeof ( Window ) )
         && type == XA_WINDOW && count > 0 ) {
        XWindowAttributes attr;
        if ( XGetWindowAttributes ( display, id, &attr ) ) {
            Window junkwin;
            if ( XTranslateCoordinates ( display, id, attr.root,
                                         -attr.border_width,
                                         -attr.border_width,
                                         &x, &y, &junkwin ) == True ) {
                monitor_dimensions ( display, screen, x, y, mon );
                return;
            }
        }
    }
    if ( pointer_get ( display, root, &x, &y ) ) {
        monitor_dimensions ( display, screen, x, y, mon );
        return;
    }

    monitor_dimensions ( display, screen, 0, 0, mon );
}

int window_send_message ( Display *display, Window target,
                          Window subject, Atom atom,
                          unsigned long protocol, unsigned long mask, Time time )
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
    int r = XSendEvent ( display, target, False, mask, &e ) ? 1 : 0;
    XFlush ( display );
    return r;
}

int take_keyboard ( Display *display, Window w )
{
    for ( int i = 0; i < 500; i++ ) {
        if ( XGrabKeyboard ( display, w, True, GrabModeAsync, GrabModeAsync, CurrentTime ) == GrabSuccess ) {
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
// bind a key combination on a root window, compensating for Lock* states
void x11_grab_key ( Display *display, unsigned int modmask, KeySym key )
{
    Screen  *screen = DefaultScreenOfDisplay ( display );
    Window  root    = RootWindow ( display, XScreenNumberOfScreen ( screen ) );
    KeyCode keycode = XKeysymToKeycode ( display, key );

    // bind to combinations of mod and lock masks, so caps and numlock don't confuse people
    XGrabKey ( display, keycode, modmask, root, True, GrabModeAsync, GrabModeAsync );
    XGrabKey ( display, keycode, modmask | LockMask, root, True, GrabModeAsync, GrabModeAsync );

    if ( NumlockMask ) {
        XGrabKey ( display, keycode, modmask | NumlockMask, root, True, GrabModeAsync, GrabModeAsync );
        XGrabKey ( display, keycode, modmask | NumlockMask | LockMask, root, True, GrabModeAsync, GrabModeAsync );
    }
}

void x11_ungrab_key ( Display *display, unsigned int modmask, KeySym key )
{
    Screen  *screen = DefaultScreenOfDisplay ( display );
    Window  root    = RootWindow ( display, XScreenNumberOfScreen ( screen ) );
    KeyCode keycode = XKeysymToKeycode ( display, key );

    // unbind to combinations of mod and lock masks, so caps and numlock don't confuse people
    XUngrabKey ( display, keycode, modmask, root );
    XUngrabKey ( display, keycode, modmask | LockMask, root );

    if ( NumlockMask ) {
        XUngrabKey ( display, keycode, modmask | NumlockMask, root );
        XUngrabKey ( display, keycode, modmask | NumlockMask | LockMask, root );
    }
}
/**
 * @param display The connection to the X server.
 *
 * Figure out what entry in the modifiermap is NumLock.
 * This sets global variable: NumlockMask
 */
static void x11_figure_out_numlock_mask ( Display *display )
{
    XModifierKeymap *modmap = XGetModifierMapping ( display );
    KeyCode         kc      = XKeysymToKeycode ( display, XK_Num_Lock );
    for ( int i = 0; i < 8; i++ ) {
        for ( int j = 0; j < ( int ) modmap->max_keypermod; j++ ) {
            if ( modmap->modifiermap[i * modmap->max_keypermod + j] == kc ) {
                NumlockMask = ( 1 << i );
            }
        }
    }
    XFreeModifiermap ( modmap );
}

// convert a Mod+key arg to mod mask and keysym
void x11_parse_key ( char *combo, unsigned int *mod, KeySym *key )
{
    unsigned int modmask = 0;

    if ( strcasestr ( combo, "shift" ) ) {
        modmask |= ShiftMask;
    }

    if ( strcasestr ( combo, "control" ) ) {
        modmask |= ControlMask;
    }

    if ( strcasestr ( combo, "mod1" ) ) {
        modmask |= Mod1Mask;
    }

    if ( strcasestr ( combo, "alt" ) ) {
        modmask |= Mod1Mask;
    }

    if ( strcasestr ( combo, "mod2" ) ) {
        modmask |= Mod2Mask;
    }

    if ( strcasestr ( combo, "mod3" ) ) {
        modmask |= Mod3Mask;
    }

    if ( strcasestr ( combo, "mod4" ) ) {
        modmask |= Mod4Mask;
    }

    if ( strcasestr ( combo, "mod5" ) ) {
        modmask |= Mod5Mask;
    }

    *mod = modmask;

    // Skip modifier (if exist) and parse key.
    char i = strlen ( combo );

    while ( i > 0 && !strchr ( "-+", combo[i - 1] ) ) {
        i--;
    }

    KeySym sym = XStringToKeysym ( combo + i );

    if ( sym == NoSymbol || ( !modmask && ( strchr ( combo, '-' ) || strchr ( combo, '+' ) ) ) ) {
        // TODO popup
        fprintf ( stderr, "sorry, cannot understand key combination: %s\n", combo );
    }

    *key = sym;
}

void x11_set_window_opacity ( Display *display, Window box, unsigned int opacity )
{
    // Scale 0-100 to 0 - UINT32_MAX.
    unsigned int opacity_set = ( unsigned int ) ( ( opacity / 100.0 ) * UINT32_MAX );
    // Set opacity.
    XChangeProperty ( display, box, netatoms[_NET_WM_WINDOW_OPACITY],
                      XA_CARDINAL, 32, PropModeReplace,
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
    if ( ee->error_code == BadWindow
         || ( ee->request_code == X_GrabButton && ee->error_code == BadAccess )
         || ( ee->request_code == X_GrabKey && ee->error_code == BadAccess )
         ) {
        return 0;
    }

    fprintf ( stderr, "error: request code=%d, error code=%d\n", ee->request_code, ee->error_code );
    return xerror ( d, ee );
}

void x11_setup ( Display *display )
{
    // Set error handle
    XSync ( display, False );
    xerror = XSetErrorHandler ( display_oops );
    XSync ( display, False );

    // determine numlock mask so we can bind on keys with and without it
    x11_figure_out_numlock_mask ( display );
    x11_create_frequently_used_atoms ( display );
}



extern Colormap    map;
extern XVisualInfo vinfo;
int                truecolor = FALSE;
void create_visual_and_colormap ( Display *display )
{
    int screen = DefaultScreen ( display );
    // Try to create TrueColor map
    if ( XMatchVisualInfo ( display, screen, 32, TrueColor, &vinfo ) ) {
        // Visual found, lets try to create map.
        map       = XCreateColormap ( display, DefaultRootWindow ( display ), vinfo.visual, AllocNone );
        truecolor = TRUE;
    }
    // Failed to create map.
    // Use the defaults then.
    if ( map == None ) {
        truecolor = FALSE;
        // Two fields we use.
        vinfo.visual = DefaultVisual ( display, screen );
        vinfo.depth  = DefaultDepth ( display, screen );
        map          = DefaultColormap ( display, screen );
    }
}
unsigned int color_get ( Display *display, const char *const name )
{
    XColor color = { 0, 0, 0, 0, 0, 0 };
    // Special format.
    if ( strncmp ( name, "argb:", 5 ) == 0 ) {
        color.pixel = strtoul ( &name[5], NULL, 16 );
        color.red   = ( ( color.pixel & 0x00FF0000 ) >> 16 ) * 255;
        color.green = ( ( color.pixel & 0x0000FF00 ) >> 8  ) * 255;
        color.blue  = ( ( color.pixel & 0x000000FF )       ) * 255;
        if ( !truecolor ) {
            // This will drop alpha part.
            Status st = XAllocColor ( display, map, &color );
            if ( st == None ) {
                fprintf ( stderr, "Failed to parse color: '%s'\n", name );
                exit ( EXIT_FAILURE );
            }
            return color.pixel;
        }
        return color.pixel;
    }
    else {
        Status st = XAllocNamedColor ( display, map, name, &color, &color );
        if ( st == None ) {
            fprintf ( stderr, "Failed to parse color: '%s'\n", name );
            exit ( EXIT_FAILURE );
        }
        return color.pixel;
    }
}

unsigned int color_background ( Display *display )
{
    if ( !config.color_enabled ) {
        return color_get ( display, config.menu_bg );
    }
    else {
        unsigned int retv = 0;

        gchar        **vals = g_strsplit ( config.color_window, ",", 2 );
        if ( vals != NULL && vals[0] != NULL ) {
            retv = color_get ( display, g_strstrip ( vals[0] ) );
        }
        g_strfreev ( vals );
        return retv;
    }
}

unsigned int color_border ( Display *display )
{
    if ( !config.color_enabled ) {
        return color_get ( display, config.menu_bc );
    }
    else {
        unsigned int retv = 0;

        gchar        **vals = g_strsplit ( config.color_window, ",", 2 );
        if ( vals != NULL && vals[0] != NULL && vals[1] != NULL ) {
            retv = color_get ( display, vals[1] );
        }
        g_strfreev ( vals );
        return retv;
    }
}
