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

#include <config.h>
#include <xkbcommon/xkbcommon.h>
#include <glib.h>
#include "widgets/textbox.h"
#include "widgets/icon.h"
#include "widgets/listview.h"
#include "widgets/scrollbar.h"

#include "theme.h"

/** The default width of the scrollbar */
#define DEFAULT_SCROLLBAR_WIDTH    8

static void scrollbar_draw ( widget *, cairo_t * );
static void scrollbar_free ( widget * );

static int scrollbar_get_desired_height ( widget *wid )
{
    // Want height we are.
    return wid->h;
}

// TODO
// This should behave more like a real scrollbar.
guint scrollbar_scroll_get_line ( const scrollbar *sb, int y )
{
    y -= sb->widget.border.top.distance;
    if ( y < 0 ) {
        return 0;
    }

    if ( y > sb->widget.h ) {
        return sb->length - 1;
    }

    short  r           = ( sb->length * sb->widget.h ) / ( (double) ( sb->length + sb->pos_length ) );
    short  handle      = sb->widget.h - r;
    double sec         = ( ( r ) / (double) ( sb->length - 1 ) );
    short  half_handle = handle / 2;
    y -= half_handle;
    y  = MIN ( MAX ( 0, y ), sb->widget.h - 2 * half_handle );

    unsigned int sel = ( ( y ) / sec );
    return MIN ( sel, sb->length - 1 );
}

static void scrollbar_scroll ( scrollbar *sb, int y )
{
    listview_set_selected ( (listview *) sb->widget.parent, scrollbar_scroll_get_line ( sb, y ) );
}

static WidgetTriggerActionResult scrollbar_trigger_action ( widget *wid, MouseBindingMouseDefaultAction action, G_GNUC_UNUSED gint x, gint y, G_GNUC_UNUSED void *user_data )
{
    scrollbar *sb = (scrollbar *) wid;
    switch ( action )
    {
    case MOUSE_CLICK_DOWN:
        return WIDGET_TRIGGER_ACTION_RESULT_GRAB_MOTION_BEGIN;
    case MOUSE_CLICK_UP:
        scrollbar_scroll ( sb, y );
        return WIDGET_TRIGGER_ACTION_RESULT_GRAB_MOTION_END;
    case MOUSE_DCLICK_DOWN:
    case MOUSE_DCLICK_UP:
        break;
    }
    return FALSE;
}

static gboolean scrollbar_motion_notify ( widget *wid, G_GNUC_UNUSED gint x, gint y )
{
    scrollbar *sb = (scrollbar *) wid;
    scrollbar_scroll ( sb, y );
    return TRUE;
}

scrollbar *scrollbar_create ( widget *parent, const char *name )
{
    scrollbar *sb = g_malloc0 ( sizeof ( scrollbar ) );
    widget_init ( WIDGET ( sb ), parent, WIDGET_TYPE_SCROLLBAR, name );
    sb->widget.x = 0;
    sb->widget.y = 0;
    sb->width    = rofi_theme_get_distance ( WIDGET ( sb ), "handle-width", DEFAULT_SCROLLBAR_WIDTH );
    int width = distance_get_pixel ( sb->width, ROFI_ORIENTATION_HORIZONTAL );
    sb->widget.w = widget_padding_get_padding_width ( WIDGET ( sb ) ) + width;
    sb->widget.h = widget_padding_get_padding_height ( WIDGET ( sb ) );

    sb->widget.draw               = scrollbar_draw;
    sb->widget.free               = scrollbar_free;
    sb->widget.trigger_action     = scrollbar_trigger_action;
    sb->widget.motion_notify      = scrollbar_motion_notify;
    sb->widget.get_desired_height = scrollbar_get_desired_height;

    sb->length     = 10;
    sb->pos        = 0;
    sb->pos_length = 4;

    return sb;
}

static void scrollbar_free ( widget *wid )
{
    scrollbar *sb = (scrollbar *) wid;
    g_free ( sb );
}

void scrollbar_set_max_value ( scrollbar *sb, unsigned int max )
{
    if ( sb != NULL ) {
        sb->length = MAX ( 1u, max );
    }
}

void scrollbar_set_handle ( scrollbar *sb, unsigned int pos )
{
    if ( sb != NULL ) {
        sb->pos = MIN ( sb->length, pos );
    }
}

void scrollbar_set_handle_length ( scrollbar *sb, unsigned int pos_length )
{
    if ( sb != NULL ) {
        sb->pos_length = MIN ( sb->length, MAX ( 1u, pos_length ) );
    }
}

/**
 * The range is the height - handle length.
 * r = h - handle;
 * handle is the element length of the handle* length of one element.
 * handle =  r / ( num ) * hl
 *
 * r = h - r / ( num) *hl
 * r*num = num*h - r*hl
 * r*num+r*hl = num*h;
 * r ( num+hl ) = num*h
 * r = (num*h)/(num+hl)
 */
static void scrollbar_draw ( widget *wid, cairo_t *draw )
{
    scrollbar    *sb = (scrollbar *) wid;
    unsigned int wh  = widget_padding_get_remaining_height ( wid );
    // Calculate position and size.
    unsigned int r      = ( sb->length * wh ) / ( (double) ( sb->length + sb->pos_length ) );
    unsigned int handle = wid->h - r;
    double       sec    = ( ( r ) / (double) ( sb->length - 1 ) );
    unsigned int height = handle;
    unsigned int y      = sb->pos * sec;
    // Set max pos.
    y = MIN ( y, wh - handle );
    // Never go out of bar.
    height = MAX ( 2, height );
    // Cap length;
    rofi_theme_get_color ( WIDGET ( sb ), "handle-color", draw );

    cairo_rectangle ( draw,
                      widget_padding_get_left ( wid ),
                      widget_padding_get_top ( wid ) + y,
                      widget_padding_get_remaining_width ( wid ),
                      height );
    cairo_fill ( draw );
}
