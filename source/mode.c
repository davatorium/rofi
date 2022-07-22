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

#include "mode.h"
#include "rofi.h"
#include "xrmoptions.h"
#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "rofi-icon-fetcher.h"
// This one should only be in mode implementations.
#include "mode-private.h"
/**
 * @ingroup MODE
 * @{
 */

int mode_init(Mode *mode) {
  g_return_val_if_fail(mode != NULL, FALSE);
  g_return_val_if_fail(mode->_init != NULL, FALSE);
  // to make sure this is initialized correctly.
  mode->fallback_icon_fetch_uid = 0;
  mode->fallback_icon_not_found = FALSE;
  return mode->_init(mode);
}

void mode_destroy(Mode *mode) {
  g_assert(mode != NULL);
  g_assert(mode->_destroy != NULL);
  mode->_destroy(mode);
}

unsigned int mode_get_num_entries(const Mode *mode) {
  g_assert(mode != NULL);
  g_assert(mode->_get_num_entries != NULL);
  return mode->_get_num_entries(mode);
}

char *mode_get_display_value(const Mode *mode, unsigned int selected_line,
                             int *state, GList **attribute_list,
                             int get_entry) {
  g_assert(mode != NULL);
  g_assert(state != NULL);
  g_assert(mode->_get_display_value != NULL);

  return mode->_get_display_value(mode, selected_line, state, attribute_list,
                                  get_entry);
}

cairo_surface_t *mode_get_icon(Mode *mode, unsigned int selected_line,
                               unsigned int height) {
  g_assert(mode != NULL);

  if (mode->_get_icon != NULL) {
    cairo_surface_t *icon = mode->_get_icon(mode, selected_line, height);
    if (icon) {
      return icon;
    }
  }

  if (mode->fallback_icon_not_found == TRUE) {
    return NULL;
  }
  if (mode->fallback_icon_fetch_uid > 0) {
    cairo_surface_t *icon =
        rofi_icon_fetcher_get(mode->fallback_icon_fetch_uid);
    return icon;
  }
  ThemeWidget *wid = rofi_config_find_widget(mode->name, NULL, TRUE);
  if (wid) {
    /** Load user entires */
    Property *p =
        rofi_theme_find_property(wid, P_STRING, "fallback-icon", TRUE);
    if (p != NULL && (p->type == P_STRING && p->value.s)) {
      mode->fallback_icon_fetch_uid =
          rofi_icon_fetcher_query(p->value.s, height);
      return NULL;
    }
  }
  mode->fallback_icon_not_found = TRUE;
  return NULL;
}

char *mode_get_completion(const Mode *mode, unsigned int selected_line) {
  g_assert(mode != NULL);
  if (mode->_get_completion != NULL) {
    return mode->_get_completion(mode, selected_line);
  }
  int state;
  g_assert(mode->_get_display_value != NULL);
  return mode->_get_display_value(mode, selected_line, &state, NULL, TRUE);
}

ModeMode mode_result(Mode *mode, int menu_retv, char **input,
                     unsigned int selected_line) {
  if (menu_retv & MENU_NEXT) {
    return NEXT_DIALOG;
  }
  if (menu_retv & MENU_PREVIOUS) {
    return PREVIOUS_DIALOG;
  }
  if (menu_retv & MENU_QUICK_SWITCH) {
    return menu_retv & MENU_LOWER_MASK;
  }

  g_assert(mode != NULL);
  g_assert(mode->_result != NULL);
  g_assert(input != NULL);

  return mode->_result(mode, menu_retv, input, selected_line);
}

int mode_token_match(const Mode *mode, rofi_int_matcher **tokens,
                     unsigned int selected_line) {
  g_assert(mode != NULL);
  g_assert(mode->_token_match != NULL);
  return mode->_token_match(mode, tokens, selected_line);
}

const char *mode_get_name(const Mode *mode) {
  g_assert(mode != NULL);
  return mode->name;
}

void mode_free(Mode **mode) {
  g_assert(mode != NULL);
  g_assert((*mode) != NULL);
  if ((*mode)->free != NULL) {
    (*mode)->free(*mode);
  }
  (*mode) = NULL;
}

void *mode_get_private_data(const Mode *mode) {
  g_assert(mode != NULL);
  return mode->private_data;
}

void mode_set_private_data(Mode *mode, void *pd) {
  g_assert(mode != NULL);
  if (pd != NULL) {
    g_assert(mode->private_data == NULL);
  }
  mode->private_data = pd;
}

const char *mode_get_display_name(const Mode *mode) {
  /** Find the widget */
  ThemeWidget *wid = rofi_config_find_widget(mode->name, NULL, TRUE);
  if (wid) {
    /** Check string property */
    Property *p = rofi_theme_find_property(wid, P_STRING, "display-name", TRUE);
    if (p != NULL && p->type == P_STRING) {
      return p->value.s;
    }
  }
  if (mode->display_name != NULL) {
    return mode->display_name;
  }
  return mode->name;
}

void mode_set_config(Mode *mode) {
  snprintf(mode->cfg_name_key, 128, "display-%s", mode->name);
  config_parser_add_option(xrm_String, mode->cfg_name_key,
                           (void **)&(mode->display_name),
                           "The display name of this browser");
}

char *mode_preprocess_input(Mode *mode, const char *input) {
  if (mode->_preprocess_input) {
    return mode->_preprocess_input(mode, input);
  }
  return g_strdup(input);
}
char *mode_get_message(const Mode *mode) {
  if (mode->_get_message) {
    return mode->_get_message(mode);
  }
  return NULL;
}
/**@}*/
