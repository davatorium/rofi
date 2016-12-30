/**
 * rofi
 *
 * MIT/X11 License
 * Modified 2016 Qball Cow <qball@gmpclient.org>
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
 */

#include <config.h>
#include <stdio.h>
#include "widgets/widget.h"
#include "widgets/widget-internal.h"
#include "widgets/window.h"
#include "theme.h"
#include "settings.h"

#define LOG_DOMAIN    "Widgets.Window"
const char *WINDOW_CLASS_NAME = "@window";

/**
 * @param window Handle to the window widget.
 * @param spacing The spacing to apply.
 *
 * Set the spacing to apply between the children in pixels.
 */
void window_set_spacing ( window * window, unsigned int spacing );

struct _window
{
    widget  widget;
    widget *child;
    int border_width;
};

static void window_update ( widget *wid  );


static int window_get_desired_height ( widget *widget )
{
    window *b = (window *) widget;
    int height = b->border_width*2;
    if ( b->child ) {
        height += widget_get_desired_height ( b->child );
    }
    return height;
}


static void window_draw ( widget *wid, cairo_t *draw )
{
    window *b = (window *) wid;

    cairo_save ( draw );
    rofi_theme_get_color ( "@window", "window" , NULL, "foreground", draw );
    cairo_set_line_width ( draw, b->border_width );
    cairo_rectangle ( draw,
            b->border_width / 2.0,
            b->border_width / 2.0,
            wid->w - b->border_width,
            wid->h - b->border_width );
    cairo_stroke ( draw );
    cairo_restore ( draw );
    widget_draw ( b->child, draw );
}

static void window_free ( widget *wid )
{
    window *b = (window *) wid;

    widget_free ( b->child );
    g_free ( b );
}

void window_add ( window *window, widget *child )
{
    if ( window == NULL ) {
        return;
    }
    window->child = child;
    child->parent = WIDGET ( window  );
    widget_update ( WIDGET ( window ) );
}

static void window_resize ( widget *widget, short w, short h )
{
    window *b = (window *) widget;
    if ( b->widget.w != w || b->widget.h != h ) {
        b->widget.w = w;
        b->widget.h = h;
        widget_update ( widget );
    }
}

static gboolean window_clicked ( widget *wid, xcb_button_press_event_t *xbe, G_GNUC_UNUSED void *udata )
{
    window *b = (window *) wid;
    if ( widget_intersect ( b->child, xbe->event_x, xbe->event_y ) ) {
        xcb_button_press_event_t rel = *xbe;
        rel.event_x -= b->child->x;
        rel.event_y -= b->child->y;
        return widget_clicked ( b->child, &rel );
    }
    return FALSE;
}
static gboolean window_motion_notify ( widget *wid, xcb_motion_notify_event_t *xme )
{
    window *b = (window *) wid;
    if ( widget_intersect ( b->child, xme->event_x, xme->event_y ) ) {
        xcb_motion_notify_event_t rel = *xme;
        rel.event_x -= b->child->x;
        rel.event_y -= b->child->y;
        return widget_motion_notify ( b->child, &rel );
    }
    return FALSE;
}

window * window_create ( const char *name )
{
    window *b = g_malloc0 ( sizeof ( window ) );
    // Initialize widget.
    widget_init ( WIDGET(b), name, WINDOW_CLASS_NAME);
    b->widget.draw               = window_draw;
    b->widget.free               = window_free;
    b->widget.resize             = window_resize;
    b->widget.update             = window_update;
    b->widget.clicked            = window_clicked;
    b->widget.motion_notify      = window_motion_notify;
    b->widget.get_desired_height = window_get_desired_height;
    b->widget.enabled            = TRUE;
    b->border_width = rofi_theme_get_integer (
            b->widget.class_name, b->widget.name, NULL,  "border-width" , config.menu_bw);

    return b;
}

static void window_update ( widget *wid  )
{
    window *b = (window *) wid;
    if ( b->child && b->child->enabled ){
        widget_resize ( WIDGET ( b->child ),
                widget_padding_get_remaining_width  (WIDGET(b))-2*b->border_width,
                widget_padding_get_remaining_height (WIDGET(b))-2*b->border_width
                );
        widget_move ( WIDGET ( b->child ),
                b->border_width+widget_padding_get_left (WIDGET(b)),
                b->border_width+widget_padding_get_top (WIDGET(b))
                );
    }
}

int window_get_border_width ( const window *window )
{
    return window->border_width*2;
}
