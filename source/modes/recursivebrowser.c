/**
 * rofi-recursive_browser
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
#include "config.h"
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
#include "modes/recursivebrowser.h"
#include "rofi.h"
#include "theme.h"

#include <stdint.h>

#include "rofi-icon-fetcher.h"

#define FILEBROWSER_CACHE_FILE "rofi3.recursivebrowsercache"
#define DEFAULT_OPEN "xdg-open"

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
const char *rb_icon_name[NUM_FILE_TYPES] = {"go-up", "folder", "gtk-file"};
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
  char *command;
  GFile *current_dir;
  FBFile *array;
  unsigned int array_length;
  unsigned int array_length_real;

  GThread *reading_thread;
  GAsyncQueue *async_queue;
  guint wake_source;
  guint end_thread;
  gboolean loading;
  int pipefd2[2];
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
  /** If we want to show hidden files. */
  gboolean show_hidden;
} recursive_browser_config = {
    .sorting_method = FB_SORT_NAME,
    .sorting_time = FB_MTIME,
    .directories_first = TRUE,
    .show_hidden = FALSE,
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

inline static void fb_resize_array(FileBrowserModePrivateData *pd) {
  if ((pd->array_length + 1) > pd->array_length_real) {
    pd->array_length_real += 10240;
    pd->array =
        g_realloc(pd->array, (pd->array_length_real + 1) * sizeof(FBFile));
  }
}

static void recursive_browser_mode_init_config(Mode *sw) {
  FileBrowserModePrivateData *pd =
      (FileBrowserModePrivateData *)mode_get_private_data(sw);
  char *msg = NULL;
  gboolean found_error = FALSE;

  ThemeWidget *wid = rofi_config_find_widget(sw->name, NULL, TRUE);

  Property *p = rofi_theme_find_property(wid, P_STRING, "sorting-method", TRUE);
  if (p != NULL && p->type == P_STRING) {
    if (g_strcmp0(p->value.s, "name") == 0) {
      recursive_browser_config.sorting_method = FB_SORT_NAME;
    } else if (g_strcmp0(p->value.s, "mtime") == 0) {
      recursive_browser_config.sorting_method = FB_SORT_TIME;
      recursive_browser_config.sorting_time = FB_MTIME;
    } else if (g_strcmp0(p->value.s, "atime") == 0) {
      recursive_browser_config.sorting_method = FB_SORT_TIME;
      recursive_browser_config.sorting_time = FB_ATIME;
    } else if (g_strcmp0(p->value.s, "ctime") == 0) {
      recursive_browser_config.sorting_method = FB_SORT_TIME;
      recursive_browser_config.sorting_time = FB_CTIME;
    } else {
      found_error = TRUE;

      msg = g_strdup_printf(
          "\"%s\" is not a valid recursivebrowser sorting method", p->value.s);
    }
  }

  p = rofi_theme_find_property(wid, P_BOOLEAN, "directories-first", TRUE);
  if (p != NULL && p->type == P_BOOLEAN) {
    recursive_browser_config.directories_first = p->value.b;
  }

  p = rofi_theme_find_property(wid, P_BOOLEAN, "show-hidden", TRUE);
  if (p != NULL && p->type == P_BOOLEAN) {
    recursive_browser_config.show_hidden = p->value.b;
  }

  p = rofi_theme_find_property(wid, P_STRING, "command", TRUE);
  if (p != NULL && p->type == P_STRING) {
    pd->command = g_strdup(p->value.s);
  } else {
    pd->command = g_strdup(DEFAULT_OPEN);
  }

  if (found_error) {
    rofi_view_error_dialog(msg, FALSE);

    g_free(msg);
  }
}

static void recursive_browser_mode_init_current_dir(Mode *sw) {
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

static void scan_dir(FileBrowserModePrivateData *pd, GFile *path) {
  char *cdir = g_file_get_path(path);
  DIR *dir = opendir(cdir);
  if (dir) {
    struct dirent *rd = NULL;
    while (pd->end_thread == FALSE && (rd = readdir(dir)) != NULL) {
      if (g_strcmp0(rd->d_name, "..") == 0) {
        continue;
      }
      if (g_strcmp0(rd->d_name, ".") == 0) {
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
      case DT_REG: {
        FBFile *f = g_malloc0(sizeof(FBFile));
        // Rofi expects utf-8, so lets convert the filename.
        f->path = g_build_filename(cdir, rd->d_name, NULL);
        f->name = g_filename_to_utf8(f->path, -1, NULL, NULL, NULL);
        if (f->name == NULL) {
          f->name = rofi_force_utf8(rd->d_name, -1);
        }
        f->type = (rd->d_type == DT_DIR) ? DIRECTORY : RFILE;
        f->icon_fetch_uid = 0;
        f->icon_fetch_size = 0;
        f->link = FALSE;

        g_async_queue_push(pd->async_queue, f);
        if (g_async_queue_length(pd->async_queue) > 10000) {
          write(pd->pipefd2[1], "r", 1);
        }
        break;
      }
      case DT_DIR: {
        char *d = g_build_filename(cdir, rd->d_name, NULL);
        GFile *dirp = g_file_new_for_path(d);
        scan_dir(pd, dirp);
        g_object_unref(dirp);
        g_free(d);
        break;
      }
      case DT_LNK: {
        FBFile *f = g_malloc0(sizeof(FBFile));
        // Rofi expects utf-8, so lets convert the filename.
        f->name = g_filename_to_utf8(rd->d_name, -1, NULL, NULL, NULL);
        if (f->name == NULL) {
          f->name = rofi_force_utf8(rd->d_name, -1);
        }
        f->path = g_build_filename(cdir, rd->d_name, NULL);
        f->icon_fetch_uid = 0;
        f->icon_fetch_size = 0;
        f->link = TRUE;
        // Default to file.
        f->type = RFILE;
        g_async_queue_push(pd->async_queue, f);
        if (g_async_queue_length(pd->async_queue) > 10000) {
          write(pd->pipefd2[1], "r", 1);
        }
        break;
      }
      }
    }
    closedir(dir);
  }

  g_free(cdir);
}
static gpointer recursive_browser_input_thread(gpointer userdata) {
  FileBrowserModePrivateData *pd = (FileBrowserModePrivateData *)userdata;
  printf("Start scan.\n");
  scan_dir(pd, pd->current_dir);
  write(pd->pipefd2[1], "r", 1);
  write(pd->pipefd2[1], "q", 1);
  printf("End scan.\n");
  return NULL;
}
static gboolean recursive_browser_async_read_proc(gint fd,
                                                  GIOCondition condition,
                                                  gpointer user_data) {
  FileBrowserModePrivateData *pd = (FileBrowserModePrivateData *)user_data;
  char command;
  // Only interrested in read events.
  if ((condition & G_IO_IN) != G_IO_IN) {
    return G_SOURCE_CONTINUE;
  }
  // Read the entry from the pipe that was used to signal this action.
  if (read(fd, &command, 1) == 1) {
    if (command == 'r') {
      FBFile *block = NULL;
      gboolean changed = FALSE;
      // Empty out the AsyncQueue (that is thread safe) from all blocks pushed
      // into it.
      while ((block = g_async_queue_try_pop(pd->async_queue)) != NULL) {

        fb_resize_array(pd);
        pd->array[pd->array_length] = *block;
        pd->array_length++;
        g_free(block);
        changed = TRUE;
      }
      if (changed) {
        rofi_view_reload();
      }
    } else if (command == 'q') {
      if (pd->loading) {
        rofi_view_set_overlay(rofi_view_get_active(), NULL);
      }
    }
  }
  return G_SOURCE_CONTINUE;
}

static int recursive_browser_mode_init(Mode *sw) {
  /**
   * Called on startup when enabled (in modes list)
   */
  if (mode_get_private_data(sw) == NULL) {
    FileBrowserModePrivateData *pd = g_malloc0(sizeof(*pd));
    mode_set_private_data(sw, (void *)pd);

    recursive_browser_mode_init_config(sw);
    recursive_browser_mode_init_current_dir(sw);

    // Load content.
    if (pipe(pd->pipefd2) == -1) {
      g_error("Failed to create pipe");
    }
    pd->wake_source = g_unix_fd_add(pd->pipefd2[0], G_IO_IN,
                                    recursive_browser_async_read_proc, pd);

    // Create the message passing queue to the UI thread.
    pd->async_queue = g_async_queue_new();
    pd->end_thread = FALSE;
    pd->reading_thread = g_thread_new(
        "dmenu-read", (GThreadFunc)recursive_browser_input_thread, pd);
    pd->loading = TRUE;
  }
  return TRUE;
}
static unsigned int recursive_browser_mode_get_num_entries(const Mode *sw) {
  const FileBrowserModePrivateData *pd =
      (const FileBrowserModePrivateData *)mode_get_private_data(sw);
  return pd->array_length;
}

static ModeMode recursive_browser_mode_result(Mode *sw, int mretv, char **input,
                                              unsigned int selected_line) {
  ModeMode retv = MODE_EXIT;
  FileBrowserModePrivateData *pd =
      (FileBrowserModePrivateData *)mode_get_private_data(sw);

  if ((mretv & MENU_CANCEL) == MENU_CANCEL) {
    ThemeWidget *wid = rofi_config_find_widget(sw->name, NULL, TRUE);
    Property *p =
        rofi_theme_find_property(wid, P_BOOLEAN, "cancel-returns-1", TRUE);
    if (p && p->type == P_BOOLEAN && p->value.b == TRUE) {
      rofi_set_return_code(1);
    }
    return MODE_EXIT;
  }
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
      if (pd->array[selected_line].type == RFILE) {
        char *d_esc = g_shell_quote(pd->array[selected_line].path);
        char *cmd = g_strdup_printf("%s %s", pd->command, d_esc);
        g_free(d_esc);
        char *cdir = g_file_get_path(pd->current_dir);
        helper_execute_command(cdir, cmd, FALSE, NULL);
        g_free(cdir);
        g_free(cmd);
        return MODE_EXIT;
      }
    }
    retv = RELOAD_DIALOG;
  } else if ((mretv & MENU_CUSTOM_INPUT)) {
    retv = RELOAD_DIALOG;
  } else if ((mretv & MENU_ENTRY_DELETE) == MENU_ENTRY_DELETE) {
    retv = RELOAD_DIALOG;
  }
  return retv;
}

static void recursive_browser_mode_destroy(Mode *sw) {
  FileBrowserModePrivateData *pd =
      (FileBrowserModePrivateData *)mode_get_private_data(sw);
  if (pd != NULL) {
    if (pd->reading_thread) {
      pd->end_thread = TRUE;
      g_thread_join(pd->reading_thread);
    }
    g_object_unref(pd->current_dir);
    g_free(pd->command);
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
static int recursive_browser_token_match(const Mode *sw,
                                         rofi_int_matcher **tokens,
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
    dr->icon_fetch_uid =
        rofi_icon_fetcher_query(rb_icon_name[dr->type], height);
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

Mode *create_new_recursive_browser(void) {
  Mode *sw = g_malloc0(sizeof(Mode));

  *sw = recursive_browser_mode;

  sw->private_data = NULL;
  return sw;
}

#if 0
ModeMode recursive_browser_mode_completer(Mode *sw, int mretv, char **input,
                                          unsigned int selected_line,
                                          char **path) {
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
          get_recursive_browser(sw, pd->current_dir);
          return RESET_DIALOG;
        }
      } else if (pd->array[selected_line].type == DIRECTORY) {
        GFile *new = g_file_new_for_path(pd->array[selected_line].path);
        g_object_unref(pd->current_dir);
        pd->current_dir = new;
        free_list(pd);
        get_recursive_browser(sw, pd->current_dir);
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
        get_recursive_browser(sw, pd->current_dir);
        return RESET_DIALOG;
      }
    }
    g_free(dir);
    retv = RELOAD_DIALOG;
  } else if ((mretv & MENU_ENTRY_DELETE) == MENU_ENTRY_DELETE) {
    recursive_browser_config.show_hidden =
        !recursive_browser_config.show_hidden;
    free_list(pd);
    get_recursive_browser(sw, pd->current_dir);
    retv = RELOAD_DIALOG;
  }
  return retv;
}
#endif

Mode recursive_browser_mode = {
    .display_name = NULL,
    .abi_version = ABI_VERSION,
    .name = "recursivebrowser",
    .cfg_name_key = "display-recursivebrowser",
    ._init = recursive_browser_mode_init,
    ._get_num_entries = recursive_browser_mode_get_num_entries,
    ._result = recursive_browser_mode_result,
    ._destroy = recursive_browser_mode_destroy,
    ._token_match = recursive_browser_token_match,
    ._get_display_value = _get_display_value,
    ._get_icon = _get_icon,
    ._get_message = _get_message,
    ._get_completion = _get_completion,
    ._preprocess_input = NULL,
    .private_data = NULL,
    .free = NULL,
};
