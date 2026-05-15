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
#define G_LOG_DOMAIN "Modes.Window"

#include "config.h"

#ifdef WINDOW_MODE

#include <stdint.h>

#include <glib.h>
#include <wayland-client.h>

#include "display.h"
#include "helper.h"
#include "modes/wayland-window.h"
#include "rofi.h"
#include "settings.h"
#include "wayland-internal.h"
#include "widgets/textbox.h"

#include "mode-private.h"
#include "rofi-icon-fetcher.h"

#include "ext-foreign-toplevel-list-v1-protocol.h"
#include "wlr-foreign-toplevel-management-unstable-v1-protocol.h"

#define EXT_FOREIGN_TOPLEVEL_VERSION 1
#define WLR_FOREIGN_TOPLEVEL_VERSION 3

enum WaylandWindowMatchingFields {
  WW_MATCH_FIELD_TITLE = 1 << 0,
  WW_MATCH_FIELD_APP_ID = 1 << 1,
  WW_MATCH_FIELD_ALL = ~0,
};

typedef struct _WaylandWindowModePrivateData {
  wayland_stuff *wayland;
  struct wl_registry *registry;
  struct ext_foreign_toplevel_list_v1 *list;
  struct zwlr_foreign_toplevel_manager_v1 *manager;
  GList *ext_toplevels; /* List of ExtForeignToplevelHandle */
  GList *wlr_toplevels; /* List of WlrForeignToplevelHandle */

  /* initial rendering complete, updates allowed */
  gboolean visible;
  int match_fields; /* bitmask of WaylandWindowMatchingFields */
  glong title_len;
  glong app_id_len;
  GRegex *window_regex;
} WaylandWindowModePrivateData;

enum ForeignToplevelState {
  TOPLEVEL_STATE_MAXIMIZED = 1
                             << ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED,
  TOPLEVEL_STATE_MINIMIZED = 1
                             << ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED,
  TOPLEVEL_STATE_ACTIVATED = 1
                             << ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED,
  TOPLEVEL_STATE_FULLSCREEN =
      1 << ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN,
  TOPLEVEL_STATE_CLOSED = 1 << 4
};

typedef struct {
  struct ext_foreign_toplevel_handle_v1 *handle;
  WaylandWindowModePrivateData *view;

  gchar *app_id;
  gchar *identifier;
} ExtForeignToplevelHandle;

typedef struct {
  struct zwlr_foreign_toplevel_handle_v1 *handle;
  WaylandWindowModePrivateData *view;

  gchar *app_id;
  glong app_id_len;
  gchar *title;
  glong title_len;
  int state;

  unsigned int cached_icon_uid;
  unsigned int cached_icon_size;
  guint cached_icon_scale;
} WlrForeignToplevelHandle;

static void ext_foreign_toplevel_handle_free(ExtForeignToplevelHandle *self) {
  if (self->handle) {
    ext_foreign_toplevel_handle_v1_destroy(self->handle);
    self->handle = NULL;
  }
  g_free(self->app_id);
  g_free(self->identifier);
  g_free(self);
}

static void wlr_foreign_toplevel_handle_free(WlrForeignToplevelHandle *self) {
  if (self->handle) {
    zwlr_foreign_toplevel_handle_v1_destroy(self->handle);
    self->handle = NULL;
  }
  g_free(self->title);
  g_free(self->app_id);
  g_free(self);
}

static void toplevels_list_update_max_len(gpointer data, gpointer user_data) {
  WaylandWindowModePrivateData *pd = (WaylandWindowModePrivateData *)user_data;
  WlrForeignToplevelHandle *entry = (WlrForeignToplevelHandle *)data;

  pd->title_len = MAX(entry->title_len, pd->title_len);
  pd->app_id_len = MAX(entry->app_id_len, pd->app_id_len);
}

/* Update column alignment and schedule reload */
static void wayland_window_update_toplevel(WlrForeignToplevelHandle *toplevel) {
  WaylandWindowModePrivateData *pd = toplevel->view;

  if (!pd->visible) {
    /* initial fetch, just add the current item */
    toplevels_list_update_max_len(toplevel, pd);
  } else {
    /* async update, recalculate from scratch */
    pd->title_len = 0;
    pd->app_id_len = 0;
    g_list_foreach(pd->wlr_toplevels, toplevels_list_update_max_len, pd);
    rofi_view_reload();
  }
}

/* requests */

static void wlr_foreign_toplevel_handle_activate(WlrForeignToplevelHandle *self,
                                                 struct wl_seat *seat) {
  zwlr_foreign_toplevel_handle_v1_activate(self->handle, seat);
}

static void wlr_foreign_toplevel_handle_close(WlrForeignToplevelHandle *self) {
  zwlr_foreign_toplevel_handle_v1_close(self->handle);
}

/* events (ext-foreign-toplevel-list-v1) */

static void ext_foreign_toplevel_handle_done(
    void *data, G_GNUC_UNUSED struct ext_foreign_toplevel_handle_v1 *handle) {
}

static void ext_foreign_toplevel_handle_closed(
    void *data, G_GNUC_UNUSED struct ext_foreign_toplevel_handle_v1 *handle) {
}

static void ext_foreign_toplevel_handle_identifier(
    void *data, G_GNUC_UNUSED struct ext_foreign_toplevel_handle_v1 *handle,
    const char *identifier) {
}

static void ext_foreign_toplevel_handle_title(
    G_GNUC_UNUSED void *data,
    G_GNUC_UNUSED struct ext_foreign_toplevel_handle_v1 *handle,
    G_GNUC_UNUSED const char *title) {
  /* ignore */
}

static void ext_foreign_toplevel_handle_app_id(
    void *data, G_GNUC_UNUSED struct ext_foreign_toplevel_handle_v1 *handle,
    const char *app_id) {
}

static struct ext_foreign_toplevel_handle_v1_listener
    ext_foreign_toplevel_handle_listener = {
        .identifier = &ext_foreign_toplevel_handle_identifier,
        .title = &ext_foreign_toplevel_handle_title,
        .app_id = &ext_foreign_toplevel_handle_app_id,
        .closed = &ext_foreign_toplevel_handle_closed,
        .done = &ext_foreign_toplevel_handle_done};

static ExtForeignToplevelHandle *
ext_foreign_toplevel_handle_new(struct ext_foreign_toplevel_handle_v1 *handle,
                                WaylandWindowModePrivateData *view) {
  return NULL;
}

static void ext_foreign_toplevel_list_toplevel(
    void *data, G_GNUC_UNUSED struct ext_foreign_toplevel_list_v1 *list,
    struct ext_foreign_toplevel_handle_v1 *toplevel) {
}

static void ext_foreign_toplevel_list_finished(
    G_GNUC_UNUSED void *data,
    struct ext_foreign_toplevel_list_v1 *list) {
  ext_foreign_toplevel_list_v1_destroy(list);
}

static struct ext_foreign_toplevel_list_v1_listener
    ext_foreign_toplevel_list_listener = {
        .toplevel = &ext_foreign_toplevel_list_toplevel,
        .finished = &ext_foreign_toplevel_list_finished};

/* events (wlr-foreign-toplevel-management-unstable-v1) */

static void wlr_foreign_toplevel_handle_title(
    void *data, G_GNUC_UNUSED struct zwlr_foreign_toplevel_handle_v1 *handle,
    const char *title) {
  WlrForeignToplevelHandle *self = (WlrForeignToplevelHandle *)data;
  if (self->title) {
    g_free(self->title);
  }
  self->title = g_strdup(title);
  self->title_len = g_utf8_strlen(self->title, -1);
}

static void wlr_foreign_toplevel_handle_app_id(
    void *data, G_GNUC_UNUSED struct zwlr_foreign_toplevel_handle_v1 *handle,
    const char *app_id) {
  WlrForeignToplevelHandle *self = (WlrForeignToplevelHandle *)data;
  if (self->app_id) {
    g_free(self->app_id);
  }
  self->app_id = g_strdup(app_id);
  self->app_id_len = g_utf8_strlen(self->app_id, -1);
}

static void wlr_foreign_toplevel_handle_output_enter(
    G_GNUC_UNUSED void *data,
    G_GNUC_UNUSED struct zwlr_foreign_toplevel_handle_v1 *handle,
    G_GNUC_UNUSED struct wl_output *output) {
  /* ignore */
}

static void wlr_foreign_toplevel_handle_output_leave(
    G_GNUC_UNUSED void *data,
    G_GNUC_UNUSED struct zwlr_foreign_toplevel_handle_v1 *handle,
    G_GNUC_UNUSED struct wl_output *output) {
  /* ignore */
}

static void wlr_foreign_toplevel_handle_state(
    void *data, G_GNUC_UNUSED struct zwlr_foreign_toplevel_handle_v1 *handle,
    struct wl_array *value) {
  WlrForeignToplevelHandle *self = (WlrForeignToplevelHandle *)data;
  uint32_t *elem;

  self->state = 0;
  wl_array_for_each(elem, value) { self->state |= 1 << *elem; }
}

static void wlr_foreign_toplevel_handle_done(
    void *data, G_GNUC_UNUSED struct zwlr_foreign_toplevel_handle_v1 *handle) {
  WlrForeignToplevelHandle *self = (WlrForeignToplevelHandle *)data;

  g_debug("wlr window %p id=%s title=%s state=%d", (void *)self,
          self->app_id, self->title, self->state);

  wayland_window_update_toplevel(self);
}

static void wlr_foreign_toplevel_handle_closed(
    void *data, G_GNUC_UNUSED struct zwlr_foreign_toplevel_handle_v1 *handle) {
  WlrForeignToplevelHandle *self = (WlrForeignToplevelHandle *)data;

  /* the handle is inert and will receive no further events */
  self->state = TOPLEVEL_STATE_CLOSED;
  self->view->wlr_toplevels = g_list_remove(self->view->wlr_toplevels, self);
  wayland_window_update_toplevel(self);
  wlr_foreign_toplevel_handle_free(self);
}

static void wlr_foreign_toplevel_handle_parent(
    G_GNUC_UNUSED void *data,
    G_GNUC_UNUSED struct zwlr_foreign_toplevel_handle_v1 *handle,
    G_GNUC_UNUSED struct zwlr_foreign_toplevel_handle_v1 *parent) {
  /* ignore */
}

static struct zwlr_foreign_toplevel_handle_v1_listener
    wlr_foreign_toplevel_handle_listener = {
        .title = &wlr_foreign_toplevel_handle_title,
        .app_id = &wlr_foreign_toplevel_handle_app_id,
        .output_enter = &wlr_foreign_toplevel_handle_output_enter,
        .output_leave = &wlr_foreign_toplevel_handle_output_leave,
        .state = &wlr_foreign_toplevel_handle_state,
        .done = &wlr_foreign_toplevel_handle_done,
        .closed = &wlr_foreign_toplevel_handle_closed,
        .parent = &wlr_foreign_toplevel_handle_parent};

static WlrForeignToplevelHandle *
wlr_foreign_toplevel_handle_new(struct zwlr_foreign_toplevel_handle_v1 *handle,
                                WaylandWindowModePrivateData *view) {
  WlrForeignToplevelHandle *self =
      (WlrForeignToplevelHandle *)g_malloc0(sizeof(WlrForeignToplevelHandle));

  self->handle = handle;
  self->view = view;
  zwlr_foreign_toplevel_handle_v1_add_listener(
      handle, &wlr_foreign_toplevel_handle_listener, self);
  return self;
}

static void wlr_foreign_toplevel_manager_toplevel(
    void *data, G_GNUC_UNUSED struct zwlr_foreign_toplevel_manager_v1 *manager,
    struct zwlr_foreign_toplevel_handle_v1 *toplevel) {
  WaylandWindowModePrivateData *pd = (WaylandWindowModePrivateData *)data;

  WlrForeignToplevelHandle *handle = wlr_foreign_toplevel_handle_new(toplevel, pd);
  pd->wlr_toplevels = g_list_prepend(pd->wlr_toplevels, handle);
}

static void wlr_foreign_toplevel_manager_finished(
    G_GNUC_UNUSED void *data,
    struct zwlr_foreign_toplevel_manager_v1 *manager) {
  zwlr_foreign_toplevel_manager_v1_destroy(manager);
}

static struct zwlr_foreign_toplevel_manager_v1_listener
    wlr_foreign_toplevel_manager_listener = {
        .toplevel = &wlr_foreign_toplevel_manager_toplevel,
        .finished = &wlr_foreign_toplevel_manager_finished};

static void handle_global(void *data, struct wl_registry *registry,
                          uint32_t name, const char *interface,
                          uint32_t version) {
  WaylandWindowModePrivateData *pd = (WaylandWindowModePrivateData *)data;

  if (g_strcmp0(interface, zwlr_foreign_toplevel_manager_v1_interface.name) ==
      0) {

    pd->manager = (struct zwlr_foreign_toplevel_manager_v1 *)wl_registry_bind(
        registry, name, &zwlr_foreign_toplevel_manager_v1_interface,
        MIN(version, WLR_FOREIGN_TOPLEVEL_VERSION));
  }
  else if (g_strcmp0(interface, ext_foreign_toplevel_list_v1_interface.name) ==
           0) {
    pd->list = (struct ext_foreign_toplevel_list_v1 *)wl_registry_bind(
        registry, name, &ext_foreign_toplevel_list_v1_interface,
        MIN(version, EXT_FOREIGN_TOPLEVEL_VERSION));
  }
}

static void handle_global_remove(G_GNUC_UNUSED void *data,
                                 G_GNUC_UNUSED struct wl_registry *registry,
                                 G_GNUC_UNUSED uint32_t name) {}

static struct wl_registry_listener registry_listener = {
    .global = &handle_global, .global_remove = &handle_global_remove};

static int wayland_window_mode_parse_fields(void) {
  int result = 0;
  char *savept = NULL;
  // Make a copy, as strtok will modify it.
  char *switcher_str = g_strdup(config.window_match_fields);

  const char *const sep = ",#";
  for (char *token = strtok_r(switcher_str, sep, &savept); token != NULL;
       token = strtok_r(NULL, sep, &savept)) {
    if (g_strcmp0(token, "all") == 0) {
      result |= WW_MATCH_FIELD_ALL;

    } else if (g_strcmp0(token, "title") == 0) {
      result |= WW_MATCH_FIELD_TITLE;

    } else if (g_strcmp0(token, "class") == 0 ||
               g_strcmp0(token, "app-id") == 0) {
      result |= WW_MATCH_FIELD_APP_ID;

    } else {
      g_warning("Unsupported window field name :%s. "
                "Wayland window switcher supports only 'title' and 'app-id' "
                "('class') fields",
                token);
    }
  }
  g_free(switcher_str);
  return result;
}

static void get_wayland_window(Mode *sw) {
  WaylandWindowModePrivateData *pd =
      (WaylandWindowModePrivateData *)mode_get_private_data(sw);

  pd->match_fields = wayland_window_mode_parse_fields();
  pd->window_regex = g_regex_new("{[-\\w]+(:-?[0-9]+)?}", 0, 0, NULL);

  pd->wayland = wayland;

  pd->registry = wl_display_get_registry(wayland->display);
  wl_registry_add_listener(pd->registry, &registry_listener, pd);
  wl_display_roundtrip(wayland->display);

  if (pd->manager == NULL) {
    g_warning("Unable to initialize Window mode: Wayland compositor does not "
              "support wlr-foreign-toplevel-management protocol");
    return;
  }

  if (pd->list == NULL) {
    g_warning("Window mode: Wayland compositor does not support "
              "ext-foreign-toplevel-list protocol; window-command will not work");
    /* don't need to abort, we'll check for empty identifiers when running window-command */
  } else {
    ext_foreign_toplevel_list_v1_add_listener(
        pd->list, &ext_foreign_toplevel_list_listener, pd);
  }

  zwlr_foreign_toplevel_manager_v1_add_listener(
      pd->manager, &wlr_foreign_toplevel_manager_listener, pd);
  /* fetch initial set of windows */
  wl_display_roundtrip(wayland->display);
  pd->visible = TRUE;
}

static void wlr_toplevels_list_item_free(gpointer data,
                                         G_GNUC_UNUSED gpointer user_data) {
  wlr_foreign_toplevel_handle_free((WlrForeignToplevelHandle *)data);
}

static void wayland_window_private_free(WaylandWindowModePrivateData *pd) {
  if (pd->wlr_toplevels) {
    g_list_foreach(pd->wlr_toplevels, wlr_toplevels_list_item_free, NULL);
    g_list_free(pd->wlr_toplevels);
    pd->wlr_toplevels = NULL;
  }

  if (pd->registry) {
    wl_registry_destroy(pd->registry);
    pd->registry = NULL;
  }

  if (pd->manager) {
    zwlr_foreign_toplevel_manager_v1_stop(pd->manager);
    pd->manager = NULL;
    wl_display_roundtrip(pd->wayland->display);
  }

  if (pd->window_regex) {
    g_regex_unref(pd->window_regex);
  }

  g_free(pd);
}

static int wayland_window_mode_init(Mode *sw) {
  /**
   * Called on startup when enabled (in modi list)
   */
  if (mode_get_private_data(sw) == NULL) {
    WaylandWindowModePrivateData *pd =
        (WaylandWindowModePrivateData *)g_malloc0(
            sizeof(WaylandWindowModePrivateData));
    mode_set_private_data(sw, (void *)pd);

    get_wayland_window(sw);
  }
  return TRUE;
}

static unsigned int wayland_window_mode_get_num_entries(const Mode *sw) {
  const WaylandWindowModePrivateData *pd =
      (const WaylandWindowModePrivateData *)mode_get_private_data(sw);

  g_return_val_if_fail(pd != NULL, 0);

  return g_list_length(pd->wlr_toplevels);
}

static ModeMode wayland_window_mode_result(Mode *sw, int mretv,
                                           G_GNUC_UNUSED char **input,
                                           unsigned int selected_line) {
  ModeMode retv = MODE_EXIT;
  WaylandWindowModePrivateData *pd =
      (WaylandWindowModePrivateData *)mode_get_private_data(sw);

  g_return_val_if_fail(pd != NULL, retv);

  if (mretv & MENU_NEXT) {
    retv = NEXT_DIALOG;
  } else if (mretv & MENU_PREVIOUS) {
    retv = PREVIOUS_DIALOG;
  } else if (mretv & MENU_QUICK_SWITCH) {
    retv = (ModeMode)(mretv & MENU_LOWER_MASK);
  } else if ((mretv & MENU_OK)) {
    rofi_view_hide();
    WlrForeignToplevelHandle *toplevel =
        (WlrForeignToplevelHandle *)g_list_nth_data(pd->wlr_toplevels, selected_line);
    wlr_foreign_toplevel_handle_activate(toplevel, pd->wayland->last_seat->seat);
    wl_display_flush(pd->wayland->display);

  } else if ((mretv & MENU_ENTRY_DELETE) == MENU_ENTRY_DELETE) {
    WlrForeignToplevelHandle *toplevel =
        (WlrForeignToplevelHandle *)g_list_nth_data(pd->wlr_toplevels, selected_line);
    wlr_foreign_toplevel_handle_close(toplevel);
    wl_display_flush(pd->wayland->display);
    ThemeWidget *wid = rofi_config_find_widget(sw->name, NULL, TRUE);
    Property *p =
        rofi_theme_find_property(wid, P_BOOLEAN, "close-on-delete", TRUE);
    if (p && p->type == P_BOOLEAN && p->value.b == FALSE) {
        return RELOAD_DIALOG;
    }

  } else if ((mretv & MENU_CUSTOM_INPUT) && *input != NULL &&
             *input[0] != '\0') {
    GError *error = NULL;
    gboolean run_in_term = ((mretv & MENU_CUSTOM_ACTION) == MENU_CUSTOM_ACTION);
    gsize lf_cmd_size = 0;
    gchar *lf_cmd = g_locale_from_utf8(*input, -1, NULL, &lf_cmd_size, &error);
    if (error != NULL) {
      g_warning("Failed to convert command to locale encoding: %s",
                error->message);
      g_error_free(error);
      return RELOAD_DIALOG;
    }

    RofiHelperExecuteContext context = {.name = NULL};
    if (!helper_execute_command(NULL, lf_cmd, run_in_term,
                                run_in_term ? &context : NULL)) {
      retv = RELOAD_DIALOG;
    }
    g_free(lf_cmd);
  } else if (mretv & MENU_CUSTOM_COMMAND) {
    retv = (mretv & MENU_LOWER_MASK);
  }

  return retv;
}

static void wayland_window_mode_destroy(Mode *sw) {
  WaylandWindowModePrivateData *pd =
      (WaylandWindowModePrivateData *)mode_get_private_data(sw);

  if (pd == NULL) {
    return;
  }

  wayland_window_private_free(pd);
  mode_set_private_data(sw, NULL);
}

static int wayland_window_token_match(const Mode *sw, rofi_int_matcher **tokens,
                                      unsigned int index) {
  WaylandWindowModePrivateData *pd =
      (WaylandWindowModePrivateData *)mode_get_private_data(sw);
  WlrForeignToplevelHandle *toplevel =
      (WlrForeignToplevelHandle *)g_list_nth_data(pd->wlr_toplevels, index);

  g_return_val_if_fail(toplevel != NULL, 0);

  int match = TRUE;

  if (tokens) {
    for (int j = 0; match && tokens[j] != NULL; j++) {
      int test = 0;
      /* See comment in window.c;
       * for each token we want to match at least one field.
       */
      rofi_int_matcher *ftokens[2] = {tokens[j], NULL};

      if ((pd->match_fields & WW_MATCH_FIELD_TITLE) &&
          toplevel->title != NULL && toplevel->title[0] != '\0') {
        test = helper_token_match(ftokens, toplevel->title);
      }

      if (test == tokens[j]->invert &&
          (pd->match_fields & WW_MATCH_FIELD_APP_ID) &&
          toplevel->app_id != NULL && toplevel->app_id[0] != '\0') {
        test = helper_token_match(ftokens, toplevel->app_id);
      }

      if (test == 0) {
        match = 0;
      }
    }
  }

  return match;
}

static void helper_eval_add_str(GString *str, const char *input, int len,
                                int max_len, int nc) {
  // g_utf8 does not work with NULL string.
  const char *input_nn = input ? input : "";
  // Both len and max_len are in characters, not bytes.
  int spaces = 0;
  if (len > 0) {
    if (nc > len) {
      int bl = g_utf8_offset_to_pointer(input_nn, len) - input_nn;
      char *tmp = g_markup_escape_text(input_nn, bl);
      g_string_append(str, tmp);
      g_free(tmp);
    } else {
      spaces = len - nc;
      char *tmp = g_markup_escape_text(input_nn, -1);
      g_string_append(str, tmp);
      g_free(tmp);
    }
  } else {
    char *tmp = g_markup_escape_text(input_nn, -1);
    g_string_append(str, tmp);
    g_free(tmp);
    if (len == 0) {
      spaces = MAX(0, max_len - nc);
    }
  }
  while (spaces--) {
    g_string_append_c(str, ' ');
  }
}

struct arg {
  const WaylandWindowModePrivateData *pd;
  WlrForeignToplevelHandle *toplevel;
};

static gboolean helper_eval_cb(const GMatchInfo *info, GString *str,
                               gpointer data) {
  struct arg *d = (struct arg *)data;
  gchar *match;
  // Get the match
  match = g_match_info_fetch(info, 0);
  if (match != NULL) {
    int l = 0;
    if (match[2] == ':') {
      l = (int)g_ascii_strtoll(&match[3], NULL, 10);
    }
    /* Most of the arguments are not supported on wayland */
    switch (match[1]) {
    case 't': /* title */
      helper_eval_add_str(str, d->toplevel->title, l, d->pd->title_len,
                          d->toplevel->title_len);
      break;
    case 'a': /* app_id */
    case 'c': /* class */
      helper_eval_add_str(str, d->toplevel->app_id, l, d->pd->app_id_len,
                          d->toplevel->app_id_len);
      break;
    }

    g_free(match);
  }
  return FALSE;
}

static char *_generate_display_string(const WaylandWindowModePrivateData *pd,
                                      WlrForeignToplevelHandle *toplevel) {

  struct arg d = {pd, toplevel};
  char *res = g_regex_replace_eval(pd->window_regex, config.window_format, -1,
                                   0, 0, helper_eval_cb, &d, NULL);
  return g_strchomp(res);
}

static char *_get_display_value(const Mode *sw, unsigned int selected_line,
                                int *state, G_GNUC_UNUSED GList **attr_list,
                                int get_entry) {
  WaylandWindowModePrivateData *pd =
      (WaylandWindowModePrivateData *)mode_get_private_data(sw);

  g_return_val_if_fail(pd != NULL, NULL);

  WlrForeignToplevelHandle *toplevel =
      (WlrForeignToplevelHandle *)g_list_nth_data(pd->wlr_toplevels, selected_line);

  if (toplevel == NULL || toplevel->state & TOPLEVEL_STATE_CLOSED) {
    return get_entry ? g_strdup("Window has vanished") : NULL;
  }

  /* This may not work because layer-surface holds focus */
  if (toplevel->state & TOPLEVEL_STATE_ACTIVATED) {
    *state |= ACTIVE;
  }
  *state |= MARKUP;

  return get_entry ? _generate_display_string(pd, toplevel) : NULL;
}

static cairo_surface_t *_get_icon(const Mode *sw, unsigned int selected_line,
                                  unsigned int height) {
  WaylandWindowModePrivateData *pd =
      (WaylandWindowModePrivateData *)mode_get_private_data(sw);
  const guint scale = display_scale();

  g_return_val_if_fail(pd != NULL, NULL);

  WlrForeignToplevelHandle *toplevel =
      (WlrForeignToplevelHandle *)g_list_nth_data(pd->wlr_toplevels, selected_line);

  /* some apps don't have app_id (WM_CLASS). this is fine */
  if (toplevel == NULL || toplevel->app_id == NULL ||
      toplevel->app_id[0] == '\0') {
    return NULL;
  }

  if (toplevel->cached_icon_uid > 0 && toplevel->cached_icon_size == height &&
      toplevel->cached_icon_scale == scale) {
    return rofi_icon_fetcher_get(toplevel->cached_icon_uid);
  }

  /**
   * Lookup icon by lowercase app_id.
   * There's no API to request multiple names, so we do the same as XCB window
   * mode and search for a lowercase WM_CLASS/app_id.
   */
  gchar *app_id_lower = g_utf8_strdown(toplevel->app_id, -1);
  toplevel->cached_icon_size = height;
  toplevel->cached_icon_scale = scale;
  toplevel->cached_icon_uid = rofi_icon_fetcher_query(app_id_lower, height);
  g_free(app_id_lower);

  return rofi_icon_fetcher_get(toplevel->cached_icon_uid);
}

#include "mode-private.h"

Mode wayland_window_mode = {.name = "window",
                            .cfg_name_key = "display-window",
                            ._init = wayland_window_mode_init,
                            ._destroy = wayland_window_mode_destroy,
                            ._get_num_entries =
                                wayland_window_mode_get_num_entries,
                            ._result = wayland_window_mode_result,
                            ._token_match = wayland_window_token_match,
                            ._get_display_value = _get_display_value,
                            ._get_icon = _get_icon,
                            ._get_completion = NULL,
                            ._preprocess_input = NULL,
                            ._get_message = NULL,
                            .private_data = NULL,
                            .free = NULL,
                            .type = MODE_TYPE_SWITCHER};

#endif // WINDOW_MODE
