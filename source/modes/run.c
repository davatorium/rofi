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

/**
 * \ingroup RUNMode
 * @{
 */

/** The log domain of this dialog. */
#define G_LOG_DOMAIN "Modes.Run"

#include <config.h>
#include <stdio.h>
#include <stdlib.h>

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>

#include "helper.h"
#include "history.h"
#include "modes/filebrowser.h"
#include "modes/run.h"
#include "rofi.h"
#include "settings.h"

#include "mode-private.h"

#include "rofi-icon-fetcher.h"
#include "timings.h"
/**
 * Name of the history file where previously chosen commands are stored.
 */
#define RUN_CACHE_FILE "rofi-3.runcache"

typedef struct {
  char *entry;
  uint32_t icon_fetch_uid;
  uint32_t icon_fetch_size;
  /* Surface holding the icon. */
  cairo_surface_t *icon;
} RunEntry;

/**
 * The internal data structure holding the private data of the Run Mode.
 */
typedef struct {
  /** list of available commands. */
  RunEntry *cmd_list;
  /** Length of the #cmd_list. */
  unsigned int cmd_list_length;

  /** Current mode. */
  gboolean file_complete;
  uint32_t selected_line;
  char *old_input;

  Mode *completer;
  char *old_completer_input;
} RunModePrivateData;

/**
 * @param cmd The cmd to execute
 * @param run_in_term Indicate if command should be run in a terminal
 *
 * Execute command and add to history.
 */
static gboolean exec_cmd(const char *cmd, int run_in_term) {
  GError *error = NULL;
  if (!cmd || !cmd[0]) {
    return FALSE;
  }
  gsize lf_cmd_size = 0;
  gchar *lf_cmd = g_locale_from_utf8(cmd, -1, NULL, &lf_cmd_size, &error);
  if (error != NULL) {
    g_warning("Failed to convert command to locale encoding: %s",
              error->message);
    g_error_free(error);
    return FALSE;
  }

  char *path = g_build_filename(cache_dir, RUN_CACHE_FILE, NULL);
  RofiHelperExecuteContext context = {.name = NULL};
  // FIXME: assume startup notification support for terminals
  if (helper_execute_command(NULL, lf_cmd, run_in_term,
                             run_in_term ? &context : NULL)) {
    /**
     * This happens in non-critical time (After launching app)
     * It is allowed to be a bit slower.
     */

    history_set(path, cmd);
    g_free(path);
    g_free(lf_cmd);
    return TRUE;
  }
  history_remove(path, cmd);
  g_free(path);
  g_free(lf_cmd);
  return FALSE;
}

/**
 * @param cmd The command to remove from history
 *
 * Remove command from history.
 */
static void delete_entry(const RunEntry *cmd) {
  char *path = g_build_filename(cache_dir, RUN_CACHE_FILE, NULL);

  history_remove(path, cmd->entry);

  g_free(path);
}

/**
 * @param a The First key to compare
 * @param b The second key to compare
 * @param data Unused.
 *
 * Function used for sorting.
 *
 * @returns returns less then, equal to and greater than zero is a is less than,
 * is a match or greater than b.
 */
static int sort_func(const void *a, const void *b, G_GNUC_UNUSED void *data) {
  const RunEntry *astr = (const RunEntry *)a;
  const RunEntry *bstr = (const RunEntry *)b;

  if (astr->entry == NULL && bstr->entry == NULL) {
    return 0;
  }
  if (astr->entry == NULL) {
    return 1;
  }
  if (bstr->entry == NULL) {
    return -1;
  }
  return g_strcmp0(astr->entry, bstr->entry);
}

/**
 * External spider to get list of executables.
 */
static RunEntry *get_apps_external(RunEntry *retv, unsigned int *length,
                                   unsigned int num_favorites) {
  int fd = execute_generator(config.run_list_command);
  if (fd >= 0) {
    FILE *inp = fdopen(fd, "r");
    if (inp) {
      char *buffer = NULL;
      size_t buffer_length = 0;

      while (getline(&buffer, &buffer_length, inp) > 0) {
        int found = 0;
        // Filter out line-end.
        if (buffer[strlen(buffer) - 1] == '\n') {
          buffer[strlen(buffer) - 1] = '\0';
        }

        // This is a nice little penalty, but doable? time will tell.
        // given num_favorites is max 25.
        for (unsigned int j = 0; found == 0 && j < num_favorites; j++) {
          if (strcasecmp(buffer, retv[j].entry) == 0) {
            found = 1;
          }
        }

        if (found == 1) {
          continue;
        }

        // No duplicate, add it.
        retv = g_realloc(retv, ((*length) + 2) * sizeof(RunEntry));
        retv[(*length)].entry = g_strdup(buffer);
        retv[(*length)].icon = NULL;
        retv[(*length)].icon_fetch_uid = 0;
        retv[(*length)].icon_fetch_size = 0;

        (*length)++;
      }
      if (buffer != NULL) {
        free(buffer);
      }
      if (fclose(inp) != 0) {
        g_warning("Failed to close stdout off executor script: '%s'",
                  g_strerror(errno));
      }
    }
  }
  retv[(*length)].entry = NULL;
  retv[(*length)].icon = NULL;
  retv[(*length)].icon_fetch_uid = 0;
  retv[(*length)].icon_fetch_size = 0;
  return retv;
}

/**
 * Internal spider used to get list of executables.
 */
static RunEntry *get_apps(unsigned int *length) {
  GError *error = NULL;
  RunEntry *retv = NULL;
  unsigned int num_favorites = 0;
  char *path;

  if (g_getenv("PATH") == NULL) {
    return NULL;
  }
  TICK_N("start");
  path = g_build_filename(cache_dir, RUN_CACHE_FILE, NULL);
  char **hretv = history_get_list(path, length);
  retv = (RunEntry *)g_malloc0((*length + 1) * sizeof(RunEntry));
  for (unsigned int i = 0; i < *length; i++) {
    retv[i].entry = hretv[i];
  }
  g_free(hretv);
  g_free(path);
  // Keep track of how many where loaded as favorite.
  num_favorites = (*length);

  path = g_strdup(g_getenv("PATH"));

  gsize l = 0;
  gchar *homedir = g_locale_to_utf8(g_get_home_dir(), -1, NULL, &l, &error);
  if (error != NULL) {
    g_debug("Failed to convert homedir to UTF-8: %s", error->message);
    for (unsigned int i = 0; retv[i].entry != NULL; i++) {
      g_free(retv[i].entry);
    }
    g_free(retv);
    g_clear_error(&error);
    g_free(homedir);
    return NULL;
  }

  const char *const sep = ":";
  char *strtok_savepointer = NULL;
  for (const char *dirname = strtok_r(path, sep, &strtok_savepointer);
       dirname != NULL; dirname = strtok_r(NULL, sep, &strtok_savepointer)) {
    char *fpath = rofi_expand_path(dirname);
    DIR *dir = opendir(fpath);
    g_debug("Checking path %s for executable.", fpath);
    g_free(fpath);

    if (dir != NULL) {
      struct dirent *dent;
      gsize dirn_len = 0;
      gchar *dirn = g_locale_to_utf8(dirname, -1, NULL, &dirn_len, &error);
      if (error != NULL) {
        g_debug("Failed to convert directory name to UTF-8: %s",
                error->message);
        g_clear_error(&error);
        closedir(dir);
        continue;
      }
      gboolean is_homedir = g_str_has_prefix(dirn, homedir);
      g_free(dirn);

      while ((dent = readdir(dir)) != NULL) {
        if (dent->d_type != DT_REG && dent->d_type != DT_LNK &&
            dent->d_type != DT_UNKNOWN) {
          continue;
        }
        // Skip dot files.
        if (dent->d_name[0] == '.') {
          continue;
        }
        if (is_homedir) {
          gchar *full_path = g_build_filename(dirname, dent->d_name, NULL);
          gboolean b = g_file_test(full_path, G_FILE_TEST_IS_EXECUTABLE);
          g_free(full_path);
          if (!b) {
            continue;
          }
        }

        gsize name_len;
        gchar *name =
            g_filename_to_utf8(dent->d_name, -1, NULL, &name_len, &error);
        if (error != NULL) {
          g_debug("Failed to convert filename to UTF-8: %s", error->message);
          g_clear_error(&error);
          g_free(name);
          continue;
        }
        // This is a nice little penalty, but doable? time will tell.
        // given num_favorites is max 25.
        int found = 0;
        for (unsigned int j = 0; found == 0 && j < num_favorites; j++) {
          if (g_strcmp0(name, retv[j].entry) == 0) {
            found = 1;
          }
        }

        if (found == 1) {
          g_free(name);
          continue;
        }

        retv = g_realloc(retv, ((*length) + 2) * sizeof(RunEntry));
        retv[(*length)].entry = name;
        retv[(*length)].icon = NULL;
        retv[(*length)].icon_fetch_uid = 0;
        retv[(*length)].icon_fetch_size = 0;
        retv[(*length) + 1].entry = NULL;
        retv[(*length) + 1].icon = NULL;
        retv[(*length) + 1].icon_fetch_uid = 0;
        retv[(*length) + 1].icon_fetch_size = 0;
        (*length)++;
      }

      closedir(dir);
    }
  }
  g_free(homedir);

  // Get external apps.
  if (config.run_list_command != NULL && config.run_list_command[0] != '\0') {
    retv = get_apps_external(retv, length, num_favorites);
  }
  // No sorting needed.
  if ((*length) == 0) {
    return retv;
  }
  // TODO: check this is still fast enough. (takes 1ms on laptop.)
  if ((*length) > num_favorites) {
    g_qsort_with_data(&(retv[num_favorites]), (*length) - num_favorites,
                      sizeof(RunEntry), sort_func, NULL);
  }
  g_free(path);

  unsigned int removed = 0;
  for (unsigned int index = num_favorites; index < ((*length) - 1); index++) {
    if (g_strcmp0(retv[index].entry, retv[index + 1].entry) == 0) {
      g_free(retv[index].entry);
      retv[index].entry = NULL;
      removed++;
    }
  }

  if ((*length) > num_favorites) {
    g_qsort_with_data(&(retv[num_favorites]), (*length) - num_favorites,
                      sizeof(RunEntry), sort_func, NULL);
  }
  // Reduce array length;
  (*length) -= removed;

  TICK_N("stop");
  return retv;
}

static int run_mode_init(Mode *sw) {
  if (sw->private_data == NULL) {
    RunModePrivateData *pd = g_malloc0(sizeof(*pd));
    sw->private_data = (void *)pd;
    pd->cmd_list = get_apps(&(pd->cmd_list_length));
    pd->completer = NULL;
  }

  return TRUE;
}
static void run_mode_destroy(Mode *sw) {
  RunModePrivateData *rmpd = (RunModePrivateData *)sw->private_data;
  if (rmpd != NULL) {
    for (unsigned int i = 0; i < rmpd->cmd_list_length; i++) {
      g_free(rmpd->cmd_list[i].entry);
      if (rmpd->cmd_list[i].icon != NULL) {
        cairo_surface_destroy(rmpd->cmd_list[i].icon);
      }
    }
    g_free(rmpd->cmd_list);
    g_free(rmpd->old_input);
    g_free(rmpd->old_completer_input);
    if (rmpd->completer != NULL) {
      mode_destroy(rmpd->completer);
      g_free(rmpd->completer);
    }
    g_free(rmpd);
    sw->private_data = NULL;
  }
}

static unsigned int run_mode_get_num_entries(const Mode *sw) {
  const RunModePrivateData *rmpd = (const RunModePrivateData *)sw->private_data;
  if (rmpd->file_complete) {
    return rmpd->completer->_get_num_entries(rmpd->completer);
  }
  return rmpd->cmd_list_length;
}

static ModeMode run_mode_result(Mode *sw, int mretv, char **input,
                                unsigned int selected_line) {
  RunModePrivateData *rmpd = (RunModePrivateData *)sw->private_data;
  ModeMode retv = MODE_EXIT;

  gboolean run_in_term = ((mretv & MENU_CUSTOM_ACTION) == MENU_CUSTOM_ACTION);
  if (rmpd->file_complete == TRUE) {

    retv = RELOAD_DIALOG;

    if ((mretv & (MENU_COMPLETE))) {
      g_free(rmpd->old_completer_input);
      rmpd->old_completer_input = *input;
      *input = NULL;
      if (rmpd->selected_line < rmpd->cmd_list_length) {
        (*input) = g_strdup(rmpd->old_input);
      }
      rmpd->file_complete = FALSE;
    } else if ((mretv & MENU_CANCEL)) {
      retv = MODE_EXIT;
    } else {
      char *path = NULL;
      retv = file_browser_mode_completer(rmpd->completer, mretv, input,
                                         selected_line, &path);
      if (retv == MODE_EXIT) {
        if (path == NULL) {
          exec_cmd(rmpd->cmd_list[rmpd->selected_line].entry, run_in_term);
        } else {
          char *arg = g_strdup_printf(
              "%s '%s'", rmpd->cmd_list[rmpd->selected_line].entry, path);
          exec_cmd(arg, run_in_term);
          g_free(arg);
        }
      }
      g_free(path);
    }
    return retv;
  }

  if ((mretv & MENU_OK) && rmpd->cmd_list[selected_line].entry != NULL) {
    if (!exec_cmd(rmpd->cmd_list[selected_line].entry, run_in_term)) {
      retv = RELOAD_DIALOG;
    }
  } else if ((mretv & MENU_CUSTOM_INPUT) && *input != NULL &&
             *input[0] != '\0') {
    if (!exec_cmd(*input, run_in_term)) {
      retv = RELOAD_DIALOG;
    }
  } else if ((mretv & MENU_ENTRY_DELETE) &&
             rmpd->cmd_list[selected_line].entry) {
    delete_entry(&(rmpd->cmd_list[selected_line]));

    // Clear the list.
    retv = RELOAD_DIALOG;
    run_mode_destroy(sw);
    run_mode_init(sw);
  } else if (mretv & MENU_CUSTOM_COMMAND) {
    retv = (mretv & MENU_LOWER_MASK);
  } else if ((mretv & MENU_COMPLETE)) {
    retv = RELOAD_DIALOG;
    if (selected_line < rmpd->cmd_list_length) {
      rmpd->selected_line = selected_line;

      g_free(rmpd->old_input);
      rmpd->old_input = g_strdup(*input);

      if (*input)
        g_free(*input);
      *input = g_strdup(rmpd->old_completer_input);

      rmpd->completer = create_new_file_browser();
      mode_init(rmpd->completer);
      rmpd->file_complete = TRUE;
    }
  }
  return retv;
}

static char *_get_display_value(const Mode *sw, unsigned int selected_line,
                                G_GNUC_UNUSED int *state,
                                G_GNUC_UNUSED GList **list, int get_entry) {
  const RunModePrivateData *rmpd = (const RunModePrivateData *)sw->private_data;
  if (rmpd->file_complete) {
    return rmpd->completer->_get_display_value(rmpd->completer, selected_line,
                                               state, list, get_entry);
  }
  return get_entry ? g_strdup(rmpd->cmd_list[selected_line].entry) : NULL;
}

static int run_token_match(const Mode *sw, rofi_int_matcher **tokens,
                           unsigned int index) {
  const RunModePrivateData *rmpd = (const RunModePrivateData *)sw->private_data;
  if (rmpd->file_complete) {
    return rmpd->completer->_token_match(rmpd->completer, tokens, index);
  }
  return helper_token_match(tokens, rmpd->cmd_list[index].entry);
}
static char *run_get_message(const Mode *sw) {
  RunModePrivateData *pd = sw->private_data;
  if (pd->file_complete) {
    if (pd->selected_line < pd->cmd_list_length) {
      char *msg = mode_get_message(pd->completer);
      if (msg) {
        char *retv =
            g_strdup_printf("File complete for: %s\n%s",
                            pd->cmd_list[pd->selected_line].entry, msg);
        g_free(msg);
        return retv;
      }
      return g_strdup_printf("File complete for: %s",
                             pd->cmd_list[pd->selected_line].entry);
    }
  }
  return NULL;
}
static cairo_surface_t *_get_icon(const Mode *sw, unsigned int selected_line,
                                  unsigned int height) {
  RunModePrivateData *pd = (RunModePrivateData *)mode_get_private_data(sw);
  if (pd->file_complete) {
    return pd->completer->_get_icon(pd->completer, selected_line, height);
  }
  g_return_val_if_fail(pd->cmd_list != NULL, NULL);
  RunEntry *dr = &(pd->cmd_list[selected_line]);

  if (dr->icon_fetch_uid > 0 && dr->icon_fetch_size == height) {
    cairo_surface_t *icon = rofi_icon_fetcher_get(dr->icon_fetch_uid);
    return icon;
  }
  /** lookup icon */
  char **str = g_strsplit(dr->entry, " ", 2);
  if (str) {
    dr->icon_fetch_uid = rofi_icon_fetcher_query(str[0], height);
    dr->icon_fetch_size = height;
    g_strfreev(str);
    cairo_surface_t *icon = rofi_icon_fetcher_get(dr->icon_fetch_uid);
    return icon;
  }
  return NULL;
}

#include "mode-private.h"
Mode run_mode = {.name = "run",
                 .cfg_name_key = "display-run",
                 ._init = run_mode_init,
                 ._get_num_entries = run_mode_get_num_entries,
                 ._result = run_mode_result,
                 ._destroy = run_mode_destroy,
                 ._token_match = run_token_match,
                 ._get_message = run_get_message,
                 ._get_display_value = _get_display_value,
                 ._get_icon = _get_icon,
                 ._get_completion = NULL,
                 ._preprocess_input = NULL,
                 .private_data = NULL,
                 .free = NULL};
/** @}*/
