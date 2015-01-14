/**
 * rofi
 *
 * MIT/X11 License
 * Copyright (c) 2012 Sean Pringle <sean.pringle@gmail.com>
 * Modified 2013-2014 Qball  Cow <qball@gmpclient.org>
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
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <ctype.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <time.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/extensions/Xinerama.h>

#include "helper.h"
#include "rofi.h"

#ifdef HAVE_I3_IPC_H
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <i3/ipc.h>
#endif

#include "run-dialog.h"
#include "ssh-dialog.h"
#include "dmenu-dialog.h"
#include "script-dialog.h"

#include "xrmoptions.h"
#include "textbox.h"

#define LINE_MARGIN    3

#ifdef HAVE_I3_IPC_H
// This setting is no longer user configurable, but partial to this file:
int  config_i3_mode = 0;
// Path to HAVE_I3_IPC_H socket.
char *i3_socket_path = NULL;
#endif

const char   *cache_dir   = NULL;
unsigned int NumlockMask  = 0;
Display      *display     = NULL;
char         *display_str = NULL;


typedef struct _Switcher
{
    char              name[32];
    switcher_callback cb;
    void              *cb_data;
    textbox           *tb;
} Switcher;

Switcher     *switchers    = NULL;
unsigned int num_switchers = 0;
unsigned int curr_switcher = 0;


/**
 * @param name Name of the switcher to lookup.
 *
 * Find the index of the switcher with name.
 *
 * @returns index of the switcher in switchers, -1 if not found.
 */
static int switcher_get ( const char *name )
{
    for ( unsigned int i = 0; i < num_switchers; i++ ) {
        if ( strcmp ( switchers[i].name, name ) == 0 ) {
            return i;
        }
    }
    return -1;
}


#ifdef HAVE_I3_IPC_H
// Focus window on HAVE_I3_IPC_H window manager.
static void focus_window_i3 ( const char *socket_path, int id )
{
    i3_ipc_header_t    head;
    char               command[128];
    int                s, len;
    ssize_t            t;
    struct sockaddr_un remote;

    if ( strlen ( socket_path ) > UNIX_PATH_MAX ) {
        fprintf ( stderr, "Socket path is to long. %zd > %d\n", strlen ( socket_path ), UNIX_PATH_MAX );
        return;
    }

    if ( ( s = socket ( AF_UNIX, SOCK_STREAM, 0 ) ) == -1 ) {
        fprintf ( stderr, "Failed to open connection to I3: %s\n", strerror ( errno ) );
        return;
    }

    remote.sun_family = AF_UNIX;
    strcpy ( remote.sun_path, socket_path );
    len = strlen ( remote.sun_path ) + sizeof ( remote.sun_family );

    if ( connect ( s, ( struct sockaddr * ) &remote, len ) == -1 ) {
        fprintf ( stderr, "Failed to connect to I3 (%s): %s\n", socket_path, strerror ( errno ) );
        close ( s );
        return;
    }


    // Formulate command
    snprintf ( command, 128, "[id=\"%d\"] focus", id );
    // Prepare header.
    memcpy ( head.magic, I3_IPC_MAGIC, 6 );
    head.size = strlen ( command );
    head.type = I3_IPC_MESSAGE_TYPE_COMMAND;
    // Send header.
    t = send ( s, &head, sizeof ( i3_ipc_header_t ), 0 );
    if ( t == -1 ) {
        char *msg = g_strdup_printf ( "Failed to send message header to i3: %s\n", strerror ( errno ) );
        error_dialog ( msg );
        g_free ( msg );
        close ( s );
        return;
    }
    // Send message
    t = send ( s, command, strlen ( command ), 0 );
    if ( t == -1 ) {
        char *msg = g_strdup_printf ( "Failed to send message body to i3: %s\n", strerror ( errno ) );
        error_dialog ( msg );
        g_free ( msg );
        close ( s );
        return;
    }
    // Receive header.
    t = recv ( s, &head, sizeof ( head ), 0 );

    if ( t == sizeof ( head ) ) {
        t = recv ( s, command, head.size, 0 );
        if ( t == head.size ) {
            // Response.
        }
    }

    close ( s );
}
#endif

void catch_exit ( __attribute__( ( unused ) ) int sig )
{
    while ( 0 < waitpid ( -1, NULL, WNOHANG ) ) {
        ;
    }
}



static int ( *xerror )( Display *, XErrorEvent * );

#define ATOM_ENUM( x )    x
#define ATOM_CHAR( x )    # x

#define EWMH_ATOMS( X )               \
    X ( _NET_CLIENT_LIST_STACKING ),  \
    X ( _NET_NUMBER_OF_DESKTOPS ),    \
    X ( _NET_CURRENT_DESKTOP ),       \
    X ( _NET_ACTIVE_WINDOW ),         \
    X ( _NET_WM_NAME ),               \
    X ( _NET_WM_STATE ),              \
    X ( _NET_WM_STATE_SKIP_TASKBAR ), \
    X ( _NET_WM_STATE_SKIP_PAGER ),   \
    X ( _NET_WM_STATE_ABOVE ),        \
    X ( _NET_WM_DESKTOP ),            \
    X ( I3_SOCKET_PATH ),             \
    X ( CLIPBOARD ),                  \
    X ( UTF8_STRING ),                \
    X ( _NET_WM_WINDOW_OPACITY )

enum { EWMH_ATOMS ( ATOM_ENUM ), NUM_NETATOMS };
const char *netatom_names[] = { EWMH_ATOMS ( ATOM_CHAR ) };
Atom       netatoms[NUM_NETATOMS];

// X error handler
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

// usable space on a monitor
typedef struct
{
    int x, y, w, h;
    int l, r, t, b;
} workarea;


// window lists
typedef struct
{
    Window *array;
    void   **data;
    int    len;
} winlist;

winlist *cache_client = NULL;
winlist *cache_xattr  = NULL;

#define WINLIST    32

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
    winlist_empty ( l );
    g_free ( l->array );
    g_free ( l->data );
    g_free ( l );
}
int winlist_find ( winlist *l, Window w )
{
// iterate backwards. theory is: windows most often accessed will be
// nearer the end. testing with kcachegrind seems to support this...
    int    i;
    Window o = 0;

    for ( i = ( l->len - 1 ); i >= 0; i-- ) {
        if ( l->array[i] == w ) {
            return i;
        }
    }

    return -1;
}

#define CLIENTTITLE    100
#define CLIENTCLASS    50
#define CLIENTNAME     50
#define CLIENTSTATE    10
#define CLIENTROLE     50

// a managable window
typedef struct
{
    Window            window, trans;
    XWindowAttributes xattr;
    char              title[CLIENTTITLE];
    char              class[CLIENTCLASS];
    char              name[CLIENTNAME];
    char              role[CLIENTROLE];
    int               states;
    Atom              state[CLIENTSTATE];
    workarea          monitor;
    int               active;
} client;


unsigned int windows_modmask;
KeySym       windows_keysym;
unsigned int rundialog_modmask;
KeySym       rundialog_keysym;
unsigned int sshdialog_modmask;
KeySym       sshdialog_keysym;

Window       main_window = None;
GC           gc          = NULL;

// g_malloc a pixel value for an X named color
static unsigned int color_get ( Display *display, const char *const name )
{
    int      screen_id = DefaultScreen ( display );
    XColor   color;
    Colormap map = DefaultColormap ( display, screen_id );
    return XAllocNamedColor ( display, map, name, &color, &color ) ? color.pixel : None;
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

static int take_keyboard ( Window w )
{
    int i;

    for ( i = 0; i < 1000; i++ ) {
        if ( XGrabKeyboard ( display, w, True, GrabModeAsync, GrabModeAsync, CurrentTime ) == GrabSuccess ) {
            return 1;
        }

        struct timespec rsl = { 0, 100000L };
        nanosleep ( &rsl, NULL );
    }

    return 0;
}
static void release_keyboard ()
{
    XUngrabKeyboard ( display, CurrentTime );
}

// XGetWindowAttributes with caching
static XWindowAttributes* window_get_attributes ( Window w )
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
static int window_get_prop ( Window w, Atom prop, Atom *type, int *items, void *buffer, unsigned int bytes ) __attribute__ ( ( nonnull ( 3, 4 ) ) );
static int window_get_prop ( Window w, Atom prop, Atom *type, int *items, void *buffer, unsigned int bytes )
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
static char* window_get_text_prop ( Window w, Atom atom )
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

static int window_get_atom_prop ( Window w, Atom atom, Atom *list, int count )
{
    Atom type;
    int  items;
    return window_get_prop ( w, atom, &type, &items, list, count * sizeof ( Atom ) ) && type == XA_ATOM ? items : 0;
}

static void window_set_atom_prop ( Window w, Atom prop, Atom *atoms, int count )
{
    XChangeProperty ( display, w, prop, XA_ATOM, 32, PropModeReplace, ( unsigned char * ) atoms, count );
}

static int window_get_cardinal_prop ( Window w, Atom atom, unsigned long *list, int count )
{
    Atom type; int items;
    return window_get_prop ( w, atom, &type, &items, list, count * sizeof ( unsigned long ) ) && type == XA_CARDINAL ? items : 0;
}

// a ClientMessage
static int window_send_message ( Window target, Window subject, Atom atom, unsigned long protocol, unsigned long mask, Time time )
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

// find the dimensions of the monitor displaying point x,y
static void monitor_dimensions ( Screen *screen, int x, int y, workarea *mon )
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

// determine which monitor holds the active window, or failing that the mouse pointer
static void monitor_active ( workarea *mon )
{
    Screen *screen = DefaultScreenOfDisplay ( display );
    Window root    = RootWindow ( display, XScreenNumberOfScreen ( screen ) );
    int    x, y;

    Window id;
    Atom   type;
    int    count;
    if ( window_get_prop ( root, netatoms[_NET_ACTIVE_WINDOW], &type, &count, &id, sizeof ( Window ) )
         && type == XA_WINDOW && count > 0 ) {
        XWindowAttributes *attr = window_get_attributes ( id );
        if ( attr != NULL ) {
            Window junkwin;
            if ( XTranslateCoordinates ( display, id, attr->root,
                                         -attr->border_width,
                                         -attr->border_width,
                                         &x, &y, &junkwin ) == True ) {
                monitor_dimensions ( screen, x, y, mon );
                return;
            }
        }
    }
    if ( pointer_get ( display, root, &x, &y ) ) {
        monitor_dimensions ( screen, x, y, mon );
        return;
    }

    monitor_dimensions ( screen, 0, 0, mon );
}

// _NET_WM_STATE_*
static int client_has_state ( client *c, Atom state )
{
    int i;

    for ( i = 0; i < c->states; i++ ) {
        if ( c->state[i] == state ) {
            return 1;
        }
    }

    return 0;
}

// collect info on any window
// doesn't have to be a window we'll end up managing
static client* window_client ( Window win )
{
    if ( win == None ) {
        return NULL;
    }

    int idx = winlist_find ( cache_client, win );

    if ( idx >= 0 ) {
        return cache_client->data[idx];
    }

    // if this fails, we're up that creek
    XWindowAttributes *attr = window_get_attributes ( win );

    if ( !attr ) {
        return NULL;
    }

    client *c = g_malloc0 ( sizeof ( client ) );
    c->window = win;

    // copy xattr so we don't have to care when stuff is freed
    memmove ( &c->xattr, attr, sizeof ( XWindowAttributes ) );
    XGetTransientForHint ( display, win, &c->trans );

    c->states = window_get_atom_prop ( win, netatoms[_NET_WM_STATE], c->state, CLIENTSTATE );

    char *name;

    if ( ( name = window_get_text_prop ( c->window, netatoms[_NET_WM_NAME] ) ) && name ) {
        snprintf ( c->title, CLIENTTITLE, "%s", name );
        g_free ( name );
    }
    else if ( XFetchName ( display, c->window, &name ) ) {
        snprintf ( c->title, CLIENTTITLE, "%s", name );
        XFree ( name );
    }

    name = window_get_text_prop ( c->window, XInternAtom ( display, "WM_WINDOW_ROLE", False ) );

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

    monitor_dimensions ( c->xattr.screen, c->xattr.x, c->xattr.y, &c->monitor );
    winlist_append ( cache_client, c->window, c );
    return c;
}



static void menu_hide_arrow_text ( int filtered_lines, int selected, int max_elements,
                                   textbox *arrowbox_top, textbox *arrowbox_bottom )
{
    if ( arrowbox_top == NULL || arrowbox_bottom == NULL ) {
        return;
    }
    int page   = ( filtered_lines > 0 ) ? selected / max_elements : 0;
    int npages = ( filtered_lines > 0 ) ? ( ( filtered_lines + max_elements - 1 ) / max_elements ) : 1;
    if ( !( page != 0 && npages > 1 ) ) {
        textbox_hide ( arrowbox_top );
    }
    if ( !( ( npages - 1 ) != page && npages > 1 ) ) {
        textbox_hide ( arrowbox_bottom );
    }
}

static void menu_set_arrow_text ( int filtered_lines, int selected, int max_elements,
                                  textbox *arrowbox_top, textbox *arrowbox_bottom )
{
    if ( arrowbox_top == NULL || arrowbox_bottom == NULL ) {
        return;
    }
    if ( filtered_lines == 0 || max_elements == 0 ) {
        return;
    }
    int page   = ( filtered_lines > 0 ) ? selected / max_elements : 0;
    int npages = ( filtered_lines > 0 ) ? ( ( filtered_lines + max_elements - 1 ) / max_elements ) : 1;
    int entry  = selected % max_elements;
    if ( page != 0 && npages > 1 ) {
        textbox_show ( arrowbox_top );
        textbox_font ( arrowbox_top, ( entry != 0 ) ? NORMAL : HIGHLIGHT );
        textbox_draw ( arrowbox_top  );
    }
    if ( ( npages - 1 ) != page && npages > 1 ) {
        textbox_show ( arrowbox_bottom );
        textbox_font ( arrowbox_bottom, ( entry != ( max_elements - 1 ) ) ? NORMAL : HIGHLIGHT );
        textbox_draw ( arrowbox_bottom  );
    }
}


static int window_match ( char **tokens, __attribute__( ( unused ) ) const char *input,
                          int case_sensitive, int index, void *data )
{
    int     match = 1;
    winlist *ids  = ( winlist * ) data;
    client  *c    = window_client ( ids->array[index] );

    if ( tokens ) {
        for ( int j = 0; match && tokens[j]; j++ ) {
            int test = 0;

            if ( !test && c->title[0] != '\0' ) {
                char *key = token_collate_key ( c->title, case_sensitive );
                test = ( strstr ( key, tokens[j] ) != NULL );
                g_free ( key );
            }

            if ( !test && c->class[0] != '\0' ) {
                char *key = token_collate_key ( c->class, case_sensitive );
                test = ( strstr ( key, tokens[j] ) != NULL );
                g_free ( key );
            }

            if ( !test && c->role[0] != '\0' ) {
                char *key = token_collate_key ( c->role, case_sensitive );
                test = ( strstr ( key, tokens[j] ) != NULL );
                g_free ( key );
            }

            if ( !test && c->name[0] != '\0' ) {
                char *key = token_collate_key ( c->name, case_sensitive );
                test = ( strstr ( key, tokens[j] ) != NULL );
                g_free ( key );
            }

            if ( test == 0 ) {
                match = 0;
            }
        }
    }

    return match;
}

static int lev_sort ( const void *p1, const void *p2, void *arg )
{
    const int *a         = p1;
    const int *b         = p2;
    int       *distances = arg;

    return distances[*a] - distances[*b];
}

static int dist ( const char *s, const char *t, int *d, int ls, int lt, int i, int j )
{
    if ( d[i * ( lt + 1 ) + j] >= 0 ) {
        return d[i * ( lt + 1 ) + j];
    }

    int x;
    if ( i == ls ) {
        x = lt - j;
    }
    else if ( j == lt ) {
        x = ls - i;
    }
    else if ( s[i] == t[j] ) {
        x = dist ( s, t, d, ls, lt, i + 1, j + 1 );
    }
    else {
        x = dist ( s, t, d, ls, lt, i + 1, j + 1 );

        int y;
        if ( ( y = dist ( s, t, d, ls, lt, i, j + 1 ) ) < x ) {
            x = y;
        }
        if ( ( y = dist ( s, t, d, ls, lt, i + 1, j ) ) < x ) {
            x = y;
        }
        x++;
    }
    return d[i * ( lt + 1 ) + j] = x;
}
static int levenshtein ( const char *s, const char *t )
{
    int    ls           = strlen ( s ), lt = strlen ( t );
    size_t array_length = ( ls + 1 ) * ( lt + 1 );

    // For some reason Coverity does not get that I initialize the
    // array in for loop.
    int *d = g_malloc_n ( array_length, sizeof ( int ) );

    for ( size_t i = 0; i < array_length; i++ ) {
        d[i] = -1;
    }

    int retv = dist ( s, t, d, ls, lt, 0, 0 );
    g_free ( d );
    return retv;
}

static void window_set_opacity ( Display *display, Window box, unsigned int opacity )
{
    // Hack to set window opacity.
    unsigned int opacity_set = ( unsigned int ) ( ( opacity / 100.0 ) * UINT32_MAX );
    XChangeProperty ( display, box, netatoms[_NET_WM_WINDOW_OPACITY],
                      XA_CARDINAL, 32, PropModeReplace,
                      ( unsigned char * ) &opacity_set, 1L );
}

Window create_window ( Display *display )
{
    Screen *screen = DefaultScreenOfDisplay ( display );
    Window root    = RootWindow ( display, XScreenNumberOfScreen ( screen ) );
    Window box     = XCreateSimpleWindow ( display, root, 0, 0, 200, 100,
                                           config.menu_bw,
                                           color_get ( display, config.menu_bc ),
                                           color_get ( display, config.menu_bg ) );

    XSelectInput ( display, box, ExposureMask | ButtonPressMask );

    gc = XCreateGC ( display, box, 0, 0 );
    XSetLineAttributes ( display, gc, 2, LineOnOffDash, CapButt, JoinMiter );
    XSetForeground ( display, gc, color_get ( display, config.menu_bc ) );
    // make it an unmanaged window
    window_set_atom_prop ( box, netatoms[_NET_WM_STATE], &netatoms[_NET_WM_STATE_ABOVE], 1 );
    XSetWindowAttributes sattr;
    sattr.override_redirect = True;
    XChangeWindowAttributes ( display, box, CWOverrideRedirect, &sattr );

    // Set the WM_NAME
    XStoreName ( display, box, "rofi" );

    window_set_opacity ( display, box, config.window_opacity );
    return box;
}

// State of the menu.

typedef struct MenuState
{
    unsigned int menu_lines;
    unsigned int max_elements;
    unsigned int max_rows;
    unsigned int columns;

    // window width,height
    unsigned int w, h;
    int          x, y;
    unsigned int element_width;

    // Update/Refilter list.
    int          update;
    int          refilter;

    // Entries
    textbox      *text;
    textbox      *prompt_tb;
    textbox      *case_indicator;
    textbox      *arrowbox_top;
    textbox      *arrowbox_bottom;
    textbox      **boxes;
    char         **filtered;
    int          *distance;
    int          *line_map;

    unsigned int num_lines;

    // Selected element.
    unsigned int selected;
    unsigned int filtered_lines;
    // Last offset in paginating.
    unsigned int last_offset;

    KeySym       prev_key;
    Time         last_button_press;

    int          quit;
    int          init;
    // Return state
    int          *selected_line;
    MenuReturn   retv;
    char         **lines;
}MenuState;

/**
 * @param state Internal state of the menu.
 *
 * Free the allocated fields in the state.
 */
static void menu_free_state ( MenuState *state )
{
    textbox_free ( state->text );
    textbox_free ( state->prompt_tb );
    textbox_free ( state->case_indicator );
    textbox_free ( state->arrowbox_bottom );
    textbox_free ( state->arrowbox_top );

    for ( unsigned int i = 0; i < state->max_elements; i++ ) {
        textbox_free ( state->boxes[i] );
    }

    g_free ( state->boxes );

    g_free ( state->filtered );
    g_free ( state->line_map );
    g_free ( state->distance );
}

/**
 * @param x [out] the calculated x position.
 * @param y [out] the calculated y position.
 * @param mon     the workarea.
 * @param h       the required height of the window.
 * @param w       the required width of the window.
 */
static void calculate_window_position ( MenuState *state, const workarea *mon )
{
    // Default location is center.
    state->y = mon->y + ( mon->h - state->h - config.menu_bw * 2 ) / 2;
    state->x = mon->x + ( mon->w - state->w - config.menu_bw * 2 ) / 2;
    // Determine window location
    switch ( config.location )
    {
    case WL_NORTH_WEST:
        state->x = mon->x;

    case WL_NORTH:
        state->y = mon->y;
        break;

    case WL_NORTH_EAST:
        state->y = mon->y;

    case WL_EAST:
        state->x = mon->x + mon->w - state->w - config.menu_bw * 2;
        break;

    case WL_EAST_SOUTH:
        state->x = mon->x + mon->w - state->w - config.menu_bw * 2;

    case WL_SOUTH:
        state->y = mon->y + mon->h - state->h - config.menu_bw * 2;
        break;

    case WL_SOUTH_WEST:
        state->y = mon->y + mon->h - state->h - config.menu_bw * 2;

    case WL_WEST:
        state->x = mon->x;
        break;

    case WL_CENTER:
    default:
        break;
    }
    // Apply offset.
    state->x += config.x_offset;
    state->y += config.y_offset;
}


/**
 * @param state Internal state of the menu.
 * @param num_lines the number of entries passed to the menu.
 *
 * Calculate the number of rows, columns and elements to display based on the
 * configuration and available data.
 */
static void menu_calculate_rows_columns ( MenuState *state )
{
    state->columns      = config.menu_columns;
    state->max_elements = MIN ( state->menu_lines * state->columns, state->num_lines );

    // Calculate the number or rows. We do this by getting the num_lines rounded up to X columns
    // (num elements is better name) then dividing by columns.
    state->max_rows = MIN ( state->menu_lines,
                            (unsigned int) (
                                ( state->num_lines + ( state->columns - state->num_lines % state->columns ) %
                                  state->columns ) / ( state->columns )
                                ) );

    if ( config.fixed_num_lines == TRUE ) {
        state->max_elements = state->menu_lines * state->columns;
        state->max_rows     = state->menu_lines;
        // If it would fit in one column, only use one column.
        if ( state->num_lines < state->max_elements ) {
            state->columns = ( state->num_lines + ( state->max_rows - state->num_lines % state->max_rows ) %
                               state->max_rows ) / state->max_rows;
            state->max_elements = state->menu_lines * state->columns;
        }
        // Sanitize.
        if ( state->columns == 0 ) {
            state->columns = 1;
        }
    }
    // More hacks.
    if ( config.hmode == TRUE ) {
        state->max_rows = 1;
    }
}

/**
 * @returns The width of the window.
 */
static int window_get_width ( )
{
    int width = 0;
    workarea mon;

    monitor_active ( &mon );

    if ( config.menu_width < 0 ) {
        double fw = textbox_get_estimated_char_width ( );
        width += -( fw * config.menu_width );
        width += 2 * config.padding + 4; // 4 = 2*SIDE_MARGIN
    }
    else{
        // Calculate as float to stop silly, big rounding down errors.
        width = config.menu_width < 101 ? ( mon.w / 100.0f ) * ( float ) config.menu_width : config.menu_width;
    }

    // Compensate for border width.
    width -= config.menu_bw * 2;

    return width;
}

/**
 * @param state Internal state of the menu.
 * @param mon the dimensions of the monitor rofi is displayed on.
 *
 * Calculate the width of the window and the width of an element.
 */
static void menu_calculate_window_and_element_width ( MenuState *state, workarea *mon )
{
    state->w = window_get_width ( );

    if ( state->columns > 0 ) {
        state->element_width = state->w - ( 2 * ( config.padding ) );
        // Divide by the # columns
        state->element_width = ( state->element_width - ( state->columns - 1 ) * LINE_MARGIN ) / state->columns;
        if ( config.hmode == TRUE ) {
            state->element_width = ( state->w - ( 2 * ( config.padding ) ) - state->max_elements * LINE_MARGIN ) / (
                state->max_elements + 1 );
        }
    }
}

/**
 * @param state Internal state of the menu.
 * @param key the Key being pressed.
 * @param modstate the modifier state.
 *
 * Keyboard navigation through the elements.
 */
static void menu_keyboard_navigation ( MenuState *state, KeySym key, unsigned int modstate )
{
    if ( key == XK_Escape
         // pressing one of the global key bindings closes the switcher. this allows fast closing of the menu if an item is not selected
         || ( ( windows_modmask == AnyModifier || modstate & windows_modmask ) && key == windows_keysym )
         || ( ( rundialog_modmask == AnyModifier || modstate & rundialog_modmask ) && key == rundialog_keysym )
         || ( ( sshdialog_modmask == AnyModifier || modstate & sshdialog_modmask ) && key == sshdialog_keysym )
         ) {
        state->retv = MENU_CANCEL;
        state->quit = TRUE;
    }
    // Up, Ctrl-p or Shift-Tab
    else if ( key == XK_Up || ( key == XK_Tab && modstate & ShiftMask ) ||
              ( key == XK_p && modstate & ControlMask )  ) {
        if ( state->selected == 0 ) {
            state->selected = state->filtered_lines;
        }

        if ( state->selected > 0 ) {
            state->selected--;
        }
        state->update = TRUE;
    }
    else if ( key == XK_Tab ) {
        if ( state->filtered_lines == 1 ) {
            if ( state->filtered[state->selected] ) {
                state->retv               = MENU_OK;
                *( state->selected_line ) = state->line_map[state->selected];
                state->quit               = 1;
            }
            else{
                fprintf ( stderr, "We should never hit this." );
                abort ();
            }
            return;
        }

        // Double tab!
        if ( state->filtered_lines == 0 && key == state->prev_key ) {
            state->retv               = MENU_NEXT;
            *( state->selected_line ) = 0;
            state->quit               = TRUE;
        }
        else{
            state->selected = state->selected < state->filtered_lines - 1 ? MIN (
                state->filtered_lines - 1, state->selected + 1 ) : 0;
            state->update = TRUE;
        }
    }
    // Down, Ctrl-n
    else if ( key == XK_Down ||
              ( key == XK_n && ( modstate & ControlMask ) ) ) {
        state->selected = state->selected < state->filtered_lines - 1 ? MIN (
            state->filtered_lines - 1, state->selected + 1 ) : 0;
        state->update = TRUE;
    }
    else if ( key == XK_Page_Up && ( modstate & ControlMask ) ) {
        if ( state->selected < state->max_rows ) {
            state->selected = 0;
        }
        else{
            state->selected -= state->max_rows;
        }
        state->update = TRUE;
    }
    else if (  key == XK_Page_Down && ( modstate & ControlMask ) ) {
        state->selected += state->max_rows;
        if ( state->selected >= state->filtered_lines ) {
            state->selected = state->filtered_lines - 1;
        }
        state->update = TRUE;
    }
    else if ( key == XK_Page_Up ) {
        if ( state->selected < state->max_elements ) {
            state->selected = 0;
        }
        else{
            state->selected -= ( state->max_elements );
        }
        state->update = TRUE;
    }
    else if ( key == XK_Page_Down ) {
        state->selected += ( state->max_elements );
        if ( state->selected >= state->filtered_lines ) {
            state->selected = state->filtered_lines - 1;
        }
        state->update = TRUE;
    }
    else if ( key == XK_Home || key == XK_KP_Home ) {
        state->selected = 0;
        state->update   = TRUE;
    }
    else if ( key == XK_End || key == XK_KP_End ) {
        state->selected = state->filtered_lines - 1;
        state->update   = TRUE;
    }
    else if ( key == XK_space && ( modstate & ControlMask ) == ControlMask ) {
        // If a valid item is selected, return that..
        if ( state->selected < state->filtered_lines && state->filtered[state->selected] != NULL ) {
            textbox_text ( state->text, state->lines[state->line_map[state->selected]] );
            textbox_cursor_end ( state->text );
            state->update   = TRUE;
            state->refilter = TRUE;
        }
    }
    state->prev_key = key;
}

/**
 * @param state Internal state of the menu.
 * @param xbe   The mouse button press event.
 *
 * mouse navigation through the elements.
 *
 * TODO: Scroll wheel.
 */
static void menu_mouse_navigation ( MenuState *state, XButtonEvent *xbe )
{
    if ( xbe->window == state->arrowbox_top->window ) {
        // Page up.
        if ( state->selected < state->max_rows ) {
            state->selected = 0;
        }
        else{
            state->selected -= state->max_elements;
        }
        state->update = TRUE;
    }
    else if ( xbe->window == state->arrowbox_bottom->window ) {
        // Page down.
        state->selected += state->max_elements;
        if ( state->selected >= state->filtered_lines ) {
            state->selected = state->filtered_lines - 1;
        }
        state->update = TRUE;
    }
    else {
        for ( unsigned int i = 0; config.sidebar_mode == TRUE && i < num_switchers; i++ ) {
            if ( switchers[i].tb->window == ( xbe->window ) ) {
                *( state->selected_line ) = i;
                state->retv               = MENU_QUICK_SWITCH;
                state->quit               = TRUE;
                return;
            }
        }
        for ( unsigned int i = 0; i < state->max_elements; i++ ) {
            if ( ( xbe->window ) == ( state->boxes[i]->window ) ) {
                // Only allow items that are visible to be selected.
                if ( ( state->last_offset + i ) >= state->filtered_lines ) {
                    continue;
                }
                //
                state->selected = state->last_offset + i;
                state->update   = TRUE;
                if ( ( xbe->time - state->last_button_press ) < 200 ) {
                    state->retv               = MENU_OK;
                    *( state->selected_line ) = state->line_map[state->selected];
                    // Quit
                    state->quit = TRUE;
                    break;
                }
                state->last_button_press = xbe->time;
            }
        }
    }
}

static void menu_refilter ( MenuState *state, char **lines, menu_match_cb mmc, void *mmc_data,
                            int sorting, int case_sensitive )
{
    unsigned int i, j = 0;
    if ( strlen ( state->text->text ) > 0 ) {
        char **tokens = tokenize ( state->text->text, case_sensitive );

        // input changed
        for ( i = 0; i < state->num_lines; i++ ) {
            int match = mmc ( tokens, lines[i], case_sensitive, i, mmc_data );

            // If each token was matched, add it to list.
            if ( match ) {
                state->line_map[j] = i;
                if ( sorting ) {
                    state->distance[i] = levenshtein ( state->text->text, lines[i] );
                }
                // Try to look-up the selected line and highlight that.
                // This is needed 'hack' to fix the dmenu 'next row' modi.
                // int to unsigned int is valid because we check negativeness of
                // selected_line
                if ( state->init == TRUE && ( state->selected_line ) != NULL &&
                     ( *( state->selected_line ) ) >= 0 &&
                     ( (unsigned int) ( *( state->selected_line ) ) ) == i ) {
                    state->selected = j;
                    state->init     = FALSE;
                }
                j++;
            }
        }
        if ( sorting ) {
            qsort_r ( state->line_map, j, sizeof ( int ), lev_sort, state->distance );
        }
        // Update the filtered list.
        for ( i = 0; i < j; i++ ) {
            state->filtered[i] = lines[state->line_map[i]];
        }
        for ( i = j; i < state->num_lines; i++ ) {
            state->filtered[i] = NULL;
        }

        // Cleanup + bookkeeping.
        state->filtered_lines = j;
        g_strfreev ( tokens );
    }
    else{
        for ( i = 0; i < state->num_lines; i++ ) {
            state->filtered[i] = lines[i];
            state->line_map[i] = i;
        }
        state->filtered_lines = state->num_lines;
    }
    state->selected = MIN ( state->selected, state->filtered_lines - 1 );

    state->refilter = FALSE;
}


static void menu_draw ( MenuState *state )
{
    unsigned int i, offset = 0;

    // selected row is always visible.
    // If selected is visible do not scroll.
    if ( ( ( state->selected - ( state->last_offset ) ) < ( state->max_elements ) )
         && ( state->selected >= ( state->last_offset ) ) ) {
        offset = state->last_offset;
    }
    else{
        // Do paginating
        int page = ( state->max_elements > 0 ) ? ( state->selected / state->max_elements ) : 0;
        offset             = page * state->max_elements;
        state->last_offset = offset;
    }

    for ( i = 0; i < state->max_elements; i++ ) {
        if ( ( i + offset ) >= state->num_lines || state->filtered[i + offset] == NULL ) {
            textbox_font ( state->boxes[i], NORMAL );
            textbox_text ( state->boxes[i], "" );
        }
        else{
            TextBoxFontType type  = ( ( ( i % state->max_rows ) & 1 ) == 0 ) ? NORMAL : ALT;
            char            *text = state->filtered[i + offset];
            TextBoxFontType tbft  = ( i + offset ) == state->selected ? HIGHLIGHT : type;
            textbox_font ( state->boxes[i], tbft );
            textbox_text ( state->boxes[i], text );
        }

        textbox_draw ( state->boxes[i] );
    }
}

static void menu_update ( MenuState *state )
{
    menu_hide_arrow_text ( state->filtered_lines, state->selected,
                           state->max_elements, state->arrowbox_top,
                           state->arrowbox_bottom );
    textbox_draw ( state->case_indicator );
    textbox_draw ( state->prompt_tb );
    textbox_draw ( state->text );
    menu_draw ( state );
    menu_set_arrow_text ( state->filtered_lines, state->selected,
                          state->max_elements, state->arrowbox_top,
                          state->arrowbox_bottom );
    // Why do we need the specian -1?
    if ( config.hmode == FALSE ) {
        int line_height = textbox_get_height ( state->text );
        XDrawLine ( display, main_window, gc, ( config.padding ),
                    line_height + ( config.padding ) + ( LINE_MARGIN  ) / 2,
                    state->w - ( ( config.padding ) ) - 1,
                    line_height + ( config.padding ) + ( LINE_MARGIN  ) / 2 );
    }

    if ( config.sidebar_mode == TRUE ) {
        int line_height = textbox_get_height ( state->text );
        XDrawLine ( display, main_window, gc,
                    ( config.padding ),
                    state->h - line_height - ( config.padding ) - 1,
                    state->w - ( ( config.padding ) ) - 1,
                    state->h - line_height - ( config.padding ) - 1 );
        for ( int j = 0; j < num_switchers; j++ ) {
            textbox_draw ( switchers[j].tb );
        }
    }


    state->update = FALSE;
}

/**
 * @param state Internal state of the menu.
 * @param xse   X selection event.
 *
 * Handle paste event.
 */
static void menu_paste ( MenuState *state, XSelectionEvent *xse )
{
    if ( xse->property == netatoms[UTF8_STRING] ) {
        char          *pbuf = NULL;
        int           di;
        unsigned long dl, rm;
        Atom          da;

        /* we have been given the current selection, now insert it into input */
        XGetWindowProperty (
            display,
            main_window,
            netatoms[UTF8_STRING],
            0,
            256 / 4,       // max length in words.
            False,         // Do not delete clipboard.
            netatoms[UTF8_STRING], &da, &di, &dl, &rm, (unsigned char * *) &pbuf );
        // If There was remaining data left.. lets ignore this.
        // Only accept it when we get bytes!
        if ( di == 8 ) {
            char *index;
            if ( ( index = strchr ( pbuf, '\n' ) ) != NULL ) {
                // Calc new length;
                dl = index - pbuf;
            }
            // Create a NULL terminated string. I am not sure how the data is returned.
            // With or without trailing 0
            char str[dl + 1];
            memcpy ( str, pbuf, dl );
            str[dl] = '\0';

            // Insert string move cursor.
            textbox_insert ( state->text, state->text->cursor, str );
            textbox_cursor ( state->text, state->text->cursor + dl );
            // Force a redraw and refiltering of the text.
            state->update   = TRUE;
            state->refilter = TRUE;
        }
        XFree ( pbuf );
    }
}

MenuReturn menu ( char **lines, unsigned int num_lines, char **input, char *prompt, Time *time,
                  int *shift, menu_match_cb mmc, void *mmc_data, int *selected_line, int sorting )
{
    MenuState    state = {
        .selected_line     = selected_line,
        .retv              = MENU_CANCEL,
        .prev_key          =             0,
        .last_button_press =             0,
        .last_offset       =             0,
        .num_lines         = num_lines,
        .distance          = NULL,
        .init              = TRUE,
        .quit              = FALSE,
        .filtered_lines    =             0,
        .max_elements      =             0,
        // We want to filter on the first run.
        .refilter = TRUE,
        .update   = FALSE,
        .lines    = lines
    };
    unsigned int i;
    workarea     mon;



    // main window isn't explicitly destroyed in case we switch modes. Reusing it prevents flicker
    XWindowAttributes attr;
    if ( main_window == None || XGetWindowAttributes ( display, main_window, &attr ) == 0 ) {
        main_window = create_window ( display );
    }
    // Get active monitor size.
    monitor_active ( &mon );


    // we need this at this point so we can get height.
    state.case_indicator = textbox_create ( main_window, TB_AUTOHEIGHT | TB_AUTOWIDTH,
                                            ( config.padding ), ( config.padding ),
                                            0, 0,
                                            NORMAL, "*" );

    // Height of a row.
    int line_height = textbox_get_height ( state.case_indicator );
    if ( config.menu_lines == 0 ) {
        // Autosize it.
        int h = mon.h - config.padding * 2 - LINE_MARGIN - config.menu_bw * 2;
        int r = ( h ) / ( line_height * config.element_height ) - 1 - config.sidebar_mode;
        state.menu_lines = r;
    }
    else {
        state.menu_lines = config.menu_lines;
    }
    menu_calculate_rows_columns ( &state );
    menu_calculate_window_and_element_width ( &state, &mon );

    // Prompt box.
    state.prompt_tb = textbox_create ( main_window, TB_AUTOHEIGHT | TB_AUTOWIDTH,
                                       ( config.padding ),
                                       ( config.padding ),
                                       0, 0, NORMAL, prompt );
    // Entry box
    int entrybox_width = (
        ( config.hmode == TRUE ) ? state.element_width : ( state.w - ( 2 * ( config.padding ) ) ) )
                         - textbox_get_width ( state.prompt_tb )
                         - textbox_get_width ( state.case_indicator );

    state.text = textbox_create ( main_window, TB_AUTOHEIGHT | TB_EDITABLE,
                                  ( config.padding ) + textbox_get_width ( state.prompt_tb ),
                                  ( config.padding ),
                                  entrybox_width, 0,
                                  NORMAL,
                                  ( input != NULL ) ? *input : "" );
    // Move indicator to end.
    textbox_move ( state.case_indicator,
                   config.padding + textbox_get_width ( state.prompt_tb ) + entrybox_width,
                   0 );

    textbox_show ( state.text );
    textbox_show ( state.prompt_tb );

    if ( config.case_sensitive ) {
        textbox_show ( state.case_indicator );
    }




    int element_height = line_height * config.element_height;
    // filtered list display
    state.boxes = g_malloc0_n ( state.max_elements, sizeof ( textbox* ) );

    int y_offset = config.padding + ( ( config.hmode == FALSE ) ? line_height : 0 );
    int x_offset = config.padding + ( ( config.hmode == FALSE ) ? 0 : ( state.element_width + LINE_MARGIN ) );

    for ( i = 0; i < state.max_elements; i++ ) {
        int line = ( i ) % state.max_rows;
        int col  = ( i ) / state.max_rows;

        int ex = col * ( state.element_width + LINE_MARGIN );
        int ey = line * element_height + ( ( config.hmode == TRUE ) ? 0 : LINE_MARGIN );

        state.boxes[i] = textbox_create ( main_window, 0,
                                          ex + x_offset,
                                          ey + y_offset,
                                          state.element_width, element_height, NORMAL, "" );
        textbox_show ( state.boxes[i] );
    }
    // Arrows
    state.arrowbox_top = textbox_create ( main_window, TB_AUTOHEIGHT | TB_AUTOWIDTH,
                                          ( config.padding ),
                                          ( config.padding ),
                                          0, 0,
                                          NORMAL,
                                          ( config.hmode == FALSE ) ? "↑" : "←" );
    state.arrowbox_bottom = textbox_create ( main_window, TB_AUTOHEIGHT | TB_AUTOWIDTH,
                                             ( config.padding ),
                                             ( config.padding ),
                                             0, 0,
                                             NORMAL,
                                             ( config.hmode == FALSE ) ? "↓" : "→" );

    if ( config.hmode == FALSE ) {
        textbox_move ( state.arrowbox_top,
                       state.w - config.padding - state.arrowbox_top->w,
                       config.padding + line_height + LINE_MARGIN );
        textbox_move ( state.arrowbox_bottom,
                       state.w - config.padding - state.arrowbox_bottom->w,
                       config.padding + state.max_rows * element_height + LINE_MARGIN );
    }
    else {
        textbox_move ( state.arrowbox_bottom,
                       state.w - config.padding - state.arrowbox_top->w,
                       config.padding );
        textbox_move ( state.arrowbox_top,
                       state.w - config.padding - state.arrowbox_bottom->w - state.arrowbox_top->w,
                       config.padding );
    }

    // filtered list
    state.filtered = (char * *) g_malloc0_n ( state.num_lines, sizeof ( char* ) );
    state.line_map = g_malloc0_n ( state.num_lines, sizeof ( int ) );
    if ( sorting ) {
        state.distance = (int *) g_malloc0_n ( state.num_lines, sizeof ( int ) );
    }

    // resize window vertically to suit
    // Subtract the margin of the last row.
    state.h = line_height + element_height * state.max_rows + ( config.padding ) * 2 + LINE_MARGIN;
    if ( config.hmode == TRUE ) {
        state.h = line_height + ( config.padding ) * 2;
    }
    // Add entry
    if ( config.sidebar_mode == TRUE ) {
        state.h += line_height + LINE_MARGIN;
    }

    // Sidebar mode.
    if ( config.menu_lines == 0 ) {
        state.h = mon.h - config.menu_bw * 2;
    }

    // Move the window to the correct x,y position.
    calculate_window_position ( &state, &mon );

    if ( config.sidebar_mode == TRUE ) {
        int line_height = textbox_get_height ( state.text );
        int width       = ( state.w - ( 2 * ( config.padding ) ) ) / num_switchers;
        for ( int j = 0; j < num_switchers; j++ ) {
            switchers[j].tb = textbox_create ( main_window, TB_CENTER,
                                               config.padding + j * width, state.h - line_height - config.padding,
                                               width, line_height, ( j == curr_switcher ) ? HIGHLIGHT : NORMAL, switchers[j].name );
            textbox_show ( switchers[j].tb );
        }
    }

    // Display it.
    XMoveResizeWindow ( display, main_window, state.x, state.y, state.w, state.h );
    XMapRaised ( display, main_window );

    // if grabbing keyboard failed, fall through
    if ( take_keyboard ( main_window ) ) {
        state.selected = 0;
        // The cast to unsigned in here is valid, we checked if selected_line > 0.
        // So its maximum range is 0-2³¹, well within the num_lines range.
        if ( ( *( state.selected_line ) ) >= 0 && (unsigned int) ( *( state.selected_line ) ) <= state.num_lines ) {
            state.selected = *( state.selected_line );
        }

        state.quit = FALSE;
        while ( !state.quit ) {
            // If something changed, refilter the list. (paste or text entered)
            if ( state.refilter ) {
                menu_refilter ( &state, lines, mmc, mmc_data, sorting, config.case_sensitive );
            }
            // Update if requested.
            if ( state.update ) {
                menu_update ( &state );
            }

            // Wait for event.
            XEvent ev;
            XNextEvent ( display, &ev );

            // Handle event.
            if ( ev.type == Expose ) {
                while ( XCheckTypedEvent ( display, Expose, &ev ) ) {
                    ;
                }
                state.update = TRUE;
            }
            // Button press event.
            else if ( ev.type == ButtonPress ) {
                while ( XCheckTypedEvent ( display, ButtonPress, &ev ) ) {
                    ;
                }
                menu_mouse_navigation ( &state, &( ev.xbutton ) );
            }
            // Paste event.
            else if ( ev.type == SelectionNotify ) {
                while ( XCheckTypedEvent ( display, SelectionNotify, &ev ) ) {
                    ;
                }
                menu_paste ( &state, &( ev.xselection ) );
            }
            // Key press event.
            else if ( ev.type == KeyPress ) {
                while ( XCheckTypedEvent ( display, KeyPress, &ev ) ) {
                    ;
                }

                if ( time ) {
                    *time = ev.xkey.time;
                }

                KeySym key = XkbKeycodeToKeysym ( display, ev.xkey.keycode, 0, 0 );

                // Handling of paste
                if ( ( ( ( ev.xkey.state & ControlMask ) == ControlMask ) && key == XK_v ) ) {
                    XConvertSelection ( display, ( ev.xkey.state & ShiftMask ) ?
                                        XA_PRIMARY : netatoms[CLIPBOARD],
                                        netatoms[UTF8_STRING], netatoms[UTF8_STRING], main_window, CurrentTime );
                }
                else if ( key == XK_Insert ) {
                    XConvertSelection ( display, ( ev.xkey.state & ShiftMask ) ?
                                        XA_PRIMARY : netatoms[CLIPBOARD],
                                        netatoms[UTF8_STRING], netatoms[UTF8_STRING], main_window, CurrentTime );
                }
                else if ( ( ( ev.xkey.state & ControlMask ) == ControlMask ) && key == XK_slash ) {
                    state.retv               = MENU_PREVIOUS;
                    *( state.selected_line ) = 0;
                    state.quit               = TRUE;
                    break;
                }
                // Menu navigation.
                else if ( ( ( ev.xkey.state & ShiftMask ) == ShiftMask ) &&
                          key == XK_slash ) {
                    state.retv               = MENU_NEXT;
                    *( state.selected_line ) = 0;
                    state.quit               = TRUE;
                    break;
                }
                // Toggle case sensitivity.
                else if ( key == XK_grave || key == XK_dead_grave ) {
                    config.case_sensitive    = !config.case_sensitive;
                    *( state.selected_line ) = 0;
                    state.refilter           = TRUE;
                    state.update             = TRUE;
                    if ( config.case_sensitive ) {
                        textbox_show ( state.case_indicator );
                    }
                    else {
                        textbox_hide ( state.case_indicator );
                    }
                }
                // Switcher short-cut
                else if ( ( ( ev.xkey.state & Mod1Mask ) == Mod1Mask ) &&
                          key >= XK_1 && key <= XK_9 ) {
                    *( state.selected_line ) = ( key - XK_1 );
                    state.retv               = MENU_QUICK_SWITCH;
                    state.quit               = TRUE;
                    break;
                }
                // Special delete entry command.
                else if ( ( ( ev.xkey.state & ShiftMask ) == ShiftMask ) &&
                          key == XK_Delete ) {
                    if ( state.filtered[state.selected] != NULL ) {
                        *( state.selected_line ) = state.line_map[state.selected];
                        state.retv               = MENU_ENTRY_DELETE;
                        state.quit               = TRUE;
                        break;
                    }
                }
                else{
                    int rc = textbox_keypress ( state.text, &ev );
                    // Row is accepted.
                    if ( rc < 0 ) {
                        if ( shift != NULL ) {
                            ( *shift ) = ( ( ev.xkey.state & ShiftMask ) == ShiftMask );
                        }

                        // If a valid item is selected, return that..
                        if ( state.selected < state.filtered_lines && state.filtered[state.selected] != NULL ) {
                            *( state.selected_line ) = state.line_map[state.selected];
                            if ( strlen ( state.text->text ) > 0 && rc == -2 ) {
                                state.retv = MENU_CUSTOM_INPUT;
                            }
                            else {
                                state.retv = MENU_OK;
                            }
                        }
                        else if ( strlen ( state.text->text ) > 0 ) {
                            state.retv = MENU_CUSTOM_INPUT;
                        }
                        else{
                            // Nothing entered and nothing selected.
                            state.retv = MENU_CANCEL;
                        }

                        state.quit = TRUE;
                    }
                    // Key press is handled by entry box.
                    else if ( rc > 0 ) {
                        state.refilter = TRUE;
                        state.update   = TRUE;
                    }
                    // Other keys.
                    else{
                        // unhandled key
                        menu_keyboard_navigation ( &state, key, ev.xkey.state );
                    }
                }
            }
        }

        release_keyboard ();
    }

    // Update input string.
    g_free ( *input );
    *input = g_strdup ( state.text->text );

    int retv = state.retv;
    menu_free_state ( &state );

    // Free the switcher boxes.
    // When state is free'ed we should no longer need these.
    if ( config.sidebar_mode == TRUE ) {
        for ( int j = 0; j < num_switchers; j++ ) {
            textbox_free ( switchers[j].tb );
            switchers[j].tb = NULL;
        }
    }

    return retv;
}

void error_dialog ( char *msg )
{
    MenuState state = {
        .selected_line     = NULL,
        .retv              = MENU_CANCEL,
        .prev_key          =           0,
        .last_button_press =           0,
        .last_offset       =           0,
        .num_lines         =           0,
        .distance          = NULL,
        .init              = FALSE,
        .quit              = FALSE,
        .filtered_lines    =           0,
        .columns           =           0,
        .update            = TRUE,
    };
    workarea  mon;
    // Get active monitor size.
    monitor_active ( &mon );
    // main window isn't explicitly destroyed in case we switch modes. Reusing it prevents flicker
    XWindowAttributes attr;
    if ( main_window == None || XGetWindowAttributes ( display, main_window, &attr ) == 0 ) {
        main_window = create_window ( display );
    }


    menu_calculate_window_and_element_width ( &state, &mon );
    state.max_elements = 0;

    state.text = textbox_create ( main_window, TB_AUTOHEIGHT,
                                  ( config.padding ),
                                  ( config.padding ),
                                  ( state.w - ( 2 * ( config.padding ) ) ),
                                  1,
                                  NORMAL,
                                  ( msg != NULL ) ? msg : "" );
    textbox_show ( state.text );
    int line_height = textbox_get_height ( state.text );

    // resize window vertically to suit
    state.h = line_height + ( config.padding ) * 2;

    // Move the window to the correct x,y position.
    calculate_window_position ( &state, &mon );
    XMoveResizeWindow ( display, main_window, state.x, state.y, state.w, state.h );

    // Display it.
    XMapRaised ( display, main_window );

    if ( take_keyboard ( main_window ) ) {
        while ( !state.quit ) {
            // Update if requested.
            if ( state.update ) {
                textbox_draw ( state.text );
                state.update = FALSE;
            }
            // Wait for event.
            XEvent ev;
            XNextEvent ( display, &ev );


            // Handle event.
            if ( ev.type == Expose ) {
                while ( XCheckTypedEvent ( display, Expose, &ev ) ) {
                    ;
                }
                state.update = TRUE;
            }
            // Key press event.
            else if ( ev.type == KeyPress ) {
                while ( XCheckTypedEvent ( display, KeyPress, &ev ) ) {
                    ;
                }
                state.quit = TRUE;
            }
        }
        release_keyboard ();
    }
}

SwitcherMode run_switcher_window ( char **input, G_GNUC_UNUSED void *data )
{
    Screen       *screen = DefaultScreenOfDisplay ( display );
    Window       root    = RootWindow ( display, XScreenNumberOfScreen ( screen ) );
    SwitcherMode retv    = MODE_EXIT;
    // find window list
    Atom         type;
    int          nwins;
    Window       wins[100];
    int          count       = 0;
    Window       curr_win_id = 0;

    // Get the active window so we can highlight this.
    if ( !( window_get_prop ( root, netatoms[_NET_ACTIVE_WINDOW], &type,
                              &count, &curr_win_id, sizeof ( Window ) )
            && type == XA_WINDOW && count > 0 ) ) {
        curr_win_id = 0;
    }

    if ( window_get_prop ( root, netatoms[_NET_CLIENT_LIST_STACKING],
                           &type, &nwins, wins, 100 * sizeof ( Window ) )
         && type == XA_WINDOW ) {
        char          pattern[50];
        int           i;
        unsigned int  classfield = 0;
        unsigned long desktops   = 0;
        // windows we actually display. may be slightly different to _NET_CLIENT_LIST_STACKING
        // if we happen to have a window destroyed while we're working...
        winlist *ids = winlist_new ();



        // calc widths of fields
        for ( i = nwins - 1; i > -1; i-- ) {
            client *c;

            if ( ( c = window_client ( wins[i] ) )
                 && !c->xattr.override_redirect
                 && !client_has_state ( c, netatoms[_NET_WM_STATE_SKIP_PAGER] )
                 && !client_has_state ( c, netatoms[_NET_WM_STATE_SKIP_TASKBAR] ) ) {
                classfield = MAX ( classfield, strlen ( c->class ) );

#ifdef HAVE_I3_IPC_H

                // In i3 mode, skip the i3bar completely.
                if ( config_i3_mode && strstr ( c->class, "i3bar" ) != NULL ) {
                    continue;
                }

#endif
                if ( c->window == curr_win_id ) {
                    c->active = TRUE;
                }
                winlist_append ( ids, c->window, NULL );
            }
        }

        // Create pattern for printing the line.
        if ( !window_get_cardinal_prop ( root, netatoms[_NET_NUMBER_OF_DESKTOPS], &desktops, 1 ) ) {
            desktops = 1;
        }
#ifdef HAVE_I3_IPC_H
        if ( config_i3_mode ) {
            sprintf ( pattern, "%%-%ds   %%s", MAX ( 5, classfield ) );
        }
        else{
#endif
        sprintf ( pattern, "%%-%ds  %%-%ds   %%s", desktops < 10 ? 1 : 2, MAX ( 5, classfield ) );
#ifdef HAVE_I3_IPC_H
    }
#endif
        char **list        = g_malloc0_n ( ( ids->len + 1 ), sizeof ( char* ) );
        unsigned int lines = 0;

        // build the actual list
        for ( i = 0; i < ( ids->len ); i++ ) {
            Window w = ids->array[i];
            client *c;

            if ( ( c = window_client ( w ) ) ) {
                // final line format
                unsigned long wmdesktop;
                char          desktop[5];
                desktop[0] = 0;
                char          *line = g_malloc ( strlen ( c->title ) + strlen ( c->class ) + classfield + 50 );
#ifdef HAVE_I3_IPC_H
                if ( !config_i3_mode ) {
#endif
                // find client's desktop. this is zero-based, so we adjust by since most
                // normal people don't think like this :-)
                if ( !window_get_cardinal_prop ( c->window, netatoms[_NET_WM_DESKTOP], &wmdesktop, 1 ) ) {
                    wmdesktop = 0xFFFFFFFF;
                }

                if ( wmdesktop < 0xFFFFFFFF ) {
                    sprintf ( desktop, "%d", (int) wmdesktop + 1 );
                }

                sprintf ( line, pattern, desktop, c->class, c->title );
#ifdef HAVE_I3_IPC_H
            }
            else{
                sprintf ( line, pattern, c->class, c->title );
            }
#endif

                list[lines++] = line;
            }
        }
        Time time;
        int selected_line = 0;
        MenuReturn mretv  = menu ( list, lines, input, "window:", &time, NULL,
                                   window_match, ids, &selected_line, config.levenshtein_sort );

        if ( mretv == MENU_NEXT ) {
            retv = NEXT_DIALOG;
        }
        else if ( mretv == MENU_PREVIOUS ) {
            retv = PREVIOUS_DIALOG;
        }
        else if ( mretv == MENU_QUICK_SWITCH ) {
            retv = selected_line;
        }
        else if ( ( mretv == MENU_OK || mretv == MENU_CUSTOM_INPUT ) && list[selected_line] ) {
#ifdef HAVE_I3_IPC_H

            if ( config_i3_mode ) {
                // Hack for i3.
                focus_window_i3 ( i3_socket_path, ids->array[selected_line] );
            }
            else
#endif
            {
                // Change to the desktop of the selected window/client.
                // TODO: get rid of strtol
                window_send_message ( root, root, netatoms[_NET_CURRENT_DESKTOP], strtol ( list[selected_line], NULL, 10 ) - 1,
                                      SubstructureNotifyMask | SubstructureRedirectMask, time );
                XSync ( display, False );

                window_send_message ( root, ids->array[selected_line], netatoms[_NET_ACTIVE_WINDOW], 2, // 2 = pager
                                      SubstructureNotifyMask | SubstructureRedirectMask, time );
            }
        }

        g_strfreev ( list );
        winlist_free ( ids );
    }

    return retv;
}

/**
 * Start dmenu mode.
 */
static int run_dmenu ()
{
    int ret_state;
    textbox_setup (
        config.menu_bg, config.menu_bg_alt, config.menu_fg,
        config.menu_hlbg,
        config.menu_hlfg );
    char *input = NULL;

    // Dmenu modi has a return state.
    ret_state = dmenu_switcher_dialog ( &input );

    g_free ( input );

    // Cleanup font setup.
    textbox_cleanup ();
    return ret_state;
}

static void run_switcher ( int do_fork, SwitcherMode mode )
{
    // we fork because it's technically possible to have multiple window
    // lists up at once on a zaphod multihead X setup.
    // this also happens to isolate the Xft font stuff in a child process
    // that gets cleaned up every time. that library shows some valgrind
    // strangeness...
    if ( do_fork == TRUE ) {
        if ( fork () ) {
            return;
        }

        display = XOpenDisplay ( display_str );
        XSync ( display, True );
    }
    // Because of the above fork, we want to do this here.
    // Make sure this is isolated to its own thread.
    textbox_setup (
        config.menu_bg, config.menu_bg_alt, config.menu_fg,
        config.menu_hlbg,
        config.menu_hlfg );
    char *input = NULL;
    // Otherwise check if requested mode is enabled.
    if ( switchers[mode].cb != NULL ) {
        do {
            SwitcherMode retv;

            curr_switcher = mode;
            retv          = switchers[mode].cb ( &input, switchers[mode].cb_data );
            // Find next enabled
            if ( retv == NEXT_DIALOG ) {
                mode = ( mode + 1 ) % num_switchers;
            }
            else if ( retv == PREVIOUS_DIALOG ) {
                mode = ( mode - 1 ) % num_switchers;
                if ( mode < 0 ) {
                    mode = num_switchers - 1;
                }
            }
            else if ( retv == RELOAD_DIALOG ) {
                // do nothing.
            }
            else if ( retv < MODE_EXIT ) {
                mode = ( retv ) % num_switchers;
            }
            else {
                mode = retv;
            }
        } while ( mode != MODE_EXIT );
    }
    g_free ( input );

    // Cleanup font setup.
    textbox_cleanup ();

    if ( do_fork == TRUE ) {
        exit ( EXIT_SUCCESS );
    }
}

/**
 * Function that listens for global key-presses.
 * This is only used when in daemon mode.
 */
static void handle_keypress ( XEvent *ev )
{
    KeySym key = XkbKeycodeToKeysym ( display, ev->xkey.keycode, 0, 0 );

    if ( ( windows_modmask == AnyModifier || ev->xkey.state & windows_modmask ) &&
         key == windows_keysym ) {
        int index = switcher_get ( "window" );
        if ( index >= 0 ) {
            run_switcher ( TRUE, index );
        }
    }

    if ( ( rundialog_modmask == AnyModifier || ev->xkey.state & rundialog_modmask ) &&
         key == rundialog_keysym ) {
        int index = switcher_get ( "run" );
        if ( index >= 0 ) {
            run_switcher ( TRUE, index );
        }
    }

    if ( ( sshdialog_modmask == AnyModifier || ev->xkey.state & sshdialog_modmask ) &&
         key == sshdialog_keysym ) {
        int index = switcher_get ( "ssh" );
        if ( index >= 0 ) {
            run_switcher ( TRUE, index );
        }
    }
}

// convert a Mod+key arg to mod mask and keysym
static void parse_key ( char *combo, unsigned int *mod, KeySym *key )
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

    // Skip modifier (if exist( and parse key.
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

// bind a key combination on a root window, compensating for Lock* states
static void grab_key ( Display *display, unsigned int modmask, KeySym key )
{
    Screen *screen  = DefaultScreenOfDisplay ( display );
    Window root     = RootWindow ( display, XScreenNumberOfScreen ( screen ) );
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


#ifdef HAVE_I3_IPC_H
/**
 * Get the i3 socket from the X root window.
 */
static inline void display_get_i3_path ( Display *display )
{
    // Get the default screen.
    Screen *screen = DefaultScreenOfDisplay ( display );
    // Find the root window (each X has one.).
    Window root = RootWindow ( display, XScreenNumberOfScreen ( screen ) );
    // Get the i3 path property.
    i3_socket_path = window_get_text_prop ( root, netatoms[I3_SOCKET_PATH] );
    // If we find it, go into i3 mode.
    config_i3_mode = ( i3_socket_path != NULL ) ? TRUE : FALSE;
}
#endif //HAVE_I3_IPC_H


/**
 * Help function. This calls man.
 */
static void help ()
{
    int code = execlp ( "man", "man", MANPAGE_PATH, NULL );

    if ( code == -1 ) {
        fprintf ( stderr, "Failed to execute man: %s\n", strerror ( errno ) );
    }
}

/**
 * Parse commandline options.
 */
static void parse_cmd_options ( int argc, char ** argv )
{
    // catch help request
    if ( find_arg ( argc, argv, "-h" ) >= 0 ||
         find_arg ( argc, argv, "-help" ) >= 0 ) {
        help ();
        exit ( EXIT_SUCCESS );
    }

    if ( find_arg ( argc, argv, "-v" ) >= 0 ||
         find_arg ( argc, argv, "-version" ) >= 0 ) {
        fprintf ( stdout, "Version: "VERSION "\n" );
        exit ( EXIT_SUCCESS );
    }

    find_arg_str ( argc, argv, "-switchers", &( config.switchers ) );
    // Parse commandline arguments about the looks.
    find_arg_uint ( argc, argv, "-opacity", &( config.window_opacity ) );

    find_arg_int ( argc, argv, "-width", &( config.menu_width ) );

    find_arg_uint ( argc, argv, "-lines", &( config.menu_lines ) );
    find_arg_uint ( argc, argv, "-columns", &( config.menu_columns ) );

    find_arg_str ( argc, argv, "-font", &( config.menu_font ) );
    find_arg_str ( argc, argv, "-fg", &( config.menu_fg ) );
    find_arg_str ( argc, argv, "-bg", &( config.menu_bg ) );
    find_arg_str ( argc, argv, "-bgalt", &( config.menu_bg_alt ) );
    find_arg_str ( argc, argv, "-hlfg", &( config.menu_hlfg ) );
    find_arg_str ( argc, argv, "-hlbg", &( config.menu_hlbg ) );
    find_arg_str ( argc, argv, "-bc", &( config.menu_bc ) );
    find_arg_uint ( argc, argv, "-bw", &( config.menu_bw ) );

    // Parse commandline arguments about size and position
    find_arg_uint ( argc, argv, "-location", &( config.location ) );
    find_arg_uint ( argc, argv, "-padding", &( config.padding ) );
    find_arg_int ( argc, argv, "-xoffset", &( config.x_offset ) );
    find_arg_int ( argc, argv, "-yoffset", &( config.y_offset ) );
    if ( find_arg ( argc, argv, "-fixed-num-lines" ) >= 0 ) {
        config.fixed_num_lines = 1;
    }
    if ( find_arg ( argc, argv, "-disable-history" ) >= 0 ) {
        config.disable_history = TRUE;
    }
    if ( find_arg ( argc, argv, "-levenshtein-sort" ) >= 0 ) {
        config.levenshtein_sort = TRUE;
    }
    if ( find_arg ( argc, argv, "-case-sensitive" ) >= 0 ) {
        config.case_sensitive = TRUE;
    }

    // Parse commandline arguments about behavior
    find_arg_str ( argc, argv, "-terminal", &( config.terminal_emulator ) );

    if ( find_arg ( argc, argv, "-hmode" ) >= 0 ) {
        config.hmode = TRUE;
    }

    find_arg_str ( argc, argv, "-ssh-client", &( config.ssh_client ) );
    find_arg_str ( argc, argv, "-ssh-command", &( config.ssh_command ) );
    find_arg_str ( argc, argv, "-run-command", &( config.run_command ) );
    find_arg_str ( argc, argv, "-run-list-command", &( config.run_list_command ) );
    find_arg_str ( argc, argv, "-run-shell-command", &( config.run_shell_command ) );

    // Keybindings
    find_arg_str ( argc, argv, "-key", &( config.window_key ) );
    find_arg_str ( argc, argv, "-rkey", &( config.run_key ) );
    find_arg_str ( argc, argv, "-skey", &( config.ssh_key ) );



    find_arg_char ( argc, argv, "-sep", &( config.separator ) );


    find_arg_int ( argc, argv, "-eh", &( config.element_height ) );

    if ( find_arg ( argc, argv, "-sidebar-mode" ) >= 0 ) {
        config.sidebar_mode = TRUE;
    }
}

/**
 * Function bound by 'atexit'.
 * Cleanup globally allocated memory.
 */
static void cleanup ()
{
    // Cleanup
    if ( display != NULL ) {
        if ( main_window != None ) {
            XFreeGC ( display, gc );
            XDestroyWindow ( display, main_window );
            XCloseDisplay ( display );
        }
    }
    if ( cache_xattr != NULL ) {
        winlist_free ( cache_xattr );
    }
    if ( cache_client != NULL ) {
        winlist_free ( cache_client );
    }
#ifdef HAVE_I3_IPC_H

    if ( i3_socket_path != NULL ) {
        g_free ( i3_socket_path );
    }

#endif

    // Cleaning up memory allocated by the Xresources file.
    // TODO, not happy with this.
    parse_xresource_free ();

    for ( unsigned int i = 0; i < num_switchers; i++ ) {
        // only used for script dialog.
        if ( switchers[i].cb_data != NULL ) {
            script_switcher_free_options ( switchers[i].cb_data );
        }
    }
    g_free ( switchers );
}

/**
 * Do some input validation, especially the first few could break things.
 * It is good to catch them beforehand.
 *
 * This functions exits the program with 1 when it finds an invalid configuration.
 */
static void config_sanity_check ( void )
{
    if ( config.element_height < 1 ) {
        fprintf ( stderr, "config.element_height is invalid. It needs to be atleast 1 line high.\n" );
        exit ( 1 );
    }
    if ( config.menu_columns == 0 ) {
        fprintf ( stderr, "config.menu_columns is invalid. You need at least one visible column.\n" );
        exit ( 1 );
    }
    if ( config.menu_width == 0 ) {
        fprintf ( stderr, "config.menu_width is invalid. You cannot have a window with no width.\n" );
        exit ( 1 );
    }
    if ( !( config.location >= WL_CENTER && config.location <= WL_WEST ) ) {
        fprintf ( stderr, "config.location is invalid. ( %d >= %d >= %d) does not hold.\n",
                  WL_WEST, config.location, WL_CENTER );
        exit ( 1 );
    }
    if ( !( config.hmode == TRUE || config.hmode == FALSE ) ) {
        fprintf ( stderr, "config.hmode is invalid.\n" );
        exit ( 1 );
    }
    // If alternative row is not set, copy the normal background color.
    if ( config.menu_bg_alt == NULL ) {
        config.menu_bg_alt = config.menu_bg;
    }
}

/**
 * Parse the switcher string, into internal array of type Switcher.
 *
 * String is split on separator ','
 * First the three build-in modi are checked: window, run, ssh
 * if that fails, a script-switcher is created.
 */
static void setup_switchers ( void )
{
    char *savept = NULL;
    // Make a copy, as strtok will modify it.
    char *switcher_str = g_strdup ( config.switchers );
    // Split token on ','. This modifies switcher_str.
    for ( char *token = strtok_r ( switcher_str, ",", &savept );
          token != NULL;
          token = strtok_r ( NULL, ",", &savept ) ) {
        // Window switcher.
        if ( strcasecmp ( token, "window" ) == 0 ) {
            // Resize and add entry.
            switchers = (Switcher *) g_realloc ( switchers, sizeof ( Switcher ) * ( num_switchers + 1 ) );
            g_strlcpy ( switchers[num_switchers].name, "window", 32 );
            switchers[num_switchers].cb      = run_switcher_window;
            switchers[num_switchers].cb_data = NULL;
            num_switchers++;
        }
        // SSh dialog
        else if ( strcasecmp ( token, "ssh" ) == 0 ) {
            // Resize and add entry.
            switchers = (Switcher *) g_realloc ( switchers, sizeof ( Switcher ) * ( num_switchers + 1 ) );
            g_strlcpy ( switchers[num_switchers].name, "ssh", 32 );
            switchers[num_switchers].cb      = ssh_switcher_dialog;
            switchers[num_switchers].cb_data = NULL;
            num_switchers++;
        }
        // Run dialog
        else if ( strcasecmp ( token, "run" ) == 0 ) {
            // Resize and add entry.
            switchers = (Switcher *) g_realloc ( switchers, sizeof ( Switcher ) * ( num_switchers + 1 ) );
            g_strlcpy ( switchers[num_switchers].name, "run", 32 );
            switchers[num_switchers].cb      = run_switcher_dialog;
            switchers[num_switchers].cb_data = NULL;
            num_switchers++;
        }
        else {
            // If not build in, use custom switchers.
            ScriptOptions *sw = script_switcher_parse_setup ( token );
            if ( sw != NULL ) {
                // Resize and add entry.
                switchers = (Switcher *) g_realloc ( switchers, sizeof ( Switcher ) * ( num_switchers + 1 ) );
                g_strlcpy ( switchers[num_switchers].name, sw->name, 32 );
                switchers[num_switchers].cb      = script_switcher_dialog;
                switchers[num_switchers].cb_data = sw;
                num_switchers++;
            }
            else{
                // Report error, don't continue.
                fprintf ( stderr, "Invalid script switcher: %s\n", token );
                token = NULL;
            }
        }
    }
    // Free string that was modified by strtok_r
    g_free ( switcher_str );
}

/**
 * Keep a copy of arc, argv around, so we can use the same parsing method
 */
static int stored_argc;
static char **stored_argv;

/**
 * @param display Pointer to the X connection to use.
 * Load configuration.
 * Following priority: (current), X, commandline arguments
 */
static inline void load_configuration ( Display *display )
{
    // Load in config from X resources.
    parse_xresource_options ( display );

    // Parse command line for settings.
    parse_cmd_options ( stored_argc, stored_argv );

    // Sanity check
    config_sanity_check ();
}

/**
 * Handle sighub request.
 * Currently we just reload the configuration.
 */
static void hup_action_handler ( int num )
{
    /**
     * Open new connection to X. It seems the XResources do not get updated
     * on the old connection.
     */
    Display *display = XOpenDisplay ( display_str );
    if ( display ) {
        load_configuration ( display );
        XCloseDisplay ( display );
    }
}


int main ( int argc, char *argv[] )
{
    stored_argc = argc;
    stored_argv = argv;

    // Get the path to the cache dir.
    cache_dir = g_get_user_cache_dir ();

    // Register cleanup function.
    atexit ( cleanup );

    // Get DISPLAY
    display_str = getenv ( "DISPLAY" );
    find_arg_str ( argc, argv, "-display", &display_str );

    if ( !( display = XOpenDisplay ( display_str ) ) ) {
        fprintf ( stderr, "cannot open display!\n" );
        return EXIT_FAILURE;
    }

    load_configuration ( display );

    // Dump.
    if ( find_arg ( argc, argv, "-dump-xresources" ) >= 0 ) {
        xresource_dump ();
        exit ( EXIT_SUCCESS );
    }

    // setup_switchers
    setup_switchers ();

    // Set up X interaction.
    signal ( SIGCHLD, catch_exit );

    // Set error handle
    XSync ( display, False );
    xerror = XSetErrorHandler ( display_oops );
    XSync ( display, False );

    // determine numlock mask so we can bind on keys with and without it
    XModifierKeymap *modmap = XGetModifierMapping ( display );
    KeyCode kc              = XKeysymToKeycode ( display, XK_Num_Lock );
    for ( int i = 0; i < 8; i++ ) {
        for ( int j = 0; j < ( int ) modmap->max_keypermod; j++ ) {
            if ( modmap->modifiermap[i * modmap->max_keypermod + j] == kc ) {
                NumlockMask = ( 1 << i );
            }
        }
    }
    XFreeModifiermap ( modmap );

    cache_client = winlist_new ();
    cache_xattr  = winlist_new ();

    // X atom values
    for ( int i = 0; i < NUM_NETATOMS; i++ ) {
        netatoms[i] = XInternAtom ( display, netatom_names[i], False );
    }

#ifdef HAVE_I3_IPC_H
    // Check for i3
    display_get_i3_path ( display );
#endif

    char *msg = NULL;
    if ( find_arg_str ( argc, argv, "-e", &( msg ) ) ) {
        textbox_setup (
            config.menu_bg, config.menu_bg_alt, config.menu_fg,
            config.menu_hlbg,
            config.menu_hlfg );
        error_dialog ( msg );
        textbox_cleanup ();
        exit ( EXIT_SUCCESS );
    }


    // flags to run immediately and exit
    char *sname = NULL;
    if ( find_arg ( argc, argv, "-dmenu" ) >= 0 || strcmp ( argv[0], "dmenu" ) == 0 ) {
        // force off sidebar mode:
        config.sidebar_mode = FALSE;
        // Check prompt
        find_arg_str ( argc, argv, "-p", &dmenu_prompt );
        find_arg_int ( argc, argv, "-l", &dmenu_selected_line );
        int retv = run_dmenu ();

        // User canceled the operation.
        if ( retv == FALSE ) {
            return EXIT_FAILURE;
        }
    }
    else if ( find_arg_str ( argc, argv, "-show", &sname ) == TRUE ) {
        int index = switcher_get ( sname );
        if ( index >= 0 ) {
            run_switcher ( FALSE, index );
        }
        else {
            fprintf ( stderr, "The %s switcher has not been enabled\n", sname );
        }
    }
    // Old modi.
    else if ( find_arg ( argc, argv, "-now" ) >= 0 ) {
        int index = switcher_get ( "window" );
        if ( index >= 0 ) {
            run_switcher ( FALSE, index );
        }
        else {
            fprintf ( stderr, "The window switcher has not been enabled\n" );
        }
    }
    else if ( find_arg ( argc, argv, "-rnow" ) >= 0 ) {
        int index = switcher_get ( "run" );
        if ( index >= 0 ) {
            run_switcher ( FALSE, index );
        }
        else {
            fprintf ( stderr, "The run dialog has not been enabled\n" );
        }
    }
    else if ( find_arg ( argc, argv, "-snow" ) >= 0 ) {
        int index = switcher_get ( "ssh" );
        if ( index >= 0 ) {
            run_switcher ( FALSE, index );
        }
        else {
            fprintf ( stderr, "The ssh dialog has not been enabled\n" );
        }
    }
    else{
        // Daemon mode, Listen to key presses..
        if ( switcher_get ( "window" ) >= 0 ) {
            parse_key ( config.window_key, &windows_modmask, &windows_keysym );
            grab_key ( display, windows_modmask, windows_keysym );
        }

        if ( switcher_get ( "run" ) >= 0 ) {
            parse_key ( config.run_key, &rundialog_modmask, &rundialog_keysym );
            grab_key ( display, rundialog_modmask, rundialog_keysym );
        }

        if ( switcher_get ( "ssh" ) >= 0 ) {
            parse_key ( config.ssh_key, &sshdialog_modmask, &sshdialog_keysym );
            grab_key ( display, sshdialog_modmask, sshdialog_keysym );
        }

        // Setup handler for sighub (reload config)
        const struct sigaction hup_action = { hup_action_handler, };
        sigaction ( SIGHUP, &hup_action, NULL );


        // Application Main loop.
        // This listens in the background for any events on the Xserver
        // catching global key presses.
        for (;; ) {
            XEvent ev;
            // caches only live for a single event
            winlist_empty ( cache_xattr );
            winlist_empty ( cache_client );

            // block and wait for something
            XNextEvent ( display, &ev );
            // If we get an event that does not belong to a window:
            // Ignore it.
            if ( ev.xany.window == None ) {
                continue;
            }
            // If keypress, handle it.
            if ( ev.type == KeyPress ) {
                handle_keypress ( &ev );
            }
        }
    }

    return EXIT_SUCCESS;
}
