/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2023 Qball Cow <qball@gmpclient.org>
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

#define G_LOG_DOMAIN "Modes.DRun"
#include "config.h"
/** The log domain of this dialog. */
#include "glib.h"

#ifdef ENABLE_DRUN
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <gio/gio.h>

#include "helper.h"
#include "history.h"
#include "mode-private.h"
#include "modes/drun.h"
#include "modes/filebrowser.h"
#include "rofi.h"
#include "settings.h"
#include "timings.h"
#include "widgets/textbox.h"
#include "xcb.h"

#include "rofi-icon-fetcher.h"

/** The filename of the history cache file. */
#define DRUN_CACHE_FILE "rofi3.druncache"

/** The filename of the drun quick-load cache file. */
#define DRUN_DESKTOP_CACHE_FILE "rofi-drun-desktop.cache"

/** The group name used in desktop files */
char *DRUN_GROUP_NAME = "Desktop Entry";

/**
 *The Internal data structure for the drun mode.
 */
typedef struct _DRunModePrivateData DRunModePrivateData;

/**
 * Used to determine the type of desktop file.
 */
typedef enum {
  /** Unknown. */
  DRUN_DESKTOP_ENTRY_TYPE_UNDETERMINED = 0,
  /** Application */
  DRUN_DESKTOP_ENTRY_TYPE_APPLICATION,
  /** Link */
  DRUN_DESKTOP_ENTRY_TYPE_LINK,
  /** KDE Service File */
  DRUN_DESKTOP_ENTRY_TYPE_SERVICE,
  /** Directory */
  DRUN_DESKTOP_ENTRY_TYPE_DIRECTORY,
} DRunDesktopEntryType;

/**
 * Store extra information about the entry.
 * Currently the executable and if it should run in terminal.
 */
typedef struct {
  DRunModePrivateData *pd;
  /* category */
  char *action;
  /* Root */
  char *root;
  /* Path to desktop file */
  char *path;
  /* Application id (.desktop filename) */
  char *app_id;
  /* Desktop id */
  char *desktop_id;
  /* Icon stuff */
  char *icon_name;
  /* Icon size is used to indicate what size is requested by the
   * gui. secondary it indicates if the request for a lookup has
   * been issued (0 not issued )
   */
  int icon_size;
  /* Surface holding the icon. */
  cairo_surface_t *icon;
  /* Executable - for Application entries only */
  char *exec;
  /* Name of the Entry */
  char *name;
  /* Generic Name */
  char *generic_name;
  /* Categories */
  char **categories;
  /* Keywords */
  char **keywords;
  /* Comments */
  char *comment;
  /* Url */
  char *url;
  /* Underlying key-file. */
  GKeyFile *key_file;
  /* Used for sorting. */
  gint sort_index;
  /* UID for the icon to display */
  uint32_t icon_fetch_uid;
  uint32_t icon_fetch_size;
  /* Type of desktop file */
  DRunDesktopEntryType type;
} DRunModeEntry;

typedef struct {
  const char *entry_field_name;
  gboolean enabled_match;
  gboolean enabled_display;
} DRunEntryField;

/** The fields that can be displayed and used for matching */
typedef enum {
  /** Name */
  DRUN_MATCH_FIELD_NAME,
  /** Generic Name */
  DRUN_MATCH_FIELD_GENERIC,
  /** Exec */
  DRUN_MATCH_FIELD_EXEC,
  /** List of categories */
  DRUN_MATCH_FIELD_CATEGORIES,
  /** List of keywords */
  DRUN_MATCH_FIELD_KEYWORDS,
  /** Comment */
  DRUN_MATCH_FIELD_COMMENT,
  /** Url */
  DRUN_MATCH_FIELD_URL,
  /** Number of DRunMatchingFields entries. */
  DRUN_MATCH_NUM_FIELDS,
} DRunMatchingFields;

/** Stores what fields should be matched on user input. based on user setting.
 */
static DRunEntryField matching_entry_fields[DRUN_MATCH_NUM_FIELDS] = {
    {
        .entry_field_name = "name",
        .enabled_match = TRUE,
        .enabled_display = TRUE,
    },
    {
        .entry_field_name = "generic",
        .enabled_match = TRUE,
        .enabled_display = TRUE,
    },
    {
        .entry_field_name = "exec",
        .enabled_match = TRUE,
        .enabled_display = TRUE,
    },
    {
        .entry_field_name = "categories",
        .enabled_match = TRUE,
        .enabled_display = TRUE,
    },
    {
        .entry_field_name = "keywords",
        .enabled_match = TRUE,
        .enabled_display = TRUE,
    },
    {
        .entry_field_name = "comment",
        .enabled_match = FALSE,
        .enabled_display = FALSE,
    },
    {
        .entry_field_name = "url",
        .enabled_match = FALSE,
        .enabled_display = FALSE,
    }};

struct _DRunModePrivateData {
  DRunModeEntry *entry_list;
  unsigned int cmd_list_length;
  unsigned int cmd_list_length_actual;
  // List of disabled entries.
  GHashTable *disabled_entries;
  unsigned int disabled_entries_length;
  unsigned int expected_line_height;

  char **show_categories;

  // Theme
  const gchar *icon_theme;
  // DE
  gchar **current_desktop_list;

  gboolean file_complete;
  Mode *completer;
  char *old_completer_input;
  uint32_t selected_line;
  char *old_input;

  gboolean disable_dbusactivate;
};

struct RegexEvalArg {
  DRunModeEntry *e;
  const char *path;
  gboolean success;
};

static gboolean drun_helper_eval_cb(const GMatchInfo *info, GString *res,
                                    gpointer data) {
  // TODO quoting is not right? Find description not very clear, need to check.
  struct RegexEvalArg *e = (struct RegexEvalArg *)data;

  gchar *match;
  // Get the match
  match = g_match_info_fetch(info, 0);
  if (match != NULL) {
    switch (match[1]) {
    case 'f':
    case 'F':
    case 'u':
    case 'U':
      if (e->path) {
        g_string_append(res, e->path);
      }
      break;
    // Unsupported
    case 'i':
      // TODO
      if (e->e && e->e->icon) {
        g_string_append_printf(res, "--icon %s", e->e->icon_name);
      }
      break;
    // Deprecated
    case 'd':
    case 'D':
    case 'n':
    case 'N':
    case 'v':
    case 'm':
      break;
    case '%':
      g_string_append(res, "%");
      break;
    case 'k':
      if (e->e->path) {
        char *esc = g_shell_quote(e->e->path);
        g_string_append(res, esc);
        g_free(esc);
      }
      break;
    case 'c':
      if (e->e->name) {
        char *esc = g_shell_quote(e->e->name);
        g_string_append(res, esc);
        g_free(esc);
      }
      break;
    // Invalid, this entry should not be processed -> throw error.
    default:
      e->success = FALSE;
      g_free(match);
      return TRUE;
    }
    g_free(match);
  }
  // Continue replacement.
  return FALSE;
}
static void launch_link_entry(DRunModeEntry *e) {
  if (e->key_file == NULL) {
    GKeyFile *kf = g_key_file_new();
    GError *error = NULL;
    gboolean res = g_key_file_load_from_file(kf, e->path, 0, &error);
    if (res) {
      e->key_file = kf;
    } else {
      g_warning("[%s] [%s] Failed to parse desktop file because: %s.",
                e->app_id, e->path, error->message);
      g_error_free(error);
      g_key_file_free(kf);
      return;
    }
  }

  gchar *url = g_key_file_get_string(e->key_file, e->action, "URL", NULL);
  if (url == NULL || strlen(url) == 0) {
    g_warning("[%s] [%s] No URL found.", e->app_id, e->path);
    g_free(url);
    return;
  }

  gsize command_len = strlen(config.drun_url_launcher) + strlen(url) +
                      2; // space + terminator = 2
  gchar *command = g_newa(gchar, command_len);
  g_snprintf(command, command_len, "%s %s", config.drun_url_launcher, url);
  g_free(url);

  g_debug("Link launch command: |%s|", command);
  if (helper_execute_command(NULL, command, FALSE, NULL)) {
    char *path = g_build_filename(cache_dir, DRUN_CACHE_FILE, NULL);
    // Store it based on the unique identifiers (desktop_id).
    history_set(path, e->desktop_id);
    g_free(path);
  }
}
static gchar *app_path_for_id(const gchar *app_id) {
  gchar *path;
  gint i;

  path = g_strconcat("/", app_id, NULL);
  for (i = 0; path[i]; i++) {
    if (path[i] == '.')
      path[i] = '/';
    if (path[i] == '-')
      path[i] = '_';
  }

  return path;
}
static GVariant *app_get_platform_data(void) {
  GVariantBuilder builder;
  const gchar *startup_id;

  g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);

  if ((startup_id = g_getenv("DESKTOP_STARTUP_ID")))
    g_variant_builder_add(&builder, "{sv}", "desktop-startup-id",
                          g_variant_new_string(startup_id));

  if ((startup_id = g_getenv("XDG_ACTIVATION_TOKEN")))
    g_variant_builder_add(&builder, "{sv}", "activation-token",
                          g_variant_new_string(startup_id));

  return g_variant_builder_end(&builder);
}

static gboolean exec_dbus_entry(DRunModeEntry *e, const char *path) {
  GVariantBuilder files;
  GDBusConnection *session;
  GError *error = NULL;
  gchar *object_path;
  GVariant *result;
  GVariant *params = NULL;
  const char *method = "Activate";
  g_debug("Trying to launch desktop file using dbus activation.");

  session = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
  if (!session) {
    g_warning("unable to connect to D-Bus: %s\n", error->message);
    g_error_free(error);
    return FALSE;
  }

  object_path = app_path_for_id(e->app_id);

  g_variant_builder_init(&files, G_VARIANT_TYPE_STRING_ARRAY);

  if (path != NULL) {
    method = "Open";
    params = g_variant_new("(as@a{sv})", &files, app_get_platform_data());
  } else {
    params = g_variant_new("(@a{sv})", app_get_platform_data());
  }
  if (path) {
    GFile *file = g_file_new_for_commandline_arg(path);
    g_variant_builder_add_value(
        &files, g_variant_new_take_string(g_file_get_uri(file)));
    g_object_unref(file);
  }
  // Wait 1500ms, otherwise assume failed.
  result = g_dbus_connection_call_sync(
      session, e->app_id, object_path, "org.freedesktop.Application", method,
      params, G_VARIANT_TYPE_UNIT, G_DBUS_CALL_FLAGS_NONE, 1500, NULL, &error);

  g_free(object_path);

  if (result) {
    g_variant_unref(result);
  } else {
    g_warning("error sending %s message to application: %s\n", "Open",
              error->message);
    g_error_free(error);
    g_object_unref(session);
    return FALSE;
  }
  g_object_unref(session);
  return TRUE;
}

static void exec_cmd_entry(DRunModePrivateData *pd, DRunModeEntry *e,
                           const char *path) {
  GError *error = NULL;
  GRegex *reg = g_regex_new("%[a-zA-Z%]", 0, 0, &error);
  if (error != NULL) {
    g_warning("Internal error, failed to create regex: %s.", error->message);
    g_error_free(error);
    return;
  }
  struct RegexEvalArg earg = {.e = e, .path = path, .success = TRUE};
  char *str = g_regex_replace_eval(reg, e->exec, -1, 0, 0, drun_helper_eval_cb,
                                   &earg, &error);
  if (error != NULL) {
    g_warning("Internal error, failed replace field codes: %s.",
              error->message);
    g_error_free(error);
    return;
  }
  g_regex_unref(reg);
  if (earg.success == FALSE) {
    g_warning("Invalid field code in Exec line: %s.", e->exec);
    ;
    return;
  }
  if (str == NULL) {
    g_warning("Nothing to execute after processing: %s.", e->exec);
    ;
    return;
  }
  g_debug("Parsed command: |%s| into |%s|.", e->exec, str);

  if (e->key_file == NULL) {
    GKeyFile *kf = g_key_file_new();
    GError *key_error = NULL;
    gboolean res = g_key_file_load_from_file(kf, e->path, 0, &key_error);
    if (res) {
      e->key_file = kf;
    } else {
      g_warning("[%s] [%s] Failed to parse desktop file because: %s.",
                e->app_id, e->path, key_error->message);
      g_error_free(key_error);
      g_key_file_free(kf);

      return;
    }
  }

  const gchar *fp = g_strstrip(str);
  gchar *exec_path =
      g_key_file_get_string(e->key_file, e->action, "Path", NULL);
  if (exec_path != NULL && strlen(exec_path) == 0) {
    // If it is empty, ignore this property. (#529)
    g_free(exec_path);
    exec_path = NULL;
  }

  RofiHelperExecuteContext context = {
      .name = e->name,
      .icon = e->icon_name,
      .app_id = e->app_id,
  };
  gboolean sn =
      g_key_file_get_boolean(e->key_file, e->action, "StartupNotify", NULL);
  gchar *wmclass = NULL;
  if (sn &&
      g_key_file_has_key(e->key_file, e->action, "StartupWMClass", NULL)) {
    context.wmclass = wmclass =
        g_key_file_get_string(e->key_file, e->action, "StartupWMClass", NULL);
  }

  /**
   * If its required to launch via dbus, do that.
   */
  gboolean launched = FALSE;
  if (!(pd->disable_dbusactivate)) {
    if (g_key_file_get_boolean(e->key_file, e->action, "DBusActivatable",
                               NULL)) {
      printf("DBus launch\n");
      launched = exec_dbus_entry(e, path);
    }
  }
  if (launched == FALSE) {
    /** Fallback to old style if not set. */

    // Returns false if not found, if key not found, we don't want run in
    // terminal.
    gboolean terminal =
        g_key_file_get_boolean(e->key_file, e->action, "Terminal", NULL);
    if (helper_execute_command(exec_path, fp, terminal, sn ? &context : NULL)) {
      char *drun_cach_path = g_build_filename(cache_dir, DRUN_CACHE_FILE, NULL);
      // Store it based on the unique identifiers (desktop_id).
      history_set(drun_cach_path, e->desktop_id);
      g_free(drun_cach_path);
    }
  }
  g_free(wmclass);
  g_free(exec_path);
  g_free(str);
}

static gboolean rofi_strv_contains(const char *const *categories,
                                   const char *const *field) {
  for (int i = 0; categories && categories[i]; i++) {
    for (int j = 0; field[j]; j++) {
      if (g_str_equal(categories[i], field[j])) {
        return TRUE;
      }
    }
  }
  return FALSE;
}
/**
 * This function absorbs/freeś path, so this is no longer available afterwards.
 */
static void read_desktop_file(DRunModePrivateData *pd, const char *root,
                              const char *path, const gchar *basename,
                              const char *action) {
  DRunDesktopEntryType desktop_entry_type =
      DRUN_DESKTOP_ENTRY_TYPE_UNDETERMINED;
  int parse_action = (config.drun_show_actions && action != DRUN_GROUP_NAME);
  // Create ID on stack.
  // We know strlen (path ) > strlen(root)+1
  const ssize_t id_len = strlen(path) - strlen(root);
  char id[id_len];
  g_strlcpy(id, &(path[strlen(root) + 1]), id_len);
  for (int index = 0; index < id_len; index++) {
    if (id[index] == '/') {
      id[index] = '-';
    }
  }

  // Check if item is on disabled list.
  if (g_hash_table_contains(pd->disabled_entries, id) && !parse_action) {
    g_debug("[%s] [%s] Skipping, was previously seen.", id, path);
    return;
  }
  GKeyFile *kf = g_key_file_new();
  GError *error = NULL;
  gboolean res = g_key_file_load_from_file(kf, path, 0, &error);
  // If error, skip to next entry
  if (!res) {
    g_debug("[%s] [%s] Failed to parse desktop file because: %s.", id, path,
            error->message);
    g_error_free(error);
    g_key_file_free(kf);
    return;
  }

  if (g_key_file_has_group(kf, action) == FALSE) {
    // No type? ignore.
    g_debug("[%s] [%s] Invalid desktop file: No %s group", id, path, action);
    g_key_file_free(kf);
    return;
  }
  // Skip non Application entries.
  gchar *key = g_key_file_get_string(kf, DRUN_GROUP_NAME, "Type", NULL);
  if (key == NULL) {
    // No type? ignore.
    g_debug("[%s] [%s] Invalid desktop file: No type indicated", id, path);
    g_key_file_free(kf);
    return;
  }
  if (!g_strcmp0(key, "Application")) {
    desktop_entry_type = DRUN_DESKTOP_ENTRY_TYPE_APPLICATION;
  } else if (!g_strcmp0(key, "Link")) {
    desktop_entry_type = DRUN_DESKTOP_ENTRY_TYPE_LINK;
  } else if (!g_strcmp0(key, "Service")) {
    desktop_entry_type = DRUN_DESKTOP_ENTRY_TYPE_SERVICE;
    g_debug("Service file detected.");
  } else {
    g_debug(
        "[%s] [%s] Skipping desktop file: Not of type Application or Link (%s)",
        id, path, key);
    g_free(key);
    g_key_file_free(kf);
    return;
  }
  g_free(key);

  // Name key is required.
  if (!g_key_file_has_key(kf, DRUN_GROUP_NAME, "Name", NULL)) {
    g_debug("[%s] [%s] Invalid desktop file: no 'Name' key present.", id, path);
    g_key_file_free(kf);
    return;
  }

  // Skip hidden entries.
  if (g_key_file_get_boolean(kf, DRUN_GROUP_NAME, "Hidden", NULL)) {
    g_debug(
        "[%s] [%s] Adding desktop file to disabled list: 'Hidden' key is true",
        id, path);
    g_key_file_free(kf);
    g_hash_table_add(pd->disabled_entries, g_strdup(id));
    return;
  }
  if (pd->current_desktop_list) {
    gboolean show = TRUE;
    // If the DE is set, check the keys.
    if (g_key_file_has_key(kf, DRUN_GROUP_NAME, "OnlyShowIn", NULL)) {
      gsize llength = 0;
      show = FALSE;
      gchar **list = g_key_file_get_string_list(kf, DRUN_GROUP_NAME,
                                                "OnlyShowIn", &llength, NULL);
      if (list) {
        for (gsize lcd = 0; !show && pd->current_desktop_list[lcd]; lcd++) {
          for (gsize lle = 0; !show && lle < llength; lle++) {
            show = (g_strcmp0(pd->current_desktop_list[lcd], list[lle]) == 0);
          }
        }
        g_strfreev(list);
      }
    }
    if (show && g_key_file_has_key(kf, DRUN_GROUP_NAME, "NotShowIn", NULL)) {
      gsize llength = 0;
      gchar **list = g_key_file_get_string_list(kf, DRUN_GROUP_NAME,
                                                "NotShowIn", &llength, NULL);
      if (list) {
        for (gsize lcd = 0; show && pd->current_desktop_list[lcd]; lcd++) {
          for (gsize lle = 0; show && lle < llength; lle++) {
            show = !(g_strcmp0(pd->current_desktop_list[lcd], list[lle]) == 0);
          }
        }
        g_strfreev(list);
      }
    }

    if (!show) {
      g_debug("[%s] [%s] Adding desktop file to disabled list: "
              "'OnlyShowIn'/'NotShowIn' keys don't match current desktop",
              id, path);
      g_key_file_free(kf);
      g_hash_table_add(pd->disabled_entries, g_strdup(id));
      return;
    }
  }
  // Skip entries that have NoDisplay set.
  if (g_key_file_get_boolean(kf, DRUN_GROUP_NAME, "NoDisplay", NULL)) {
    g_debug("[%s] [%s] Adding desktop file to disabled list: 'NoDisplay' key "
            "is true",
            id, path);
    g_key_file_free(kf);
    g_hash_table_add(pd->disabled_entries, g_strdup(id));
    return;
  }

  // We need Exec, don't support DBusActivatable
  if (desktop_entry_type == DRUN_DESKTOP_ENTRY_TYPE_APPLICATION &&
      !g_key_file_has_key(kf, DRUN_GROUP_NAME, "Exec", NULL)) {
    g_debug("[%s] [%s] Unsupported desktop file: no 'Exec' key present for "
            "type Application.",
            id, path);
    g_key_file_free(kf);
    return;
  }
  if (desktop_entry_type == DRUN_DESKTOP_ENTRY_TYPE_SERVICE &&
      !g_key_file_has_key(kf, DRUN_GROUP_NAME, "Exec", NULL)) {
    g_debug("[%s] [%s] Unsupported desktop file: no 'Exec' key present for "
            "type Service.",
            id, path);
    g_key_file_free(kf);
    return;
  }
  if (desktop_entry_type == DRUN_DESKTOP_ENTRY_TYPE_LINK &&
      !g_key_file_has_key(kf, DRUN_GROUP_NAME, "URL", NULL)) {
    g_debug("[%s] [%s] Unsupported desktop file: no 'URL' key present for type "
            "Link.",
            id, path);
    g_key_file_free(kf);
    return;
  }

  if (g_key_file_has_key(kf, DRUN_GROUP_NAME, "TryExec", NULL)) {
    char *te = g_key_file_get_string(kf, DRUN_GROUP_NAME, "TryExec", NULL);
    if (!g_path_is_absolute(te)) {
      char *fp = g_find_program_in_path(te);
      if (fp == NULL) {
        g_free(te);
        g_key_file_free(kf);
        return;
      }
      g_free(fp);
    } else {
      if (g_file_test(te, G_FILE_TEST_IS_EXECUTABLE) == FALSE) {
        g_free(te);
        g_key_file_free(kf);
        return;
      }
    }
    g_free(te);
  }

  char **categories = NULL;
  if (pd->show_categories) {
    categories = g_key_file_get_locale_string_list(
        kf, DRUN_GROUP_NAME, "Categories", NULL, NULL, NULL);
    if (!rofi_strv_contains((const char *const *)categories,
                            (const char *const *)pd->show_categories)) {
      g_strfreev(categories);
      g_key_file_free(kf);
      return;
    }
  }

  size_t nl = ((pd->cmd_list_length) + 1);
  if (nl >= pd->cmd_list_length_actual) {
    pd->cmd_list_length_actual += 256;
    pd->entry_list = g_realloc(pd->entry_list, pd->cmd_list_length_actual *
                                                   sizeof(*(pd->entry_list)));
  }
  // Make sure order is preserved, this will break when cmd_list_length is
  // bigger then INT_MAX. This is not likely to happen.
  if (G_UNLIKELY(pd->cmd_list_length > INT_MAX)) {
    // Default to smallest value.
    pd->entry_list[pd->cmd_list_length].sort_index = INT_MIN;
  } else {
    pd->entry_list[pd->cmd_list_length].sort_index = -nl;
  }
  pd->entry_list[pd->cmd_list_length].icon_size = 0;
  pd->entry_list[pd->cmd_list_length].icon_fetch_uid = 0;
  pd->entry_list[pd->cmd_list_length].icon_fetch_size = 0;
  pd->entry_list[pd->cmd_list_length].root = g_strdup(root);
  pd->entry_list[pd->cmd_list_length].path = g_strdup(path);
  pd->entry_list[pd->cmd_list_length].desktop_id = g_strdup(id);
  pd->entry_list[pd->cmd_list_length].app_id =
      g_strndup(basename, strlen(basename) - strlen(".desktop"));
  gchar *n =
      g_key_file_get_locale_string(kf, DRUN_GROUP_NAME, "Name", NULL, NULL);

  if (action != DRUN_GROUP_NAME) {
    gchar *na = g_key_file_get_locale_string(kf, action, "Name", NULL, NULL);
    gchar *l = g_strdup_printf("%s - %s", n, na);
    g_free(n);
    n = l;
  }
  pd->entry_list[pd->cmd_list_length].name = n;
  pd->entry_list[pd->cmd_list_length].action = DRUN_GROUP_NAME;
  gchar *gn = g_key_file_get_locale_string(kf, DRUN_GROUP_NAME, "GenericName",
                                           NULL, NULL);
  pd->entry_list[pd->cmd_list_length].generic_name = gn;
  if (matching_entry_fields[DRUN_MATCH_FIELD_KEYWORDS].enabled_match ||
      matching_entry_fields[DRUN_MATCH_FIELD_CATEGORIES].enabled_display) {
    pd->entry_list[pd->cmd_list_length].keywords =
        g_key_file_get_locale_string_list(kf, DRUN_GROUP_NAME, "Keywords", NULL,
                                          NULL, NULL);
  } else {
    pd->entry_list[pd->cmd_list_length].keywords = NULL;
  }

  if (matching_entry_fields[DRUN_MATCH_FIELD_CATEGORIES].enabled_match ||
      matching_entry_fields[DRUN_MATCH_FIELD_CATEGORIES].enabled_display) {
    if (categories) {
      pd->entry_list[pd->cmd_list_length].categories = categories;
      categories = NULL;
    } else {
      pd->entry_list[pd->cmd_list_length].categories =
          g_key_file_get_locale_string_list(kf, DRUN_GROUP_NAME, "Categories",
                                            NULL, NULL, NULL);
    }
  } else {
    pd->entry_list[pd->cmd_list_length].categories = NULL;
  }
  g_strfreev(categories);

  pd->entry_list[pd->cmd_list_length].type = desktop_entry_type;
  if (desktop_entry_type == DRUN_DESKTOP_ENTRY_TYPE_APPLICATION ||
      desktop_entry_type == DRUN_DESKTOP_ENTRY_TYPE_SERVICE) {
    pd->entry_list[pd->cmd_list_length].exec =
        g_key_file_get_string(kf, action, "Exec", NULL);
  } else {
    pd->entry_list[pd->cmd_list_length].exec = NULL;
  }

  if (matching_entry_fields[DRUN_MATCH_FIELD_COMMENT].enabled_match ||
      matching_entry_fields[DRUN_MATCH_FIELD_COMMENT].enabled_display) {
    pd->entry_list[pd->cmd_list_length].comment = g_key_file_get_locale_string(
        kf, DRUN_GROUP_NAME, "Comment", NULL, NULL);
  } else {
    pd->entry_list[pd->cmd_list_length].comment = NULL;
  }
  if (matching_entry_fields[DRUN_MATCH_FIELD_URL].enabled_match ||
      matching_entry_fields[DRUN_MATCH_FIELD_URL].enabled_display) {
    pd->entry_list[pd->cmd_list_length].url =
        g_key_file_get_locale_string(kf, DRUN_GROUP_NAME, "URL", NULL, NULL);
  } else {
    pd->entry_list[pd->cmd_list_length].url = NULL;
  }
  pd->entry_list[pd->cmd_list_length].icon_name =
      g_key_file_get_locale_string(kf, DRUN_GROUP_NAME, "Icon", NULL, NULL);
  pd->entry_list[pd->cmd_list_length].icon = NULL;

  // Keep keyfile around.
  pd->entry_list[pd->cmd_list_length].key_file = kf;
  // We don't want to parse items with this id anymore.
  g_hash_table_add(pd->disabled_entries, g_strdup(id));
  g_debug("[%s] Using file %s.", id, path);
  (pd->cmd_list_length)++;

  if (!parse_action) {
    gsize actions_length = 0;
    char **actions = g_key_file_get_string_list(kf, DRUN_GROUP_NAME, "Actions",
                                                &actions_length, NULL);
    for (gsize iter = 0; iter < actions_length; iter++) {
      char *new_action = g_strdup_printf("Desktop Action %s", actions[iter]);
      read_desktop_file(pd, root, path, basename, new_action);
      g_free(new_action);
    }
    g_strfreev(actions);
  }
  return;
}

/**
 * Internal spider used to get list of executables.
 */
static void walk_dir(DRunModePrivateData *pd, const char *root,
                     const char *dirname, const gboolean recursive) {
  DIR *dir;

  g_debug("Checking directory %s for desktop files.", dirname);
  dir = opendir(dirname);
  if (dir == NULL) {
    return;
  }

  struct dirent *file;
  gchar *filename = NULL;
  struct stat st;
  while ((file = readdir(dir)) != NULL) {
    if (file->d_name[0] == '.') {
      continue;
    }
    switch (file->d_type) {
    case DT_LNK:
    case DT_REG:
    case DT_DIR:
    case DT_UNKNOWN:
      filename = g_build_filename(dirname, file->d_name, NULL);
      break;
    default:
      continue;
    }

    // On a link, or if FS does not support providing this information
    // Fallback to stat method.
    if (file->d_type == DT_LNK || file->d_type == DT_UNKNOWN) {
      file->d_type = DT_UNKNOWN;
      if (stat(filename, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
          file->d_type = DT_DIR;
        } else if (S_ISREG(st.st_mode)) {
          file->d_type = DT_REG;
        }
      }
    }

    switch (file->d_type) {
    case DT_REG:
      // Skip files not ending on .desktop.
      if (g_str_has_suffix(file->d_name, ".desktop")) {
        read_desktop_file(pd, root, filename, file->d_name, DRUN_GROUP_NAME);
      }
      break;
    case DT_DIR:
      if (recursive) {
        walk_dir(pd, root, filename, recursive);
      }
      break;
    default:
      break;
    }
    g_free(filename);
  }
  closedir(dir);
}
/**
 * @param entry The command entry to remove from history
 *
 * Remove command from history.
 */
static void delete_entry_history(const DRunModeEntry *entry) {
  char *path = g_build_filename(cache_dir, DRUN_CACHE_FILE, NULL);
  history_remove(path, entry->desktop_id);
  g_free(path);
}

static void get_apps_history(DRunModePrivateData *pd) {
  TICK_N("Start drun history");
  unsigned int length = 0;
  gchar *path = g_build_filename(cache_dir, DRUN_CACHE_FILE, NULL);
  gchar **retv = history_get_list(path, &length);
  for (unsigned int index = 0; index < length; index++) {
    for (size_t i = 0; i < pd->cmd_list_length; i++) {
      if (g_strcmp0(pd->entry_list[i].desktop_id, retv[index]) == 0) {
        unsigned int sort_index = length - index;
        if (G_LIKELY(sort_index < INT_MAX)) {
          pd->entry_list[i].sort_index = sort_index;
        } else {
          // This won't sort right anymore, but never gonna hit it anyway.
          pd->entry_list[i].sort_index = INT_MAX;
        }
      }
    }
  }
  g_strfreev(retv);
  g_free(path);
  TICK_N("Stop drun history");
}

static gint drun_int_sort_list(gconstpointer a, gconstpointer b,
                               G_GNUC_UNUSED gpointer user_data) {
  DRunModeEntry *da = (DRunModeEntry *)a;
  DRunModeEntry *db = (DRunModeEntry *)b;

  if (da->sort_index < 0 && db->sort_index < 0) {
    if (da->name == NULL && db->name == NULL) {
      return 0;
    }
    if (da->name == NULL) {
      return -1;
    }
    if (db->name == NULL) {
      return 1;
    }
    return g_utf8_collate(da->name, db->name);
  }
  return db->sort_index - da->sort_index;
}

/*******************************************
 * Cache voodoo                            *
 *******************************************/

/** Version of the DRUN cache file format. */
#define CACHE_VERSION 3
static void drun_write_str(FILE *fd, const char *str) {
  size_t l = (str == NULL ? 0 : strlen(str));
  fwrite(&l, sizeof(l), 1, fd);
  // Only write string if it is not NULL or empty.
  if (l > 0) {
    // Also writeout terminating '\0'
    fwrite(str, 1, l + 1, fd);
  }
}
static void drun_write_integer(FILE *fd, int32_t val) {
  fwrite(&val, sizeof(val), 1, fd);
}
static void drun_read_integer(FILE *fd, int32_t *type) {
  if (fread(type, sizeof(int32_t), 1, fd) != 1) {
    g_warning("Failed to read entry, cache corrupt?");
    return;
  }
}
static void drun_read_string(FILE *fd, char **str) {
  size_t l = 0;

  if (fread(&l, sizeof(l), 1, fd) != 1) {
    g_warning("Failed to read entry, cache corrupt?");
    return;
  }
  (*str) = NULL;
  if (l > 0) {
    // Include \0
    l++;
    (*str) = g_malloc(l);
    if (fread((*str), 1, l, fd) != l) {
      g_warning("Failed to read entry, cache corrupt?");
    }
  }
}
static void drun_write_strv(FILE *fd, char **str) {
  guint vl = (str == NULL ? 0 : g_strv_length(str));
  fwrite(&vl, sizeof(vl), 1, fd);
  for (guint index = 0; index < vl; index++) {
    drun_write_str(fd, str[index]);
  }
}
static void drun_read_stringv(FILE *fd, char ***str) {
  guint vl = 0;
  (*str) = NULL;
  if (fread(&vl, sizeof(vl), 1, fd) != 1) {
    g_warning("Failed to read entry, cache corrupt?");
    return;
  }
  if (vl > 0) {
    // Include terminating NULL entry.
    (*str) = g_malloc0((vl + 1) * sizeof(**str));
    for (guint index = 0; index < vl; index++) {
      drun_read_string(fd, &((*str)[index]));
    }
  }
}

static void write_cache(DRunModePrivateData *pd, const char *cache_file) {
  if (cache_file == NULL || config.drun_use_desktop_cache == FALSE) {
    return;
  }
  TICK_N("DRUN Write CACHE: start");

  FILE *fd = fopen(cache_file, "w");
  if (fd == NULL) {
    g_warning("Failed to write to cache file");
    return;
  }
  uint8_t version = CACHE_VERSION;
  fwrite(&version, sizeof(version), 1, fd);

  fwrite(&(pd->cmd_list_length), sizeof(pd->cmd_list_length), 1, fd);
  for (unsigned int index = 0; index < pd->cmd_list_length; index++) {
    DRunModeEntry *entry = &(pd->entry_list[index]);

    drun_write_str(fd, entry->action);
    drun_write_str(fd, entry->root);
    drun_write_str(fd, entry->path);
    drun_write_str(fd, entry->app_id);
    drun_write_str(fd, entry->desktop_id);
    drun_write_str(fd, entry->icon_name);
    drun_write_str(fd, entry->exec);
    drun_write_str(fd, entry->name);
    drun_write_str(fd, entry->generic_name);

    drun_write_strv(fd, entry->categories);
    drun_write_strv(fd, entry->keywords);

    drun_write_str(fd, entry->comment);
    drun_write_str(fd, entry->url);
    drun_write_integer(fd, (int32_t)entry->type);
  }

  fclose(fd);
  TICK_N("DRUN Write CACHE: end");
}

/**
 * Read cache file. returns FALSE when success.
 */
static gboolean drun_read_cache(DRunModePrivateData *pd,
                                const char *cache_file) {
  if (cache_file == NULL || config.drun_use_desktop_cache == FALSE) {
    return TRUE;
  }

  if (config.drun_reload_desktop_cache) {
    return TRUE;
  }
  TICK_N("DRUN Read CACHE: start");
  FILE *fd = fopen(cache_file, "r");
  if (fd == NULL) {
    TICK_N("DRUN Read CACHE: stop");
    return TRUE;
  }

  // Read version.
  uint8_t version = 0;

  if (fread(&version, sizeof(version), 1, fd) != 1) {
    fclose(fd);
    g_warning("Cache corrupt, ignoring.");
    TICK_N("DRUN Read CACHE: stop");
    return TRUE;
  }

  if (version != CACHE_VERSION) {
    fclose(fd);
    g_warning("Cache file wrong version, ignoring.");
    TICK_N("DRUN Read CACHE: stop");
    return TRUE;
  }

  if (fread(&(pd->cmd_list_length), sizeof(pd->cmd_list_length), 1, fd) != 1) {
    fclose(fd);
    g_warning("Cache corrupt, ignoring.");
    TICK_N("DRUN Read CACHE: stop");
    return TRUE;
  }
  // set actual length to length;
  pd->cmd_list_length_actual = pd->cmd_list_length;

  pd->entry_list =
      g_malloc0(pd->cmd_list_length_actual * sizeof(*(pd->entry_list)));

  for (unsigned int index = 0; index < pd->cmd_list_length; index++) {
    DRunModeEntry *entry = &(pd->entry_list[index]);

    drun_read_string(fd, &(entry->action));
    drun_read_string(fd, &(entry->root));
    drun_read_string(fd, &(entry->path));
    drun_read_string(fd, &(entry->app_id));
    drun_read_string(fd, &(entry->desktop_id));
    drun_read_string(fd, &(entry->icon_name));
    drun_read_string(fd, &(entry->exec));
    drun_read_string(fd, &(entry->name));
    drun_read_string(fd, &(entry->generic_name));

    drun_read_stringv(fd, &(entry->categories));
    drun_read_stringv(fd, &(entry->keywords));

    drun_read_string(fd, &(entry->comment));
    drun_read_string(fd, &(entry->url));
    int32_t type = 0;
    drun_read_integer(fd, &(type));
    entry->type = type;
  }

  fclose(fd);
  TICK_N("DRUN Read CACHE: stop");
  return FALSE;
}

static void get_apps(DRunModePrivateData *pd) {
  char *cache_file = g_build_filename(cache_dir, DRUN_DESKTOP_CACHE_FILE, NULL);
  TICK_N("Get Desktop apps (start)");
  if (drun_read_cache(pd, cache_file)) {
    ThemeWidget *wid = rofi_config_find_widget(drun_mode.name, NULL, TRUE);

    /** Load desktop entries */
    Property *p =
        rofi_theme_find_property(wid, P_BOOLEAN, "scan-desktop", FALSE);
    if (p != NULL && (p->type == P_BOOLEAN && p->value.b)) {
      const gchar *dir;
      // First read the user directory.
      dir = g_get_user_special_dir(G_USER_DIRECTORY_DESKTOP);
      walk_dir(pd, dir, dir, FALSE);
      TICK_N("Get Desktop dir apps");
    }
    /** Load user entires */
    p = rofi_theme_find_property(wid, P_BOOLEAN, "parse-user", TRUE);
    if (p == NULL || (p->type == P_BOOLEAN && p->value.b)) {
      gchar *dir;
      // First read the user directory.
      dir = g_build_filename(g_get_user_data_dir(), "applications", NULL);
      walk_dir(pd, dir, dir, TRUE);
      g_free(dir);
      TICK_N("Get Desktop apps (user dir)");
    }

    /** Load application entires */
    p = rofi_theme_find_property(wid, P_BOOLEAN, "parse-system", TRUE);
    if (p == NULL || (p->type == P_BOOLEAN && p->value.b)) {
      // Then read thee system data dirs.
      const gchar *const *sys = g_get_system_data_dirs();
      for (const gchar *const *iter = sys; *iter != NULL; ++iter) {
        gboolean unique = TRUE;
        // Stupid duplicate detection, better then walking dir.
        for (const gchar *const *iterd = sys; iterd != iter; ++iterd) {
          if (g_strcmp0(*iter, *iterd) == 0) {
            unique = FALSE;
          }
        }
        // Check, we seem to be getting empty string...
        if (unique && (**iter) != '\0') {
          char *dir = g_build_filename(*iter, "applications", NULL);
          walk_dir(pd, dir, dir, TRUE);
          g_free(dir);
        }
      }
      TICK_N("Get Desktop apps (system dirs)");
    }
    pd->disable_dbusactivate = FALSE;
    p = rofi_theme_find_property(wid, P_BOOLEAN, "DBusActivatable", TRUE);
    if (p != NULL && (p->type == P_BOOLEAN && p->value.b == FALSE)) {
      pd->disable_dbusactivate = TRUE;
    }
    get_apps_history(pd);

    g_qsort_with_data(pd->entry_list, pd->cmd_list_length,
                      sizeof(DRunModeEntry), drun_int_sort_list, NULL);

    TICK_N("Sorting done.");

    write_cache(pd, cache_file);
  }
  g_free(cache_file);
}

static void drun_mode_parse_entry_fields(void) {
  char *savept = NULL;
  // Make a copy, as strtok will modify it.
  char *switcher_str = g_strdup(config.drun_match_fields);
  const char *const sep = ",#";
  // Split token on ','. This modifies switcher_str.
  for (unsigned int i = 0; i < DRUN_MATCH_NUM_FIELDS; i++) {
    matching_entry_fields[i].enabled_match = FALSE;
    matching_entry_fields[i].enabled_display = FALSE;
  }
  for (char *token = strtok_r(switcher_str, sep, &savept); token != NULL;
       token = strtok_r(NULL, sep, &savept)) {
    if (strcmp(token, "all") == 0) {
      for (unsigned int i = 0; i < DRUN_MATCH_NUM_FIELDS; i++) {
        matching_entry_fields[i].enabled_match = TRUE;
        matching_entry_fields[i].enabled_display = TRUE;
      }
      break;
    }
    gboolean matched = FALSE;
    for (unsigned int i = 0; i < DRUN_MATCH_NUM_FIELDS; i++) {
      const char *entry_name = matching_entry_fields[i].entry_field_name;
      if (g_ascii_strcasecmp(token, entry_name) == 0) {
        matching_entry_fields[i].enabled_match = TRUE;
        matching_entry_fields[i].enabled_display = TRUE;
        matched = TRUE;
      }
    }
    if (!matched) {
      g_warning("Invalid entry name :%s", token);
    }
  }
  // Free string that was modified by strtok_r
  g_free(switcher_str);
}

static void drun_mode_parse_display_format(void) {
  for (int i = 0; i < DRUN_MATCH_NUM_FIELDS; i++) {
    if (matching_entry_fields[i].enabled_display)
      continue;

    gchar *search_term =
        g_strdup_printf("{%s}", matching_entry_fields[i].entry_field_name);
    if (strstr(config.drun_display_format, search_term)) {
      matching_entry_fields[i].enabled_match = TRUE;
    }
    g_free(search_term);
  }
}

static int drun_mode_init(Mode *sw) {
  if (mode_get_private_data(sw) != NULL) {
    return TRUE;
  }
  DRunModePrivateData *pd = g_malloc0(sizeof(*pd));
  pd->disabled_entries =
      g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
  mode_set_private_data(sw, (void *)pd);
  // current desktop
  const char *current_desktop = g_getenv("XDG_CURRENT_DESKTOP");
  pd->current_desktop_list =
      current_desktop ? g_strsplit(current_desktop, ":", 0) : NULL;

  if (config.drun_categories && *(config.drun_categories)) {
    pd->show_categories = g_strsplit(config.drun_categories, ",", 0);
  }

  drun_mode_parse_entry_fields();
  drun_mode_parse_display_format();
  get_apps(pd);

  pd->completer = NULL;
  return TRUE;
}
static void drun_entry_clear(DRunModeEntry *e) {
  g_free(e->root);
  g_free(e->path);
  g_free(e->app_id);
  g_free(e->desktop_id);
  if (e->icon != NULL) {
    cairo_surface_destroy(e->icon);
  }
  g_free(e->icon_name);
  g_free(e->exec);
  g_free(e->name);
  g_free(e->generic_name);
  g_free(e->comment);
  if (e->action != DRUN_GROUP_NAME) {
    g_free(e->action);
  }
  g_strfreev(e->categories);
  g_strfreev(e->keywords);
  if (e->key_file) {
    g_key_file_free(e->key_file);
  }
}

static ModeMode drun_mode_result(Mode *sw, int mretv, char **input,
                                 unsigned int selected_line) {
  DRunModePrivateData *rmpd = (DRunModePrivateData *)mode_get_private_data(sw);
  ModeMode retv = MODE_EXIT;

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
      retv = mode_completer_result(rmpd->completer, mretv, input, selected_line,
                                   &path);
      if (retv == MODE_EXIT) {
        exec_cmd_entry(rmpd, &(rmpd->entry_list[rmpd->selected_line]), path);
      }
      g_free(path);
    }
    return retv;
  }
  if ((mretv & MENU_OK)) {
    switch (rmpd->entry_list[selected_line].type) {
    case DRUN_DESKTOP_ENTRY_TYPE_SERVICE:
    case DRUN_DESKTOP_ENTRY_TYPE_APPLICATION:
      exec_cmd_entry(rmpd, &(rmpd->entry_list[selected_line]), NULL);
      break;
    case DRUN_DESKTOP_ENTRY_TYPE_LINK:
      launch_link_entry(&(rmpd->entry_list[selected_line]));
      break;
    default:
      g_assert_not_reached();
    }
  } else if ((mretv & MENU_CUSTOM_INPUT) && *input != NULL &&
             *input[0] != '\0') {
    RofiHelperExecuteContext context = {.name = NULL};
    gboolean run_in_term = ((mretv & MENU_CUSTOM_ACTION) == MENU_CUSTOM_ACTION);
    // FIXME: We assume startup notification in terminals, not in others
    if (!helper_execute_command(NULL, *input, run_in_term,
                                run_in_term ? &context : NULL)) {
      retv = RELOAD_DIALOG;
    }
  } else if ((mretv & MENU_ENTRY_DELETE) &&
             selected_line < rmpd->cmd_list_length) {
    // Positive sort index means it is in history.
    if (rmpd->entry_list[selected_line].sort_index >= 0) {
      delete_entry_history(&(rmpd->entry_list[selected_line]));
      drun_entry_clear(&(rmpd->entry_list[selected_line]));
      memmove(&(rmpd->entry_list[selected_line]),
              &rmpd->entry_list[selected_line + 1],
              sizeof(DRunModeEntry) *
                  (rmpd->cmd_list_length - selected_line - 1));
      rmpd->cmd_list_length--;
    }
    retv = RELOAD_DIALOG;
  } else if (mretv & MENU_CUSTOM_COMMAND) {
    retv = (mretv & MENU_LOWER_MASK);
  } else if ((mretv & MENU_COMPLETE)) {
    retv = RELOAD_DIALOG;
    if (selected_line < rmpd->cmd_list_length) {
      switch (rmpd->entry_list[selected_line].type) {
      case DRUN_DESKTOP_ENTRY_TYPE_SERVICE:
      case DRUN_DESKTOP_ENTRY_TYPE_APPLICATION: {
        GRegex *regex = g_regex_new("%[fFuU]", 0, 0, NULL);

        if (g_regex_match(regex, rmpd->entry_list[selected_line].exec, 0,
                          NULL)) {
          rmpd->selected_line = selected_line;
          // TODO add check if it supports passing file.

          g_free(rmpd->old_input);
          rmpd->old_input = g_strdup(*input);

          if (*input)
            g_free(*input);
          *input = g_strdup(rmpd->old_completer_input);

          const Mode *comp = rofi_get_completer();
          if (comp) {
            rmpd->completer = mode_create(comp);
            mode_init(rmpd->completer);
            rmpd->file_complete = TRUE;
          }
        }
        g_regex_unref(regex);
      }
      default:
        break;
      }
    }
  }
  return retv;
}
static void drun_mode_destroy(Mode *sw) {
  DRunModePrivateData *rmpd = (DRunModePrivateData *)mode_get_private_data(sw);
  if (rmpd != NULL) {
    for (size_t i = 0; i < rmpd->cmd_list_length; i++) {
      drun_entry_clear(&(rmpd->entry_list[i]));
    }
    g_hash_table_destroy(rmpd->disabled_entries);
    g_free(rmpd->entry_list);

    g_free(rmpd->old_completer_input);
    g_free(rmpd->old_input);
    if (rmpd->completer != NULL) {
      mode_destroy(rmpd->completer);
      g_free(rmpd->completer);
    }

    g_strfreev(rmpd->current_desktop_list);
    g_strfreev(rmpd->show_categories);
    g_free(rmpd);
    mode_set_private_data(sw, NULL);
  }
}

static char *_get_display_value(const Mode *sw, unsigned int selected_line,
                                int *state, G_GNUC_UNUSED GList **list,
                                int get_entry) {
  DRunModePrivateData *pd = (DRunModePrivateData *)mode_get_private_data(sw);

  if (pd->file_complete) {
    return pd->completer->_get_display_value(pd->completer, selected_line,
                                             state, list, get_entry);
  }
  *state |= MARKUP;
  if (!get_entry) {
    return NULL;
  }
  if (pd->entry_list == NULL) {
    // Should never get here.
    return g_strdup("Failed");
  }
  /* Free temp storage. */
  DRunModeEntry *dr = &(pd->entry_list[selected_line]);
  gchar *cats = NULL;
  if (dr->categories) {
    char *tcats = g_strjoinv(",", dr->categories);
    if (tcats) {
      cats = g_markup_escape_text(tcats, -1);
      g_free(tcats);
    }
  }
  gchar *keywords = NULL;
  if (dr->keywords) {
    char *tkeyw = g_strjoinv(",", dr->keywords);
    if (tkeyw) {
      keywords = g_markup_escape_text(tkeyw, -1);
      g_free(tkeyw);
    }
  }
  // Needed for display.
  char *egn = NULL;
  char *en = NULL;
  char *ec = NULL;
  char *ee = NULL;
  char *eu = NULL;
  if (dr->generic_name) {
    egn = g_markup_escape_text(dr->generic_name, -1);
  }
  if (dr->name) {
    en = g_markup_escape_text(dr->name, -1);
  }
  if (dr->comment) {
    ec = g_markup_escape_text(dr->comment, -1);
  }
  if (dr->url) {
    eu = g_markup_escape_text(dr->url, -1);
  }
  if (dr->exec) {
    ee = g_markup_escape_text(dr->exec, -1);
  }

  char *retv = helper_string_replace_if_exists(
      config.drun_display_format, "{generic}", egn, "{name}", en, "{comment}",
      ec, "{exec}", ee, "{categories}", cats, "{keywords}", keywords, "{url}",
      eu, (char *)0);
  g_free(egn);
  g_free(en);
  g_free(ec);
  g_free(eu);
  g_free(ee);
  g_free(cats);
  g_free(keywords);
  return retv;
}

static cairo_surface_t *_get_icon(const Mode *sw, unsigned int selected_line,
                                  unsigned int height) {
  DRunModePrivateData *pd = (DRunModePrivateData *)mode_get_private_data(sw);
  if (pd->file_complete) {
    return pd->completer->_get_icon(pd->completer, selected_line, height);
  }
  g_return_val_if_fail(pd->entry_list != NULL, NULL);
  DRunModeEntry *dr = &(pd->entry_list[selected_line]);
  if (dr->icon_name != NULL) {
    if (dr->icon_fetch_uid > 0 && dr->icon_fetch_size == height) {
      cairo_surface_t *icon = rofi_icon_fetcher_get(dr->icon_fetch_uid);
      return icon;
    }
    dr->icon_fetch_uid = rofi_icon_fetcher_query(dr->icon_name, height);
    dr->icon_fetch_size = height;
    cairo_surface_t *icon = rofi_icon_fetcher_get(dr->icon_fetch_uid);
    return icon;
  }
  return NULL;
}

static char *drun_get_completion(const Mode *sw, unsigned int index) {
  DRunModePrivateData *pd = (DRunModePrivateData *)mode_get_private_data(sw);
  /* Free temp storage. */
  DRunModeEntry *dr = &(pd->entry_list[index]);
  if (dr->generic_name == NULL) {
    return g_strdup(dr->name);
  }
  return g_strdup_printf("%s", dr->name);
}

static int drun_token_match(const Mode *data, rofi_int_matcher **tokens,
                            unsigned int index) {
  DRunModePrivateData *rmpd =
      (DRunModePrivateData *)mode_get_private_data(data);
  if (rmpd->file_complete) {
    return rmpd->completer->_token_match(rmpd->completer, tokens, index);
  }
  int match = 1;
  if (tokens) {
    for (int j = 0; match && tokens[j] != NULL; j++) {
      int test = 0;
      rofi_int_matcher *ftokens[2] = {tokens[j], NULL};
      // Match name
      if (matching_entry_fields[DRUN_MATCH_FIELD_NAME].enabled_match) {
        if (rmpd->entry_list[index].name) {
          test = helper_token_match(ftokens, rmpd->entry_list[index].name);
        }
      }
      if (matching_entry_fields[DRUN_MATCH_FIELD_GENERIC].enabled_match) {
        // Match generic name
        if (test == tokens[j]->invert && rmpd->entry_list[index].generic_name) {
          test =
              helper_token_match(ftokens, rmpd->entry_list[index].generic_name);
        }
      }
      if (matching_entry_fields[DRUN_MATCH_FIELD_EXEC].enabled_match) {
        // Match executable name.
        if (test == tokens[j]->invert && rmpd->entry_list[index].exec) {
          test = helper_token_match(ftokens, rmpd->entry_list[index].exec);
        }
      }
      if (matching_entry_fields[DRUN_MATCH_FIELD_CATEGORIES].enabled_match) {
        // Match against category.
        if (test == tokens[j]->invert) {
          gchar **list = rmpd->entry_list[index].categories;
          for (int iter = 0; test == tokens[j]->invert && list && list[iter];
               iter++) {
            test = helper_token_match(ftokens, list[iter]);
          }
        }
      }
      if (matching_entry_fields[DRUN_MATCH_FIELD_KEYWORDS].enabled_match) {
        // Match against category.
        if (test == tokens[j]->invert) {
          gchar **list = rmpd->entry_list[index].keywords;
          for (int iter = 0; test == tokens[j]->invert && list && list[iter];
               iter++) {
            test = helper_token_match(ftokens, list[iter]);
          }
        }
      }
      if (matching_entry_fields[DRUN_MATCH_FIELD_URL].enabled_match) {

        // Match executable name.
        if (test == tokens[j]->invert && rmpd->entry_list[index].url) {
          test = helper_token_match(ftokens, rmpd->entry_list[index].url);
        }
      }
      if (matching_entry_fields[DRUN_MATCH_FIELD_COMMENT].enabled_match) {

        // Match executable name.
        if (test == tokens[j]->invert && rmpd->entry_list[index].comment) {
          test = helper_token_match(ftokens, rmpd->entry_list[index].comment);
        }
      }
      if (test == 0) {
        match = 0;
      }
    }
  }

  return match;
}

static unsigned int drun_mode_get_num_entries(const Mode *sw) {
  const DRunModePrivateData *pd =
      (const DRunModePrivateData *)mode_get_private_data(sw);
  if (pd->file_complete) {
    return pd->completer->_get_num_entries(pd->completer);
  }
  return pd->cmd_list_length;
}
static char *drun_get_message(const Mode *sw) {
  DRunModePrivateData *pd = sw->private_data;
  if (pd->file_complete) {
    if (pd->selected_line < pd->cmd_list_length) {
      char *msg = mode_get_message(pd->completer);
      if (msg) {
        char *retv =
            g_strdup_printf("File complete for: %s\n%s",
                            pd->entry_list[pd->selected_line].name, msg);
        g_free(msg);
        return retv;
      }
      return g_strdup_printf("File complete for: %s",
                             pd->entry_list[pd->selected_line].name);
    }
  }
  return NULL;
}
#include "mode-private.h"
/** The DRun Mode interface. */
Mode drun_mode = {.name = "drun",
                  .cfg_name_key = "display-drun",
                  ._init = drun_mode_init,
                  ._get_num_entries = drun_mode_get_num_entries,
                  ._result = drun_mode_result,
                  ._destroy = drun_mode_destroy,
                  ._token_match = drun_token_match,
                  ._get_message = drun_get_message,
                  ._get_completion = drun_get_completion,
                  ._get_display_value = _get_display_value,
                  ._get_icon = _get_icon,
                  ._preprocess_input = NULL,
                  .private_data = NULL,
                  .free = NULL,
                  .type = MODE_TYPE_SWITCHER};

#endif // ENABLE_DRUN
