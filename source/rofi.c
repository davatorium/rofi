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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
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
#include <X11/Xft/Xft.h>
#include <X11/Xresource.h>
#include <X11/extensions/Xinerama.h>

#include "rofi.h"

#ifdef HAVE_I3_IPC_H
#include <errno.h>
#include <linux/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <i3/ipc.h>
#endif

#include <basedir.h>

#include "run-dialog.h"
#include "ssh-dialog.h"
#include "dmenu-dialog.h"

#include "xrmoptions.h"


#define LINE_MARGIN            4

#define OPAQUE                 0xffffffff
#define OPACITY                "_NET_WM_WINDOW_OPACITY"

#ifdef HAVE_I3_IPC_H
#define I3_SOCKET_PATH_PROP    "I3_SOCKET_PATH"
// This setting is no longer user configurable, but partial to this file:
int  config_i3_mode = 0;
// Path to HAVE_I3_IPC_H socket.
char *i3_socket_path = NULL;
#endif


xdgHandle  xdg_handle;
const char *cache_dir = NULL;

char       *active_font = NULL;

/**
 * Shared 'token_match' function.
 * Matches tokenized.
 */
int token_match ( char **tokens, const char *input,
                  __attribute__( ( unused ) ) int index,
                  __attribute__( ( unused ) ) void *data )
{
    int match = 1;

    // Do a tokenized match.
    if ( tokens )
    {
        for ( int j = 1; match && tokens[j]; j++ )
        {
            match = ( strcasestr ( input, tokens[j] ) != NULL );
        }
    }

    return match;
}


static char **tokenize ( const char *input )
{
    if ( input == NULL )
    {
        return NULL;
    }

    char *saveptr = NULL, *token;
    char **retv   = NULL;
    // First entry is always full (modified) stringtext.
    int  num_tokens = 1;

    //First entry is string that is modified.
    retv    = malloc ( 2 * sizeof ( char* ) );
    retv[0] = strdup ( input );
    retv[1] = NULL;

    // Iterate over tokens.
    // strtok should still be valid for utf8.
    for (
        token = strtok_r ( retv[0], " ", &saveptr );
        token != NULL;
        token = strtok_r ( NULL, " ", &saveptr ) )
    {
        retv                 = realloc ( retv, sizeof ( char* ) * ( num_tokens + 2 ) );
        retv[num_tokens + 1] = NULL;
        retv[num_tokens]     = token;
        num_tokens++;
    }

    return retv;
}

static inline void tokenize_free ( char **ip )
{
    if ( ip == NULL )
    {
        return;
    }

    free ( ip[0] );
    free ( ip );
}

#ifdef HAVE_I3_IPC_H
// Focus window on HAVE_I3_IPC_H window manager.
static void focus_window_i3 ( const char *socket_path, int id )
{
    i3_ipc_header_t    head;
    char               command[128];
    int                s, t, len;
    struct sockaddr_un remote;

    if ( strlen ( socket_path ) > UNIX_PATH_MAX )
    {
        fprintf ( stderr, "Socket path is to long. %zd > %d\n", strlen ( socket_path ), UNIX_PATH_MAX );
        return;
    }

    if ( ( s = socket ( AF_UNIX, SOCK_STREAM, 0 ) ) == -1 )
    {
        fprintf ( stderr, "Failed to open connection to I3: %s\n", strerror ( errno ) );
        return;
    }

    remote.sun_family = AF_UNIX;
    strcpy ( remote.sun_path, socket_path );
    len = strlen ( remote.sun_path ) + sizeof ( remote.sun_family );

    if ( connect ( s, ( struct sockaddr * ) &remote, len ) == -1 )
    {
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
    send ( s, &head, sizeof ( head ), 0 );
    // Send message
    send ( s, command, strlen ( command ), 0 );
    // Receive header.
    t = recv ( s, &head, sizeof ( head ), 0 );

    if ( t == sizeof ( head ) )
    {
        recv ( s, command, head.size, 0 );
    }

    close ( s );
}
#endif

void catch_exit ( __attribute__( ( unused ) ) int sig )
{
    while ( 0 < waitpid ( -1, NULL, WNOHANG ) )
    {
        ;
    }
}


// cli arg handling
static int find_arg ( const int argc, char * const argv[], const char * const key )
{
    int i;

    for ( i = 0; i < argc && strcasecmp ( argv[i], key ); i++ )
    {
        ;
    }

    return i < argc ? i : -1;
}
static void find_arg_str ( int argc, char *argv[], char *key, char** val )
{
    int i = find_arg ( argc, argv, key );

    if ( val != NULL && i > 0 && i < argc - 1 )
    {
        *val = argv[i + 1];
    }
}

static void find_arg_int ( int argc, char *argv[], char *key, unsigned int *val )
{
    int i = find_arg ( argc, argv, key );

    if ( val != NULL && i > 0 && i < ( argc - 1 ) )
    {
        *val = strtol ( argv[i + 1], NULL, 10 );
    }
}

unsigned int NumlockMask = 0;
Display      *display    = NULL;
Screen       *screen;
Window       root;
int          screen_id;

static int   ( *xerror )( Display *, XErrorEvent * );

#define ATOM_ENUM( x )    x
#define ATOM_CHAR( x )    # x

#define EWMH_ATOMS( X )                    \
    X ( _NET_SUPPORTING_WM_CHECK ),        \
    X ( _NET_CLIENT_LIST ),                \
    X ( _NET_CLIENT_LIST_STACKING ),       \
    X ( _NET_NUMBER_OF_DESKTOPS ),         \
    X ( _NET_CURRENT_DESKTOP ),            \
    X ( _NET_DESKTOP_GEOMETRY ),           \
    X ( _NET_DESKTOP_VIEWPORT ),           \
    X ( _NET_WORKAREA ),                   \
    X ( _NET_ACTIVE_WINDOW ),              \
    X ( _NET_CLOSE_WINDOW ),               \
    X ( _NET_MOVERESIZE_WINDOW ),          \
    X ( _NET_WM_NAME ),                    \
    X ( _NET_WM_STATE ),                   \
    X ( _NET_WM_STATE_SKIP_TASKBAR ),      \
    X ( _NET_WM_STATE_SKIP_PAGER ),        \
    X ( _NET_WM_STATE_FULLSCREEN ),        \
    X ( _NET_WM_STATE_ABOVE ),             \
    X ( _NET_WM_STATE_BELOW ),             \
    X ( _NET_WM_STATE_DEMANDS_ATTENTION ), \
    X ( _NET_WM_DESKTOP ),                 \
    X ( _NET_SUPPORTED )

enum { EWMH_ATOMS ( ATOM_ENUM ), NETATOMS };
const char *netatom_names[] = { EWMH_ATOMS ( ATOM_CHAR ) };
Atom       netatoms[NETATOMS];

// X error handler
static int display_oops ( __attribute__( ( unused ) ) Display *d, XErrorEvent *ee )
{
    if ( ee->error_code == BadWindow
         || ( ee->request_code == X_GrabButton && ee->error_code == BadAccess )
         || ( ee->request_code == X_GrabKey && ee->error_code == BadAccess )
         )
    {
        return 0;
    }

    fprintf ( stderr, "error: request code=%d, error code=%d\n", ee->request_code, ee->error_code );
    return xerror ( display, ee );
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

#define winlist_ascend( l, i, w )     for ( ( i ) = 0; ( i ) < ( l )->len && ( ( ( w ) = ( l )->array[i] ) || 1 ); ( i )++ )
#define winlist_descend( l, i, w )    for ( ( i ) = ( l )->len - 1; ( i ) >= 0 && ( ( ( w ) = ( l )->array[i] ) || 1 ); ( i )-- )

#define WINLIST    32

winlist* winlist_new ()
{
    winlist *l = malloc ( sizeof ( winlist ) );
    l->len   = 0;
    l->array = malloc ( sizeof ( Window ) * ( WINLIST + 1 ) );
    l->data  = malloc ( sizeof ( void* ) * ( WINLIST + 1 ) );
    return l;
}
int winlist_append ( winlist *l, Window w, void *d )
{
    if ( l->len > 0 && !( l->len % WINLIST ) )
    {
        l->array = realloc ( l->array, sizeof ( Window ) * ( l->len + WINLIST + 1 ) );
        l->data  = realloc ( l->data, sizeof ( void* ) * ( l->len + WINLIST + 1 ) );
    }
    // Make clang-check happy.
    // TODO: make clang-check clear this should never be 0.
    if ( l->data == NULL || l->array == NULL )
    {
        return 0;
    }

    l->data[l->len]    = d;
    l->array[l->len++] = w;
    return l->len - 1;
}
void winlist_empty ( winlist *l )
{
    while ( l->len > 0 )
    {
        free ( l->data[--( l->len )] );
    }
}
void winlist_free ( winlist *l )
{
    winlist_empty ( l );
    free ( l->array );
    free ( l->data );
    free ( l );
}
int winlist_find ( winlist *l, Window w )
{
// iterate backwards. theory is: windows most often accessed will be
// nearer the end. testing with kcachegrind seems to support this...
    int    i;
    Window o = 0;

    winlist_descend ( l, i, o ) if ( w == o )
    {
        return i;
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



// malloc a pixel value for an X named color
static unsigned int color_get ( const char *const name )
{
    XColor   color;
    Colormap map = DefaultColormap ( display, screen_id );
    return XAllocNamedColor ( display, map, name, &color, &color ) ? color.pixel : None;
}

// find mouse pointer location
int pointer_get ( Window root, int *x, int *y )
{
    *x = 0;
    *y = 0;
    Window       rr, cr;
    int          rxr, ryr, wxr, wyr;
    unsigned int mr;

    if ( XQueryPointer ( display, root, &rr, &cr, &rxr, &ryr, &wxr, &wyr, &mr ) )
    {
        *x = rxr;
        *y = ryr;
        return 1;
    }

    return 0;
}

static int take_keyboard ( Window w )
{
    int i;

    for ( i = 0; i < 1000; i++ )
    {
        if ( XGrabKeyboard ( display, w, True, GrabModeAsync, GrabModeAsync, CurrentTime ) == GrabSuccess )
        {
            return 1;
        }

        struct timespec rsl = { 0, 50000L };
        nanosleep ( &rsl, NULL );
    }

    return 0;
}
void release_keyboard ()
{
    XUngrabKeyboard ( display, CurrentTime );
}

// XGetWindowAttributes with caching
XWindowAttributes* window_get_attributes ( Window w )
{
    int idx = winlist_find ( cache_xattr, w );

    if ( idx < 0 )
    {
        XWindowAttributes *cattr = malloc ( sizeof ( XWindowAttributes ) );

        if ( XGetWindowAttributes ( display, w, cattr ) )
        {
            winlist_append ( cache_xattr, w, cattr );
            return cattr;
        }

        free ( cattr );
        return NULL;
    }

    return cache_xattr->data[idx];
}

// retrieve a property of any type from a window
int window_get_prop ( Window w, Atom prop, Atom *type, int *items, void *buffer, unsigned int bytes )
{
    Atom _type;

    if ( !type )
    {
        type = &_type;
    }

    int _items;

    if ( !items )
    {
        items = &_items;
    }

    int           format;
    unsigned long nitems, nbytes;
    unsigned char *ret = NULL;
    memset ( buffer, 0, bytes );

    if ( XGetWindowProperty ( display, w, prop, 0, bytes / 4, False, AnyPropertyType, type,
                              &format, &nitems, &nbytes, &ret ) == Success && ret && *type != None && format )
    {
        if ( format == 8 )
        {
            memmove ( buffer, ret, MIN ( bytes, nitems ) );
        }

        if ( format == 16 )
        {
            memmove ( buffer, ret, MIN ( bytes, nitems * sizeof ( short ) ) );
        }

        if ( format == 32 )
        {
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
char* window_get_text_prop ( Window w, Atom atom )
{
    XTextProperty prop;
    char          *res   = NULL;
    char          **list = NULL;
    int           count;

    if ( XGetTextProperty ( display, w, &prop, atom ) && prop.value && prop.nitems )
    {
        if ( prop.encoding == XA_STRING )
        {
            res = malloc ( strlen ( ( char * ) prop.value ) + 1 );
            // make clang-check happy.
            if ( res )
            {
                strcpy ( res, ( char * ) prop.value );
            }
        }
        else if ( Xutf8TextPropertyToTextList ( display, &prop, &list, &count ) >= Success && count > 0 && *list )
        {
            res = malloc ( strlen ( *list ) + 1 );
            // make clang-check happy.
            if ( res )
            {
                strcpy ( res, *list );
            }
            XFreeStringList ( list );
        }
    }

    if ( prop.value )
    {
        XFree ( prop.value );
    }

    return res;
}

int window_get_atom_prop ( Window w, Atom atom, Atom *list, int count )
{
    Atom type;
    int  items;
    return window_get_prop ( w, atom, &type, &items, list, count * sizeof ( Atom ) ) && type == XA_ATOM ? items : 0;
}

void window_set_atom_prop ( Window w, Atom prop, Atom *atoms, int count )
{
    XChangeProperty ( display, w, prop, XA_ATOM, 32, PropModeReplace, ( unsigned char * ) atoms, count );
}

int window_get_cardinal_prop ( Window w, Atom atom, unsigned long *list, int count )
{
    Atom type; int items;
    return window_get_prop ( w, atom, &type, &items, list, count * sizeof ( unsigned long ) ) && type == XA_CARDINAL ? items : 0;
}

// a ClientMessage
int window_send_message ( Window target, Window subject, Atom atom, unsigned long protocol, unsigned long mask, Time time )
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
void monitor_dimensions ( Screen *screen, int x, int y, workarea *mon )
{
    memset ( mon, 0, sizeof ( workarea ) );
    mon->w = WidthOfScreen ( screen );
    mon->h = HeightOfScreen ( screen );

// locate the current monitor
    if ( XineramaIsActive ( display ) )
    {
        int                monitors;
        XineramaScreenInfo *info = XineramaQueryScreens ( display, &monitors );

        if ( info )
        {
            for ( int i = 0; i < monitors; i++ )
            {
                if ( INTERSECT ( x, y, 1, 1, info[i].x_org, info[i].y_org, info[i].width, info[i].height ) )
                {
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
void monitor_active ( workarea *mon )
{
    Window root = RootWindow ( display, XScreenNumberOfScreen ( screen ) );

#if 0
    // Comment this code out as it seems to break things.
    // This code was not working before in simpleswitcher to begin with.
    // After fixing the broken code, problems started.
    Window id;
    Atom   type;
    int    count;
    if ( window_get_prop ( root, netatoms[_NET_ACTIVE_WINDOW], &type, &count, &id, sizeof ( Window ) )
         && type == XA_WINDOW && count > 0 )
    {
        XWindowAttributes *attr = window_get_attributes ( id );
        // Window might not exists anymore.
        // This could very well be a bug in i3.
        if ( attr != NULL )
        {
            monitor_dimensions ( screen, attr->x, attr->y, mon );
            return;
        }
    }
#endif
    int x, y;

    if ( pointer_get ( root, &x, &y ) )
    {
        monitor_dimensions ( screen, x, y, mon );
        return;
    }

    monitor_dimensions ( screen, 0, 0, mon );
}

// _NET_WM_STATE_*
int client_has_state ( client *c, Atom state )
{
    int i;

    for ( i = 0; i < c->states; i++ )
    {
        if ( c->state[i] == state )
        {
            return 1;
        }
    }

    return 0;
}

// collect info on any window
// doesn't have to be a window we'll end up managing
client* window_client ( Window win )
{
    if ( win == None )
    {
        return NULL;
    }

    int idx = winlist_find ( cache_client, win );

    if ( idx >= 0 )
    {
        return cache_client->data[idx];
    }

    // if this fails, we're up that creek
    XWindowAttributes *attr = window_get_attributes ( win );

    if ( !attr )
    {
        return NULL;
    }

    client *c = calloc ( 1, sizeof ( client ) );
    c->window = win;

    // copy xattr so we don't have to care when stuff is freed
    memmove ( &c->xattr, attr, sizeof ( XWindowAttributes ) );
    XGetTransientForHint ( display, win, &c->trans );

    c->states = window_get_atom_prop ( win, netatoms[_NET_WM_STATE], c->state, CLIENTSTATE );

    char *name;

    if ( ( name = window_get_text_prop ( c->window, netatoms[_NET_WM_NAME] ) ) && name )
    {
        snprintf ( c->title, CLIENTTITLE, "%s", name );
        free ( name );
    }
    else if ( XFetchName ( display, c->window, &name ) )
    {
        snprintf ( c->title, CLIENTTITLE, "%s", name );
        XFree ( name );
    }

    name = window_get_text_prop ( c->window, XInternAtom ( display, "WM_WINDOW_ROLE", False ) );

    if ( name != NULL )
    {
        snprintf ( c->role, CLIENTROLE, "%s", name );
        XFree ( name );
    }

    XClassHint chint;

    if ( XGetClassHint ( display, c->window, &chint ) )
    {
        snprintf ( c->class, CLIENTCLASS, "%s", chint.res_class );
        snprintf ( c->name, CLIENTNAME, "%s", chint.res_name );
        XFree ( chint.res_class );
        XFree ( chint.res_name );
    }

    monitor_dimensions ( c->xattr.screen, c->xattr.x, c->xattr.y, &c->monitor );
    winlist_append ( cache_client, c->window, c );
    return c;
}

unsigned int windows_modmask;
KeySym       windows_keysym;
unsigned int rundialog_modmask;
KeySym       rundialog_keysym;
unsigned int sshdialog_modmask;
KeySym       sshdialog_keysym;

Window       main_window = None;
GC           gc          = NULL;

#include "textbox.h"

void menu_hide_arrow_text ( int filtered_lines, int selected, int max_elements,
                            textbox *arrowbox_top, textbox *arrowbox_bottom )
{
    if ( arrowbox_top == NULL || arrowbox_bottom == NULL )
    {
        return;
    }
    int page   = ( filtered_lines > 0 ) ? selected / max_elements : 0;
    int npages = ( filtered_lines > 0 ) ? ( ( filtered_lines + max_elements - 1 ) / max_elements ) : 1;
    if ( !( page != 0 && npages > 1 ) )
    {
        textbox_hide ( arrowbox_top );
    }
    if ( !( ( npages - 1 ) != page && npages > 1 ) )
    {
        textbox_hide ( arrowbox_bottom );
    }
}

void menu_set_arrow_text ( int filtered_lines, int selected, int max_elements,
                           textbox *arrowbox_top, textbox *arrowbox_bottom )
{
    if ( arrowbox_top == NULL || arrowbox_bottom == NULL )
    {
        return;
    }
    int page   = ( filtered_lines > 0 ) ? selected / max_elements : 0;
    int npages = ( filtered_lines > 0 ) ? ( ( filtered_lines + max_elements - 1 ) / max_elements ) : 1;
    int entry  = selected % max_elements;
    if ( page != 0 && npages > 1 )
    {
        textbox_show ( arrowbox_top );
        textbox_font ( arrowbox_top, ( entry != 0 ) ? NORMAL : HIGHLIGHT );
        textbox_draw ( arrowbox_top  );
    }
    if ( ( npages - 1 ) != page && npages > 1 )
    {
        textbox_show ( arrowbox_bottom );
        textbox_font ( arrowbox_bottom, ( entry != ( max_elements - 1 ) ) ? NORMAL : HIGHLIGHT );
        textbox_draw ( arrowbox_bottom  );
    }
}

void menu_draw ( textbox **boxes,
                 int max_elements,
                 int num_lines,
                 int *last_offset,
                 int selected,
                 char **filtered )
{
    int i, offset = 0;

    // selected row is always visible.
    // If selected is visible do not scroll.
    if ( ( selected - ( *last_offset ) ) < ( max_elements ) && ( selected - ( *last_offset ) ) >= 0 )
    {
        offset = *last_offset;
    }
    else
    {
        // Do paginating
        int page = ( max_elements > 0 ) ? ( selected / max_elements ) : 0;
        offset       = page * max_elements;
        *last_offset = offset;
    }

    for ( i = 0; i < max_elements; i++ )
    {
        if ( ( i + offset ) >= num_lines || filtered[i + offset] == NULL )
        {
            textbox_font ( boxes[i], NORMAL );
            textbox_text ( boxes[i], "" );
        }
        else
        {
            char            *text = filtered[i + offset];
            TextBoxFontType tbft  = ( i + offset ) == selected ? HIGHLIGHT : NORMAL;
            char            *font = config.menu_font;
            // Check for active
            if ( text[0] == '*' )
            {
                // Skip the '*'
                text++;
                // Use the active version of font.
                tbft = ( tbft == HIGHLIGHT ) ? ACTIVE_HIGHLIGHT : ACTIVE;
            }
            textbox_font ( boxes[i], tbft );
            textbox_text ( boxes[i], text );
        }

        textbox_draw ( boxes[i] );
    }
}


/* Very bad implementation of tab completion.
 * It will complete to the common prefix
 */
static int calculate_common_prefix ( char **filtered, int max_elements )
{
    int length_prefix = 0;

    if ( filtered && filtered[0] != NULL )
    {
        int  found = 1;
        char *p    = filtered[0];

        do
        {
            found = 1;

            for ( int j = 0; j < max_elements && filtered[j] != NULL; j++ )
            {
                if ( filtered[j][length_prefix] == '\0' || filtered[j][length_prefix] != *p )
                {
                    if ( found )
                    {
                        found = 0;
                    }

                    break;
                }
            }

            if ( found )
            {
                length_prefix++;
            }

            p++;
        } while ( found );
        // cut off to be valid utf8.
        for ( int j = 0; j < length_prefix; )
        {
            if ( ( filtered[0][j] & 0x80 ) == 0 )
            {
                j++;
            }
            else if ( ( filtered[0][j] & 0xf0 ) == 0xc0 )
            {
                // 2 bytes
                if ( ( j + 2 ) >= length_prefix )
                {
                    length_prefix = j;
                }
                j += 2;
            }
            else if ( ( filtered[0][j] & 0xf0 ) == 0xe0 )
            {
                // 3 bytes
                if ( ( j + 3 ) >= length_prefix )
                {
                    length_prefix = j;
                }
                j += 3;
            }
            else if ( ( filtered[0][j] & 0xf0 ) == 0xf0 )
            {
                // 4 bytes
                if ( ( j + 4 ) >= length_prefix )
                {
                    length_prefix = j;
                }
                j += 4;
            }
            else
            {
                j++;
            };
        }
    }
    return length_prefix;
}


int window_match ( char **tokens, __attribute__( ( unused ) ) const char *input, int index, void *data )
{
    int     match = 1;
    winlist *ids  = ( winlist * ) data;
    client  *c    = window_client ( ids->array[index] );

    if ( tokens )
    {
        for ( int j = 1; match && tokens[j]; j++ )
        {
            int test = 0;

            if ( !test && c->title[0] != '\0' )
            {
                test = ( strcasestr ( c->title, tokens[j] ) != NULL );
            }

            if ( !test && c->class[0] != '\0' )
            {
                test = ( strcasestr ( c->class, tokens[j] ) != NULL );
            }

            if ( !test && c->role[0] != '\0' )
            {
                test = ( strcasestr ( c->role, tokens[j] ) != NULL );
            }

            if ( !test && c->name[0] != '\0' )
            {
                test = ( strcasestr ( c->name, tokens[j] ) != NULL );
            }

            if ( test == 0 )
            {
                match = 0;
            }
        }
    }

    return match;
}

MenuReturn menu ( char **lines, char **input, char *prompt, Time *time, int *shift,
                  menu_match_cb mmc, void *mmc_data, int *selected_line )
{
    int          retv = MENU_CANCEL;
    unsigned int i, j;
    unsigned int columns = config.menu_columns;
    workarea     mon;


    unsigned int num_lines   = 0;
    int          last_offset = 0;

    for (; lines != NULL && lines[num_lines]; num_lines++ )
    {
        ;
    }


    unsigned int max_elements = MIN ( config.menu_lines * columns, num_lines );
    // TODO, clean this up.
    // Calculate the number or rows.
    // we do this by getting the num_lines rounded up to X columns (num elements is better name)
    // then dividing by columns.
    unsigned int max_rows = MIN ( config.menu_lines,
                                  (unsigned int) (
                                      ( num_lines + ( columns - num_lines % columns ) % columns ) /
                                      ( columns )
                                      ) );

    if ( config.fixed_num_lines == TRUE )
    {
        max_elements = config.menu_lines * columns;
        max_rows     = config.menu_lines;
        // If it would fit in one column, only use one column.
        if ( num_lines < max_elements )
        {
            columns      = ( num_lines + ( max_rows - num_lines % max_rows ) % max_rows ) / max_rows;
            max_elements = config.menu_lines * columns;
        }
        // Sanitize.
        if ( columns == 0 )
        {
            columns = 1;
        }
    }
    // More hacks.
    if ( config.hmode == TRUE )
    {
        max_rows = 1;
    }

    // Get active monitor size.
    monitor_active ( &mon );

    // Calculate as float to stop silly, big rounding down errors.
    int w             = config.menu_width < 101 ? ( mon.w / 100.0f ) * ( float ) config.menu_width : config.menu_width;
    int x             = mon.x + ( mon.w - w ) / 2;
    int element_width = w - ( 2 * ( config.padding ) );
    // Divide by the # columns
    element_width /= columns;
    if ( config.hmode == TRUE )
    {
        element_width = ( w - ( 2 * ( config.padding ) ) - max_elements * LINE_MARGIN ) / ( max_elements + 1 );
    }

    Window            box;
    XWindowAttributes attr;

    // main window isn't explicitly destroyed in case we switch modes. Reusing it prevents flicker
    if ( main_window != None && XGetWindowAttributes ( display, main_window, &attr ) )
    {
        box = main_window;
    }
    else
    {
        box = XCreateSimpleWindow ( display, root, x, 0, w, 300,
                                    config.menu_bw, color_get ( config.menu_bc ),
                                    color_get ( config.menu_bg ) );
        XSelectInput ( display, box, ExposureMask );


        gc = XCreateGC ( display, box, 0, 0 );
        XSetLineAttributes ( display, gc, 2, LineOnOffDash, CapButt, JoinMiter );
        XSetForeground ( display, gc, color_get ( config.menu_bc ) );
        // make it an unmanaged window
        window_set_atom_prop ( box, netatoms[_NET_WM_STATE], &netatoms[_NET_WM_STATE_ABOVE], 1 );
        XSetWindowAttributes sattr;
        sattr.override_redirect = True;
        XChangeWindowAttributes ( display, box, CWOverrideRedirect, &sattr );
        main_window = box;

        // Set the WM_NAME
        XStoreName ( display, box, "rofi" );

        // Hack to set window opacity.
        unsigned int opacity_set = ( unsigned int ) ( ( config.window_opacity / 100.0 ) * OPAQUE );
        XChangeProperty ( display, box, XInternAtom ( display, OPACITY, False ),
                          XA_CARDINAL, 32, PropModeReplace,
                          ( unsigned char * ) &opacity_set, 1L );
    }

    // search text input

    textbox *prompt_tb = textbox_create ( box, TB_AUTOHEIGHT | TB_AUTOWIDTH,
                                          ( config.padding ),
                                          ( config.padding ),
                                          0, 0,
                                          NORMAL,
                                          prompt );

    textbox *text = textbox_create ( box, TB_AUTOHEIGHT | TB_EDITABLE,
                                     ( config.padding ) + prompt_tb->w,
                                     ( config.padding ),
                                     ( ( config.hmode == TRUE ) ?
                                       element_width : ( w - ( 2 * ( config.padding ) ) ) ) - prompt_tb->w, 1,
                                     NORMAL,
                                     ( input != NULL ) ? *input : "" );

    int line_height = text->font->ascent + text->font->descent;

    textbox_show ( text );
    textbox_show ( prompt_tb );


    // filtered list display
    textbox **boxes = calloc ( 1, sizeof ( textbox* ) * max_elements );

    for ( i = 0; i < max_elements; i++ )
    {
        int line = ( i ) % max_rows + ( ( config.hmode == FALSE ) ? 1 : 0 );
        int col  = ( i ) / max_rows + ( ( config.hmode == FALSE ) ? 0 : 1 );
        boxes[i] = textbox_create ( box,
                                    0,
                                    ( config.padding ) + col * ( element_width + LINE_MARGIN ),                           // X
                                    line * line_height + config.padding + ( ( config.hmode == TRUE ) ? 0 : LINE_MARGIN ), // y
                                    element_width,                                                                        // w
                                    line_height,                                                                          // h
                                    NORMAL, "" );
        textbox_show ( boxes[i] );
    }
    // Arrows
    textbox *arrowbox_top = NULL, *arrowbox_bottom = NULL;
    if ( config.hmode == FALSE )
    {
        arrowbox_top = textbox_create ( box, TB_AUTOHEIGHT | TB_AUTOWIDTH,
                                        ( config.padding ),
                                        ( config.padding ),
                                        0, 0,
                                        NORMAL,
                                        "↑" );
        arrowbox_bottom = textbox_create ( box, TB_AUTOHEIGHT | TB_AUTOWIDTH,
                                           ( config.padding ),
                                           ( config.padding ),
                                           0, 0,
                                           NORMAL,
                                           "↓" );

        textbox_move ( arrowbox_top,
                       w - config.padding - arrowbox_top->w,
                       config.padding + line_height + LINE_MARGIN );
        textbox_move ( arrowbox_bottom,
                       w - config.padding - arrowbox_bottom->w,
                       config.padding + max_rows * line_height + LINE_MARGIN );
    }

    // filtered list
    char         **filtered     = calloc ( num_lines, sizeof ( char* ) );
    int          *line_map      = calloc ( num_lines, sizeof ( int ) );
    unsigned int filtered_lines = 0;

    if ( input && *input )
    {
        char **tokens = tokenize ( *input );

        // input changed
        for ( i = 0, j = 0; i < num_lines; i++ )
        {
            int match = mmc ( tokens, lines[i], i, mmc_data );

            // If each token was matched, add it to list.
            if ( match )
            {
                line_map[j]   = i;
                filtered[j++] = lines[i];
                filtered_lines++;
            }
        }

        tokenize_free ( tokens );
    }
    else
    {
        int jin = 0;
        for ( i = 0; i < num_lines; i++ )
        {
            filtered[jin] = lines[i];
            line_map[jin] = i;
            jin++;
            filtered_lines++;
        }
    }

    // resize window vertically to suit
    // Subtract the margin of the last row.
    int h = line_height * ( max_rows + 1 ) + ( config.padding ) * 2 + LINE_MARGIN;


    if ( config.hmode == TRUE )
    {
        h = line_height + ( config.padding ) * 2;
    }

    // Default location is center.
    int y = mon.y + ( mon.h - h ) / 2;

    // Determine window location
    switch ( config.location )
    {
    case WL_NORTH_WEST:
        x = mon.x;

    case WL_NORTH:
        y = mon.y;
        break;

    case WL_NORTH_EAST:
        y = mon.y;

    case WL_EAST:
        x = mon.x + mon.w - w - config.menu_bw * 2;
        break;

    case WL_EAST_SOUTH:
        x = mon.x + mon.w - w - config.menu_bw * 2;

    case WL_SOUTH:
        y = mon.y + mon.h - h - config.menu_bw * 2;
        break;

    case WL_SOUTH_WEST:
        y = mon.y + mon.h - h - config.menu_bw * 2;

    case WL_WEST:
        x = mon.x;
        break;

    case WL_CENTER:
    default:
        break;
    }
    // Apply offset.
    y += config.y_offset;
    x += config.x_offset;


    XMoveResizeWindow ( display, box, x, y, w, h );
    XMapRaised ( display, box );

    // if grabbing keyboard failed, fall through
    if ( take_keyboard ( box ) )
    {
        KeySym       prev_key = 0;
        unsigned int selected = 0;
        for (;; )
        {
            XEvent ev;
            XNextEvent ( display, &ev );

            if ( ev.type == Expose )
            {
                while ( XCheckTypedEvent ( display, Expose, &ev ) )
                {
                    ;
                }

                textbox_draw ( text );
                textbox_draw ( prompt_tb );
                menu_draw ( boxes, max_elements, num_lines, &last_offset, selected, filtered );
                menu_set_arrow_text ( filtered_lines, selected,
                                      max_elements, arrowbox_top,
                                      arrowbox_bottom );
                // Why do we need the specian -1?
                if ( config.hmode == FALSE && max_elements > 0 )
                {
                    XDrawLine ( display, main_window, gc, ( config.padding ),
                                line_height + ( config.padding ) + ( LINE_MARGIN - 2 ) / 2,
                                w - ( ( config.padding ) ) - 1,
                                line_height + ( config.padding ) + ( LINE_MARGIN - 2 ) / 2 );
                }
            }
            else if ( ev.type == KeyPress )
            {
                while ( XCheckTypedEvent ( display, KeyPress, &ev ) )
                {
                    ;
                }

                if ( time )
                {
                    *time = ev.xkey.time;
                }

                KeySym key = XkbKeycodeToKeysym ( display, ev.xkey.keycode, 0, 0 );

                if ( ( ( ev.xkey.state & ShiftMask ) == ShiftMask ) &&
                     key == XK_slash )
                {
                    retv           = MENU_NEXT;
                    *selected_line = 0;
                    break;
                }
                else if ( ( ( ev.xkey.state & ShiftMask ) == ShiftMask ) &&
                          key == XK_Delete )
                {
                    if ( filtered[selected] != NULL )
                    {
                        *selected_line = line_map[selected];
                        retv           = MENU_ENTRY_DELETE;
                        break;
                    }
                }

                int rc = textbox_keypress ( text, &ev );

                if ( rc < 0 )
                {
                    if ( shift != NULL )
                    {
                        ( *shift ) = ( ( ev.xkey.state & ShiftMask ) == ShiftMask );
                    }

                    if ( filtered && filtered[selected] )
                    {
                        retv           = MENU_OK;
                        *selected_line = line_map[selected];
                    }
                    else
                    {
                        retv = MENU_CUSTOM_INPUT;
                    }

                    break;
                }
                else if ( rc )
                {
                    char **tokens = tokenize ( text->text );

                    // input changed
                    for ( i = 0, j = 0; i < num_lines; i++ )
                    {
                        int match = mmc ( tokens, lines[i], i, mmc_data );

                        // If each token was matched, add it to list.
                        if ( match )
                        {
                            line_map[j]   = i;
                            filtered[j++] = lines[i];
                        }
                    }

                    // Cleanup + bookkeeping.
                    filtered_lines = j;
                    selected       = MIN ( selected, j - 1 );

                    for (; j < num_lines; j++ )
                    {
                        filtered[j] = NULL;
                    }

                    if ( config.zeltak_mode && filtered_lines == 1 )
                    {
                        if ( filtered[selected] )
                        {
                            retv           = MENU_OK;
                            *selected_line = line_map[selected];
                        }
                        else
                        {
                            fprintf ( stderr, "We should never hit this." );
                            abort ();
                        }

                        break;
                    }

                    tokenize_free ( tokens );
                }
                else
                {
                    // unhandled key
                    KeySym key = XkbKeycodeToKeysym ( display, ev.xkey.keycode, 0, 0 );

                    if ( key == XK_Escape
                         // pressing one of the global key bindings closes the switcher. this allows fast closing of the menu if an item is not selected
                         || ( ( windows_modmask == AnyModifier || ev.xkey.state & windows_modmask ) && key == windows_keysym )
                         || ( ( rundialog_modmask == AnyModifier || ev.xkey.state & rundialog_modmask ) && key == rundialog_keysym )
                         || ( ( sshdialog_modmask == AnyModifier || ev.xkey.state & sshdialog_modmask ) && key == sshdialog_keysym )
                         )
                    {
                        retv = MENU_CANCEL;
                        break;
                    }
                    else
                    {
                        // Up or Shift-Tab
                        if ( key == XK_Up || ( key == XK_Tab && ev.xkey.state & ShiftMask ) ||
                             ( key == XK_j && ev.xkey.state & ControlMask )  )
                        {
                            if ( selected == 0 )
                            {
                                selected = filtered_lines;
                            }

                            if ( selected > 0 )
                            {
                                selected--;
                            }
                        }
                        else if ( key == XK_Down ||
                                  ( key == XK_k && ev.xkey.state & ControlMask ) )
                        {
                            selected = selected < filtered_lines - 1 ? MIN ( filtered_lines - 1, selected + 1 ) : 0;
                        }
                        else if ( key == XK_Page_Up )
                        {
                            if ( selected < max_elements )
                            {
                                selected = 0;
                            }
                            else
                            {
                                selected -= ( max_elements - 1 );
                            }
                        }
                        else if ( key == XK_Page_Down )
                        {
                            selected += ( max_elements - 1 );

                            if ( selected >= num_lines )
                            {
                                selected = num_lines - 1;
                            }
                        }
                        else if ( key == XK_h && ev.xkey.state & ControlMask )
                        {
                            if ( selected < max_rows )
                            {
                                selected = 0;
                            }
                            else
                            {
                                selected -= max_rows;
                            }
                        }
                        else if (  key == XK_l && ev.xkey.state & ControlMask )
                        {
                            selected += max_rows;
                            if ( selected >= num_lines )
                            {
                                selected = num_lines - 1;
                            }
                        }
                        else if ( key == XK_Home || key == XK_KP_Home )
                        {
                            selected = 0;
                        }
                        else if ( key == XK_End || key == XK_KP_End )
                        {
                            selected = num_lines - 1;
                        }
                        else if ( key == XK_Tab )
                        {
                            if ( filtered_lines == 1 )
                            {
                                if ( filtered[selected] )
                                {
                                    retv           = MENU_OK;
                                    *selected_line = line_map[selected];
                                }
                                else
                                {
                                    fprintf ( stderr, "We should never hit this." );
                                    abort ();
                                }

                                break;
                            }

                            int length_prefix = calculate_common_prefix ( filtered, num_lines );

                            // TODO: memcmp to utf8 aware cmp.
                            if ( length_prefix && memcmp ( filtered[0], text->text, length_prefix ) )
                            {
                                // Do not want to modify original string, so make copy.
                                // not eff..
                                char * str = malloc ( sizeof ( char ) * ( length_prefix + 1 ) );
                                memcpy ( str, filtered[0], length_prefix );
                                str[length_prefix] = '\0';
                                textbox_text ( text, str );
                                textbox_cursor_end ( text );
                                free ( str );
                            }
                            // Double tab!
                            else if ( filtered_lines == 0 && key == prev_key )
                            {
                                retv           = MENU_NEXT;
                                *selected_line = 0;
                                break;
                            }
                            else
                            {
                                selected = selected < filtered_lines - 1 ? MIN ( filtered_lines - 1, selected + 1 ) : 0;
                            }
                        }
                    }
                }
                menu_hide_arrow_text ( filtered_lines, selected,
                                       max_elements, arrowbox_top,
                                       arrowbox_bottom );
                textbox_draw ( text );
                textbox_draw ( prompt_tb );
                menu_draw ( boxes, max_elements, num_lines, &last_offset, selected, filtered );
                menu_set_arrow_text ( filtered_lines, selected,
                                      max_elements, arrowbox_top,
                                      arrowbox_bottom );
                prev_key = key;
            }
        }

        release_keyboard ();
    }

    free ( *input );

    *input = strdup ( text->text );

    textbox_free ( text );
    textbox_free ( prompt_tb );
    textbox_free ( arrowbox_bottom );
    textbox_free ( arrowbox_top );

    for ( i = 0; i < max_elements; i++ )
    {
        textbox_free ( boxes[i] );
    }

    free ( boxes );

    free ( filtered );
    free ( line_map );

    return retv;
}




SwitcherMode run_switcher_window ( char **input )
{
    SwitcherMode retv = MODE_EXIT;
    // find window list
    Atom         type;
    int          nwins;
    Window       wins[100];
    int          count       = 0;
    Window       curr_win_id = 0;

    // Get the active window so we can highlight this.
    if ( !( window_get_prop ( root, netatoms[_NET_ACTIVE_WINDOW], &type,
                              &count, &curr_win_id, sizeof ( Window ) )
            && type == XA_WINDOW && count > 0 ) )
    {
        curr_win_id = 0;
    }

    if ( window_get_prop ( root, netatoms[_NET_CLIENT_LIST_STACKING],
                           &type, &nwins, wins, 100 * sizeof ( Window ) )
         && type == XA_WINDOW )
    {
        char          pattern[50];
        int           i;
        unsigned int  classfield = 0;
        unsigned long desktops   = 0;
        // windows we actually display. may be slightly different to _NET_CLIENT_LIST_STACKING
        // if we happen to have a window destroyed while we're working...
        winlist *ids = winlist_new ();



        // calc widths of fields
        for ( i = nwins - 1; i > -1; i-- )
        {
            client *c;

            if ( ( c = window_client ( wins[i] ) )
                 && !c->xattr.override_redirect
                 && !client_has_state ( c, netatoms[_NET_WM_STATE_SKIP_PAGER] )
                 && !client_has_state ( c, netatoms[_NET_WM_STATE_SKIP_TASKBAR] ) )
            {
                classfield = MAX ( classfield, strlen ( c->class ) );

#ifdef HAVE_I3_IPC_H

                // In i3 mode, skip the i3bar completely.
                if ( config_i3_mode && strstr ( c->class, "i3bar" ) != NULL )
                {
                    continue;
                }

#endif
                if ( c->window == curr_win_id )
                {
                    c->active = TRUE;
                }
                winlist_append ( ids, c->window, NULL );
            }
        }

        // Create pattern for printing the line.
        if ( !window_get_cardinal_prop ( root, netatoms[_NET_NUMBER_OF_DESKTOPS], &desktops, 1 ) )
        {
            desktops = 1;
        }
#ifdef HAVE_I3_IPC_H
        if ( config_i3_mode )
        {
            sprintf ( pattern, "%%s%%-%ds   %%s", MAX ( 5, classfield ) );
        }
        else
        {
#endif
        sprintf ( pattern, "%%s%%-%ds  %%-%ds   %%s", desktops < 10 ? 1 : 2, MAX ( 5, classfield ) );
#ifdef HAVE_I3_IPC_H
    }
#endif
        char **list = calloc ( ( ids->len + 1 ), sizeof ( char* ) );
        int lines   = 0;

        // build the actual list
        Window w = 0;
        winlist_ascend ( ids, i, w )
        {
            client *c;

            if ( ( c = window_client ( w ) ) )
            {
                // final line format
                unsigned long wmdesktop;
                char          desktop[5];
                desktop[0] = 0;
                char          *line = malloc ( strlen ( c->title ) + strlen ( c->class ) + classfield + 50 );
#ifdef HAVE_I3_IPC_H
                if ( !config_i3_mode )
                {
#endif
                // find client's desktop. this is zero-based, so we adjust by since most
                // normal people don't think like this :-)
                if ( !window_get_cardinal_prop ( c->window, netatoms[_NET_WM_DESKTOP], &wmdesktop, 1 ) )
                {
                    wmdesktop = 0xFFFFFFFF;
                }

                if ( wmdesktop < 0xFFFFFFFF )
                {
                    sprintf ( desktop, "%d", (int) wmdesktop + 1 );
                }

                sprintf ( line, pattern, ( c->active ) ? "*" : "", desktop, c->class, c->title );
#ifdef HAVE_I3_IPC_H
            }
            else
            {
                sprintf ( line, pattern, ( c->active ) ? "*" : "", c->class, c->title );
            }
#endif

                list[lines++] = line;
            }
        }
        Time time;
        int selected_line = 0;
        MenuReturn mretv  = menu ( list, input, "window:", &time, NULL, window_match, ids, &selected_line );

        if ( mretv == MENU_NEXT )
        {
            retv = NEXT_DIALOG;
        }
        else if ( mretv == MENU_OK && list[selected_line] )
        {
#ifdef HAVE_I3_IPC_H

            if ( config_i3_mode )
            {
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


        for ( i = 0; i < lines; i++ )
        {
            free ( list[i] );
        }

        free ( list );
        winlist_free ( ids );
    }

    return retv;
}

static void run_switcher ( int do_fork, SwitcherMode mode )
{
    // we fork because it's technically possible to have multiple window
    // lists up at once on a zaphod multihead X setup.
    // this also happens to isolate the Xft font stuff in a child process
    // that gets cleaned up every time. that library shows some valgrind
    // strangeness...
    if ( do_fork == TRUE )
    {
        if ( fork () )
        {
            return;
        }

        display = XOpenDisplay ( 0 );
        XSync ( display, True );
    }

    char *input = NULL;

    do
    {
        SwitcherMode retv = MODE_EXIT;

        if ( mode == WINDOW_SWITCHER )
        {
            retv = run_switcher_window ( &input );
        }
        else if ( mode == RUN_DIALOG )
        {
            retv = run_switcher_dialog ( &input );
        }
        else if ( mode == SSH_DIALOG )
        {
            retv = ssh_switcher_dialog ( &input );
        }
        else if ( mode == DMENU_DIALOG )
        {
            retv = dmenu_switcher_dialog ( &input );
        }

        if ( retv == NEXT_DIALOG )
        {
            mode = ( mode + 1 ) % NUM_DIALOGS;
        }
        else
        {
            mode = retv;
        }
    } while ( mode != MODE_EXIT );

    free ( input );

    if ( do_fork == TRUE )
    {
        exit ( EXIT_SUCCESS );
    }
}

// KeyPress event
static void handle_keypress ( XEvent *ev )
{
    KeySym key = XkbKeycodeToKeysym ( display, ev->xkey.keycode, 0, 0 );

    if ( ( windows_modmask == AnyModifier || ev->xkey.state & windows_modmask ) &&
         key == windows_keysym )
    {
        run_switcher ( TRUE, WINDOW_SWITCHER );
    }

    if ( ( rundialog_modmask == AnyModifier || ev->xkey.state & rundialog_modmask ) &&
         key == rundialog_keysym )
    {
        run_switcher ( TRUE, RUN_DIALOG );
    }

    if ( ( sshdialog_modmask == AnyModifier || ev->xkey.state & sshdialog_modmask ) &&
         key == sshdialog_keysym )
    {
        run_switcher ( TRUE, SSH_DIALOG );
    }
}

// convert a Mod+key arg to mod mask and keysym
void parse_key ( char *combo, unsigned int *mod, KeySym *key )
{
    unsigned int modmask = 0;

    if ( strcasestr ( combo, "shift" ) )
    {
        modmask |= ShiftMask;
    }

    if ( strcasestr ( combo, "control" ) )
    {
        modmask |= ControlMask;
    }

    if ( strcasestr ( combo, "mod1" ) )
    {
        modmask |= Mod1Mask;
    }

    if ( strcasestr ( combo, "alt" ) )
    {
        modmask |= Mod1Mask;
    }

    if ( strcasestr ( combo, "mod2" ) )
    {
        modmask |= Mod2Mask;
    }

    if ( strcasestr ( combo, "mod3" ) )
    {
        modmask |= Mod3Mask;
    }

    if ( strcasestr ( combo, "mod4" ) )
    {
        modmask |= Mod4Mask;
    }

    if ( strcasestr ( combo, "mod5" ) )
    {
        modmask |= Mod5Mask;
    }

    *mod = modmask ? modmask : AnyModifier;

    char i = strlen ( combo );

    while ( i > 0 && !strchr ( "-+", combo[i - 1] ) )
    {
        i--;
    }

    KeySym sym = XStringToKeysym ( combo + i );

    if ( sym == NoSymbol || ( !modmask && ( strchr ( combo, '-' ) || strchr ( combo, '+' ) ) ) )
    {
        fprintf ( stderr, "sorry, cannot understand key combination: %s\n", combo );
        exit ( EXIT_FAILURE );
    }

    *key = sym;
}

// bind a key combination on a root window, compensating for Lock* states
void grab_key ( unsigned int modmask, KeySym key )
{
    KeyCode keycode = XKeysymToKeycode ( display, key );
    XUngrabKey ( display, keycode, AnyModifier, root );

    if ( modmask != AnyModifier )
    {
        // bind to combinations of mod and lock masks, so caps and numlock don't confuse people
        XGrabKey ( display, keycode, modmask, root, True, GrabModeAsync, GrabModeAsync );
        XGrabKey ( display, keycode, modmask | LockMask, root, True, GrabModeAsync, GrabModeAsync );

        if ( NumlockMask )
        {
            XGrabKey ( display, keycode, modmask | NumlockMask, root, True, GrabModeAsync, GrabModeAsync );
            XGrabKey ( display, keycode, modmask | NumlockMask | LockMask, root, True, GrabModeAsync, GrabModeAsync );
        }
    }
    else
    {
        // nice simple single key bind
        XGrabKey ( display, keycode, AnyModifier, root, True, GrabModeAsync, GrabModeAsync );
    }
}


#ifdef HAVE_I3_IPC_H
static inline void display_get_i3_path ( Display *display )
{
    config_i3_mode = 0;
    Atom atom = XInternAtom ( display, I3_SOCKET_PATH_PROP, True );

    if ( atom != None )
    {
        i3_socket_path = window_get_text_prop ( root, atom );

        if ( i3_socket_path != NULL )
        {
            config_i3_mode = 1;
        }
    }
}
#endif //HAVE_I3_IPC_H


/**
 * Help function. This calls man.
 */
void help ()
{
    int code = execlp ( "man", "man", MANPAGE_PATH, NULL );

    if ( code == -1 )
    {
        fprintf ( stderr, "Failed to execute man: %s\n", strerror ( errno ) );
    }
}

static void parse_cmd_options ( int argc, char ** argv )
{
    // catch help request
    if ( find_arg ( argc, argv, "-h" ) >= 0 ||
         find_arg ( argc, argv, "-help" ) >= 0 )
    {
        help ();
        exit ( EXIT_SUCCESS );
    }

    if ( find_arg ( argc, argv, "-v" ) >= 0 ||
         find_arg ( argc, argv, "-version" ) >= 0 )
    {
        fprintf ( stdout, "Version: "VERSION "\n" );
        exit ( EXIT_SUCCESS );
    }

    // Parse commandline arguments about the looks.
    find_arg_int ( argc, argv, "-opacity", &( config.window_opacity ) );

    find_arg_int ( argc, argv, "-width", &( config.menu_width ) );

    find_arg_int ( argc, argv, "-lines", &( config.menu_lines ) );
    find_arg_int ( argc, argv, "-columns", &( config.menu_columns ) );

    find_arg_str ( argc, argv, "-font", &( config.menu_font ) );
    find_arg_str ( argc, argv, "-fg", &( config.menu_fg ) );
    find_arg_str ( argc, argv, "-bg", &( config.menu_bg ) );
    find_arg_str ( argc, argv, "-hlfg", &( config.menu_hlfg ) );
    find_arg_str ( argc, argv, "-hlbg", &( config.menu_hlbg ) );
    find_arg_str ( argc, argv, "-bc", &( config.menu_bc ) );
    find_arg_int ( argc, argv, "-bw", &( config.menu_bw ) );

    // Parse commandline arguments about size and position
    find_arg_int ( argc, argv, "-loc", &( config.location ) );
    find_arg_int ( argc, argv, "-padding", &( config.padding ) );
    find_arg_int ( argc, argv, "-xoffset", &( config.x_offset ) );
    find_arg_int ( argc, argv, "-yoffset", &( config.y_offset ) );
    if ( find_arg ( argc, argv, "-fixed-num-lines" ) >= 0 )
    {
        config.fixed_num_lines = 1;
    }

    // Parse commandline arguments about behavior
    find_arg_str ( argc, argv, "-terminal", &( config.terminal_emulator ) );
    if ( find_arg ( argc, argv, "-zeltak" ) >= 0 )
    {
        config.zeltak_mode = 1;
    }

    if ( find_arg ( argc, argv, "-hmode" ) >= 0 )
    {
        config.hmode = TRUE;
    }

    if ( find_arg ( argc, argv, "-ssh-set-title" ) >= 0 )
    {
        char *value;
        find_arg_str ( argc, argv, "-ssh-set-title", &value );
        if ( strcasecmp ( value, "true" ) == 0 )
        {
            config.ssh_set_title = TRUE;
        }
        else
        {
            config.ssh_set_title = FALSE;
        }
    }

    // Keybindings
    find_arg_str ( argc, argv, "-key", &( config.window_key ) );
    find_arg_str ( argc, argv, "-rkey", &( config.run_key ) );
    find_arg_str ( argc, argv, "-skey", &( config.ssh_key ) );


    // Dump.
    if ( find_arg ( argc, argv, "-dump-xresources" ) >= 0 )
    {
        xresource_dump ();
        exit ( EXIT_SUCCESS );
    }
}

static void cleanup ()
{
    textbox_cleanup ();
    // Cleanup
    if ( display != NULL )
    {
        if ( main_window != None )
        {
            XFreeGC ( display, gc );
            XDestroyWindow ( display, main_window );
            XCloseDisplay ( display );
        }
    }
    if ( cache_xattr != NULL )
    {
        winlist_free ( cache_xattr );
    }
    if ( cache_client != NULL )
    {
        winlist_free ( cache_client );
    }
#ifdef HAVE_I3_IPC_H

    if ( i3_socket_path != NULL )
    {
        free ( i3_socket_path );
    }

#endif

    // Cleaning up memory allocated by the Xresources file.
    // TODO, not happy with this.
    parse_xresource_free ();

    // Whipe the handle.. (not working)
    xdgWipeHandle ( &xdg_handle );

    free ( active_font );
}

/**
 * Do some input validation, especially the first few could break things.
 * It is good to catch them beforehand.
 *
 * This functions exits the program with 1 when it finds an invalid configuration.
 */
void config_sanity_check ( void )
{
    if ( config.menu_lines == 0 )
    {
        fprintf ( stderr, "config.menu_lines is invalid. You need at least one visible line.\n" );
        exit ( 1 );
    }
    if ( config.menu_columns == 0 )
    {
        fprintf ( stderr, "config.menu_columns is invalid. You need at least one visible column.\n" );
        exit ( 1 );
    }

    if ( config.menu_width == 0 )
    {
        fprintf ( stderr, "config.menu_width is invalid. You cannot have a window with no width.\n" );
        exit ( 1 );
    }

    if ( !( config.location >= WL_CENTER && config.location <= WL_WEST ) )
    {
        fprintf ( stderr, "config.location is invalid. ( %d >= %d >= %d) does not hold.\n",
                  WL_WEST, config.location, WL_CENTER );
        exit ( 1 );
    }

    if ( !( config.hmode == TRUE || config.hmode == FALSE ) )
    {
        fprintf ( stderr, "config.hmode is invalid.\n" );
        exit ( 1 );
    }
}


int main ( int argc, char *argv[] )
{
    // Initialize xdg, so we can grab the xdgCacheHome
    if ( xdgInitHandle ( &xdg_handle ) == NULL )
    {
        fprintf ( stderr, "Failed to initialize XDG\n" );
        return EXIT_FAILURE;
    }

    // Get the path to the cache dir.
    cache_dir = xdgCacheHome ( &xdg_handle );

    // Register cleanup function.
    atexit ( cleanup );

    // Get DISPLAY
    char *display_str = getenv ( "DISPLAY" );
    find_arg_str ( argc, argv, "-display", &display_str );

    if ( !( display = XOpenDisplay ( display_str ) ) )
    {
        fprintf ( stderr, "cannot open display!\n" );
        return EXIT_FAILURE;
    }

    // Load in config from X resources.
    parse_xresource_options ( display );

    // Parse command line for settings.
    parse_cmd_options ( argc, argv );

    // Sanity check
    config_sanity_check ();

    // Generate the font string for the line that indicates a selected item.
    if ( asprintf ( &active_font, "%s:slant=italic", config.menu_font ) < 0 )
    {
        fprintf ( stderr, "Failed to construct active string: %s\n", strerror ( errno ) );
        return EXIT_FAILURE;
    }

    // Set up X interaction.
    signal ( SIGCHLD, catch_exit );
    screen    = DefaultScreenOfDisplay ( display );
    screen_id = DefaultScreen ( display );
    root      = DefaultRootWindow ( display );
    // Set error handle
    XSync ( display, False );
    xerror = XSetErrorHandler ( display_oops );
    XSync ( display, False );

    // determine numlock mask so we can bind on keys with and without it
    XModifierKeymap *modmap = XGetModifierMapping ( display );

    for ( int i = 0; i < 8; i++ )
    {
        for ( int j = 0; j < ( int ) modmap->max_keypermod; j++ )
        {
            if ( modmap->modifiermap[i * modmap->max_keypermod + j] == XKeysymToKeycode ( display, XK_Num_Lock ) )
            {
                NumlockMask = ( 1 << i );
            }
        }
    }


    textbox_setup ( config.menu_font, active_font,
                    config.menu_bg, config.menu_fg,
                    config.menu_hlbg,
                    config.menu_hlfg );

    XFreeModifiermap ( modmap );

    cache_client = winlist_new ();
    cache_xattr  = winlist_new ();

    // X atom values
    for ( int i = 0; i < NETATOMS; i++ )
    {
        netatoms[i] = XInternAtom ( display, netatom_names[i], False );
    }

#ifdef HAVE_I3_IPC_H
    // Check for i3
    display_get_i3_path ( display );
#endif


    // flags to run immediately and exit
    if ( find_arg ( argc, argv, "-now" ) >= 0 )
    {
        run_switcher ( FALSE, WINDOW_SWITCHER );
    }
    else if ( find_arg ( argc, argv, "-rnow" ) >= 0 )
    {
        run_switcher ( FALSE, RUN_DIALOG );
    }
    else if ( find_arg ( argc, argv, "-snow" ) >= 0 )
    {
        run_switcher ( FALSE, SSH_DIALOG );
    }
    else if ( find_arg ( argc, argv, "-dmenu" ) >= 0 )
    {
        find_arg_str ( argc, argv, "-p", &dmenu_prompt );
        run_switcher ( FALSE, DMENU_DIALOG );
    }
    else
    {
        // Daemon mode, Listen to key presses..
        parse_key ( config.window_key, &windows_modmask, &windows_keysym );
        grab_key ( windows_modmask, windows_keysym );

        parse_key ( config.run_key, &rundialog_modmask, &rundialog_keysym );
        grab_key ( rundialog_modmask, rundialog_keysym );

        parse_key ( config.ssh_key, &sshdialog_modmask, &sshdialog_keysym );
        grab_key ( sshdialog_modmask, sshdialog_keysym );

        // Main loop
        for (;; )
        {
            XEvent ev;
            // caches only live for a single event
            winlist_empty ( cache_xattr );
            winlist_empty ( cache_client );

            // block and wait for something
            XNextEvent ( display, &ev );

            if ( ev.xany.window == None )
            {
                continue;
            }

            if ( ev.type == KeyPress )
            {
                handle_keypress ( &ev );
            }
        }
    }

    return EXIT_SUCCESS;
}
