/**
 *   MIT/X11 License
 *   Modified  (c) 2016 Qball Cow <qball@gmpclient.org>
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
#include <xkbcommon/xkbcommon.h>
#include <glib.h>
#include <string.h>
#include "separator.h"
#include "x11-helper.h"
#include "settings.h"

const char *const _separator_style_none = "none";
const char *const _separator_style_dash = "dash";
static void separator_draw ( Widget *widget, cairo_t *draw );
static void separator_free ( Widget * );

separator *separator_create ( short h )
{
    separator *sb = g_malloc0 ( sizeof ( separator ) );

    sb->widget.x = 0;
    sb->widget.y = 0;
    sb->widget.w = 1; 
    sb->widget.h = MAX ( 1, h );

    sb->widget.draw = separator_draw;
    sb->widget.free = separator_free;

    // Enabled by default
    sb->widget.enabled = TRUE;
    return sb;
}

static void separator_free ( Widget *widget )
{
    separator *sb = (separator *) widget;
    g_free ( sb );
}

static void separator_draw ( Widget *widget, cairo_t *draw )
{
    if ( strcmp ( config.separator_style, _separator_style_none ) ) {
        color_separator ( draw );
        if ( strcmp ( config.separator_style, _separator_style_dash ) == 0 ) {
            const double dashes[1] = { 4 };
            cairo_set_dash ( draw, dashes, 1, 0.0 );
        }
        double half = widget->h/2.0;
        cairo_move_to ( draw, widget->x, widget->y + half );
        cairo_line_to ( draw, widget->x+ widget->w, widget->y + half ); 
        cairo_stroke ( draw );
    }
}

