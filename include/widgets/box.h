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

#ifndef ROFI_HBOX_H
#define ROFI_HBOX_H

#include "rofi-types.h"
#include "widget.h"

/**
 * @defgroup box box
 * @ingroup widget
 *
 * Widget used to pack multiple widgets either horizontally or vertically.
 * It supports packing widgets horizontally or vertically. Child widgets are
 * always expanded to the maximum size in the opposite direction of the packing
 * direction. e.g. vertically packed widgets use the full box width.
 *
 * @{
 */

/**
 * Abstract handle to the box widget internal state.
 */
typedef struct _box box;

/**
 * @param parent The widgets parent.
 * @param name The name of the widget.
 * @param type The packing direction of the newly created box.
 *
 * @returns a newly created box, free with #widget_free
 */
box *box_create(widget *parent, const char *name, RofiOrientation type);

/**
 * @param box   Handle to the box widget.
 * @param child Handle to the child widget to pack.
 * @param expand If the child widget should expand and use all available space.
 *
 * Add a widget to the box.
 */
void box_add(box *box, widget *child, gboolean expand);
/**@}*/
#endif // ROFI_HBOX_H
