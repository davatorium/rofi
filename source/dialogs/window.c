/**
 * rofi
 *
 * MIT/X11 License
 * Copyright 2013-2016 Qball Cow <qball@gmpclient.org>
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

#ifdef WINDOW_MODE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_atom.h>

#include <glib.h>

#include "xcb-internal.h"
#include "xcb.h"

#include "rofi.h"
#include "settings.h"
#include "helper.h"
#include "textbox.h"
#include "x11-helper.h"
#include "i3-support.h"
#include "dialogs/window.h"

#define WINLIST             32

#define CLIENTSTATE         10
#define CLIENTWINDOWTYPE    10

// a manageable window
typedef struct
{
    xcb_window_t                      window;
    xcb_get_window_attributes_reply_t xattr;
    char                              *title;
    char                              *class;
    char                              *name;
    char                              *role;
    int                               states;
    xcb_atom_t                        state[CLIENTSTATE];
    int                               window_types;
    xcb_atom_t                        window_type[CLIENTWINDOWTYPE];
    int                               active;
    int                               demands;
    long                              hint_flags;
} client;

// window lists
typedef struct
{
    xcb_window_t *array;
    client       **data;
    int          len;
} winlist;

winlist *cache_client = NULL;

/**
 * Create a window list, pre-seeded with WINLIST entries.
 *
 * @returns A new window list.
 */
static winlist* winlist_new ()
{
    winlist *l = g_malloc ( sizeof ( winlist ) );
    l->len   = 0;
    l->array = g_malloc_n ( WINLIST + 1, sizeof ( xcb_window_t ) );
    l->data  = g_malloc_n ( WINLIST + 1, sizeof ( client* ) );
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
static int winlist_append ( winlist *l, xcb_window_t w, client *d )
{
    if ( l->len > 0 && !( l->len % WINLIST ) ) {
        l->array = g_realloc ( l->array, sizeof ( xcb_window_t ) * ( l->len + WINLIST + 1 ) );
        l->data  = g_realloc ( l->data, sizeof ( client* ) * ( l->len + WINLIST + 1 ) );
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
        client *c = l->data[--l->len];
        if ( c != NULL ) {
            g_free ( c->title );
            g_free ( c->class );
            g_free ( c->name );
            g_free ( c->role );
            g_free ( c );
        }
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
static int winlist_find ( winlist *l, xcb_window_t w )
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
    if ( cache_client == NULL ) {
        cache_client = winlist_new ();
    }
}

/**
 * Free the cache.
 */
static void x11_cache_free ( void )
{
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
static xcb_get_window_attributes_reply_t * window_get_attributes ( xcb_window_t w )
{
    xcb_get_window_attributes_cookie_t c  = xcb_get_window_attributes ( xcb->connection, w );
    xcb_get_window_attributes_reply_t  *r = xcb_get_window_attributes_reply ( xcb->connection, c, NULL );
    if ( r ) {
        return r;
    }
    return NULL;
}
// _NET_WM_STATE_*
static int client_has_state ( client *c, xcb_atom_t state )
{
    for ( int i = 0; i < c->states; i++ ) {
        if ( c->state[i] == state ) {
            return 1;
        }
    }

    return 0;
}
static int client_has_window_type ( client *c, xcb_atom_t type )
{
    for ( int i = 0; i < c->window_types; i++ ) {
        if ( c->window_type[i] == type ) {
            return 1;
        }
    }

    return 0;
}

static client* window_client ( xcb_window_t win )
{
    if ( win == XCB_WINDOW_NONE ) {
        return NULL;
    }

    int idx = winlist_find ( cache_client, win );

    if ( idx >= 0 ) {
        return cache_client->data[idx];
    }

    // if this fails, we're up that creek
    xcb_get_window_attributes_reply_t *attr = window_get_attributes ( win );

    if ( !attr ) {
        return NULL;
    }
    client *c = g_malloc0 ( sizeof ( client ) );
    c->window = win;

    // copy xattr so we don't have to care when stuff is freed
    memmove ( &c->xattr, attr, sizeof ( xcb_get_window_attributes_reply_t ) );

    xcb_get_property_cookie_t  cky = xcb_ewmh_get_wm_state ( &xcb->ewmh, win );
    xcb_ewmh_get_atoms_reply_t states;
    if ( xcb_ewmh_get_wm_state_reply ( &xcb->ewmh, cky, &states, NULL ) ) {
        c->states = MIN ( CLIENTSTATE, states.atoms_len );
        memcpy ( c->state, states.atoms, MIN ( CLIENTSTATE, states.atoms_len ) * sizeof ( xcb_atom_t ) );
        xcb_ewmh_get_atoms_reply_wipe ( &states );
    }
    cky = xcb_ewmh_get_wm_window_type ( &xcb->ewmh, win );
    if ( xcb_ewmh_get_wm_window_type_reply ( &xcb->ewmh, cky, &states, NULL ) ) {
        c->window_types = MIN ( CLIENTWINDOWTYPE, states.atoms_len );
        memcpy ( c->window_type, states.atoms, MIN ( CLIENTWINDOWTYPE, states.atoms_len ) * sizeof ( xcb_atom_t ) );
        xcb_ewmh_get_atoms_reply_wipe ( &states );
    }

    c->title = window_get_text_prop ( c->window, xcb->ewmh._NET_WM_NAME );
    if ( c->title == NULL ) {
        c->title = window_get_text_prop ( c->window, XCB_ATOM_WM_NAME );
    }

    c->role = window_get_text_prop ( c->window, netatoms[WM_WINDOW_ROLE] );

    cky = xcb_icccm_get_wm_class ( xcb->connection, c->window );
    xcb_icccm_get_wm_class_reply_t wcr;
    if ( xcb_icccm_get_wm_class_reply ( xcb->connection, cky, &wcr, NULL ) ) {
        c->class = rofi_latin_to_utf8_strdup ( wcr.class_name, -1 );
        c->name  = rofi_latin_to_utf8_strdup ( wcr.instance_name, -1 );
        xcb_icccm_get_wm_class_reply_wipe ( &wcr );
    }

    xcb_get_property_cookie_t cc = xcb_icccm_get_wm_hints ( xcb->connection, c->window );
    xcb_icccm_wm_hints_t      r;
    if ( xcb_icccm_get_wm_hints_reply ( xcb->connection, cc, &r, NULL ) ) {
        c->hint_flags = r.flags;
    }

    /** Do UTF-8 Check, should not be needed, does not hurt here to be paranoid. */
    {
        c->title = rofi_force_utf8 ( c->title );
        c->class = rofi_force_utf8 ( c->class );
        c->name  = rofi_force_utf8 ( c->name );
        c->role  = rofi_force_utf8 ( c->role );
    }

    winlist_append ( cache_client, c->window, c );
    g_free ( attr );
    return c;
}

typedef struct
{
    unsigned int id;
    char         **cmd_list;
    unsigned int cmd_list_length;
    winlist      *ids;
    int          config_i3_mode;
    // Current window.
    unsigned int index;
    char         *cache;
} ModeModePrivateData;

static int window_match ( const Mode *sw, GRegex **tokens, unsigned int index )
{
    ModeModePrivateData *rmpd = (ModeModePrivateData *) mode_get_private_data ( sw );
    int                 match = 1;
    const winlist       *ids  = ( winlist * ) rmpd->ids;
    // Want to pull directly out of cache, X calls are not thread safe.
    int                 idx = winlist_find ( cache_client, ids->array[index] );
    g_assert ( idx >= 0 );
    client              *c = cache_client->data[idx];

    if ( tokens ) {
        for ( int j = 0; match && tokens != NULL && tokens[j] != NULL; j++ ) {
            int test = 0;
            // Dirty hack. Normally token_match does _all_ the matching,
            // Now we want it to match only one item at the time.
            // If hack not in place it would not match queries spanning multiple fields.
            // e.g. when searching 'title element' and 'class element'
            GRegex *ftokens[2] = { tokens[j], NULL };
            if ( !test && c->title != NULL && c->title[0] != '\0' ) {
                test = token_match ( ftokens, c->title);
            }

            if ( !test && c->class != NULL && c->class[0] != '\0' ) {
                test = token_match ( ftokens, c->class);
            }

            if ( !test && c->role != NULL && c->role[0] != '\0' ) {
                test = token_match ( ftokens, c->role);
            }

            if ( !test && c->name != NULL && c->name[0] != '\0' ) {
                test = token_match ( ftokens, c->name);
            }

            if ( test == 0 ) {
                match = 0;
            }
        }
    }

    return match;
}

static unsigned int window_mode_get_num_entries ( const Mode *sw )
{
    const ModeModePrivateData *pd = (const ModeModePrivateData *) mode_get_private_data ( sw );
    return pd->cmd_list_length;
}
static void _window_mode_load_data ( Mode *sw, unsigned int cd )
{
    ModeModePrivateData *pd = (ModeModePrivateData *) mode_get_private_data ( sw );
    // find window list
    int                 nwins = 0;
    xcb_window_t        wins[100];
    xcb_window_t        curr_win_id;
    // Create cache

    x11_cache_create ();
    // Check for i3
    pd->config_i3_mode = i3_support_initialize ( xcb );
    xcb_get_property_cookie_t c = xcb_ewmh_get_active_window ( &( xcb->ewmh ), xcb->screen_nbr );
    if ( !xcb_ewmh_get_active_window_reply ( &xcb->ewmh, c, &curr_win_id, NULL ) ) {
        curr_win_id = 0;
    }

    // Get the current desktop.
    unsigned int current_desktop = 0;
    c = xcb_ewmh_get_current_desktop ( &xcb->ewmh, xcb->screen_nbr );
    if ( !xcb_ewmh_get_current_desktop_reply ( &xcb->ewmh, c, &current_desktop, NULL ) ) {
        current_desktop = 0;
    }

    c = xcb_ewmh_get_client_list_stacking ( &xcb->ewmh, 0 );
    xcb_ewmh_get_windows_reply_t clients;
    if ( xcb_ewmh_get_client_list_stacking_reply ( &xcb->ewmh, c, &clients, NULL ) ) {
        nwins = MIN ( 100, clients.windows_len );
        memcpy ( wins, clients.windows, nwins * sizeof ( xcb_window_t ) );
        xcb_ewmh_get_windows_reply_wipe ( &clients );
    }
    else {
        c = xcb_ewmh_get_client_list ( &xcb->ewmh, xcb->screen_nbr );
        if  ( xcb_ewmh_get_client_list_reply ( &xcb->ewmh, c, &clients, NULL ) ) {
            nwins = MIN ( 100, clients.windows_len );
            memcpy ( wins, clients.windows, nwins * sizeof ( xcb_window_t ) );
            xcb_ewmh_get_windows_reply_wipe ( &clients );
        }
    }
    if (  nwins > 0 ) {
        char         pattern[50];
        int          i;
        unsigned int classfield = 0;
        unsigned int desktops   = 0;
        // windows we actually display. May be slightly different to _NET_CLIENT_LIST_STACKING
        // if we happen to have a window destroyed while we're working...
        pd->ids = winlist_new ();

        // calc widths of fields
        for ( i = nwins - 1; i > -1; i-- ) {
            client *c = window_client ( wins[i] );
            if ( ( c != NULL )
                 && !c->xattr.override_redirect
                 && !client_has_window_type ( c, xcb->ewmh._NET_WM_WINDOW_TYPE_DOCK )
                 && !client_has_window_type ( c, xcb->ewmh._NET_WM_WINDOW_TYPE_DESKTOP )
                 && !client_has_state ( c, xcb->ewmh._NET_WM_STATE_SKIP_PAGER )
                 && !client_has_state ( c, xcb->ewmh._NET_WM_STATE_SKIP_TASKBAR ) ) {
                classfield = MAX ( classfield, ( c->class != NULL ) ? ( strlen ( c->class ) ) : 0 );

                if ( client_has_state ( c, xcb->ewmh._NET_WM_STATE_DEMANDS_ATTENTION ) ) {
                    c->demands = TRUE;
                }
                if ( ( c->hint_flags & XCB_ICCCM_WM_HINT_X_URGENCY ) != 0 ) {
                    c->demands = TRUE;
                }

                if ( c->window == curr_win_id ) {
                    c->active = TRUE;
                }
                winlist_append ( pd->ids, c->window, NULL );
            }
        }

        // Create pattern for printing the line.
        xcb_get_property_cookie_t c = xcb_ewmh_get_number_of_desktops ( &xcb->ewmh, xcb->screen_nbr );
        if ( !xcb_ewmh_get_number_of_desktops_reply ( &xcb->ewmh, c, &desktops, NULL ) ) {
            desktops = 1;
        }

        if ( pd->config_i3_mode ) {
            snprintf ( pattern, 50, "%%-%ds   %%s", MAX ( 5, classfield ) );
        }
        else{
            snprintf ( pattern, 50, "%%-%ds  %%-%ds   %%s", desktops < 10 ? 1 : 2,
                       MAX ( 5, classfield ) );
        }
        pd->cmd_list = g_malloc0_n ( ( pd->ids->len + 1 ), sizeof ( char* ) );

        // build the actual list
        for ( i = 0; i < ( pd->ids->len ); i++ ) {
            xcb_window_t w = pd->ids->array[i];
            client       *c;

            if ( ( c = window_client ( w ) ) ) {
                // final line format
                char   desktop[5];
                desktop[0] = 0;
                size_t len =
                    ( ( c->title != NULL ) ? strlen ( c->title ) : 0 ) + ( c->class ? strlen ( c->class ) : 0 ) + classfield + 50;
                char   *line = g_malloc ( len );
                if ( !pd->config_i3_mode ) {
                    uint32_t                  wmdesktop = 0;
                    // find client's desktop.
                    xcb_get_property_cookie_t cookie;
                    xcb_get_property_reply_t  *r;

                    cookie =
                        xcb_get_property ( xcb->connection, 0, c->window, xcb->ewmh._NET_WM_DESKTOP, XCB_ATOM_CARDINAL, 0,
                                           1 );
                    r = xcb_get_property_reply ( xcb->connection, cookie, NULL );
                    if ( r && r->type == XCB_ATOM_CARDINAL ) {
                        wmdesktop = *( (uint32_t *) xcb_get_property_value ( r ) );
                    }
                    if ( r && r->type != XCB_ATOM_CARDINAL ) {
                        // Assume the client is on all desktops.
                        wmdesktop = 0xFFFFFFFF;
                    }
                    else if ( cd && wmdesktop != current_desktop ) {
                        g_free ( line );
                        free ( r );
                        continue;
                    }
                    free ( r );

                    if ( wmdesktop < 0xFFFFFFFF ) {
                        snprintf ( desktop, 5, "%u", (uint32_t) wmdesktop );
                    }

                    snprintf ( line, len, pattern, desktop, c->class ? c->class : "", c->title ? c->title : "" );
                }
                else{
                    snprintf ( line, len, pattern, c->class ? c->class : "", c->title ? c->title : "" );
                }

                pd->cmd_list[pd->cmd_list_length++] = line;
            }
        }
    }
}
static int window_mode_init ( Mode *sw )
{
    if ( mode_get_private_data ( sw ) == NULL ) {
        ModeModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
        mode_set_private_data ( sw, (void *) pd );
        _window_mode_load_data ( sw, FALSE );
    }
    return TRUE;
}
static int window_mode_init_cd ( Mode *sw )
{
    if ( mode_get_private_data ( sw ) == NULL ) {
        ModeModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
        mode_set_private_data ( sw, (void *) pd );
        _window_mode_load_data ( sw, TRUE );
    }
    return TRUE;
}

static inline int act_on_window ( xcb_window_t window )
{
    int  retv   = TRUE;
    char **args = NULL;
    int  argc   = 0;
    char window_str[100]; /* We are probably safe here */

    g_snprintf ( window_str, sizeof window_str, "%d", window );

    helper_parse_setup ( config.window_command, &args, &argc, "{window}", window_str, NULL );

    GError *error = NULL;
    g_spawn_async ( NULL, args, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error );
    if ( error != NULL ) {
        char *msg = g_strdup_printf ( "Failed to execute action for window: '%s'\nError: '%s'", window_str, error->message );
        rofi_view_error_dialog ( msg, FALSE  );
        g_free ( msg );
        // print error.
        g_error_free ( error );
        retv = FALSE;
    }

    // Free the args list.
    g_strfreev ( args );
    return retv;
}

static ModeMode window_mode_result ( Mode *sw, int mretv, G_GNUC_UNUSED char **input,
                                     unsigned int selected_line )
{
    ModeModePrivateData *rmpd = (ModeModePrivateData *) mode_get_private_data ( sw );
    ModeMode            retv  = MODE_EXIT;
    if ( mretv & MENU_NEXT ) {
        retv = NEXT_DIALOG;
    }
    else if ( mretv & MENU_PREVIOUS ) {
        retv = PREVIOUS_DIALOG;
    }
    else if ( ( mretv & MENU_QUICK_SWITCH ) == MENU_QUICK_SWITCH ) {
        retv = ( mretv & MENU_LOWER_MASK );
    }
    else if ( ( mretv & ( MENU_OK ) ) && rmpd->cmd_list[selected_line] ) {
        if ( mretv & MENU_CUSTOM_ACTION ) {
            act_on_window ( rmpd->ids->array[selected_line] );
        }
        else if ( rmpd->config_i3_mode ) {
            // Hack for i3.
            i3_support_focus_window ( rmpd->ids->array[selected_line] );
        }
        else {
            // Get the current desktop.
            unsigned int              current_desktop = 0;
            xcb_get_property_cookie_t c               = xcb_ewmh_get_current_desktop ( &xcb->ewmh, xcb->screen_nbr );
            if ( !xcb_ewmh_get_current_desktop_reply ( &xcb->ewmh, c, &current_desktop, NULL ) ) {
                current_desktop = 0;
            }

            // Get the desktop of the client to switch to
            uint32_t                  wmdesktop = 0;
            xcb_get_property_cookie_t cookie;
            xcb_get_property_reply_t  *r;

            cookie = xcb_get_property ( xcb->connection,
                                        0,
                                        rmpd->ids->array[selected_line],
                                        xcb->ewmh._NET_WM_DESKTOP,
                                        XCB_ATOM_CARDINAL,
                                        0,
                                        1 );
            r = xcb_get_property_reply ( xcb->connection, cookie, NULL );
            if ( r && r->type == XCB_ATOM_CARDINAL ) {
                wmdesktop = *( (uint32_t *) xcb_get_property_value ( r ) );
            }
            if ( r && r->type != XCB_ATOM_CARDINAL ) {
                // Assume the client is on all desktops.
                wmdesktop = current_desktop;
            }
            free ( r );

            // If we have to switch the desktop, do
            if ( wmdesktop != current_desktop ) {
                xcb_ewmh_request_change_current_desktop ( &xcb->ewmh,
                                                          xcb->screen_nbr,
                                                          wmdesktop,
                                                          XCB_CURRENT_TIME );
            }

            // Activate the window
            xcb_ewmh_request_change_active_window ( &xcb->ewmh, xcb->screen_nbr, rmpd->ids->array[selected_line],
                                                    XCB_EWMH_CLIENT_SOURCE_TYPE_OTHER,
                                                    XCB_CURRENT_TIME, XCB_WINDOW_NONE );
            xcb_flush ( xcb->connection );
        }
    }
    else if ( ( mretv & ( MENU_ENTRY_DELETE ) ) == MENU_ENTRY_DELETE ) {
        /* TODO: WM_DELETE_WINDOW support, see i3 */
        xcb_destroy_window ( xcb->connection, rmpd->ids->array[selected_line] );
        xcb_flush ( xcb->connection );
    }
    return retv;
}

static void window_mode_destroy ( Mode *sw )
{
    ModeModePrivateData *rmpd = (ModeModePrivateData *) mode_get_private_data ( sw );
    if ( rmpd != NULL ) {
        g_strfreev ( rmpd->cmd_list );
        winlist_free ( rmpd->ids );
        i3_support_free_internals ();
        x11_cache_free ();
        g_free ( rmpd->cache );
        g_free ( rmpd );
        mode_set_private_data ( sw, NULL );
    }
}

static char *_get_display_value ( const Mode *sw, unsigned int selected_line, int *state, int get_entry )
{
    ModeModePrivateData *rmpd = mode_get_private_data ( sw );
    if ( window_client ( rmpd->ids->array[selected_line] )->demands ) {
        *state |= URGENT;
    }
    if ( window_client ( rmpd->ids->array[selected_line] )->active ) {
        *state |= ACTIVE;
    }
    return get_entry ? g_strdup ( rmpd->cmd_list[selected_line] ) : NULL;
}


#include "mode-private.h"
Mode window_mode =
{
    .name               = "window",
    .cfg_name_key       = "display-window",
    ._init              = window_mode_init,
    ._get_num_entries   = window_mode_get_num_entries,
    ._result            = window_mode_result,
    ._destroy           = window_mode_destroy,
    ._token_match       = window_match,
    ._get_display_value = _get_display_value,
    ._get_completion    = NULL,
    .private_data       = NULL,
    .free               = NULL
};
Mode window_mode_cd =
{
    .name               = "windowcd",
    .cfg_name_key       = "display-windowcd",
    ._init              = window_mode_init_cd,
    ._get_num_entries   = window_mode_get_num_entries,
    ._result            = window_mode_result,
    ._destroy           = window_mode_destroy,
    ._token_match       = window_match,
    ._get_display_value = _get_display_value,
    ._get_completion    = NULL,
    .private_data       = NULL,
    .free               = NULL
};

#endif // WINDOW_MODE

