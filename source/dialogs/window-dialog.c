/**
 * rofi
 *
 * MIT/X11 License
 * Copyright 2013-2015 Qball  Cow <qball@gmpclient.org>
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
#include "dialogs/window-dialog.h"


#define WINLIST        32

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
    winlist_free ( cache_client );
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

    monitor_dimensions ( display, c->xattr.screen, c->xattr.x, c->xattr.y, &c->monitor );
    winlist_append ( cache_client, c->window, c );
    return c;
}



static int window_match ( char **tokens, __attribute__( ( unused ) ) const char *input,
                          int case_sensitive, int index, void *data )
{
    int     match = 1;
    winlist *ids  = ( winlist * ) data;
    client  *c    = window_client ( display, ids->array[index] );

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
    // Create cache

    x11_cache_create ();
    // Check for i3
    int config_i3_mode = i3_support_initialize ( display );

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
        // windows we actually display. may be slightly different to _NET_CLIENT_LIST_STACKING
        // if we happen to have a window destroyed while we're working...
        winlist *ids = winlist_new ();



        // calc widths of fields
        for ( i = nwins - 1; i > -1; i-- ) {
            client *c;

            if ( ( c = window_client ( display, wins[i] ) )
                 && !c->xattr.override_redirect
                 && !client_has_state ( c, netatoms[_NET_WM_STATE_SKIP_PAGER] )
                 && !client_has_state ( c, netatoms[_NET_WM_STATE_SKIP_TASKBAR] ) ) {
                classfield = MAX ( classfield, strlen ( c->class ) );

                // In i3 mode, skip the i3bar completely.
                if ( config_i3_mode && strstr ( c->class, "i3bar" ) != NULL ) {
                    continue;
                }

                if ( c->window == curr_win_id ) {
                    c->active = TRUE;
                }
                winlist_append ( ids, c->window, NULL );
            }
        }

        // Create pattern for printing the line.
        if ( !window_get_cardinal_prop ( display, root, netatoms[_NET_NUMBER_OF_DESKTOPS], &desktops, 1 ) ) {
            desktops = 1;
        }
        if ( config_i3_mode ) {
            sprintf ( pattern, "%%-%ds   %%s", MAX ( 5, classfield ) );
        }
        else{
            sprintf ( pattern, "%%-%ds  %%-%ds   %%s", desktops < 10 ? 1 : 2, MAX ( 5, classfield ) );
        }
        char         **list = g_malloc0_n ( ( ids->len + 1 ), sizeof ( char* ) );
        unsigned int lines  = 0;

        // build the actual list
        for ( i = 0; i < ( ids->len ); i++ ) {
            Window w = ids->array[i];
            client *c;

            if ( ( c = window_client ( display, w ) ) ) {
                // final line format
                unsigned long wmdesktop;
                char          desktop[5];
                desktop[0] = 0;
                char          *line = g_malloc ( strlen ( c->title ) + strlen ( c->class ) + classfield + 50 );
                if ( !config_i3_mode ) {
                    // find client's desktop. this is zero-based, so we adjust by since most
                    // normal people don't think like this :-)
                    if ( !window_get_cardinal_prop ( display, c->window, netatoms[_NET_WM_DESKTOP], &wmdesktop, 1 ) ) {
                        wmdesktop = 0xFFFFFFFF;
                    }

                    if ( wmdesktop < 0xFFFFFFFF ) {
                        sprintf ( desktop, "%d", (int) wmdesktop + 1 );
                    }

                    sprintf ( line, pattern, desktop, c->class, c->title );
                }
                else{
                    sprintf ( line, pattern, c->class, c->title );
                }

                list[lines++] = line;
            }
        }
        Time       time;
        int        selected_line = 0;
        MenuReturn mretv         = menu ( list, lines, input, "window:", &time, NULL,
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
            if ( config_i3_mode ) {
                // Hack for i3.
                i3_support_focus_window ( ids->array[selected_line] );
            }
            else{
                // Change to the desktop of the selected window/client.
                // TODO: get rid of strtol
                window_send_message ( display, root, root, netatoms[_NET_CURRENT_DESKTOP],
                        strtol ( list[selected_line], NULL, 10 ) - 1,
                        SubstructureNotifyMask | SubstructureRedirectMask, time );
                XSync ( display, False );

                window_send_message ( display, root, ids->array[selected_line],
                        netatoms[_NET_ACTIVE_WINDOW], 2, // 2 = pager
                        SubstructureNotifyMask | SubstructureRedirectMask, time );
            }
        }

        g_strfreev ( list );
        winlist_free ( ids );
    }

    i3_support_free_internals ();
    x11_cache_free ();
    return retv;
}
