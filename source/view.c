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

/** The Rofi View log domain */
#define G_LOG_DOMAIN "View"

#include <config.h>
#include <locale.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <glib.h>

#include "rofi.h"

#include "settings.h"
#include "timings.h"

#include "display.h"
#include "helper-theme.h"
#include "helper.h"
#include "mode.h"

#include "view-internal.h"
#include "view.h"

#include "theme.h"

#ifdef ENABLE_XCB
#include "xcb-internal.h"
#include "xcb.h"
#else
#include "xcb-dummy.h"
#endif

static const view_proxy *proxy;

void view_init(const view_proxy *view_in) { proxy = view_in; }

/** Thread pool used for filtering */
GThreadPool *tpool = NULL;

/** Global pointer to the currently active RofiViewState */
RofiViewState *current_active_menu = NULL;

struct _rofi_view_cache_state CacheState = {
    .main_window = XCB_WINDOW_NONE,
    .flags = MENU_NORMAL,
    .views = G_QUEUE_INIT,
    .refilter_timeout = 0,
    .refilter_timeout_count = 0,
    .max_refilter_time = 0.0,
    .delayed_mode = FALSE,
    .user_timeout = 0,
    .overlay_timeout = 0,
    .entry_history_enable = TRUE,
    .entry_history = NULL,
    .entry_history_length = 0,
    .entry_history_index = 0,
};

static char *get_matching_state(RofiViewState *state) {
  if (state->case_sensitive) {
    if (config.sort) {
      return "±";
    } else {
      return "-";
    }
  } else {
    if (config.sort) {
      return "+";
    }
  }
  return " ";
}

/**
 * Levenshtein Sorting.
 */
static int lev_sort(const void *p1, const void *p2, void *arg) {
  const int *a = p1;
  const int *b = p2;
  int *distances = arg;

  return distances[*a] - distances[*b];
}

static void screenshot_taken_user_callback(const char *path) {
  if (config.on_screenshot_taken == NULL)
    return;

  char **args = NULL;
  int argv = 0;
  helper_parse_setup(config.on_screenshot_taken, &args, &argv, "{path}", path,
                     (char *)0);
  if (args != NULL)
    helper_execute(NULL, args, "", config.on_screenshot_taken, NULL);
}

/**
 * Stores a screenshot of Rofi at that point in time.
 */
void rofi_capture_screenshot(void) {
  RofiViewState *state = current_active_menu;
  if (state == NULL || state->main_window == NULL) {
    g_warning("Nothing to screenshot.");
    return;
  }
  const char *outp = g_getenv("ROFI_PNG_OUTPUT");
  const char *xdg_pict_dir = g_get_user_special_dir(G_USER_DIRECTORY_PICTURES);
  if (outp == NULL && xdg_pict_dir == NULL) {
    g_warning("XDG user picture directory or ROFI_PNG_OUTPUT is not set. "
              "Cannot store screenshot.");
    return;
  }
  // Get current time.
  GDateTime *now = g_date_time_new_now_local();
  // Format filename.
  char *timestmp = g_date_time_format(now, "rofi-%Y-%m-%d-%H%M");
  char *filename = g_strdup_printf("%s-%05d.png", timestmp, 0);
  // Build full path
  char *fpath = NULL;
  if (outp == NULL) {
    int index = 0;
    fpath = g_build_filename(xdg_pict_dir, filename, NULL);
    while (g_file_test(fpath, G_FILE_TEST_EXISTS) && index < 99999) {
      g_free(fpath);
      g_free(filename);
      // Try the next index.
      index++;
      // Format filename.
      filename = g_strdup_printf("%s-%05d.png", timestmp, index);
      // Build full path
      fpath = g_build_filename(xdg_pict_dir, filename, NULL);
    }
  } else {
    fpath = g_strdup(outp);
  }
  fprintf(stderr, color_green "Storing screenshot %s\n" color_reset, fpath);
  cairo_surface_t *surf = cairo_image_surface_create(
      CAIRO_FORMAT_ARGB32, state->width, state->height);
  cairo_status_t status = cairo_surface_status(surf);
  if (status != CAIRO_STATUS_SUCCESS) {
    g_warning("Failed to produce screenshot '%s', got error: '%s'", fpath,
              cairo_status_to_string(status));
  } else {
    cairo_t *draw = cairo_create(surf);
    status = cairo_status(draw);
    if (status != CAIRO_STATUS_SUCCESS) {
      g_warning("Failed to produce screenshot '%s', got error: '%s'", fpath,
                cairo_status_to_string(status));
    } else {
      widget_draw(WIDGET(state->main_window), draw);
      status = cairo_surface_write_to_png(surf, fpath);
      if (status != CAIRO_STATUS_SUCCESS) {
        g_warning("Failed to produce screenshot '%s', got error: '%s'", fpath,
                  cairo_status_to_string(status));
      }
      screenshot_taken_user_callback(fpath);
    }
    cairo_destroy(draw);
  }
  // Cleanup
  cairo_surface_destroy(surf);
  g_free(fpath);
  g_free(filename);
  g_free(timestmp);
  g_date_time_unref(now);
}

static void rofi_view_update_prompt(RofiViewState *state) {
  if (state->prompt) {
    const char *str = mode_get_display_name(state->sw);
    textbox_text(state->prompt, str);
  }
}

extern GList *list_of_warning_msgs;
static void rofi_view_reload_message_bar(RofiViewState *state) {
  if (state->mesg_box == NULL) {
    return;
  }
  char *msg = mode_get_message(state->sw);
  if (msg || list_of_warning_msgs) {
    /** we want to popin warning here. */

    GString *emesg = g_string_new(msg);
    if (list_of_warning_msgs) {
      if (msg) {
        g_string_append_c(emesg, '\n');
      }
      g_string_append(
          emesg, "The following warnings were detected when starting rofi:\n");
      GList *iter = g_list_first(list_of_warning_msgs);
      int index = 0;
      for (; iter != NULL && index < 2; iter = g_list_next(iter)) {
        GString *in_msg = (GString *)(iter->data);
        g_string_append(emesg, "\n\n");
        g_string_append(emesg, in_msg->str);
        index++;
      }
      if (g_list_length(iter) > 1) {
        g_string_append_printf(emesg, "\nThere are <b>%u</b> more errors.",
                               g_list_length(iter) - 1);
      }
    }
    textbox_text(state->mesg_tb, emesg->str);
    widget_enable(WIDGET(state->mesg_box));
    g_string_free(emesg, TRUE);
    g_free(msg);
  } else {
    widget_disable(WIDGET(state->mesg_box));
  }
}

static void rofi_view_take_action(const char *name) {
  ThemeWidget *wid = rofi_config_find_widget(name, NULL, TRUE);
  if (wid) {
    /** Check string property */
    Property *p = rofi_theme_find_property(wid, P_STRING, "action", TRUE);
    if (p != NULL && p->type == P_STRING) {
      const char *action = p->value.s;
      guint id = key_binding_get_action_from_name(action);
      if (id != UINT32_MAX) {
        rofi_view_trigger_action(rofi_view_get_active(), SCOPE_GLOBAL, id);
      } else {
        g_warning("Failed to parse keybinding: %s\r\n", action);
      }
    }
  }
}
static gboolean rofi_view_user_timeout(G_GNUC_UNUSED gpointer data) {
  CacheState.user_timeout = 0;
  rofi_view_take_action("timeout");
  return G_SOURCE_REMOVE;
}

static void rofi_view_set_user_timeout(G_GNUC_UNUSED gpointer data) {
  if (CacheState.user_timeout > 0) {
    g_source_remove(CacheState.user_timeout);
    CacheState.user_timeout = 0;
  }
  {
    /** Find the widget */
    ThemeWidget *wid = rofi_config_find_widget("timeout", NULL, TRUE);
    if (wid) {
      /** Check string property */
      Property *p = rofi_theme_find_property(wid, P_INTEGER, "delay", TRUE);
      if (p != NULL && p->type == P_INTEGER && p->value.i > 0) {
        int delay = p->value.i;
        CacheState.user_timeout =
            g_timeout_add(delay * 1000, rofi_view_user_timeout, NULL);
      } else {
        Property *prop = rofi_theme_find_property(wid, P_DOUBLE, "delay", TRUE);
        if (prop != NULL && prop->type == P_DOUBLE && prop->value.f > 0.01) {
          double delay = prop->value.f;
          CacheState.user_timeout =
              g_timeout_add(delay * 1000, rofi_view_user_timeout, NULL);
        }
      }
    }
  }
}

void rofi_view_restart(RofiViewState *state) {
  state->quit = FALSE;
  state->retv = MENU_CANCEL;
}

RofiViewState *rofi_view_get_active(void) { return current_active_menu; }

textbox *rofi_view_get_active_text(void) {
  RofiViewState *state = rofi_view_get_active();
  if (state) {
    return state->text;
  }
  return NULL;
}

void rofi_view_remove_active(RofiViewState *state) {
  if (state == current_active_menu) {
    rofi_view_set_active(NULL);
  } else if (state) {
    g_queue_remove(&(CacheState.views), state);
  }
}
void rofi_view_set_active(RofiViewState *state) {
  if (current_active_menu != NULL && state != NULL) {
    g_queue_push_head(&(CacheState.views), current_active_menu);
    // TODO check.
    current_active_menu = state;
    g_debug("stack view.");
    rofi_view_window_update_size(current_active_menu);
    rofi_view_queue_redraw();
    return;
  } else if (state == NULL && !g_queue_is_empty(&(CacheState.views))) {
    g_debug("pop view.");
    current_active_menu = g_queue_pop_head(&(CacheState.views));
    rofi_view_window_update_size(current_active_menu);
    rofi_view_queue_redraw();
    return;
  }
  g_assert((current_active_menu == NULL && state != NULL) ||
           (current_active_menu != NULL && state == NULL));
  current_active_menu = state;
  rofi_view_queue_redraw();
}

void rofi_view_set_selected_line(RofiViewState *state,
                                 unsigned int selected_line) {
  state->selected_line = selected_line;
  // Find the line.
  unsigned int selected = 0;
  for (unsigned int i = 0; ((state->selected_line)) < UINT32_MAX && !selected &&
                           i < state->filtered_lines;
       i++) {
    if (state->line_map[i] == (state->selected_line)) {
      selected = i;
      break;
    }
  }
  listview_set_selected(state->list_view, selected);
#ifdef ENABLE_XCB
  if (config.backend == DISPLAY_XCB) {
    // Clear the window and force an expose event resulting in a redraw.
    xcb_clear_area(xcb->connection, 1, CacheState.main_window, 0, 0, 1, 1);
    xcb_flush(xcb->connection);
  }
#endif
}

void rofi_view_free(RofiViewState *state) {
  if (state->tokens) {
    helper_tokenize_free(state->tokens);
    state->tokens = NULL;
  }
  // Do this here?
  // Wait for final release?
  widget_free(WIDGET(state->main_window));

  g_free(state->line_map);
  g_free(state->distance);
  // Free the switcher boxes.
  // When state is free'ed we should no longer need these.
  g_free(state->modes);
  state->num_modes = 0;
  g_free(state);
}

MenuReturn rofi_view_get_return_value(const RofiViewState *state) {
  return state->retv;
}

unsigned int rofi_view_get_selected_line(const RofiViewState *state) {
  return state->selected_line;
}

unsigned int rofi_view_get_next_position(const RofiViewState *state) {
  unsigned int next_pos = state->selected_line;
  unsigned int selected = listview_get_selected(state->list_view);
  if ((selected + 1) < state->num_lines) {
    (next_pos) = state->line_map[selected + 1];
  }
  return next_pos;
}

unsigned int rofi_view_get_completed(const RofiViewState *state) {
  return state->quit;
}

const char *rofi_view_get_user_input(const RofiViewState *state) {
  if (state->text) {
    return state->text->text;
  }
  return NULL;
}

/**
 * Create a new, 0 initialized RofiViewState structure.
 *
 * @returns a new 0 initialized RofiViewState
 */
static RofiViewState *__rofi_view_state_create(void) {
  return g_malloc0(sizeof(RofiViewState));
}

/**
 * Thread state for workers started for the view.
 */
typedef struct _thread_state_view {
  /** Generic thread state. */
  thread_state st;

  /** Condition. */
  GCond *cond;
  /** Lock for condition. */
  GMutex *mutex;
  /** Count that is protected by lock. */
  unsigned int *acount;

  /** Current state. */
  RofiViewState *state;
  /** Start row for this worker. */
  unsigned int start;
  /** Stop row for this worker. */
  unsigned int stop;
  /** Rows processed. */
  unsigned int count;

  /** Pattern input to filter. */
  const char *pattern;
  /** Length of pattern. */
  glong plen;
} thread_state_view;
/**
 * @param data A thread_state object.
 * @param user_data User data to pass to thread_state callback
 *
 * Small wrapper function that is internally used to pass a job to a worker.
 */
static void rofi_view_call_thread(gpointer data, gpointer user_data) {
  thread_state *t = (thread_state *)data;
  t->callback(t, user_data);
}

static void filter_elements(thread_state *ts,
                            G_GNUC_UNUSED gpointer user_data) {
  thread_state_view *t = (thread_state_view *)ts;
  for (unsigned int i = t->start; i < t->stop; i++) {
    int match = mode_token_match(t->state->sw, t->state->tokens, i);
    // If each token was matched, add it to list.
    if (match) {
      t->state->line_map[t->start + t->count] = i;
      if (config.sort) {
        // This is inefficient, need to fix it.
        char *str = mode_get_completion(t->state->sw, i);
        glong slen = g_utf8_strlen(str, -1);
        switch (config.sorting_method_enum) {
        case SORT_FZF:
          t->state->distance[i] = rofi_scorer_fuzzy_evaluate(
              t->pattern, t->plen, str, slen, t->state->case_sensitive);
          break;
        case SORT_NORMAL:
        default:
          t->state->distance[i] = levenshtein(t->pattern, t->plen, str, slen,
                                              t->state->case_sensitive);
          break;
        }
        g_free(str);
      }
      t->count++;
    }
  }
  if (t->acount != NULL) {
    g_mutex_lock(t->mutex);
    (*(t->acount))--;
    g_cond_signal(t->cond);
    g_mutex_unlock(t->mutex);
  }
}

void input_history_initialize(void) {
  if (CacheState.entry_history_enable == FALSE) {
    return;
  }
  CacheState.entry_history = NULL;
  CacheState.entry_history_index = 0;
  CacheState.entry_history_length = 0;

  gchar *path = g_build_filename(cache_dir, "rofi-entry-history.txt", NULL);
  if (g_file_test(path, G_FILE_TEST_EXISTS)) {
    FILE *fp = fopen(path, "r");
    if (fp) {
      char *line = NULL;
      size_t len = 0;
      ssize_t nread;
      while ((nread = getline(&line, &len, fp)) != -1) {
        CacheState.entry_history = g_realloc(
            CacheState.entry_history,
            sizeof(EntryHistoryIndex) * (CacheState.entry_history_length + 1));
        if (line[nread - 1] == '\n') {
          line[nread - 1] = '\0';
          nread--;
        }
        CacheState.entry_history[CacheState.entry_history_length].string =
            g_strdup(line);
        CacheState.entry_history[CacheState.entry_history_length].index =
            strlen(line);
        CacheState.entry_history_length++;
        CacheState.entry_history_index++;
      }
      free(line);
      fclose(fp);
    }
  }
  g_free(path);
  CacheState.entry_history = g_realloc(
      CacheState.entry_history,
      sizeof(EntryHistoryIndex) * (CacheState.entry_history_length + 1));
  CacheState.entry_history[CacheState.entry_history_length].string =
      g_strdup("");
  CacheState.entry_history[CacheState.entry_history_length].index = 0;
  CacheState.entry_history_length++;
}
void input_history_save(void) {
  if (CacheState.entry_history_enable == FALSE) {
    return;
  }
  if (CacheState.entry_history_length > 0) {
    // History max.
    int max_history = 20;
    ThemeWidget *wid = rofi_config_find_widget("entry", NULL, TRUE);
    if (wid) {
      Property *p =
          rofi_theme_find_property(wid, P_INTEGER, "max-history", TRUE);
      if (p != NULL && p->type == P_INTEGER) {
        max_history = p->value.i;
      }
    }
    gchar *path = g_build_filename(cache_dir, "rofi-entry-history.txt", NULL);
    g_debug("Entry filename output: '%s'", path);
    FILE *fp = fopen(path, "w");
    if (fp) {
      gssize start = MAX(0, (CacheState.entry_history_length - max_history));
      for (gssize i = start; i < CacheState.entry_history_length; i++) {
        if (strlen(CacheState.entry_history[i].string) > 0) {
          fprintf(fp, "%s\n", CacheState.entry_history[i].string);
        }
      }
      fclose(fp);
    }
    g_free(path);
  }
  // Cleanups.
  if (CacheState.entry_history != NULL) {
    for (ssize_t i = 0; i < CacheState.entry_history_length; i++) {
      g_free(CacheState.entry_history[i].string);
    }
    g_free(CacheState.entry_history);
    CacheState.entry_history = NULL;
    CacheState.entry_history_length = 0;
    CacheState.entry_history_index = 0;
  }
}

/**
 * Nav helper functions, to avoid duplicate code.
 */

/**
 * @param state The current RofiViewState
 *
 * Tab handling.
 */
static void rofi_view_nav_row_tab(RofiViewState *state) {
  if (state->filtered_lines == 1) {
    state->retv = MENU_OK;
    (state->selected_line) =
        state->line_map[listview_get_selected(state->list_view)];
    state->quit = 1;
    return;
  }

  // Double tab!
  if (state->filtered_lines == 0 && ROW_TAB == state->prev_action) {
    state->retv = MENU_NEXT;
    (state->selected_line) = 0;
    state->quit = TRUE;
  } else {
    listview_nav_down(state->list_view);
  }
  state->prev_action = ROW_TAB;
}
/**
 * @param state The current RofiViewState
 *
 * complete current row.
 */
inline static void rofi_view_nav_row_select(RofiViewState *state) {
  if (state->list_view == NULL) {
    return;
  }
  unsigned int selected = listview_get_selected(state->list_view);
  // If a valid item is selected, return that..
  if (selected < state->filtered_lines) {
    char *str = mode_get_completion(state->sw, state->line_map[selected]);
    textbox_text(state->text, str);
    g_free(str);
    textbox_keybinding(state->text, MOVE_END);
    state->refilter = TRUE;
  }
}

/**
 * @param state The current RofiViewState
 *
 * Move the selection to first row.
 */
inline static void rofi_view_nav_first(RofiViewState *state) {
  //    state->selected = 0;
  listview_set_selected(state->list_view, 0);
}

/**
 * @param state The current RofiViewState
 *
 * Move the selection to last row.
 */
inline static void rofi_view_nav_last(RofiViewState *state) {
  // If no lines, do nothing.
  if (state->filtered_lines == 0) {
    return;
  }
  // state->selected = state->filtered_lines - 1;
  listview_set_selected(state->list_view, -1);
}
static void selection_changed_user_callback(unsigned int index,
                                            RofiViewState *state) {
  if (config.on_selection_changed == NULL)
    return;

  int fstate = 0;
  char *text = mode_get_display_value(state->sw, state->line_map[index],
                                      &fstate, NULL, TRUE);
  char **args = NULL;
  int argv = 0;
  helper_parse_setup(config.on_selection_changed, &args, &argv, "{entry}", text,
                     (char *)0);
  if (args != NULL)
    helper_execute(NULL, args, "", config.on_selection_changed, NULL);
  g_free(text);
}
static void selection_changed_callback(G_GNUC_UNUSED listview *lv,
                                       unsigned int index, void *udata) {
  RofiViewState *state = (RofiViewState *)udata;
  if (index < state->filtered_lines) {
    if (state->previous_line != state->line_map[index]) {
      selection_changed_user_callback(index, state);
      state->previous_line = state->line_map[index];
    }
  }
  if (state->tb_current_entry) {
    if (index < state->filtered_lines) {
      int fstate = 0;
      char *text = mode_get_display_value(state->sw, state->line_map[index],
                                          &fstate, NULL, TRUE);
      textbox_text(state->tb_current_entry, text);
      g_free(text);
    } else {
      textbox_text(state->tb_current_entry, "");
    }
  }
  if (state->icon_current_entry) {
    if (index < state->filtered_lines) {
      int icon_height =
          widget_get_desired_height(WIDGET(state->icon_current_entry),
                                    WIDGET(state->icon_current_entry)->w);
      cairo_surface_t *surf_icon =
          mode_get_icon(state->sw, state->line_map[index], icon_height);
      icon_set_surface(state->icon_current_entry, surf_icon);
    } else {
      icon_set_surface(state->icon_current_entry, NULL);
    }
  }
}
static void update_callback(textbox *t, icon *ico, unsigned int index,
                            void *udata, TextBoxFontType *type, gboolean full) {
  RofiViewState *state = (RofiViewState *)udata;
  if (full) {
    GList *add_list = NULL;
    int fstate = 0;
    char *text = mode_get_display_value(state->sw, state->line_map[index],
                                        &fstate, &add_list, TRUE);
    (*type) |= fstate;

    if (ico) {
      int icon_height = widget_get_desired_height(WIDGET(ico), WIDGET(ico)->w);
      cairo_surface_t *surf_icon =
          mode_get_icon(state->sw, state->line_map[index], icon_height);
      icon_set_surface(ico, surf_icon);
    }
    if (t) {
      // TODO needed for markup.
      textbox_font(t, *type);
      // Move into list view.
      textbox_text(t, text);
      PangoAttrList *list = textbox_get_pango_attributes(t);
      if (list != NULL) {
        pango_attr_list_ref(list);
      } else {
        list = pango_attr_list_new();
      }

      if (state->tokens) {
        RofiHighlightColorStyle th = {ROFI_HL_BOLD | ROFI_HL_UNDERLINE,
                                      {0.0, 0.0, 0.0, 0.0}};
        th = rofi_theme_get_highlight(WIDGET(t), "highlight", th);
        helper_token_match_get_pango_attr(th, state->tokens,
                                          textbox_get_visible_text(t), list);
      }
      for (GList *iter = g_list_first(add_list); iter != NULL;
           iter = g_list_next(iter)) {
        pango_attr_list_insert(list, (PangoAttribute *)(iter->data));
      }
      textbox_set_pango_attributes(t, list);
      pango_attr_list_unref(list);
    }

    g_list_free(add_list);
    g_free(text);
  } else {
    // Never called.
    int fstate = 0;
    mode_get_display_value(state->sw, state->line_map[index], &fstate, NULL,
                           FALSE);
    (*type) |= fstate;
    // TODO needed for markup.
    textbox_font(t, *type);
  }
}
static void page_changed_callback(void) {
  rofi_view_workers_finalize();
  rofi_view_workers_initialize();
}

static void _rofi_view_reload_row(RofiViewState *state) {
  g_free(state->line_map);
  g_free(state->distance);
  state->num_lines = mode_get_num_entries(state->sw);
  state->line_map = g_malloc0_n(state->num_lines, sizeof(unsigned int));
  state->distance = g_malloc0_n(state->num_lines, sizeof(int));
  listview_set_max_lines(state->list_view, state->num_lines);
  rofi_view_reload_message_bar(state);
}

static gboolean rofi_view_refilter_real(RofiViewState *state) {
  CacheState.refilter_timeout = 0;
  CacheState.refilter_timeout_count = 0;
  if (state->sw == NULL) {
    return G_SOURCE_REMOVE;
  }
  GTimer *timer = g_timer_new();
  TICK_N("Filter start");
  if (state->reload) {
    _rofi_view_reload_row(state);
    state->reload = FALSE;
  }
  TICK_N("Filter reload rows");
  if (state->tokens) {
    helper_tokenize_free(state->tokens);
    state->tokens = NULL;
  }
  TICK_N("Filter tokenize");
  if (state->text && strlen(state->text->text) > 0) {

    listview_set_filtered(state->list_view, TRUE);
    unsigned int j = 0;
    gchar *pattern = mode_preprocess_input(state->sw, state->text->text);
    glong plen = pattern ? g_utf8_strlen(pattern, -1) : 0;
    state->case_sensitive = parse_case_sensitivity(state->text->text);
    state->tokens = helper_tokenize(pattern, state->case_sensitive);

    if (config.case_smart && state->case_indicator) {
      textbox_text(state->case_indicator, get_matching_state(state));
    }
    /**
     * On long lists it can be beneficial to parallelize.
     * If number of threads is 1, no thread is spawn.
     * If number of threads > 1 and there are enough (> 1000) items, spawn jobs
     * for the thread pool. For large lists with 8 threads I see a factor three
     * speedup of the whole function.
     */
    unsigned int nt = MAX(1, state->num_lines / 500);
    // Limit the number of jobs, it could cause stack overflow if we don´t
    // limit.
    nt = MIN(nt, config.threads * 4);
    thread_state_view states[nt];
    GCond cond;
    GMutex mutex;
    g_mutex_init(&mutex);
    g_cond_init(&cond);
    unsigned int count = nt;
    unsigned int steps = (state->num_lines + nt) / nt;
    for (unsigned int i = 0; i < nt; i++) {
      states[i].state = state;
      states[i].start = i * steps;
      states[i].stop = MIN(state->num_lines, (i + 1) * steps);
      states[i].count = 0;
      states[i].cond = &cond;
      states[i].mutex = &mutex;
      states[i].acount = &count;
      states[i].plen = plen;
      states[i].pattern = pattern;
      states[i].st.callback = filter_elements;
      states[i].st.free = NULL;
      states[i].st.priority = G_PRIORITY_HIGH;
      if (i > 0) {
        g_thread_pool_push(tpool, &states[i], NULL);
      }
    }
    // Run one in this thread.
    rofi_view_call_thread(&states[0], NULL);
    // No need to do this with only one thread.
    if (nt > 1) {
      g_mutex_lock(&mutex);
      while (count > 0) {
        g_cond_wait(&cond, &mutex);
      }
      g_mutex_unlock(&mutex);
    }
    g_cond_clear(&cond);
    g_mutex_clear(&mutex);
    for (unsigned int i = 0; i < nt; i++) {
      if (j != states[i].start) {
        memmove(&(state->line_map[j]), &(state->line_map[states[i].start]),
                sizeof(unsigned int) * (states[i].count));
      }
      j += states[i].count;
    }
    if (config.sort) {
      g_qsort_with_data(state->line_map, j, sizeof(int), lev_sort,
                        state->distance);
    }

    // Cleanup + bookkeeping.
    state->filtered_lines = j;
    g_free(pattern);

    double elapsed = g_timer_elapsed(timer, NULL);

    CacheState.max_refilter_time = elapsed;
  } else {
    listview_set_filtered(state->list_view, FALSE);
    for (unsigned int i = 0; i < state->num_lines; i++) {
      state->line_map[i] = i;
    }
    state->filtered_lines = state->num_lines;
  }
  TICK_N("Filter matching done");
  listview_set_num_elements(state->list_view, state->filtered_lines);

  if (state->tb_filtered_rows) {
    char *r = g_strdup_printf("%u", state->filtered_lines);
    textbox_text(state->tb_filtered_rows, r);
    g_free(r);
  }
  if (state->tb_total_rows) {
    char *r = g_strdup_printf("%u", state->num_lines);
    textbox_text(state->tb_total_rows, r);
    g_free(r);
  }
  TICK_N("Update filter lines");

  if (config.auto_select == TRUE && state->filtered_lines == 1 &&
      state->num_lines > 1) {
    (state->selected_line) =
        state->line_map[listview_get_selected(state->list_view)];
    state->retv = MENU_OK;
    state->quit = TRUE;
  }

  // Size the window.
  int height = rofi_view_calculate_window_height(state);
  if (height != state->height) {
    state->height = height;
    rofi_view_calculate_window_position(state);
    rofi_view_window_update_size(state);
    g_debug("Resize based on re-filter");
  }
  TICK_N("Filter resize window based on window ");
  state->refilter = FALSE;
  TICK_N("Filter done");
  rofi_view_update(state, TRUE);

  g_timer_destroy(timer);
  return G_SOURCE_REMOVE;
}
void rofi_view_refilter(RofiViewState *state) {
  CacheState.refilter_timeout_count++;
  if (CacheState.refilter_timeout != 0) {

    g_source_remove(CacheState.refilter_timeout);
    CacheState.refilter_timeout = 0;
  }
  if (CacheState.max_refilter_time > (config.refilter_timeout_limit / 1000.0) &&
      state->text && strlen(state->text->text) > 0 &&
      CacheState.refilter_timeout_count < 25) {
    if (CacheState.delayed_mode == FALSE) {
      g_warning(
          "Filtering took %f seconds ( %f ), switching to delayed filter\n",
          CacheState.max_refilter_time, config.refilter_timeout_limit / 1000.0);
      CacheState.delayed_mode = TRUE;
    }
    CacheState.refilter_timeout =
        g_timeout_add(200, (GSourceFunc)rofi_view_refilter_real, state);
  } else {
    if (CacheState.delayed_mode == TRUE && state->text &&
        strlen(state->text->text) > 0 &&
        CacheState.refilter_timeout_count < 25) {
      g_warning(
          "Filtering took %f seconds , switching back to instant filter\n",
          CacheState.max_refilter_time);
      CacheState.delayed_mode = FALSE;
    }
    rofi_view_refilter_real(state);
  }
}
static void rofi_view_refilter_force(RofiViewState *state) {
  if (CacheState.refilter_timeout != 0) {
    g_source_remove(CacheState.refilter_timeout);
    CacheState.refilter_timeout = 0;
  }
  if (state->refilter) {
    rofi_view_refilter_real(state);
  }
}
/**
 * @param state The Menu Handle
 *
 * Check if a finalize function is set, and if sets executes it.
 */
void process_result(RofiViewState *state);
void rofi_view_finalize(RofiViewState *state) {
  if (state && state->finalize != NULL) {
    state->finalize(state);
  }
}

/**
 * This function should be called when the input of the entry is changed.
 * TODO: Evaluate if this needs to be a 'signal' on textbox?
 */
static void rofi_view_input_changed(void) {
  rofi_view_take_action("inputchange");

  RofiViewState *state = current_active_menu;
  if (CacheState.entry_history_enable && state) {
    if (CacheState.entry_history[CacheState.entry_history_index].string !=
        NULL) {
      g_free(CacheState.entry_history[CacheState.entry_history_index].string);
    }
    CacheState.entry_history[CacheState.entry_history_index].string =
        textbox_get_text(state->text);
    CacheState.entry_history[CacheState.entry_history_index].index =
        textbox_get_cursor(state->text);
  }
}

#ifdef ENABLE_WAYLAND
static void rofi_view_clipboard_callback(char *clipboard_data,
                                         G_GNUC_UNUSED void *user_data) {
  RofiViewState *state = rofi_view_get_active();
  if (clipboard_data != NULL) {
    if (state != NULL) {
      rofi_view_handle_text(state, clipboard_data);
    }
    g_free(clipboard_data);
  }
}
#endif

static void rofi_view_trigger_global_action(KeyBindingAction action) {
  RofiViewState *state = rofi_view_get_active();
  switch (action) {
  // Handling of paste
  case PASTE_PRIMARY:
#ifdef ENABLE_XCB
    if (config.backend == DISPLAY_XCB) {
      xcb_convert_selection(xcb->connection, CacheState.main_window,
                            XCB_ATOM_PRIMARY, xcb->ewmh.UTF8_STRING,
                            xcb->ewmh.UTF8_STRING, XCB_CURRENT_TIME);
      xcb_flush(xcb->connection);
    }
#endif
#ifdef ENABLE_WAYLAND
    if (config.backend == DISPLAY_WAYLAND) {
      display_get_clipboard_data(CLIPBOARD_PRIMARY,
                                 rofi_view_clipboard_callback, NULL);
    }
#endif
    break;
  case PASTE_SECONDARY:
#ifdef ENABLE_XCB
    if (config.backend == DISPLAY_XCB) {
      xcb_convert_selection(xcb->connection, CacheState.main_window,
                            netatoms[CLIPBOARD], xcb->ewmh.UTF8_STRING,
                            xcb->ewmh.UTF8_STRING, XCB_CURRENT_TIME);
      xcb_flush(xcb->connection);
    }
#endif
#ifdef ENABLE_WAYLAND
    if (config.backend == DISPLAY_WAYLAND) {
      display_get_clipboard_data(CLIPBOARD_DEFAULT,
                                 rofi_view_clipboard_callback, NULL);
    }
#endif
    break;
  case COPY_SECONDARY: {
    char *data = NULL;
    unsigned int selected = listview_get_selected(state->list_view);
    if (selected < state->filtered_lines) {
      data = mode_get_completion(state->sw, state->line_map[selected]);
    } else if (state->text && state->text->text) {
      data = g_strdup(state->text->text);
    }
    if (data) {
#ifdef ENABLE_XCB
      if (config.backend == DISPLAY_XCB) {
        xcb_stuff_set_clipboard(data);
        xcb_set_selection_owner(xcb->connection, CacheState.main_window,
                                netatoms[CLIPBOARD], XCB_CURRENT_TIME);
        xcb_flush(xcb->connection);
      }
#endif
#ifdef ENABLE_WAYLAND
      if (config.backend == DISPLAY_WAYLAND) {
        // TODO
      }
#endif
    }
  } break;
  case SCREENSHOT:
    rofi_capture_screenshot();
    break;
  case CHANGE_ELLIPSIZE:
    if (state->list_view) {
      listview_toggle_ellipsizing(state->list_view);
    }
    break;
  case TOGGLE_SORT:
    if (state->case_indicator != NULL) {
      config.sort = !config.sort;
      state->refilter = TRUE;
      textbox_text(state->case_indicator, get_matching_state(state));
    }
    break;
  case MODE_PREVIOUS:
    state->retv = MENU_PREVIOUS;
    (state->selected_line) = 0;
    state->quit = TRUE;
    break;
  // Menu navigation.
  case MODE_NEXT:
    state->retv = MENU_NEXT;
    (state->selected_line) = 0;
    state->quit = TRUE;
    break;
  case MODE_COMPLETE: {
    unsigned int selected = listview_get_selected(state->list_view);
    state->selected_line = UINT32_MAX;
    if (selected < state->filtered_lines) {
      state->selected_line = state->line_map[selected];
    }
    state->retv = MENU_COMPLETE;
    state->quit = TRUE;
    break;
  }
  // Toggle case sensitivity.
  case TOGGLE_CASE_SENSITIVITY:
    if (state->case_indicator != NULL) {
      config.case_sensitive = !config.case_sensitive;
      (state->selected_line) = 0;
      state->refilter = TRUE;
      textbox_text(state->case_indicator, get_matching_state(state));
    }
    break;
  // Special delete entry command.
  case DELETE_ENTRY: {
    unsigned int selected = listview_get_selected(state->list_view);
    if (selected < state->filtered_lines) {
      (state->selected_line) = state->line_map[selected];
      state->retv = MENU_ENTRY_DELETE;
      state->quit = TRUE;
    }
    break;
  }
  case SELECT_ELEMENT_1:
  case SELECT_ELEMENT_2:
  case SELECT_ELEMENT_3:
  case SELECT_ELEMENT_4:
  case SELECT_ELEMENT_5:
  case SELECT_ELEMENT_6:
  case SELECT_ELEMENT_7:
  case SELECT_ELEMENT_8:
  case SELECT_ELEMENT_9:
  case SELECT_ELEMENT_10: {
    unsigned int index = action - SELECT_ELEMENT_1;
    if (index < state->filtered_lines) {
      state->selected_line = state->line_map[index];
      state->retv = MENU_OK;
      state->quit = TRUE;
    }
    break;
  }
  case CUSTOM_1:
  case CUSTOM_2:
  case CUSTOM_3:
  case CUSTOM_4:
  case CUSTOM_5:
  case CUSTOM_6:
  case CUSTOM_7:
  case CUSTOM_8:
  case CUSTOM_9:
  case CUSTOM_10:
  case CUSTOM_11:
  case CUSTOM_12:
  case CUSTOM_13:
  case CUSTOM_14:
  case CUSTOM_15:
  case CUSTOM_16:
  case CUSTOM_17:
  case CUSTOM_18:
  case CUSTOM_19: {
    state->selected_line = UINT32_MAX;
    unsigned int selected = listview_get_selected(state->list_view);
    if (selected < state->filtered_lines) {
      (state->selected_line) = state->line_map[selected];
    }
    state->retv = MENU_CUSTOM_COMMAND | ((action - CUSTOM_1) & MENU_LOWER_MASK);
    state->quit = TRUE;
    break;
  }
  // If you add a binding here, make sure to add it to
  // rofi_view_keyboard_navigation too
  case CANCEL:
    state->retv = MENU_CANCEL;
    state->quit = TRUE;
    break;
  case ELEMENT_NEXT:
    listview_nav_next(state->list_view);
    break;
  case ELEMENT_PREV:
    listview_nav_prev(state->list_view);
    break;
  case ROW_UP:
    listview_nav_up(state->list_view);
    break;
  case ROW_TAB:
    rofi_view_nav_row_tab(state);
    break;
  case ROW_DOWN:
    listview_nav_down(state->list_view);
    break;
  case ROW_LEFT:
    listview_nav_left(state->list_view);
    break;
  case ROW_RIGHT:
    listview_nav_right(state->list_view);
    break;
  case PAGE_PREV:
    listview_nav_page_prev(state->list_view);
    break;
  case PAGE_NEXT:
    listview_nav_page_next(state->list_view);
    break;
  case ROW_FIRST:
    rofi_view_nav_first(state);
    break;
  case ROW_LAST:
    rofi_view_nav_last(state);
    break;
  case ROW_SELECT:
    rofi_view_nav_row_select(state);
    break;
  // If you add a binding here, make sure to add it to textbox_keybinding too
  case MOVE_CHAR_BACK: {
    if (textbox_keybinding(state->text, action) == 0) {
      listview_nav_left(state->list_view);
    }
    break;
  }
  case MOVE_CHAR_FORWARD: {
    if (textbox_keybinding(state->text, action) == 0) {
      listview_nav_right(state->list_view);
    }
    break;
  }
  case CLEAR_LINE:
  case MOVE_FRONT:
  case MOVE_END:
  case REMOVE_TO_EOL:
  case REMOVE_TO_SOL:
  case TRANSPOSE_CHARS:
  case REMOVE_WORD_BACK:
  case REMOVE_WORD_FORWARD:
  case REMOVE_CHAR_FORWARD:
  case MOVE_WORD_BACK:
  case MOVE_WORD_FORWARD:
  case REMOVE_CHAR_BACK: {
    int rc = textbox_keybinding(state->text, action);
    if (rc == 1) {
      // Entry changed.
      state->refilter = TRUE;
      rofi_view_input_changed();
    } else if (rc == 2) {
      // Movement.
    }
    break;
  }
  case ACCEPT_ALT: {
    rofi_view_refilter_force(state);
    unsigned int selected = listview_get_selected(state->list_view);
    state->selected_line = UINT32_MAX;
    if (selected < state->filtered_lines) {
      (state->selected_line) = state->line_map[selected];
      state->retv = MENU_OK;
    } else {
      // Nothing entered and nothing selected.
      state->retv = MENU_CUSTOM_INPUT;
    }
    state->retv |= MENU_CUSTOM_ACTION;
    state->quit = TRUE;
    break;
  }
  case ACCEPT_CUSTOM: {
    rofi_view_refilter_force(state);
    state->selected_line = UINT32_MAX;
    state->retv = MENU_CUSTOM_INPUT;
    state->quit = TRUE;
    break;
  }
  case ACCEPT_CUSTOM_ALT: {
    rofi_view_refilter_force(state);
    state->selected_line = UINT32_MAX;
    state->retv = MENU_CUSTOM_INPUT | MENU_CUSTOM_ACTION;
    state->quit = TRUE;
    break;
  }
  case ACCEPT_ENTRY: {
    rofi_view_refilter_force(state);
    // If a valid item is selected, return that..
    unsigned int selected = listview_get_selected(state->list_view);
    state->selected_line = UINT32_MAX;
    if (selected < state->filtered_lines) {
      (state->selected_line) = state->line_map[selected];
      state->retv = MENU_OK;
    } else {
      // Nothing entered and nothing selected.
      state->retv = MENU_CUSTOM_INPUT;
    }
    state->quit = TRUE;
    break;
  }
  case ENTRY_HISTORY_DOWN: {
    if (CacheState.entry_history_enable && state->text) {
      CacheState.entry_history[CacheState.entry_history_index].index =
          textbox_get_cursor(state->text);
      if (CacheState.entry_history_index > 0) {
        CacheState.entry_history_index--;
      }
      if (state->text) {
        textbox_text(
            state->text,
            CacheState.entry_history[CacheState.entry_history_index].string);
        textbox_cursor(
            state->text,
            CacheState.entry_history[CacheState.entry_history_index].index);
        state->refilter = TRUE;
      }
    }
    break;
  }
  case ENTRY_HISTORY_UP: {
    if (CacheState.entry_history_enable && state->text) {
      if (CacheState.entry_history[CacheState.entry_history_index].string !=
          NULL) {
        g_free(CacheState.entry_history[CacheState.entry_history_index].string);
      }
      CacheState.entry_history[CacheState.entry_history_index].string =
          textbox_get_text(state->text);
      CacheState.entry_history[CacheState.entry_history_index].index =
          textbox_get_cursor(state->text);
      // Don't create up if current is empty.
      if (strlen(
              CacheState.entry_history[CacheState.entry_history_index].string) >
          0) {
        CacheState.entry_history_index++;
        if (CacheState.entry_history_index >= CacheState.entry_history_length) {
          CacheState.entry_history =
              g_realloc(CacheState.entry_history,
                        sizeof(EntryHistoryIndex) *
                            (CacheState.entry_history_length + 1));
          CacheState.entry_history[CacheState.entry_history_length].string =
              g_strdup("");
          CacheState.entry_history[CacheState.entry_history_length].index = 0;
          CacheState.entry_history_length++;
        }
      }
      textbox_text(
          state->text,
          CacheState.entry_history[CacheState.entry_history_index].string);
      textbox_cursor(
          state->text,
          CacheState.entry_history[CacheState.entry_history_index].index);
      state->refilter = TRUE;
    }
    break;
  }
  case MATCHER_UP:
    helper_select_next_matching_mode();
    rofi_view_refilter(state);
    rofi_view_set_overlay_timeout(state, helper_get_matching_mode_str());
    break;
  case MATCHER_DOWN:
    helper_select_previous_matching_mode();
    rofi_view_refilter(state);
    rofi_view_set_overlay_timeout(state, helper_get_matching_mode_str());
    break;
  }
}

gboolean rofi_view_check_action(RofiViewState *state, BindingsScope scope,
                                guint action) {
  switch (scope) {
  case SCOPE_GLOBAL:
    return TRUE;
  case SCOPE_MOUSE_LISTVIEW:
  case SCOPE_MOUSE_LISTVIEW_ELEMENT:
  case SCOPE_MOUSE_EDITBOX:
  case SCOPE_MOUSE_SCROLLBAR:
  case SCOPE_MOUSE_MODE_SWITCHER: {
    gint x = state->mouse.x, y = state->mouse.y;
    widget *target = widget_find_mouse_target(WIDGET(state->main_window),
                                              (WidgetType)scope, x, y);
    if (target == NULL) {
      return FALSE;
    }
    widget_xy_to_relative(target, &x, &y);
    switch (widget_check_action(target, action, x, y)) {
    case WIDGET_TRIGGER_ACTION_RESULT_IGNORED:
      return FALSE;
    case WIDGET_TRIGGER_ACTION_RESULT_GRAB_MOTION_END:
    case WIDGET_TRIGGER_ACTION_RESULT_GRAB_MOTION_BEGIN:
    case WIDGET_TRIGGER_ACTION_RESULT_HANDLED:
      return TRUE;
    }
    break;
  }
  }
  return FALSE;
}

void rofi_view_trigger_action(RofiViewState *state, BindingsScope scope,
                              guint action) {
  rofi_view_set_user_timeout(NULL);
  switch (scope) {
  case SCOPE_GLOBAL:
    rofi_view_trigger_global_action(action);
    return;
  case SCOPE_MOUSE_LISTVIEW:
  case SCOPE_MOUSE_LISTVIEW_ELEMENT:
  case SCOPE_MOUSE_EDITBOX:
  case SCOPE_MOUSE_SCROLLBAR:
  case SCOPE_MOUSE_MODE_SWITCHER: {
    gint x = state->mouse.x, y = state->mouse.y;
    // If we already captured a motion, always forward action to this widget.
    widget *target = state->mouse.motion_target;
    // If we have not a previous captured motion, lookup widget.
    if (target == NULL) {
      target = widget_find_mouse_target(WIDGET(state->main_window),
                                        (WidgetType)scope, x, y);
    }
    if (target == NULL) {
      return;
    }
    widget_xy_to_relative(target, &x, &y);
    switch (widget_trigger_action(target, action, x, y)) {
    case WIDGET_TRIGGER_ACTION_RESULT_IGNORED:
      return;
    case WIDGET_TRIGGER_ACTION_RESULT_GRAB_MOTION_END:
      target = NULL;
      rofi_fallthrough;
    case WIDGET_TRIGGER_ACTION_RESULT_GRAB_MOTION_BEGIN:
      state->mouse.motion_target = target;
      rofi_fallthrough;
    case WIDGET_TRIGGER_ACTION_RESULT_HANDLED:
      return;
    }
    break;
  }
  }
}

void rofi_view_handle_text(RofiViewState *state, const char *text) {
  if (state == NULL || state->text == NULL) {
    return;
  }
  if (textbox_append_text(state->text, text, strlen(text))) {
    state->refilter = TRUE;
    rofi_view_input_changed();
  }
}

#if 0
static X11CursorType rofi_cursor_type_to_x11_cursor_type ( RofiCursorType type )
{
    switch ( type )
    {
    case ROFI_CURSOR_DEFAULT:
        return CURSOR_DEFAULT;

    case ROFI_CURSOR_POINTER:
        return CURSOR_POINTER;

    case ROFI_CURSOR_TEXT:
        return CURSOR_TEXT;
    }

    return CURSOR_DEFAULT;
}
#endif

static RofiCursorType rofi_view_resolve_cursor(RofiViewState *state, gint x,
                                               gint y) {
  widget *target = widget_find_mouse_target(WIDGET(state->main_window),
                                            WIDGET_TYPE_UNKNOWN, x, y);

  return target != NULL ? target->cursor_type : ROFI_CURSOR_DEFAULT;
}

void rofi_view_handle_mouse_motion(RofiViewState *state, gint x, gint y,
                                   gboolean find_mouse_target) {
  state->mouse.x = x;
  state->mouse.y = y;

  RofiCursorType cursor_type = rofi_view_resolve_cursor(state, x, y);

  rofi_view_set_cursor(cursor_type);

  if (find_mouse_target) {
    widget *target = widget_find_mouse_target(
        WIDGET(state->main_window), WIDGET_TYPE_LISTVIEW_ELEMENT, x, y);

    if (target != NULL) {
      state->mouse.motion_target = target;
    }
  }

  if (state->mouse.motion_target != NULL) {
    widget_xy_to_relative(state->mouse.motion_target, &x, &y);
    widget_motion_notify(state->mouse.motion_target, x, y);

    if (find_mouse_target) {
      state->mouse.motion_target = NULL;
    }
  }
}

static void rofi_quit_user_callback(RofiViewState *state) {
  if (state->retv & MENU_OK) {
    if (config.on_entry_accepted == NULL)
      return;
    int fstate = 0;
    unsigned int selected = listview_get_selected(state->list_view);
    // TODO: handle custom text
    if (selected >= state->filtered_lines)
      return;
    // Pass selected text to custom command
    char *text = mode_get_display_value(state->sw, state->line_map[selected],
                                        &fstate, NULL, TRUE);
    char **args = NULL;
    int argv = 0;
    helper_parse_setup(config.on_entry_accepted, &args, &argv, "{entry}", text,
                       (char *)0);
    if (args != NULL)
      helper_execute(NULL, args, "", config.on_entry_accepted, NULL);
    g_free(text);
  } else if (state->retv & MENU_CANCEL) {
    if (config.on_menu_canceled == NULL)
      return;
    helper_execute_command(NULL, config.on_menu_canceled, FALSE, NULL);
  } else if (state->retv & MENU_NEXT || state->retv & MENU_PREVIOUS ||
             state->retv & MENU_QUICK_SWITCH || state->retv & MENU_COMPLETE) {
    if (config.on_mode_changed == NULL)
      return;
    // TODO: pass mode name to custom command
    helper_execute_command(NULL, config.on_mode_changed, FALSE, NULL);
  }
}

void rofi_view_maybe_update(RofiViewState *state) {
  if (rofi_view_get_completed(state)) {
    // Exec custom user commands
    rofi_quit_user_callback(state);
    // This menu is done.
    rofi_view_finalize(state);
    // If there a state. (for example error) reload it.
    state = rofi_view_get_active();

    // cleanup, if no more state to display.
    if (state == NULL) {
      // Quit main-loop.
      rofi_quit_main_loop();
      return;
    }
  }

  // Update if requested.
  if (state->refilter) {
    rofi_view_refilter(state);
  }
  rofi_view_update(state, TRUE);
  return;
}
WidgetTriggerActionResult textbox_button_trigger_action(
    widget *wid, MouseBindingMouseDefaultAction action, G_GNUC_UNUSED gint x,
    G_GNUC_UNUSED gint y, G_GNUC_UNUSED void *user_data) {
  RofiViewState *state = (RofiViewState *)user_data;
  switch (action) {
  case MOUSE_CLICK_DOWN: {
    const char *type = rofi_theme_get_string(wid, "action", NULL);
    if (type) {
      if (state->list_view) {
        (state->selected_line) =
            state->line_map[listview_get_selected(state->list_view)];
      } else {
        (state->selected_line) = UINT32_MAX;
      }
      guint id = key_binding_get_action_from_name(type);
      if (id != UINT32_MAX) {
        rofi_view_trigger_global_action(id);
      }
      state->skip_absorb = TRUE;
      return WIDGET_TRIGGER_ACTION_RESULT_HANDLED;
    }
  }
  case MOUSE_CLICK_UP:
  case MOUSE_DCLICK_DOWN:
  case MOUSE_DCLICK_UP:
    break;
  }
  return WIDGET_TRIGGER_ACTION_RESULT_IGNORED;
}
static WidgetTriggerActionResult textbox_sidebar_modes_trigger_action(
    widget *wid, MouseBindingMouseDefaultAction action, G_GNUC_UNUSED gint x,
    G_GNUC_UNUSED gint y, G_GNUC_UNUSED void *user_data) {
  RofiViewState *state = (RofiViewState *)user_data;
  unsigned int i;
  for (i = 0; i < state->num_modes; i++) {
    if (WIDGET(state->modes[i]) == wid) {
      break;
    }
  }
  if (i == state->num_modes) {
    return WIDGET_TRIGGER_ACTION_RESULT_IGNORED;
  }

  switch (action) {
  case MOUSE_CLICK_DOWN:
    state->retv = MENU_QUICK_SWITCH | (i & MENU_LOWER_MASK);
    state->quit = TRUE;
    state->skip_absorb = TRUE;
    return WIDGET_TRIGGER_ACTION_RESULT_HANDLED;
  case MOUSE_CLICK_UP:
  case MOUSE_DCLICK_DOWN:
  case MOUSE_DCLICK_UP:
    break;
  }
  return WIDGET_TRIGGER_ACTION_RESULT_IGNORED;
}

// @TODO don't like this construction.
static void rofi_view_listview_mouse_activated_cb(listview *lv, gboolean custom,
                                                  void *udata) {
  RofiViewState *state = (RofiViewState *)udata;
  state->retv = MENU_OK;
  if (custom) {
    state->retv |= MENU_CUSTOM_ACTION;
  }
  (state->selected_line) = state->line_map[listview_get_selected(lv)];
  // Quit
  state->quit = TRUE;
  state->skip_absorb = TRUE;
}

static void rofi_view_add_widget(RofiViewState *state, widget *parent_widget,
                                 const char *name) {
  char *defaults = NULL;
  widget *wid = NULL;

  /**
   * MAINBOX
   */
  if (strcmp(name, "mainbox") == 0) {
    wid = (widget *)box_create(parent_widget, name, ROFI_ORIENTATION_VERTICAL);
    box_add((box *)parent_widget, WIDGET(wid), TRUE);
    if (config.sidebar_mode) {
      defaults = "inputbar,message,listview,mode-switcher";
    } else {
      defaults = "inputbar,message,listview";
    }
  }
  /**
   * INPUTBAR
   */
  else if (strcmp(name, "inputbar") == 0) {
    wid =
        (widget *)box_create(parent_widget, name, ROFI_ORIENTATION_HORIZONTAL);
    defaults = "prompt,entry,overlay,case-indicator";
    box_add((box *)parent_widget, WIDGET(wid), FALSE);
  }
  /**
   * PROMPT
   */
  else if (strcmp(name, "prompt") == 0) {
    if (state->prompt != NULL) {
      g_error("Prompt widget can only be added once to the layout.");
      return;
    }
    // Prompt box.
    state->prompt =
        textbox_create(parent_widget, WIDGET_TYPE_TEXTBOX_TEXT, name,
                       TB_AUTOWIDTH | TB_AUTOHEIGHT, NORMAL, "", 0, 0);
    rofi_view_update_prompt(state);
    box_add((box *)parent_widget, WIDGET(state->prompt), FALSE);
    defaults = NULL;
  } else if (strcmp(name, "num-rows") == 0) {
    state->tb_total_rows =
        textbox_create(parent_widget, WIDGET_TYPE_TEXTBOX_TEXT, name,
                       TB_AUTOWIDTH | TB_AUTOHEIGHT, NORMAL, "", 0, 0);
    box_add((box *)parent_widget, WIDGET(state->tb_total_rows), FALSE);
    defaults = NULL;
  } else if (strcmp(name, "num-filtered-rows") == 0) {
    state->tb_filtered_rows =
        textbox_create(parent_widget, WIDGET_TYPE_TEXTBOX_TEXT, name,
                       TB_AUTOWIDTH | TB_AUTOHEIGHT, NORMAL, "", 0, 0);
    box_add((box *)parent_widget, WIDGET(state->tb_filtered_rows), FALSE);
    defaults = NULL;
  } else if (strcmp(name, "textbox-current-entry") == 0) {
    state->tb_current_entry =
        textbox_create(parent_widget, WIDGET_TYPE_TEXTBOX_TEXT, name,
                       TB_MARKUP | TB_AUTOHEIGHT, NORMAL, "", 0, 0);
    box_add((box *)parent_widget, WIDGET(state->tb_current_entry), FALSE);
    defaults = NULL;
  } else if (strcmp(name, "icon-current-entry") == 0) {
    state->icon_current_entry = icon_create(parent_widget, name);
    box_add((box *)parent_widget, WIDGET(state->icon_current_entry), FALSE);
    defaults = NULL;
  }
  /**
   * CASE INDICATOR
   */
  else if (strcmp(name, "case-indicator") == 0) {
    if (state->case_indicator != NULL) {
      g_error("Case indicator widget can only be added once to the layout.");
      return;
    }
    state->case_indicator =
        textbox_create(parent_widget, WIDGET_TYPE_TEXTBOX_TEXT, name,
                       TB_AUTOWIDTH | TB_AUTOHEIGHT, NORMAL, "*", 0, 0);
    // Add small separator between case indicator and text box.
    box_add((box *)parent_widget, WIDGET(state->case_indicator), FALSE);
    textbox_text(state->case_indicator, get_matching_state(state));
  }
  /**
   * ENTRY BOX
   */
  else if (strcmp(name, "entry") == 0) {
    if (state->text != NULL) {
      g_error("Entry textbox widget can only be added once to the layout.");
      return;
    }
    // Entry box
    TextboxFlags tfl = TB_EDITABLE;
    tfl |= ((state->menu_flags & MENU_PASSWORD) == MENU_PASSWORD) ? TB_PASSWORD
                                                                  : 0;
    state->text = textbox_create(parent_widget, WIDGET_TYPE_EDITBOX, name,
                                 tfl | TB_AUTOHEIGHT, NORMAL, NULL, 0, 0);
    box_add((box *)parent_widget, WIDGET(state->text), TRUE);
  }
  /**
   * MESSAGE
   */
  else if (strcmp(name, "message") == 0) {
    if (state->mesg_box != NULL) {
      g_error("Message widget can only be added once to the layout.");
      return;
    }
    state->mesg_box = container_create(parent_widget, name);
    state->mesg_tb = textbox_create(
        WIDGET(state->mesg_box), WIDGET_TYPE_TEXTBOX_TEXT, "textbox",
        TB_AUTOHEIGHT | TB_MARKUP | TB_WRAP, NORMAL, NULL, 0, 0);
    container_add(state->mesg_box, WIDGET(state->mesg_tb));
    rofi_view_reload_message_bar(state);
    box_add((box *)parent_widget, WIDGET(state->mesg_box), FALSE);
  }
  /**
   * LISTVIEW
   */
  else if (strcmp(name, "listview") == 0) {
    if (state->list_view != NULL) {
      g_error("Listview widget can only be added once to the layout.");
      return;
    }
    state->list_view =
        listview_create(parent_widget, name, update_callback,
                        page_changed_callback, state, config.element_height, 0);
    listview_set_selection_changed_callback(
        state->list_view, selection_changed_callback, (void *)state);
    box_add((box *)parent_widget, WIDGET(state->list_view), TRUE);
    listview_set_scroll_type(state->list_view, config.scroll_method);
    listview_set_mouse_activated_cb(
        state->list_view, rofi_view_listview_mouse_activated_cb, state);

    listview_set_max_lines(state->list_view, state->num_lines);
  }
  /**
   * MODE SWITCHER
   */
  else if (strcmp(name, "mode-switcher") == 0 || strcmp(name, "sidebar") == 0) {
    if (state->sidebar_bar != NULL) {
      g_error("Mode-switcher can only be added once to the layout.");
      return;
    }
    state->sidebar_bar =
        box_create(parent_widget, name, ROFI_ORIENTATION_HORIZONTAL);
    box_add((box *)parent_widget, WIDGET(state->sidebar_bar), FALSE);
    state->num_modes = rofi_get_num_enabled_modes();
    state->modes = g_malloc0(state->num_modes * sizeof(textbox *));
    for (unsigned int j = 0; j < state->num_modes; j++) {
      const Mode *mode = rofi_get_mode(j);
      state->modes[j] = textbox_create(
          WIDGET(state->sidebar_bar), WIDGET_TYPE_MODE_SWITCHER, "button",
          TB_AUTOHEIGHT, (mode == state->sw) ? HIGHLIGHT : NORMAL,
          mode_get_display_name(mode), 0.5, 0.5);
      box_add(state->sidebar_bar, WIDGET(state->modes[j]), TRUE);
      widget_set_trigger_action_handler(
          WIDGET(state->modes[j]), textbox_sidebar_modes_trigger_action, state);
    }
  } else if (g_ascii_strcasecmp(name, "overlay") == 0) {
    state->overlay = textbox_create(
        WIDGET(parent_widget), WIDGET_TYPE_TEXTBOX_TEXT, "overlay",
        TB_AUTOWIDTH | TB_AUTOHEIGHT, URGENT, "blaat", 0.5, 0);
    box_add((box *)parent_widget, WIDGET(state->overlay), FALSE);
    widget_disable(WIDGET(state->overlay));
  } else if (g_ascii_strncasecmp(name, "textbox", 7) == 0) {
    textbox *t = textbox_create(parent_widget, WIDGET_TYPE_TEXTBOX_TEXT, name,
                                TB_AUTOHEIGHT | TB_WRAP, NORMAL, "", 0, 0);
    box_add((box *)parent_widget, WIDGET(t), TRUE);
  } else if (g_ascii_strncasecmp(name, "button", 6) == 0) {
    textbox *t = textbox_create(parent_widget, WIDGET_TYPE_EDITBOX, name,
                                TB_AUTOHEIGHT | TB_WRAP, NORMAL, "", 0, 0);
    box_add((box *)parent_widget, WIDGET(t), TRUE);
    widget_set_trigger_action_handler(WIDGET(t), textbox_button_trigger_action,
                                      state);
  } else if (g_ascii_strncasecmp(name, "icon", 4) == 0) {
    icon *t = icon_create(parent_widget, name);
    /* small hack to make it clickable */
    const char *type = rofi_theme_get_string(WIDGET(t), "action", NULL);
    if (type) {
      WIDGET(t)->type = WIDGET_TYPE_EDITBOX;
    }
    box_add((box *)parent_widget, WIDGET(t), TRUE);
    widget_set_trigger_action_handler(WIDGET(t), textbox_button_trigger_action,
                                      state);
  } else {
    wid = (widget *)box_create(parent_widget, name, ROFI_ORIENTATION_VERTICAL);
    box_add((box *)parent_widget, WIDGET(wid), TRUE);
    // g_error("The widget %s does not exists. Invalid layout.", name);
  }
  if (wid) {
    GList *list = rofi_theme_get_list_strings(wid, "children");
    if (list == NULL) {
      if (defaults) {
        char **a = g_strsplit(defaults, ",", 0);
        for (int i = 0; a && a[i]; i++) {
          rofi_view_add_widget(state, wid, a[i]);
        }
        g_strfreev(a);
      }
    } else {
      for (const GList *iter = g_list_first(list); iter != NULL;
           iter = g_list_next(iter)) {
        rofi_view_add_widget(state, wid, (const char *)iter->data);
      }
      g_list_free_full(list, g_free);
    }
  }
}

RofiViewState *rofi_view_create(Mode *sw, const char *input,
                                MenuFlags menu_flags,
                                void (*finalize)(RofiViewState *)) {
  TICK();
  RofiViewState *state = __rofi_view_state_create();
  state->menu_flags = menu_flags;
  state->sw = sw;
  state->selected_line = UINT32_MAX;
  state->previous_line = UINT32_MAX;
  state->retv = MENU_CANCEL;
  state->distance = NULL;
  state->quit = FALSE;
  state->skip_absorb = FALSE;
  // We want to filter on the first run.
  state->refilter = TRUE;
  state->finalize = finalize;
  state->mouse_seen = FALSE;

  // In password mode, disable the entry history.
  if ((menu_flags & MENU_PASSWORD) == MENU_PASSWORD) {
    CacheState.entry_history_enable = FALSE;
    g_debug("Disable entry history, because password setup.");
  }
  if (config.disable_history) {
    CacheState.entry_history_enable = FALSE;
    g_debug("Disable entry history, because history disable flag.");
  }
  // Request the lines to show.
  state->num_lines = mode_get_num_entries(sw);

  if (state->sw) {
    char *title =
        g_strdup_printf("rofi - %s", mode_get_display_name(state->sw));
    rofi_view_set_window_title(title);
    g_free(title);
  } else {
    rofi_view_set_window_title("rofi");
  }
  TICK_N("Startup notification");

  // Get active monitor size.
  TICK_N("Get active monitor");

  state->main_window = box_create(NULL, "window", ROFI_ORIENTATION_VERTICAL);
  // Get children.
  GList *list =
      rofi_theme_get_list_strings(WIDGET(state->main_window), "children");
  if (list == NULL) {
    rofi_view_add_widget(state, WIDGET(state->main_window), "mainbox");
  } else {
    for (const GList *iter = list; iter != NULL; iter = g_list_next(iter)) {
      rofi_view_add_widget(state, WIDGET(state->main_window),
                           (const char *)iter->data);
    }
    g_list_free_full(list, g_free);
  }

  if (state->text && input) {
    textbox_text(state->text, input);
    textbox_cursor_end(state->text);
  }

  // filtered list
  state->line_map = g_malloc0_n(state->num_lines, sizeof(unsigned int));
  state->distance = (int *)g_malloc0_n(state->num_lines, sizeof(int));

  rofi_view_calculate_window_width(state);
  // Only needed when window is fixed size.
  if ((CacheState.flags & MENU_NORMAL_WINDOW) == MENU_NORMAL_WINDOW) {
    listview_set_fixed_num_lines(state->list_view);
  }

  state->height = rofi_view_calculate_window_height(state);
  // Move the window to the correct x,y position.
  rofi_view_calculate_window_position(state);
  rofi_view_window_update_size(state);

  state->quit = FALSE;
  rofi_view_refilter(state);
  rofi_view_update(state, TRUE);
#ifdef ENABLE_XCB
  if (xcb->connection) {
    xcb_map_window(xcb->connection, CacheState.main_window);
  }
#endif
  widget_queue_redraw(WIDGET(state->main_window));
  rofi_view_ping_mouse(state);
#ifdef ENABLE_XCB
  if (xcb->connection) {
    xcb_flush(xcb->connection);
  }
#endif

  rofi_view_set_user_timeout(NULL);
  /* When Override Redirect, the WM will not let us know we can take focus, so
   * just steal it */
  if (((menu_flags & MENU_NORMAL_WINDOW) == 0)) {
    display_set_input_focus(CacheState.main_window);
  }

#ifdef ENABLE_XCB
  if (xcb->sncontext != NULL) {
    sn_launchee_context_complete(xcb->sncontext);
  }
#endif
  return state;
}

static void rofi_error_user_callback(const char *msg) {
  if (config.on_menu_error == NULL)
    return;

  char **args = NULL;
  int argv = 0;
  helper_parse_setup(config.on_menu_error, &args, &argv, "{error}", msg,
                     (char *)0);
  if (args != NULL)
    helper_execute(NULL, args, "", config.on_menu_error, NULL);
}

int rofi_view_error_dialog(const char *msg, int markup) {
  RofiViewState *state = __rofi_view_state_create();
  state->retv = MENU_CANCEL;
  state->menu_flags = MENU_ERROR_DIALOG;
  state->finalize = process_result;

  state->main_window = box_create(NULL, "window", ROFI_ORIENTATION_VERTICAL);
  box *new_box = box_create(WIDGET(state->main_window), "error-message",
                            ROFI_ORIENTATION_VERTICAL);
  box_add(state->main_window, WIDGET(new_box), TRUE);
  state->text =
      textbox_create(WIDGET(new_box), WIDGET_TYPE_TEXTBOX_TEXT, "textbox",
                     (TB_AUTOHEIGHT | TB_WRAP) + ((markup) ? TB_MARKUP : 0),
                     NORMAL, (msg != NULL) ? msg : "", 0, 0);
  box_add(new_box, WIDGET(state->text), TRUE);

  // Make sure we enable fixed num lines when in normal window mode.
  if ((CacheState.flags & MENU_NORMAL_WINDOW) == MENU_NORMAL_WINDOW) {
    listview_set_fixed_num_lines(state->list_view);
  }
  rofi_view_calculate_window_width(state);
  state->height = rofi_view_calculate_window_height(state);

  // Calculate window position.
  rofi_view_calculate_window_position(state);

  // Move the window to the correct x,y position.
  rofi_view_window_update_size(state);

#ifdef ENABLE_XCB
  // Display it.
  if (config.backend == DISPLAY_XCB) {
    xcb_map_window(xcb->connection, CacheState.main_window);
  }
#endif
  widget_queue_redraw(WIDGET(state->main_window));

#ifdef ENABLE_XCB
  if (xcb->sncontext != NULL) {
    sn_launchee_context_complete(xcb->sncontext);
  }
#endif

  // Exec custom command
  rofi_error_user_callback(msg);

  // Set it as current window.
  rofi_view_set_active(state);
  return TRUE;
}

static int rofi_thread_workers_sort(gconstpointer a, gconstpointer b,
                                    gpointer data G_GNUC_UNUSED) {
  thread_state *tsa = (thread_state *)a;
  thread_state *tsb = (thread_state *)b;
  // lower number is lower priority..  a is sorted above is a > b.
  return tsa->priority - tsb->priority;
}

static void rofi_thread_pool_state_free(gpointer data) {
  if (data) {
    // This is a weirdness from glib that should not happen.
    // It pushes in a 1 to msg sleeping threads to wake up.
    // This should be removed from queue to avoid hitting this method.
    // In practice, we still hit it (and crash)
    if (GPOINTER_TO_UINT(data) == 1) {
      // Ignore this entry.
      g_debug("Glib thread-pool bug, received pointer with value 1.");
      return;
    }
    thread_state *ts = (thread_state *)data;
    if (ts->free) {
      ts->free(data);
    }
  }
}

void rofi_view_workers_initialize(void) {
  TICK_N("Setup Threadpool, start");
  if (config.threads == 0) {
    config.threads = 1;
    long procs = sysconf(_SC_NPROCESSORS_CONF);
    if (procs > 0) {
      config.threads = MIN(procs, 128l);
    }
  }
  // Create thread pool
  GError *error = NULL;
  tpool = g_thread_pool_new_full(rofi_view_call_thread, NULL,
                                 rofi_thread_pool_state_free, config.threads,
                                 FALSE, &error);
  if (error == NULL) {
    // Idle threads should stick around for a max of 60 seconds.
    g_thread_pool_set_max_idle_time(60000);
    // We are allowed to have
    g_thread_pool_set_max_threads(tpool, config.threads, &error);
  }
  // If error occurred during setup of pool, tell user and exit.
  if (error != NULL) {
    g_warning("Failed to setup thread pool: '%s'", error->message);
    g_error_free(error);
    exit(EXIT_FAILURE);
  }
  g_thread_pool_set_sort_function(tpool, rofi_thread_workers_sort, NULL);
  TICK_N("Setup Threadpool, done");
}
void rofi_view_workers_finalize(void) {
  if (tpool) {
    // Discard all unprocessed jobs and don't wait for current jobs in execution
    g_thread_pool_free(tpool, TRUE, FALSE);
    tpool = NULL;
  }
}
Mode *rofi_view_get_mode(RofiViewState *state) { return state->sw; }

static gboolean rofi_view_overlay_timeout(G_GNUC_UNUSED gpointer user_data) {
  RofiViewState *state = rofi_view_get_active();
  if (state) {
    widget_disable(WIDGET(state->overlay));
  }
  CacheState.overlay_timeout = 0;
  rofi_view_queue_redraw();
  return G_SOURCE_REMOVE;
}

void rofi_view_set_overlay_timeout(RofiViewState *state, const char *text) {
  if (state->overlay == NULL || state->list_view == NULL) {
    return;
  }
  if (text == NULL) {
    widget_disable(WIDGET(state->overlay));
    return;
  }
  rofi_view_set_overlay(state, text);
  int timeout = rofi_theme_get_integer(WIDGET(state->overlay), "timeout", 3);
  CacheState.overlay_timeout =
      g_timeout_add_seconds(timeout, rofi_view_overlay_timeout, state);
}

void rofi_view_set_overlay(RofiViewState *state, const char *text) {
  if (state->overlay == NULL || state->list_view == NULL) {
    return;
  }
  if (CacheState.overlay_timeout > 0) {
    g_source_remove(CacheState.overlay_timeout);
    CacheState.overlay_timeout = 0;
  }
  if (text == NULL) {
    widget_disable(WIDGET(state->overlay));
    return;
  }
  widget_enable(WIDGET(state->overlay));
  textbox_text(state->overlay, text);
  // We want to queue a repaint.
  rofi_view_queue_redraw();
}

void rofi_view_clear_input(RofiViewState *state) {
  if (state->text) {
    textbox_text(state->text, "");
    rofi_view_set_selected_line(state, 0);
  }
}

void rofi_view_ellipsize_listview(RofiViewState *state,
                                  PangoEllipsizeMode mode) {
  listview_set_ellipsize(state->list_view, mode);
}

void rofi_view_switch_mode(RofiViewState *state, Mode *mode) {
  state->sw = mode;
  // Update prompt;
  if (state->prompt) {
    rofi_view_update_prompt(state);
  }
  if (state->sw) {
    char *title =
        g_strdup_printf("rofi - %s", mode_get_display_name(state->sw));
    rofi_view_set_window_title(title);
    g_free(title);
  } else {
    rofi_view_set_window_title("rofi");
  }
  if (state->sidebar_bar) {
    for (unsigned int j = 0; j < state->num_modes; j++) {
      const Mode *tb_mode = rofi_get_mode(j);
      textbox_font(state->modes[j],
                   (tb_mode == state->sw) ? HIGHLIGHT : NORMAL);
    }
  }
  rofi_view_restart(state);
  state->reload = TRUE;
  state->refilter = TRUE;
  rofi_view_refilter_force(state);
  rofi_view_update(state, TRUE);
}

/** ------ */

void rofi_view_update(RofiViewState *state, gboolean qr) {
  proxy->update(state, qr);
}

void rofi_view_temp_configure_notify(RofiViewState *state,
                                     xcb_configure_notify_event_t *xce) {
  proxy->temp_configure_notify(state, xce);
}

void rofi_view_temp_click_to_exit(RofiViewState *state, xcb_window_t target) {
  proxy->temp_click_to_exit(state, target);
}

void rofi_view_frame_callback(void) { proxy->frame_callback(); }

void rofi_view_queue_redraw(void) { proxy->queue_redraw(); }

void rofi_view_set_window_title(const char *title) {
  proxy->set_window_title(title);
}

void rofi_view_calculate_window_position(RofiViewState *state) {
  proxy->calculate_window_position(state);
}

void rofi_view_calculate_window_width(struct RofiViewState *state) {
  proxy->calculate_window_width(state);
}

int rofi_view_calculate_window_height(RofiViewState *state) {
  return proxy->calculate_window_height(state);
}

void rofi_view_window_update_size(RofiViewState *state) {
  proxy->window_update_size(state);
}

void rofi_view_set_cursor(RofiCursorType type) { proxy->set_cursor(type); }

void rofi_view_cleanup(void) { proxy->cleanup(); }

void rofi_view_hide(void) { proxy->hide(); }

void rofi_view_reload(void) { proxy->reload(); }

void __create_window(MenuFlags menu_flags) {
  proxy->__create_window(menu_flags);
}

xcb_window_t rofi_view_get_window(void) { return proxy->get_window(); }

void rofi_view_get_current_monitor(int *width, int *height) {
  proxy->get_current_monitor(width, height);
}

void rofi_view_set_size(RofiViewState *state, gint width, gint height) {
  proxy->set_size(state, width, height);
}

void rofi_view_get_size(RofiViewState *state, gint *width, gint *height) {
  proxy->get_size(state, width, height);
}

void rofi_view_ping_mouse(RofiViewState *state) { proxy->ping_mouse(state); }

void rofi_view_pool_refresh(void) { proxy->pool_refresh(); }
