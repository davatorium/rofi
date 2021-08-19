/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2018 Qball Cow <qball@gmpclient.org>
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

#ifndef ROFI_ICON_H
#define ROFI_ICON_H

#include "widget.h"

/**
 * @defgroup icon icon
 * @ingroup widget
 *
 *
 * @{
 */

/**
 * Abstract handle to the icon widget internal state.
 */
typedef struct _icon icon;

/**
 * @param parent The widget's parent
 * @param name The name of the widget.
 *
 * @returns a newly created icon, free with #widget_free
 */
icon *icon_create(widget *parent, const char *name);

/**
 * @param icon The icon widget handle.
 * @param size  The size of the icon.
 *
 */
void icon_set_size(widget *icon, const int size);

/**
 * @param icon The icon widget handle.
 * @param surf The surface to display.
 */
void icon_set_surface(icon *icon, cairo_surface_t *surf);
/**@}*/
#endif // ROFI_ICON_H
