/**
 * rofi
 *
 * MIT/X11 License
 * Copyright 2013-2017 Qball Cow <qball@gmpclient.org>
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

#include "wayland.h"

#include "rofi.h"
#include "settings.h"
#include "helper.h"
#include "widgets/textbox.h"
#include "x11-helper.h"
#include "dialogs/window.h"

#define WINLIST             32

#define LOG_DOMAIN          "Dialogs.Window"

typedef struct
{
    GRegex *window_regex;
    GArray *entries;
    unsigned int title_len;
    unsigned int app_id_len;
    unsigned int workspace_len;
} ModeModePrivateData;

typedef struct
{
    ModeModePrivateData *pd;
    struct zww_window_switcher_window_v1 *window;
    gchar *title;
    gchar *app_id;
    gchar *workspace;
} ModeWaylandWindow;

static void window_free ( gpointer data )
{
    ModeWaylandWindow *window = data;

    g_free(window->title);
    g_free(window->app_id);
    g_free(window->workspace);
}

static int window_match ( const Mode *sw, GRegex **tokens, unsigned int index )
{
    ModeModePrivateData *rmpd = (ModeModePrivateData *) mode_get_private_data ( sw );
    ModeWaylandWindow *window = &g_array_index(rmpd->entries, ModeWaylandWindow, index);

    if ( ! tokens ) {
        return 1;
    }

    int                 match = 1;
    for ( int j = 0; match && tokens != NULL && tokens[j] != NULL; j++ ) {
        int test = 0;
        // Dirty hack. Normally helper_token_match does _all_ the matching,
        // Now we want it to match only one item at the time.
        // If hack not in place it would not match queries spanning multiple fields.
        // e.g. when searching 'title element' and 'class element'
        GRegex *ftokens[2] = { tokens[j], NULL };
        if ( window->title != NULL ) {
            test = helper_token_match ( ftokens, window->title );
        }

        if ( !test && window->app_id != NULL ) {
            test = helper_token_match ( ftokens, window->app_id );
        }

        if ( !test && window->workspace != NULL ) {
            test = helper_token_match ( ftokens, window->workspace );
        }

        if ( test == 0 ) {
            match = 0;
        }
    }

    return match;
}

static unsigned int window_mode_get_num_entries ( const Mode *sw )
{
    const ModeModePrivateData *pd = (const ModeModePrivateData *) mode_get_private_data ( sw );

    return pd->entries->len;
}

static void window_mode_window_event_title(void *data, struct zww_window_switcher_window_v1 *win, const char *title)
{
    ModeWaylandWindow *window = data;
    gsize l;
    g_assert(window->window == win);

    window->title = g_strdup(title);
    l = strlen(window->title);
    window->pd->title_len = MAX(window->pd->title_len, l);
}

static void window_mode_window_event_app_id(void *data, struct zww_window_switcher_window_v1 *win, const char *app_id)
{
    ModeWaylandWindow *window = data;
    gsize l;
    g_assert(window->window == win);

    window->app_id = g_strdup(app_id);
    l = strlen(window->app_id);
    window->pd->app_id_len = MAX(window->pd->app_id_len, l);
}

static void window_mode_window_event_workspace(void *data, struct zww_window_switcher_window_v1 *win, const char *workspace)
{
    ModeWaylandWindow *window = data;
    gsize l;
    g_assert(window->window == win);

    window->workspace = g_strdup(workspace);
    l = strlen(window->workspace);
    window->pd->workspace_len = MAX(window->pd->workspace_len, l);
}

static void window_mode_window_event_done(void *data, struct zww_window_switcher_window_v1 *win)
{
    ModeWaylandWindow *window = data;
    g_assert(window->window == win);
}

static const struct zww_window_switcher_window_v1_listener wayland_window_switcher_window_listener = {
    .title = window_mode_window_event_title,
    .app_id = window_mode_window_event_app_id,
    .workspace = window_mode_window_event_workspace,
    .done = window_mode_window_event_done,
};

static void window_mode_event_window(void *data, struct zww_window_switcher_v1 *switcher, struct zww_window_switcher_window_v1 *win)
{
    ModeModePrivateData *pd = data;
    g_assert(wayland->window_switcher == switcher);
    ModeWaylandWindow *window, window_ = {
        .pd = pd,
        .window = win,
    };

    pd->entries = g_array_append_val(pd->entries, window_);
    window = &g_array_index(pd->entries, ModeWaylandWindow, pd->entries->len - 1);

    zww_window_switcher_window_v1_add_listener(window->window, &wayland_window_switcher_window_listener, window);
}

static const struct zww_window_switcher_v1_listener wayland_window_switcher_listener = {
    .window = window_mode_event_window,
};

static gboolean _window_mode_load_data ( Mode *sw, unsigned int cd )
{
    ModeModePrivateData *pd = (ModeModePrivateData *) mode_get_private_data ( sw );
    if ( pd != NULL ) {
        return TRUE;
    }

    if ( wayland->global_names[WAYLAND_GLOBAL_WINDOW_SWITCHER] == 0 )
        return FALSE;

    (void)cd;// FIXME: need some more protocol for that

    pd = g_new0 ( ModeModePrivateData, 1 );
    pd->entries = g_array_sized_new(FALSE, FALSE, sizeof(ModeWaylandWindow), WINLIST);
    g_array_set_clear_func(pd->entries, window_free);
    pd->window_regex = g_regex_new ( "{[-\\w]+(:-?[0-9]+)?}", 0, 0, NULL );
    mode_set_private_data ( sw, (void *) pd );

    wayland->window_switcher = wl_registry_bind(wayland->registry, wayland->global_names[WAYLAND_GLOBAL_WINDOW_SWITCHER], &zww_window_switcher_v1_interface, WW_WINDOW_SWITCHER_INTERFACE_VERSION);
    zww_window_switcher_v1_add_listener(wayland->window_switcher, &wayland_window_switcher_listener, pd);
    wl_display_roundtrip(wayland->display);
    return TRUE;
}

static int window_mode_init ( Mode *sw )
{
    return _window_mode_load_data ( sw, FALSE );
}
static int window_mode_init_cd ( Mode *sw )
{
    return _window_mode_load_data ( sw, TRUE );
}

static inline int act_on_window ( ModeWaylandWindow *window )
{
    int  retv   = TRUE;
    char **args = NULL;
    int  argc   = 0;
    char window_regex[100]; /* We are probably safe here */

    helper_parse_setup ( config.window_command, &args, &argc, "{window}", window->title, NULL );

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
    ModeWaylandWindow *window = &g_array_index(rmpd->entries, ModeWaylandWindow, selected_line);
    if ( mretv & MENU_NEXT ) {
        retv = NEXT_DIALOG;
    }
    else if ( mretv & MENU_PREVIOUS ) {
        retv = PREVIOUS_DIALOG;
    }
    else if ( ( mretv & MENU_QUICK_SWITCH ) == MENU_QUICK_SWITCH ) {
        retv = ( mretv & MENU_LOWER_MASK );
    }
    else if ( ( mretv & ( MENU_OK ) ) ) {
        if ( mretv & MENU_CUSTOM_ACTION ) {
            act_on_window ( window );
        }
        else {
            zww_window_switcher_window_v1_switch_to(window->window, wayland->last_seat->seat, wayland->last_seat->serial);
        }
    }
    else if ( ( mretv & ( MENU_ENTRY_DELETE ) ) == MENU_ENTRY_DELETE ) {
        zww_window_switcher_window_v1_close(window->window, wayland->last_seat->seat, wayland->last_seat->serial);
    }
    return retv;
}

static void window_mode_destroy ( Mode *sw )
{
    ModeModePrivateData *rmpd = (ModeModePrivateData *) mode_get_private_data ( sw );
    if ( rmpd != NULL ) {
        g_array_free(rmpd->entries, TRUE);
        g_regex_unref ( rmpd->window_regex );
        g_free ( rmpd );
        mode_set_private_data ( sw, NULL );
    }
}
struct arg
{
    const ModeModePrivateData *pd;
    ModeWaylandWindow *window;
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
            helper_eval_add_str ( str, d->window->workspace, l, d->pd->workspace_len );
        }
        else if ( match[1] == 'c' ) {
            helper_eval_add_str ( str, d->window->app_id, l, d->pd->app_id_len );
        }
        else if ( match[1] == 't' ) {
            helper_eval_add_str ( str, d->window->title, l, d->pd->title_len );
        }
        g_free ( match );
    }
    return FALSE;
}
static char * _generate_display_string ( const ModeModePrivateData *pd, ModeWaylandWindow *window )
{
    struct arg d    = { pd, window };
    char       *res = g_regex_replace_eval ( pd->window_regex, config.window_format, -1, 0, 0,
                                             helper_eval_cb, &d, NULL );
    return g_strchomp ( res );
}

static char *_get_display_value ( const Mode *sw, unsigned int selected_line, int *state, G_GNUC_UNUSED GList **list, int get_entry )
{
    ModeModePrivateData *rmpd = mode_get_private_data ( sw );
    ModeWaylandWindow *window = &g_array_index(rmpd->entries, ModeWaylandWindow, selected_line);
    /*
    if ( c->demands ) {
        *state |= URGENT;
    }
    if ( c->active ) {
        *state |= ACTIVE;
    }
    */
    (void)state;
    return get_entry ? _generate_display_string ( rmpd, window ) : NULL;
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
    ._get_completion    = NULL,
    ._preprocess_input  = NULL,
    .private_data       = NULL,
    .free               = NULL
};

#endif // WINDOW_MODE
