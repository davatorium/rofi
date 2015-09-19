/**
 *   MIT/X11 License
 *   Modified  (c) 2015 Qball Cow <qball@gmpclient.org>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining
 *   a copy of this software and associated documentation files (the
 *   "Software"), to deal in the Software without restriction, including
 *   without limitation the rights to use, copy, modify, merge, publish,
 *   distribute, sublicense, and/or sell copies of the Software, and to
 *   permit persons to whom the Software is furnished to do so, subject to
 *   the following conditions:
 *
 *   The above copyright notice and this permission notice shall be
 *   included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <config.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xft/Xft.h>
#include <glib.h>
#include "scrollbar.h"
#include "rofi.h"
#include "x11-helper.h"

extern Display *display;

scrollbar *scrollbar_create ( Window parent, XVisualInfo *vinfo, Colormap map,
                              short x, short y, short w, short h )
{
    scrollbar *sb = g_malloc0 ( sizeof ( scrollbar ) );

    sb->parent = parent;
    sb->x      = x;
    sb->y      = y;
    sb->w      = MAX ( 1, w );
    sb->h      = MAX ( 1, h );

    sb->length     = 10;
    sb->pos        = 0;
    sb->pos_length = 4;

    XSetWindowAttributes attr;
    attr.colormap         = map;
    attr.border_pixel     = color_border ( display );
    attr.background_pixel = color_background ( display );
    sb->window            =
        XCreateWindow ( display, sb->parent, sb->x, sb->y, sb->w, sb->h, 0, vinfo->depth,
                        InputOutput, vinfo->visual, CWColormap |
                        CWBorderPixel | CWBackPixel,
                        &attr );

    XSelectInput ( display, sb->window, ExposureMask | ButtonPressMask | Button1MotionMask  );
    sb->gc = XCreateGC ( display, sb->window, 0, 0 );
    XSetForeground ( display, sb->gc, color_separator ( display ) );
    //XSetFillStyle ( display, sb->gc, FillSolid);

    // Create GC.
    return sb;
}

void scrollbar_show ( scrollbar *sb )
{
    if ( sb != NULL ) {
        XMapWindow ( display, sb->window );
    }
}
void scrollbar_hide ( scrollbar *sb )
{
    if ( sb != NULL ) {
        XUnmapWindow ( display, sb->window );
    }
}

void scrollbar_free ( scrollbar *sb )
{
    if ( sb != NULL ) {
        XFreeGC ( display, sb->gc );
        XDestroyWindow ( display, sb->window );
        g_free ( sb );
    }
}

void scrollbar_set_max_value ( scrollbar *sb, unsigned int max )
{
    if ( sb != NULL ) {
        sb->length = MAX ( 1, max );
    }
}

void scrollbar_set_handle ( scrollbar *sb, unsigned int pos )
{
    if ( sb != NULL ) {
        sb->pos = MIN ( sb->length, MAX ( 0, pos ) );
    }
}

void scrollbar_set_handle_length ( scrollbar *sb, unsigned int pos_length )
{
    if ( sb != NULL ) {
        sb->pos_length = MIN ( sb->length, MAX ( 1, pos_length ) );
    }
}

void scrollbar_draw ( scrollbar *sb )
{
    if ( sb != NULL ) {
        // Calculate position and size.
        const short bh     = sb->h - 0;
        float       sec    = ( ( bh ) / (float) sb->length );
        short       height = sb->pos_length * sec;
        short       y      = sb->pos * sec;
        // Set max pos.
        y = MIN ( y, bh - 2 );
        // Never go out of bar.
        height = MAX ( 2, height );
        // Cap length;
        height = MIN ( bh - y + 1, ( height ) );
        // Redraw base window
        XClearWindow ( display, sb->window );
        // Paint the handle.
        XFillRectangle ( display, sb->window, sb->gc, config.line_margin, y, sb->w -
                         config.line_margin,
                         height );
    }
}
void scrollbar_resize ( scrollbar *sb, int w, int h )
{
    if ( sb != NULL ) {
        if ( h > 0 ) {
            sb->h = h;
        }
        if ( w > 0 ) {
            sb->w = w;
        }
        XResizeWindow ( display, sb->window, sb->w, sb->h );
    }
}
void scrollbar_move ( scrollbar *sb, int x, int y )
{
    if ( sb != NULL ) {
        sb->x = x;
        sb->y = y;
        XMoveWindow ( display, sb->window, x, y );
    }
}

unsigned int scrollbar_clicked ( scrollbar *sb, int y )
{
    if ( sb != NULL ) {
        y = MIN ( MAX ( 1, y ), sb->h - 1 ) - 1;
        const short  bh  = sb->h - 2;
        float        sec = ( ( bh ) / (float) sb->length );
        unsigned int sel = y / sec;
        return MIN ( sel, sb->length - 1 );
    }
    return 0;
}
