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

#define G_LOG_DOMAIN "Widgets.Icon"

/** The log domain of this widget. */
#include "cairo.h"
#include "config.h"
#include "rofi-types.h"

#include "theme.h"
#include "widgets/icon.h"
#include "widgets/widget-internal.h"
#include "widgets/widget.h"
#include <math.h>
#include <stdio.h>

#include "rofi-icon-fetcher.h"

struct _icon {
  widget widget;

  // Size of the icon.
  int width;
  int height;
  int old_height;
  int old_width;

  int size_set;

  int squared;

  int resolve_num;
  char **icon_names;

  uint32_t icon_fetch_id;

  double yalign, xalign;

  // Source surface.
  cairo_surface_t *icon;
};

void icon_set_icon_names(icon *wid, char const *const *icon_names) {
  if (icon_names == NULL && wid->icon_names == NULL) {
    if (wid->icon_fetch_id != 0) {
      rofi_icon_fetcher_remove_widget(wid->icon_fetch_id, WIDGET(wid));
    }
    wid->icon_fetch_id = 0;
    if (wid->icon) {
      cairo_surface_destroy(wid->icon);
      wid->icon = NULL;
      widget_queue_redraw(WIDGET(wid));
    }
    wid->resolve_num = -1;
    return;
  }
  if (icon_names && wid->icon_names &&
      g_strv_equal(icon_names, (char const *const *)wid->icon_names)) {
    return;
  }
  if (wid->icon_fetch_id != 0) {
    rofi_icon_fetcher_remove_widget(wid->icon_fetch_id, WIDGET(wid));
  }
  wid->icon_fetch_id = 0;
  if (wid->icon_names) {
    g_strfreev(wid->icon_names);
  }
  wid->icon_names = g_strdupv((char **)icon_names);
  wid->resolve_num = 0;

  if (wid->icon) {
    cairo_surface_destroy(wid->icon);
    wid->icon = NULL;
  }

  if (wid->icon_names && wid->icon_names[0]) {

    int w = wid->widget.w;
    int h = wid->widget.h;
    if ((wid->size_set & 1) == 1) {
      w = wid->width;
    }
    if ((wid->size_set & 2) == 2) {
      h = wid->height;
    }
    gboolean done = TRUE;
    do {
      wid->icon_fetch_id = rofi_icon_fetcher_query_widget(
          wid->icon_names[wid->resolve_num], w, h, WIDGET(wid));
      done = rofi_icon_fetcher_get_ex(wid->icon_fetch_id, &(wid->icon));
      if (done) {
        rofi_icon_fetcher_remove_widget(wid->icon_fetch_id, WIDGET(wid));
        wid->icon_fetch_id = 0;
        if (wid->icon) {
          cairo_surface_reference(wid->icon);
          wid->resolve_num = -1;
          widget_queue_redraw(WIDGET(wid));
          return;
        }
        wid->resolve_num++;
      }
    } while (done && wid->icon_names[wid->resolve_num] != NULL);
    if (wid->icon_names[wid->resolve_num] == NULL) {
      wid->resolve_num = -1;
    }
    widget_queue_redraw(WIDGET(wid));
    return;
  }
  widget_queue_redraw(WIDGET(wid));
  wid->resolve_num = -1;
}

static int icon_get_desired_height(widget *wid, G_GNUC_UNUSED const int width) {
  icon *b = (icon *)wid;
  int height = b->height;
  if (b->squared == FALSE) {
    if (b->icon) {
      int iconh = cairo_image_surface_get_height(b->icon);
      int iconw = cairo_image_surface_get_width(b->icon);
      b->old_height = height = ceil(((double)width / iconw) * iconh);
      // printf("adjusted height: %d %d\n", iconw, iconh);
    } else {
      height = b->old_height;
      // printf("adjusted height: %d \n", height);
    }
  }
  height += widget_padding_get_padding_height(wid);
  // printf("desired height: %s %d for %d %p\n", wid->name, height, width,
  //        b->icon);
  return height;
}
static int icon_get_desired_width(widget *wid, G_GNUC_UNUSED const int height) {
  icon *b = (icon *)wid;
  int width = b->width;
  if (b->squared == FALSE) {
    if (b->icon) {
      int iconh = cairo_image_surface_get_height(b->icon);
      int iconw = cairo_image_surface_get_width(b->icon);
      b->old_width = width = ceil(iconw * ((double)height / iconh));
    } else {
      width = b->old_width;
    }
  }
  width += widget_padding_get_padding_width(wid);
  return width;
}

static void icon_draw(widget *wid, cairo_t *draw) {
  icon *b = (icon *)wid;
  // If no icon is loaded. quit.
  while (b->icon == NULL && b->icon_fetch_id > 0) {
    gboolean done = rofi_icon_fetcher_get_ex(b->icon_fetch_id, &(b->icon));
    if (done) {
      rofi_icon_fetcher_remove_widget(b->icon_fetch_id, wid);
      b->icon_fetch_id = 0;
      if (b->icon) {
        cairo_surface_reference(b->icon);
        widget_update(wid);
      } else {
        if (b->icon_names && b->resolve_num >= 0) {
          b->resolve_num++;

          if (b->icon_names[b->resolve_num]) {
            b->icon_fetch_id = rofi_icon_fetcher_query_widget(
                b->icon_names[b->resolve_num], b->widget.w, b->widget.h,
                WIDGET(wid));
          } else {
            b->resolve_num = -1;
          }
        }
      }
    } else {
      // query in flight.
      break;
    }
  }
  if (b->icon == NULL) {
    return;
  }
  int iconh = cairo_image_surface_get_height(b->icon);
  int iconw = cairo_image_surface_get_width(b->icon);
  int lpad = widget_padding_get_left(WIDGET(b));
  int rpad = widget_padding_get_right(WIDGET(b));
  int tpad = widget_padding_get_top(WIDGET(b));
  int bpad = widget_padding_get_bottom(WIDGET(b));
  double scalex = (double)(b->widget.w - lpad - rpad) / iconw;
  double scaley = (double)(b->widget.h - tpad - bpad) / iconh;
  double scale = MIN(scalex, scaley);
  //  scale = MIN(1.0, scale);

  cairo_save(draw);

  cairo_translate(
      draw, lpad + (b->widget.w - iconw * scale - lpad - rpad) * b->xalign,
      tpad + (b->widget.h - iconh * scale - tpad - bpad) * b->yalign);
  cairo_scale(draw, scale, scale);
  if (rofi_theme_has_property(WIDGET(wid), P_COLOR, "tint")) {
    cairo_pattern_t *pat = cairo_pattern_create_for_surface(b->icon);
    cairo_set_source_rgb(draw, 0, 0, 0);
    rofi_theme_get_color(WIDGET(wid), "tint", draw);
    cairo_mask(draw, pat);
    cairo_set_operator(draw, CAIRO_OPERATOR_HSL_LUMINOSITY);
    cairo_pattern_destroy(pat);
  }
  cairo_set_source_surface(draw, b->icon, 0, 0);
  cairo_paint(draw);
  cairo_restore(draw);
}

static void icon_free(widget *wid) {
  icon *b = (icon *)wid;

  g_strfreev(b->icon_names);
  if (b->icon) {
    cairo_surface_destroy(b->icon);
  }
  if (b->icon_fetch_id != 0) {
    rofi_icon_fetcher_remove_widget(b->icon_fetch_id, WIDGET(wid));
  }
}

static void icon_resize(widget *wid, short w, short h,
                        const unsigned int scale) {
  icon *b = (icon *)wid;
  if (b->widget.w != w || scale != b->widget.scale || b->widget.h != h) {
    b->widget.w = w;
    b->widget.h = h;
    b->widget.scale = scale;
    char **icon_names = b->icon_names;
    b->icon_names = NULL;
    icon_set_icon_names(b, (char const *const *)icon_names);
    g_strfreev(icon_names);
    widget_update(wid);
  }
}

void icon_set_surface(icon *icon_widget, cairo_surface_t *surf) {
  if (icon_widget->icon_fetch_id != 0) {
    rofi_icon_fetcher_remove_widget(icon_widget->icon_fetch_id,
                                    WIDGET(icon_widget));
  }
  icon_widget->icon_fetch_id = 0;
  if (surf == icon_widget->icon) {
    return;
  }
  if (icon_widget->icon) {
    cairo_surface_destroy(icon_widget->icon);
    icon_widget->icon = NULL;
  }
  if (surf) {
    cairo_surface_reference(surf);
    icon_widget->icon = surf;
  }
  g_strfreev(icon_widget->icon_names);
  icon_widget->icon_names = NULL;
  icon_widget->resolve_num = -1;
  widget_update(WIDGET(icon_widget));
}

icon *icon_create(widget *parent, const char *name) {
  icon *b = g_atomic_rc_box_new0(icon);

  // Initialize widget.
  widget_init(WIDGET(b), parent, WIDGET_TYPE_UNKNOWN, name);
  b->widget.draw = icon_draw;
  b->widget.free = icon_free;
  b->widget.resize = icon_resize;
  b->widget.get_desired_height = icon_get_desired_height;
  b->widget.get_desired_width = icon_get_desired_width;

  if (rofi_theme_has_property(WIDGET(b), P_PADDING, "size")) {
    RofiDistance d = rofi_theme_get_distance(WIDGET(b), "size", 16);
    int size = distance_get_pixel(d, ROFI_ORIENTATION_HORIZONTAL);
    b->size_set = 3;
    b->width = size;
    size = distance_get_pixel(d, ROFI_ORIENTATION_VERTICAL);
    b->height = size;
  }

  if (rofi_theme_has_property(WIDGET(b), P_PADDING, "width")) {
    RofiDistance d = rofi_theme_get_distance(WIDGET(b), "width", 16);
    b->width = distance_get_pixel(d, ROFI_ORIENTATION_HORIZONTAL);
    b->size_set |= 1;
  }
  if (rofi_theme_has_property(WIDGET(b), P_PADDING, "height")) {
    RofiDistance d = rofi_theme_get_distance(WIDGET(b), "height", 16);
    b->height = distance_get_pixel(d, ROFI_ORIENTATION_VERTICAL);
    b->size_set |= 2;
  }

  b->squared = rofi_theme_get_boolean(WIDGET(b), "squared", TRUE);

  const char *filename = rofi_theme_get_string(WIDGET(b), "filename", NULL);
  if (filename) {
    char **retv = g_malloc0(2 * sizeof(char *));
    retv[0] = g_strdup(filename);
    icon_set_icon_names(b, (char const *const *)retv);
    g_strfreev(retv);
  }
  b->yalign = rofi_theme_get_double(WIDGET(b), "vertical-align", 0.5);
  b->yalign = MAX(0, MIN(1.0, b->yalign));
  b->xalign = rofi_theme_get_double(WIDGET(b), "horizontal-align", 0.5);
  b->xalign = MAX(0, MIN(1.0, b->xalign));

  return b;
}
