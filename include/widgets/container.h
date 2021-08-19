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

#ifndef ROFI_CONTAINER_H
#define ROFI_CONTAINER_H

#include "widget.h"

/**
 * @defgroup container container
 * @ingroup widget
 *
 *
 * @{
 */

/**
 * Abstract handle to the container widget internal state.
 */
typedef struct _container container;

/**
 * @param parent The widget's parent
 * @param name The name of the widget.
 *
 * @returns a newly created container, free with #widget_free
 */
container *container_create(widget *parent, const char *name);

/**
 * @param container   Handle to the container widget.
 * @param child Handle to the child widget to pack.
 *
 * Add a widget to the container.
 */
void container_add(container *container, widget *child);
/**@}*/
#endif // ROFI_CONTAINER_H
