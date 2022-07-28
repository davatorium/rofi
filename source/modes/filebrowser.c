/**
 * rofi-file_browser
 *
 * MIT/X11 License
 * Copyright (c) 2017 Qball Cow <qball@gmpclient.org>
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
 */
#include <errno.h>
#include <gio/gio.h>
#include <gmodule.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dirent.h>
#include <glib/gstdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "helper.h"
#include "history.h"
#include "mode-private.h"
#include "mode.h"
#include "modes/filebrowser.h"
#include "rofi.h"
#include "theme.h"

#include <stdint.h>

#include "rofi-icon-fetcher.h"

#define FILEBROWSER_CACHE_FILE "rofi3.filebrowsercache"

#if defined(__APPLE__)
#define st_atim st_atimespec
#define st_ctim st_ctimespec
#define st_mtim st_mtimespec
#endif

/**
 * The internal data structure holding the private data of the TEST Mode.
 */
enum FBFileType {
  UP,
  DIRECTORY,
  RFILE,
  NUM_FILE_TYPES,
};

/**
 * Possible sorting methods
 */
enum FBSortingMethod {
  FB_SORT_NAME,
  FB_SORT_TIME,
};

/**
 * Type of time to sort by
 */
enum FBSortingTime {
  FB_MTIME,
  FB_ATIME,
  FB_CTIME,
};

/** Icons to use for the file type */
const char *icon_name[NUM_FILE_TYPES] = {"go-up", "folder", "gtk-file"};
typedef struct {
  char *name;
  char *path;
  enum FBFileType type;
  uint32_t icon_fetch_uid;
  uint32_t icon_fetch_size;
  gboolean link;
  time_t time;
} FBFile;

typedef struct {
  GFile *current_dir;
  FBFile *array;
  unsigned int array_length;
  unsigned int array_length_real;
} FileBrowserModePrivateData;

/**
 * The sorting settings used in file-browser.
 */
struct {
  /** Field to sort on. */
  enum FBSortingMethod sorting_method;
  /** If sorting on time, what time entry. */
  enum FBSortingTime sorting_time;
  /** If we want to display directories above files. */
  gboolean directories_first;
} file_browser_config = {
    .sorting_method = FB_SORT_NAME,
    .sorting_time = FB_MTIME,
    .directories_first = TRUE,
};

static void free_list(FileBrowserModePrivateData *pd) {
  for (unsigned int i = 0; i < pd->array_length; i++) {
    FBFile *fb = &(pd->array[i]);
    g_free(fb->name);
    g_free(fb->path);
  }
  g_free(pd->array);
  pd->array = NULL;
  pd->array_length = 0;
  pd->array_length_real = 0;
}
#include <dirent.h>
#include <sys/types.h>

static gint compare_name(gconstpointer a, gconstpointer b,
                         G_GNUC_UNUSED gpointer data) {
  FBFile *fa = (FBFile *)a;
  FBFile *fb = (FBFile *)b;

  if (file_browser_config.directories_first && fa->type != fb->type) {
    return fa->type - fb->type;
  }

  return g_strcmp0(fa->name, fb->name);
}

static gint compare_time(gconstpointer a, gconstpointer b,
                         G_GNUC_UNUSED gpointer data) {
  FBFile *fa = (FBFile *)a;
  FBFile *fb = (FBFile *)b;

  if (file_browser_config.directories_first && fa->type != fb->type) {
    return fa->type - fb->type;
  }

  if (fa->time < 0) {
    return -1;
  }

  if (fb->time < 0) {
    return 1;
  }

  return fb->time - fa->time;
}

static gint compare(gconstpointer a, gconstpointer b, gpointer data) {
  GCompareDataFunc comparator = NULL;

  switch (file_browser_config.sorting_method) {
  case FB_SORT_NAME:
    comparator = compare_name;
    break;
  case FB_SORT_TIME:
    comparator = compare_time;
    break;
  default:
    comparator = compare_name;
    break;
  }

  return comparator(a, b, data);
}

static time_t get_time(const GStatBuf *statbuf) {
  switch (file_browser_config.sorting_time) {
  case FB_MTIME:
    return statbuf->st_mtim.tv_sec;
  case FB_ATIME:
    return statbuf->st_atim.tv_sec;
  case FB_CTIME:
    return statbuf->st_ctim.tv_sec;
  default:
    return 0;
  }
}

static void set_time(FBFile *file) {
  // GError *error = NULL;
  //  gchar *path = g_filename_from_utf8(file->path, -1, NULL, NULL, &error);
  //  if (error) {
  //    g_warning("Failed to convert filename: %s: %s", file->path,
  //    error->message); g_error_free(error); return;
  //  }

  GStatBuf statbuf;

  if (g_lstat(file->path, &statbuf) == 0) {
    file->time = get_time(&statbuf);
  } else {
    g_warning("Failed to stat file: %s, %s", file->path, strerror(errno));
  }

  //  g_free(path);
}

inline static void fb_resize_array(FileBrowserModePrivateData *pd) {
  if ((pd->array_length + 1) > pd->array_length_real) {
    pd->array_length_real += 256;
    pd->array =
        g_realloc(pd->array, (pd->array_length_real + 1) * sizeof(FBFile));
  }
}

static void get_file_browser(Mode *sw) {
  FileBrowserModePrivateData *pd =
      (FileBrowserModePrivateData *)mode_get_private_data(sw);
  /**
   * Get the entries to display.
   * this gets called on plugin initialization.
   */
  char *cdir = g_file_get_path(pd->current_dir);
  DIR *dir = opendir(cdir);
  if (dir) {
    struct dirent *rd = NULL;
    while ((rd = readdir(dir)) != NULL) {
      if (g_strcmp0(rd->d_name, "..") == 0) {
        fb_resize_array(pd);
        // Rofi expects utf-8, so lets convert the filename.
        pd->array[pd->array_length].name = g_strdup("..");
        pd->array[pd->array_length].path = NULL;
        pd->array[pd->array_length].type = UP;
        pd->array[pd->array_length].icon_fetch_uid = 0;
        pd->array[pd->array_length].icon_fetch_size = 0;
        pd->array[pd->array_length].link = FALSE;
        pd->array[pd->array_length].time = -1;
        pd->array_length++;
        continue;
      }
      if (rd->d_name[0] == '.') {
        continue;
      }

      switch (rd->d_type) {
      case DT_BLK:
      case DT_CHR:
      case DT_FIFO:
      case DT_UNKNOWN:
      case DT_SOCK:
      default:
        break;
      case DT_REG:
      case DT_DIR:
        fb_resize_array(pd);
        // Rofi expects utf-8, so lets convert the filename.
        pd->array[pd->array_length].name =
            g_filename_to_utf8(rd->d_name, -1, NULL, NULL, NULL);
        if (pd->array[pd->array_length].name == NULL) {
          pd->array[pd->array_length].name = rofi_force_utf8(rd->d_name, -1);
        }
        pd->array[pd->array_length].path =
            g_build_filename(cdir, rd->d_name, NULL);
        pd->array[pd->array_length].type =
            (rd->d_type == DT_DIR) ? DIRECTORY : RFILE;
        pd->array[pd->array_length].icon_fetch_uid = 0;
        pd->array[pd->array_length].icon_fetch_size = 0;
        pd->array[pd->array_length].link = FALSE;

        if (file_browser_config.sorting_method == FB_SORT_TIME) {
          set_time(&pd->array[pd->array_length]);
        }

        pd->array_length++;
        break;
      case DT_LNK:
        fb_resize_array(pd);
        // Rofi expects utf-8, so lets convert the filename.
        pd->array[pd->array_length].name =
            g_filename_to_utf8(rd->d_name, -1, NULL, NULL, NULL);
        if (pd->array[pd->array_length].name == NULL) {
          pd->array[pd->array_length].name = rofi_force_utf8(rd->d_name, -1);
        }
        pd->array[pd->array_length].path =
            g_build_filename(cdir, rd->d_name, NULL);
        pd->array[pd->array_length].icon_fetch_uid = 0;
        pd->array[pd->array_length].icon_fetch_size = 0;
        pd->array[pd->array_length].link = TRUE;
        // Default to file.
        pd->array[pd->array_length].type = RFILE;
        {
          // If we have link, use a stat to fine out what it is, if we fail, we
          // mark it as file.
          // TODO have a 'broken link' mode?
          // Convert full path to right encoding.
          // DD: Path should be in file encoding, not utf-8
          //          char *file =
          //          g_filename_from_utf8(pd->array[pd->array_length].path,
          //                                            -1, NULL, NULL, NULL);
          if (pd->array[pd->array_length].path) {
            GStatBuf statbuf;
            if (g_stat(pd->array[pd->array_length].path, &statbuf) == 0) {
              if (S_ISDIR(statbuf.st_mode)) {
                pd->array[pd->array_length].type = DIRECTORY;
              } else if (S_ISREG(statbuf.st_mode)) {
                pd->array[pd->array_length].type = RFILE;
              }

              if (file_browser_config.sorting_method == FB_SORT_TIME) {
                pd->array[pd->array_length].time = get_time(&statbuf);
              }
            } else {
              g_warning("Failed to stat file: %s, %s",
                        pd->array[pd->array_length].path, strerror(errno));
            }

            //            g_free(file);
          }
        }
        pd->array_length++;
        break;
      }
    }
    closedir(dir);
  }
  g_free(cdir);
  g_qsort_with_data(pd->array, pd->array_length, sizeof(FBFile), compare, NULL);
}

static void file_browser_mode_init_config(Mode *sw) {
  char *msg = NULL;
  gboolean found_error = FALSE;

  ThemeWidget *wid = rofi_config_find_widget(sw->name, NULL, TRUE);

  Property *p = rofi_theme_find_property(wid, P_STRING, "sorting-method", TRUE);
  if (p != NULL && p->type == P_STRING) {
    if (g_strcmp0(p->value.s, "name") == 0) {
      file_browser_config.sorting_method = FB_SORT_NAME;
    } else if (g_strcmp0(p->value.s, "mtime") == 0) {
      file_browser_config.sorting_method = FB_SORT_TIME;
      file_browser_config.sorting_time = FB_MTIME;
    } else if (g_strcmp0(p->value.s, "atime") == 0) {
      file_browser_config.sorting_method = FB_SORT_TIME;
      file_browser_config.sorting_time = FB_ATIME;
    } else if (g_strcmp0(p->value.s, "ctime") == 0) {
      file_browser_config.sorting_method = FB_SORT_TIME;
      file_browser_config.sorting_time = FB_CTIME;
    } else {
      found_error = TRUE;

      msg = g_strdup_printf("\"%s\" is not a valid filebrowser sorting method",
                            p->value.s);
    }
  }

  p = rofi_theme_find_property(wid, P_BOOLEAN, "directories-first", TRUE);
  if (p != NULL && p->type == P_BOOLEAN) {
    file_browser_config.directories_first = p->value.b;
  }

  if (found_error) {
    rofi_view_error_dialog(msg, FALSE);

    g_free(msg);
  }
}

static void file_browser_mode_init_current_dir(Mode *sw) {
  FileBrowserModePrivateData *pd =
      (FileBrowserModePrivateData *)mode_get_private_data(sw);

  ThemeWidget *wid = rofi_config_find_widget(sw->name, NULL, TRUE);

  Property *p = rofi_theme_find_property(wid, P_STRING, "directory", TRUE);

  gboolean config_has_valid_dir = p != NULL && p->type == P_STRING &&
                                  g_file_test(p->value.s, G_FILE_TEST_IS_DIR);

  if (config_has_valid_dir) {
    pd->current_dir = g_file_new_for_path(p->value.s);
  } else {
    char *current_dir = NULL;
    char *cache_file =
        g_build_filename(cache_dir, FILEBROWSER_CACHE_FILE, NULL);

    if (g_file_get_contents(cache_file, &current_dir, NULL, NULL)) {
      if (g_file_test(current_dir, G_FILE_TEST_IS_DIR)) {
        pd->current_dir = g_file_new_for_path(current_dir);
      }

      g_free(current_dir);
    }

    // Store it based on the unique identifiers (desktop_id).
    g_free(cache_file);
  }

  if (pd->current_dir == NULL) {
    pd->current_dir = g_file_new_for_path(g_get_home_dir());
  }
}

static int file_browser_mode_init(Mode *sw) {
  /**
   * Called on startup when enabled (in modes list)
   */
  if (mode_get_private_data(sw) == NULL) {
    FileBrowserModePrivateData *pd = g_malloc0(sizeof(*pd));
    mode_set_private_data(sw, (void *)pd);

    file_browser_mode_init_config(sw);
    file_browser_mode_init_current_dir(sw);

    // Load content.
    get_file_browser(sw);
  }
  return TRUE;
}
static unsigned int file_browser_mode_get_num_entries(const Mode *sw) {
  const FileBrowserModePrivateData *pd =
      (const FileBrowserModePrivateData *)mode_get_private_data(sw);
  return pd->array_length;
}

static ModeMode file_browser_mode_result(Mode *sw, int mretv, char **input,
                                         unsigned int selected_line) {
  ModeMode retv = MODE_EXIT;
  FileBrowserModePrivateData *pd =
      (FileBrowserModePrivateData *)mode_get_private_data(sw);

  gboolean special_command =
      ((mretv & MENU_CUSTOM_ACTION) == MENU_CUSTOM_ACTION);
  if (mretv & MENU_NEXT) {
    retv = NEXT_DIALOG;
  } else if (mretv & MENU_PREVIOUS) {
    retv = PREVIOUS_DIALOG;
  } else if (mretv & MENU_QUICK_SWITCH) {
    retv = (mretv & MENU_LOWER_MASK);
  } else if (mretv & MENU_CUSTOM_COMMAND) {
    retv = (mretv & MENU_LOWER_MASK);
  } else if ((mretv & MENU_OK)) {
    if (selected_line < pd->array_length) {
      if (pd->array[selected_line].type == UP) {
        GFile *new = g_file_get_parent(pd->current_dir);
        if (new) {
          g_object_unref(pd->current_dir);
          pd->current_dir = new;
          free_list(pd);
          get_file_browser(sw);
          return RESET_DIALOG;
        }
      } else if ((pd->array[selected_line].type == RFILE) ||
                 (pd->array[selected_line].type == DIRECTORY &&
                  special_command)) {
        char *d_esc = g_shell_quote(pd->array[selected_line].path);
        char *cmd = g_strdup_printf("xdg-open %s", d_esc);
        g_free(d_esc);
        char *cdir = g_file_get_path(pd->current_dir);
        helper_execute_command(cdir, cmd, FALSE, NULL);
        g_free(cdir);
        g_free(cmd);
        return MODE_EXIT;
      } else if (pd->array[selected_line].type == DIRECTORY) {
        char *path = g_build_filename(cache_dir, FILEBROWSER_CACHE_FILE, NULL);
        g_file_set_contents(path, pd->array[selected_line].path, -1, NULL);
        g_free(path);
        GFile *new = g_file_new_for_path(pd->array[selected_line].path);
        g_object_unref(pd->current_dir);
        pd->current_dir = new;
        free_list(pd);
        get_file_browser(sw);
        return RESET_DIALOG;
      }
    }
    retv = RELOAD_DIALOG;
  } else if ((mretv & MENU_CUSTOM_INPUT)) {
    if (special_command) {
      GFile *new = g_file_get_parent(pd->current_dir);
      if (new) {
        g_object_unref(pd->current_dir);
        pd->current_dir = new;
        free_list(pd);
        get_file_browser(sw);
      }
      return RESET_DIALOG;
    }
    if (*input) {
      char *p = rofi_expand_path(*input);
      char *dir = g_filename_from_utf8(p, -1, NULL, NULL, NULL);
      g_free(p);
      if (g_file_test(dir, G_FILE_TEST_EXISTS)) {
        if (g_file_test(dir, G_FILE_TEST_IS_DIR)) {
          g_object_unref(pd->current_dir);
          pd->current_dir = g_file_new_for_path(dir);
          g_free(dir);
          free_list(pd);
          get_file_browser(sw);
          return RESET_DIALOG;
        }
      }
      g_free(dir);
      retv = RELOAD_DIALOG;
    }
  } else if ((mretv & MENU_ENTRY_DELETE) == MENU_ENTRY_DELETE) {
    retv = RELOAD_DIALOG;
  }
  return retv;
}

static void file_browser_mode_destroy(Mode *sw) {
  FileBrowserModePrivateData *pd =
      (FileBrowserModePrivateData *)mode_get_private_data(sw);
  if (pd != NULL) {
    g_object_unref(pd->current_dir);
    free_list(pd);
    g_free(pd);
    mode_set_private_data(sw, NULL);
  }
}

static char *_get_display_value(const Mode *sw, unsigned int selected_line,
                                G_GNUC_UNUSED int *state,
                                G_GNUC_UNUSED GList **attr_list,
                                int get_entry) {
  FileBrowserModePrivateData *pd =
      (FileBrowserModePrivateData *)mode_get_private_data(sw);

  // Only return the string if requested, otherwise only set state.
  if (!get_entry) {
    return NULL;
  }
  if (pd->array[selected_line].type == UP) {
    return g_strdup(" ..");
  }
  if (pd->array[selected_line].link) {
    return g_strconcat("@", pd->array[selected_line].name, NULL);
  }
  return g_strdup(pd->array[selected_line].name);
}

/**
 * @param sw The mode object.
 * @param tokens The tokens to match against.
 * @param index  The index in this plugin to match against.
 *
 * Match the entry.
 *
 * @returns try when a match.
 */
static int file_browser_token_match(const Mode *sw, rofi_int_matcher **tokens,
                                    unsigned int index) {
  FileBrowserModePrivateData *pd =
      (FileBrowserModePrivateData *)mode_get_private_data(sw);

  // Call default matching function.
  return helper_token_match(tokens, pd->array[index].name);
}

static cairo_surface_t *_get_icon(const Mode *sw, unsigned int selected_line,
                                  unsigned int height) {
  FileBrowserModePrivateData *pd =
      (FileBrowserModePrivateData *)mode_get_private_data(sw);
  g_return_val_if_fail(pd->array != NULL, NULL);
  FBFile *dr = &(pd->array[selected_line]);
  if (dr->icon_fetch_uid > 0 && dr->icon_fetch_size == height) {
    return rofi_icon_fetcher_get(dr->icon_fetch_uid);
  }
  if (rofi_icon_fetcher_file_is_image(dr->path)) {
    dr->icon_fetch_uid = rofi_icon_fetcher_query(dr->path, height);
  } else {
    dr->icon_fetch_uid = rofi_icon_fetcher_query(icon_name[dr->type], height);
  }
  dr->icon_fetch_size = height;
  return rofi_icon_fetcher_get(dr->icon_fetch_uid);
}

static char *_get_message(const Mode *sw) {
  FileBrowserModePrivateData *pd =
      (FileBrowserModePrivateData *)mode_get_private_data(sw);
  if (pd->current_dir) {
    char *dirname = g_file_get_parse_name(pd->current_dir);
    char *str =
        g_markup_printf_escaped("<b>Current directory:</b> %s", dirname);
    g_free(dirname);
    return str;
  }
  return "n/a";
}

static char *_get_completion(const Mode *sw, unsigned int index) {
  FileBrowserModePrivateData *pd =
      (FileBrowserModePrivateData *)mode_get_private_data(sw);

  char *d = g_strescape(pd->array[index].path, NULL);
  return d;
}

Mode *create_new_file_browser(void) {
  Mode *sw = g_malloc0(sizeof(Mode));

  *sw = file_browser_mode;

  sw->private_data = NULL;
  return sw;
}

#if 1
ModeMode file_browser_mode_completer(Mode *sw, int mretv, char **input,
                                     unsigned int selected_line, char **path) {
  ModeMode retv = MODE_EXIT;
  FileBrowserModePrivateData *pd =
      (FileBrowserModePrivateData *)mode_get_private_data(sw);
  if (mretv & MENU_NEXT) {
    retv = NEXT_DIALOG;
  } else if (mretv & MENU_PREVIOUS) {
    retv = PREVIOUS_DIALOG;
  } else if (mretv & MENU_QUICK_SWITCH) {
    retv = (mretv & MENU_LOWER_MASK);
  } else if ((mretv & MENU_OK)) {
    if (selected_line < pd->array_length) {
      if (pd->array[selected_line].type == UP) {
        GFile *new = g_file_get_parent(pd->current_dir);
        if (new) {
          g_object_unref(pd->current_dir);
          pd->current_dir = new;
          free_list(pd);
          get_file_browser(sw);
          return RESET_DIALOG;
        }
      } else if (pd->array[selected_line].type == DIRECTORY) {
        GFile *new = g_file_new_for_path(pd->array[selected_line].path);
        g_object_unref(pd->current_dir);
        pd->current_dir = new;
        free_list(pd);
        get_file_browser(sw);
        return RESET_DIALOG;
      } else if (pd->array[selected_line].type == RFILE) {
        *path = g_strescape(pd->array[selected_line].path, NULL);
        return MODE_EXIT;
      }
    }
    retv = RELOAD_DIALOG;
  } else if ((mretv & MENU_CUSTOM_INPUT) && *input) {
    char *p = rofi_expand_path(*input);
    char *dir = g_filename_from_utf8(p, -1, NULL, NULL, NULL);
    g_free(p);
    if (g_file_test(dir, G_FILE_TEST_EXISTS)) {
      if (g_file_test(dir, G_FILE_TEST_IS_DIR)) {
        g_object_unref(pd->current_dir);
        pd->current_dir = g_file_new_for_path(dir);
        g_free(dir);
        free_list(pd);
        get_file_browser(sw);
        return RESET_DIALOG;
      }
    }
    g_free(dir);
    retv = RELOAD_DIALOG;
  } else if ((mretv & MENU_ENTRY_DELETE) == MENU_ENTRY_DELETE) {
    retv = RELOAD_DIALOG;
  }
  return retv;
}
#endif

Mode file_browser_mode = {
    .display_name = NULL,
    .abi_version = ABI_VERSION,
    .name = "filebrowser",
    .cfg_name_key = "display-filebrowser",
    ._init = file_browser_mode_init,
    ._get_num_entries = file_browser_mode_get_num_entries,
    ._result = file_browser_mode_result,
    ._destroy = file_browser_mode_destroy,
    ._token_match = file_browser_token_match,
    ._get_display_value = _get_display_value,
    ._get_icon = _get_icon,
    ._get_message = _get_message,
    ._get_completion = _get_completion,
    ._preprocess_input = NULL,
    .private_data = NULL,
    .free = NULL,
};
