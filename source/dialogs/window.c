/**
 * rofi
 *
 * MIT/X11 License
 * Copyright 2013-2015 Qball Cow <qball@gmpclient.org>
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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include "rofi.h"
#include "helper.h"
#include "x11-helper.h"
#include "i3-support.h"
#include "dialogs/window.h"


#define WINLIST        32

#define CLIENTTITLE    100
#define CLIENTCLASS    50
#define CLIENTNAME     50
#define CLIENTSTATE    10
#define CLIENTROLE     50

// a manageable window
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
    int               demands;
    long              hint_flags;
} client;
// TODO
extern Display *display;
// window lists
typedef struct
{
    Window *array;
    void   **data;
    int    len;
} winlist;

winlist *cache_client = NULL;
winlist *cache_xattr  = NULL;

/**
 * Create a window list, pre-seeded with WINLIST entries.
 *
 * @returns A new window list.
 */
static winlist* winlist_new ()
{
    winlist *l = g_malloc ( sizeof ( winlist ) );
    l->len   = 0;
    l->array = g_malloc_n ( WINLIST + 1, sizeof ( Window ) );
    l->data  = g_malloc_n ( WINLIST + 1, sizeof ( void* ) );
    return l;
}

/**
 * @param l The winlist.
 * @param w The window to add.
 * @param d Data pointer.
 *
 * Add one entry. If Full, extend with WINLIST entries.
 *
 * @returns 0 if failed, 1 is successful.
 */
static int winlist_append ( winlist *l, Window w, void *d )
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

static void winlist_empty ( winlist *l )
{
    while ( l->len > 0 ) {
        g_free ( l->data[--( l->len )] );
    }
}

/**
 * @param l The winlist entry
 *
 * Free the winlist.
 */
static void winlist_free ( winlist *l )
{
    if ( l != NULL ) {
        winlist_empty ( l );
        g_free ( l->array );
        g_free ( l->data );
        g_free ( l );
    }
}

/**
 * @param l The winlist.
 * @param w The window to find.
 *
 * Find the window in the list, and return the array entry.
 *
 * @returns -1 if failed, index is successful.
 */
static int winlist_find ( winlist *l, Window w )
{
// iterate backwards. Theory is: windows most often accessed will be
// nearer the end. Testing with kcachegrind seems to support this...
    int i;

    for ( i = ( l->len - 1 ); i >= 0; i-- ) {
        if ( l->array[i] == w ) {
            return i;
        }
    }

    return -1;
}
/**
 * Create empty X11 cache for windows and windows attributes.
 */
static void x11_cache_create ( void )
{
    cache_client = winlist_new ();
    cache_xattr  = winlist_new ();
}

/**
 * Free the cache.
 */
static void x11_cache_free ( void )
{
    winlist_free ( cache_xattr );
    cache_xattr = NULL;
    winlist_free ( cache_client );
    cache_client = NULL;
}

/**
 * @param d Display connection to X server
 * @param w window
 *
 * Get window attributes.
 * This functions uses caching.
 *
 * @returns a XWindowAttributes
 */
static XWindowAttributes* window_get_attributes ( Display *display, Window w )
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
// _NET_WM_STATE_*
static int client_has_state ( client *c, Atom state )
{
    for ( int i = 0; i < c->states; i++ ) {
        if ( c->state[i] == state ) {
            return 1;
        }
    }

    return 0;
}

static client* window_client ( Display *display, Window win )
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

    XWMHints *wh;
    if ( ( wh = XGetWMHints ( display, c->window ) ) != NULL ) {
        c->hint_flags = wh->flags;
        XFree ( wh );
    }

    monitor_dimensions ( display, c->xattr.screen, c->xattr.x, c->xattr.y, &c->monitor );
    winlist_append ( cache_client, c->window, c );
    return c;
}

typedef struct _SwitcherModePrivateData
{
    unsigned int id;
    char         **cmd_list;
    unsigned int cmd_list_length;
    winlist      *ids;
    int          config_i3_mode;
    int          init;
    // Current window.
    unsigned int index;
    char         *cache;
} SwitcherModePrivateData;

static int window_match ( char **tokens, __attribute__( ( unused ) ) const char *input,
                          int case_sensitive, unsigned int index, Switcher *sw )
{
    SwitcherModePrivateData *rmpd = (SwitcherModePrivateData *) sw->private_data;
    int                     match = 1;
    winlist                 *ids  = ( winlist * ) rmpd->ids;
    client                  *c    = window_client ( display, ids->array[index] );

    if ( tokens ) {
        for ( int j = 0; match && tokens != NULL && tokens[j] != NULL; j++ ) {
            int test = 0;
            // Dirty hack. Normally token_match does _all_ the matching,
            // Now we want it to match only one item at the time.
            // If hack not in place it would not match queries spanning multiple fields.
            // e.g. when searching 'title element' and 'class element'
            char *ftokens[2] = { tokens[j], NULL };
            if ( !test && c->title[0] != '\0' ) {
                test = token_match ( ftokens, c->title, case_sensitive, 0, NULL );
            }

            if ( !test && c->class[0] != '\0' ) {
                test = token_match ( ftokens, c->class, case_sensitive, 0, NULL );
            }

            if ( !test && c->role[0] != '\0' ) {
                test = token_match ( ftokens, c->role, case_sensitive, 0, NULL );
            }

            if ( !test && c->name[0] != '\0' ) {
                test = token_match ( ftokens, c->name, case_sensitive, 0, NULL );
            }

            if ( test == 0 ) {
                match = 0;
            }
        }
    }

    return match;
}
static void window_mode_init ( Switcher *sw )
{
    if ( sw->private_data == NULL ) {
        SwitcherModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
        sw->private_data = (void *) pd;
        pd->init         = FALSE;
    }
}

static char ** window_mode_get_data ( unsigned int *length, Switcher *sw )
{
    SwitcherModePrivateData *pd = (SwitcherModePrivateData *) sw->private_data;
    if ( !pd->init ) {
        Screen *screen = DefaultScreenOfDisplay ( display );
        Window root    = RootWindow ( display, XScreenNumberOfScreen ( screen ) );
        // find window list
        Atom   type;
        int    nwins;
        Window wins[100];
        int    count       = 0;
        Window curr_win_id = 0;
        // Create cache

        x11_cache_create ();
        // Check for i3
        pd->config_i3_mode = i3_support_initialize ( display );

        // Get the active window so we can highlight this.
        if ( !( window_get_prop ( display, root, netatoms[_NET_ACTIVE_WINDOW], &type,
                                  &count, &curr_win_id, sizeof ( Window ) )
                && type == XA_WINDOW && count > 0 ) ) {
            curr_win_id = 0;
        }

        if ( window_get_prop ( display, root, netatoms[_NET_CLIENT_LIST_STACKING],
                               &type, &nwins, wins, 100 * sizeof ( Window ) )
             && type == XA_WINDOW ) {
            char          pattern[50];
            int           i;
            unsigned int  classfield = 0;
            unsigned long desktops   = 0;
            // windows we actually display. May be slightly different to _NET_CLIENT_LIST_STACKING
            // if we happen to have a window destroyed while we're working...
            pd->ids = winlist_new ();



            // calc widths of fields
            for ( i = nwins - 1; i > -1; i-- ) {
                client *c;

                if ( ( c = window_client ( display, wins[i] ) )
                     && !c->xattr.override_redirect
                     && !client_has_state ( c, netatoms[_NET_WM_STATE_SKIP_PAGER] )
                     && !client_has_state ( c, netatoms[_NET_WM_STATE_SKIP_TASKBAR] ) ) {
                    classfield = MAX ( classfield, strlen ( c->class ) );

                    // In i3 mode, skip the i3bar completely.
                    if ( pd->config_i3_mode && strstr ( c->class, "i3bar" ) != NULL ) {
                        continue;
                    }
                    if ( client_has_state ( c, netatoms[_NET_WM_STATE_DEMANDS_ATTENTION] ) ) {
                        c->demands = TRUE;
                    }
                    if ( ( c->hint_flags & XUrgencyHint ) == XUrgencyHint ) {
                        c->demands = TRUE;
                    }

                    if ( c->window == curr_win_id ) {
                        c->active = TRUE;
                    }
                    winlist_append ( pd->ids, c->window, NULL );
                }
            }

            // Create pattern for printing the line.
            if ( !window_get_cardinal_prop ( display, root, netatoms[_NET_NUMBER_OF_DESKTOPS], &desktops, 1 ) ) {
                desktops = 1;
            }
            if ( pd->config_i3_mode ) {
                sprintf ( pattern, "%%-%ds   %%s", MAX ( 5, classfield ) );
            }
            else{
                sprintf ( pattern, "%%-%ds  %%-%ds   %%s", desktops < 10 ? 1 : 2, MAX ( 5, classfield ) );
            }
            pd->cmd_list = g_malloc0_n ( ( pd->ids->len + 1 ), sizeof ( char* ) );

            // build the actual list
            for ( i = 0; i < ( pd->ids->len ); i++ ) {
                Window w = pd->ids->array[i];
                client *c;

                if ( ( c = window_client ( display, w ) ) ) {
                    // final line format
                    unsigned long wmdesktop;
                    char          desktop[5];
                    desktop[0] = 0;
                    char          *line = g_malloc ( strlen ( c->title ) + strlen ( c->class ) + classfield + 50 );
                    if ( !pd->config_i3_mode ) {
                        // find client's desktop.
                        if ( !window_get_cardinal_prop ( display, c->window, netatoms[_NET_WM_DESKTOP], &wmdesktop, 1 ) ) {
                            // Assume the client is on all desktops.
                            wmdesktop = 0xFFFFFFFF;
                        }

                        if ( wmdesktop < 0xFFFFFFFF ) {
                            sprintf ( desktop, "%d", (int) wmdesktop );
                        }

                        sprintf ( line, pattern, desktop, c->class, c->title );
                    }
                    else{
                        sprintf ( line, pattern, c->class, c->title );
                    }

                    pd->cmd_list[pd->cmd_list_length++] = line;
                }
            }
        }
        pd->init = TRUE;
    }
    *length = pd->cmd_list_length;
    return pd->cmd_list;
}
static SwitcherMode window_mode_result ( int mretv, G_GNUC_UNUSED char **input, unsigned int selected_line, Switcher *sw )
{
    SwitcherModePrivateData *rmpd = (SwitcherModePrivateData *) sw->private_data;
    SwitcherMode            retv  = MODE_EXIT;
    if ( mretv & MENU_NEXT ) {
        retv = NEXT_DIALOG;
    }
    else if ( mretv & MENU_PREVIOUS ) {
        retv = PREVIOUS_DIALOG;
    }
    else if ( ( mretv & MENU_QUICK_SWITCH ) == MENU_QUICK_SWITCH ) {
        retv = ( mretv & MENU_LOWER_MASK );
    }
    else if ( ( mretv & ( MENU_OK | MENU_CUSTOM_INPUT ) ) && rmpd->cmd_list[selected_line] ) {
        if ( rmpd->config_i3_mode ) {
            // Hack for i3.
            i3_support_focus_window ( rmpd->ids->array[selected_line] );
        }
        else{
            Screen *screen = DefaultScreenOfDisplay ( display );
            Window root    = RootWindow ( display, XScreenNumberOfScreen ( screen ) );
            // Change to the desktop of the selected window/client.
            // TODO: get rid of strtol
            window_send_message ( display, root, root, netatoms[_NET_CURRENT_DESKTOP],
                                  strtol ( rmpd->cmd_list[selected_line], NULL, 10 ),
                                  SubstructureNotifyMask | SubstructureRedirectMask, 0 );
            XSync ( display, False );

            window_send_message ( display, root, rmpd->ids->array[selected_line],
                                  netatoms[_NET_ACTIVE_WINDOW], 2, // 2 = pager
                                  SubstructureNotifyMask | SubstructureRedirectMask, 0 );
        }
    }
    return retv;
}


static void window_mode_destroy ( Switcher *sw )
{
    SwitcherModePrivateData *rmpd = (SwitcherModePrivateData *) sw->private_data;
    if ( rmpd != NULL ) {
        g_strfreev ( rmpd->cmd_list );
        winlist_free ( rmpd->ids );
        i3_support_free_internals ();
        x11_cache_free ();
        g_free ( rmpd->cache );
        g_free ( rmpd );
        sw->private_data = NULL;
    }
}

static const char *mgrv ( unsigned int selected_line, void *sw, G_GNUC_UNUSED int *state )
{
    SwitcherModePrivateData *rmpd = ( (Switcher *) sw )->private_data;
    if ( window_client ( display, rmpd->ids->array[selected_line] )->demands ) {
        *state |= URGENT;
    }
    if ( window_client ( display, rmpd->ids->array[selected_line] )->active ) {
        *state |= ACTIVE;
    }
    return rmpd->cmd_list[selected_line];
}

Switcher window_mode =
{
    .name         = "window",
    .tb           = NULL,
    .keycfg       = NULL,
    .keystr       = NULL,
    .modmask      = AnyModifier,
    .init         = window_mode_init,
    .get_data     = window_mode_get_data,
    .result       = window_mode_result,
    .destroy      = window_mode_destroy,
    .token_match  = window_match,
    .mgrv         = mgrv,
    .private_data = NULL,
    .free         = NULL
};
