/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2017 Qball Cow <qball@gmpclient.org>
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

#define G_LOG_DOMAIN    "Widgets.Box"

#include <config.h>
#include <stdio.h>
#include "widgets/widget.h"
#include "widgets/widget-internal.h"
#include "widgets/box.h"
#include "theme.h"

/** Default spacing used in the box*/
#define DEFAULT_SPACING    2

struct _box
{
    widget   widget;
    boxType  type;
    int      max_size;
    // Padding between elements
    Distance spacing;

    GList    *children;
};

static void box_update ( widget *wid  );

static int box_get_desired_height ( widget *wid )
{
    box *b      = (box *) wid;
    int spacing = distance_get_pixel ( b->spacing, b->type == BOX_VERTICAL ? ORIENTATION_VERTICAL : ORIENTATION_HORIZONTAL );
    int height  = 0;
    if ( b->type == BOX_VERTICAL ) {
        int active_widgets = 0;
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
        if ( active_widgets > 0 ) {
            height += ( active_widgets - 1 ) * spacing;
        }
    }
    else {
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
    int spacing           = distance_get_pixel ( b->spacing, ORIENTATION_VERTICAL );
    int expanding_widgets = 0;
    int active_widgets    = 0;
    int rem_width         = widget_padding_get_remaining_width ( WIDGET ( b ) );
    int rem_height        = widget_padding_get_remaining_height ( WIDGET ( b ) );
    for ( GList *iter = g_list_first ( b->children ); iter != NULL; iter = g_list_next ( iter ) ) {
        widget * child = (widget *) iter->data;
        if ( child->enabled && child->expand == FALSE ) {
            widget_resize ( child, rem_width, widget_get_desired_height ( child ) );
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
        if ( child->h > 0 ) {
            b->max_size += child->h;
        }
    }
    if ( active_widgets > 0 ) {
        b->max_size += ( active_widgets - 1 ) * spacing;
    }
    if ( b->max_size > rem_height ) {
        b->max_size = rem_height;
        g_debug ( "Widgets to large (height) for box: %d %d", b->max_size, b->widget.h );
        return;
    }
    if ( active_widgets > 0 ) {
        int    top   = widget_padding_get_top ( WIDGET ( b ) );
        double rem   = rem_height - b->max_size;
        int    index = 0;
        for ( GList *iter = g_list_first ( b->children ); iter != NULL; iter = g_list_next ( iter ) ) {
            widget * child = (widget *) iter->data;
            if ( child->enabled == FALSE ) {
                continue;
            }
            if ( child->expand == TRUE ) {
                // Re-calculate to avoid round issues leaving one pixel left.
                int expanding_widgets_size = ( rem ) / ( expanding_widgets - index );
                widget_move ( child, widget_padding_get_left ( WIDGET ( b ) ), top );
                top += expanding_widgets_size;
                widget_resize ( child, rem_width, expanding_widgets_size );
                top += spacing;
                rem -= expanding_widgets_size;
                index++;
            }
            else {
                widget_move ( child, widget_padding_get_left ( WIDGET ( b ) ), top );
                top += widget_get_height (  child );
                top += spacing;
            }
        }
    }
    b->max_size += widget_padding_get_padding_height ( WIDGET ( b ) );
}
static void hori_calculate_size ( box *b )
{
    int spacing           = distance_get_pixel ( b->spacing, ORIENTATION_HORIZONTAL );
    int expanding_widgets = 0;
    int active_widgets    = 0;
    int rem_width         = widget_padding_get_remaining_width ( WIDGET ( b ) );
    int rem_height        = widget_padding_get_remaining_height ( WIDGET ( b ) );
    for ( GList *iter = g_list_first ( b->children ); iter != NULL; iter = g_list_next ( iter ) ) {
        widget * child = (widget *) iter->data;
        if ( child->enabled && child->expand == FALSE ) {
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
        if ( child->h > 0 ) {
            b->max_size += child->w;
        }
    }
    b->max_size += MAX ( 0, ( ( active_widgets - 1 ) * spacing ) );
    if ( b->max_size > ( rem_width ) ) {
        b->max_size = rem_width;
        g_debug ( "Widgets to large (width) for box: %d %d", b->max_size, b->widget.w );
        return;
    }
    if ( active_widgets > 0 ) {
        int    left  = widget_padding_get_left ( WIDGET ( b ) );
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
                widget_move ( child, left, widget_padding_get_top ( WIDGET ( b ) ) );
                left += expanding_widgets_size;
                widget_resize ( child, expanding_widgets_size, rem_height );
                left += spacing;
                rem  -= expanding_widgets_size;
                index++;
            }
            else {
                widget_move ( child, left, widget_padding_get_top ( WIDGET ( b ) ) );
                left += widget_get_width (  child );
                left += spacing;
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

static int box_sort_children ( gconstpointer a, gconstpointer b )
{
    widget *child_a = (widget *) a;
    widget *child_b = (widget *) b;

    return child_a->index - child_b->index;
}

void box_add ( box *box, widget *child, gboolean expand, int index )
{
    if ( box == NULL ) {
        return;
    }
    // Make sure box is width/heigh enough.
    if ( box->type == BOX_VERTICAL ) {
        int width = box->widget.w;
        width         = MAX ( width, child->w + widget_padding_get_padding_width ( WIDGET ( box ) ) );
        box->widget.w = width;
    }
    else {
        int height = box->widget.h;
        height        = MAX ( height, child->h + widget_padding_get_padding_height ( WIDGET ( box ) ) );
        box->widget.h = height;
    }
    child->expand = rofi_theme_get_boolean ( child, "expand", expand );
    child->index  = rofi_theme_get_integer_exact ( child, "index", index );
    child->parent = WIDGET ( box );
    box->children = g_list_append ( box->children, (void *) child );
    box->children = g_list_sort   ( box->children, box_sort_children );
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

static widget *box_find_mouse_target ( widget *wid, WidgetType type, gint x, gint y )
{
    box *b = (box *) wid;
    for ( GList *iter = g_list_first ( b->children ); iter != NULL; iter = g_list_next ( iter ) ) {
        widget * child = (widget *) iter->data;
        if ( !child->enabled ) {
            continue;
        }
        if ( widget_intersect ( child, x, y ) ) {
            gint   rx      = x - child->x;
            gint   ry      = y - child->y;
            widget *target = widget_find_mouse_target ( child, type, rx, ry );
            if ( target != NULL ) {
                return target;
            }
        }
    }
    return NULL;
}

box * box_create ( const char *name, boxType type )
{
    box *b = g_malloc0 ( sizeof ( box ) );
    // Initialize widget.
    widget_init ( WIDGET ( b ), WIDGET_TYPE_UNKNOWN, name );
    b->type                      = type;
    b->widget.draw               = box_draw;
    b->widget.free               = box_free;
    b->widget.resize             = box_resize;
    b->widget.update             = box_update;
    b->widget.find_mouse_target  = box_find_mouse_target;
    b->widget.get_desired_height = box_get_desired_height;
    b->widget.enabled            = rofi_theme_get_boolean ( WIDGET ( b ), "enabled", TRUE );

    b->spacing = rofi_theme_get_distance ( WIDGET ( b ), "spacing", DEFAULT_SPACING );
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
