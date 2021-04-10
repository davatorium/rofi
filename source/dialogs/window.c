/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2020 Qball Cow <qball@gmpclient.org>
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

/** The log domain of this dialog. */
#define G_LOG_DOMAIN    "Dialogs.Window"

#include <config.h>

#ifdef WINDOW_MODE

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
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
#include "widgets/textbox.h"
#include "dialogs/window.h"

#include "timings.h"

#include "rofi-icon-fetcher.h"

#define WINLIST             32

#define CLIENTSTATE         10
#define CLIENTWINDOWTYPE    10

// Fields to match in window mode
typedef struct
{
    char     *field_name;
    gboolean enabled;
} WinModeField;

typedef enum
{
    WIN_MATCH_FIELD_TITLE,
    WIN_MATCH_FIELD_CLASS,
    WIN_MATCH_FIELD_ROLE,
    WIN_MATCH_FIELD_NAME,
    WIN_MATCH_FIELD_DESKTOP,
    WIN_MATCH_NUM_FIELDS,
} WinModeMatchingFields;

static WinModeField matching_window_fields[WIN_MATCH_NUM_FIELDS] = {
    { .field_name = "title",   .enabled = TRUE, },
    { .field_name = "class",   .enabled = TRUE, },
    { .field_name = "role",    .enabled = TRUE, },
    { .field_name = "name",    .enabled = TRUE, },
    { .field_name = "desktop", .enabled = TRUE, }
};

static gboolean     window_matching_fields_parsed = FALSE;

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
    uint32_t                          wmdesktop;
    char                              *wmdesktopstr;
    cairo_surface_t                   *icon;
    gboolean                          icon_checked;
    uint32_t                          icon_fetch_uid;
    gboolean                          thumbnail_checked;
} client;

// window lists
typedef struct
{
    xcb_window_t *array;
    client       **data;
    int          len;
} winlist;

typedef struct
{
    unsigned int id;
    winlist      *ids;
    // Current window.
    unsigned int index;
    char         *cache;
    unsigned int wmdn_len;
    unsigned int clf_len;
    unsigned int name_len;
    unsigned int title_len;
    unsigned int role_len;
    GRegex       *window_regex;
} ModeModePrivateData;

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
            if ( c->icon ) {
                cairo_surface_destroy ( c->icon );
            }
            g_free ( c->title );
            g_free ( c->class );
            g_free ( c->name );
            g_free ( c->role );
            g_free ( c->wmdesktopstr );
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

static client* window_client ( ModeModePrivateData *pd, xcb_window_t win )
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
    pd->title_len = MAX ( c->title ? g_utf8_strlen ( c->title, -1 ) : 0, pd->title_len );

    c->role      = window_get_text_prop ( c->window, netatoms[WM_WINDOW_ROLE] );
    pd->role_len = MAX ( c->role ? g_utf8_strlen ( c->role, -1 ) : 0, pd->role_len );

    cky = xcb_icccm_get_wm_class ( xcb->connection, c->window );
    xcb_icccm_get_wm_class_reply_t wcr;
    if ( xcb_icccm_get_wm_class_reply ( xcb->connection, cky, &wcr, NULL ) ) {
        c->class     = rofi_latin_to_utf8_strdup ( wcr.class_name, -1 );
        c->name      = rofi_latin_to_utf8_strdup ( wcr.instance_name, -1 );
        pd->name_len = MAX ( c->name ? g_utf8_strlen ( c->name, -1 ) : 0, pd->name_len );
        xcb_icccm_get_wm_class_reply_wipe ( &wcr );
    }

    xcb_get_property_cookie_t cc = xcb_icccm_get_wm_hints ( xcb->connection, c->window );
    xcb_icccm_wm_hints_t      r;
    if ( xcb_icccm_get_wm_hints_reply ( xcb->connection, cc, &r, NULL ) ) {
        c->hint_flags = r.flags;
    }

    winlist_append ( cache_client, c->window, c );
    g_free ( attr );
    return c;
}
static int window_match ( const Mode *sw, rofi_int_matcher **tokens, unsigned int index )
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
            // Dirty hack. Normally helper_token_match does _all_ the matching,
            // Now we want it to match only one item at the time.
            // If hack not in place it would not match queries spanning multiple fields.
            // e.g. when searching 'title element' and 'class element'
            rofi_int_matcher *ftokens[2] = { tokens[j], NULL };
            if ( c->title != NULL && c->title[0] != '\0' && matching_window_fields[WIN_MATCH_FIELD_TITLE].enabled ) {
                test = helper_token_match ( ftokens, c->title );
            }

            if ( test == tokens[j]->invert && c->class != NULL && c->class[0] != '\0' && matching_window_fields[WIN_MATCH_FIELD_CLASS].enabled ) {
                test = helper_token_match ( ftokens, c->class );
            }

            if ( test == tokens[j]->invert && c->role != NULL && c->role[0] != '\0' && matching_window_fields[WIN_MATCH_FIELD_ROLE].enabled ) {
                test = helper_token_match ( ftokens, c->role );
            }

            if ( test == tokens[j]->invert && c->name != NULL && c->name[0] != '\0' && matching_window_fields[WIN_MATCH_FIELD_NAME].enabled ) {
                test = helper_token_match ( ftokens, c->name );
            }
            if ( test == tokens[j]->invert && c->wmdesktopstr != NULL && c->wmdesktopstr[0] != '\0' && matching_window_fields[WIN_MATCH_FIELD_DESKTOP].enabled ) {
                test = helper_token_match ( ftokens, c->wmdesktopstr );
            }

            if ( test == 0 ) {
                match = 0;
            }
        }
    }

    return match;
}

static void window_mode_parse_fields ()
{
    window_matching_fields_parsed = TRUE;
    char               *savept = NULL;
    // Make a copy, as strtok will modify it.
    char               *switcher_str = g_strdup ( config.window_match_fields );
    const char * const sep           = ",#";
    // Split token on ','. This modifies switcher_str.
    for ( unsigned int i = 0; i < WIN_MATCH_NUM_FIELDS; i++ ) {
        matching_window_fields[i].enabled = FALSE;
    }
    for ( char *token = strtok_r ( switcher_str, sep, &savept ); token != NULL;
          token = strtok_r ( NULL, sep, &savept ) ) {
        if ( strcmp ( token, "all" ) == 0 ) {
            for ( unsigned int i = 0; i < WIN_MATCH_NUM_FIELDS; i++ ) {
                matching_window_fields[i].enabled = TRUE;
            }
            break;
        }
        else {
            gboolean matched = FALSE;
            for ( unsigned int i = 0; i < WIN_MATCH_NUM_FIELDS; i++ ) {
                const char * field_name = matching_window_fields[i].field_name;
                if ( strcmp ( token, field_name ) == 0 ) {
                    matching_window_fields[i].enabled = TRUE;
                    matched                           = TRUE;
                }
            }
            if ( !matched ) {
                g_warning ( "Invalid window field name :%s", token );
            }
        }
    }
    // Free string that was modified by strtok_r
    g_free ( switcher_str );
}

static unsigned int window_mode_get_num_entries ( const Mode *sw )
{
    const ModeModePrivateData *pd = (const ModeModePrivateData *) mode_get_private_data ( sw );

    return pd->ids ? pd->ids->len : 0;
}
/**
 * Small helper function to find the right entry in the ewmh reply.
 * Is there a call for this?
 */
const char *invalid_desktop_name = "n/a";
static const char * _window_name_list_entry  ( const char *str, uint32_t length, int entry )
{
    uint32_t offset = 0;
    int      index  = 0;
    while ( index < entry && offset < length ) {
        if ( str[offset] == 0 ) {
            index++;
        }
        offset++;
    }
    if ( offset >= length ) {
        return invalid_desktop_name;
    }
    return &str[offset];
}
static void _window_mode_load_data ( Mode *sw, unsigned int cd )
{
    ModeModePrivateData *pd = (ModeModePrivateData *) mode_get_private_data ( sw );
    // find window list
    xcb_window_t        curr_win_id;
    int                 found = 0;

    // Create cache

    x11_cache_create ();
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

    c = xcb_ewmh_get_client_list_stacking ( &xcb->ewmh, xcb->screen_nbr );
    xcb_ewmh_get_windows_reply_t clients = { 0, };
    if ( xcb_ewmh_get_client_list_stacking_reply ( &xcb->ewmh, c, &clients, NULL ) ) {
        found = 1;
    }
    else {
        c = xcb_ewmh_get_client_list ( &xcb->ewmh, xcb->screen_nbr );
        if  ( xcb_ewmh_get_client_list_reply ( &xcb->ewmh, c, &clients, NULL ) ) {
            found = 1;
        }
    }
    if ( !found ) {
        return;
    }

    if (  clients.windows_len > 0 ) {
        int i;
        // windows we actually display. May be slightly different to _NET_CLIENT_LIST_STACKING
        // if we happen to have a window destroyed while we're working...
        pd->ids = winlist_new ();

        xcb_get_property_cookie_t         c = xcb_ewmh_get_desktop_names ( &xcb->ewmh, xcb->screen_nbr );
        xcb_ewmh_get_utf8_strings_reply_t names;
        int                               has_names = FALSE;
        if ( xcb_ewmh_get_desktop_names_reply ( &xcb->ewmh, c, &names, NULL ) ) {
            has_names = TRUE;
        }
        // calc widths of fields
        for ( i = clients.windows_len - 1; i > -1; i-- ) {
            client *c = window_client ( pd, clients.windows[i] );
            if ( ( c != NULL )
                 && !c->xattr.override_redirect
                 && !client_has_window_type ( c, xcb->ewmh._NET_WM_WINDOW_TYPE_DOCK )
                 && !client_has_window_type ( c, xcb->ewmh._NET_WM_WINDOW_TYPE_DESKTOP )
                 && !client_has_state ( c, xcb->ewmh._NET_WM_STATE_SKIP_PAGER )
                 && !client_has_state ( c, xcb->ewmh._NET_WM_STATE_SKIP_TASKBAR ) ) {
                pd->clf_len = MAX ( pd->clf_len, ( c->class != NULL ) ? ( g_utf8_strlen ( c->class, -1 ) ) : 0 );

                if ( client_has_state ( c, xcb->ewmh._NET_WM_STATE_DEMANDS_ATTENTION ) ) {
                    c->demands = TRUE;
                }
                if ( ( c->hint_flags & XCB_ICCCM_WM_HINT_X_URGENCY ) != 0 ) {
                    c->demands = TRUE;
                }

                if ( c->window == curr_win_id ) {
                    c->active = TRUE;
                }
                // find client's desktop.
                xcb_get_property_cookie_t cookie;
                xcb_get_property_reply_t  *r;

                c->wmdesktop = 0xFFFFFFFF;
                cookie       =
                    xcb_get_property ( xcb->connection, 0, c->window, xcb->ewmh._NET_WM_DESKTOP, XCB_ATOM_CARDINAL, 0,
                                       1 );
                r = xcb_get_property_reply ( xcb->connection, cookie, NULL );
                if ( r ) {
                    if ( r->type == XCB_ATOM_CARDINAL ) {
                        c->wmdesktop = *( (uint32_t *) xcb_get_property_value ( r ) );
                    }
                    free ( r );
                }
                if ( c->wmdesktop != 0xFFFFFFFF ) {
                    if ( has_names ) {
                        if ( ( current_window_manager & WM_PANGO_WORKSPACE_NAMES ) == WM_PANGO_WORKSPACE_NAMES ) {
                            char *output = NULL;
                            if ( pango_parse_markup ( _window_name_list_entry ( names.strings, names.strings_len,
                                                                                c->wmdesktop ), -1, 0, NULL, &output, NULL, NULL ) ) {
                                c->wmdesktopstr = output;
                            }
                            else {
                                c->wmdesktopstr = g_strdup ( "Invalid name" );
                            }
                        }
                        else {
                            c->wmdesktopstr = g_strdup ( _window_name_list_entry ( names.strings, names.strings_len, c->wmdesktop ) );
                        }
                    }
                    else {
                        c->wmdesktopstr = g_strdup_printf ( "%u", (uint32_t) c->wmdesktop );
                    }
                }
                else {
                    c->wmdesktopstr = g_strdup ( "" );
                }
                pd->wmdn_len = MAX ( pd->wmdn_len, g_utf8_strlen ( c->wmdesktopstr, -1 ) );
                if ( cd && c->wmdesktop != current_desktop ) {
                    continue;
                }
                winlist_append ( pd->ids, c->window, NULL );
            }
        }

        if ( has_names ) {
            xcb_ewmh_get_utf8_strings_reply_wipe ( &names );
        }
    }
    xcb_ewmh_get_windows_reply_wipe ( &clients );
}
static int window_mode_init ( Mode *sw )
{
    if ( mode_get_private_data ( sw ) == NULL ) {
        ModeModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
        pd->window_regex = g_regex_new ( "{[-\\w]+(:-?[0-9]+)?}", 0, 0, NULL );
        mode_set_private_data ( sw, (void *) pd );
        _window_mode_load_data ( sw, FALSE );
        if ( !window_matching_fields_parsed ) {
            window_mode_parse_fields ();
        }
    }
    return TRUE;
}
static int window_mode_init_cd ( Mode *sw )
{
    if ( mode_get_private_data ( sw ) == NULL ) {
        ModeModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
        pd->window_regex = g_regex_new ( "{[-\\w]+(:-?[0-9]+)?}", 0, 0, NULL );
        mode_set_private_data ( sw, (void *) pd );
        _window_mode_load_data ( sw, TRUE );
        if ( !window_matching_fields_parsed ) {
            window_mode_parse_fields ();
        }
    }
    return TRUE;
}

static inline int act_on_window ( xcb_window_t window )
{
    int  retv   = TRUE;
    char **args = NULL;
    int  argc   = 0;
    char window_regex[100]; /* We are probably safe here */

    g_snprintf ( window_regex, sizeof window_regex, "%d", window );

    helper_parse_setup ( config.window_command, &args, &argc, "{window}", window_regex, (char *) 0 );

    GError *error = NULL;
    g_spawn_async ( NULL, args, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error );
    if ( error != NULL ) {
        char *msg = g_strdup_printf ( "Failed to execute action for window: '%s'\nError: '%s'", window_regex, error->message );
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
    if ( ( mretv & ( MENU_OK ) ) ) {
        if ( mretv & MENU_CUSTOM_ACTION ) {
            act_on_window ( rmpd->ids->array[selected_line] );
        }
        else {
            // Disable reverting input focus to previous window.
            xcb->focus_revert = 0;
            rofi_view_hide ();
            if ( ( current_window_manager & WM_DO_NOT_CHANGE_CURRENT_DESKTOP ) == 0 ) {
                // Get the desktop of the client to switch to
                uint32_t                  wmdesktop = 0;
                xcb_get_property_cookie_t cookie;
                xcb_get_property_reply_t  *r;
                // Get the current desktop.
                unsigned int              current_desktop = 0;
                xcb_get_property_cookie_t c               = xcb_ewmh_get_current_desktop ( &xcb->ewmh, xcb->screen_nbr );
                if ( !xcb_ewmh_get_current_desktop_reply ( &xcb->ewmh, c, &current_desktop, NULL ) ) {
                    current_desktop = 0;
                }

                cookie = xcb_get_property ( xcb->connection, 0, rmpd->ids->array[selected_line],
                                            xcb->ewmh._NET_WM_DESKTOP, XCB_ATOM_CARDINAL, 0, 1 );
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
                                                              xcb->screen_nbr, wmdesktop, XCB_CURRENT_TIME );
                }
            }
            // Activate the window
            xcb_ewmh_request_change_active_window ( &xcb->ewmh, xcb->screen_nbr, rmpd->ids->array[selected_line],
                                                    XCB_EWMH_CLIENT_SOURCE_TYPE_OTHER,
                                                    XCB_CURRENT_TIME, rofi_view_get_window () );
            xcb_flush ( xcb->connection );
        }
    }
    else if ( ( mretv & ( MENU_ENTRY_DELETE ) ) == MENU_ENTRY_DELETE ) {
        xcb_ewmh_request_close_window ( &( xcb->ewmh ), xcb->screen_nbr, rmpd->ids->array[selected_line], XCB_CURRENT_TIME, XCB_EWMH_CLIENT_SOURCE_TYPE_OTHER );
        xcb_flush ( xcb->connection );
    }
    else if ( ( mretv & MENU_CUSTOM_INPUT ) && *input != NULL && *input[0] != '\0' ) {
        GError   *error      = NULL;
        gboolean run_in_term = ( ( mretv & MENU_CUSTOM_ACTION ) == MENU_CUSTOM_ACTION );
        gsize    lf_cmd_size = 0;
        gchar    *lf_cmd     = g_locale_from_utf8 ( *input, -1, NULL, &lf_cmd_size, &error );
        if ( error != NULL ) {
            g_warning ( "Failed to convert command to locale encoding: %s", error->message );
            g_error_free ( error );
            return RELOAD_DIALOG;
        }

        RofiHelperExecuteContext context = { .name = NULL };
        if (  !helper_execute_command ( NULL, lf_cmd, run_in_term, run_in_term ? &context : NULL ) ) {
            retv = RELOAD_DIALOG;
        }
        g_free ( lf_cmd );
    } else if ( mretv & MENU_CUSTOM_COMMAND ) {
        retv = ( mretv & MENU_LOWER_MASK );
    }
    return retv;
}

static void window_mode_destroy ( Mode *sw )
{
    ModeModePrivateData *rmpd = (ModeModePrivateData *) mode_get_private_data ( sw );
    if ( rmpd != NULL ) {
        winlist_free ( rmpd->ids );
        x11_cache_free ();
        g_free ( rmpd->cache );
        g_regex_unref ( rmpd->window_regex );
        g_free ( rmpd );
        mode_set_private_data ( sw, NULL );
    }
}
struct arg
{
    const ModeModePrivateData *pd;
    client                    *c;
};

static void helper_eval_add_str ( GString *str, const char *input, int l, int max_len )
{
    // g_utf8 does not work with NULL string.
    const char *input_nn = input ? input : "";
    // Both l and max_len are in characters, not bytes.
    int        nc     = g_utf8_strlen ( input_nn, -1 );
    int        spaces = 0;
    if ( l == 0 ) {
        spaces = MAX ( 0, max_len - nc );
        g_string_append ( str, input_nn );
    }
    else {
        if ( nc > l ) {
            int bl = g_utf8_offset_to_pointer ( input_nn, l ) - input_nn;
            g_string_append_len ( str, input_nn, bl );
        }
        else {
            spaces = l - nc;
            g_string_append ( str, input_nn );
        }
    }
    while ( spaces-- ) {
        g_string_append_c ( str, ' ' );
    }
}
static gboolean helper_eval_cb ( const GMatchInfo *info, GString *str, gpointer data )
{
    struct arg *d = (struct arg *) data;
    gchar      *match;
    // Get the match
    match = g_match_info_fetch ( info, 0 );
    if ( match != NULL ) {
        int l = 0;
        if ( match[2] == ':' ) {
            l = (int) g_ascii_strtoll ( &match[3], NULL, 10 );
            if ( l < 0 && config.menu_width < 0 ) {
                l = -config.menu_width + l;
            }
            if ( l < 0 ) {
                l = 0;
            }
        }
        if ( match[1] == 'w' ) {
            helper_eval_add_str ( str, d->c->wmdesktopstr, l, d->pd->wmdn_len );
        }
        else if ( match[1] == 'c' ) {
            helper_eval_add_str ( str, d->c->class, l, d->pd->clf_len );
        }
        else if ( match[1] == 't' ) {
            helper_eval_add_str ( str, d->c->title, l, d->pd->title_len );
        }
        else if ( match[1] == 'n' ) {
            helper_eval_add_str ( str, d->c->name, l, d->pd->name_len );
        }
        else if ( match[1] == 'r' ) {
            helper_eval_add_str ( str, d->c->role, l, d->pd->role_len );
        }
        g_free ( match );
    }
    return FALSE;
}
static char * _generate_display_string ( const ModeModePrivateData *pd, client *c )
{
    struct arg d    = { pd, c };
    char       *res = g_regex_replace_eval ( pd->window_regex, config.window_format, -1, 0, 0,
                                             helper_eval_cb, &d, NULL );
    return g_strchomp ( res );
}

static char *_get_display_value ( const Mode *sw, unsigned int selected_line, int *state, G_GNUC_UNUSED GList **list, int get_entry )
{
    ModeModePrivateData *rmpd = mode_get_private_data ( sw );
    client              *c    = window_client ( rmpd, rmpd->ids->array[selected_line] );
    if ( c == NULL ) {
        return get_entry ? g_strdup ( "Window has fanished" ) : NULL;
    }
    if ( c->demands ) {
        *state |= URGENT;
    }
    if ( c->active ) {
        *state |= ACTIVE;
    }
    return get_entry ? _generate_display_string ( rmpd, c ) : NULL;
}

/**
 * Icon code borrowed from https://github.com/olejorgenb/extract-window-icon
 */
static cairo_user_data_key_t data_key;

/** Create a surface object from this image data.
 * \param width The width of the image.
 * \param height The height of the image
 * \param data The image's data in ARGB format, will be copied by this function.
 */
static cairo_surface_t * draw_surface_from_data ( int width, int height, uint32_t *data )
{
    unsigned long int len = width * height;
    unsigned long int i;
    uint32_t          *buffer = g_new0 ( uint32_t, len );
    cairo_surface_t   *surface;

    /* Cairo wants premultiplied alpha, meh :( */
    for ( i = 0; i < len; i++ ) {
        uint8_t a     = ( data[i] >> 24 ) & 0xff;
        double  alpha = a / 255.0;
        uint8_t r     = ( ( data[i] >> 16 ) & 0xff ) * alpha;
        uint8_t g     = ( ( data[i] >> 8 ) & 0xff ) * alpha;
        uint8_t b     = ( ( data[i] >> 0 ) & 0xff ) * alpha;
        buffer[i] = ( a << 24 ) | ( r << 16 ) | ( g << 8 ) | b;
    }

    surface = cairo_image_surface_create_for_data ( (unsigned char *) buffer,
                                                    CAIRO_FORMAT_ARGB32,
                                                    width,
                                                    height,
                                                    width * 4 );
    /* This makes sure that buffer will be freed */
    cairo_surface_set_user_data ( surface, &data_key, buffer, g_free );

    return surface;
}
static cairo_surface_t * ewmh_window_icon_from_reply ( xcb_get_property_reply_t *r, uint32_t preferred_size )
{
    uint32_t *data, *end, *found_data = 0;
    uint32_t found_size = 0;

    if ( !r || r->type != XCB_ATOM_CARDINAL || r->format != 32 || r->length < 2 ) {
        return 0;
    }

    data = (uint32_t *) xcb_get_property_value ( r );
    if ( !data ) {
        return 0;
    }

    end = data + r->length;

    /* Goes over the icon data and picks the icon that best matches the size preference.
     * In case the size match is not exact, picks the closest bigger size if present,
     * closest smaller size otherwise.
     */
    while ( data + 1 < end ) {
        /* check whether the data size specified by width and height fits into the array we got */
        uint64_t data_size = (uint64_t) data[0] * data[1];
        if ( data_size > (uint64_t) ( end - data - 2 ) ) {
            break;
        }

        /* use the greater of the two dimensions to match against the preferred size */
        uint32_t size = MAX ( data[0], data[1] );

        /* pick the icon if it's a better match than the one we already have */
        gboolean found_icon_too_small   = found_size < preferred_size;
        gboolean found_icon_too_large   = found_size > preferred_size;
        gboolean icon_empty             = data[0] == 0 || data[1] == 0;
        gboolean better_because_bigger  = found_icon_too_small && size > found_size;
        gboolean better_because_smaller = found_icon_too_large &&
                                          size >= preferred_size && size < found_size;
        if ( !icon_empty && ( better_because_bigger || better_because_smaller || found_size == 0 ) ) {
            found_data = data;
            found_size = size;
        }

        data += data_size + 2;
    }

    if ( !found_data ) {
        return 0;
    }

    return draw_surface_from_data ( found_data[0], found_data[1], found_data + 2 );
}
/** Get NET_WM_ICON. */
static cairo_surface_t * get_net_wm_icon ( xcb_window_t xid, uint32_t preferred_size )
{
    xcb_get_property_cookie_t cookie = xcb_get_property_unchecked (
        xcb->connection, FALSE, xid,
        xcb->ewmh._NET_WM_ICON, XCB_ATOM_CARDINAL, 0, UINT32_MAX );
    xcb_get_property_reply_t *r       = xcb_get_property_reply ( xcb->connection, cookie, NULL );
    cairo_surface_t          *surface = ewmh_window_icon_from_reply ( r, preferred_size );
    free ( r );
    return surface;
}
static cairo_surface_t *_get_icon ( const Mode *sw, unsigned int selected_line, int size )
{
    ModeModePrivateData *rmpd = mode_get_private_data ( sw );
    client              *c    = window_client ( rmpd, rmpd->ids->array[selected_line] );
    if ( config.window_thumbnail && c->thumbnail_checked == FALSE ) {
        c->icon              = x11_helper_get_screenshot_surface_window ( c->window, size );
        c->thumbnail_checked = TRUE;
    }
    if ( c->icon == NULL && c->icon_checked == FALSE ) {
        c->icon         = get_net_wm_icon ( rmpd->ids->array[selected_line], size );
        c->icon_checked = TRUE;
    }
    if ( c->icon == NULL && c->class ) {
        if ( c->icon_fetch_uid > 0 ) {
            return rofi_icon_fetcher_get ( c->icon_fetch_uid );
        }
        c->icon_fetch_uid = rofi_icon_fetcher_query ( c->class, size );
        return rofi_icon_fetcher_get ( c->icon_fetch_uid );
    }
    return c->icon;
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
    ._get_icon          = _get_icon,
    ._get_completion    = NULL,
    ._preprocess_input  = NULL,
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
    ._get_icon          = _get_icon,
    ._get_completion    = NULL,
    ._preprocess_input  = NULL,
    .private_data       = NULL,
    .free               = NULL
};

#endif // WINDOW_MODE
