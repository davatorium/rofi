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
#include "widgets/widget.h"
#include "widgets/widget-internal.h"
#include "widgets/separator.h"
#include "x11-helper.h"
#include "settings.h"
#include "theme.h"

const char *SEPARATOR_CLASS_NAME = "@separator";
/**
 * @param sp The separator widget handle.
 * @param style_str String representation of the style.
 *
 * Sets the line style based on the string style_str
 */
void separator_set_line_style_from_string ( separator *sp, const char *style_str );

/**
 * @param sp The separator widget handle.
 * @param style The new style.
 *
 * Sets the line style.
 */
void separator_set_line_style ( separator *sp, separator_line_style style );

/**
 * Internal structure for the separator.
 */
struct _separator
{
    widget               widget;
    separator_type       type;
    separator_line_style line_style;
};

/** Configuration value for separator style indicating no line */
const char *const _separator_style_none = "none";
/** Configuration value for separator style indicating dashed line. */
const char *const _separator_style_dash = "dash";
static void separator_draw ( widget *, cairo_t * );
static void separator_free ( widget * );

separator *separator_create ( const char *name, separator_type type, short sw )
{
    separator *sb = g_malloc0 ( sizeof ( separator ) );
    widget_init ( WIDGET (sb), name, SEPARATOR_CLASS_NAME );
    sb->type = type;
    sb->widget.x = 0;
    sb->widget.y = 0;
    if ( sb->type == S_HORIZONTAL ) {
        sb->widget.w = 1;
        sb->widget.h = MAX ( 1, sw )+sb->widget.pad.top+sb->widget.pad.bottom;
    }
    else {
        sb->widget.h = 1;
        sb->widget.w = MAX ( 1, sw )+sb->widget.pad.left+sb->widget.pad.right;
    }

    sb->widget.draw = separator_draw;
    sb->widget.free = separator_free;

    // Enabled by default
    sb->widget.enabled = TRUE;

    const char *line_style = rofi_theme_get_string ( sb->widget.class_name, sb->widget.name, NULL, "line-style", "solid");
    separator_set_line_style_from_string ( sb, line_style );
    return sb;
}

static void separator_free ( widget *wid )
{
    separator *sb = (separator *) wid;
    g_free ( sb );
}

void separator_set_line_style ( separator *sp, separator_line_style style )
{
    if ( sp ) {
        sp->line_style = style;
        widget_need_redraw ( WIDGET ( sp ) );
    }
}
void separator_set_line_style_from_string ( separator *sp, const char *style_str )
{
    if ( !sp  ) {
        return;
    }
    separator_line_style style = S_LINE_SOLID;
    if ( g_strcmp0 ( style_str, _separator_style_none ) == 0 ) {
        style = S_LINE_NONE;
    }
    else if ( g_strcmp0 ( style_str, _separator_style_dash ) == 0 ) {
        style = S_LINE_DASH;
    }
    separator_set_line_style ( sp, style );
}

static void separator_draw ( widget *wid, cairo_t *draw )
{
    separator *sep = (separator *) wid;
    if ( sep->line_style == S_LINE_NONE ) {
        // Nothing to draw.
        return;
    }
    color_separator ( draw );
    rofi_theme_get_color ( wid->class_name, wid->name, NULL, "foreground", draw );
    if ( sep->line_style == S_LINE_DASH ) {
        const double dashes[1] = { 4 };
        cairo_set_dash ( draw, dashes, 1, 0.0 );
    }
    if ( sep->type == S_HORIZONTAL ) {
        int height= wid->h-wid->pad.top-wid->pad.bottom;
        cairo_set_line_width ( draw, height );
        double half = height / 2.0;
        cairo_move_to ( draw, wid->pad.left,         wid->pad.top + half );
        cairo_line_to ( draw, wid->w-wid->pad.right, wid->pad.top + half );
    }
    else {
        int width = wid->w-wid->pad.left-wid->pad.right;
        cairo_set_line_width ( draw, width);
        double half = width / 2.0;
        cairo_move_to ( draw, wid->pad.left + half, wid->pad.top);
        cairo_line_to ( draw, wid->pad.left + half, wid->h-wid->pad.bottom );
    }
    cairo_stroke ( draw );
}
