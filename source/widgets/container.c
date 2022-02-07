/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2022 Qball Cow <qball@gmpclient.org>
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

/** The log domain of this widget. */
#define G_LOG_DOMAIN "Widgets.Container"

#include "widgets/container.h"
#include "theme.h"
#include "widgets/widget-internal.h"
#include "widgets/widget.h"
#include <stdio.h>

struct _container {
  widget widget;
  widget *child;
};

static void container_update(widget *wid);

static int container_get_desired_height(widget *widget, const int width) {
  container *b = (container *)widget;
  int height = 0;
  if (b->child) {
    height += widget_get_desired_height(b->child, width);
  }
  height += widget_padding_get_padding_height(widget);
  return height;
}

static void container_draw(widget *wid, cairo_t *draw) {
  container *b = (container *)wid;

  widget_draw(b->child, draw);
}

static void container_free(widget *wid) {
  container *b = (container *)wid;

  widget_free(b->child);
  g_free(b);
}

void container_add(container *container, widget *child) {
  if (container == NULL) {
    return;
  }
  container->child = child;
  g_assert(child->parent == WIDGET(container));
  widget_update(WIDGET(container));
}

static void container_resize(widget *widget, short w, short h) {
  container *b = (container *)widget;
  if (b->widget.w != w || b->widget.h != h) {
    b->widget.w = w;
    b->widget.h = h;
    widget_update(widget);
  }
}

static widget *container_find_mouse_target(widget *wid, WidgetType type, gint x,
                                           gint y) {
  container *b = (container *)wid;
  if (!widget_intersect(b->child, x, y)) {
    return NULL;
  }

  x -= b->child->x;
  y -= b->child->y;
  return widget_find_mouse_target(b->child, type, x, y);
}

static void container_set_state(widget *wid, const char *state) {
  container *b = (container *)wid;
  widget_set_state(b->child, state);
}

container *container_create(widget *parent, const char *name) {
  container *b = g_malloc0(sizeof(container));
  // Initialize widget.
  widget_init(WIDGET(b), parent, WIDGET_TYPE_UNKNOWN, name);
  b->widget.draw = container_draw;
  b->widget.free = container_free;
  b->widget.resize = container_resize;
  b->widget.update = container_update;
  b->widget.find_mouse_target = container_find_mouse_target;
  b->widget.get_desired_height = container_get_desired_height;
  b->widget.set_state = container_set_state;
  return b;
}

static void container_update(widget *wid) {
  container *b = (container *)wid;
  if (b->child && b->child->enabled) {
    widget_resize(WIDGET(b->child),
                  widget_padding_get_remaining_width(WIDGET(b)),
                  widget_padding_get_remaining_height(WIDGET(b)));
    widget_move(WIDGET(b->child), widget_padding_get_left(WIDGET(b)),
                widget_padding_get_top(WIDGET(b)));
  }
}
