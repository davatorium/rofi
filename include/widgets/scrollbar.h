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

#ifndef ROFI_SCROLLBAR_H
#define ROFI_SCROLLBAR_H
#include "widgets/widget-internal.h"
#include "widgets/widget.h"
#include <cairo.h>

/**
 * @defgroup Scrollbar Scrollbar
 * @ingroup widget
 *
 * @{
 */
/**
 * Internal structure for the scrollbar.
 */
typedef struct _scrollbar {
  widget widget;
  unsigned int length;
  unsigned int pos;
  unsigned int pos_length;
  RofiDistance width;
} scrollbar;

/**
 * @param parent The parent widget.
 * @param name  The name of the widget.
 *
 * Create a new scrollbar
 *
 * @returns the scrollbar object.
 */
scrollbar *scrollbar_create(widget *parent, const char *name);

/**
 * @param sb scrollbar object
 * @param pos_length new length
 *
 * set the length of the handle relative to the max value of bar.
 */
void scrollbar_set_handle_length(scrollbar *sb, unsigned int pos_length);

/**
 * @param sb scrollbar object
 * @param pos new position
 *
 * set the position of the handle relative to the set max value of bar.
 */
void scrollbar_set_handle(scrollbar *sb, unsigned int pos);

/**
 * @param sb scrollbar object
 * @param max the new max
 *
 * set the max value of the bar.
 */
void scrollbar_set_max_value(scrollbar *sb, unsigned int max);

/**
 * @param sb scrollbar object
 * @param y  clicked position
 *
 * Calculate the position of the click relative to the max value of bar
 */
guint scrollbar_scroll_get_line(const scrollbar *sb, int y);

/**@}*/
#endif // ROFI_SCROLLBAR_H
