/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2018 Qball Cow <qball@gmpclient.org>
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

/** The log domain of this widget. */
#define G_LOG_DOMAIN    "Widgets.Icon"

#include <config.h>
#include <stdio.h>
#include "widgets/widget.h"
#include "widgets/widget-internal.h"
#include "widgets/icon.h"
#include "theme.h"

#include "rofi-icon-fetcher.h"

struct _icon
{
    widget          widget;

    // Size of the icon.
    int             size;

    uint32_t        icon_fetch_id;

    double          yalign,xalign;

    // Source surface.
    cairo_surface_t *icon;
};

static int icon_get_desired_height ( widget *widget )
{
    icon *b     = (icon *) widget;
    int  height = b->size;
    height += widget_padding_get_padding_height ( widget );
    return height;
}
static int icon_get_desired_width ( widget *widget )
{
    icon *b    = (icon *) widget;
    int  width = b->size;
    width += widget_padding_get_padding_width ( widget );
    return width;
}

static void icon_draw ( widget *wid, cairo_t *draw )
{
    icon *b = (icon *) wid;
    // If no icon is loaded. quit.
    if ( b->icon == NULL && b->icon_fetch_id > 0 ) {
        b->icon = rofi_icon_fetcher_get ( b->icon_fetch_id );
        if ( b->icon ) {
            cairo_surface_reference ( b->icon );
        }
    }
    if ( b->icon == NULL  ) {
        return;
    }
    int    iconh = cairo_image_surface_get_height ( b->icon );
    int    iconw = cairo_image_surface_get_width ( b->icon );
    int    icons = MAX ( iconh, iconw );
    double scale = (double) b->size / icons;

    int lpad = widget_padding_get_left   ( WIDGET ( b ) ) ;
    int rpad = widget_padding_get_right  ( WIDGET ( b ) ) ;
    int tpad = widget_padding_get_top    ( WIDGET ( b ) ) ;
    int bpad = widget_padding_get_bottom ( WIDGET ( b ) ) ;

    cairo_save ( draw );

    cairo_translate ( draw,
            lpad + ( b->widget.w - iconw * scale - lpad -rpad )*b->xalign,
            tpad + ( b->widget.h- iconh * scale -tpad - bpad )*b->yalign );
    cairo_scale ( draw, scale, scale );
    cairo_set_source_surface ( draw, b->icon, 0, 0 );
    cairo_paint ( draw );
    cairo_restore ( draw );
}

static void icon_free ( widget *wid )
{
    icon *b = (icon *) wid;

    if ( b->icon ) {
        cairo_surface_destroy ( b->icon );
    }

    g_free ( b );
}

static void icon_resize ( widget *widget, short w, short h )
{
    icon *b = (icon *) widget;
    if ( b->widget.w != w || b->widget.h != h ) {
        b->widget.w = w;
        b->widget.h = h;
        widget_update ( widget );
    }
}

void icon_set_surface ( icon *icon, cairo_surface_t *surf )
{
    icon->icon_fetch_id = 0;
    if ( icon->icon ) {
        cairo_surface_destroy ( icon->icon );
        icon->icon = NULL;
    }
    if ( surf ) {
        cairo_surface_reference ( surf );
        icon->icon = surf;
    }
    widget_queue_redraw ( WIDGET ( icon ) );
}

icon * icon_create ( widget *parent, const char *name )
{
    icon *b = g_malloc0 ( sizeof ( icon ) );

    b->size = 16;
    // Initialize widget.
    widget_init ( WIDGET ( b ), parent, WIDGET_TYPE_UNKNOWN, name );
    b->widget.draw               = icon_draw;
    b->widget.free               = icon_free;
    b->widget.resize             = icon_resize;
    b->widget.get_desired_height = icon_get_desired_height;
    b->widget.get_desired_width  = icon_get_desired_width;

    RofiDistance d = rofi_theme_get_distance ( WIDGET (b), "size" , b->size );
    b->size = distance_get_pixel ( d, ROFI_ORIENTATION_VERTICAL );

    const char * filename = rofi_theme_get_string ( WIDGET ( b ), "filename", NULL );
    if ( filename ) {
        b->icon_fetch_id = rofi_icon_fetcher_query ( filename, b->size );
    }
    b->yalign = rofi_theme_get_double ( WIDGET ( b ), "vertical-align", 0.5 );
    b->yalign = MAX ( 0, MIN ( 1.0, b->yalign ) );
    b->xalign = rofi_theme_get_double ( WIDGET ( b ), "horizontal-align", 0.5 );
    b->xalign = MAX ( 0, MIN ( 1.0, b->xalign ) );

    return b;
}
