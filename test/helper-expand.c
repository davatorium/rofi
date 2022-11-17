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
#include "display.h"
#include "rofi-icon-fetcher.h"
#include "rofi.h"
#include "settings.h"
#include "theme.h"
#include "widgets/textbox.h"
#include "xcb-internal.h"
#include "xcb.h"
#include <assert.h>
#include <glib.h>
#include <helper.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <xcb/xcb_ewmh.h>

static int test = 0;

#define TASSERT(a)                                                             \
  {                                                                            \
    assert(a);                                                                 \
    printf("Test %i passed (%s)\n", ++test, #a);                               \
  }
#define TASSERTE(a, b)                                                         \
  {                                                                            \
    if ((a) == (b)) {                                                          \
      printf("Test %i passed (%s == %s) (%u == %u)\n", ++test, #a, #b, a, b);  \
    } else {                                                                   \
      printf("Test %i failed (%s == %s) (%u != %u)\n", ++test, #a, #b, a, b);  \
      abort();                                                                 \
    }                                                                          \
  }
ThemeWidget *rofi_theme = NULL;
void rofi_clear_error_messages(void) {}
void rofi_clear_warning_messages(void) {}
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

double textbox_get_estimated_char_height(void) { return 12.0; }
void rofi_view_get_current_monitor(int *width, int *height) {
  *width = 1920;
  *height = 1080;
}
double textbox_get_estimated_ch(void) { return 9.0; }
gboolean rofi_theme_parse_string(G_GNUC_UNUSED const char *string) { return 0; }

void rofi_add_error_message(G_GNUC_UNUSED GString *msg) {}
void rofi_add_warning_message(G_GNUC_UNUSED GString *msg) {}

int rofi_view_error_dialog(const char *msg, G_GNUC_UNUSED int markup) {
  fputs(msg, stderr);
  return TRUE;
}

int monitor_active(G_GNUC_UNUSED workarea *mon) { return 0; }

void display_startup_notification(
    G_GNUC_UNUSED RofiHelperExecuteContext *context,
    G_GNUC_UNUSED GSpawnChildSetupFunc *child_setup,
    G_GNUC_UNUSED gpointer *user_data) {}

int main(int argc, char **argv) {
  cmd_set_arguments(argc, argv);

  if (setlocale(LC_ALL, "") == NULL) {
    fprintf(stderr, "Failed to set locale.\n");
    return EXIT_FAILURE;
  }

  /**
   * Test some path functions. Not easy as not sure what is right output on
   * travis.
   */
  // Test if root is preserved.
  char *str = rofi_expand_path("/");
  TASSERT(strcmp(str, "/") == 0);
  g_free(str);
  // Test is relative path is preserved.
  str = rofi_expand_path("../AUTHORS");
  TASSERT(strcmp(str, "../AUTHORS") == 0);
  g_free(str);
  // Test another one.
  str = rofi_expand_path("/bin/false");
  TASSERT(strcmp(str, "/bin/false") == 0);
  g_free(str);
  // See if user paths get expanded in full path.
  str = rofi_expand_path("~/");
  const char *hd = g_get_home_dir();
  TASSERT(strcmp(str, hd) == 0);
  g_free(str);
  str = rofi_expand_path("~root/");
  TASSERT(str[0] == '/');
  g_free(str);
}
