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

#include "config.h"
#include <stdlib.h>
#include <unistd.h>

#include "display.h"
#include "settings.h"
#include "xcb.h"
#include "xrmoptions.h"
#include <assert.h>
#include <cairo-xlib.h>
#include <glib.h>
#include <history.h>
#include <rofi.h>
#include <stdio.h>
#include <string.h>
#include <widgets/textbox.h>
#include <xcb/xcb.h>

#include "rofi-icon-fetcher.h"
static int test = 0;
unsigned int normal_window_mode = 0;
int rofi_is_in_dmenu_mode = 0;

#define TASSERT(a)                                                             \
  {                                                                            \
    assert(a);                                                                 \
    printf("Test %3i passed (%s)\n", ++test, #a);                              \
  }

#include "view.h"

ThemeWidget *rofi_configuration = NULL;

void rofi_timings_tick(G_GNUC_UNUSED const char *file,
                       G_GNUC_UNUSED char const *str, G_GNUC_UNUSED int line,
                       G_GNUC_UNUSED char const *msg);
void rofi_timings_tick(G_GNUC_UNUSED const char *file,
                       G_GNUC_UNUSED char const *str, G_GNUC_UNUSED int line,
                       G_GNUC_UNUSED char const *msg) {}
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
void rofi_view_get_current_monitor(G_GNUC_UNUSED int *width,
                                   G_GNUC_UNUSED int *height) {}
int rofi_view_error_dialog(const char *msg, G_GNUC_UNUSED int markup) {
  fputs(msg, stderr);
  return FALSE;
}

int monitor_active(G_GNUC_UNUSED workarea *mon) { return 0; }

void display_startup_notification(
    G_GNUC_UNUSED RofiHelperExecuteContext *context,
    G_GNUC_UNUSED GSpawnChildSetupFunc *child_setup,
    G_GNUC_UNUSED gpointer *user_data) {}

int main(G_GNUC_UNUSED int argc, G_GNUC_UNUSED char **argv) {
  cairo_surface_t *surf =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 100, 100);
  cairo_t *draw = cairo_create(surf);
  PangoContext *p = pango_cairo_create_context(draw);

  textbox_setup();
  textbox_set_pango_context("default", p);

  textbox *box = textbox_create(NULL, WIDGET_TYPE_TEXTBOX_TEXT, "textbox",
                                TB_EDITABLE | TB_AUTOWIDTH | TB_AUTOHEIGHT,
                                NORMAL, "test", 0, 0);
  TASSERT(box != NULL);

  textbox_keybinding(box, MOVE_END);
  TASSERT(box->cursor == 4);
  textbox_cursor(box, -1);
  TASSERT(box->cursor == 0);
  textbox_cursor(box, 8);
  TASSERT(box->cursor == 4);
  textbox_cursor(box, 2);
  TASSERT(box->cursor == 2);
  textbox_insert(box, 3, "bo", 2);
  TASSERT(strcmp(box->text, "tesbot") == 0);
  textbox_keybinding(box, MOVE_END);
  TASSERT(box->cursor == 6);

  TASSERT(widget_get_width(WIDGET(box)) > 0);
  TASSERT(textbox_get_height(box) > 0);

  TASSERT(widget_get_width(WIDGET(box)) >= textbox_get_font_width(box));
  TASSERT(textbox_get_height(box) >= textbox_get_font_height(box));

  TASSERT(textbox_get_estimated_char_width() > 0);

  textbox_keybinding(box, REMOVE_CHAR_BACK);
  TASSERT(strcmp(box->text, "tesbo") == 0);
  TASSERT(box->cursor == 5);

  textbox_keybinding(box, MOVE_CHAR_BACK);
  TASSERT(box->cursor == 4);
  textbox_keybinding(box, REMOVE_CHAR_FORWARD);
  TASSERT(strcmp(box->text, "tesb") == 0);
  textbox_keybinding(box, MOVE_CHAR_BACK);
  TASSERT(box->cursor == 3);
  textbox_keybinding(box, MOVE_CHAR_FORWARD);
  TASSERT(box->cursor == 4);
  textbox_keybinding(box, MOVE_CHAR_FORWARD);
  TASSERT(box->cursor == 4);
  // Cursor after delete section.
  textbox_delete(box, 0, 1);
  TASSERT(strcmp(box->text, "esb") == 0);
  TASSERT(box->cursor == 3);
  // Cursor before delete.
  textbox_text(box, "aap noot mies");
  TASSERT(strcmp(box->text, "aap noot mies") == 0);
  textbox_cursor(box, 3);
  TASSERT(box->cursor == 3);
  textbox_delete(box, 3, 6);
  TASSERT(strcmp(box->text, "aapmies") == 0);
  TASSERT(box->cursor == 3);

  // Cursor within delete
  textbox_text(box, "aap noot mies");
  TASSERT(strcmp(box->text, "aap noot mies") == 0);
  textbox_cursor(box, 5);
  TASSERT(box->cursor == 5);
  textbox_delete(box, 3, 6);
  TASSERT(strcmp(box->text, "aapmies") == 0);
  TASSERT(box->cursor == 3);
  // Cursor after delete.
  textbox_text(box, "aap noot mies");
  TASSERT(strcmp(box->text, "aap noot mies") == 0);
  textbox_cursor(box, 11);
  TASSERT(box->cursor == 11);
  textbox_delete(box, 3, 6);
  TASSERT(strcmp(box->text, "aapmies") == 0);
  TASSERT(box->cursor == 5);

  textbox_text(box, "aap noot mies");
  textbox_cursor(box, 8);
  textbox_keybinding(box, REMOVE_WORD_BACK);
  TASSERT(box->cursor == 4);
  TASSERT(strcmp(box->text, "aap  mies") == 0);
  textbox_keybinding(box, REMOVE_TO_EOL);
  TASSERT(box->cursor == 4);
  TASSERT(strcmp(box->text, "aap ") == 0);
  textbox_text(box, "aap noot mies");
  textbox_cursor(box, 8);
  textbox_keybinding(box, REMOVE_WORD_FORWARD);
  TASSERT(strcmp(box->text, "aap noot") == 0);
  textbox_keybinding(box, MOVE_FRONT);
  TASSERT(box->cursor == 0);
  textbox_keybinding(box, CLEAR_LINE);
  TASSERT(strcmp(box->text, "") == 0);
  textbox_text(box, "aap noot mies");
  textbox_keybinding(box, MOVE_END);
  textbox_keybinding(box, MOVE_WORD_BACK);
  TASSERT(box->cursor == 9);
  textbox_keybinding(box, MOVE_WORD_BACK);
  TASSERT(box->cursor == 4);
  textbox_keybinding(box, REMOVE_TO_SOL);
  TASSERT(strcmp(box->text, "noot mies") == 0);
  TASSERT(box->cursor == 0);

  textbox_font(box, HIGHLIGHT);
  // textbox_draw ( box, draw );

  widget_move(WIDGET(box), 12, 13);
  TASSERT(box->widget.x == 12);
  TASSERT(box->widget.y == 13);

  widget_free(WIDGET(box));
  textbox_cleanup();
}
