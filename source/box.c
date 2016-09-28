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
#include "box.h"


struct _Box 
{
    Widget widget;
    BoxType type;
    int max_size;

    GList *children;
} ;


static void vert_calculate_size ( Box *b )
{
    int bottom = b->widget.h;
    int top = 0;
    int expanding_widgets = 0;
    b->max_size = 0;
    for ( GList *iter = g_list_first ( b->children ); iter != NULL; iter = g_list_next ( iter)){
        Widget * child = (Widget *)iter->data; 
        if ( ! child->enabled ) {
            continue;
        }
        if ( child->expand == TRUE ){
            expanding_widgets++;
            continue;
        }
        if ( child->end ){
            // @TODO hack. figure out better way.
            bottom -= widget_get_height (  child ); 
            widget_move ( child, child->x, bottom);
        } else {
            widget_move ( child, child->x, top );
            top += widget_get_height (  child ); 
        }
        widget_resize ( child, b->widget.w, child->h);
        b->max_size += child->h;
    }
    if ( expanding_widgets > 0 ){
        double rem = (bottom - top );
        int index = 0;
        for ( GList *iter = g_list_first ( b->children ); iter != NULL; iter = g_list_next ( iter)){
            Widget * child = (Widget *)iter->data; 
            if ( child->enabled && child->expand == TRUE ){
                // Re-calculate to avoid round issues leaving one pixel left.
                int expanding_widgets_size = (rem)/(expanding_widgets-index);
                if ( child->end ) {
                    bottom -= expanding_widgets_size;
                    widget_move ( child, child->x, bottom );
                    widget_resize ( child, b->widget.w, expanding_widgets_size );
                } else {
                    widget_move ( child, child->x, top );
                    top += expanding_widgets_size; 
                    widget_resize ( child, b->widget.w, expanding_widgets_size );
                }
                rem -= expanding_widgets_size;
                index++;
            }
        }
    }
}
static void hori_calculate_size ( Box *b )
{
    int right = b->widget.w;
    int left  = 0;
    int expanding_widgets = 0;
    b->max_size = 0;
    for ( GList *iter = g_list_first ( b->children ); iter != NULL; iter = g_list_next ( iter)){
        Widget * child = (Widget *)iter->data; 
        if ( ! child->enabled ) {
            continue;
        }
        if ( child->expand == TRUE ){
            expanding_widgets++;
            continue;
        }
        if ( child->end ){
            right -= widget_get_width (  child ); 
            widget_move ( child, right, child->y );
        } else {
            widget_move ( child, left, child->y );
            left += widget_get_width (  child ); 
        }
        widget_resize ( child, child->w, b->widget.h); 
        b->max_size += child->w;
    }
    if ( expanding_widgets > 0 ){
        double rem = (right-left);
        int index = 0;
        for ( GList *iter = g_list_first ( b->children ); iter != NULL; iter = g_list_next ( iter)){
            Widget * child = (Widget *)iter->data; 
            if ( child->enabled && child->expand == TRUE ){
                // Re-calculate to avoid round issues leaving one pixel left.
                int expanding_widgets_size = (rem)/(expanding_widgets-index);
                if ( child->end ) {
                    right -= expanding_widgets_size;
                    widget_move ( child, right, child->y );
                    widget_resize ( child, expanding_widgets_size, b->widget.h );
                } else {
                    widget_move ( child, left, child->y );
                    left += expanding_widgets_size; 
                    widget_resize ( child, expanding_widgets_size, b->widget.h );
                }
                rem -= expanding_widgets_size;
                index++;
            }
        }
    }
}

static void box_draw ( Widget *widget, cairo_t *draw )
{
    Box *b = (Box *)widget;
    // Store current state.
    cairo_save ( draw );
    // Define a clipmask so we won't draw outside out widget.
    cairo_rectangle ( draw, widget->x, widget->y, widget->w, widget->h );
    cairo_clip ( draw );
    // Set new x/y possition.
    cairo_translate ( draw, widget->x, widget->y );
    for ( GList *iter = g_list_first ( b->children ); iter != NULL; iter = g_list_next ( iter)){
        Widget * child = (Widget *)iter->data; 
        widget_draw ( child, draw );
    }
    cairo_restore ( draw );
}

static void box_free ( Widget *widget )
{
    Box *b = (Box *)widget;

    for ( GList *iter = g_list_first ( b->children ); iter != NULL; iter = g_list_next ( iter)){
        Widget * child = (Widget *)iter->data; 
        widget_free(child);
    }


    g_free(b);

}

void box_add ( Box *box, Widget *child, gboolean expand, gboolean end )
{
    child->expand = expand;
    child->end    = end;
    if ( end ) {
        box->children = g_list_prepend( box->children, (void *)child );
    }else {
        box->children = g_list_append ( box->children, (void *)child );
    }
    box_update ( box );
}

static void box_resize ( Widget *widget, short w, short h )
{
    Box *box = (Box *)widget;
    if ( box->widget.w != w || box->widget.h != h ){
        box->widget.w = w;
        box->widget.h = h;
        box_update ( box );
    }
}

Box * box_create ( BoxType type, short x, short y, short w, short h )
{
    Box *b = g_malloc0 ( sizeof ( Box ) );
    b->type = type;
    b->widget.x = x;
    b->widget.y = y;
    b->widget.w = w;
    b->widget.h = h;
    b->widget.draw = box_draw;
    b->widget.free = box_free;
    b->widget.resize = box_resize;
    b->widget.enabled = TRUE;

    return b;
}


void box_update ( Box *box )
{
    switch ( box->type ) {
        case BOX_VERTICAL:
           vert_calculate_size ( box );
           break;
        case BOX_HORIZONTAL:
        default: 
           hori_calculate_size ( box );
    }
}
int box_get_fixed_pixels ( Box *box )
{
    return box->max_size;
}
