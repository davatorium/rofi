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

#include <assert.h>
#include <glib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "display.h"
#include "rofi.h"
#include "theme.h"
#include "widgets/textbox.h"
#include "xcb.h"
#include <helper.h>
#include <keyb.h>
#include <mode-private.h>
#include <mode.h>
#include <modes/help-keys.h>
#include <xkbcommon/xkbcommon.h>

#include "rofi-icon-fetcher.h"
#include <check.h>

ThemeWidget *rofi_theme = NULL;

uint32_t rofi_icon_fetcher_query(G_GNUC_UNUSED const char *name,
                                 G_GNUC_UNUSED const int size) {
  return 0;
}
uint32_t rofi_icon_fetcher_query_advanced(G_GNUC_UNUSED const char *name,
                                          G_GNUC_UNUSED const int wsize,
                                          G_GNUC_UNUSED const int hsize) {
  return 0;
}
void rofi_clear_error_messages(void) {}
void rofi_clear_warning_messages(void) {}
cairo_surface_t *rofi_icon_fetcher_get(G_GNUC_UNUSED const uint32_t uid) {
  return NULL;
}

gboolean rofi_theme_parse_string(G_GNUC_UNUSED const char *string) {
  return FALSE;
}

double textbox_get_estimated_char_height(void) { return 16.0; }
double textbox_get_estimated_ch(void) { return 9.0; }
void rofi_add_error_message(G_GNUC_UNUSED GString *msg) {}
void rofi_add_warning_message(G_GNUC_UNUSED GString *msg) {}
int monitor_active(G_GNUC_UNUSED workarea *d) { return 0; }
int rofi_view_error_dialog(const char *msg, G_GNUC_UNUSED int markup) {
  fputs(msg, stderr);
  return TRUE;
}
void rofi_view_get_current_monitor(G_GNUC_UNUSED int *width,
                                   G_GNUC_UNUSED int *height) {}
RofiViewState *rofi_view_get_active(void) { return NULL; }
gboolean rofi_view_check_action(G_GNUC_UNUSED RofiViewState *state,
                                G_GNUC_UNUSED BindingsScope scope,
                                G_GNUC_UNUSED guint action) {
  return FALSE;
}
void rofi_view_trigger_action(G_GNUC_UNUSED RofiViewState *state,
                              G_GNUC_UNUSED BindingsScope scope,
                              G_GNUC_UNUSED guint action) {}

void display_startup_notification(
    G_GNUC_UNUSED RofiHelperExecuteContext *context,
    G_GNUC_UNUSED GSpawnChildSetupFunc *child_setup,
    G_GNUC_UNUSED gpointer *user_data) {}

#ifndef _ck_assert_ptr_null
/* Pointer against NULL comparison macros with improved output
 * compared to ck_assert(). */
/* OP may only be == or !=  */
#define _ck_assert_ptr_null(X, OP)                                             \
  do {                                                                         \
    const void *_ck_x = (X);                                                   \
    ck_assert_msg(_ck_x OP NULL, "Assertion '%s' failed: %s == %#x",           \
                  #X " " #OP " NULL", #X, _ck_x);                              \
  } while (0)

#define ck_assert_ptr_null(X) _ck_assert_ptr_null(X, ==)
#define ck_assert_ptr_nonnull(X) _ck_assert_ptr_null(X, !=)
#endif
static void test_mode_setup(void) {
  ck_assert_int_eq(mode_init(&help_keys_mode), TRUE);
}
static void test_mode_teardown(void) { mode_destroy(&help_keys_mode); }

START_TEST(test_mode_create) {
  ck_assert_ptr_nonnull(help_keys_mode.private_data);
}
END_TEST

START_TEST(test_mode_destroy) {
  mode_destroy(&help_keys_mode);
  ck_assert_ptr_null(help_keys_mode.private_data);
}
END_TEST

START_TEST(test_mode_num_items) {
  unsigned int rows = mode_get_num_entries(&help_keys_mode);
  ck_assert_int_eq(rows, 81);
  for (unsigned int i = 0; i < rows; i++) {
    int state = 0;
    GList *list = NULL;
    char *v = mode_get_display_value(&help_keys_mode, i, &state, &list, TRUE);
    ck_assert_ptr_nonnull(v);
    g_free(v);
    v = mode_get_display_value(&help_keys_mode, i, &state, &list, FALSE);
    ck_assert_ptr_null(v);
  }
  mode_destroy(&help_keys_mode);
}
END_TEST

START_TEST(test_mode_result) {
  char *res;

  res = NULL;
  ck_assert_int_eq(mode_result(&help_keys_mode, MENU_NEXT, &res, 0),
                   NEXT_DIALOG);
  g_free(res);

  res = NULL;
  ck_assert_int_eq(mode_result(&help_keys_mode, MENU_PREVIOUS, &res, 0),
                   PREVIOUS_DIALOG);
  g_free(res);

  res = NULL;
  ck_assert_int_eq(mode_result(&help_keys_mode, MENU_QUICK_SWITCH | 1, &res, 0),
                   1);
  g_free(res);

  res = NULL;
  ck_assert_int_eq(mode_result(&help_keys_mode, MENU_QUICK_SWITCH | 2, &res, 0),
                   2);
  g_free(res);
}
END_TEST

START_TEST(test_mode_match_entry) {
  rofi_int_matcher **t = helper_tokenize("primary-paste", FALSE);
  ck_assert_ptr_nonnull(t);

  ck_assert_int_eq(mode_token_match(&help_keys_mode, t, 0), TRUE);
  ck_assert_int_eq(mode_token_match(&help_keys_mode, t, 1), FALSE);
  helper_tokenize_free(t);
  t = helper_tokenize("y-paste", FALSE);
  ck_assert_ptr_nonnull(t);

  ck_assert_int_eq(mode_token_match(&help_keys_mode, t, 0), TRUE);
  ck_assert_int_eq(mode_token_match(&help_keys_mode, t, 1), TRUE);
  ck_assert_int_eq(mode_token_match(&help_keys_mode, t, 2), FALSE);
  helper_tokenize_free(t);
}
END_TEST

static Suite *mode_suite(void) {
  Suite *s;
  TCase *tc_core;

  s = suite_create("Mode");

  /* Core test case */
  tc_core = tcase_create("HelpKeys");
  tcase_add_checked_fixture(tc_core, test_mode_setup, test_mode_teardown);
  tcase_add_test(tc_core, test_mode_create);
  tcase_add_test(tc_core, test_mode_num_items);
  tcase_add_test(tc_core, test_mode_result);
  tcase_add_test(tc_core, test_mode_destroy);
  tcase_add_test(tc_core, test_mode_match_entry);
  suite_add_tcase(s, tc_core);

  return s;
}

int main(G_GNUC_UNUSED int argc, G_GNUC_UNUSED char **argv) {
  setup_abe();
  int number_failed = 0;
  Suite *s;
  SRunner *sr;

  s = mode_suite();
  sr = srunner_create(s);

  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
