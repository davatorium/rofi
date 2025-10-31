/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2020 Qball Cow <qball@gmpclient.org>
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

/** The Rofi View log domain */
#define G_LOG_DOMAIN "View"

#include <config.h>
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <cairo.h>

#include "rofi.h"
#include "settings.h"
#include "timings.h"

#include "display.h"
#include "helper-theme.h"
#include "helper.h"
#include "mode.h"
#include "modes/modes.h"

#include "view-internal.h"
#include "view.h"

#include "theme.h"

/**
 * @param state The handle to the view
 * @param qr    Indicate if queue_redraw should be called on changes.
 *
 * Update the state of the view. This involves filter state.
 */
static void wayland_rofi_view_update(RofiViewState *state, gboolean qr);

/**
 * Structure holding some state
 */
static struct {
  /** Main flags */
  MenuFlags flags;
  /** timeout for reloading */
  guint idle_timeout;
  /** debug counter for redraws */
  unsigned long long count;
  /** redraw idle time. */
  guint repaint_source;
  /** Window fullscreen */
  gboolean fullscreen;

  int monitor_width;
  int monitor_height;
} WlState = {
    .flags = MENU_NORMAL,
    .idle_timeout = 0,
    .count = 0L,
    .repaint_source = 0,
    .fullscreen = FALSE,
    .monitor_width = 0,
    .monitor_height = 0,
};

static void wayland_rofi_view_get_current_monitor(int *width, int *height) {
  // TODO: handle changing monitor resolution
  if (WlState.monitor_width == 0 && WlState.monitor_height == 0) {
    display_get_surface_dimensions(&WlState.monitor_width,
                                   &WlState.monitor_height);
  }

  if (width) {
    *width = WlState.monitor_width;
  }
  if (height) {
    *height = WlState.monitor_height;
  }
}

static gboolean wayland_rofi_view_repaint(G_GNUC_UNUSED void *data) {
  RofiViewState *state = rofi_view_get_active();
  if (state) {
    // Repaint the view (if needed).
    // After a resize the edit_pixmap surface might not contain anything
    // anymore. If we already re-painted, this does nothing.
    rofi_view_maybe_update(state);
    WlState.repaint_source = 0;
  }
  return G_SOURCE_REMOVE;
}

static const int loc_transtable[9] = {
    WL_CENTER, WL_NORTH | WL_WEST, WL_NORTH, WL_NORTH | WL_EAST,
    WL_EAST,   WL_SOUTH | WL_EAST, WL_SOUTH, WL_SOUTH | WL_WEST,
    WL_WEST};

static void wayland_rofi_view_calculate_window_position(
    G_GNUC_UNUSED RofiViewState *state) {}

static int rofi_get_location(RofiViewState *state) {
  return rofi_theme_get_position(WIDGET(state->main_window), "location",
                                 loc_transtable[config.location]);
}

static int rofi_get_offset_px(RofiViewState *state, RofiOrientation ori) {
  char *property = ori == ROFI_ORIENTATION_HORIZONTAL ? "x-offset" : "y-offset";

  RofiDistance offset =
      rofi_theme_get_distance(WIDGET(state->main_window), property, 0);
  return distance_get_pixel(offset, ori);
}

static void wayland_rofi_view_window_update_size(RofiViewState *state) {
  if (state == NULL) {
    return;
  }
  int offset_x = rofi_get_offset_px(state, ROFI_ORIENTATION_HORIZONTAL);
  int offset_y = rofi_get_offset_px(state, ROFI_ORIENTATION_VERTICAL);

  widget_resize(WIDGET(state->main_window), state->width, state->height);
  display_set_surface_dimensions(state->width, state->height, offset_x,
                                 offset_y, rofi_get_location(state));
  rofi_view_pool_refresh();
}

static void wayland_rofi_view_set_size(RofiViewState *state, gint width,
                                       gint height) {
  if (width > -1) {
    state->width = width;
  }
  if (height > -1) {
    state->height = height;
  }
  wayland_rofi_view_window_update_size(state);
}

static void wayland_rofi_view_get_size(RofiViewState *state, gint *width,
                                       gint *height) {
  *width = state->width;
  *height = state->height;
}

static void wayland_rofi_view_ping_mouse(RofiViewState *state) { (void)state; }

static gboolean wayland_rofi_view_reload_idle(G_GNUC_UNUSED gpointer data) {
  RofiViewState *state = rofi_view_get_active();

  if (state) {
    // For UI update on this.
    if (state->tb_total_rows) {
      char *r = g_strdup_printf("%u", mode_get_num_entries(state->sw));
      textbox_text(state->tb_total_rows, r);
      g_free(r);
    }
    state->reload = TRUE;
    state->refilter = TRUE;

    rofi_view_maybe_update(state);
  }
  WlState.idle_timeout = 0;
  return G_SOURCE_REMOVE;
}

static void wayland_rofi_view_reload(void) {
  // @TODO add check if current view is equal to the callee
  if (WlState.idle_timeout == 0) {
    WlState.idle_timeout =
        g_timeout_add(1000 / 15, wayland_rofi_view_reload_idle, NULL);
  }
}
static void wayland_rofi_view_queue_redraw(void) {
  RofiViewState *state = rofi_view_get_active();
  if (state && WlState.repaint_source == 0) {
    WlState.count++;
    g_debug("redraw %llu", WlState.count);

    widget_queue_redraw(WIDGET(state->main_window));

    WlState.repaint_source = g_idle_add_full(
        G_PRIORITY_HIGH_IDLE, wayland_rofi_view_repaint, NULL, NULL);
  }
}

static void wayland___create_window(MenuFlags menu_flags) {
  input_history_initialize();

  TICK_N("create cairo surface");
  // TODO should we update the drawable each time?
  PangoContext *p = pango_context_new();
  pango_context_set_font_map(p, pango_cairo_font_map_get_default());
  TICK_N("pango cairo font setup");

  WlState.flags = menu_flags;
  // Setup dpi
  PangoFontMap *font_map = pango_cairo_font_map_get_default();
  if (config.dpi > 1) {
    pango_cairo_font_map_set_resolution((PangoCairoFontMap *)font_map,
                                        (double)config.dpi);
  } else if (config.dpi == 0 || config.dpi == 1) {
    // This is an heuristic that works well for simple cases (eg single monitor)
    // Accurate dpi information only comes after we display the first surface on
    // the screen when it's too late to use for metric units.
    double dpi = wayland_get_dpi_estimation();
    if (dpi >= 0) {
      config.dpi = dpi;
      pango_cairo_font_map_set_resolution((PangoCairoFontMap *)font_map, dpi);
    } else {
      g_warning(
          "DPI auto-detect failed, the output is not known yet or does not "
          "provide a physical size");
      config.dpi =
          pango_cairo_font_map_get_resolution((PangoCairoFontMap *)font_map);
    }
  } else {
    // default pango is 96.
    config.dpi =
        pango_cairo_font_map_get_resolution((PangoCairoFontMap *)font_map);
  }
  // Setup font.
  // Dummy widget.
  box *win = box_create(NULL, "window", ROFI_ORIENTATION_HORIZONTAL);
  const char *font =
      rofi_theme_get_string(WIDGET(win), "font", config.menu_font);
  if (font) {
    PangoFontDescription *pfd = pango_font_description_from_string(font);
    if (helper_validate_font(pfd, font)) {
      pango_context_set_font_description(p, pfd);
    }
    pango_font_description_free(pfd);
  }
  PangoLanguage *l = pango_language_get_default();
  pango_context_set_language(p, l);
  TICK_N("configure font");

  // Tell textbox to use this context.
  textbox_set_pango_context(font, p);
  // cleanup
  g_object_unref(p);

  WlState.fullscreen = rofi_theme_get_boolean(WIDGET(win), "fullscreen", FALSE);

  if (WlState.fullscreen) {
    display_set_fullscreen_mode();
  }

  widget_free(WIDGET(win));

  TICK_N("done");
}

/**
 * @param state Internal state of the menu.
 *
 * Calculate the width of the window and the width of an element.
 */
static void wayland_rofi_view_calculate_window_width(RofiViewState *state) {
  int screen_width = 1920;
  display_get_surface_dimensions(&screen_width, NULL);

  if (WlState.fullscreen == TRUE) {
    state->width = screen_width;
    return;
  }

  // Calculate as float to stop silly, big rounding down errors.
  state->width = (screen_width / 100.0f) * DEFAULT_MENU_WIDTH;
  // Use theme configured width, if set.
  RofiDistance width = rofi_theme_get_distance(WIDGET(state->main_window),
                                               "width", state->width);
  state->width = distance_get_pixel(width, ROFI_ORIENTATION_HORIZONTAL);
}

static void wayland_rofi_view_update(RofiViewState *state, gboolean qr) {
  if (!widget_need_redraw(WIDGET(state->main_window))) {
    return;
  }
  g_debug("Redraw view");
  TICK();
  if (state->pool == NULL) {
    state->pool = display_buffer_pool_new(state->width, state->height);
  }
  cairo_surface_t *surface = display_buffer_pool_get_next_buffer(state->pool);
  if (surface == NULL) {
    // no available buffer, bail out
    return;
  }
  cairo_t *d = cairo_create(surface);
  cairo_set_operator(d, CAIRO_OPERATOR_SOURCE);
  // Paint the background transparent.
  cairo_set_source_rgba(d, 0, 0, 0, 0.0);
  guint scale = display_scale();
  cairo_surface_set_device_scale(surface, scale, scale);
  cairo_paint(d);
  TICK_N("Background");

  // Always paint as overlay over the background.
  cairo_set_operator(d, CAIRO_OPERATOR_OVER);
  widget_draw(WIDGET(state->main_window), d);

  TICK_N("widgets");
  cairo_destroy(d);
  display_surface_commit(surface);

  if (qr) {
    wayland_rofi_view_queue_redraw();
  }
}

/**
 * @param state The Menu Handle
 *
 * Check if a finalize function is set, and if sets executes it.
 */
void process_result(RofiViewState *state);

static void wayland_rofi_view_frame_callback(void) {
  if (WlState.repaint_source == 0) {
    WlState.repaint_source = g_idle_add_full(
        G_PRIORITY_HIGH_IDLE, wayland_rofi_view_repaint, NULL, NULL);
  }
}

static int wayland_rofi_view_calculate_window_height(RofiViewState *state) {
  if (WlState.fullscreen == TRUE) {
    int height = 1080;
    display_get_surface_dimensions(NULL, &height);
    return height;
  }

  RofiDistance h =
      rofi_theme_get_distance(WIDGET(state->main_window), "height", 0);
  unsigned int height = distance_get_pixel(h, ROFI_ORIENTATION_VERTICAL);
  // If height is set, return it.
  if (height > 0) {
    return height;
  }
  // Autosize based on widgets.
  widget *main_window = WIDGET(state->main_window);

  return widget_get_desired_height(main_window, state->width);
}

static void wayland_rofi_view_hide(void) { display_early_cleanup(); }

static void wayland_rofi_view_cleanup(void) {
  g_debug("Cleanup.");
  if (WlState.idle_timeout > 0) {
    g_source_remove(WlState.idle_timeout);
    WlState.idle_timeout = 0;
  }
  if (CacheState.refilter_timeout > 0) {
    g_source_remove(CacheState.refilter_timeout);
    CacheState.refilter_timeout = 0;
  }
  if (CacheState.overlay_timeout) {
    g_source_remove(CacheState.overlay_timeout);
    CacheState.overlay_timeout = 0;
  }
  if (CacheState.user_timeout > 0) {
    g_source_remove(CacheState.user_timeout);
    CacheState.user_timeout = 0;
  }
  if (WlState.repaint_source > 0) {
    g_source_remove(WlState.repaint_source);
    WlState.repaint_source = 0;
  }

  input_history_save();
}

static void
wayland_rofi_view_set_window_title(G_GNUC_UNUSED const char *title) {}

static void wayland_rofi_view_pool_refresh(void) {
  RofiViewState *state = rofi_view_get_active();
  if (state == NULL) {
    return;
  }
  display_buffer_pool_free(state->pool);
  state->pool = NULL;
  wayland_rofi_view_update(state, TRUE);
}

static view_proxy view_ = {
    .update = wayland_rofi_view_update,
    .temp_configure_notify = NULL,
    .temp_click_to_exit = NULL,
    .frame_callback = wayland_rofi_view_frame_callback,
    .queue_redraw = wayland_rofi_view_queue_redraw,

    .set_window_title = wayland_rofi_view_set_window_title,
    .calculate_window_position = wayland_rofi_view_calculate_window_position,
    .calculate_window_height = wayland_rofi_view_calculate_window_height,
    .calculate_window_width = wayland_rofi_view_calculate_window_width,
    .window_update_size = wayland_rofi_view_window_update_size,
    .set_cursor = wayland_display_set_cursor_type,
    .ping_mouse = wayland_rofi_view_ping_mouse,

    .cleanup = wayland_rofi_view_cleanup,
    .hide = wayland_rofi_view_hide,
    .reload = wayland_rofi_view_reload,

    .__create_window = wayland___create_window,
    .get_window = NULL,
    .get_current_monitor = wayland_rofi_view_get_current_monitor,

    .set_size = wayland_rofi_view_set_size,
    .get_size = wayland_rofi_view_get_size,

    .pool_refresh = wayland_rofi_view_pool_refresh,
};

const view_proxy *wayland_view_proxy = &view_;
