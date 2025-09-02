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
#ifdef XCB_IMDKIT
#include <xcb-imdkit/encoding.h>
#endif
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xkb.h>
#include <xkbcommon/xkbcommon-x11.h>

#include <cairo-xcb.h>
#include <cairo.h>
#include <gio/gio.h>

/** Indicated we understand the startup notification api is not yet stable.*/
#define SN_API_NOT_YET_FROZEN
#include "rofi.h"
#include <libsn/sn.h>

#include "settings.h"
#include "timings.h"

#include "display.h"
#include "helper-theme.h"
#include "helper.h"
#include "mode.h"
#include "modes/modes.h"
#include "xcb-internal.h"
#include "xrmoptions.h"

#include "view-internal.h"
#include "view.h"

#include "theme.h"

#include "xcb.h"

static int xcb_rofi_view_calculate_window_height(RofiViewState *state);

static void xcb_rofi_view_set_window_title(const char *title);

static void xcb_rofi_view_queue_redraw(void);

#ifdef XCB_IMDKIT
static void xim_commit_string(xcb_xim_t *im, G_GNUC_UNUSED xcb_xic_t ic,
                              G_GNUC_UNUSED uint32_t flag, char *str,
                              uint32_t length, G_GNUC_UNUSED uint32_t *keysym,
                              G_GNUC_UNUSED size_t nKeySym,
                              G_GNUC_UNUSED void *user_data);
static void xim_disconnected(G_GNUC_UNUSED xcb_xim_t *im,
                             G_GNUC_UNUSED void *user_data);
xcb_xim_im_callback xim_callback = {.forward_event =
                                        x11_event_handler_fowarding,
                                    .commit_string = xim_commit_string,
                                    .disconnected = xim_disconnected};
#endif

/** Thread pool used for filtering */
extern GThreadPool *tpool;

/**
 * Structure holding cached state.
 */
static struct {
  /** surface containing the fake background. */
  cairo_surface_t *fake_bg;
  /** Draw context  for main window */
  xcb_gcontext_t gc;
  /** Main X11 side pixmap to draw on. */
  xcb_pixmap_t edit_pixmap;
  /** Cairo Surface for edit_pixmap */
  cairo_surface_t *edit_surf;
  /** Drawable context for edit_surf */
  cairo_t *edit_draw;
  /** Indicate that fake background should be drawn relative to the window */
  int fake_bgrel;
  /** Current work area */
  workarea mon;
  /** timeout for reloading */
  guint idle_timeout;
  /** debug counter for redraws */
  unsigned long long count;
  /** redraw idle time. */
  guint repaint_source;
  /** Window fullscreen */
  gboolean fullscreen;
  /** Cursor type */
  X11CursorType cursor_type;
} XcbState = {
    .fake_bg = NULL,
    .edit_surf = NULL,
    .edit_draw = NULL,
    .fake_bgrel = FALSE,
    .idle_timeout = 0,
    .count = 0L,
    .repaint_source = 0,
    .fullscreen = FALSE,
};

static void xcb_rofi_view_get_current_monitor(int *width, int *height) {
  if (width) {
    *width = XcbState.mon.w;
  }
  if (height) {
    *height = XcbState.mon.h;
  }
}

/**
 * Code used for benchmarking drawing the gui, this will keep updating the UI as
 * fast as possible.
 */
gboolean do_bench = TRUE;

/**
 * Internal structure that hold benchmarking information.
 */
static struct {
  /** timer used for timestamping. */
  GTimer *time;
  /** number of draws done. */
  uint64_t draws;
  /** previous timestamp */
  double last_ts;
  /** minimum draw time. */
  double min;
} BenchMark = {.time = NULL, .draws = 0, .last_ts = 0.0, .min = G_MAXDOUBLE};

static gboolean bench_update(void) {
  if (!config.benchmark_ui) {
    return FALSE;
  }
  BenchMark.draws++;
  if (BenchMark.time == NULL) {
    BenchMark.time = g_timer_new();
  }

  if ((BenchMark.draws & 1023) == 0) {
    double ts = g_timer_elapsed(BenchMark.time, NULL);
    double fps = 1024 / (ts - BenchMark.last_ts);

    if (fps < BenchMark.min) {
      BenchMark.min = fps;
    }
    printf("current: %.2f fps, avg: %.2f fps, min: %.2f fps, %lu draws\r\n",
           fps, BenchMark.draws / ts, BenchMark.min, BenchMark.draws);

    BenchMark.last_ts = ts;
  }
  return TRUE;
}

static gboolean xcb_rofi_view_repaint(G_GNUC_UNUSED void *data) {
  RofiViewState *state = rofi_view_get_active();
  if (state) {
    // Repaint the view (if needed).
    // After a resize the edit_pixmap surface might not contain anything
    // anymore. If we already re-painted, this does nothing.

    TICK_N("Update start");
    rofi_view_update(state, FALSE);
    g_debug("expose event");
    TICK_N("Expose");
    xcb_copy_area(xcb->connection, XcbState.edit_pixmap, CacheState.main_window,
                  XcbState.gc, 0, 0, 0, 0, state->width, state->height);
    xcb_flush(xcb->connection);
    TICK_N("flush");
    XcbState.repaint_source = 0;
  }
  return (bench_update() == TRUE) ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

/**
 * @param state The handle to the view
 * @param qr    Indicate if queue_redraw should be called on changes.
 *
 * Update the state of the view. This involves filter state.
 */
static void xcb_rofi_view_update(RofiViewState *state, gboolean qr) {
  if (!widget_need_redraw(WIDGET(state->main_window))) {
    return;
  }
  g_debug("Redraw view");
  TICK();
  cairo_t *d = XcbState.edit_draw;
  cairo_set_operator(d, CAIRO_OPERATOR_SOURCE);
  if (XcbState.fake_bg != NULL) {
    if (XcbState.fake_bgrel) {
      cairo_set_source_surface(d, XcbState.fake_bg, 0.0, 0.0);
    } else {
      cairo_set_source_surface(d, XcbState.fake_bg,
                               (double)(XcbState.mon.x - state->x),
                               (double)(XcbState.mon.y - state->y));
    }
  } else {
    // Paint the background transparent.
    cairo_set_source_rgba(d, 0, 0, 0, 0.0);
  }
  cairo_paint(d);

  // Always paint as overlay over the background.
  cairo_set_operator(d, CAIRO_OPERATOR_OVER);

  TICK_N("Background");
  widget_draw(WIDGET(state->main_window), d);

#ifdef XCB_IMDKIT
  if (config.enable_imdkit) {
    int x = widget_get_x_pos(&state->text->widget) +
            textbox_get_cursor_x_pos(state->text);
    int y = widget_get_y_pos(&state->text->widget) +
            widget_get_height(&state->text->widget);
    rofi_set_im_window_pos(x, y);
  }
#endif

  TICK_N("widgets");
  cairo_surface_flush(XcbState.edit_surf);
  if (qr) {
    rofi_view_queue_redraw();
  }
}


/**
 * Calculates the window position
 */
/** Convert the old location to the new location type.
 * 123
 * 804
 * 765
 *
 * nw n ne
 * w  c e
 * sw s se
 */
static const int loc_transtable[9] = {
    WL_CENTER, WL_NORTH | WL_WEST, WL_NORTH, WL_NORTH | WL_EAST,
    WL_EAST,   WL_SOUTH | WL_EAST, WL_SOUTH, WL_SOUTH | WL_WEST,
    WL_WEST};
static void xcb_rofi_view_calculate_window_position(RofiViewState *state) {
  int location = rofi_theme_get_position(WIDGET(state->main_window), "location",
                                         loc_transtable[config.location]);
  int anchor =
      rofi_theme_get_position(WIDGET(state->main_window), "anchor", location);

  if (XcbState.fullscreen) {
    state->x = XcbState.mon.x;
    state->y = XcbState.mon.y;
    return;
  }
  state->y = XcbState.mon.y + (XcbState.mon.h) / 2;
  state->x = XcbState.mon.x + (XcbState.mon.w) / 2;
  // Determine window location
  switch (location) {
  case WL_NORTH_WEST:
    state->x = XcbState.mon.x;
    rofi_fallthrough;
  case WL_NORTH:
    state->y = XcbState.mon.y;
    break;
  case WL_NORTH_EAST:
    state->y = XcbState.mon.y;
    rofi_fallthrough;
  case WL_EAST:
    state->x = XcbState.mon.x + XcbState.mon.w;
    break;
  case WL_SOUTH_EAST:
    state->x = XcbState.mon.x + XcbState.mon.w;
    rofi_fallthrough;
  case WL_SOUTH:
    state->y = XcbState.mon.y + XcbState.mon.h;
    break;
  case WL_SOUTH_WEST:
    state->y = XcbState.mon.y + XcbState.mon.h;
    rofi_fallthrough;
  case WL_WEST:
    state->x = XcbState.mon.x;
    break;
  case WL_CENTER:;
    rofi_fallthrough;
  default:
    break;
  }
  switch (anchor) {
  case WL_SOUTH_WEST:
    state->y -= state->height;
    break;
  case WL_SOUTH:
    state->x -= state->width / 2;
    state->y -= state->height;
    break;
  case WL_SOUTH_EAST:
    state->x -= state->width;
    state->y -= state->height;
    break;
  case WL_NORTH_EAST:
    state->x -= state->width;
    break;
  case WL_NORTH_WEST:
    break;
  case WL_NORTH:
    state->x -= state->width / 2;
    break;
  case WL_EAST:
    state->x -= state->width;
    state->y -= state->height / 2;
    break;
  case WL_WEST:
    state->y -= state->height / 2;
    break;
  case WL_CENTER:
    state->y -= state->height / 2;
    state->x -= state->width / 2;
    break;
  default:
    break;
  }
  // Apply offset.
  RofiDistance x = rofi_theme_get_distance(WIDGET(state->main_window),
                                           "x-offset", config.x_offset);
  RofiDistance y = rofi_theme_get_distance(WIDGET(state->main_window),
                                           "y-offset", config.y_offset);
  state->x += distance_get_pixel(x, ROFI_ORIENTATION_HORIZONTAL);
  state->y += distance_get_pixel(y, ROFI_ORIENTATION_VERTICAL);
}

static void xcb_rofi_view_window_update_size(RofiViewState *state) {
  if (state == NULL) {
    return;
  }
  uint16_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
                  XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
  uint32_t vals[] = {state->x, state->y, state->width, state->height};

  // Display it.
  xcb_configure_window(xcb->connection, CacheState.main_window, mask, vals);
  cairo_destroy(XcbState.edit_draw);
  cairo_surface_destroy(XcbState.edit_surf);

  xcb_free_pixmap(xcb->connection, XcbState.edit_pixmap);
  XcbState.edit_pixmap = xcb_generate_id(xcb->connection);
  xcb_create_pixmap(xcb->connection, depth->depth, XcbState.edit_pixmap,
                    CacheState.main_window, state->width, state->height);

  XcbState.edit_surf =
      cairo_xcb_surface_create(xcb->connection, XcbState.edit_pixmap, visual,
                               state->width, state->height);
  XcbState.edit_draw = cairo_create(XcbState.edit_surf);

  g_debug("Re-size window based internal request: %dx%d.", state->width,
          state->height);
  // Should wrap main window in a widget.
  widget_resize(WIDGET(state->main_window), state->width, state->height);
}

static X11CursorType rofi_cursor_type_to_x11_cursor_type(RofiCursorType type) {
  switch (type) {
  case ROFI_CURSOR_DEFAULT:
    return CURSOR_DEFAULT;

  case ROFI_CURSOR_POINTER:
    return CURSOR_POINTER;

  case ROFI_CURSOR_TEXT:
    return CURSOR_TEXT;
  }

  return CURSOR_DEFAULT;
}

static void xcb_rofi_view_set_cursor(RofiCursorType type) {
  X11CursorType x11_type = rofi_cursor_type_to_x11_cursor_type(type);

  if (x11_type == XcbState.cursor_type) {
    return;
  }

  XcbState.cursor_type = x11_type;

  x11_set_cursor(CacheState.main_window, x11_type);
}

static void xcb_rofi_view_ping_mouse(RofiViewState *state) {
  xcb_query_pointer_cookie_t pointer_cookie =
      xcb_query_pointer(xcb->connection, CacheState.main_window);
  xcb_query_pointer_reply_t *pointer_reply =
      xcb_query_pointer_reply(xcb->connection, pointer_cookie, NULL);

  if (pointer_reply == NULL) {
    return;
  }

  rofi_view_handle_mouse_motion(state, pointer_reply->win_x,
                                pointer_reply->win_y, config.hover_select);

  free(pointer_reply);
}

static gboolean xcb_rofi_view_reload_idle(G_GNUC_UNUSED gpointer data) {
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
    xcb_rofi_view_queue_redraw();
  }
  XcbState.idle_timeout = 0;
  return G_SOURCE_REMOVE;
}

static void xcb_rofi_view_reload(void) {
  // @TODO add check if current view is equal to the callee
  if (XcbState.idle_timeout == 0) {
    XcbState.idle_timeout =
        g_timeout_add(1000 / 100, xcb_rofi_view_reload_idle, NULL);
  }
}
static void xcb_rofi_view_queue_redraw(void) {
  RofiViewState *state = rofi_view_get_active();

  if (state && XcbState.repaint_source == 0) {
    XcbState.count++;
    g_debug("redraw %llu", XcbState.count);
    XcbState.repaint_source = g_idle_add_full(
        G_PRIORITY_HIGH_IDLE, xcb_rofi_view_repaint, NULL, NULL);
  }
}

static void
xcb_rofi_view_setup_fake_transparency(widget *win,
                                      const char *const fake_background) {
  if (XcbState.fake_bg == NULL) {
    cairo_surface_t *s = NULL;
    /**
     * Select Background to use for fake transparency.
     * Current options: 'real', 'screenshot','background'
     */
    TICK_N("Fake start");
    if (g_strcmp0(fake_background, "real") == 0) {
      return;
    } else if (g_strcmp0(fake_background, "screenshot") == 0) {
      s = x11_helper_get_screenshot_surface();
    } else if (g_strcmp0(fake_background, "background") == 0) {
      s = x11_helper_get_bg_surface();
    } else {
      char *fpath = rofi_expand_path(fake_background);
      g_debug("Opening %s to use as background.", fpath);
      s = cairo_image_surface_create_from_png(fpath);
      XcbState.fake_bgrel = TRUE;
      g_free(fpath);
    }
    TICK_N("Get surface.");
    if (s != NULL) {
      if (cairo_surface_status(s) != CAIRO_STATUS_SUCCESS) {
        g_debug("Failed to open surface fake background: %s",
                cairo_status_to_string(cairo_surface_status(s)));
        cairo_surface_destroy(s);
        s = NULL;
      } else {
        XcbState.fake_bg = cairo_image_surface_create(
            CAIRO_FORMAT_ARGB32, XcbState.mon.w, XcbState.mon.h);

        int blur = rofi_theme_get_integer(WIDGET(win), "blur", 0);
        cairo_t *dr = cairo_create(XcbState.fake_bg);
        if (XcbState.fake_bgrel) {
          cairo_set_source_surface(dr, s, 0, 0);
        } else {
          cairo_set_source_surface(dr, s, -XcbState.mon.x, -XcbState.mon.y);
        }
        cairo_paint(dr);
        cairo_destroy(dr);
        cairo_surface_destroy(s);
        if (blur > 0) {
          cairo_image_surface_blur(XcbState.fake_bg, (double)blur, 0);
          TICK_N("BLUR");
        }
      }
    }
    TICK_N("Fake transparency");
  }
}

#ifdef XCB_IMDKIT
static void xim_commit_string(xcb_xim_t *im, G_GNUC_UNUSED xcb_xic_t ic,
                              G_GNUC_UNUSED uint32_t flag, char *str,
                              uint32_t length, G_GNUC_UNUSED uint32_t *keysym,
                              G_GNUC_UNUSED size_t nKeySym,
                              G_GNUC_UNUSED void *user_data) {
  RofiViewState *state = rofi_view_get_active();
  if (state == NULL) {
    return;
  }

#ifndef XCB_IMDKIT_1_0_3_LOWER
  if (xcb_xim_get_encoding(im) == XCB_XIM_UTF8_STRING) {
    rofi_view_handle_text(state, str);
  } else if (xcb_xim_get_encoding(im) == XCB_XIM_COMPOUND_TEXT) {
    size_t newLength = 0;
    char *utf8 = xcb_compound_text_to_utf8(str, length, &newLength);
    if (utf8) {
      rofi_view_handle_text(state, utf8);
    }
  }
#else
  size_t newLength = 0;
  char *utf8 = xcb_compound_text_to_utf8(str, length, &newLength);
  if (utf8) {
    rofi_view_handle_text(state, utf8);
  }
#endif
}

static void xim_disconnected(G_GNUC_UNUSED xcb_xim_t *im,
                             G_GNUC_UNUSED void *user_data) {
  xcb->ic = 0;
}

static void create_ic_callback(xcb_xim_t *im, xcb_xic_t new_ic,
                               G_GNUC_UNUSED void *user_data) {
  xcb->ic = new_ic;
  if (xcb->ic) {
    xcb_xim_set_ic_focus(im, xcb->ic);
  }
}

gboolean rofi_set_im_window_pos(int new_x, int new_y) {
  if (!xcb->ic)
    return false;

  static xcb_point_t spot = {.x = 0, .y = 0};
  if (spot.x != new_x || spot.y != new_y) {
    spot.x = new_x;
    spot.y = new_y;
    xcb_xim_nested_list nested = xcb_xim_create_nested_list(
        xcb->im, XCB_XIM_XNSpotLocation, &spot, NULL);
    xcb_xim_set_ic_values(xcb->im, xcb->ic, NULL, NULL, XCB_XIM_XNClientWindow,
                          &CacheState.main_window, XCB_XIM_XNFocusWindow,
                          &CacheState.main_window, XCB_XIM_XNPreeditAttributes,
                          &nested, NULL);
    free(nested.data);
  }
  return true;
}
static void open_xim_callback(xcb_xim_t *im, G_GNUC_UNUSED void *user_data) {
  RofiViewState *state = rofi_view_get_active();
  uint32_t input_style = XCB_IM_PreeditPosition | XCB_IM_StatusArea;
  xcb_point_t spot;
  spot.x = widget_get_x_pos(&state->text->widget) +
           textbox_get_cursor_x_pos(state->text);
  spot.y = widget_get_y_pos(&state->text->widget) +
           widget_get_height(&state->text->widget);
  xcb_xim_nested_list nested =
      xcb_xim_create_nested_list(im, XCB_XIM_XNSpotLocation, &spot, NULL);
  xcb_xim_create_ic(
      im, create_ic_callback, NULL, XCB_XIM_XNInputStyle, &input_style,
      XCB_XIM_XNClientWindow, &CacheState.main_window, XCB_XIM_XNFocusWindow,
      &CacheState.main_window, XCB_XIM_XNPreeditAttributes, &nested, NULL);
  free(nested.data);
}
#endif

static void xcb___create_window(MenuFlags menu_flags) {
  input_history_initialize();

  uint32_t selmask = XCB_CW_BACK_PIXMAP | XCB_CW_BORDER_PIXEL |
                     XCB_CW_BIT_GRAVITY | XCB_CW_BACKING_STORE |
                     XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
  uint32_t xcb_event_masks =
      XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS |
      XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_KEY_PRESS |
      XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_KEYMAP_STATE |
      XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_FOCUS_CHANGE |
      XCB_EVENT_MASK_BUTTON_1_MOTION | XCB_EVENT_MASK_POINTER_MOTION;

  uint32_t selval[] = {XCB_BACK_PIXMAP_NONE, 0,
                       XCB_GRAVITY_STATIC,   XCB_BACKING_STORE_NOT_USEFUL,
                       xcb_event_masks,      map};

#ifdef XCB_IMDKIT
  if (config.enable_imdkit) {
    xcb_xim_set_im_callback(xcb->im, &xim_callback, NULL);

    // Open connection to XIM server.
    xcb_xim_open(xcb->im, open_xim_callback, true, NULL);
  }
#endif

  xcb_window_t box_window = xcb_generate_id(xcb->connection);
  xcb_void_cookie_t cc = xcb_create_window_checked(
      xcb->connection, depth->depth, box_window, xcb_stuff_get_root_window(), 0,
      0, 200, 100, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, visual->visual_id, selmask,
      selval);
  xcb_generic_error_t *error;
  error = xcb_request_check(xcb->connection, cc);
  if (error) {
    g_error("xcb_create_window() failed error=0x%x\n", error->error_code);
    exit(EXIT_FAILURE);
  }

  TICK_N("xcb create window");
  XcbState.gc = xcb_generate_id(xcb->connection);
  xcb_create_gc(xcb->connection, XcbState.gc, box_window, 0, 0);

  TICK_N("xcb create gc");
  // Create a drawable.
  XcbState.edit_pixmap = xcb_generate_id(xcb->connection);
  xcb_create_pixmap(xcb->connection, depth->depth, XcbState.edit_pixmap,
                    CacheState.main_window, 200, 100);

  XcbState.edit_surf = cairo_xcb_surface_create(
      xcb->connection, XcbState.edit_pixmap, visual, 200, 100);
  XcbState.edit_draw = cairo_create(XcbState.edit_surf);

  TICK_N("create cairo surface");
  // Set up pango context.
  cairo_font_options_t *fo = cairo_font_options_create();
  // Take font description from xlib surface
  cairo_surface_get_font_options(XcbState.edit_surf, fo);
  // TODO should we update the drawable each time?
  PangoContext *p = pango_cairo_create_context(XcbState.edit_draw);
  // Set the font options from the xlib surface
  pango_cairo_context_set_font_options(p, fo);
  TICK_N("pango cairo font setup");

  CacheState.main_window = box_window;
  CacheState.flags = menu_flags;
  monitor_active(&(XcbState.mon));
  // Setup dpi
  if (config.dpi > 1) {
    PangoFontMap *font_map = pango_cairo_font_map_get_default();
    pango_cairo_font_map_set_resolution((PangoCairoFontMap *)font_map,
                                        (double)config.dpi);
  } else if (config.dpi == 0 || config.dpi == 1) {
    // Auto-detect mode.
    double dpi = 96;
    if (XcbState.mon.mh > 0 && config.dpi == 1) {
      dpi = (XcbState.mon.h * 25.4) / (double)(XcbState.mon.mh);
    } else {
      dpi = (xcb->screen->height_in_pixels * 25.4) /
            (double)(xcb->screen->height_in_millimeters);
    }

    g_debug("Auto-detected DPI: %.2lf", dpi);
    PangoFontMap *font_map = pango_cairo_font_map_get_default();
    pango_cairo_font_map_set_resolution((PangoCairoFontMap *)font_map, dpi);
    config.dpi = dpi;
  } else {
    // default pango is 96.
    PangoFontMap *font_map = pango_cairo_font_map_get_default();
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
  cairo_font_options_destroy(fo);

  TICK_N("textbox setup");
  // // make it an unmanaged window
  if (((menu_flags & MENU_TRANSIENT_WINDOW) != 0)) {
    xcb_atom_t atoms[] = {xcb->ewmh._NET_WM_STATE_MODAL};

    window_set_atom_prop(box_window, xcb->ewmh._NET_WM_STATE, atoms,
                         sizeof(atoms) / sizeof(xcb_atom_t));
    window_set_atom_prop(box_window, xcb->ewmh._NET_WM_WINDOW_TYPE,
                         &(xcb->ewmh._NET_WM_WINDOW_TYPE_UTILITY), 1);
    x11_disable_decoration(box_window);

    xcb_window_t active_window;
    xcb_get_property_cookie_t awc;
    awc = xcb_ewmh_get_active_window(&xcb->ewmh, xcb->screen_nbr);

    if (xcb_ewmh_get_active_window_reply(&xcb->ewmh, awc, &active_window,
                                         NULL)) {
      xcb_change_property(xcb->connection, XCB_PROP_MODE_REPLACE, box_window,
                          XCB_ATOM_WM_TRANSIENT_FOR, XCB_ATOM_WINDOW, 32, 1,
                          &active_window);
    }
  } else if (((menu_flags & MENU_NORMAL_WINDOW) == 0)) {
    window_set_atom_prop(box_window, xcb->ewmh._NET_WM_STATE,
                         &(xcb->ewmh._NET_WM_STATE_ABOVE), 1);
    uint32_t values[] = {1};
    xcb_change_window_attributes(xcb->connection, box_window,
                                 XCB_CW_OVERRIDE_REDIRECT, values);
  } else {
    window_set_atom_prop(box_window, xcb->ewmh._NET_WM_WINDOW_TYPE,
                         &(xcb->ewmh._NET_WM_WINDOW_TYPE_NORMAL), 1);
    x11_disable_decoration(box_window);
  }

  TICK_N("setup window attributes");
  XcbState.fullscreen =
      rofi_theme_get_boolean(WIDGET(win), "fullscreen", FALSE);
  if (XcbState.fullscreen) {
    xcb_atom_t atoms[] = {xcb->ewmh._NET_WM_STATE_FULLSCREEN,
                          xcb->ewmh._NET_WM_STATE_ABOVE};
    window_set_atom_prop(box_window, xcb->ewmh._NET_WM_STATE, atoms,
                         sizeof(atoms) / sizeof(xcb_atom_t));
  }

  xcb_atom_t protocols[] = {netatoms[WM_TAKE_FOCUS]};
  xcb_icccm_set_wm_protocols(xcb->connection, box_window,
                             xcb->ewmh.WM_PROTOCOLS, G_N_ELEMENTS(protocols),
                             protocols);

  TICK_N("setup window fullscreen");
  // Set the WM_NAME
  xcb_rofi_view_set_window_title("rofi");
  const char wm_class_name[] = "rofi\0Rofi";
  xcb_icccm_set_wm_class(xcb->connection, box_window, sizeof(wm_class_name),
                         wm_class_name);

  TICK_N("setup window name and class");
  const char *transparency =
      rofi_theme_get_string(WIDGET(win), "transparency", NULL);
  if (transparency) {
    xcb_rofi_view_setup_fake_transparency(WIDGET(win), transparency);
  }
  if (xcb->sncontext != NULL) {
    sn_launchee_context_setup_window(xcb->sncontext, CacheState.main_window);
  }
  TICK_N("setup startup notification");
  widget_free(WIDGET(win));
  TICK_N("done");

  // Set the PID.
  pid_t pid = getpid();
  xcb_ewmh_set_wm_pid(&(xcb->ewmh), CacheState.main_window, pid);

  // Get hostname
  const char *hostname = g_get_host_name();
  char *ahost = g_hostname_to_ascii(hostname);
  if (ahost != NULL) {
    xcb_icccm_set_wm_client_machine(xcb->connection, CacheState.main_window,
                                    XCB_ATOM_STRING, 8, strlen(ahost), ahost);
    g_free(ahost);
  }
}

/**
 * @param state Internal state of the menu.
 *
 * Calculate the width of the window and the width of an element.
 */
static void xcb_rofi_view_calculate_window_width(RofiViewState *state) {
  if (XcbState.fullscreen) {
    state->width = XcbState.mon.w;
    return;
  }
  // Calculate as float to stop silly, big rounding down errors.
  state->width = (XcbState.mon.w / 100.0f) * DEFAULT_MENU_WIDTH;
  // Use theme configured width, if set.
  RofiDistance width = rofi_theme_get_distance(WIDGET(state->main_window),
                                               "width", state->width);
  state->width = distance_get_pixel(width, ROFI_ORIENTATION_HORIZONTAL);
}

/**
 * Handle window configure event.
 * Handles resizes.
 */
static void
xcb_rofi_view_temp_configure_notify(RofiViewState *state,
                                    xcb_configure_notify_event_t *xce) {
  if (xce->window == CacheState.main_window) {
    if (state->x != xce->x || state->y != xce->y) {
      state->x = xce->x;
      state->y = xce->y;
      widget_queue_redraw(WIDGET(state->main_window));
    }
    if (state->width != xce->width || state->height != xce->height) {
      state->width = xce->width;
      state->height = xce->height;

      cairo_destroy(XcbState.edit_draw);
      cairo_surface_destroy(XcbState.edit_surf);

      xcb_free_pixmap(xcb->connection, XcbState.edit_pixmap);
      XcbState.edit_pixmap = xcb_generate_id(xcb->connection);
      xcb_create_pixmap(xcb->connection, depth->depth, XcbState.edit_pixmap,
                        CacheState.main_window, state->width, state->height);

      XcbState.edit_surf =
          cairo_xcb_surface_create(xcb->connection, XcbState.edit_pixmap,
                                   visual, state->width, state->height);
      XcbState.edit_draw = cairo_create(XcbState.edit_surf);
      g_debug("Re-size window based external request: %d %d", state->width,
              state->height);
      widget_resize(WIDGET(state->main_window), state->width, state->height);
    }
  }
}

/**
 * Quit rofi on click (outside of view )
 */
static void xcb_rofi_view_temp_click_to_exit(RofiViewState *state,
                                             xcb_window_t target) {
  if ((CacheState.flags & MENU_NORMAL_WINDOW) == 0) {
    if (target != CacheState.main_window) {
      state->quit = TRUE;
      state->retv = MENU_CANCEL;
    }
  }
}

static void xcb_rofi_view_frame_callback(void) {
  if (XcbState.repaint_source == 0) {
    XcbState.count++;
    g_debug("redraw %llu", XcbState.count);
    XcbState.repaint_source = g_idle_add_full(
        G_PRIORITY_HIGH_IDLE, xcb_rofi_view_repaint, NULL, NULL);
  }
}

static int xcb_rofi_view_calculate_window_height(RofiViewState *state) {
  if (XcbState.fullscreen == TRUE) {
    return XcbState.mon.h;
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

static void xcb_rofi_view_hide(void) {
  if (CacheState.main_window != XCB_WINDOW_NONE) {
    display_revert_input_focus();
    xcb_unmap_window(xcb->connection, CacheState.main_window);
    display_early_cleanup();
  }
}

static void xcb_rofi_view_cleanup(void) {
  // Clear clipboard data.
  xcb_stuff_set_clipboard(NULL);
  g_debug("Cleanup.");
  if (XcbState.idle_timeout > 0) {
    g_source_remove(XcbState.idle_timeout);
    XcbState.idle_timeout = 0;
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
  if (XcbState.repaint_source > 0) {
    g_source_remove(XcbState.repaint_source);
    XcbState.repaint_source = 0;
  }
  if (XcbState.fake_bg) {
    cairo_surface_destroy(XcbState.fake_bg);
    XcbState.fake_bg = NULL;
  }
  if (XcbState.edit_draw) {
    cairo_destroy(XcbState.edit_draw);
    XcbState.edit_draw = NULL;
  }
  if (XcbState.edit_surf) {
    cairo_surface_destroy(XcbState.edit_surf);
    XcbState.edit_surf = NULL;
  }
  if (CacheState.main_window != XCB_WINDOW_NONE) {
    g_debug("Unmapping and free'ing window");
    xcb_unmap_window(xcb->connection, CacheState.main_window);
    xcb_free_gc(xcb->connection, XcbState.gc);
    xcb_free_pixmap(xcb->connection, XcbState.edit_pixmap);
    xcb_destroy_window(xcb->connection, CacheState.main_window);
    CacheState.main_window = XCB_WINDOW_NONE;
  }
  if (map != XCB_COLORMAP_NONE) {
    xcb_free_colormap(xcb->connection, map);
    map = XCB_COLORMAP_NONE;
  }
  xcb_flush(xcb->connection);
  g_assert(g_queue_is_empty(&(CacheState.views)));

  input_history_save();
}

static xcb_window_t xcb_rofi_view_get_window(void) {
  return CacheState.main_window;
}

static void xcb_rofi_view_set_window_title(const char *title) {
  ssize_t len = strlen(title);
  xcb_change_property(xcb->connection, XCB_PROP_MODE_REPLACE,
                      CacheState.main_window, xcb->ewmh._NET_WM_NAME,
                      xcb->ewmh.UTF8_STRING, 8, len, title);
  xcb_change_property(xcb->connection, XCB_PROP_MODE_REPLACE,
                      CacheState.main_window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING,
                      8, len, title);
}

static view_proxy view_ = {
    .update = xcb_rofi_view_update,
    .temp_configure_notify = xcb_rofi_view_temp_configure_notify,
    .temp_click_to_exit = xcb_rofi_view_temp_click_to_exit,
    .frame_callback = xcb_rofi_view_frame_callback,
    .queue_redraw = xcb_rofi_view_queue_redraw,

    .set_window_title = xcb_rofi_view_set_window_title,
    .calculate_window_position = xcb_rofi_view_calculate_window_position,
    .calculate_window_width = xcb_rofi_view_calculate_window_width,
    .calculate_window_height = xcb_rofi_view_calculate_window_height,
    .window_update_size = xcb_rofi_view_window_update_size,
    .set_cursor = xcb_rofi_view_set_cursor,
    .ping_mouse = xcb_rofi_view_ping_mouse,

    .cleanup = xcb_rofi_view_cleanup,
    .hide = xcb_rofi_view_hide,
    .reload = xcb_rofi_view_reload,

    .__create_window = xcb___create_window,
    .get_window = xcb_rofi_view_get_window,
    .get_current_monitor = xcb_rofi_view_get_current_monitor,

    .set_size = NULL,
    .get_size = NULL,
};

const view_proxy *xcb_view_proxy = &view_;
