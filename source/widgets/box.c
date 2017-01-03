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
#include "theme.h"

#define LOG_DOMAIN    "Widgets.Box"

/** Class name of box widget */
const char *BOX_CLASS_NAME = "@box";

/** Default spacing used in the box*/
#define DEFAULT_SPACING  2

/**
 * @param box Handle to the box widget.
 * @param spacing The spacing to apply.
 *
 * Set the spacing to apply between the children in pixels.
 */
void box_set_spacing ( box * box, unsigned int spacing );

struct _box
{
    widget  widget;
    boxType type;
    int     max_size;
    // Padding between elements
    int     spacing;

    GList   *children;
};

static void box_update ( widget *wid  );

static int box_get_desired_height ( widget *wid )
{
    box *b = (box *)wid;
    int active_widgets = 0;
    int height = 0;
    if ( b->type == BOX_VERTICAL ){
        for ( GList *iter = g_list_first ( b->children ); iter != NULL; iter = g_list_next ( iter ) ) {
            widget * child = (widget *) iter->data;
            if ( !child->enabled ) {
                continue;
            }
            active_widgets++;
            if ( child->expand == TRUE ) {
                height += widget_get_desired_height ( child );
                continue;
            }
            height += widget_get_desired_height ( child );
        }
        if ( active_widgets > 0 ){
            height += (active_widgets - 1)*b->spacing;
        }
    } else {
        for ( GList *iter = g_list_first ( b->children ); iter != NULL; iter = g_list_next ( iter ) ) {
            widget * child = (widget *) iter->data;
            if ( !child->enabled ) {
                continue;
            }
            height = MAX ( widget_get_desired_height ( child ), height );
        }
    }
    height += widget_padding_get_padding_height ( wid );
    return height;
}


static void vert_calculate_size ( box *b )
{
    int expanding_widgets = 0;
    int active_widgets    = 0;
    int rem_width = widget_padding_get_remaining_width ( WIDGET (b) );
    int rem_height = widget_padding_get_remaining_height ( WIDGET (b) );
    for ( GList *iter = g_list_first ( b->children ); iter != NULL; iter = g_list_next ( iter ) ) {
        widget * child = (widget *) iter->data;
        if ( child->enabled && child->expand == FALSE ){
            widget_resize ( child, rem_width, widget_get_desired_height (child) );
        }
    }
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
        if ( child->h > 0 ){
            b->max_size += child->h;
        }
    }
    if ( active_widgets > 0 ){
        b->max_size += ( active_widgets - 1 ) * b->spacing;
    }
    if ( b->max_size > rem_height ) {
        b->max_size = rem_height;
        g_log ( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Widgets to large (height) for box: %d %d", b->max_size, b->widget.h );
        return;
    }
    if ( active_widgets > 0 ) {
        int    bottom = b->widget.h - widget_padding_get_bottom ( WIDGET( b ) );
        int    top    = widget_padding_get_top ( WIDGET ( b ) );
        double rem    = rem_height - b->max_size;
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
                    widget_move ( child, widget_padding_get_left ( WIDGET ( b ) ), bottom );
                    widget_resize ( child, rem_width, expanding_widgets_size );
                    bottom -= b->spacing;
                }
                else {
                    widget_move ( child, widget_padding_get_left ( WIDGET ( b ) ), top );
                    top += expanding_widgets_size;
                    widget_resize ( child, rem_width, expanding_widgets_size );
                    top += b->spacing;
                }
                rem -= expanding_widgets_size;
                index++;
        //        b->max_size += widget_padding_get_padding_height ( child);
            }
            else if ( child->end ) {
                bottom -= widget_get_height (  child );
                widget_move ( child, widget_padding_get_left ( WIDGET ( b ) ), bottom );
                bottom -= b->spacing;
            }
            else {
                widget_move ( child, widget_padding_get_left ( WIDGET ( b ) ), top );
                top += widget_get_height (  child );
                top += b->spacing;
            }
        }
    }
    b->max_size += widget_padding_get_padding_height ( WIDGET (b) );
}
static void hori_calculate_size ( box *b )
{
    int expanding_widgets = 0;
    int active_widgets    = 0;
    int rem_width = widget_padding_get_remaining_width ( WIDGET (b) );
    int rem_height = widget_padding_get_remaining_height ( WIDGET (b) );
    for ( GList *iter = g_list_first ( b->children ); iter != NULL; iter = g_list_next ( iter ) ) {
        widget * child = (widget *) iter->data;
        if ( child->enabled && child->expand == FALSE ){
            widget_resize ( child, child->w, rem_height );
        }
    }
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
        if ( child->h > 0 ){
        b->max_size += child->w;
        }
    }
    b->max_size += MAX ( 0, ( ( active_widgets - 1 ) * b->spacing ) );
    if ( b->max_size > (rem_width)) {
        b->max_size = rem_width;
        g_log ( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Widgets to large (width) for box: %d %d", b->max_size, b->widget.w );
        return;
    }
    if ( active_widgets > 0 ) {
        int    right = b->widget.w-widget_padding_get_right ( WIDGET (b) );
        int    left  = widget_padding_get_left ( WIDGET (b) );
        double rem   = rem_width - b->max_size;
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
                    widget_move ( child, right, widget_padding_get_top ( WIDGET ( b ) ));
                    widget_resize ( child, expanding_widgets_size, rem_height );
                    right -= b->spacing;
                }
                else {
                    widget_move ( child, left, widget_padding_get_top ( WIDGET ( b ) ) );
                    left += expanding_widgets_size;
                    widget_resize ( child, expanding_widgets_size, rem_height );
                    left += b->spacing;
                }
                rem -= expanding_widgets_size;
                index++;
        //        b->max_size += widget_padding_get_padding_width ( child);
            }
            else if ( child->end ) {
                right -= widget_get_width (  child );
                widget_move ( child, right, widget_padding_get_top ( WIDGET ( b ) ) );
                right -= b->spacing;
            }
            else {
                widget_move ( child, left, widget_padding_get_top ( WIDGET ( b ) ) );
                left += widget_get_width (  child );
                left += b->spacing;
            }
        }
    }
    b->max_size += widget_padding_get_padding_width ( WIDGET ( b ) );
}

static void box_draw ( widget *wid, cairo_t *draw )
{
    box *b = (box *) wid;
    for ( GList *iter = g_list_first ( b->children ); iter != NULL; iter = g_list_next ( iter ) ) {
        widget * child = (widget *) iter->data;
        widget_draw ( child, draw );
    }
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
    // Make sure box is width/heigh enough.
    if ( box->type == BOX_VERTICAL){
        int width=box->widget.w;
        width = MAX ( width, child->w+widget_padding_get_padding_width ( WIDGET ( box ) ));
        box->widget.w = width;
    } else {
        int height = box->widget.h;
        height = MAX (height, child->h+widget_padding_get_padding_height ( WIDGET ( box )));
        box->widget.h = height;
    }
    child->expand = expand;
    child->end    = end;
    child->parent = WIDGET ( box );
    box->children = g_list_append ( box->children, (void *) child );
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

box * box_create ( const char *name, boxType type )
{
    box *b = g_malloc0 ( sizeof ( box ) );
    // Initialize widget.
    widget_init ( WIDGET(b), name, BOX_CLASS_NAME);
    b->type                      = type;
    b->widget.draw               = box_draw;
    b->widget.free               = box_free;
    b->widget.resize             = box_resize;
    b->widget.update             = box_update;
    b->widget.clicked            = box_clicked;
    b->widget.motion_notify      = box_motion_notify;
    b->widget.get_desired_height = box_get_desired_height;
    b->widget.enabled             = TRUE;

    box_set_spacing ( b, distance_get_pixel (rofi_theme_get_distance ( b->widget.class_name, b->widget.name, NULL, "spacing",DEFAULT_SPACING )));
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
        printf("max size: %d\n", box->max_size);
        return box->max_size;
    }
    return 0;
}

void box_set_spacing ( box * box, unsigned int spacing )
{
    if ( box != NULL ) {
        box->spacing = spacing;
        widget_queue_redraw ( WIDGET ( box ) );
    }
}
