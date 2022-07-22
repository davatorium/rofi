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

/** The log domain of this dialog. */
#define G_LOG_DOMAIN "Modes.Combi"

#include "helper.h"
#include "settings.h"
#include <rofi.h>
#include <stdio.h>
#include <stdlib.h>

#include "mode-private.h"
#include "widgets/textbox.h"
#include <modes/modes.h>
#include <pango/pango.h>
#include <theme.h>

/**
 * Combi Mode
 */
typedef struct {
  Mode *mode;
  gboolean disable;
} CombiMode;

typedef struct {
  // List of (combined) entries.
  unsigned int cmd_list_length;
  // List to validate where each switcher starts.
  unsigned int *starts;
  unsigned int *lengths;
  // List of switchers to combine.
  unsigned int num_switchers;
  CombiMode *switchers;
} CombiModePrivateData;

static void combi_mode_parse_switchers(Mode *sw) {
  CombiModePrivateData *pd = mode_get_private_data(sw);
  char *savept = NULL;
  // Make a copy, as strtok will modify it.
  char *switcher_str = g_strdup(config.combi_modes);
  const char *const sep = ",#";
  // Split token on ','. This modifies switcher_str.
  for (char *token = strtok_r(switcher_str, sep, &savept); token != NULL;
       token = strtok_r(NULL, sep, &savept)) {
    /* Check against recursion. */
    if (g_strcmp0(token, sw->name) == 0) {
      g_warning("You cannot add '%s' to the list of combined modes.", sw->name);
      continue;
    }
    // Resize and add entry.
    pd->switchers = (CombiMode *)g_realloc(
        pd->switchers, sizeof(CombiMode) * (pd->num_switchers + 1));

    Mode *mode = rofi_collect_modes_search(token);
    if (mode != NULL) {
      pd->switchers[pd->num_switchers].disable = FALSE;
      pd->switchers[pd->num_switchers++].mode = mode;
      continue;
    }
    // If not build in, use custom switchers.
    mode = script_mode_parse_setup(token);
    if (mode != NULL) {
      pd->switchers[pd->num_switchers].disable = FALSE;
      pd->switchers[pd->num_switchers++].mode = mode;
      continue;
    }
    // Report error, don't continue.
    g_warning("Invalid script switcher: %s", token);
    token = NULL;
  }
  // Free string that was modified by strtok_r
  g_free(switcher_str);
}
static unsigned int combi_mode_get_num_entries(const Mode *sw) {
  const CombiModePrivateData *pd =
      (const CombiModePrivateData *)mode_get_private_data(sw);
  unsigned int length = 0;
  for (unsigned int i = 0; i < pd->num_switchers; i++) {
    unsigned int entries = mode_get_num_entries(pd->switchers[i].mode);
    pd->starts[i] = length;
    pd->lengths[i] = entries;
    length += entries;
  }
  return length;
}

static int combi_mode_init(Mode *sw) {
  if (mode_get_private_data(sw) == NULL) {
    CombiModePrivateData *pd = g_malloc0(sizeof(*pd));
    mode_set_private_data(sw, (void *)pd);
    combi_mode_parse_switchers(sw);
    pd->starts = g_malloc0(sizeof(int) * pd->num_switchers);
    pd->lengths = g_malloc0(sizeof(int) * pd->num_switchers);
    for (unsigned int i = 0; i < pd->num_switchers; i++) {
      if (!mode_init(pd->switchers[i].mode)) {
        return FALSE;
      }
    }
    if (pd->cmd_list_length == 0) {
      pd->cmd_list_length = combi_mode_get_num_entries(sw);
    }
  }
  return TRUE;
}
static void combi_mode_destroy(Mode *sw) {
  CombiModePrivateData *pd = (CombiModePrivateData *)mode_get_private_data(sw);
  if (pd != NULL) {
    g_free(pd->starts);
    g_free(pd->lengths);
    // Cleanup switchers.
    for (unsigned int i = 0; i < pd->num_switchers; i++) {
      mode_destroy(pd->switchers[i].mode);
    }
    g_free(pd->switchers);
    g_free(pd);
    mode_set_private_data(sw, NULL);
  }
}
static ModeMode combi_mode_result(Mode *sw, int mretv, char **input,
                                  unsigned int selected_line) {
  CombiModePrivateData *pd = mode_get_private_data(sw);

  if (input[0][0] == '!') {
    int switcher = -1;
    // Implement strchrnul behaviour.
    char *eob = g_utf8_strchr(input[0], -1, ' ');
    if (eob == NULL) {
      eob = &(input[0][strlen(input[0])]);
    }
    ssize_t bang_len = g_utf8_pointer_to_offset(input[0], eob) - 1;
    if (bang_len > 0) {
      for (unsigned i = 0; i < pd->num_switchers; i++) {
        const char *mode_name = mode_get_name(pd->switchers[i].mode);
        size_t mode_name_len = g_utf8_strlen(mode_name, -1);
        if ((size_t)bang_len <= mode_name_len &&
            utf8_strncmp(&input[0][1], mode_name, bang_len) == 0) {
          switcher = i;
          break;
        }
      }
    }
    if (switcher >= 0) {
      if (eob[0] == ' ') {
        char *n = eob + 1;
        return mode_result(pd->switchers[switcher].mode, mretv, &n,
                           selected_line - pd->starts[switcher]);
      }
      return MODE_EXIT;
    }
  } else if ((mretv & MENU_COMPLETE)) {
    return RELOAD_DIALOG;
  }

  for (unsigned i = 0; i < pd->num_switchers; i++) {
    if (selected_line >= pd->starts[i] &&
        selected_line < (pd->starts[i] + pd->lengths[i])) {
      return mode_result(pd->switchers[i].mode, mretv, input,
                         selected_line - pd->starts[i]);
    }
  }
  if ((mretv & MENU_CUSTOM_INPUT)) {
    return mode_result(pd->switchers[0].mode, mretv, input, selected_line);
  }
  return MODE_EXIT;
}
static int combi_mode_match(const Mode *sw, rofi_int_matcher **tokens,
                            unsigned int index) {
  CombiModePrivateData *pd = mode_get_private_data(sw);
  for (unsigned i = 0; i < pd->num_switchers; i++) {
    if (pd->switchers[i].disable) {
      continue;
    }
    if (index >= pd->starts[i] && index < (pd->starts[i] + pd->lengths[i])) {
      return mode_token_match(pd->switchers[i].mode, tokens,
                              index - pd->starts[i]);
    }
  }
  return 0;
}
static char *combi_mgrv(const Mode *sw, unsigned int selected_line, int *state,
                        GList **attr_list, int get_entry) {
  CombiModePrivateData *pd = mode_get_private_data(sw);
  if (!get_entry) {
    for (unsigned i = 0; i < pd->num_switchers; i++) {
      if (selected_line >= pd->starts[i] &&
          selected_line < (pd->starts[i] + pd->lengths[i])) {
        mode_get_display_value(pd->switchers[i].mode,
                               selected_line - pd->starts[i], state, attr_list,
                               FALSE);
        return NULL;
      }
    }
    return NULL;
  }
  for (unsigned i = 0; i < pd->num_switchers; i++) {
    if (selected_line >= pd->starts[i] &&
        selected_line < (pd->starts[i] + pd->lengths[i])) {
      char *retv;
      char *str = retv = mode_get_display_value(pd->switchers[i].mode,
                                                selected_line - pd->starts[i],
                                                state, attr_list, TRUE);
      const char *dname = mode_get_display_name(pd->switchers[i].mode);

      if (!config.combi_hide_mode_prefix) {
        if (!(*state & MARKUP)) {
          char *tmp = str;
          str = g_markup_escape_text(tmp, -1);
          g_free(tmp);
          *state |= MARKUP;
        }

        retv = helper_string_replace_if_exists(
            config.combi_display_format, "{mode}", dname, "{text}", str, NULL);
        g_free(str);

        if (attr_list != NULL) {
          ThemeWidget *wid = rofi_config_find_widget(sw->name, NULL, TRUE);
          Property *p = rofi_theme_find_property(
              wid, P_COLOR, pd->switchers[i].mode->name, TRUE);
          if (p != NULL) {
            PangoAttribute *pa = pango_attr_foreground_new(
                p->value.color.red * 65535, p->value.color.green * 65535,
                p->value.color.blue * 65535);
            pa->start_index = PANGO_ATTR_INDEX_FROM_TEXT_BEGINNING;
            pa->end_index = strlen(dname);
            *attr_list = g_list_append(*attr_list, pa);
          }
        }
      }
      return retv;
    }
  }

  return NULL;
}
static char *combi_get_completion(const Mode *sw, unsigned int index) {
  CombiModePrivateData *pd = mode_get_private_data(sw);
  for (unsigned i = 0; i < pd->num_switchers; i++) {
    if (index >= pd->starts[i] && index < (pd->starts[i] + pd->lengths[i])) {
      char *comp =
          mode_get_completion(pd->switchers[i].mode, index - pd->starts[i]);
      char *mcomp =
          g_strdup_printf("!%s %s", mode_get_name(pd->switchers[i].mode), comp);
      g_free(comp);
      return mcomp;
    }
  }
  // Should never get here.
  g_assert_not_reached();
  return NULL;
}

static cairo_surface_t *combi_get_icon(const Mode *sw, unsigned int index,
                                       unsigned int height) {
  CombiModePrivateData *pd = mode_get_private_data(sw);
  for (unsigned i = 0; i < pd->num_switchers; i++) {
    if (index >= pd->starts[i] && index < (pd->starts[i] + pd->lengths[i])) {
      cairo_surface_t *icon =
          mode_get_icon(pd->switchers[i].mode, index - pd->starts[i], height);
      return icon;
    }
  }
  return NULL;
}

static char *combi_preprocess_input(Mode *sw, const char *input) {
  CombiModePrivateData *pd = mode_get_private_data(sw);
  for (unsigned i = 0; i < pd->num_switchers; i++) {
    pd->switchers[i].disable = FALSE;
  }
  if (input != NULL && input[0] == '!') {
    // Implement strchrnul behaviour.
    const char *eob = g_utf8_strchr(input, -1, ' ');
    if (eob == NULL) {
      // Set it to end.
      eob = &(input[strlen(input)]);
    }
    ssize_t bang_len = g_utf8_pointer_to_offset(input, eob) - 1;
    if (bang_len > 0) {
      for (unsigned i = 0; i < pd->num_switchers; i++) {
        const char *mode_name = mode_get_name(pd->switchers[i].mode);
        size_t mode_name_len = g_utf8_strlen(mode_name, -1);
        if (!((size_t)bang_len <= mode_name_len &&
              utf8_strncmp(&input[1], mode_name, bang_len) == 0)) {
          // No match.
          pd->switchers[i].disable = TRUE;
        }
      }
      if (eob[0] == '\0' || eob[1] == '\0') {
        return NULL;
      }
      return g_strdup(eob + 1);
    }
  }
  return g_strdup(input);
}

Mode combi_mode = {.name = "combi",
                   .cfg_name_key = "display-combi",
                   ._init = combi_mode_init,
                   ._get_num_entries = combi_mode_get_num_entries,
                   ._result = combi_mode_result,
                   ._destroy = combi_mode_destroy,
                   ._token_match = combi_mode_match,
                   ._get_completion = combi_get_completion,
                   ._get_display_value = combi_mgrv,
                   ._get_icon = combi_get_icon,
                   ._preprocess_input = combi_preprocess_input,
                   .private_data = NULL,
                   .free = NULL};
