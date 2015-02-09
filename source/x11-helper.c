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

#define OVERLAP( a, b, c, d )                      ( ( ( a ) == ( c ) && ( b ) == ( d ) ) || MIN ( ( a ) + ( b ), ( c ) + ( d ) ) - MAX ( ( a ), ( c ) ) > 0 )
#define INTERSECT( x, y, w, h, x1, y1, w1, h1 )    ( OVERLAP ( ( x ), ( w ), ( x1 ), ( w1 ) ) && OVERLAP ( ( y ), ( h ), ( y1 ), ( h1 ) ) )
#include "x11-helper.h"

#define WINLIST    32

// Mask indicating num-lock.
unsigned int NumlockMask = 0;

winlist      *cache_client = NULL;
winlist      *cache_xattr  = NULL;

winlist* winlist_new ()
{
    winlist *l = g_malloc ( sizeof ( winlist ) );
    l->len   = 0;
    l->array = g_malloc_n ( WINLIST + 1, sizeof ( Window ) );
    l->data  = g_malloc_n ( WINLIST + 1, sizeof ( void* ) );
    return l;
}

int winlist_append ( winlist *l, Window w, void *d )
{
    if ( l->len > 0 && !( l->len % WINLIST ) ) {
        l->array = g_realloc ( l->array, sizeof ( Window ) * ( l->len + WINLIST + 1 ) );
        l->data  = g_realloc ( l->data, sizeof ( void* ) * ( l->len + WINLIST + 1 ) );
    }
    // Make clang-check happy.
    // TODO: make clang-check clear this should never be 0.
    if ( l->data == NULL || l->array == NULL ) {
        return 0;
    }

    l->data[l->len]    = d;
    l->array[l->len++] = w;
    return l->len - 1;
}

void winlist_empty ( winlist *l )
{
    while ( l->len > 0 ) {
        g_free ( l->data[--( l->len )] );
    }
}

void winlist_free ( winlist *l )
{
    if ( l != NULL ) {
        winlist_empty ( l );
        g_free ( l->array );
        g_free ( l->data );
        g_free ( l );
    }
}

int winlist_find ( winlist *l, Window w )
{
// iterate backwards. theory is: windows most often accessed will be
// nearer the end. testing with kcachegrind seems to support this...
    int i;

    for ( i = ( l->len - 1 ); i >= 0; i-- ) {
        if ( l->array[i] == w ) {
            return i;
        }
    }

    return -1;
}


XWindowAttributes* window_get_attributes ( Display *display, Window w )
{
    int idx = winlist_find ( cache_xattr, w );

    if ( idx < 0 ) {
        XWindowAttributes *cattr = g_malloc ( sizeof ( XWindowAttributes ) );

        if ( XGetWindowAttributes ( display, w, cattr ) ) {
            winlist_append ( cache_xattr, w, cattr );
            return cattr;
        }

        g_free ( cattr );
        return NULL;
    }

    return cache_xattr->data[idx];
}

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
            res = g_malloc ( strlen ( ( char * ) prop.value ) + 1 );
            // make clang-check happy.
            if ( res ) {
                strcpy ( res, ( char * ) prop.value );
            }
        }
        else if ( Xutf8TextPropertyToTextList ( display, &prop, &list, &count ) >= Success && count > 0 && *list ) {
            res = g_malloc ( strlen ( *list ) + 1 );
            // make clang-check happy.
            if ( res ) {
                strcpy ( res, *list );
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


void x11_cache_create ( void )
{
    cache_client = winlist_new ();
    cache_xattr  = winlist_new ();
}

void x11_cache_empty ( void )
{
    winlist_empty ( cache_xattr );
    winlist_empty ( cache_client );
}

void x11_cache_free ( void )
{
    winlist_free ( cache_xattr );
    winlist_free ( cache_client );
}

// find the dimensions of the monitor displaying point x,y
static void monitor_dimensions ( Display *display, Screen *screen, int x, int y, workarea *mon )
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

// _NET_WM_STATE_*
int client_has_state ( client *c, Atom state )
{
    for ( int i = 0; i < c->states; i++ ) {
        if ( c->state[i] == state ) {
            return 1;
        }
    }

    return 0;
}

client* window_client ( Display *display, Window win )
{
    if ( win == None ) {
        return NULL;
    }

    int idx = winlist_find ( cache_client, win );

    if ( idx >= 0 ) {
        return cache_client->data[idx];
    }

    // if this fails, we're up that creek
    XWindowAttributes *attr = window_get_attributes ( display, win );

    if ( !attr ) {
        return NULL;
    }

    client *c = g_malloc0 ( sizeof ( client ) );
    c->window = win;

    // copy xattr so we don't have to care when stuff is freed
    memmove ( &c->xattr, attr, sizeof ( XWindowAttributes ) );
    XGetTransientForHint ( display, win, &c->trans );

    c->states = window_get_atom_prop ( display, win, netatoms[_NET_WM_STATE], c->state, CLIENTSTATE );

    char *name;

    if ( ( name = window_get_text_prop ( display, c->window, netatoms[_NET_WM_NAME] ) ) && name ) {
        snprintf ( c->title, CLIENTTITLE, "%s", name );
        g_free ( name );
    }
    else if ( XFetchName ( display, c->window, &name ) ) {
        snprintf ( c->title, CLIENTTITLE, "%s", name );
        XFree ( name );
    }

    name = window_get_text_prop ( display, c->window, XInternAtom ( display, "WM_WINDOW_ROLE", False ) );

    if ( name != NULL ) {
        snprintf ( c->role, CLIENTROLE, "%s", name );
        XFree ( name );
    }

    XClassHint chint;

    if ( XGetClassHint ( display, c->window, &chint ) ) {
        snprintf ( c->class, CLIENTCLASS, "%s", chint.res_class );
        snprintf ( c->name, CLIENTNAME, "%s", chint.res_name );
        XFree ( chint.res_class );
        XFree ( chint.res_name );
    }

    monitor_dimensions ( display, c->xattr.screen, c->xattr.x, c->xattr.y, &c->monitor );
    winlist_append ( cache_client, c->window, c );
    return c;
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
        XWindowAttributes *attr = window_get_attributes ( display, id );
        if ( attr != NULL ) {
            Window junkwin;
            if ( XTranslateCoordinates ( display, id, attr->root,
                                         -attr->border_width,
                                         -attr->border_width,
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
    if ( XGrabKeyboard ( display, w, True, GrabModeAsync, GrabModeAsync, CurrentTime ) == GrabSuccess ) {
        return 1;
    }

    return 0;
}

void release_keyboard ( Display *display )
{
    XUngrabKeyboard ( display, CurrentTime );
}
// bind a key combination on a root window, compensating for Lock* states
void grab_key ( Display *display, unsigned int modmask, KeySym key )
{
    Screen  *screen = DefaultScreenOfDisplay ( display );
    Window  root    = RootWindow ( display, XScreenNumberOfScreen ( screen ) );
    KeyCode keycode = XKeysymToKeycode ( display, key );
    XUngrabKey ( display, keycode, AnyModifier, root );

    if ( modmask != AnyModifier ) {
        // bind to combinations of mod and lock masks, so caps and numlock don't confuse people
        XGrabKey ( display, keycode, modmask, root, True, GrabModeAsync, GrabModeAsync );
        XGrabKey ( display, keycode, modmask | LockMask, root, True, GrabModeAsync, GrabModeAsync );

        if ( NumlockMask ) {
            XGrabKey ( display, keycode, modmask | NumlockMask, root, True, GrabModeAsync, GrabModeAsync );
            XGrabKey ( display, keycode, modmask | NumlockMask | LockMask, root, True, GrabModeAsync, GrabModeAsync );
        }
    }
    else{
        // nice simple single key bind
        XGrabKey ( display, keycode, AnyModifier, root, True, GrabModeAsync, GrabModeAsync );
    }
}

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

    // If no modifier mask is set, allow any modifier.
    *mod = modmask ? modmask : AnyModifier;

    // Skip modifier (if exist) and parse key.
    char i = strlen ( combo );

    while ( i > 0 && !strchr ( "-+", combo[i - 1] ) ) {
        i--;
    }

    KeySym sym = XStringToKeysym ( combo + i );

    if ( sym == NoSymbol || ( !modmask && ( strchr ( combo, '-' ) || strchr ( combo, '+' ) ) ) ) {
        fprintf ( stderr, "sorry, cannot understand key combination: %s\n", combo );
        exit ( EXIT_FAILURE );
    }

    *key = sym;
}

void x11_set_window_opacity ( Display *display, Window box, unsigned int opacity )
{
    // Hack to set window opacity.
    unsigned int opacity_set = ( unsigned int ) ( ( opacity / 100.0 ) * UINT32_MAX );
    XChangeProperty ( display, box, netatoms[_NET_WM_WINDOW_OPACITY],
                      XA_CARDINAL, 32, PropModeReplace,
                      ( unsigned char * ) &opacity_set, 1L );
}

static void x11_create_frequently_used_atoms ( Display *display )
{
    // X atom values
    for ( int i = 0; i < NUM_NETATOMS; i++ ) {
        netatoms[i] = XInternAtom ( display, netatom_names[i], False );
    }
}


static int ( *xerror )( Display *, XErrorEvent * );
/**
 * @param d The display on witch the error occurred.
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
    x11_cache_create ();
    x11_create_frequently_used_atoms ( display );
}
