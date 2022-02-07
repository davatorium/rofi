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

#ifndef ROFI_XCB_INTERNAL_H
#define ROFI_XCB_INTERNAL_H
/** Indication we accept that startup notification api is not yet frozen */
#define SN_API_NOT_YET_FROZEN
#include <glib.h>
#include <libsn/sn.h>

#include <libgwater-xcb.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>

#include <nkutils-bindings.h>

/**
 * Structure to keep xcb stuff around.
 */
struct _xcb_stuff {
  GMainLoop *main_loop;
  GWaterXcbSource *source;
  xcb_connection_t *connection;
  xcb_ewmh_connection_t ewmh;
  xcb_screen_t *screen;
  int screen_nbr;
  SnDisplay *sndisplay;
  SnLauncheeContext *sncontext;
  struct _workarea *monitors;
  struct {
    /** Flag indicating first event */
    uint8_t first_event;
    /** Keyboard device id */
    int32_t device_id;
  } xkb;
  xcb_timestamp_t last_timestamp;
  NkBindingsSeat *bindings_seat;
  gboolean mouse_seen;
  xcb_window_t focus_revert;
};

#endif
