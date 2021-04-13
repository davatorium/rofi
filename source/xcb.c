/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2012 Sean Pringle <sean.pringle@gmail.com>
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

/** Log domain for this module */
#define G_LOG_DOMAIN    "X11Helper"

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <glib.h>
#include <cairo.h>
#include <cairo-xcb.h>
#include <math.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/randr.h>
#include <xcb/xinerama.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xproto.h>
#include <xcb/xkb.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-x11.h>
/** Indicate that we know the startup notification api is not yet stable. */
#define SN_API_NOT_YET_FROZEN
/** This function is declared as sn_launcher_context_set_application_id but implemented as sn_launcher_set_application_id. Quick Fix. */
#define sn_launcher_context_set_application_id    sn_launcher_set_application_id
#include "rofi-types.h"
#include <libsn/sn.h>
#include "display.h"
#include "xcb-internal.h"
#include "xcb.h"
#include "settings.h"
#include "helper.h"
#include "timings.h"

#include <rofi.h>

/** Minimal randr preferred for running rofi (1.5) (Major version number) */
#define RANDR_PREF_MAJOR_VERSION    1
/** Minimal randr preferred for running rofi (1.5) (Minor version number) */
#define RANDR_PREF_MINOR_VERSION    5

/** Checks if the if x and y is inside rectangle. */
#define INTERSECT( x, y, x1, y1, w1, h1 )    ( ( ( ( x ) >= ( x1 ) ) && ( ( x ) < ( x1 + w1 ) ) ) && ( ( ( y ) >= ( y1 ) ) && ( ( y ) < ( y1 + h1 ) ) ) )
WindowManagerQuirk current_window_manager = WM_EWHM;

/**
 * Structure holding xcb objects needed to function.
 */
struct _xcb_stuff xcb_int = {
    .connection = NULL,
    .screen     = NULL,
    .screen_nbr = -1,
    .sndisplay  = NULL,
    .sncontext  = NULL,
    .monitors   = NULL
};
xcb_stuff         *xcb = &xcb_int;

/**
 * Depth of root window.
 */
xcb_depth_t      *depth  = NULL;
xcb_visualtype_t *visual = NULL;
xcb_colormap_t   map     = XCB_COLORMAP_NONE;
/**
 * Visual of the root window.
 */
static xcb_visualtype_t *root_visual = NULL;
xcb_atom_t              netatoms[NUM_NETATOMS];
const char              *netatom_names[] = { EWMH_ATOMS ( ATOM_CHAR ) };

static xcb_visualtype_t * lookup_visual ( xcb_screen_t   *s, xcb_visualid_t visual )
{
    xcb_depth_iterator_t d;
    d = xcb_screen_allowed_depths_iterator ( s );
    for (; d.rem; xcb_depth_next ( &d ) ) {
        xcb_visualtype_iterator_t v = xcb_depth_visuals_iterator ( d.data );
        for (; v.rem; xcb_visualtype_next ( &v ) ) {
            if ( v.data->visual_id == visual ) {
                return v.data;
            }
        }
    }
    return 0;
}

/* This blur function was originally created my MacSlow and published on his website:
 * http://macslow.thepimp.net. I'm not entirely sure he's proud of it, but it has
 * proved immeasurably useful for me. */

static uint32_t* create_kernel(double radius, double deviation, uint32_t *sum2) {
	int size = 2 * (int)(radius) + 1;
	uint32_t* kernel = (uint32_t*)(g_malloc(sizeof(uint32_t) * (size + 1)));
	double radiusf = fabs(radius) + 1.0;
	double value = -radius;
	double sum = 0.0;
	int i;

    if(deviation == 0.0) {
        deviation = sqrt( -(radiusf * radiusf) / (2.0 * log(1.0 / 255.0)));
    }

	kernel[0] = size;

	for(i = 0; i < size; i++) {
        kernel[1 + i] = INT16_MAX / (2.506628275 * deviation) * exp(-((value * value) / (2.0 * (deviation * deviation)))) ;

		sum += kernel[1 + i];
		value += 1.0;
	}

    *sum2 = sum;

	return kernel;
}

void cairo_image_surface_blur(cairo_surface_t* surface, double radius, double deviation)
{
    uint32_t* horzBlur;
    uint32_t * kernel = 0;
    cairo_format_t format;
    unsigned int channels;

    if(cairo_surface_status(surface)) return ;

    uint8_t *data = cairo_image_surface_get_data(surface);
    format = cairo_image_surface_get_format(surface);
    const int width = cairo_image_surface_get_width(surface);
    const int height = cairo_image_surface_get_height(surface);
    const int stride = cairo_image_surface_get_stride(surface);

    if(format == CAIRO_FORMAT_ARGB32) channels = 4;
    else return ;

    horzBlur = (uint32_t*)(g_malloc(sizeof(uint32_t) * height * stride));
    TICK();
    uint32_t sum = 0;
    kernel = create_kernel(radius, deviation, &sum);
    TICK_N("BLUR: kernel");

    /* Horizontal pass. */
    uint32_t *horzBlur_ptr = horzBlur;
    for(int iY = 0; iY < height; iY++) {
        const int iYs = iY*stride;
        for(int iX = 0; iX < width; iX++) {
            uint32_t red = 0;
            uint32_t green = 0;
            uint32_t blue = 0;
            uint32_t alpha = 0;
            int offset = (int)(kernel[0]) / -2;

            for(int i = 0; i < (int)(kernel[0]); i++) {
                int x = iX + offset;

                if(x < 0 || x >= width){
                    offset++;
                    continue;
                }

                uint8_t *dataPtr = &data[iYs + x * channels];
                const uint32_t kernip1 = kernel[i + 1];

                blue  += kernip1 * dataPtr[0];
                green += kernip1 * dataPtr[1];
                red   += kernip1 * dataPtr[2];
                alpha += kernip1 * dataPtr[3];
                offset++;
            }


            *horzBlur_ptr++ = blue/sum;
            *horzBlur_ptr++ = green/sum;
            *horzBlur_ptr++ = red/sum;
            *horzBlur_ptr++ = alpha/sum;
        }
    }
    TICK_N("BLUR: hori");

    /* Vertical pass. */
    for(int iY = 0; iY < height; iY++) {
        for(int iX = 0; iX < width; iX++) {
            uint32_t red = 0;
            uint32_t green = 0;
            uint32_t blue = 0;
            uint32_t alpha = 0;
            int offset = (int)(kernel[0]) / -2;

            const int iXs = iX*channels;
            for(int i = 0; i < (int)(kernel[0]); i++) {
                int y = iY + offset;

                if(y < 0 || y >= height) {
                    offset++;
                    continue;
                }

                uint32_t *dataPtr = &horzBlur[y * stride + iXs];
                const uint32_t kernip1 = kernel[i + 1];

                blue  += kernip1 * dataPtr[0];
                green += kernip1 * dataPtr[1];
                red   += kernip1 * dataPtr[2];
                alpha += kernip1 * dataPtr[3];

                offset++;
            }


            *data++ = blue/sum;
            *data++ = green/sum;
            *data++ = red/sum;
            *data++ = alpha/sum;
        }
    }
    TICK_N("BLUR: vert");

    free(kernel);
    free(horzBlur);

    return ;
}



cairo_surface_t *x11_helper_get_screenshot_surface_window ( xcb_window_t window, int size )
{
    xcb_get_geometry_cookie_t cookie;
    xcb_get_geometry_reply_t  *reply;

    cookie = xcb_get_geometry ( xcb->connection, window );
    reply  = xcb_get_geometry_reply ( xcb->connection, cookie, NULL );
    if ( reply == NULL ) {
        return NULL;
    }

    xcb_get_window_attributes_cookie_t attributesCookie = xcb_get_window_attributes ( xcb->connection, window );
    xcb_get_window_attributes_reply_t  *attributes      = xcb_get_window_attributes_reply ( xcb->connection,
                                                                                            attributesCookie,
                                                                                            NULL );
    if ( attributes == NULL || ( attributes->map_state != XCB_MAP_STATE_VIEWABLE ) ) {
        free ( reply );
        if ( attributes ) {
            free ( attributes );
        }
        return NULL;
    }
    // Create a cairo surface for the window.
    xcb_visualtype_t * vt = lookup_visual ( xcb->screen, attributes->visual );
    free ( attributes );

    cairo_surface_t *t = cairo_xcb_surface_create ( xcb->connection, window, vt, reply->width, reply->height );

    if ( cairo_surface_status ( t ) != CAIRO_STATUS_SUCCESS ) {
        cairo_surface_destroy ( t );
        free ( reply );
        return NULL;
    }

    // Scale the image, as we don't want to keep large one around.
    int             max   = MAX ( reply->width, reply->height );
    double          scale = (double) size / max;

    cairo_surface_t *s2 = cairo_surface_create_similar_image ( t, CAIRO_FORMAT_ARGB32, reply->width * scale, reply->height * scale );
    free ( reply );

    if ( cairo_surface_status ( s2 ) != CAIRO_STATUS_SUCCESS ) {
        cairo_surface_destroy ( t );
        return NULL;
    }
    // Paint it in.
    cairo_t *d = cairo_create ( s2 );
    cairo_scale ( d, scale, scale );
    cairo_set_source_surface ( d, t, 0, 0 );
    cairo_paint ( d );
    cairo_destroy ( d );

    cairo_surface_destroy ( t );
    return s2;
}
/**
 * Holds for each supported modifier the possible modifier mask.
 * Check x11_mod_masks[MODIFIER]&mask != 0 to see if MODIFIER is activated.
 */
cairo_surface_t *x11_helper_get_screenshot_surface ( void )
{
    return cairo_xcb_surface_create ( xcb->connection,
                                      xcb_stuff_get_root_window (), root_visual,
                                      xcb->screen->width_in_pixels, xcb->screen->height_in_pixels );
}

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

/****
 * Code used to get monitor layout.
 */

/**
 * Free monitor structure.
 */
static void x11_monitor_free ( workarea *m )
{
    g_free ( m->name );
    g_free ( m );
}

static void x11_monitors_free ( void )
{
    while ( xcb->monitors != NULL ) {
        workarea *m = xcb->monitors;
        xcb->monitors = m->next;
        x11_monitor_free ( m );
    }
}

/**
 * Create monitor based on output id
 */
static workarea * x11_get_monitor_from_output ( xcb_randr_output_t out )
{
    xcb_randr_get_output_info_reply_t  *op_reply;
    xcb_randr_get_crtc_info_reply_t    *crtc_reply;
    xcb_randr_get_output_info_cookie_t it = xcb_randr_get_output_info ( xcb->connection, out, XCB_CURRENT_TIME );
    op_reply = xcb_randr_get_output_info_reply ( xcb->connection, it, NULL );
    if ( op_reply->crtc == XCB_NONE ) {
        free ( op_reply );
        return NULL;
    }
    xcb_randr_get_crtc_info_cookie_t ct = xcb_randr_get_crtc_info ( xcb->connection, op_reply->crtc, XCB_CURRENT_TIME );
    crtc_reply = xcb_randr_get_crtc_info_reply ( xcb->connection, ct, NULL );
    if ( !crtc_reply ) {
        free ( op_reply );
        return NULL;
    }
    workarea *retv = g_malloc0 ( sizeof ( workarea ) );
    retv->x = crtc_reply->x;
    retv->y = crtc_reply->y;
    retv->w = crtc_reply->width;
    retv->h = crtc_reply->height;

    retv->mw = op_reply->mm_width;
    retv->mh = op_reply->mm_height;

    char *tname    = (char *) xcb_randr_get_output_info_name ( op_reply );
    int  tname_len = xcb_randr_get_output_info_name_length ( op_reply );

    retv->name = g_malloc0 ( ( tname_len + 1 ) * sizeof ( char ) );
    memcpy ( retv->name, tname, tname_len );

    free ( crtc_reply );
    free ( op_reply );
    return retv;
}

#if  ( ( ( XCB_RANDR_MAJOR_VERSION >= RANDR_PREF_MAJOR_VERSION ) && ( XCB_RANDR_MINOR_VERSION >= RANDR_PREF_MINOR_VERSION ) ) \
    || XCB_RANDR_MAJOR_VERSION > RANDR_PREF_MAJOR_VERSION  )
/**
 * @param mon The randr monitor to parse.
 *
 * Create monitor based on xrandr monitor id.
 *
 * @returns A workarea representing the monitor mon
 */
static workarea *x11_get_monitor_from_randr_monitor ( xcb_randr_monitor_info_t *mon )
{
    // Query to the name of the monitor.
    xcb_generic_error_t        *err;
    xcb_get_atom_name_cookie_t anc         = xcb_get_atom_name ( xcb->connection, mon->name );
    xcb_get_atom_name_reply_t  *atom_reply = xcb_get_atom_name_reply ( xcb->connection, anc, &err );
    if ( err != NULL ) {
        g_warning ( "Could not get RandR monitor name: X11 error code %d\n", err->error_code );
        free ( err );
        return NULL;
    }
    workarea *retv = g_malloc0 ( sizeof ( workarea ) );

    // Is primary monitor.
    retv->primary = mon->primary;

    // Position and size.
    retv->x = mon->x;
    retv->y = mon->y;
    retv->w = mon->width;
    retv->h = mon->height;

    // Physical
    retv->mw = mon->width_in_millimeters;
    retv->mh = mon->height_in_millimeters;

    // Name
    retv->name = g_strdup_printf ( "%.*s", xcb_get_atom_name_name_length ( atom_reply ), xcb_get_atom_name_name ( atom_reply ) );

    // Free name atom.
    free ( atom_reply );

    return retv;
}
#endif

static int x11_is_extension_present ( const char *extension )
{
    xcb_query_extension_cookie_t randr_cookie = xcb_query_extension ( xcb->connection, strlen ( extension ), extension );

    xcb_query_extension_reply_t  *randr_reply = xcb_query_extension_reply ( xcb->connection, randr_cookie, NULL );

    int                          present = randr_reply->present;

    free ( randr_reply );

    return present;
}

static void x11_build_monitor_layout_xinerama ()
{
    xcb_xinerama_query_screens_cookie_t screens_cookie = xcb_xinerama_query_screens_unchecked (
        xcb->connection
        );

    xcb_xinerama_query_screens_reply_t *screens_reply = xcb_xinerama_query_screens_reply (
        xcb->connection,
        screens_cookie,
        NULL
        );

    xcb_xinerama_screen_info_iterator_t screens_iterator = xcb_xinerama_query_screens_screen_info_iterator (
        screens_reply
        );

    for (; screens_iterator.rem > 0; xcb_xinerama_screen_info_next ( &screens_iterator ) ) {
        workarea *w = g_malloc0 ( sizeof ( workarea ) );

        w->x = screens_iterator.data->x_org;
        w->y = screens_iterator.data->y_org;
        w->w = screens_iterator.data->width;
        w->h = screens_iterator.data->height;

        w->next       = xcb->monitors;
        xcb->monitors = w;
    }

    int index = 0;
    for ( workarea *iter = xcb->monitors; iter; iter = iter->next ) {
        iter->monitor_id = index++;
    }

    free ( screens_reply );
}

static void x11_build_monitor_layout ()
{
    if ( xcb->monitors ) {
        return;
    }
    // If RANDR is not available, try Xinerama
    if ( !x11_is_extension_present ( "RANDR" ) ) {
        // Check if xinerama is available.
        if ( x11_is_extension_present ( "XINERAMA" ) ) {
            g_debug ( "Query XINERAMA for monitor layout." );
            x11_build_monitor_layout_xinerama ();
            return;
        }
        g_debug ( "No RANDR or Xinerama available for getting monitor layout." );
        return;
    }
    g_debug ( "Query RANDR for monitor layout." );

    g_debug ( "Randr XCB api version: %d.%d.", XCB_RANDR_MAJOR_VERSION, XCB_RANDR_MINOR_VERSION );
#if  ( ( ( XCB_RANDR_MAJOR_VERSION == RANDR_PREF_MAJOR_VERSION ) && ( XCB_RANDR_MINOR_VERSION >= RANDR_PREF_MINOR_VERSION ) ) \
    || XCB_RANDR_MAJOR_VERSION > RANDR_PREF_MAJOR_VERSION  )
    xcb_randr_query_version_cookie_t cversion = xcb_randr_query_version ( xcb->connection,
                                                                          RANDR_PREF_MAJOR_VERSION, RANDR_PREF_MINOR_VERSION );
    xcb_randr_query_version_reply_t  *rversion = xcb_randr_query_version_reply ( xcb->connection, cversion, NULL );
    if ( rversion ) {
        g_debug ( "Found randr version: %d.%d", rversion->major_version, rversion->minor_version );
        // Check if we are 1.5 and up.
        if ( ( ( rversion->major_version == RANDR_PREF_MAJOR_VERSION ) && ( rversion->minor_version >= RANDR_PREF_MINOR_VERSION ) ) ||
             ( rversion->major_version > RANDR_PREF_MAJOR_VERSION ) ) {
            xcb_randr_get_monitors_cookie_t t       = xcb_randr_get_monitors ( xcb->connection, xcb->screen->root, 1 );
            xcb_randr_get_monitors_reply_t  *mreply = xcb_randr_get_monitors_reply ( xcb->connection, t, NULL );
            if ( mreply ) {
                xcb_randr_monitor_info_iterator_t iter = xcb_randr_get_monitors_monitors_iterator ( mreply );
                while ( iter.rem > 0 ) {
                    workarea *w = x11_get_monitor_from_randr_monitor ( iter.data );
                    if ( w ) {
                        w->next       = xcb->monitors;
                        xcb->monitors = w;
                    }
                    xcb_randr_monitor_info_next ( &iter );
                }
                free ( mreply );
            }
        }
        free ( rversion );
    }
#endif

    // If no monitors found.
    if ( xcb->monitors == NULL ) {
        xcb_randr_get_screen_resources_current_reply_t  *res_reply;
        xcb_randr_get_screen_resources_current_cookie_t src;
        src       = xcb_randr_get_screen_resources_current ( xcb->connection, xcb->screen->root );
        res_reply = xcb_randr_get_screen_resources_current_reply ( xcb->connection, src, NULL );
        if ( !res_reply ) {
            return;  //just report error
        }
        int                mon_num = xcb_randr_get_screen_resources_current_outputs_length ( res_reply );
        xcb_randr_output_t *ops    = xcb_randr_get_screen_resources_current_outputs ( res_reply );

        // Get primary.
        xcb_randr_get_output_primary_cookie_t pc      = xcb_randr_get_output_primary ( xcb->connection, xcb->screen->root );
        xcb_randr_get_output_primary_reply_t  *pc_rep = xcb_randr_get_output_primary_reply ( xcb->connection, pc, NULL );

        for ( int i = mon_num - 1; i >= 0; i-- ) {
            workarea *w = x11_get_monitor_from_output ( ops[i] );
            if ( w ) {
                w->next       = xcb->monitors;
                xcb->monitors = w;
                if ( pc_rep && pc_rep->output == ops[i] ) {
                    w->primary = TRUE;
                }
            }
        }
        // If exists, free primary output reply.
        if ( pc_rep ) {
            free ( pc_rep );
        }
        free ( res_reply );
    }

    // Number monitor
    int index = 0;
    for ( workarea *iter = xcb->monitors; iter; iter = iter->next ) {
        iter->monitor_id = index++;
    }
}

void display_dump_monitor_layout ( void )
{
    int is_term = isatty ( fileno ( stdout ) );
    printf ( "Monitor layout:\n" );
    for ( workarea *iter = xcb->monitors; iter; iter = iter->next ) {
        printf ( "%s              ID%s: %d", ( is_term ) ? color_bold : "", is_term ? color_reset : "", iter->monitor_id );
        if ( iter->primary ) {
            printf ( " (primary)" );
        }
        printf ( "\n" );
        printf ( "%s            name%s: %s\n", ( is_term ) ? color_bold : "", is_term ? color_reset : "", iter->name );
        printf ( "%s        position%s: %d,%d\n", ( is_term ) ? color_bold : "", is_term ? color_reset : "", iter->x, iter->y );
        printf ( "%s            size%s: %d,%d\n", ( is_term ) ? color_bold : "", is_term ? color_reset : "", iter->w, iter->h );
        if ( iter->mw > 0 && iter->mh > 0 ) {
            printf ( "%s            size%s: %dmm,%dmm  dpi: %.0f,%.0f\n",
                     ( is_term ) ? color_bold : "",
                     is_term ? color_reset : "",
                     iter->mw,
                     iter->mh,
                     iter->w * 25.4 / (double) iter->mw,
                     iter->h * 25.4 / (double) iter->mh
                     );
        }
        printf ( "\n" );
    }
}

void display_startup_notification ( RofiHelperExecuteContext *context, GSpawnChildSetupFunc *child_setup, gpointer *user_data )
{
    if ( context == NULL ) {
        return;
    }

    SnLauncherContext *sncontext;

    sncontext = sn_launcher_context_new ( xcb->sndisplay, xcb->screen_nbr );

    sn_launcher_context_set_name ( sncontext, context->name );
    sn_launcher_context_set_description ( sncontext, context->description );
    if ( context->binary != NULL ) {
        sn_launcher_context_set_binary_name ( sncontext, context->binary );
    }
    if ( context->icon != NULL ) {
        sn_launcher_context_set_icon_name ( sncontext, context->icon );
    }
    if ( context->app_id != NULL ) {
        sn_launcher_context_set_application_id ( sncontext, context->app_id );
    }
    if ( context->wmclass != NULL ) {
        sn_launcher_context_set_wmclass ( sncontext, context->wmclass );
    }

    xcb_get_property_cookie_t c;
    unsigned int              current_desktop = 0;

    c = xcb_ewmh_get_current_desktop ( &xcb->ewmh, xcb->screen_nbr );
    if ( xcb_ewmh_get_current_desktop_reply ( &xcb->ewmh, c, &current_desktop, NULL ) ) {
        sn_launcher_context_set_workspace ( sncontext, current_desktop );
    }

    sn_launcher_context_initiate ( sncontext, "rofi", context->command, xcb->last_timestamp );

    *child_setup = (GSpawnChildSetupFunc) sn_launcher_context_setup_child_process;
    *user_data   = sncontext;
}

static int monitor_get_dimension ( int monitor_id, workarea *mon )
{
    memset ( mon, 0, sizeof ( workarea ) );
    mon->w = xcb->screen->width_in_pixels;
    mon->h = xcb->screen->height_in_pixels;

    workarea *iter = NULL;
    for ( iter = xcb->monitors; iter; iter = iter->next ) {
        if ( iter->monitor_id == monitor_id ) {
            *mon = *iter;
            return TRUE;
        }
    }
    return FALSE;
}
// find the dimensions of the monitor displaying point x,y
static void monitor_dimensions ( int x, int y, workarea *mon )
{
    memset ( mon, 0, sizeof ( workarea ) );
    mon->w = xcb->screen->width_in_pixels;
    mon->h = xcb->screen->height_in_pixels;

    for ( workarea *iter = xcb->monitors; iter; iter = iter->next ) {
        if ( INTERSECT ( x, y, iter->x, iter->y, iter->w, iter->h ) ) {
            *mon = *iter;
            break;
        }
    }
}

/**
 * @param root The X11 window used to find the pointer position. Usually the root window.
 * @param x    The x position of the mouse [out]
 * @param y    The y position of the mouse [out]
 *
 * find mouse pointer location
 *
 * @returns TRUE when found, FALSE otherwise
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
        return TRUE;
    }

    return FALSE;
}
static int monitor_active_from_winid ( xcb_drawable_t id, workarea *mon )
{
    xcb_window_t              root = xcb->screen->root;
    xcb_get_geometry_cookie_t c    = xcb_get_geometry ( xcb->connection, id );
    xcb_get_geometry_reply_t  *r   = xcb_get_geometry_reply ( xcb->connection, c, NULL );
    if ( r ) {
        xcb_translate_coordinates_cookie_t ct = xcb_translate_coordinates ( xcb->connection, id, root, r->x, r->y );
        xcb_translate_coordinates_reply_t  *t = xcb_translate_coordinates_reply ( xcb->connection, ct, NULL );
        if ( t ) {
            // place the menu above the window
            // if some window is focused, place menu above window, else fall
            // back to selected monitor.
            mon->x = t->dst_x - r->x;
            mon->y = t->dst_y - r->y;
            mon->w = r->width;
            mon->h = r->height;
            free ( r );
            free ( t );
            return TRUE;
        }
        free ( r );
    }
    return FALSE;
}
static int monitor_active_from_id_focused ( int mon_id, workarea *mon )
{
    int                       retv = FALSE;
    xcb_window_t              active_window;
    xcb_get_property_cookie_t awc;
    awc = xcb_ewmh_get_active_window ( &xcb->ewmh, xcb->screen_nbr );
    if ( !xcb_ewmh_get_active_window_reply ( &xcb->ewmh, awc, &active_window, NULL ) ) {
        g_debug ( "Failed to get active window, falling back to mouse location (-5)." );
        return retv;
    }
    xcb_query_tree_cookie_t tree_cookie = xcb_query_tree ( xcb->connection, active_window );
    xcb_query_tree_reply_t  *tree_reply = xcb_query_tree_reply ( xcb->connection, tree_cookie, NULL );
    if ( !tree_reply ) {
        g_debug ( "Failed to get parent window, falling back to mouse location (-5)." );
        return retv;
    }
    // get geometry.
    xcb_get_geometry_cookie_t c  = xcb_get_geometry ( xcb->connection, active_window );
    xcb_get_geometry_reply_t  *r = xcb_get_geometry_reply ( xcb->connection, c, NULL );
    if ( !r ) {
        g_debug ( "Failed to get geometry of active window, falling back to mouse location (-5)." );
        free ( tree_reply );
        return retv;
    }
    xcb_translate_coordinates_cookie_t ct = xcb_translate_coordinates ( xcb->connection, tree_reply->parent, r->root, r->x, r->y );
    xcb_translate_coordinates_reply_t  *t = xcb_translate_coordinates_reply ( xcb->connection, ct, NULL );
    if ( t ) {
        if ( mon_id == -2 ) {
            // place the menu above the window
            // if some window is focused, place menu above window, else fall
            // back to selected monitor.
            mon->x = t->dst_x - r->x;
            mon->y = t->dst_y - r->y;
            mon->w = r->width;
            mon->h = r->height;
            retv   = TRUE;
        }
        else if ( mon_id == -4 ) {
            monitor_dimensions ( t->dst_x, t->dst_y, mon );
            retv = TRUE;
        }
        free ( t );
    }
    else {
        g_debug ( "Failed to get translate position of active window, falling back to mouse location (-5)." );
    }
    free ( r );
    free ( tree_reply );
    return retv;
}
static int monitor_active_from_id ( int mon_id, workarea *mon )
{
    xcb_window_t root = xcb->screen->root;
    int          x, y;
    // At mouse position.
    if ( mon_id == -3 ) {
        if ( pointer_get ( root, &x, &y ) ) {
            monitor_dimensions ( x, y, mon );
            mon->x = x;
            mon->y = y;
            return TRUE;
        }
    }
    // Focused monitor
    else if ( mon_id == -1 ) {
        // Get the current desktop.
        unsigned int              current_desktop = 0;
        xcb_get_property_cookie_t gcdc;
        gcdc = xcb_ewmh_get_current_desktop ( &xcb->ewmh, xcb->screen_nbr );
        if  ( xcb_ewmh_get_current_desktop_reply ( &xcb->ewmh, gcdc, &current_desktop, NULL ) ) {
            xcb_get_property_cookie_t             c = xcb_ewmh_get_desktop_viewport ( &xcb->ewmh, xcb->screen_nbr );
            xcb_ewmh_get_desktop_viewport_reply_t vp;
            if ( xcb_ewmh_get_desktop_viewport_reply ( &xcb->ewmh, c, &vp, NULL ) ) {
                if ( current_desktop < vp.desktop_viewport_len ) {
                    monitor_dimensions ( vp.desktop_viewport[current_desktop].x,
                                         vp.desktop_viewport[current_desktop].y, mon );
                    xcb_ewmh_get_desktop_viewport_reply_wipe ( &vp );
                    return TRUE;
                }
                else {
                    g_debug ( "Viewport does not exist for current desktop: %d, falling back to mouse location (-5)", current_desktop );
                }
                xcb_ewmh_get_desktop_viewport_reply_wipe ( &vp );
            }
            else {
                g_debug ( "Failed to get viewport for current desktop: %d, falling back to mouse location (-5).", current_desktop );
            }
        }
        else {
            g_debug ( "Failed to get current desktop, falling back to mouse location (-5)." );
        }
    }
    else if ( mon_id == -2 || mon_id == -4 ) {
        if ( monitor_active_from_id_focused ( mon_id, mon ) ) {
            return TRUE;
        }
    }
    // Monitor that has mouse pointer.
    else if ( mon_id == -5 ) {
        if ( pointer_get ( root, &x, &y ) ) {
            monitor_dimensions ( x, y, mon );
            return TRUE;
        }
        // This is our give up point.
        return FALSE;
    }
    g_debug ( "Failed to find monitor, fall back to monitor showing mouse." );
    return monitor_active_from_id ( -5, mon );
}

// determine which monitor holds the active window, or failing that the mouse pointer
int monitor_active ( workarea *mon )
{
    if ( config.monitor != NULL ) {
        for ( workarea *iter = xcb->monitors; iter; iter = iter->next ) {
            if ( g_strcmp0 ( config.monitor, iter->name ) == 0 ) {
                *mon = *iter;
                return TRUE;
            }
        }
    }
    // Grab primary.
    if ( g_strcmp0 ( config.monitor, "primary" ) == 0 ) {
        for ( workarea *iter = xcb->monitors; iter; iter = iter->next ) {
            if ( iter->primary ) {
                *mon = *iter;
                return TRUE;
            }
        }
    }
    if ( g_str_has_prefix ( config.monitor, "wid:" ) ) {
        char           *end = NULL;
        xcb_drawable_t win  = g_ascii_strtoll ( config.monitor + 4, &end, 0 );
        if ( end != config.monitor ) {
            if ( monitor_active_from_winid ( win, mon ) ) {
                return TRUE;
            }
        }
    }
    {
        // IF fail, fall back to classic mode.
        char   *end   = NULL;
        gint64 mon_id = g_ascii_strtoll ( config.monitor, &end, 0 );
        if ( end != config.monitor ) {
            if ( mon_id >= 0 ) {
                if ( monitor_get_dimension ( mon_id, mon ) ) {
                    return TRUE;
                }
                g_warning ( "Failed to find selected monitor." );
            }
            else {
                return monitor_active_from_id ( mon_id, mon );
            }
        }
    }
    // Fallback.
    monitor_dimensions ( 0, 0, mon );
    return FALSE;
}

/**
 * @param state Internal state of the menu.
 * @param xse   X selection event.
 *
 * Handle paste event.
 */
static void rofi_view_paste ( RofiViewState *state, xcb_selection_notify_event_t *xse )
{
    if ( xse->property == XCB_ATOM_NONE ) {
        g_warning ( "Failed to convert selection" );
    }
    else if ( xse->property == xcb->ewmh.UTF8_STRING ) {
        gchar *text = window_get_text_prop ( xse->requestor, xcb->ewmh.UTF8_STRING );
        if ( text != NULL && text[0] != '\0' ) {
            unsigned int dl = strlen ( text );
            // Strip new line
            for ( unsigned int i = 0; i < dl; i++ ) {
                if ( text[i] == '\n' ) {
                    text[i] = '\0';
                }
            }
            rofi_view_handle_text ( state, text );
        }
        g_free ( text );
    }
    else {
        g_warning ( "Failed" );
    }
}

static gboolean x11_button_to_nk_bindings_button ( guint32 x11_button, NkBindingsMouseButton *button )
{
    switch ( x11_button )
    {
    case 1:
        *button = NK_BINDINGS_MOUSE_BUTTON_PRIMARY;
        break;
    case 3:
        *button = NK_BINDINGS_MOUSE_BUTTON_SECONDARY;
        break;
    case 2:
        *button = NK_BINDINGS_MOUSE_BUTTON_MIDDLE;
        break;
    case 8:
        *button = NK_BINDINGS_MOUSE_BUTTON_BACK;
        break;
    case 9:
        *button = NK_BINDINGS_MOUSE_BUTTON_FORWARD;
        break;
    case 4:
    case 5:
    case 6:
    case 7:
        return FALSE;
    default:
        *button = NK_BINDINGS_MOUSE_BUTTON_EXTRA + x11_button;
    }
    return TRUE;
}

static gboolean x11_button_to_nk_bindings_scroll ( guint32 x11_button, NkBindingsScrollAxis *axis, gint32 *steps )
{
    *steps = 1;
    switch ( x11_button )
    {
    case 4:
        *steps = -1;
    /* fallthrough */
    case 5:
        *axis = NK_BINDINGS_SCROLL_AXIS_VERTICAL;
        break;
    case 6:
        *steps = -1;
    /* fallthrough */
    case 7:
        *axis = NK_BINDINGS_SCROLL_AXIS_HORIZONTAL;
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

/**
 * Process X11 events in the main-loop (gui-thread) of the application.
 */
static void main_loop_x11_event_handler_view ( xcb_generic_event_t *event )
{
    RofiViewState *state = rofi_view_get_active ();
    if ( state == NULL ) {
        return;
    }

    switch ( event->response_type & ~0x80 )
    {
    case XCB_CLIENT_MESSAGE:
    {
        xcb_client_message_event_t *cme = (xcb_client_message_event_t *) event;
        xcb_atom_t atom = cme->data.data32[0];
        xcb_timestamp_t time = cme->data.data32[1];
        if ( atom == netatoms[WM_TAKE_FOCUS] )
        {
            xcb_set_input_focus ( xcb->connection, XCB_INPUT_FOCUS_NONE, cme->window, time );
            xcb_flush ( xcb->connection );
        }
        break;
    }
    case XCB_EXPOSE:
        rofi_view_frame_callback ();
        break;
    case XCB_CONFIGURE_NOTIFY:
    {
        xcb_configure_notify_event_t *xce = (xcb_configure_notify_event_t *) event;
        rofi_view_temp_configure_notify ( state, xce );
        break;
    }
    case XCB_MOTION_NOTIFY:
    {
        xcb_motion_notify_event_t *xme        = (xcb_motion_notify_event_t *) event;
        gboolean                  button_mask = xme->state & XCB_EVENT_MASK_BUTTON_1_MOTION;
        if (  button_mask && config.click_to_exit == TRUE ) {
            xcb->mouse_seen = TRUE;
        }
        rofi_view_handle_mouse_motion ( state, xme->event_x, xme->event_y, !button_mask );
        break;
    }
    case XCB_BUTTON_PRESS:
    {
        xcb_button_press_event_t *bpe = (xcb_button_press_event_t *) event;
        NkBindingsMouseButton    button;
        NkBindingsScrollAxis     axis;
        gint32                   steps;

        xcb->last_timestamp = bpe->time;
        rofi_view_handle_mouse_motion ( state, bpe->event_x, bpe->event_y, FALSE );
        if ( x11_button_to_nk_bindings_button ( bpe->detail, &button ) ) {
            nk_bindings_seat_handle_button ( xcb->bindings_seat, NULL, button, NK_BINDINGS_BUTTON_STATE_PRESS, bpe->time );
        }
        else if ( x11_button_to_nk_bindings_scroll ( bpe->detail, &axis, &steps ) ) {
            nk_bindings_seat_handle_scroll ( xcb->bindings_seat, NULL, axis, steps );
        }
        break;
    }
    case XCB_BUTTON_RELEASE:
    {
        xcb_button_release_event_t *bre = (xcb_button_release_event_t *) event;
        NkBindingsMouseButton      button;

        xcb->last_timestamp = bre->time;
        if ( x11_button_to_nk_bindings_button ( bre->detail, &button ) ) {
            nk_bindings_seat_handle_button ( xcb->bindings_seat, NULL, button, NK_BINDINGS_BUTTON_STATE_RELEASE, bre->time );
        }
        if ( config.click_to_exit == TRUE ) {
            if ( !xcb->mouse_seen ) {
                rofi_view_temp_click_to_exit ( state, bre->event );
            }
            xcb->mouse_seen = FALSE;
        }
        break;
    }
    // Paste event.
    case XCB_SELECTION_NOTIFY:
        rofi_view_paste ( state, (xcb_selection_notify_event_t *) event );
        break;
    case XCB_KEYMAP_NOTIFY:
    {
        xcb_keymap_notify_event_t *kne = (xcb_keymap_notify_event_t *) event;
        for ( gint32 by = 0; by < 31; ++by ) {
            for ( gint8 bi = 0; bi < 7; ++bi ) {
                if ( kne->keys[by] & ( 1 << bi ) ) {
                    // X11 keycodes starts at 8
                    nk_bindings_seat_handle_key ( xcb->bindings_seat, NULL, ( 8 * by + bi ) + 8, NK_BINDINGS_KEY_STATE_PRESSED );
                }
            }
        }
        break;
    }
    case XCB_KEY_PRESS:
    {
        xcb_key_press_event_t *xkpe = (xcb_key_press_event_t *) event;
        gchar                 *text;

        xcb->last_timestamp = xkpe->time;
        text                = nk_bindings_seat_handle_key_with_modmask ( xcb->bindings_seat, NULL, xkpe->state, xkpe->detail, NK_BINDINGS_KEY_STATE_PRESS );
        if ( text != NULL ) {
            rofi_view_handle_text ( state, text );
        }
        break;
    }
    case XCB_KEY_RELEASE:
    {
        xcb_key_release_event_t *xkre = (xcb_key_release_event_t *) event;
        xcb->last_timestamp = xkre->time;
        nk_bindings_seat_handle_key ( xcb->bindings_seat, NULL, xkre->detail, NK_BINDINGS_KEY_STATE_RELEASE );
        break;
    }
    default:
        break;
    }
    rofi_view_maybe_update ( state );
}

static gboolean main_loop_x11_event_handler ( xcb_generic_event_t *ev, G_GNUC_UNUSED gpointer user_data )
{
    if ( ev == NULL ) {
        int status = xcb_connection_has_error ( xcb->connection );
        if ( status > 0 ) {
            g_warning ( "The XCB connection to X server had a fatal error: %d", status );
            g_main_loop_quit ( xcb->main_loop );
            return G_SOURCE_REMOVE;
        }
        else {
            g_warning ( "main_loop_x11_event_handler: ev == NULL, status == %d", status );
            return G_SOURCE_CONTINUE;
        }
    }
    uint8_t type = ev->response_type & ~0x80;
    if ( type == xcb->xkb.first_event ) {
        switch ( ev->pad0 )
        {
        case XCB_XKB_MAP_NOTIFY:
        {
            struct xkb_keymap *keymap = xkb_x11_keymap_new_from_device ( nk_bindings_seat_get_context ( xcb->bindings_seat ), xcb->connection, xcb->xkb.device_id, 0 );
            struct xkb_state  *state  = xkb_x11_state_new_from_device ( keymap, xcb->connection, xcb->xkb.device_id );
            nk_bindings_seat_update_keymap ( xcb->bindings_seat, keymap, state );
            xkb_keymap_unref ( keymap );
            xkb_state_unref ( state );
            break;
        }
        case XCB_XKB_STATE_NOTIFY:
        {
            xcb_xkb_state_notify_event_t *ksne = (xcb_xkb_state_notify_event_t *) ev;
            nk_bindings_seat_update_mask ( xcb->bindings_seat, NULL,
                                           ksne->baseMods,
                                           ksne->latchedMods,
                                           ksne->lockedMods,
                                           ksne->baseGroup,
                                           ksne->latchedGroup,
                                           ksne->lockedGroup );
            rofi_view_maybe_update ( rofi_view_get_active () );
            break;
        }
        }
        return G_SOURCE_CONTINUE;
    }
    if ( xcb->sndisplay != NULL ) {
        sn_xcb_display_process_event ( xcb->sndisplay, ev );
    }
    main_loop_x11_event_handler_view ( ev );
    return G_SOURCE_CONTINUE;
}

void rofi_xcb_set_input_focus ( xcb_window_t w )
{
    if ( config.steal_focus != TRUE ) {
        xcb->focus_revert = 0;
        return;
    }
    xcb_generic_error_t *error;
    xcb_get_input_focus_reply_t *freply;
    xcb_get_input_focus_cookie_t fcookie = xcb_get_input_focus ( xcb->connection );
    freply = xcb_get_input_focus_reply ( xcb->connection, fcookie, &error );
    if ( error != NULL ) {
        g_warning ( "Could not get input focus (error %d), will revert focus to best effort", error->error_code );
        free ( error );
        xcb->focus_revert = 0;
    } else {
        xcb->focus_revert = freply->focus;
    }
    xcb_set_input_focus ( xcb->connection, XCB_INPUT_FOCUS_POINTER_ROOT, w, XCB_CURRENT_TIME );
    xcb_flush ( xcb->connection );
}

void rofi_xcb_revert_input_focus (void)
{
    if ( xcb->focus_revert == 0 )
        return;

    xcb_set_input_focus ( xcb->connection, XCB_INPUT_FOCUS_POINTER_ROOT, xcb->focus_revert, XCB_CURRENT_TIME );
    xcb_flush ( xcb->connection );
}

static int take_pointer ( xcb_window_t w, int iters )
{
    int i = 0;
    while ( TRUE ) {
        if ( xcb_connection_has_error ( xcb->connection ) ) {
            g_warning ( "Connection has error" );
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
        if ( ( ++i ) > iters ) {
            break;
        }
        struct timespec del = {
            .tv_sec  = 0,
            .tv_nsec = 1000000
        };
        nanosleep ( &del, NULL );
    }
    return 0;
}

static int take_keyboard ( xcb_window_t w, int iters )
{
    int i = 0;
    while ( TRUE ) {
        if ( xcb_connection_has_error ( xcb->connection ) ) {
            g_warning ( "Connection has error" );
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
        if ( ( ++i ) > iters ) {
            break;
        }
        struct timespec del = {
            .tv_sec  = 0,
            .tv_nsec = 1000000
        };
        nanosleep ( &del, NULL );
    }
    return 0;
}

static void release_keyboard ( void )
{
    xcb_ungrab_keyboard ( xcb->connection, XCB_CURRENT_TIME );
}
static void release_pointer ( void )
{
    xcb_ungrab_pointer ( xcb->connection, XCB_CURRENT_TIME );
}

/** X server error depth. to handle nested errors. */
static int error_trap_depth = 0;
static void error_trap_push ( G_GNUC_UNUSED SnDisplay *display, G_GNUC_UNUSED xcb_connection_t *xdisplay )
{
    ++error_trap_depth;
}

static void error_trap_pop ( G_GNUC_UNUSED SnDisplay *display, xcb_connection_t *xdisplay )
{
    if ( error_trap_depth == 0 ) {
        g_warning ( "Error trap underflow!" );
        exit ( EXIT_FAILURE );
    }

    xcb_flush ( xdisplay );
    --error_trap_depth;
}

/**
 * Fill in the list of frequently used X11 Atoms.
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

static void x11_helper_discover_window_manager ( void )
{
    xcb_window_t              wm_win = 0;
    xcb_get_property_cookie_t cc     = xcb_ewmh_get_supporting_wm_check_unchecked ( &xcb->ewmh,
                                                                                    xcb_stuff_get_root_window () );

    if ( xcb_ewmh_get_supporting_wm_check_reply ( &xcb->ewmh, cc, &wm_win, NULL ) ) {
        xcb_ewmh_get_utf8_strings_reply_t wtitle;
        xcb_get_property_cookie_t         cookie = xcb_ewmh_get_wm_name_unchecked ( &( xcb->ewmh ), wm_win );
        if (  xcb_ewmh_get_wm_name_reply ( &( xcb->ewmh ), cookie, &wtitle, (void *) 0 ) ) {
            if ( wtitle.strings_len > 0 ) {
                g_debug ( "Found window manager: %s", wtitle.strings );
                if ( g_strcmp0 ( wtitle.strings, "i3" ) == 0 ) {
                    current_window_manager = WM_DO_NOT_CHANGE_CURRENT_DESKTOP | WM_PANGO_WORKSPACE_NAMES;
                }
            }
            xcb_ewmh_get_utf8_strings_reply_wipe ( &wtitle );
        }
    }
}

gboolean display_setup ( GMainLoop *main_loop, NkBindings *bindings )
{
    // Get DISPLAY, first env, then argument.
    // We never modify display_str content.
    char *display_str = ( char *) g_getenv ( "DISPLAY" );
    find_arg_str (  "-display", &display_str );

    xcb->main_loop = main_loop;
    xcb->source    = g_water_xcb_source_new ( g_main_loop_get_context ( xcb->main_loop ), display_str, &xcb->screen_nbr, main_loop_x11_event_handler, NULL, NULL );
    if ( xcb->source == NULL ) {
        g_warning ( "Failed to open display: %s", display_str );
        return FALSE;
    }
    xcb->connection = g_water_xcb_source_get_connection ( xcb->source );

    TICK_N ( "Open Display" );

    xcb->screen = xcb_aux_get_screen ( xcb->connection, xcb->screen_nbr );

    x11_build_monitor_layout ();

    xcb_intern_atom_cookie_t *ac     = xcb_ewmh_init_atoms ( xcb->connection, &xcb->ewmh );
    xcb_generic_error_t      *errors = NULL;
    xcb_ewmh_init_atoms_replies ( &xcb->ewmh, ac, &errors );
    if ( errors ) {
        g_warning ( "Failed to create EWMH atoms" );
        free ( errors );
    }
    // Discover the current active window manager.
    x11_helper_discover_window_manager ();
    TICK_N ( "Setup XCB" );

    if ( xkb_x11_setup_xkb_extension ( xcb->connection, XKB_X11_MIN_MAJOR_XKB_VERSION, XKB_X11_MIN_MINOR_XKB_VERSION,
                                       XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS, NULL, NULL, &xcb->xkb.first_event, NULL ) < 0 ) {
        g_warning ( "cannot setup XKB extension!" );
        return FALSE;
    }

    xcb->xkb.device_id = xkb_x11_get_core_keyboard_device_id ( xcb->connection );

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
    xcb_xkb_select_events ( xcb->connection, xcb->xkb.device_id, required_events, /* affectWhich */
                            0,                                                    /* clear */
                            required_events,                                      /* selectAll */
                            required_map_parts,                                   /* affectMap */
                            required_map_parts,                                   /* map */
                            &details );

    xcb->bindings_seat = nk_bindings_seat_new ( bindings, XKB_CONTEXT_NO_FLAGS );
    struct xkb_keymap *keymap = xkb_x11_keymap_new_from_device ( nk_bindings_seat_get_context ( xcb->bindings_seat ), xcb->connection, xcb->xkb.device_id, XKB_KEYMAP_COMPILE_NO_FLAGS );
    if ( keymap == NULL ) {
        g_warning ( "Failed to get Keymap for current keyboard device." );
        return FALSE;
    }
    struct xkb_state *state = xkb_x11_state_new_from_device ( keymap, xcb->connection, xcb->xkb.device_id );
    if ( state == NULL ) {
        g_warning ( "Failed to get state object for current keyboard device." );
        return FALSE;
    }

    nk_bindings_seat_update_keymap ( xcb->bindings_seat, keymap, state );
    xkb_state_unref ( state );
    xkb_keymap_unref ( keymap );

    // determine numlock mask so we can bind on keys with and without it
    x11_create_frequently_used_atoms (  );

    if ( xcb_connection_has_error ( xcb->connection ) ) {
        g_warning ( "Connection has error" );
        return FALSE;
    }

    // startup not.
    xcb->sndisplay = sn_xcb_display_new ( xcb->connection, error_trap_push, error_trap_pop );
    if ( xcb_connection_has_error ( xcb->connection ) ) {
        g_warning ( "Connection has error" );
        return FALSE;
    }

    if ( xcb->sndisplay != NULL ) {
        xcb->sncontext = sn_launchee_context_new_from_environment ( xcb->sndisplay, xcb->screen_nbr );
    }
    if ( xcb_connection_has_error ( xcb->connection ) ) {
        g_warning ( "Connection has error" );
        return FALSE;
    }

    return TRUE;
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

/** Retry count of grabbing keyboard. */
unsigned int lazy_grab_retry_count_kb = 0;
/** Retry count of grabbing pointer. */
unsigned int lazy_grab_retry_count_pt = 0;
static gboolean lazy_grab_pointer ( G_GNUC_UNUSED gpointer data )
{
    // After 5 sec.
    if ( lazy_grab_retry_count_pt > ( 5 * 1000 ) ) {
        g_warning ( "Failed to grab pointer after %u times. Giving up.", lazy_grab_retry_count_pt );
        return G_SOURCE_REMOVE;
    }
    if ( take_pointer ( xcb_stuff_get_root_window (), 0 ) ) {
        return G_SOURCE_REMOVE;
    }
    lazy_grab_retry_count_pt++;
    return G_SOURCE_CONTINUE;
}
static gboolean lazy_grab_keyboard ( G_GNUC_UNUSED gpointer data )
{
    // After 5 sec.
    if ( lazy_grab_retry_count_kb > ( 5 * 1000 ) ) {
        g_warning ( "Failed to grab keyboard after %u times. Giving up.", lazy_grab_retry_count_kb );
        g_main_loop_quit ( xcb->main_loop );
        return G_SOURCE_REMOVE;
    }
    if ( take_keyboard ( xcb_stuff_get_root_window (), 0 ) ) {
        return G_SOURCE_REMOVE;
    }
    lazy_grab_retry_count_kb++;
    return G_SOURCE_CONTINUE;
}

gboolean display_late_setup ( void )
{
    x11_create_visual_and_colormap ();

    /**
     * Create window (without showing)
     */
    // Try to grab the keyboard as early as possible.
    // We grab this using the rootwindow (as dmenu does it).
    // this seems to result in the smallest delay for most people.
    if ( find_arg ( "-normal-window" ) >= 0 ) {
        return TRUE;
    }
    if ( find_arg ( "-no-lazy-grab" ) >= 0 ) {
        if ( !take_keyboard ( xcb_stuff_get_root_window (), 500 ) ) {
            g_warning ( "Failed to grab keyboard, even after %d uS.", 500 * 1000 );
            return FALSE;
        }
        if ( !take_pointer ( xcb_stuff_get_root_window (), 100 ) ) {
            g_warning ( "Failed to grab mouse pointer, even after %d uS.", 100 * 1000 );
        }
    }
    else {
        if ( !take_keyboard ( xcb_stuff_get_root_window (), 0 ) ) {
            g_timeout_add ( 1, lazy_grab_keyboard, NULL );
        }
        if ( !take_pointer ( xcb_stuff_get_root_window (), 0 ) ) {
            g_timeout_add ( 1, lazy_grab_pointer, NULL );
        }
    }
    return TRUE;
}

xcb_window_t xcb_stuff_get_root_window ( void )
{
    return xcb->screen->root;
}

void display_early_cleanup ( void )
{
    release_keyboard ( );
    release_pointer ( );
    xcb_flush ( xcb->connection );
}

void display_cleanup ( void )
{
    if ( xcb->connection == NULL ) {
        return;
    }

    g_debug ( "Cleaning up XCB and XKB" );

    nk_bindings_seat_free ( xcb->bindings_seat );
    if ( xcb->sncontext != NULL ) {
        sn_launchee_context_unref ( xcb->sncontext );
        xcb->sncontext = NULL;
    }
    if ( xcb->sndisplay != NULL ) {
        sn_display_unref ( xcb->sndisplay );
        xcb->sndisplay = NULL;
    }
    x11_monitors_free ();
    xcb_ewmh_connection_wipe ( &( xcb->ewmh ) );
    xcb_flush ( xcb->connection );
    xcb_aux_sync ( xcb->connection );
    g_water_xcb_source_free ( xcb->source );
    xcb->source     = NULL;
    xcb->connection = NULL;
    xcb->screen     = NULL;
    xcb->screen_nbr = 0;
}

void x11_disable_decoration ( xcb_window_t window )
{
    // Flag used to indicate we are setting the decoration type.
    const uint32_t MWM_HINTS_DECORATIONS = ( 1 << 1 );
    // Motif property data structure
    struct MotifWMHints
    {
        uint32_t flags;
        uint32_t functions;
        uint32_t decorations;
        int32_t  inputMode;
        uint32_t state;
    };

    struct MotifWMHints hints;
    hints.flags       = MWM_HINTS_DECORATIONS;
    hints.decorations = 0;
    hints.functions   = 0;
    hints.inputMode   = 0;
    hints.state       = 0;

    xcb_atom_t ha = netatoms[_MOTIF_WM_HINTS];
    xcb_change_property ( xcb->connection, XCB_PROP_MODE_REPLACE, window, ha, ha, 32, 5, &hints );
}
