/**
 * rofi
 *
 * MIT/X11 License
 * Modified 2016-2017 Qball Cow <qball@gmpclient.org>
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
#include "widgets/container.h"
#include "theme.h"

#define LOG_DOMAIN    "Widgets.Window"

struct _window
{
    widget widget;
    widget *child;
};

static void container_update ( widget *wid  );

static int container_get_desired_height ( widget *widget )
{
    container *b     = (container *) widget;
    int       height = 0;
    if ( b->child ) {
        height += widget_get_desired_height ( b->child );
    }
    height += widget_padding_get_padding_height ( widget );
    return height;
}

static void container_draw ( widget *wid, cairo_t *draw )
{
    container *b = (container *) wid;

    widget_draw ( b->child, draw );
}

static void container_free ( widget *wid )
{
    container *b = (container *) wid;

    widget_free ( b->child );
    g_free ( b );
}

void container_add ( container *container, widget *child )
{
    if ( container == NULL ) {
        return;
    }
    container->child = child;
    child->parent    = WIDGET ( container  );
    widget_update ( WIDGET ( container ) );
}

static void container_resize ( widget *widget, short w, short h )
{
    container *b = (container *) widget;
    if ( b->widget.w != w || b->widget.h != h ) {
        b->widget.w = w;
        b->widget.h = h;
        widget_update ( widget );
    }
}

static gboolean container_clicked ( widget *wid, widget_button_event *be, G_GNUC_UNUSED void *udata )
{
    container *b = (container *) wid;
    if ( widget_intersect ( b->child, be->x, be->y ) ) {
        widget_button_event rel = *be;
        rel.x -= b->child->x;
        rel.y -= b->child->y;
        return widget_clicked ( b->child, &rel );
    }
    return FALSE;
}
static gboolean container_motion_notify ( widget *wid, widget_motion_event *me )
{
    container *b = (container *) wid;
    if ( widget_intersect ( b->child, me->x, me->y ) ) {
        widget_motion_event rel = *me;
        rel.x -= b->child->x;
        rel.y -= b->child->y;
        return widget_motion_notify ( b->child, &rel );
    }
    return FALSE;
}

container * container_create ( const char *name )
{
    container *b = g_malloc0 ( sizeof ( container ) );
    // Initialize widget.
    widget_init ( WIDGET ( b ), name );
    b->widget.draw               = container_draw;
    b->widget.free               = container_free;
    b->widget.resize             = container_resize;
    b->widget.update             = container_update;
    b->widget.clicked            = container_clicked;
    b->widget.motion_notify      = container_motion_notify;
    b->widget.get_desired_height = container_get_desired_height;
    b->widget.enabled            = TRUE;
    return b;
}

static void container_update ( widget *wid  )
{
    container *b = (container *) wid;
    if ( b->child && b->child->enabled ) {
        widget_resize ( WIDGET ( b->child ),
                        widget_padding_get_remaining_width  ( WIDGET ( b ) ),
                        widget_padding_get_remaining_height ( WIDGET ( b ) )
                        );
        widget_move ( WIDGET ( b->child ),
                      widget_padding_get_left ( WIDGET ( b ) ),
                      widget_padding_get_top ( WIDGET ( b ) )
                      );
    }
}
