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
#include "widgets/box.h"

#define LOG_DOMAIN    "Widgets.Box"

struct _box
{
    widget  widget;
    boxType type;
    int     max_size;
    // Padding between elements
    int     padding;

    GList   *children;
};

static void box_update ( widget *wid  );

static void vert_calculate_size ( box *b )
{
    int expanding_widgets = 0;
    int active_widgets    = 0;
    b->max_size = 0;
    for ( GList *iter = g_list_first ( b->children ); iter != NULL; iter = g_list_next ( iter ) ) {
        widget * child = (widget *) iter->data;
        if ( !child->enabled ) {
            continue;
        }
        active_widgets++;
        if ( child->expand == TRUE ) {
            expanding_widgets++;
            continue;
        }
        b->max_size += child->h;
    }
    b->max_size += MAX ( 0, ( ( active_widgets - 1 ) * b->padding ) );
    if ( b->max_size > b->widget.h ) {
        g_log ( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Widgets to large (height) for box: %d %d", b->max_size, b->widget.h );
        return;
    }
    if ( active_widgets > 0 ) {
        int    bottom = b->widget.h;
        int    top    = 0;
        double rem    = b->widget.h - b->max_size;
        int    index  = 0;
        for ( GList *iter = g_list_first ( b->children ); iter != NULL; iter = g_list_next ( iter ) ) {
            widget * child = (widget *) iter->data;
            if ( child->enabled == FALSE ) {
                continue;
            }
            if ( child->expand == TRUE ) {
                // Re-calculate to avoid round issues leaving one pixel left.
                int expanding_widgets_size = ( rem ) / ( expanding_widgets - index );
                if ( child->end ) {
                    bottom -= expanding_widgets_size;
                    widget_move ( child, child->x, bottom );
                    widget_resize ( child, b->widget.w, expanding_widgets_size );
                    bottom -= b->padding;
                }
                else {
                    widget_move ( child, child->x, top );
                    top += expanding_widgets_size;
                    widget_resize ( child, b->widget.w, expanding_widgets_size );
                    top += b->padding;
                }
                rem -= expanding_widgets_size;
                index++;
            }
            else if ( child->end ) {
                bottom -= widget_get_height (  child );
                widget_move ( child, child->x, bottom );
                widget_resize ( child, b->widget.w, child->h );
                bottom -= b->padding;
            }
            else {
                widget_move ( child, child->x, top );
                top += widget_get_height (  child );
                widget_resize ( child, b->widget.w, child->h );
                top += b->padding;
            }
        }
    }
}
static void hori_calculate_size ( box *b )
{
    int expanding_widgets = 0;
    int active_widgets    = 0;
    b->max_size = 0;
    for ( GList *iter = g_list_first ( b->children ); iter != NULL; iter = g_list_next ( iter ) ) {
        widget * child = (widget *) iter->data;
        if ( !child->enabled ) {
            continue;
        }
        active_widgets++;
        if ( child->expand == TRUE ) {
            expanding_widgets++;
            continue;
        }
        // Size used by fixed width widgets.
        b->max_size += child->w;
    }
    b->max_size += MAX ( 0, ( ( active_widgets - 1 ) * b->padding ) );
    if ( b->max_size > b->widget.w ) {
        g_log ( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Widgets to large (width) for box: %d %d", b->max_size, b->widget.w );
        return;
    }
    if ( active_widgets > 0 ) {
        int    right = b->widget.w;
        int    left  = 0;
        double rem   = b->widget.w - b->max_size;
        int    index = 0;
        for ( GList *iter = g_list_first ( b->children ); iter != NULL; iter = g_list_next ( iter ) ) {
            widget * child = (widget *) iter->data;
            if ( child->enabled == FALSE  ) {
                continue;
            }
            if ( child->expand == TRUE ) {
                // Re-calculate to avoid round issues leaving one pixel left.
                int expanding_widgets_size = ( rem ) / ( expanding_widgets - index );
                if ( child->end ) {
                    right -= expanding_widgets_size;
                    widget_move ( child, right, child->y );
                    widget_resize ( child, expanding_widgets_size, b->widget.h );
                    right -= b->padding;
                }
                else {
                    widget_move ( child, left, child->y );
                    left += expanding_widgets_size;
                    widget_resize ( child, expanding_widgets_size, b->widget.h );
                    left += b->padding;
                }
                rem -= expanding_widgets_size;
                index++;
            }
            else if ( child->end ) {
                right -= widget_get_width (  child );
                widget_move ( child, right, child->y );
                widget_resize ( child, child->w, b->widget.h );
                right -= b->padding;
            }
            else {
                widget_move ( child, left, child->y );
                left += widget_get_width (  child );
                widget_resize ( child, child->w, b->widget.h );
                left += b->padding;
            }
        }
    }
}

static void box_draw ( widget *wid, cairo_t *draw )
{
    box *b = (box *) wid;
    // Store current state.
    cairo_save ( draw );
    // Define a clipmask so we won't draw outside out widget.
    cairo_rectangle ( draw, wid->x, wid->y, wid->w, wid->h );
    cairo_clip ( draw );
    // Set new x/y possition.
    cairo_translate ( draw, wid->x, wid->y );
    for ( GList *iter = g_list_first ( b->children ); iter != NULL; iter = g_list_next ( iter ) ) {
        widget * child = (widget *) iter->data;
        widget_draw ( child, draw );
    }
    cairo_restore ( draw );
}

static void box_free ( widget *wid )
{
    box *b = (box *) wid;

    for ( GList *iter = g_list_first ( b->children ); iter != NULL; iter = g_list_next ( iter ) ) {
        widget * child = (widget *) iter->data;
        widget_free ( child );
    }
    g_list_free ( b->children );
    g_free ( b );
}

void box_add ( box *box, widget *child, gboolean expand, gboolean end )
{
    if ( box == NULL ) {
        return;
    }
    child->expand = expand;
    child->end    = end;
    child->parent = WIDGET ( box );
    if ( end ) {
        box->children = g_list_prepend ( box->children, (void *) child );
    }
    else {
        box->children = g_list_append ( box->children, (void *) child );
    }
    widget_update ( WIDGET ( box ) );
}

static void box_resize ( widget *widget, short w, short h )
{
    box *b = (box *) widget;
    if ( b->widget.w != w || b->widget.h != h ) {
        b->widget.w = w;
        b->widget.h = h;
        widget_update ( widget );
    }
}

static gboolean box_clicked ( widget *wid, xcb_button_press_event_t *xbe, G_GNUC_UNUSED void *udata )
{
    box *b = (box *) wid;
    for ( GList *iter = g_list_first ( b->children ); iter != NULL; iter = g_list_next ( iter ) ) {
        widget * child = (widget *) iter->data;
        if ( !child->enabled ) {
            continue;
        }
        if ( widget_intersect ( child, xbe->event_x, xbe->event_y ) ) {
            xcb_button_press_event_t rel = *xbe;
            rel.event_x -= child->x;
            rel.event_y -= child->y;
            return widget_clicked ( child, &rel );
        }
    }
    return FALSE;
}
static gboolean box_motion_notify ( widget *wid, xcb_motion_notify_event_t *xme )
{
    box *b = (box *) wid;
    for ( GList *iter = g_list_first ( b->children ); iter != NULL; iter = g_list_next ( iter ) ) {
        widget * child = (widget *) iter->data;
        if ( !child->enabled ) {
            continue;
        }
        if ( widget_intersect ( child, xme->event_x, xme->event_y ) ) {
            xcb_motion_notify_event_t rel = *xme;
            rel.event_x -= child->x;
            rel.event_y -= child->y;
            return widget_motion_notify ( child, &rel );
        }
    }
    return FALSE;
}

box * box_create ( boxType type, short x, short y, short w, short h )
{
    box *b = g_malloc0 ( sizeof ( box ) );
    b->type                 = type;
    b->widget.x             = x;
    b->widget.y             = y;
    b->widget.w             = w;
    b->widget.h             = h;
    b->widget.draw          = box_draw;
    b->widget.free          = box_free;
    b->widget.resize        = box_resize;
    b->widget.update        = box_update;
    b->widget.clicked       = box_clicked;
    b->widget.motion_notify = box_motion_notify;
    b->widget.enabled       = TRUE;

    return b;
}

static void box_update ( widget *wid  )
{
    box *b = (box *) wid;
    switch ( b->type )
    {
    case BOX_VERTICAL:
        vert_calculate_size ( b );
        break;
    case BOX_HORIZONTAL:
    default:
        hori_calculate_size ( b );
    }
}
int box_get_fixed_pixels ( box *box )
{
    if ( box != NULL ) {
        return box->max_size;
    }
    return 0;
}

void box_set_padding ( box * box, unsigned int padding )
{
    if ( box != NULL ) {
        box->padding = padding;
        widget_queue_redraw ( WIDGET ( box ) );
    }
}
