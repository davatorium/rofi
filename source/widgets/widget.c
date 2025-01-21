/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2023 Qball Cow <qball@gmpclient.org>
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
#include "config.h"

#include "theme.h"
#include "widgets/widget-internal.h"
#include "widgets/widget.h"
#include <glib.h>
#include <math.h>

void widget_init(widget *wid, widget *parent, WidgetType type,
                 const char *name) {
  wid->type = type;
  wid->parent = parent;
  wid->name = g_strdup(name);
  wid->def_padding = WIDGET_PADDING_INIT;
  wid->def_border = WIDGET_PADDING_INIT;
  wid->def_border_radius = WIDGET_PADDING_INIT;
  wid->def_margin = WIDGET_PADDING_INIT;

  wid->padding = rofi_theme_get_padding(wid, "padding", wid->def_padding);
  wid->border = rofi_theme_get_padding(wid, "border", wid->def_border);
  wid->border_radius =
      rofi_theme_get_padding(wid, "border-radius", wid->def_border_radius);
  wid->margin = rofi_theme_get_padding(wid, "margin", wid->def_margin);

  wid->cursor_type =
      rofi_theme_get_cursor_type(wid, "cursor", ROFI_CURSOR_DEFAULT);

  // enabled by default
  wid->enabled = rofi_theme_get_boolean(wid, "enabled", TRUE);
}

void widget_set_state(widget *wid, const char *state) {
  if (wid == NULL) {
    return;
  }
  if (g_strcmp0(wid->state, state)) {
    wid->state = state;
    // Update border.
    wid->border = rofi_theme_get_padding(wid, "border", wid->def_border);
    wid->border_radius =
        rofi_theme_get_padding(wid, "border-radius", wid->def_border_radius);
    if (wid->set_state != NULL) {
      wid->set_state(wid, state);
    }
    widget_queue_redraw(wid);
  }
}

int widget_intersect(const widget *wid, int x, int y) {
  if (wid == NULL) {
    return FALSE;
  }

  if (x >= (wid->x) && x < (wid->x + wid->w) && y >= (wid->y) &&
      y < (wid->y + wid->h)) {
    return TRUE;
  }
  return FALSE;
}

void widget_resize(widget *wid, short w, short h) {
  if (wid == NULL) {
    return;
  }
  if (wid->resize != NULL) {
    if (wid->w != w || wid->h != h) {
      wid->resize(wid, w, h);
    }
  } else {
    wid->w = w;
    wid->h = h;
  }
  // On a resize we always want to update.
  widget_queue_redraw(wid);
}
void widget_move(widget *wid, short x, short y) {
  if (wid == NULL) {
    return;
  }
  wid->x = x;
  wid->y = y;
}
void widget_set_type(widget *wid, WidgetType type) {
  if (wid == NULL) {
    return;
  }
  wid->type = type;
}

gboolean widget_enabled(widget *wid) {
  if (wid == NULL) {
    return FALSE;
  }
  return wid->enabled;
}

void widget_set_enabled(widget *wid, gboolean enabled) {
  if (wid == NULL) {
    return;
  }
  if (wid->enabled != enabled) {
    wid->enabled = enabled;
    widget_update(wid);
    widget_update(wid->parent);
    widget_queue_redraw(wid);
  }
}

void widget_draw(widget *wid, cairo_t *d) {
  if (wid == NULL) {
    return;
  }
  // Check if enabled and if draw is implemented.
  if (wid->enabled && wid->draw) {
    // Don't draw if there is no space.
    if (wid->h < 1 || wid->w < 1) {
      wid->need_redraw = FALSE;
      return;
    }
    // Store current state.
    cairo_save(d);
    const int margin_left =
        distance_get_pixel(wid->margin.left, ROFI_ORIENTATION_HORIZONTAL);
    const int margin_top =
        distance_get_pixel(wid->margin.top, ROFI_ORIENTATION_VERTICAL);
    const int margin_right =
        distance_get_pixel(wid->margin.right, ROFI_ORIENTATION_HORIZONTAL);
    const int margin_bottom =
        distance_get_pixel(wid->margin.bottom, ROFI_ORIENTATION_VERTICAL);
    const int left =
        distance_get_pixel(wid->border.left, ROFI_ORIENTATION_HORIZONTAL);
    const int right =
        distance_get_pixel(wid->border.right, ROFI_ORIENTATION_HORIZONTAL);
    const int top =
        distance_get_pixel(wid->border.top, ROFI_ORIENTATION_VERTICAL);
    const int bottom =
        distance_get_pixel(wid->border.bottom, ROFI_ORIENTATION_VERTICAL);
    int radius_bl = distance_get_pixel(wid->border_radius.left,
                                       ROFI_ORIENTATION_HORIZONTAL);
    int radius_tr = distance_get_pixel(wid->border_radius.right,
                                       ROFI_ORIENTATION_HORIZONTAL);
    int radius_tl =
        distance_get_pixel(wid->border_radius.top, ROFI_ORIENTATION_VERTICAL);
    int radius_br = distance_get_pixel(wid->border_radius.bottom,
                                       ROFI_ORIENTATION_VERTICAL);

    double vspace =
        wid->h - margin_top - margin_bottom - top / 2.0 - bottom / 2.0;
    double hspace =
        wid->w - margin_left - margin_right - left / 2.0 - right / 2.0;
    if ((radius_bl + radius_tl) > (vspace)) {
      int j = ((vspace) / 2.0);
      radius_bl = MIN(radius_bl, j);
      radius_tl = MIN(radius_tl, j);
    }
    if ((radius_br + radius_tr) > (vspace)) {
      int j = ((vspace) / 2.0);
      radius_br = MIN(radius_br, j);
      radius_tr = MIN(radius_tr, j);
    }
    if ((radius_tl + radius_tr) > (hspace)) {
      int j = ((hspace) / 2.0);
      radius_tr = MIN(radius_tr, j);
      radius_tl = MIN(radius_tl, j);
    }
    if ((radius_bl + radius_br) > (hspace)) {
      int j = ((hspace) / 2.0);
      radius_br = MIN(radius_br, j);
      radius_bl = MIN(radius_bl, j);
    }

    // Background painting.
    // Set new x/y position.
    cairo_translate(d, wid->x, wid->y);
    cairo_set_line_width(d, 0);

    // Outer outline outlines
    double x1, y1, x2, y2;
    x1 = margin_left + left / 2.0, y1 = margin_top + top / 2.0,
    x2 = wid->w - margin_right - right / 2.0,
    y2 = wid->h - margin_bottom - bottom / 2.0;

    if (radius_tl > 0) {
      cairo_move_to(d, x1, y1 + radius_tl);
      cairo_arc(d, x1 + radius_tl, y1 + radius_tl, radius_tl, -1.0 * G_PI,
                -G_PI_2);
    } else {
      cairo_move_to(d, x1, y1);
    }
    if (radius_tr > 0) {
      cairo_line_to(d, x2 - radius_tr, y1);
      cairo_arc(d, x2 - radius_tr, y1 + radius_tr, radius_tr, -G_PI_2,
                0 * G_PI);
    } else {
      cairo_line_to(d, x2, y1);
    }
    if (radius_br > 0) {
      cairo_line_to(d, x2, y2 - radius_br);
      cairo_arc(d, x2 - radius_br, y2 - radius_br, radius_br, 0.0 * G_PI,
                G_PI_2);
    } else {
      cairo_line_to(d, x2, y2);
    }
    if (radius_bl > 0) {
      cairo_line_to(d, x1 + radius_bl, y2);
      cairo_arc(d, x1 + radius_bl, y2 - radius_bl, radius_bl, G_PI_2,
                1.0 * G_PI);
    } else {
      cairo_line_to(d, x1, y2);
    }
    cairo_close_path(d);

    cairo_set_source_rgba(d, 1.0, 1.0, 1.0, 1.0);
    rofi_theme_get_color(wid, "background-color", d);
    cairo_fill_preserve(d);
    if (rofi_theme_get_image(wid, "background-image", d)) {
      cairo_fill_preserve(d);
    }
    cairo_clip(d);

    wid->draw(wid, d);
    wid->need_redraw = FALSE;

    cairo_restore(d);

    if (left != 0 || top != 0 || right != 0 || bottom != 0) {
      cairo_save(d);
      //      cairo_set_operator(d, CAIRO_OPERATOR_ADD);
      cairo_translate(d, wid->x, wid->y);
      cairo_new_path(d);
      rofi_theme_get_color(wid, "border-color", d);

      // Calculate the different offsets for the corners.
      double minof_tr = MIN(right / 2.0, top / 2.0);
      double minof_tl = MIN(left / 2.0, top / 2.0);
      double minof_br = MIN(right / 2.0, bottom / 2.0);
      double minof_bl = MIN(left / 2.0, bottom / 2.0);
      // Inner radius
      double radius_inner_tl = radius_tl - minof_tl;
      double radius_inner_tr = radius_tr - minof_tr;
      double radius_inner_bl = radius_bl - minof_bl;
      double radius_inner_br = radius_br - minof_br;

      // Offsets of the different lines in each corner.
      //
      //      |             |
      //     ttl           ttr
      //      |             |
      // -ltl-###############-rtr-
      //      $             $
      //      $             $
      // -lbl-###############-rbr-
      //      |             |
      //     bbl           bbr
      //      |             |
      //
      // The left and right part ($) start at thinkness top bottom when no
      // radius
      double offset_ltl =
          (radius_inner_tl > 0) ? (left) + radius_inner_tl : left;
      double offset_rtr =
          (radius_inner_tr > 0) ? (right) + radius_inner_tr : right;
      double offset_lbl =
          (radius_inner_bl > 0) ? (left) + radius_inner_bl : left;
      double offset_rbr =
          (radius_inner_br > 0) ? (right) + radius_inner_br : right;
      // The top and bottom part (#) go into the corner when no radius
      double offset_ttl = (radius_inner_tl > 0) ? (top) + radius_inner_tl
                          : (radius_tl > 0)     ? top
                                                : 0;
      double offset_ttr = (radius_inner_tr > 0) ? (top) + radius_inner_tr
                          : (radius_tr > 0)     ? top
                                                : 0;
      double offset_bbl = (radius_inner_bl > 0) ? (bottom) + radius_inner_bl
                          : (radius_bl > 0)     ? bottom
                                                : 0;
      double offset_bbr = (radius_inner_br > 0) ? (bottom) + radius_inner_br
                          : (radius_br > 0)     ? bottom
                                                : 0;

      if (left > 0) {
        cairo_set_line_width(d, left);
        distance_get_linestyle(wid->border.left, d);
        cairo_move_to(d, x1, margin_top + offset_ttl);
        cairo_line_to(d, x1, wid->h - margin_bottom - offset_bbl);
        cairo_stroke(d);
      }
      if (right > 0) {
        cairo_set_line_width(d, right);
        distance_get_linestyle(wid->border.right, d);
        cairo_move_to(d, x2, margin_top + offset_ttr);
        cairo_line_to(d, x2, wid->h - margin_bottom - offset_bbr);
        cairo_stroke(d);
      }
      if (top > 0) {
        cairo_set_line_width(d, top);
        distance_get_linestyle(wid->border.top, d);
        cairo_move_to(d, margin_left + offset_ltl, y1);
        cairo_line_to(d, wid->w - margin_right - offset_rtr, y1);
        cairo_stroke(d);
      }
      if (bottom > 0) {
        cairo_set_line_width(d, bottom);
        distance_get_linestyle(wid->border.bottom, d);
        cairo_move_to(d, margin_left + offset_lbl, y2);
        cairo_line_to(d, wid->w - margin_right - offset_rbr, y2);
        cairo_stroke(d);
      }
      if (radius_tl > 0) {
        double radius_outer = radius_tl + minof_tl;
        cairo_arc(d, margin_left + radius_outer, margin_top + radius_outer,
                  radius_outer, -G_PI, -G_PI_2);
        cairo_line_to(d, margin_left + offset_ltl, margin_top);
        cairo_line_to(d, margin_left + offset_ltl, margin_top + top);
        if (radius_inner_tl > 0) {
          cairo_arc_negative(d, margin_left + left + radius_inner_tl,
                             margin_top + top + radius_inner_tl,
                             radius_inner_tl, -G_PI_2, G_PI);
          cairo_line_to(d, margin_left + left, margin_top + offset_ttl);
        }
        cairo_line_to(d, margin_left, margin_top + offset_ttl);
        cairo_close_path(d);
        cairo_fill(d);
      }
      if (radius_tr > 0) {
        double radius_outer = radius_tr + minof_tr;
        cairo_arc(d, wid->w - margin_right - radius_outer,
                  margin_top + radius_outer, radius_outer, -G_PI_2, 0);
        cairo_line_to(d, wid->w - margin_right, margin_top + offset_ttr);
        cairo_line_to(d, wid->w - margin_right - right,
                      margin_top + offset_ttr);
        if (radius_inner_tr > 0) {
          cairo_arc_negative(d, wid->w - margin_right - right - radius_inner_tr,
                             margin_top + top + radius_inner_tr,
                             radius_inner_tr, 0, -G_PI_2);
          cairo_line_to(d, wid->w - margin_right - offset_rtr,
                        margin_top + top);
        }
        cairo_line_to(d, wid->w - margin_right - offset_rtr, margin_top);
        cairo_close_path(d);
        cairo_fill(d);
      }
      if (radius_br > 0) {
        double radius_outer = radius_br + minof_br;
        cairo_arc(d, wid->w - margin_right - radius_outer,
                  wid->h - margin_bottom - radius_outer, radius_outer, 0.0,
                  G_PI_2);
        cairo_line_to(d, wid->w - margin_right - offset_rbr,
                      wid->h - margin_bottom);
        cairo_line_to(d, wid->w - margin_right - offset_rbr,
                      wid->h - margin_bottom - bottom);
        if (radius_inner_br > 0) {
          cairo_arc_negative(d, wid->w - margin_right - right - radius_inner_br,
                             wid->h - margin_bottom - bottom - radius_inner_br,
                             radius_inner_br, G_PI_2, 0.0);
          cairo_line_to(d, wid->w - margin_right - right,
                        wid->h - margin_bottom - offset_bbr);
        }
        cairo_line_to(d, wid->w - margin_right,
                      wid->h - margin_bottom - offset_bbr);
        cairo_close_path(d);
        cairo_fill(d);
      }
      if (radius_bl > 0) {
        double radius_outer = radius_bl + minof_bl;
        cairo_arc(d, margin_left + radius_outer,
                  wid->h - margin_bottom - radius_outer, radius_outer, G_PI_2,
                  G_PI);
        cairo_line_to(d, margin_left, wid->h - margin_bottom - offset_bbl);
        cairo_line_to(d, margin_left + left,
                      wid->h - margin_bottom - offset_bbl);
        if (radius_inner_bl > 0) {
          cairo_arc_negative(d, margin_left + left + radius_inner_bl,
                             wid->h - margin_bottom - bottom - radius_inner_bl,
                             radius_inner_bl, G_PI, G_PI_2);
          cairo_line_to(d, margin_left + offset_lbl,
                        wid->h - margin_bottom - bottom);
        }
        cairo_line_to(d, margin_left + offset_lbl, wid->h - margin_bottom);
        cairo_close_path(d);

        cairo_fill(d);
      }
      cairo_restore(d);
    }
  }
}

void widget_free(widget *wid) {
  if (wid == NULL) {
    return;
  }
  if (wid->name != NULL) {
    g_free(wid->name);
  }
  if (wid->free != NULL) {
    wid->free(wid);
  }
}

int widget_get_height(widget *wid) {
  if (wid == NULL) {
    return 0;
  }
  if (wid->get_height == NULL) {
    return wid->h;
  }
  return wid->get_height(wid);
}
int widget_get_width(widget *wid) {
  if (wid == NULL) {
    return 0;
  }
  if (wid->get_width == NULL) {
    return wid->w;
  }
  return wid->get_width(wid);
}
int widget_get_x_pos(widget *wid) {
  if (wid == NULL) {
    return 0;
  }
  return wid->x;
}
int widget_get_y_pos(widget *wid) {
  if (wid == NULL) {
    return 0;
  }
  return wid->y;
}

void widget_xy_to_relative(widget *wid, gint *x, gint *y) {
  *x -= wid->x;
  *y -= wid->y;
  if (wid->parent == NULL) {
    return;
  }
  widget_xy_to_relative(wid->parent, x, y);
}

void widget_update(widget *wid) {
  if (wid == NULL) {
    return;
  }
  // When (desired )size of wid changes.
  if (wid->update != NULL) {
    wid->update(wid);
  }
}

void widget_queue_redraw(widget *wid) {
  if (wid == NULL) {
    return;
  }
  widget *iter = wid;
  // Find toplevel widget.
  while (iter->parent != NULL) {
    iter->need_redraw = TRUE;
    iter = iter->parent;
  }
  iter->need_redraw = TRUE;
}

gboolean widget_need_redraw(widget *wid) {
  if (wid == NULL) {
    return FALSE;
  }
  if (!wid->enabled) {
    return FALSE;
  }
  return wid->need_redraw;
}

widget *widget_find_mouse_target(widget *wid, WidgetType type, gint x, gint y) {
  if (wid == NULL) {
    return NULL;
  }

  if (wid->find_mouse_target != NULL) {
    widget *target = wid->find_mouse_target(wid, type, x, y);
    if (target != NULL) {
      return target;
    }
  }

  if (wid->type == type || type == WIDGET_TYPE_UNKNOWN) {
    return wid;
  }

  return NULL;
}

WidgetTriggerActionResult widget_check_action(widget *wid,
                                              G_GNUC_UNUSED guint action,
                                              G_GNUC_UNUSED gint x,
                                              G_GNUC_UNUSED gint y) {
  if (wid == NULL) {
    return FALSE;
  }
  if (wid->trigger_action == NULL) {
    return FALSE;
  }
  /*
   * TODO: We should probably add a check_action callback to the widgets
   * to do extra checks
   */
  return WIDGET_TRIGGER_ACTION_RESULT_HANDLED;
}

WidgetTriggerActionResult widget_trigger_action(widget *wid, guint action,
                                                gint x, gint y) {
  if (wid == NULL) {
    return FALSE;
  }
  if (wid->trigger_action == NULL) {
    return FALSE;
  }
  return wid->trigger_action(wid, action, x, y, wid->trigger_action_cb_data);
}

void widget_set_trigger_action_handler(widget *wid, widget_trigger_action_cb cb,
                                       void *cb_data) {
  if (wid == NULL) {
    return;
  }
  wid->trigger_action = cb;
  wid->trigger_action_cb_data = cb_data;
}

gboolean widget_motion_notify(widget *wid, gint x, gint y) {
  if (wid == NULL) {
    return FALSE;
  }
  if (wid->motion_notify == NULL) {
    return FALSE;
  }
  return wid->motion_notify(wid, x, y);
}

int widget_padding_get_left(const widget *wid) {
  if (wid == NULL) {
    return 0;
  }
  int distance =
      distance_get_pixel(wid->padding.left, ROFI_ORIENTATION_HORIZONTAL);
  distance += distance_get_pixel(wid->border.left, ROFI_ORIENTATION_HORIZONTAL);
  distance += distance_get_pixel(wid->margin.left, ROFI_ORIENTATION_HORIZONTAL);
  return distance;
}
int widget_padding_get_right(const widget *wid) {
  if (wid == NULL) {
    return 0;
  }
  int distance =
      distance_get_pixel(wid->padding.right, ROFI_ORIENTATION_HORIZONTAL);
  distance +=
      distance_get_pixel(wid->border.right, ROFI_ORIENTATION_HORIZONTAL);
  distance +=
      distance_get_pixel(wid->margin.right, ROFI_ORIENTATION_HORIZONTAL);
  return distance;
}
int widget_padding_get_top(const widget *wid) {
  if (wid == NULL) {
    return 0;
  }
  int distance =
      distance_get_pixel(wid->padding.top, ROFI_ORIENTATION_VERTICAL);
  distance += distance_get_pixel(wid->border.top, ROFI_ORIENTATION_VERTICAL);
  distance += distance_get_pixel(wid->margin.top, ROFI_ORIENTATION_VERTICAL);
  return distance;
}
int widget_padding_get_bottom(const widget *wid) {
  if (wid == NULL) {
    return 0;
  }
  int distance =
      distance_get_pixel(wid->padding.bottom, ROFI_ORIENTATION_VERTICAL);
  distance += distance_get_pixel(wid->border.bottom, ROFI_ORIENTATION_VERTICAL);
  distance += distance_get_pixel(wid->margin.bottom, ROFI_ORIENTATION_VERTICAL);
  return distance;
}

int widget_padding_get_remaining_width(const widget *wid) {
  int width = wid->w;
  width -= widget_padding_get_left(wid);
  width -= widget_padding_get_right(wid);
  return width;
}
int widget_padding_get_remaining_height(const widget *wid) {
  int height = wid->h;
  height -= widget_padding_get_top(wid);
  height -= widget_padding_get_bottom(wid);
  return height;
}
int widget_padding_get_padding_height(const widget *wid) {
  int height = 0;
  height += widget_padding_get_top(wid);
  height += widget_padding_get_bottom(wid);
  return height;
}
int widget_padding_get_padding_width(const widget *wid) {
  int width = 0;
  width += widget_padding_get_left(wid);
  width += widget_padding_get_right(wid);
  return width;
}

int widget_get_desired_height(widget *wid, const int width) {
  if (wid == NULL) {
    return 0;
  }
  if (wid->get_desired_height == NULL) {
    return wid->h;
  }
  return wid->get_desired_height(wid, width);
}
int widget_get_desired_width(widget *wid, const int height) {
  if (wid == NULL) {
    return 0;
  }
  if (wid->get_desired_width == NULL) {
    return wid->w;
  }
  return wid->get_desired_width(wid, height);
}

int widget_get_absolute_xpos(widget *wid) {
  if (wid == NULL) {
    return 0;
  }
  int retv = wid->x;
  if (wid->parent != NULL) {
    retv += widget_get_absolute_xpos(wid->parent);
  }
  return retv;
}
int widget_get_absolute_ypos(widget *wid) {
  if (wid == NULL) {
    return 0;
  }
  int retv = wid->y;
  if (wid->parent != NULL) {
    retv += widget_get_absolute_ypos(wid->parent);
  }
  return retv;
}
