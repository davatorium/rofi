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
#define TASSERTL(a, b)                                                         \
  {                                                                            \
    if ((a) == (b)) {                                                          \
      printf("Test %i passed (%s == %s) (%d == %d)\n", ++test, #a, #b, a, b);  \
    } else {                                                                   \
      printf("Test %i failed (%s == %s) (%d != %d)\n", ++test, #a, #b, a, b);  \
      abort();                                                                 \
    }                                                                          \
  }

#include "widgets/textbox.h"

ThemeWidget *rofi_theme = NULL;

gboolean rofi_theme_parse_string(G_GNUC_UNUSED const char *string) {
  return FALSE;
}

uint32_t rofi_icon_fetcher_query(G_GNUC_UNUSED const char *name,
                                 G_GNUC_UNUSED const int size) {
  return 0;
}
void rofi_clear_error_messages(void) {}
void rofi_clear_warning_messages(void) {}
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
   * Char function
   */

  TASSERT(helper_parse_char("\\n") == '\n');
  TASSERT(helper_parse_char("\\a") == '\a');
  TASSERT(helper_parse_char("\\b") == '\b');
  TASSERT(helper_parse_char("\\t") == '\t');
  TASSERT(helper_parse_char("\\v") == '\v');
  TASSERT(helper_parse_char("\\f") == '\f');
  TASSERT(helper_parse_char("\\r") == '\r');
  TASSERT(helper_parse_char("\\\\") == '\\');
  TASSERT(helper_parse_char("\\0") == 0);
  TASSERT(helper_parse_char("\\x77") == 'w');
  TASSERT(helper_parse_char("\\x0A") == '\n');

  /**
   * tokenize
   */

  TASSERT(levenshtein("aap", g_utf8_strlen("aap", -1), "aap",
                      g_utf8_strlen("aap", -1), 0) == 0);
  TASSERT(levenshtein("aap", g_utf8_strlen("aap", -1), "aap ",
                      g_utf8_strlen("aap ", -1), 0) == 1);
  TASSERT(levenshtein("aap ", g_utf8_strlen("aap ", -1), "aap",
                      g_utf8_strlen("aap", -1), 0) == 1);
  TASSERTE(levenshtein("aap", g_utf8_strlen("aap", -1), "aap noot",
                       g_utf8_strlen("aap noot", -1), 0),
           5u);
  TASSERTE(levenshtein("aap", g_utf8_strlen("aap", -1), "noot aap",
                       g_utf8_strlen("noot aap", -1), 0),
           5u);
  TASSERTE(levenshtein("aap", g_utf8_strlen("aap", -1), "noot aap mies",
                       g_utf8_strlen("noot aap mies", -1), 0),
           10u);
  TASSERTE(levenshtein("noot aap mies", g_utf8_strlen("noot aap mies", -1),
                       "aap", g_utf8_strlen("aap", -1), 0),
           10u);
  TASSERTE(levenshtein("otp", g_utf8_strlen("otp", -1), "noot aap",
                       g_utf8_strlen("noot aap", -1), 0),
           5u);
  /**
   * Quick converision check.
   */
  {
    char *str = rofi_latin_to_utf8_strdup("\xA1\xB5", 2);
    TASSERT(g_utf8_collate(str, "¡µ") == 0);
    g_free(str);
  }

  {
    char *str = rofi_force_utf8("Valid utf8", 10);
    TASSERT(g_utf8_collate(str, "Valid utf8") == 0);
    g_free(str);
    char in[] = "Valid utf8 until \xc3\x28 we continue here";
    TASSERT(g_utf8_validate(in, -1, NULL) == FALSE);
    str = rofi_force_utf8(in, strlen(in));
    TASSERT(g_utf8_validate(str, -1, NULL) == TRUE);
    TASSERT(g_utf8_collate(str, "Valid utf8 until �( we continue here") == 0);
    g_free(str);
  }
  {
    TASSERT(utf8_strncmp("aapno", "aap€", 3) == 0);
    TASSERT(utf8_strncmp("aapno", "aap€", 4) != 0);
    TASSERT(utf8_strncmp("aapno", "a", 4) != 0);
    TASSERT(utf8_strncmp("a", "aap€", 4) != 0);
    //        char in[] = "Valid utf8 until \xc3\x28 we continue here";
    //        TASSERT ( utf8_strncmp ( in, "Valid", 3 ) == 0);
  }
  {
    TASSERTL(
        rofi_scorer_fuzzy_evaluate("aap noot mies", 12, "aap noot mies", 12, 0),
        -605);
    TASSERTL(rofi_scorer_fuzzy_evaluate("anm", 3, "aap noot mies", 12, 0),
             -155);
    TASSERTL(rofi_scorer_fuzzy_evaluate("blu", 3, "aap noot mies", 12, 0),
             1073741824);
    TASSERTL(rofi_scorer_fuzzy_evaluate("Anm", 3, "aap noot mies", 12, 1),
             1073741754);
    TASSERTL(rofi_scorer_fuzzy_evaluate("Anm", 3, "aap noot mies", 12, 0),
             -155);
    TASSERTL(rofi_scorer_fuzzy_evaluate("aap noot mies", 12, "Anm", 3, 0),
             1073741824);
  }

  /**
   * Case sensitivity
   */
  {
    int case_smart = config.case_smart;
    int case_sensitive = config.case_sensitive;
    {
      config.case_smart = FALSE;
      config.case_sensitive = FALSE;
      TASSERT(parse_case_sensitivity("all lower case 你好") == 0);
      TASSERT(parse_case_sensitivity("not All lowEr Case 你好") == 0);
      config.case_sensitive = TRUE;
      TASSERT(parse_case_sensitivity("all lower case 你好") == 1);
      TASSERT(parse_case_sensitivity("not All lowEr Case 你好") == 1);
    }
    {
      config.case_smart = TRUE;
      config.case_sensitive = TRUE;
      TASSERT(parse_case_sensitivity("all lower case") == 0);
      TASSERT(parse_case_sensitivity("AAAAAAAAAAAA") == 1);
      config.case_sensitive = FALSE;
      TASSERT(parse_case_sensitivity("all lower case 你好") == 0);
      TASSERT(parse_case_sensitivity("not All lowEr Case 你好") == 1);
    }
    config.case_smart = case_smart;
    config.case_sensitive = case_sensitive;
  }

  char *a;
  a = helper_string_replace_if_exists(
      "{terminal} [-t {title} blub ]-e {cmd}", "{cmd}", "aap", "{title}",
      "some title", "{terminal}", "rofi-sensible-terminal", NULL);
  printf("%s\n", a);
  TASSERT(g_utf8_collate(
              a, "rofi-sensible-terminal -t some title blub -e aap") == 0);
  g_free(a);
  a = helper_string_replace_if_exists("{terminal} [-t {title} blub ]-e {cmd}",
                                      "{cmd}", "aap", "{terminal}",
                                      "rofi-sensible-terminal", NULL);
  printf("%s\n", a);
  TASSERT(g_utf8_collate(a, "rofi-sensible-terminal -e aap") == 0);
  g_free(a);
  a = helper_string_replace_if_exists(
      "{name} [<span weight='light' size='small'><i>({category})</i></span>]",
      "{name}", "Librecad", "{category}", "Desktop app", "{terminal}",
      "rofi-sensible-terminal", NULL);
  printf("%s\n", a);
  TASSERT(g_utf8_collate(a, "Librecad <span weight='light' "
                            "size='small'><i>(Desktop app)</i></span>") == 0);
  g_free(a);
  a = helper_string_replace_if_exists(
      "{name}[ <span weight='light' size='small'><i>({category})</i></span>]",
      "{name}", "Librecad", "{terminal}", "rofi-sensible-terminal", NULL);
  TASSERT(g_utf8_collate(a, "Librecad") == 0);
  g_free(a);
  a = helper_string_replace_if_exists(
      "{terminal} [{title} blub ]-e {cmd}", "{cmd}", "aap", "{title}",
      "some title", "{terminal}", "rofi-sensible-terminal", NULL);
  printf("%s\n", a);
  TASSERT(g_utf8_collate(a, "rofi-sensible-terminal some title blub -e aap") ==
          0);
  g_free(a);
  a = helper_string_replace_if_exists(
      "{terminal} [{title} blub ]-e {cmd}", "{cmd}", "aap", "{title}", NULL,
      "{terminal}", "rofi-sensible-terminal", NULL);
  printf("%s\n", a);
  TASSERT(g_utf8_collate(a, "rofi-sensible-terminal -e aap") == 0);
  g_free(a);
}
