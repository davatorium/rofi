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

#define G_LOG_DOMAIN    "Widgets.Icon"

#include <config.h>
#include <stdio.h>
#include "widgets/widget.h"
#include "widgets/widget-internal.h"
#include "widgets/icon.h"
#include "theme.h"

struct _icon
{
    widget widget;

    // Size of the icon.
    int size;

    // Source surface.
    cairo_surface_t  *icon;
};


static int icon_get_desired_height ( widget *widget )
{
    icon *b     = (icon *) widget;
    int       height = b->size;
    height += widget_padding_get_padding_height ( widget );
    return height;
}
static int icon_get_desired_width ( widget *widget )
{
    icon *b     = (icon *) widget;
    int       width = b->size;
    width += widget_padding_get_padding_width ( widget );
    return width;
}

static void icon_draw ( widget *wid, cairo_t *draw )
{
    icon *b = (icon *) wid;
    // If no icon is loaded. quit.
    if ( b->icon == NULL ) {
        return;
    }
    int    iconh = cairo_image_surface_get_height ( b->icon );
    int    iconw = cairo_image_surface_get_width ( b->icon );
    int    icons = MAX ( iconh, iconw );
    double scale = (double) b->size/ icons;

    cairo_save ( draw );

    cairo_translate ( draw, ( b->size - iconw * scale ) / 2.0, ( b->size - iconh * scale ) / 2.0 );
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

    b->size                      = 16;
    // Initialize widget.
    widget_init ( WIDGET ( b ), parent, WIDGET_TYPE_UNKNOWN, name );
    b->widget.draw               = icon_draw;
    b->widget.free               = icon_free;
    b->widget.resize             = icon_resize;
    b->widget.get_desired_height = icon_get_desired_height;
    b->widget.get_desired_width  = icon_get_desired_width;


    b->size = rofi_theme_get_integer ( WIDGET ( b ), "size", b->size );

    const char * filename = rofi_theme_get_string ( WIDGET ( b ), "filename", NULL );
    if ( filename ) {
        b->icon = cairo_image_surface_create_from_png ( filename );
        if ( b->icon == NULL ) {
            g_warning("Failed to load icon: %s\n", filename);
        } else {
            if ( cairo_surface_status ( b->icon ) != CAIRO_STATUS_SUCCESS ) {
                g_warning ( "Failed to load icon: %s\n", filename );
                cairo_surface_destroy ( b->icon );
                b->icon = NULL;
            }
        }
    }


    return b;
}

