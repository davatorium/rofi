/*
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

#include <string.h>
#include <glib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xkb.h>
#include <cairo.h>
#include <cairo-xcb.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-x11.h>
#include <libgwater-xcb.h>

#include "view.h"
#include "xkb.h"
#include "display.h"
#include "helper.h"
#include "xcb.h"

#define LOG_DOMAIN "Xcb"

static const char *netatom_names[] = { EWMH_ATOMS ( ATOM_CHAR ) };

static xcb_stuff xcb_;
xcb_stuff *xcb = &xcb_;

struct _display_buffer_pool {
    int dummy;
};

static void x11_create_frequently_used_atoms ( void )
{
    // X atom values
    for ( int i = 0; i < NUM_NETATOMS; i++ ) {
        xcb_intern_atom_cookie_t cc = xcb_intern_atom ( xcb->connection, 0, strlen ( netatom_names[i] ), netatom_names[i] );
        xcb_intern_atom_reply_t  *r = xcb_intern_atom_reply ( xcb->connection, cc, NULL );
        if ( r ) {
            xcb->netatoms[i] = r->atom;
            free ( r );
        }
    }
}

static void x11_create_visual_and_colormap ( void )
{
    xcb_depth_t          *root_depth = NULL;
    xcb_depth_iterator_t depth_iter;
    for ( depth_iter = xcb_screen_allowed_depths_iterator ( xcb->screen ); depth_iter.rem; xcb_depth_next ( &depth_iter ) ) {
        xcb_depth_t               *d = depth_iter.data;

        xcb_visualtype_iterator_t visual_iter;
        for ( visual_iter = xcb_depth_visuals_iterator ( d ); visual_iter.rem; xcb_visualtype_next ( &visual_iter ) ) {
            xcb_visualtype_t *v = visual_iter.data;
            if ( ( v->bits_per_rgb_value == 8 ) && ( d->depth == 32 ) && ( v->_class == XCB_VISUAL_CLASS_TRUE_COLOR ) ) {
                xcb->depth  = d;
                xcb->visual = v;
            }
            if ( xcb->screen->root_visual == v->visual_id ) {
                root_depth  = d;
                xcb->root_visual = v;
            }
        }
    }
    if ( xcb->visual != NULL ) {
        xcb_void_cookie_t   c;
        xcb_generic_error_t *e;
        xcb->map = xcb_generate_id ( xcb->connection );
        c   = xcb_create_colormap_checked ( xcb->connection, XCB_COLORMAP_ALLOC_NONE, xcb->map, xcb->screen->root, xcb->visual->visual_id );
        e   = xcb_request_check ( xcb->connection, c );
        if ( e ) {
            xcb->depth  = NULL;
            xcb->visual = NULL;
            free ( e );
        }
    }

    if ( xcb->visual == NULL ) {
        xcb->depth  = root_depth;
        xcb->visual = xcb->root_visual;
        xcb->map    = xcb->screen->default_colormap;
    }
}

static void x11_helper_discover_window_manager ( void )
{
    xcb_window_t              wm_win = 0;
    xcb_get_property_cookie_t cc     = xcb_ewmh_get_supporting_wm_check_unchecked ( &xcb->ewmh, xcb->screen->root );

    if ( xcb_ewmh_get_supporting_wm_check_reply ( &xcb->ewmh, cc, &wm_win, NULL ) ) {
        xcb_ewmh_get_utf8_strings_reply_t wtitle;
        xcb_get_property_cookie_t         cookie = xcb_ewmh_get_wm_name_unchecked ( &( xcb->ewmh ), wm_win );
        if (  xcb_ewmh_get_wm_name_reply ( &( xcb->ewmh ), cookie, &wtitle, (void *) 0 ) ) {
            if ( wtitle.strings_len > 0 ) {
                g_log ( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Found window manager: %s", wtitle.strings );
                if ( g_strcmp0 ( wtitle.strings, "i3" ) == 0 ) {
                    xcb->wm = WM_I3;
                }
                else if  ( g_strcmp0 ( wtitle.strings, "awesome" ) == 0 ) {
                    xcb->wm = WM_AWESOME;
                }
                else if  ( g_strcmp0 ( wtitle.strings, "Openbox" ) == 0 ) {
                    xcb->wm = WM_OPENBOX;
                }
            }
            xcb_ewmh_get_utf8_strings_reply_wipe ( &wtitle );
        }
    }
}

static gboolean xcb_callback(xcb_generic_event_t *ev, G_GNUC_UNUSED gpointer user_data)
{
    if ( ev == NULL ) {
        int status = xcb_connection_has_error ( xcb->connection );
        fprintf ( stderr, "The XCB connection to X server had a fatal error: %d\n", status );
        g_main_loop_quit ( xcb->main_loop );
        return G_SOURCE_REMOVE;
    }
    uint8_t type = ev->response_type & ~0x80;
    if ( type == xcb->xkb_first_event ) {
        switch ( ev->pad0 )
        {
        case XCB_XKB_MAP_NOTIFY:
            xkb_state_unref ( xcb->xkb.state );
            xkb_keymap_unref ( xcb->xkb.keymap );
            xcb->xkb.keymap = xkb_x11_keymap_new_from_device ( xcb->xkb.context, xcb->connection, xcb->xkb_device_id, 0 );
            xcb->xkb.state  = xkb_x11_state_new_from_device ( xcb->xkb.keymap, xcb->connection, xcb->xkb_device_id );
            break;
        case XCB_XKB_STATE_NOTIFY:
        {
            xcb_xkb_state_notify_event_t *ksne = (xcb_xkb_state_notify_event_t *) ev;
            guint                        modmask;
            xkb_state_update_mask ( xcb->xkb.state,
                                    ksne->baseMods,
                                    ksne->latchedMods,
                                    ksne->lockedMods,
                                    ksne->baseGroup,
                                    ksne->latchedGroup,
                                    ksne->lockedGroup );
            modmask = xkb_get_modmask ( &xcb->xkb, XKB_KEY_NoSymbol );
            if ( modmask == 0 ) {
                abe_trigger_release ( );

                // Because of abe_trigger, state of rofi can be changed. handle this!
                rofi_view_maybe_update();
            }
            break;
        }
        }
        return G_SOURCE_CONTINUE;
    }

    RofiViewState *state = rofi_view_get_active ();
    if ( state == NULL ) {
        return G_SOURCE_CONTINUE;
    }

    switch ( ev->response_type & ~0x80 )
    {
    case XCB_CONFIGURE_NOTIFY:
    {
        xcb_configure_notify_event_t *xce = (xcb_configure_notify_event_t *) ev;
        if ( xce->window == xcb->main_window ) {
            /* FIXME: handle this
            if ( state->x != xce->x || state->y != xce->y ) {
                state->x = xce->x;
                state->y = xce->y;
                widget_queue_redraw ( WIDGET ( state->main_window ) );
            }
            if ( state->width != xce->width || state->height != xce->height ) {
                state->width  = xce->width;
                state->height = xce->height;

                xcb_free_pixmap ( xcb->connection, CacheState.edit_pixmap );
                CacheState.edit_pixmap = xcb_generate_id ( xcb->connection );
                xcb_create_pixmap ( xcb->connection, xcb->depth->depth, CacheState.edit_pixmap, CacheState.main_window,
                                    state->width, state->height );

                g_log ( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Re-size window based external request: %d %d\n", state->width, state->height );
                widget_resize ( WIDGET ( state->main_window ), state->width, state->height );
            }
            */
        }
        break;
    }
    case XCB_MOTION_NOTIFY:
    {
        /* FIXME: handle this
        if ( config.click_to_exit == TRUE ) {
            state->mouse_seen = TRUE;
        }
        */
        xcb_motion_notify_event_t *xme = (xcb_motion_notify_event_t *) ev;
        widget_motion_event me = {
            .x = xme->event_x,
            .y = xme->event_y,
            .time = xme->time,
        };
        rofi_view_handle_mouse_motion ( &me );
        break;
    }
    case XCB_BUTTON_PRESS:
    {
        xcb_button_press_event_t *xbe = (xcb_button_press_event_t *) ev;
        switch ( xbe->detail )
        {
        case 4:
            rofi_view_mouse_navigation(ROFI_MOUSE_WHEEL_UP);
            break;
        case 5:
            rofi_view_mouse_navigation(ROFI_MOUSE_WHEEL_DOWN);
            break;
        case 6:
            rofi_view_mouse_navigation(ROFI_MOUSE_WHEEL_LEFT);
            break;
        case 7:
            rofi_view_mouse_navigation(ROFI_MOUSE_WHEEL_RIGHT);
            break;
        case 1:
        case 2:
        case 3:
        {
            widget_button_event be = {
                .button = xbe->detail,
                .x = xbe->event_x,
                .y = xbe->event_y,
                .time = xbe->time,
            };
            rofi_view_handle_mouse_button(&be);
            break;
        }
        default:
            break;
        }
        break;
    }
    case XCB_BUTTON_RELEASE:
    {
        /* FIXME: add config back
        if ( config.click_to_exit == TRUE ) {
            if ( ( CacheState.flags & MENU_NORMAL_WINDOW ) == 0 ) */ {
                xcb_button_release_event_t *bre = (xcb_button_release_event_t *) ev;
                if ( /*( state->mouse_seen == FALSE ) && */( bre->event != xcb->main_window ) ) {
                    rofi_view_quit(state);
                }
            }
            /*
        }
        */
        break;
    }
    // Paste event.
    case XCB_SELECTION_NOTIFY:
        //FIXME: rofi_view_paste ( state, (xcb_selection_notify_event_t *) ev );
        break;
    case XCB_KEYMAP_NOTIFY:
    {
        xcb_keymap_notify_event_t *kne     = (xcb_keymap_notify_event_t *) ev;
        for ( gint32 by = 0; by < 31; ++by ) {
            for ( gint8 bi = 0; bi < 7; ++bi ) {
                if ( kne->keys[by] & ( 1 << bi ) ) {
                    // X11 keycodes starts at 8
                    xkb_keysym_t key = xkb_state_key_get_one_sym ( xcb->xkb.state, ( 8 * by + bi ) + 8 );
                    widget_modifier_mask       modstate = xkb_get_modmask ( &xcb->xkb, key);
                    abe_find_action ( modstate, key );
                }
            }
        }
        break;
    }
    case XCB_KEY_PRESS:
    {
        xcb_key_press_event_t *xkpe    = (xcb_key_press_event_t *) ev;
        widget_modifier_mask modmask;
        xkb_keysym_t keysym;
        char *text;
        int          len = 0;

        keysym = xkb_handle_key(&xcb->xkb, xkpe->detail, &text, &len);
        modmask = xkb_get_modmask(&xcb->xkb, keysym);

        rofi_view_handle_keypress ( modmask, keysym, text, len );
        break;
    }
    case XCB_KEY_RELEASE:
    {
        xcb_key_release_event_t *xkre    = (xcb_key_release_event_t *) ev;
        widget_modifier_mask modmask;
        xkb_keysym_t keysym;
        char *text;
        int          len = 0;

        keysym = xkb_handle_key(&xcb->xkb, xkre->detail, &text, &len);
        modmask = xkb_get_modmask(&xcb->xkb, keysym);

        if ( modmask != 0 )
            return G_SOURCE_CONTINUE;
        abe_trigger_release ();
        break;
    }
    default:
        break;
    }

    rofi_view_maybe_update();
    return G_SOURCE_CONTINUE;
}

gboolean display_init(GMainLoop *main_loop, const gchar *display)
{
    xcb->main_loop = main_loop;

    xcb->main_loop_source = g_water_xcb_source_new(NULL, display, &xcb->screen_nbr, xcb_callback, NULL, NULL);
    if ( xcb->main_loop_source == NULL )
        return FALSE;

    xcb->connection = g_water_xcb_source_get_connection(xcb->main_loop_source);
    xcb->screen = xcb_aux_get_screen ( xcb->connection, xcb->screen_nbr );

    if ( xkb_x11_setup_xkb_extension ( xcb->connection, XKB_X11_MIN_MAJOR_XKB_VERSION, XKB_X11_MIN_MINOR_XKB_VERSION,
                                           XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS, NULL, NULL, &xcb->xkb_first_event, NULL ) < 0 ) {
        fprintf ( stderr, "cannot setup XKB extension!\n" );
        return FALSE;
    }

    xcb->xkb.context = xkb_context_new ( XKB_CONTEXT_NO_FLAGS );
    if ( xcb->xkb.context == NULL ) {
        fprintf ( stderr, "cannot create XKB context!\n" );
        return FALSE;
    }
    xcb->xkb_device_id = xkb_x11_get_core_keyboard_device_id ( xcb->connection );

    enum
    {
        required_events =
            ( XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY |
              XCB_XKB_EVENT_TYPE_MAP_NOTIFY |
              XCB_XKB_EVENT_TYPE_STATE_NOTIFY ),

        required_nkn_details =
            ( XCB_XKB_NKN_DETAIL_KEYCODES ),

        required_map_parts   =
            ( XCB_XKB_MAP_PART_KEY_TYPES |
              XCB_XKB_MAP_PART_KEY_SYMS |
              XCB_XKB_MAP_PART_MODIFIER_MAP |
              XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS |
              XCB_XKB_MAP_PART_KEY_ACTIONS |
              XCB_XKB_MAP_PART_VIRTUAL_MODS |
              XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP ),

        required_state_details =
            ( XCB_XKB_STATE_PART_MODIFIER_BASE |
              XCB_XKB_STATE_PART_MODIFIER_LATCH |
              XCB_XKB_STATE_PART_MODIFIER_LOCK |
              XCB_XKB_STATE_PART_GROUP_BASE |
              XCB_XKB_STATE_PART_GROUP_LATCH |
              XCB_XKB_STATE_PART_GROUP_LOCK ),
    };

    static const xcb_xkb_select_events_details_t details = {
        .affectNewKeyboard  = required_nkn_details,
        .newKeyboardDetails = required_nkn_details,
        .affectState        = required_state_details,
        .stateDetails       = required_state_details,
    };
    xcb_xkb_select_events ( xcb->connection, xcb->xkb_device_id, required_events, /* affectWhich */
                            0,                                               /* clear */
                            required_events,                                 /* selectAll */
                            required_map_parts,                              /* affectMap */
                            required_map_parts,                              /* map */
                            &details );

    xcb->xkb.keymap = xkb_x11_keymap_new_from_device ( xcb->xkb.context, xcb->connection, xcb->xkb_device_id, XKB_KEYMAP_COMPILE_NO_FLAGS );
    if ( xcb->xkb.keymap == NULL ) {
        fprintf ( stderr, "Failed to get Keymap for current keyboard device.\n" );
        return FALSE;
    }
    xcb->xkb.state = xkb_x11_state_new_from_device ( xcb->xkb.keymap, xcb->connection, xcb->xkb_device_id );
    if ( xcb->xkb.state == NULL ) {
        fprintf ( stderr, "Failed to get state object for current keyboard device.\n" );
        return FALSE;
    }

    xkb_common_init(&xcb->xkb);
    x11_create_frequently_used_atoms();
    x11_create_visual_and_colormap();

    xcb_intern_atom_cookie_t *ac     = xcb_ewmh_init_atoms ( xcb->connection, &xcb->ewmh );
    xcb_generic_error_t      *errors = NULL;
    xcb_ewmh_init_atoms_replies ( &xcb->ewmh, ac, &errors );
    if ( errors ) {
        fprintf ( stderr, "Failed to create EWMH atoms\n" );
        free ( errors );
        return FALSE;
    }
    x11_helper_discover_window_manager();

    xcb_flush(xcb->connection);
    xcb_grab_keyboard ( xcb->connection, 1, xcb->screen->root, XCB_CURRENT_TIME, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC );
    xcb_grab_pointer ( xcb->connection, 1, xcb->screen->root, XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, xcb->screen->root, XCB_NONE, XCB_CURRENT_TIME );

    return TRUE;
}

void display_cleanup(void)
{
    xcb_ungrab_pointer ( xcb->connection, XCB_CURRENT_TIME );
    xcb_ungrab_keyboard ( xcb->connection, XCB_CURRENT_TIME );
    xcb_flush(xcb->connection);
    g_water_xcb_source_free(xcb->main_loop_source);
}

display_buffer_pool *display_buffer_pool_new(gint width, gint height)
{
    xcb->width = width;
    xcb->height = height;

    uint32_t          selmask  = 0;
    uint32_t          selval[7];
    gsize i = 0;

    selmask |= XCB_CW_BACK_PIXMAP;
    selval[i++] = XCB_BACK_PIXMAP_NONE;

    selmask |= XCB_CW_BORDER_PIXEL;
    selval[i++] = 0;

    selmask |= XCB_CW_BIT_GRAVITY;
    selval[i++] = XCB_GRAVITY_STATIC;

    selmask |= XCB_CW_BACKING_STORE;
    selval[i++] = XCB_BACKING_STORE_NOT_USEFUL;

    selmask |= XCB_CW_OVERRIDE_REDIRECT;
    selval[i++] = 1;

    selmask |= XCB_CW_EVENT_MASK;
    selval[i++] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
                  XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_KEYMAP_STATE |
                  XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_BUTTON_1_MOTION;

    selmask |= XCB_CW_COLORMAP;
    selval[i++] = xcb->map;

    xcb->main_window = xcb_generate_id(xcb->connection);
    xcb_void_cookie_t cc  = xcb_create_window_checked ( xcb->connection, xcb->depth->depth, xcb->main_window, xcb->screen->root, 0, 0, xcb->width, xcb->height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, xcb->visual->visual_id, selmask, selval );
    xcb_generic_error_t *error;
    error = xcb_request_check ( xcb->connection, cc );
    if ( error ) {
        printf ( "xcb_create_window() failed error=0x%x\n", error->error_code );
        return FALSE;
    }

    xcb->gc = xcb_generate_id ( xcb->connection );
    xcb_create_gc ( xcb->connection, xcb->gc, xcb->main_window, 0, 0 );

    return NULL;
}

void display_buffer_pool_free(G_GNUC_UNUSED display_buffer_pool *pool)
{
    xcb_free_gc(xcb->connection, xcb->gc);
    xcb_destroy_window(xcb->connection, xcb->main_window);
}

static const cairo_user_data_key_t xcb_user_data;
cairo_surface_t *display_buffer_pool_get_next_buffer(G_GNUC_UNUSED display_buffer_pool *pool)
{
    xcb_pixmap_t p = xcb_generate_id ( xcb->connection );
    cairo_surface_t *surface;

    xcb_create_pixmap ( xcb->connection, xcb->depth->depth, p, xcb->main_window, xcb->width, xcb->height );

    surface = cairo_xcb_surface_create ( xcb->connection, p, xcb->visual, xcb->width, xcb->height );
    cairo_surface_set_user_data(surface, &xcb_user_data, GUINT_TO_POINTER(p), NULL);
    return surface;
}
void display_surface_commit(cairo_surface_t *surface)
{
    xcb_pixmap_t p = GPOINTER_TO_UINT(cairo_surface_get_user_data(surface, &xcb_user_data));
    cairo_surface_destroy(surface);

    xcb_copy_area ( xcb->connection, p, xcb->main_window, xcb->gc, 0, 0, 0, 0, xcb->width, xcb->height );
    xcb_free_pixmap(xcb->connection, p);

    if ( ! xcb->mapped ) {
        xcb->mapped = TRUE;
        xcb_map_window(xcb->connection, xcb->main_window);
    }

    xcb_flush(xcb->connection);
}

// retrieve a text property from a window
// technically we could use window_get_prop(), but this is better for character set support
char* window_get_text_prop ( xcb_window_t w, xcb_atom_t atom )
{
    xcb_get_property_cookie_t c  = xcb_get_property ( xcb->connection, 0, w, atom, XCB_GET_PROPERTY_TYPE_ANY, 0, UINT_MAX );
    xcb_get_property_reply_t  *r = xcb_get_property_reply ( xcb->connection, c, NULL );
    if ( r ) {
        if ( xcb_get_property_value_length ( r ) > 0 ) {
            char *str = NULL;
            if ( r->type == xcb->netatoms[UTF8_STRING] ) {
                str = g_strndup ( xcb_get_property_value ( r ), xcb_get_property_value_length ( r ) );
            }
            else if ( r->type == xcb->netatoms[STRING] ) {
                str = rofi_latin_to_utf8_strdup ( xcb_get_property_value ( r ), xcb_get_property_value_length ( r ) );
            }
            else {
                str = g_strdup ( "Invalid encoding." );
            }

            free ( r );
            return str;
        }
        free ( r );
    }
    return NULL;
}
