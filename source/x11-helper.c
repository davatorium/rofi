/**
 * rofi
 *
 * MIT/X11 License
 * Copyright (c) 2012 Sean Pringle <sean.pringle@gmail.com>
 * Modified 2013-2016 Qball Cow <qball@gmpclient.org>
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
#include <cairo.h>
#include <cairo-xcb.h>

#include <xcb/xcb.h>
#include <xcb/xinerama.h>
#include "xcb-internal.h"
#include "xcb.h"
#include "settings.h"
#include "helper.h"

#include <rofi.h>
#define OVERLAP( a, b, c,                          \
                 d )       ( ( ( a ) == ( c ) &&   \
                               ( b ) == ( d ) ) || \
                             MIN ( ( a ) + ( b ), ( c ) + ( d ) ) - MAX ( ( a ), ( c ) ) > 0 )
#define INTERSECT( x, y, w, h, x1, y1, w1,                   \
                   h1 )    ( OVERLAP ( ( x ), ( w ), ( x1 ), \
                                       ( w1 ) ) && OVERLAP ( ( y ), ( h ), ( y1 ), ( h1 ) ) )
#include "x11-helper.h"
#include "xkb-internal.h"

struct _xcb_stuff xcb_int = {
    .connection   = NULL,
    .screen       = NULL,
    .screen_nbr   =    -1,
    .sndisplay    = NULL,
    .sncontext    = NULL,
    .has_xinerama = FALSE,
};
xcb_stuff         *xcb = &xcb_int;

enum
{
    X11MOD_SHIFT,
    X11MOD_CONTROL,
    X11MOD_ALT,
    X11MOD_META,
    X11MOD_SUPER,
    X11MOD_HYPER,
    X11MOD_ANY,
    NUM_X11MOD
};

xcb_depth_t         *depth           = NULL;
xcb_visualtype_t    *visual          = NULL;
xcb_colormap_t      map              = XCB_COLORMAP_NONE;
xcb_depth_t         *root_depth      = NULL;
xcb_visualtype_t    *root_visual     = NULL;
xcb_atom_t          netatoms[NUM_NETATOMS];
const char          *netatom_names[] = { EWMH_ATOMS ( ATOM_CHAR ) };
static unsigned int x11_mod_masks[NUM_X11MOD];

static xcb_pixmap_t get_root_pixmap ( xcb_connection_t *c,
                                      xcb_screen_t *screen,
                                      xcb_atom_t atom )
{
    xcb_get_property_cookie_t cookie;
    xcb_get_property_reply_t  *reply;
    xcb_pixmap_t              rootpixmap = XCB_NONE;

    cookie = xcb_get_property ( c,
                                0,
                                screen->root,
                                atom,
                                XCB_ATOM_PIXMAP,
                                0,
                                1 );

    reply = xcb_get_property_reply ( c, cookie, NULL );

    if ( reply ) {
        if ( xcb_get_property_value_length ( reply ) == sizeof ( xcb_pixmap_t ) ) {
            memcpy ( &rootpixmap, xcb_get_property_value ( reply ), sizeof ( xcb_pixmap_t ) );
        }
        free ( reply );
    }

    return rootpixmap;
}

cairo_surface_t * x11_helper_get_bg_surface ( void )
{
    xcb_pixmap_t pm = get_root_pixmap ( xcb->connection, xcb->screen, netatoms[ESETROOT_PMAP_ID] );
    if ( pm == XCB_NONE ) {
        return NULL;
    }
    return cairo_xcb_surface_create ( xcb->connection, pm, root_visual,
                                      xcb->screen->width_in_pixels, xcb->screen->height_in_pixels );
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
            if ( r->type == netatoms[UTF8_STRING] ) {
                str = g_strndup ( xcb_get_property_value ( r ), xcb_get_property_value_length ( r ) );
            }
            else if ( r->type == netatoms[STRING] ) {
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

void window_set_atom_prop ( xcb_window_t w, xcb_atom_t prop, xcb_atom_t *atoms, int count )
{
    xcb_change_property ( xcb->connection, XCB_PROP_MODE_REPLACE, w, prop, XCB_ATOM_ATOM, 32, count, atoms );
}

int monitor_get_smallest_size ( void )
{
    xcb_generic_error_t *error;
    int                 size = MIN ( xcb->screen->width_in_pixels, xcb->screen->height_in_pixels );

    if ( !xcb->has_xinerama ) {
        return size;
    }

    xcb_xinerama_query_screens_cookie_t cookie_screen;

    cookie_screen = xcb_xinerama_query_screens ( xcb->connection );
    xcb_xinerama_query_screens_reply_t *query_screens;
    query_screens = xcb_xinerama_query_screens_reply ( xcb->connection, cookie_screen, &error );
    if ( error ) {
        fprintf ( stderr, "Error getting screen info\n" );
        return size;
    }
    xcb_xinerama_screen_info_t *screens = xcb_xinerama_query_screens_screen_info ( query_screens );
    int                        len      = xcb_xinerama_query_screens_screen_info_length ( query_screens );
    for ( int i = 0; i < len; i++ ) {
        xcb_xinerama_screen_info_t *info = &screens[i];
        size = MIN ( info->width, size );
        size = MIN ( info->height, size );
    }
    free ( query_screens );

    return size;
}
int monitor_get_dimension ( int monitor, workarea *mon )
{
    xcb_generic_error_t *error = NULL;
    memset ( mon, 0, sizeof ( workarea ) );
    mon->w = xcb->screen->width_in_pixels;
    mon->h = xcb->screen->height_in_pixels;

    if ( !xcb->has_xinerama ) {
        return FALSE;
    }

    xcb_xinerama_query_screens_cookie_t cookie_screen;
    cookie_screen = xcb_xinerama_query_screens ( xcb->connection );
    xcb_xinerama_query_screens_reply_t  *query_screens;
    query_screens = xcb_xinerama_query_screens_reply ( xcb->connection, cookie_screen, &error );
    if ( error ) {
        fprintf ( stderr, "Error getting screen info\n" );
        return FALSE;
    }
    xcb_xinerama_screen_info_t *screens = xcb_xinerama_query_screens_screen_info ( query_screens );
    int                        len      = xcb_xinerama_query_screens_screen_info_length ( query_screens );
    if ( monitor < len ) {
        xcb_xinerama_screen_info_t *info = &screens[monitor];
        mon->w = info->width;
        mon->h = info->height;
        mon->x = info->x_org;
        mon->y = info->y_org;
        free ( query_screens );
        return TRUE;
    }
    free ( query_screens );

    return FALSE;
}
// find the dimensions of the monitor displaying point x,y
void monitor_dimensions ( int x, int y, workarea *mon )
{
    xcb_generic_error_t *error = NULL;
    memset ( mon, 0, sizeof ( workarea ) );
    mon->w = xcb->screen->width_in_pixels;
    mon->h = xcb->screen->height_in_pixels;

    if ( !xcb->has_xinerama ) {
        return;
    }

    xcb_xinerama_query_screens_cookie_t cookie_screen;
    cookie_screen = xcb_xinerama_query_screens ( xcb->connection );
    xcb_xinerama_query_screens_reply_t  *query_screens;
    query_screens = xcb_xinerama_query_screens_reply ( xcb->connection, cookie_screen, &error );
    if ( error ) {
        fprintf ( stderr, "Error getting screen info\n" );
        return;
    }
    xcb_xinerama_screen_info_t *screens = xcb_xinerama_query_screens_screen_info ( query_screens );
    int                        len      = xcb_xinerama_query_screens_screen_info_length ( query_screens );
    for ( int i = 0; i < len; i++ ) {
        xcb_xinerama_screen_info_t *info = &screens[i];
        if ( INTERSECT ( x, y, 1, 1, info->x_org, info->y_org, info->width, info->height ) ) {
            mon->w = info->width;
            mon->h = info->height;
            mon->x = info->x_org;
            mon->y = info->y_org;
            break;
        }
    }
    free ( query_screens );
}

/**
 * @param x  The x position of the mouse [out]
 * @param y  The y position of the mouse [out]
 *
 * find mouse pointer location
 *
 * @returns 1 when found
 */
static int pointer_get ( xcb_window_t root, int *x, int *y )
{
    *x = 0;
    *y = 0;
    xcb_query_pointer_cookie_t c  = xcb_query_pointer ( xcb->connection, root );
    xcb_query_pointer_reply_t  *r = xcb_query_pointer_reply ( xcb->connection, c, NULL );
    if ( r ) {
        *x = r->root_x;
        *y = r->root_y;
        free ( r );
        return 1;
    }

    return 0;
}

// determine which monitor holds the active window, or failing that the mouse pointer
void monitor_active ( workarea *mon )
{
    xcb_window_t root = xcb->screen->root;
    int          x, y;

    if ( config.monitor >= 0 ) {
        if ( monitor_get_dimension ( config.monitor, mon ) ) {
            return;
        }
        fprintf ( stderr, "Failed to find selected monitor.\n" );
    }
    if ( config.monitor == -3 ) {
        if ( pointer_get ( root, &x, &y ) ) {
            monitor_dimensions ( x, y, mon );
            mon->x = x;
            mon->y = y;
            return;
        }
    }
    // Get the current desktop.
    unsigned int current_desktop = 0;
    if ( config.monitor == -1 && xcb_ewmh_get_current_desktop_reply ( &xcb->ewmh,
                                                                      xcb_ewmh_get_current_desktop ( &xcb->ewmh, xcb->screen_nbr ),
                                                                      &current_desktop, NULL ) ) {
        xcb_get_property_cookie_t             c = xcb_ewmh_get_desktop_viewport ( &xcb->ewmh, xcb->screen_nbr );
        xcb_ewmh_get_desktop_viewport_reply_t vp;
        if ( xcb_ewmh_get_desktop_viewport_reply ( &xcb->ewmh, c, &vp, NULL ) ) {
            if ( current_desktop < vp.desktop_viewport_len ) {
                monitor_dimensions ( vp.desktop_viewport[current_desktop].x,
                                     vp.desktop_viewport[current_desktop].y, mon );
                xcb_ewmh_get_desktop_viewport_reply_wipe ( &vp );
                return;
            }
            xcb_ewmh_get_desktop_viewport_reply_wipe ( &vp );
        }
    }

    xcb_window_t active_window;
    if ( xcb_ewmh_get_active_window_reply ( &xcb->ewmh,
                                            xcb_ewmh_get_active_window ( &xcb->ewmh, xcb->screen_nbr ), &active_window, NULL ) ) {
        // get geometry.
        xcb_get_geometry_cookie_t c  = xcb_get_geometry ( xcb->connection, active_window );
        xcb_get_geometry_reply_t  *r = xcb_get_geometry_reply ( xcb->connection, c, NULL );
        if ( r ) {
            xcb_translate_coordinates_cookie_t ct = xcb_translate_coordinates ( xcb->connection, active_window, root, r->x, r->y );
            xcb_translate_coordinates_reply_t  *t = xcb_translate_coordinates_reply ( xcb->connection, ct, NULL );
            if ( t ) {
                if ( config.monitor == -2 ) {
                    // place the menu above the window
                    // if some window is focused, place menu above window, else fall
                    // back to selected monitor.
                    mon->x = t->dst_x - r->x;
                    mon->y = t->dst_y - r->y;
                    mon->w = r->width;
                    mon->h = r->height;
                    mon->t = r->border_width;
                    mon->b = r->border_width;
                    mon->l = r->border_width;
                    mon->r = r->border_width;
                    free ( r );
                    free ( t );
                    return;
                }
                else if ( config.monitor == -4 ) {
                    monitor_dimensions ( t->dst_x, t->dst_y, mon );
                    free ( r );
                    free ( t );
                    return;
                }
            }
            free ( r );
        }
    }
    if ( pointer_get ( root, &x, &y ) ) {
        monitor_dimensions ( x, y, mon );
        return;
    }

    monitor_dimensions ( 0, 0, mon );
}
int take_pointer ( xcb_window_t w )
{
    for ( int i = 0; i < 500; i++ ) {
        if ( xcb_connection_has_error ( xcb->connection ) ) {
            fprintf ( stderr, "Connection has error\n" );
            exit ( EXIT_FAILURE );
        }
        xcb_grab_pointer_cookie_t cc = xcb_grab_pointer ( xcb->connection, 1, w, XCB_EVENT_MASK_BUTTON_RELEASE,
                                                          XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, w, XCB_NONE, XCB_CURRENT_TIME );
        xcb_grab_pointer_reply_t  *r = xcb_grab_pointer_reply ( xcb->connection, cc, NULL );
        if ( r ) {
            if ( r->status == XCB_GRAB_STATUS_SUCCESS ) {
                free ( r );
                return 1;
            }
            free ( r );
        }
        usleep ( 1000 );
    }
    fprintf ( stderr, "Failed to grab pointer.\n" );
    return 0;
}
int take_keyboard ( xcb_window_t w )
{
    for ( int i = 0; i < 500; i++ ) {
        if ( xcb_connection_has_error ( xcb->connection ) ) {
            fprintf ( stderr, "Connection has error\n" );
            exit ( EXIT_FAILURE );
        }
        xcb_grab_keyboard_cookie_t cc = xcb_grab_keyboard ( xcb->connection,
                                                            1, w, XCB_CURRENT_TIME, XCB_GRAB_MODE_ASYNC,
                                                            XCB_GRAB_MODE_ASYNC );
        xcb_grab_keyboard_reply_t *r = xcb_grab_keyboard_reply ( xcb->connection, cc, NULL );
        if ( r ) {
            if ( r->status == XCB_GRAB_STATUS_SUCCESS ) {
                free ( r );
                return 1;
            }
            free ( r );
        }
        usleep ( 1000 );
    }

    return 0;
}

void release_keyboard ( void )
{
    xcb_ungrab_keyboard ( xcb->connection, XCB_CURRENT_TIME );
}
void release_pointer ( void )
{
    xcb_ungrab_pointer ( xcb->connection, XCB_CURRENT_TIME );
}

static unsigned int x11_find_mod_mask ( xkb_stuff *xkb, ... )
{
    va_list         names;
    const char      *name;
    xkb_mod_index_t i;
    unsigned int    mask = 0;
    va_start ( names, xkb );
    while ( ( name = va_arg ( names, const char * ) ) != NULL ) {
        i = xkb_keymap_mod_get_index ( xkb->keymap, name );
        if ( i != XKB_MOD_INVALID ) {
            mask |= 1 << i;
        }
    }
    va_end ( names );
    return mask;
}

static void x11_figure_out_masks ( xkb_stuff *xkb )
{
    x11_mod_masks[X11MOD_SHIFT]   = x11_find_mod_mask ( xkb, XKB_MOD_NAME_SHIFT, NULL );
    x11_mod_masks[X11MOD_CONTROL] = x11_find_mod_mask ( xkb, XKB_MOD_NAME_CTRL, NULL );
    x11_mod_masks[X11MOD_ALT]     = x11_find_mod_mask ( xkb, XKB_MOD_NAME_ALT, "Alt", "LAlt", "RAlt", "AltGr", "Mod5", "LevelThree", NULL );
    x11_mod_masks[X11MOD_META]    = x11_find_mod_mask ( xkb, "Meta", NULL );
    x11_mod_masks[X11MOD_SUPER]   = x11_find_mod_mask ( xkb, XKB_MOD_NAME_LOGO, "Super", NULL );
    x11_mod_masks[X11MOD_HYPER]   = x11_find_mod_mask ( xkb, "Hyper", NULL );

    gsize i;
    for ( i = 0; i < X11MOD_ANY; ++i ) {
        x11_mod_masks[X11MOD_ANY] |= x11_mod_masks[i];
    }
}

unsigned int x11_canonalize_mask ( unsigned int mask )
{
    // Bits 13 and 14 of the modifiers together are the group number, and
    // should be ignored when looking up key bindings
    mask &= x11_mod_masks[X11MOD_ANY];

    gsize i;
    for ( i = 0; i < X11MOD_ANY; ++i ) {
        if ( mask & x11_mod_masks[i] ) {
            mask |= x11_mod_masks[i];
        }
    }
    return mask;
}

unsigned int x11_get_current_mask ( xkb_stuff *xkb )
{
    unsigned int mask = 0;
    for ( gsize i = 0; i < xkb_keymap_num_mods ( xkb->keymap ); ++i ) {
        if ( xkb_state_mod_index_is_active ( xkb->state, i, XKB_STATE_MODS_EFFECTIVE ) ) {
            mask |= ( 1 << i );
        }
    }
    return x11_canonalize_mask ( mask );
}

// convert a Mod+key arg to mod mask and keysym
gboolean x11_parse_key ( char *combo, unsigned int *mod, xkb_keysym_t *key, gboolean *release )
{
    GString      *str    = g_string_new ( "" );
    unsigned int modmask = 0;

    if ( g_str_has_prefix ( combo, "!" ) ) {
        ++combo;
        *release = TRUE;
    }

    if ( strcasestr ( combo, "shift" ) ) {
        modmask |= x11_mod_masks[X11MOD_SHIFT];
        if ( x11_mod_masks[X11MOD_SHIFT] == 0 ) {
            g_string_append_printf ( str, "X11 configured keyboard has no <b>Shift</b> key.\n" );
        }
    }
    if ( strcasestr ( combo, "control" ) ) {
        modmask |= x11_mod_masks[X11MOD_CONTROL];
        if ( x11_mod_masks[X11MOD_CONTROL] == 0 ) {
            g_string_append_printf ( str, "X11 configured keyboard has no <b>Control</b> key.\n" );
        }
    }
    if ( strcasestr ( combo, "alt" ) ) {
        modmask |= x11_mod_masks[X11MOD_ALT];
        if ( x11_mod_masks[X11MOD_ALT] == 0 ) {
            g_string_append_printf ( str, "X11 configured keyboard has no <b>Alt</b> key.\n" );
        }
    }
    if ( strcasestr ( combo, "super" ) ) {
        modmask |= x11_mod_masks[X11MOD_SUPER];
        if ( x11_mod_masks[X11MOD_SUPER] == 0 ) {
            g_string_append_printf ( str, "X11 configured keyboard has no <b>Super</b> key.\n" );
        }
    }
    if ( strcasestr ( combo, "meta" ) ) {
        modmask |= x11_mod_masks[X11MOD_META];
        if ( x11_mod_masks[X11MOD_META] == 0 ) {
            g_string_append_printf ( str, "X11 configured keyboard has no <b>Meta</b> key.\n" );
        }
    }
    if ( strcasestr ( combo, "hyper" ) ) {
        modmask |= x11_mod_masks[X11MOD_HYPER];
        if ( x11_mod_masks[X11MOD_HYPER] == 0 ) {
            g_string_append_printf ( str, "X11 configured keyboard has no <b>Hyper</b> key.\n" );
        }
    }
    int seen_mod = FALSE;
    if ( strcasestr ( combo, "Mod" ) ) {
        seen_mod = TRUE;
    }

    // Find location of modifier (if it exists)
    char i = strlen ( combo );

    while ( i > 0 && !strchr ( "-+", combo[i - 1] ) ) {
        i--;
    }

    // if there's no "-" or "+", we might be binding directly to a modifier key - no modmask
    if ( i == 0 ) {
        *mod = 0;
    }
    else {
        *mod = modmask;
    }

    // Parse key
    xkb_keysym_t sym = XKB_KEY_NoSymbol;
    if ( ( modmask & x11_mod_masks[X11MOD_SHIFT] ) != 0 ) {
        gchar * str = g_utf8_next_char ( combo + i );
        // If it is a single char, we make a capital out of it.
        if ( str != NULL && *str == '\0' ) {
            int      l = 0;
            char     buff[8];
            gunichar v = g_utf8_get_char ( combo + i );
            gunichar u = g_unichar_toupper ( v );
            if ( ( l = g_unichar_to_utf8 ( u, buff ) ) ) {
                buff[l] = '\0';
                sym     = xkb_keysym_from_name ( buff, XKB_KEYSYM_NO_FLAGS );
            }
        }
    }
    if ( sym == XKB_KEY_NoSymbol ) {
        sym = xkb_keysym_from_name ( combo + i, XKB_KEYSYM_CASE_INSENSITIVE );
    }

    if ( sym == XKB_KEY_NoSymbol || ( !modmask && ( strchr ( combo, '-' ) || strchr ( combo, '+' ) ) ) ) {
        g_string_append_printf ( str, "Sorry, rofi cannot understand the key combination: <i>%s</i>\n", combo );
        g_string_append ( str, "\nRofi supports the following modifiers:\n\t" );
        g_string_append ( str, "<i>Shift,Control,Alt,Super,Meta,Hyper</i>" );
        if ( seen_mod ) {
            g_string_append ( str, "\n\n<b>Mod1,Mod2,Mod3,Mod4,Mod5 are no longer supported, use one of the above.</b>" );
        }
    }
    if ( str->len > 0 ) {
        rofi_view_error_dialog ( str->str, TRUE );
        g_string_free ( str, TRUE );
        return FALSE;
    }
    g_string_free ( str, TRUE );
    *key = sym;
    return TRUE;
}

void x11_set_window_opacity ( xcb_window_t box, unsigned int opacity )
{
    // Scale 0-100 to 0 - UINT32_MAX.
    unsigned int opacity_set = ( unsigned int ) ( ( opacity / 100.0 ) * UINT32_MAX );

    xcb_change_property ( xcb->connection, XCB_PROP_MODE_REPLACE, box,
                          netatoms[_NET_WM_WINDOW_OPACITY], XCB_ATOM_CARDINAL, 32, 1L, &opacity_set );
}

/**
 * @param display The connection to the X server.
 *
 * Fill in the list of Atoms.
 */
static void x11_create_frequently_used_atoms ( void )
{
    // X atom values
    for ( int i = 0; i < NUM_NETATOMS; i++ ) {
        xcb_intern_atom_cookie_t cc = xcb_intern_atom ( xcb->connection, 0, strlen ( netatom_names[i] ), netatom_names[i] );
        xcb_intern_atom_reply_t  *r = xcb_intern_atom_reply ( xcb->connection, cc, NULL );
        if ( r ) {
            netatoms[i] = r->atom;
            free ( r );
        }
    }
}

void x11_setup ( xkb_stuff *xkb )
{
    // determine numlock mask so we can bind on keys with and without it
    x11_figure_out_masks ( xkb );
    x11_create_frequently_used_atoms (  );
}

void x11_create_visual_and_colormap ( void )
{
    xcb_depth_iterator_t depth_iter;
    for ( depth_iter = xcb_screen_allowed_depths_iterator ( xcb->screen ); depth_iter.rem; xcb_depth_next ( &depth_iter ) ) {
        xcb_depth_t               *d = depth_iter.data;

        xcb_visualtype_iterator_t visual_iter;
        for ( visual_iter = xcb_depth_visuals_iterator ( d ); visual_iter.rem; xcb_visualtype_next ( &visual_iter ) ) {
            xcb_visualtype_t *v = visual_iter.data;
            if ( ( d->depth == 32 ) && ( v->_class == XCB_VISUAL_CLASS_TRUE_COLOR ) ) {
                depth  = d;
                visual = v;
            }
            if ( xcb->screen->root_visual == v->visual_id ) {
                root_depth  = d;
                root_visual = v;
            }
        }
    }
    if ( visual != NULL ) {
        xcb_void_cookie_t   c;
        xcb_generic_error_t *e;
        map = xcb_generate_id ( xcb->connection );
        c   = xcb_create_colormap_checked ( xcb->connection, XCB_COLORMAP_ALLOC_NONE, map, xcb->screen->root, visual->visual_id );
        e   = xcb_request_check ( xcb->connection, c );
        if ( e ) {
            depth  = NULL;
            visual = NULL;
            free ( e );
        }
    }

    if ( visual == NULL ) {
        depth  = root_depth;
        visual = root_visual;
        map    = xcb->screen->default_colormap;
    }
}

Color color_get ( const char *const name )
{
    char *copy  = g_strdup ( name );
    char *cname = g_strstrip ( copy );

    union
    {
        struct
        {
            uint8_t b;
            uint8_t g;
            uint8_t r;
            uint8_t a;
        }        sep;
        uint32_t pixel;
    } color = {
        .pixel = 0xffffffff,
    };
    // Special format.
    if ( strncmp ( cname, "argb:", 5 ) == 0 ) {
        color.pixel = strtoul ( &cname[5], NULL, 16 );
    }
    else if ( strncmp ( cname, "#", 1 ) == 0 ) {
        unsigned long val    = strtoul ( &cname[1], NULL, 16 );
        ssize_t       length = strlen ( &cname[1] );
        switch ( length )
        {
        case 3:
            color.sep.a = 0xff;
            color.sep.r = 16 * ( ( val & 0xF00 ) >> 8 );
            color.sep.g = 16 * ( ( val & 0x0F0 ) >> 4 );
            color.sep.b = 16 * ( val & 0x00F );
            break;
        case 6:
            color.pixel = val;
            color.sep.a = 0xff;
            break;
        case 8:
            color.pixel = val;
            break;
        default:
            break;
        }
    }
    else {
        xcb_alloc_named_color_cookie_t cc = xcb_alloc_named_color ( xcb->connection,
                                                                    map, strlen ( cname ), cname );
        xcb_alloc_named_color_reply_t  *r = xcb_alloc_named_color_reply ( xcb->connection, cc, NULL );
        if ( r ) {
            color.sep.a = 0xFF;
            color.sep.r = r->visual_red;
            color.sep.g = r->visual_green;
            color.sep.b = r->visual_blue;
            free ( r );
        }
    }
    g_free ( copy );

    Color ret = {
        .red   = color.sep.r / 255.0,
        .green = color.sep.g / 255.0,
        .blue  = color.sep.b / 255.0,
        .alpha = color.sep.a / 255.0,
    };
    return ret;
}

void x11_helper_set_cairo_rgba ( cairo_t *d, Color col )
{
    cairo_set_source_rgba ( d, col.red, col.green, col.blue, col.alpha );
}
/**
 * Color cache.
 *
 * This stores the current color until
 */
enum
{
    BACKGROUND,
    BORDER,
    SEPARATOR
};
static struct
{
    Color        color;
    unsigned int set;
} color_cache[3];

void color_background ( cairo_t *d )
{
    if ( !color_cache[BACKGROUND].set ) {
        gchar **vals = g_strsplit ( config.color_window, ",", 3 );
        if ( vals != NULL && vals[0] != NULL ) {
            color_cache[BACKGROUND].color = color_get ( vals[0] );
        }
        g_strfreev ( vals );
        color_cache[BACKGROUND].set = TRUE;
    }

    x11_helper_set_cairo_rgba ( d, color_cache[BACKGROUND].color );
}

void color_border ( cairo_t *d  )
{
    if ( !color_cache[BORDER].set ) {
        gchar **vals = g_strsplit ( config.color_window, ",", 3 );
        if ( vals != NULL && vals[0] != NULL && vals[1] != NULL ) {
            color_cache[BORDER].color = color_get ( vals[1] );
        }
        g_strfreev ( vals );
        color_cache[BORDER].set = TRUE;
    }
    x11_helper_set_cairo_rgba ( d, color_cache[BORDER].color );
}

void color_separator ( cairo_t *d )
{
    if ( !color_cache[SEPARATOR].set ) {
        gchar **vals = g_strsplit ( config.color_window, ",", 3 );
        if ( vals != NULL && vals[0] != NULL && vals[1] != NULL && vals[2] != NULL  ) {
            color_cache[SEPARATOR].color = color_get ( vals[2] );
        }
        else if ( vals != NULL && vals[0] != NULL && vals[1] != NULL ) {
            color_cache[SEPARATOR].color = color_get ( vals[1] );
        }
        g_strfreev ( vals );
        color_cache[SEPARATOR].set = TRUE;
    }
    x11_helper_set_cairo_rgba ( d, color_cache[SEPARATOR].color );
}

xcb_window_t xcb_stuff_get_root_window ( xcb_stuff *xcb )
{
    return xcb->screen->root;
}

void xcb_stuff_wipe ( xcb_stuff *xcb )
{
    if ( xcb->connection != NULL ) {
        if ( xcb->sncontext != NULL ) {
            sn_launchee_context_unref ( xcb->sncontext );
            xcb->sncontext = NULL;
        }
        if ( xcb->sndisplay != NULL ) {
            sn_display_unref ( xcb->sndisplay );
            xcb->sndisplay = NULL;
        }
        xcb_disconnect ( xcb->connection );
        xcb->connection = NULL;
        xcb->screen     = NULL;
        xcb->screen_nbr = 0;
    }
}
