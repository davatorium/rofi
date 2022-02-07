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

/** Log domain used by timings.*/
#define G_LOG_DOMAIN "Timings"

#include "timings.h"
#include "config.h"
#include "rofi.h"
#include <stdio.h>
/**
 * Timer used to calculate time stamps.
 */
GTimer *global_timer = NULL;
/**
 * Last timestamp made.
 */
double global_timer_last = 0.0;

void rofi_timings_init(void) {
  global_timer = g_timer_new();
  double now = g_timer_elapsed(global_timer, NULL);
  g_debug("%4.6f (%2.6f): Started", now, 0.0);
}

void rofi_timings_tick(const char *file, char const *str, int line,
                       char const *msg) {
  double now = g_timer_elapsed(global_timer, NULL);

  g_debug("%4.6f (%2.6f): %s:%s:%-3d %s", now, now - global_timer_last, file,
          str, line, msg);
  global_timer_last = now;
}

void rofi_timings_quit(void) {
  double now = g_timer_elapsed(global_timer, NULL);
  g_debug("%4.6f (%2.6f): Stopped", now, 0.0);
  g_timer_destroy(global_timer);
}
