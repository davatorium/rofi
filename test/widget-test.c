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

#include "display.h"
#include "rofi-icon-fetcher.h"
#include "rofi.h"
#include "xcb.h"
#include "xrmoptions.h"
#include <assert.h>
#include <glib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <widgets/widget-internal.h>
#include <widgets/widget.h>
unsigned int test = 0;
#define TASSERT(a)                                                             \
  {                                                                            \
    assert(a);                                                                 \
    printf("Test %3u passed (%s)\n", ++test, #a);                              \
  }

ThemeWidget *rofi_configuration = NULL;

uint32_t rofi_icon_fetcher_query(G_GNUC_UNUSED const char *name,
                                 G_GNUC_UNUSED const int size) {
  return 0;
}
uint32_t rofi_icon_fetcher_query_advanced(G_GNUC_UNUSED const char *name,
                                          G_GNUC_UNUSED const int wsize,
                                          G_GNUC_UNUSED const int hsize) {
  return 0;
}

cairo_surface_t *rofi_icon_fetcher_get(G_GNUC_UNUSED const uint32_t uid) {
  return NULL;
}

gboolean config_parse_set_property(G_GNUC_UNUSED const Property *p,
                                   G_GNUC_UNUSED char **error) {
  return FALSE;
}
void rofi_add_error_message(G_GNUC_UNUSED GString *msg) {}
void rofi_add_warning_message(G_GNUC_UNUSED GString *msg) {}
void rofi_view_queue_redraw(void) {}
int monitor_active(G_GNUC_UNUSED workarea *mon) { return 0; }
void rofi_view_get_current_monitor(G_GNUC_UNUSED int *width,
                                   G_GNUC_UNUSED int *height) {}
int rofi_view_error_dialog(const char *msg, G_GNUC_UNUSED int markup) {
  fputs(msg, stderr);
  return FALSE;
}

void display_startup_notification(
    G_GNUC_UNUSED RofiHelperExecuteContext *context,
    G_GNUC_UNUSED GSpawnChildSetupFunc *child_setup,
    G_GNUC_UNUSED gpointer *user_data) {}

int main(G_GNUC_UNUSED int argc, G_GNUC_UNUSED char **argv) {
  //    box 20 by 40
  widget *wid = (widget *)g_malloc0(sizeof(widget));
  widget_resize(wid, 20, 40);
  widget_move(wid, 10, 10);
  // Getter, setter x pos
  //
  TASSERT(widget_get_x_pos(wid) == 10);
  TASSERT(widget_get_y_pos(wid) == 10);

  // Left of box
  TASSERT(widget_intersect(wid, 0, 0) == 0);
  TASSERT(widget_intersect(wid, 0, 10) == 0);
  TASSERT(widget_intersect(wid, 0, 25) == 0);
  TASSERT(widget_intersect(wid, 0, 40) == 0);
  TASSERT(widget_intersect(wid, 0, 50) == 0);
  TASSERT(widget_intersect(wid, 9, 0) == 0);
  TASSERT(widget_intersect(wid, 9, 10) == 0);
  TASSERT(widget_intersect(wid, 9, 25) == 0);
  TASSERT(widget_intersect(wid, 9, 40) == 0);
  TASSERT(widget_intersect(wid, 9, 50) == 0);
  TASSERT(widget_intersect(wid, 10, 0) == 0);
  TASSERT(widget_intersect(wid, 10, 10) == 1);
  TASSERT(widget_intersect(wid, 10, 25) == 1);
  TASSERT(widget_intersect(wid, 10, 40) == 1);
  TASSERT(widget_intersect(wid, 10, 50) == 0);

  // Middle

  TASSERT(widget_intersect(wid, 25, 0) == 0);
  TASSERT(widget_intersect(wid, 25, 10) == 1);
  TASSERT(widget_intersect(wid, 25, 25) == 1);
  TASSERT(widget_intersect(wid, 25, 40) == 1);
  TASSERT(widget_intersect(wid, 25, 50) == 0);

  // Right
  TASSERT(widget_intersect(wid, 29, 0) == 0);
  TASSERT(widget_intersect(wid, 29, 10) == 1);
  TASSERT(widget_intersect(wid, 29, 25) == 1);
  TASSERT(widget_intersect(wid, 29, 40) == 1);
  TASSERT(widget_intersect(wid, 29, 50) == 0);

  TASSERT(widget_intersect(wid, 30, 0) == 0);
  TASSERT(widget_intersect(wid, 30, 10) == 0);
  TASSERT(widget_intersect(wid, 30, 25) == 0);
  TASSERT(widget_intersect(wid, 30, 40) == 0);
  TASSERT(widget_intersect(wid, 30, 50) == 0);

  widget_move(wid, 30, 30);
  // Left of box
  TASSERT(widget_intersect(wid, 10, 20) == 0);
  TASSERT(widget_intersect(wid, 10, 30) == 0);
  TASSERT(widget_intersect(wid, 10, 45) == 0);
  TASSERT(widget_intersect(wid, 10, 60) == 0);
  TASSERT(widget_intersect(wid, 10, 70) == 0);
  TASSERT(widget_intersect(wid, 19, 20) == 0);
  TASSERT(widget_intersect(wid, 19, 30) == 0);
  TASSERT(widget_intersect(wid, 19, 45) == 0);
  TASSERT(widget_intersect(wid, 19, 60) == 0);
  TASSERT(widget_intersect(wid, 19, 70) == 0);
  TASSERT(widget_intersect(wid, 30, 20) == 0);
  TASSERT(widget_intersect(wid, 30, 30) == 1);
  TASSERT(widget_intersect(wid, 30, 45) == 1);
  TASSERT(widget_intersect(wid, 30, 60) == 1);
  TASSERT(widget_intersect(wid, 30, 70) == 0);

  // Middle

  TASSERT(widget_intersect(wid, 20 + 25, 20 + 0) == 0);
  TASSERT(widget_intersect(wid, 20 + 25, 20 + 10) == 1);
  TASSERT(widget_intersect(wid, 20 + 25, 20 + 25) == 1);
  TASSERT(widget_intersect(wid, 20 + 25, 20 + 40) == 1);
  TASSERT(widget_intersect(wid, 20 + 25, 20 + 50) == 0);

  TASSERT(widget_intersect(wid, 20 + 29, 20 + 0) == 0);
  TASSERT(widget_intersect(wid, 20 + 29, 20 + 10) == 1);
  TASSERT(widget_intersect(wid, 20 + 29, 20 + 25) == 1);
  TASSERT(widget_intersect(wid, 20 + 29, 20 + 40) == 1);
  TASSERT(widget_intersect(wid, 20 + 29, 20 + 50) == 0);

  TASSERT(widget_intersect(wid, 20 + 30, 20 + 0) == 0);
  TASSERT(widget_intersect(wid, 20 + 30, 20 + 10) == 0);
  TASSERT(widget_intersect(wid, 20 + 30, 20 + 25) == 0);
  TASSERT(widget_intersect(wid, 20 + 30, 20 + 40) == 0);
  TASSERT(widget_intersect(wid, 20 + 30, 20 + 50) == 0);

  // Right
  TASSERT(widget_intersect(wid, 20 + 29, 20 + 0) == 0);
  TASSERT(widget_intersect(wid, 20 + 29, 20 + 10) == 1);
  TASSERT(widget_intersect(wid, 20 + 29, 20 + 25) == 1);
  TASSERT(widget_intersect(wid, 20 + 29, 20 + 40) == 1);
  TASSERT(widget_intersect(wid, 20 + 29, 20 + 50) == 0);

  TASSERT(widget_intersect(wid, 20 + 30, 20 + 0) == 0);
  TASSERT(widget_intersect(wid, 20 + 30, 20 + 10) == 0);
  TASSERT(widget_intersect(wid, 20 + 30, 20 + 25) == 0);
  TASSERT(widget_intersect(wid, 20 + 30, 20 + 40) == 0);
  TASSERT(widget_intersect(wid, 20 + 30, 20 + 50) == 0);

  TASSERT(widget_intersect(wid, -100, -100) == 0);
  TASSERT(widget_intersect(wid, INT_MIN, INT_MIN) == 0);
  TASSERT(widget_intersect(wid, INT_MAX, INT_MAX) == 0);

  // Other wrappers.
  TASSERT(widget_get_height(wid) == wid->h);
  TASSERT(widget_get_width(wid) == wid->w);

  TASSERT(widget_enabled(wid) == FALSE);
  widget_enable(wid);
  TASSERT(widget_enabled(wid) == TRUE);
  widget_disable(wid);
  TASSERT(widget_enabled(wid) == FALSE);
  // Null pointer tests.
  TASSERT(widget_intersect(NULL, 0, 0) == 0);
  widget_move(NULL, 0, 0);
  TASSERT(widget_get_height(NULL) == 0);
  TASSERT(widget_get_width(NULL) == 0);
  TASSERT(widget_enabled(NULL) == 0);
  widget_disable(NULL);
  widget_enable(NULL);
  widget_draw(NULL, NULL);
  widget_free(NULL);
  widget_resize(NULL, 0, 0);
  widget_update(NULL);
  widget_queue_redraw(NULL);
  TASSERT(widget_need_redraw(NULL) == FALSE);
  widget_trigger_action(NULL, 0, 0, 0);
  widget_set_trigger_action_handler(NULL, NULL, NULL);

  g_free(wid);
}
