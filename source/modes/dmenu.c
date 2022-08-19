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
#define G_LOG_DOMAIN "Modes.DMenu"

#include "modes/dmenu.h"
#include "helper.h"
#include "rofi-icon-fetcher.h"
#include "rofi.h"
#include "settings.h"
#include "view.h"
#include "widgets/textbox.h"
#include "xrmoptions.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <gio/gio.h>
#include <gio/gunixinputstream.h>
#include <glib-unix.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "modes/dmenuscriptshared.h"

static int dmenu_mode_init(Mode *sw);
static int dmenu_token_match(const Mode *sw, rofi_int_matcher **tokens,
                             unsigned int index);
static cairo_surface_t *
dmenu_get_icon(const Mode *sw, unsigned int selected_line, unsigned int height);
static char *dmenu_get_message(const Mode *sw);

static inline unsigned int bitget(uint32_t const *const array,
                                  unsigned int index) {
  uint32_t bit = index % 32;
  uint32_t val = array[index / 32];
  return (val >> bit) & 1;
}

static inline void bittoggle(uint32_t *const array, unsigned int index) {
  uint32_t bit = index % 32;
  uint32_t *v = &array[index / 32];
  *v ^= 1 << bit;
}

typedef struct {
  /** Settings */
  // Separator.
  char separator;

  unsigned int selected_line;
  char *message;
  char *format;
  struct rofi_range_pair *urgent_list;
  unsigned int num_urgent_list;
  struct rofi_range_pair *active_list;
  unsigned int num_active_list;
  uint32_t *selected_list;
  unsigned int num_selected_list;
  unsigned int do_markup;
  // List with entries.
  DmenuScriptEntry *cmd_list;
  unsigned int cmd_list_real_length;
  unsigned int cmd_list_length;
  unsigned int only_selected;
  unsigned int selected_count;

  gchar **columns;
  gchar *column_separator;
  gboolean multi_select;

  GThread *reading_thread;
  GAsyncQueue *async_queue;
  gboolean async;
  FILE *fd_file;
  int fd;
  int pipefd[2];
  int pipefd2[2];
  guint wake_source;
  gboolean loading;

  char *ballot_selected;
  char *ballot_unselected;
} DmenuModePrivateData;

#define BLOCK_LINES_SIZE 2048
typedef struct {
  unsigned int length;
  DmenuScriptEntry values[BLOCK_LINES_SIZE];
  DmenuModePrivateData *pd;
} Block;

static void read_add_block(DmenuModePrivateData *pd, Block **block, char *data,
                           gsize len) {

  if ((*block) == NULL) {
    (*block) = g_malloc0(sizeof(Block));
    (*block)->pd = pd;
    (*block)->length = 0;
  }
  gsize data_len = len;
  // Init.
  (*block)->values[(*block)->length].icon_fetch_uid = 0;
  (*block)->values[(*block)->length].icon_fetch_size = 0;
  (*block)->values[(*block)->length].icon_name = NULL;
  (*block)->values[(*block)->length].meta = NULL;
  (*block)->values[(*block)->length].info = NULL;
  (*block)->values[(*block)->length].nonselectable = FALSE;
  char *end = data;
  while (end < data + len && *end != '\0') {
    end++;
  }
  if (end != data + len) {
    data_len = end - data;
    dmenuscript_parse_entry_extras(NULL, &((*block)->values[(*block)->length]),
                                   end + 1, len - data_len);
  }
  char *utfstr = rofi_force_utf8(data, data_len);
  (*block)->values[(*block)->length].entry = utfstr;
  (*block)->values[(*block)->length + 1].entry = NULL;

  (*block)->length++;
}

static void read_add(DmenuModePrivateData *pd, char *data, gsize len) {
  gsize data_len = len;
  if ((pd->cmd_list_length + 2) > pd->cmd_list_real_length) {
    pd->cmd_list_real_length = MAX(pd->cmd_list_real_length * 2, 512);
    pd->cmd_list = g_realloc(pd->cmd_list, (pd->cmd_list_real_length) *
                                               sizeof(DmenuScriptEntry));
  }
  // Init.
  pd->cmd_list[pd->cmd_list_length].icon_fetch_uid = 0;
  pd->cmd_list[pd->cmd_list_length].icon_fetch_size = 0;
  pd->cmd_list[pd->cmd_list_length].icon_name = NULL;
  pd->cmd_list[pd->cmd_list_length].meta = NULL;
  pd->cmd_list[pd->cmd_list_length].info = NULL;
  pd->cmd_list[pd->cmd_list_length].nonselectable = FALSE;
  char *end = data;
  while (end < data + len && *end != '\0') {
    end++;
  }
  if (end != data + len) {
    data_len = end - data;
    dmenuscript_parse_entry_extras(NULL, &(pd->cmd_list[pd->cmd_list_length]),
                                   end + 1, len - data_len);
  }
  char *utfstr = rofi_force_utf8(data, data_len);
  pd->cmd_list[pd->cmd_list_length].entry = utfstr;
  pd->cmd_list[pd->cmd_list_length + 1].entry = NULL;

  pd->cmd_list_length++;
}

/**
 * This method is called from a  GSource that responds to READ available event
 * on the file descriptor of the IPC pipe with the reading thread.
 * This method runs in the same thread as the UI and updates the dmenu mode
 * internal administratinos with new items.
 *
 * The data is copied not via the pipe, but via the Async Queue.
 * A maximal BLOCK_LINES_SIZE items are added with one block.
 */
static gboolean dmenu_async_read_proc(gint fd, GIOCondition condition,
                                      gpointer user_data) {
  DmenuModePrivateData *pd = (DmenuModePrivateData *)user_data;
  char command;
  // Only interrested in read events.
  if ((condition & G_IO_IN) != G_IO_IN) {
    return G_SOURCE_CONTINUE;
  }
  // Read the entry from the pipe that was used to signal this action.
  if (read(fd, &command, 1) == 1) {
    if (command == 'r') {
      Block *block = NULL;
      gboolean changed = FALSE;
      // Empty out the AsyncQueue (that is thread safe) from all blocks pushed
      // into it.
      while ((block = g_async_queue_try_pop(pd->async_queue)) != NULL) {

        if (pd->cmd_list_real_length < (pd->cmd_list_length + block->length)) {
          pd->cmd_list_real_length = MAX(pd->cmd_list_real_length * 2, 4096);
          pd->cmd_list = g_realloc(pd->cmd_list, sizeof(DmenuScriptEntry) *
                                                     pd->cmd_list_real_length);
        }
        memcpy(&(pd->cmd_list[pd->cmd_list_length]), &(block->values[0]),
               sizeof(DmenuScriptEntry) * block->length);
        pd->cmd_list_length += block->length;
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

static void read_input_sync(DmenuModePrivateData *pd, unsigned int pre_read) {
  ssize_t nread = 0;
  size_t len = 0;
  char *line = NULL;
  while (pre_read > 0 &&
         (nread = getdelim(&line, &len, pd->separator, pd->fd_file)) != -1) {
    if (line[nread - 1] == pd->separator) {
      nread--;
      line[nread] = '\0';
    }
    read_add(pd, line, nread);
    pre_read--;
  }
  free(line);
  return;
}
static gpointer read_input_thread(gpointer userdata) {
  DmenuModePrivateData *pd = (DmenuModePrivateData *)userdata;
  ssize_t nread = 0;
  ssize_t len = 0;
  char *line = NULL;
  // Create the message passing queue to the UI thread.
  pd->async_queue = g_async_queue_new();
  Block *block = NULL;

  GTimer *tim = g_timer_new();
  int fd = pd->fd;
  while (1) {
    // Wait for input from the input or from the main thread.
    fd_set rfds;
    // We wait for 0.25 seconds, before we flush what we have.
    struct timeval tv = {.tv_sec = 0, .tv_usec = 250000};

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    FD_SET(pd->pipefd[0], &rfds);

    int retval = select(MAX(fd, pd->pipefd[0]) + 1, &rfds, NULL, NULL, &tv);
    if (retval == -1) {
      g_warning("select failed, giving up.");
      break;
    } else if (retval) {
      // We get input from the UI thread, this is always an abort.
      if (FD_ISSET(pd->pipefd[0], &rfds)) {
        break;
      }
      //  Input data is available.
      if (FD_ISSET(fd, &rfds)) {
        ssize_t readbytes = 0;
        if ((nread + 1024) > len) {
          line = g_realloc(line, (nread + 1024));
          len = nread + 1024;
        }
        readbytes = read(fd, &line[nread], 1023);
        if (readbytes > 0) {
          nread += readbytes;
          line[nread] = '\0';
          ssize_t i = 0;
          while (i < nread) {
            if (line[i] == pd->separator) {
              line[i] = '\0';
              read_add_block(pd, &block, line, i);
              memmove(&line[0], &line[i + 1], nread - (i + 1));
              nread -= (i + 1);
              i = 0;
              if (block) {
                double elapsed = g_timer_elapsed(tim, NULL);
                if ( elapsed >= 0.1 || block->length == BLOCK_LINES_SIZE) {
                  g_timer_start(tim);
                  g_async_queue_push(pd->async_queue, block);
                  block = NULL;
                  write(pd->pipefd2[1], "r", 1);
                }
              }
            } else {
              i++;
            }
          }
        } else {
          // remainder in buffer, then quit.
          if (nread > 0) {
            line[nread] = '\0';
            read_add_block(pd, &block, line, nread);
          }
          if (block) {
            g_timer_start(tim);
            g_async_queue_push(pd->async_queue, block);
            block = NULL;
            write(pd->pipefd2[1], "r", 1);
          }
          break;
        }
      }
    } else {
      // Timeout, pushout remainder data.
      if (nread > 0) {
        line[nread] = '\0';
        read_add_block(pd, &block, line, nread);
        nread = 0;
      }
      if (block) {
        g_timer_start(tim);
        g_async_queue_push(pd->async_queue, block);
        block = NULL;
        write(pd->pipefd2[1], "r", 1);
      }
    }
  }
  g_timer_destroy(tim);
  free(line);
  write(pd->pipefd2[1], "q", 1);
  return NULL;
}

static unsigned int dmenu_mode_get_num_entries(const Mode *sw) {
  const DmenuModePrivateData *rmpd =
      (const DmenuModePrivateData *)mode_get_private_data(sw);
  unsigned int retv = rmpd->cmd_list_length;
  return retv;
}

static gchar *dmenu_format_output_string(const DmenuModePrivateData *pd,
                                         const char *input,
                                         const unsigned int index,
                                         gboolean multi_select) {
  if (pd->columns == NULL) {
    if (multi_select) {
      if (pd->selected_list && bitget(pd->selected_list, index) == TRUE) {
        return g_strdup_printf("%s%s", pd->ballot_selected, input);
      } else {
        return g_strdup_printf("%s%s", pd->ballot_unselected, input);
      }
    }
    return g_strdup(input);
  }
  char *retv = NULL;
  char **splitted =
      g_regex_split_simple(pd->column_separator, input, G_REGEX_CASELESS, 00);
  uint32_t ns = 0;
  for (; splitted && splitted[ns]; ns++) {
    ;
  }
  GString *str_retv = g_string_new("");

  if (multi_select) {
    if (pd->selected_list && bitget(pd->selected_list, index) == TRUE) {
      g_string_append(str_retv, pd->ballot_selected);
    } else {
      g_string_append(str_retv, pd->ballot_unselected);
    }
  }
  for (uint32_t i = 0; pd->columns && pd->columns[i]; i++) {
    unsigned int index =
        (unsigned int)g_ascii_strtoull(pd->columns[i], NULL, 10);
    if (index <= ns && index > 0) {
      if (index == 1) {
        g_string_append(str_retv, splitted[index - 1]);
      } else {
        g_string_append_c(str_retv, '\t');
        g_string_append(str_retv, splitted[index - 1]);
      }
    }
  }
  g_strfreev(splitted);
  retv = str_retv->str;
  g_string_free(str_retv, FALSE);
  return retv;
}

static inline unsigned int get_index(unsigned int length, int index) {
  if (index >= 0) {
    return index;
  }
  if (((unsigned int)-index) <= length) {
    return length + index;
  }
  // Out of range.
  return UINT_MAX;
}

static char *dmenu_get_completion_data(const Mode *data, unsigned int index) {
  Mode *sw = (Mode *)data;
  DmenuModePrivateData *pd = (DmenuModePrivateData *)mode_get_private_data(sw);
  DmenuScriptEntry *retv = (DmenuScriptEntry *)pd->cmd_list;
  return dmenu_format_output_string(pd, retv[index].entry, index, FALSE);
}

static char *get_display_data(const Mode *data, unsigned int index, int *state,
                              G_GNUC_UNUSED GList **list, int get_entry) {
  Mode *sw = (Mode *)data;
  DmenuModePrivateData *pd = (DmenuModePrivateData *)mode_get_private_data(sw);
  DmenuScriptEntry *retv = (DmenuScriptEntry *)pd->cmd_list;
  for (unsigned int i = 0; i < pd->num_active_list; i++) {
    unsigned int start =
        get_index(pd->cmd_list_length, pd->active_list[i].start);
    unsigned int stop = get_index(pd->cmd_list_length, pd->active_list[i].stop);
    if (index >= start && index <= stop) {
      *state |= ACTIVE;
    }
  }
  for (unsigned int i = 0; i < pd->num_urgent_list; i++) {
    unsigned int start =
        get_index(pd->cmd_list_length, pd->urgent_list[i].start);
    unsigned int stop = get_index(pd->cmd_list_length, pd->urgent_list[i].stop);
    if (index >= start && index <= stop) {
      *state |= URGENT;
    }
  }
  if (pd->selected_list && bitget(pd->selected_list, index) == TRUE) {
    *state |= SELECTED;
  }
  if (pd->do_markup) {
    *state |= MARKUP;
  }
  char *my_retv =
      (get_entry ? dmenu_format_output_string(pd, retv[index].entry, index,
                                              pd->multi_select)
                 : NULL);
  return my_retv;
}

static void dmenu_mode_free(Mode *sw) {
  if (mode_get_private_data(sw) == NULL) {
    return;
  }
  DmenuModePrivateData *pd = (DmenuModePrivateData *)mode_get_private_data(sw);
  if (pd != NULL) {

    for (size_t i = 0; i < pd->cmd_list_length; i++) {
      if (pd->cmd_list[i].entry) {
        g_free(pd->cmd_list[i].entry);
        g_free(pd->cmd_list[i].icon_name);
        g_free(pd->cmd_list[i].meta);
        g_free(pd->cmd_list[i].info);
      }
    }
    g_free(pd->cmd_list);
    g_free(pd->urgent_list);
    g_free(pd->active_list);
    g_free(pd->selected_list);

    g_free(pd);
    mode_set_private_data(sw, NULL);
  }
}

#include "mode-private.h"
/** dmenu Mode object. */
Mode dmenu_mode = {.name = "dmenu",
                   .cfg_name_key = "display-combi",
                   ._init = dmenu_mode_init,
                   ._get_num_entries = dmenu_mode_get_num_entries,
                   ._result = NULL,
                   ._destroy = dmenu_mode_free,
                   ._token_match = dmenu_token_match,
                   ._get_display_value = get_display_data,
                   ._get_icon = dmenu_get_icon,
                   ._get_completion = dmenu_get_completion_data,
                   ._preprocess_input = NULL,
                   ._get_message = dmenu_get_message,
                   .private_data = NULL,
                   .free = NULL,
                   .display_name = "dmenu"};

static int dmenu_mode_init(Mode *sw) {
  if (mode_get_private_data(sw) != NULL) {
    return TRUE;
  }
  mode_set_private_data(sw, g_malloc0(sizeof(DmenuModePrivateData)));
  DmenuModePrivateData *pd = (DmenuModePrivateData *)mode_get_private_data(sw);

  pd->async = TRUE;

  // For now these only work in sync mode.
  if (find_arg("-sync") >= 0 || find_arg("-dump") >= 0 ||
      find_arg("-select") >= 0 || find_arg("-no-custom") >= 0 ||
      find_arg("-only-match") >= 0 || config.auto_select ||
      find_arg("-selected-row") >= 0) {
    pd->async = FALSE;
  }

  pd->separator = '\n';
  pd->selected_line = UINT32_MAX;

  find_arg_str("-mesg", &(pd->message));

  // Input data separator.
  find_arg_char("-sep", &(pd->separator));

  find_arg_uint("-selected-row", &(pd->selected_line));
  // By default we print the unescaped line back.
  pd->format = "s";

  // Allow user to override the output format.
  find_arg_str("-format", &(pd->format));
  // Urgent.
  char *str = NULL;
  find_arg_str("-u", &str);
  if (str != NULL) {
    parse_ranges(str, &(pd->urgent_list), &(pd->num_urgent_list));
  }
  // Active
  str = NULL;
  find_arg_str("-a", &str);
  if (str != NULL) {
    parse_ranges(str, &(pd->active_list), &(pd->num_active_list));
  }

  // DMENU COMPATIBILITY
  unsigned int lines = DEFAULT_MENU_LINES;
  find_arg_uint("-l", &(lines));
  if (lines != DEFAULT_MENU_LINES) {
    Property *p = rofi_theme_property_create(P_INTEGER);
    p->name = g_strdup("lines");
    p->value.i = lines;
    ThemeWidget *widget =
        rofi_theme_find_or_create_name(rofi_theme, "listview");
    GHashTable *table =
        g_hash_table_new_full(g_str_hash, g_str_equal, NULL,
                              (GDestroyNotify)rofi_theme_property_free);

    g_hash_table_replace(table, p->name, p);
    rofi_theme_widget_add_properties(widget, table);
    g_hash_table_destroy(table);
  }

  str = NULL;
  find_arg_str("-window-title", &str);
  if (str) {
    dmenu_mode.display_name = str;
  }

  /**
   * Dmenu compatibility.
   * `-b` put on bottom.
   */
  if (find_arg("-b") >= 0) {
    config.location = 6;
  }
  /* -i case insensitive */
  config.case_sensitive = TRUE;
  if (find_arg("-i") >= 0) {
    config.case_sensitive = FALSE;
  }
  if (pd->async) {
    pd->fd = STDIN_FILENO;
    if (find_arg_str("-input", &str)) {
      char *estr = rofi_expand_path(str);
      pd->fd = open(str, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
      if (pd->fd == -1) {
        char *msg = g_markup_printf_escaped(
            "Failed to open file: <b>%s</b>:\n\t<i>%s</i>", estr,
            g_strerror(errno));
        rofi_view_error_dialog(msg, TRUE);
        g_free(msg);
        g_free(estr);
        return TRUE;
      }
      g_free(estr);
    }

    if (pipe(pd->pipefd) == -1) {
      g_error("Failed to create pipe");
    }
    if (pipe(pd->pipefd2) == -1) {
      g_error("Failed to create pipe");
    }
    pd->wake_source =
        g_unix_fd_add(pd->pipefd2[0], G_IO_IN, dmenu_async_read_proc, pd);
    pd->reading_thread =
        g_thread_new("dmenu-read", (GThreadFunc)read_input_thread, pd);
    pd->loading = TRUE;
  } else {
    pd->fd_file = stdin;
    str = NULL;
    if (find_arg_str("-input", &str)) {
      char *estr = rofi_expand_path(str);
      pd->fd_file = fopen(str, "r");
      if (pd->fd_file == NULL) {
        char *msg = g_markup_printf_escaped(
            "Failed to open file: <b>%s</b>:\n\t<i>%s</i>", estr,
            g_strerror(errno));
        rofi_view_error_dialog(msg, TRUE);
        g_free(msg);
        g_free(estr);
        return TRUE;
      }
      g_free(estr);
    }

    read_input_sync(pd, -1);
  }
  gchar *columns = NULL;
  if (find_arg_str("-display-columns", &columns)) {
    pd->columns = g_strsplit(columns, ",", 0);
    pd->column_separator = "\t";
    find_arg_str("-display-column-separator", &pd->column_separator);
  }
  return TRUE;
}

static int dmenu_token_match(const Mode *sw, rofi_int_matcher **tokens,
                             unsigned int index) {
  DmenuModePrivateData *rmpd =
      (DmenuModePrivateData *)mode_get_private_data(sw);

  /** Strip out the markup when matching. */
  char *esc = NULL;
  if (rmpd->do_markup) {
    pango_parse_markup(rmpd->cmd_list[index].entry, -1, 0, NULL, &esc, NULL,
                       NULL);
  } else {
    esc = rmpd->cmd_list[index].entry;
  }
  if (esc) {
    //        int retv = helper_token_match ( tokens, esc );
    int match = 1;
    if (tokens) {
      for (int j = 0; match && tokens[j] != NULL; j++) {
        rofi_int_matcher *ftokens[2] = {tokens[j], NULL};
        int test = 0;
        test = helper_token_match(ftokens, esc);
        if (test == tokens[j]->invert && rmpd->cmd_list[index].meta) {
          test = helper_token_match(ftokens, rmpd->cmd_list[index].meta);
        }

        if (test == 0) {
          match = 0;
        }
      }
    }
    if (rmpd->do_markup) {
      g_free(esc);
    }
    return match;
  }
  return FALSE;
}
static char *dmenu_get_message(const Mode *sw) {
  DmenuModePrivateData *pd = (DmenuModePrivateData *)mode_get_private_data(sw);
  if (pd->message) {
    return g_strdup(pd->message);
  }
  return NULL;
}
static cairo_surface_t *dmenu_get_icon(const Mode *sw,
                                       unsigned int selected_line,
                                       unsigned int height) {
  DmenuModePrivateData *pd = (DmenuModePrivateData *)mode_get_private_data(sw);

  g_return_val_if_fail(pd->cmd_list != NULL, NULL);
  DmenuScriptEntry *dr = &(pd->cmd_list[selected_line]);
  if (dr->icon_name == NULL) {
    return NULL;
  }
  if (dr->icon_fetch_uid > 0 && dr->icon_fetch_size == height) {
    return rofi_icon_fetcher_get(dr->icon_fetch_uid);
  }
  uint32_t uid = dr->icon_fetch_uid =
      rofi_icon_fetcher_query(dr->icon_name, height);
  dr->icon_fetch_size = height;

  return rofi_icon_fetcher_get(uid);
}

static void dmenu_finish(DmenuModePrivateData *pd, RofiViewState *state,
                         int retv) {

  if (pd->reading_thread) {
    // Stop listinig to new messages from reading thread.
    if (pd->wake_source > 0) {
      g_source_remove(pd->wake_source);
    }
    // signal stop.
    write(pd->pipefd[1], "q", 1);
    g_thread_join(pd->reading_thread);
    pd->reading_thread = NULL;
    /* empty the queue, remove idle callbacks if still pending. */
    g_async_queue_lock(pd->async_queue);
    Block *block = NULL;
    while ((block = g_async_queue_try_pop_unlocked(pd->async_queue)) != NULL) {
      g_free(block);
    }
    g_async_queue_unlock(pd->async_queue);
    g_async_queue_unref(pd->async_queue);
    pd->async_queue = NULL;
    close(pd->pipefd[0]);
    close(pd->pipefd[1]);
  }
  if (pd->fd_file != NULL) {
    if (pd->fd_file != stdin) {
      fclose(pd->fd_file);
    }
  }
  if (retv == FALSE) {
    rofi_set_return_code(EXIT_FAILURE);
  } else if (retv >= 10) {
    rofi_set_return_code(retv);
  } else {
    rofi_set_return_code(EXIT_SUCCESS);
  }
  rofi_view_set_active(NULL);
  rofi_view_free(state);
  mode_destroy(&dmenu_mode);
}

static void dmenu_print_results(DmenuModePrivateData *pd, const char *input) {
  DmenuScriptEntry *cmd_list = pd->cmd_list;
  int seen = FALSE;
  if (pd->selected_list != NULL) {
    for (unsigned int st = 0; st < pd->cmd_list_length; st++) {
      if (bitget(pd->selected_list, st)) {
        seen = TRUE;
        rofi_output_formatted_line(pd->format, cmd_list[st].entry, st, input);
      }
    }
  }
  if (!seen) {
    const char *cmd = input;
    if (pd->selected_line != UINT32_MAX) {
      cmd = cmd_list[pd->selected_line].entry;
    }
    if (cmd) {
      rofi_output_formatted_line(pd->format, cmd, pd->selected_line, input);
    }
  }
}

static void dmenu_finalize(RofiViewState *state) {
  int retv = FALSE;
  DmenuModePrivateData *pd =
      (DmenuModePrivateData *)rofi_view_get_mode(state)->private_data;

  unsigned int cmd_list_length = pd->cmd_list_length;
  DmenuScriptEntry *cmd_list = pd->cmd_list;

  char *input = g_strdup(rofi_view_get_user_input(state));
  pd->selected_line = rofi_view_get_selected_line(state);
  ;
  MenuReturn mretv = rofi_view_get_return_value(state);
  unsigned int next_pos = rofi_view_get_next_position(state);
  int restart = 0;
  // Special behavior.
  if (pd->only_selected) {
    /**
     * Select item mode.
     */
    restart = 1;
    // Skip if no valid item is selected.
    if ((mretv & MENU_CANCEL) == MENU_CANCEL) {
      // In no custom mode we allow canceling.
      restart = (find_arg("-only-match") >= 0);
    } else if (pd->selected_line != UINT32_MAX) {
      if ((mretv & MENU_CUSTOM_ACTION) && pd->multi_select) {
        restart = TRUE;
        pd->loading = FALSE;
        if (pd->selected_list == NULL) {
          pd->selected_list =
              g_malloc0(sizeof(uint32_t) * (pd->cmd_list_length / 32 + 1));
        }
        pd->selected_count +=
            (bitget(pd->selected_list, pd->selected_line) ? (-1) : (1));
        bittoggle(pd->selected_list, pd->selected_line);
        // Move to next line.
        pd->selected_line = MIN(next_pos, cmd_list_length - 1);
        if (pd->selected_count > 0) {
          char *str =
              g_strdup_printf("%u/%u", pd->selected_count, pd->cmd_list_length);
          rofi_view_set_overlay(state, str);
          g_free(str);
        } else {
          rofi_view_set_overlay(state, NULL);
        }
      } else if ((mretv & (MENU_OK | MENU_CUSTOM_COMMAND)) &&
                 cmd_list[pd->selected_line].entry != NULL) {
        if (cmd_list[pd->selected_line].nonselectable == TRUE) {
          g_free(input);
          return;
        }
        dmenu_print_results(pd, input);
        retv = TRUE;
        if ((mretv & MENU_CUSTOM_COMMAND)) {
          retv = 10 + (mretv & MENU_LOWER_MASK);
        }
        g_free(input);
        dmenu_finish(pd, state, retv);
        return;
      } else {
        pd->selected_line = next_pos - 1;
      }
    }
    // Restart
    rofi_view_restart(state);
    rofi_view_set_selected_line(state, pd->selected_line);
    if (!restart) {
      dmenu_finish(pd, state, retv);
    }
    return;
  }
  // We normally do not want to restart the loop.
  restart = FALSE;
  // Normal mode
  if ((mretv & MENU_OK) && pd->selected_line != UINT32_MAX &&
      cmd_list[pd->selected_line].entry != NULL) {
    // Check if entry is non-selectable.
    if (cmd_list[pd->selected_line].nonselectable == TRUE) {
      g_free(input);
      return;
    }
    if ((mretv & MENU_CUSTOM_ACTION) && pd->multi_select) {
      restart = TRUE;
      if (pd->selected_list == NULL) {
        pd->selected_list =
            g_malloc0(sizeof(uint32_t) * (pd->cmd_list_length / 32 + 1));
      }
      pd->selected_count +=
          (bitget(pd->selected_list, pd->selected_line) ? (-1) : (1));
      bittoggle(pd->selected_list, pd->selected_line);
      // Move to next line.
      pd->selected_line = MIN(next_pos, cmd_list_length - 1);
      if (pd->selected_count > 0) {
        char *str =
            g_strdup_printf("%u/%u", pd->selected_count, pd->cmd_list_length);
        rofi_view_set_overlay(state, str);
        g_free(str);
      } else {
        rofi_view_set_overlay(state, NULL);
      }
    } else {
      dmenu_print_results(pd, input);
    }
    retv = TRUE;
  }
  // Custom input
  else if ((mretv & (MENU_CUSTOM_INPUT))) {
    dmenu_print_results(pd, input);

    retv = TRUE;
  }
  // Quick switch with entry selected.
  else if ((mretv & MENU_CUSTOM_COMMAND)) {
    dmenu_print_results(pd, input);

    restart = FALSE;
    retv = 10 + (mretv & MENU_LOWER_MASK);
  }
  g_free(input);
  if (restart) {
    rofi_view_restart(state);
    rofi_view_set_selected_line(state, pd->selected_line);
  } else {
    dmenu_finish(pd, state, retv);
  }
}

int dmenu_mode_dialog(void) {
  mode_init(&dmenu_mode);
  MenuFlags menu_flags = MENU_NORMAL;
  DmenuModePrivateData *pd = (DmenuModePrivateData *)dmenu_mode.private_data;

  char *input = NULL;
  unsigned int cmd_list_length = pd->cmd_list_length;
  DmenuScriptEntry *cmd_list = pd->cmd_list;

  pd->only_selected = FALSE;
  pd->multi_select = FALSE;
  pd->ballot_selected = "☑ ";
  pd->ballot_unselected = "☐ ";
  find_arg_str("-ballot-selected-str", &(pd->ballot_selected));
  find_arg_str("-ballot-unselected-str", &(pd->ballot_unselected));
  if (find_arg("-multi-select") >= 0) {
    pd->multi_select = TRUE;
  }
  if (find_arg("-markup-rows") >= 0) {
    pd->do_markup = TRUE;
  }
  if (find_arg("-only-match") >= 0 || find_arg("-no-custom") >= 0) {
    pd->only_selected = TRUE;
    if (cmd_list_length == 0) {
      return TRUE;
    }
  }
  if (config.auto_select && cmd_list_length == 1) {
    rofi_output_formatted_line(pd->format, cmd_list[0].entry, 0, config.filter);
    return TRUE;
  }
  if (find_arg("-password") >= 0) {
    menu_flags |= MENU_PASSWORD;
  }
  /* copy filter string */
  input = g_strdup(config.filter);

  char *select = NULL;
  find_arg_str("-select", &select);
  if (select != NULL) {
    rofi_int_matcher **tokens = helper_tokenize(select, config.case_sensitive);
    unsigned int i = 0;
    for (i = 0; i < cmd_list_length; i++) {
      if (helper_token_match(tokens, cmd_list[i].entry)) {
        pd->selected_line = i;
        break;
      }
    }
    helper_tokenize_free(tokens);
  }
  if (find_arg("-dump") >= 0) {
    rofi_int_matcher **tokens = helper_tokenize(
        config.filter ? config.filter : "", config.case_sensitive);
    unsigned int i = 0;
    for (i = 0; i < cmd_list_length; i++) {
      if (tokens == NULL || helper_token_match(tokens, cmd_list[i].entry)) {
        rofi_output_formatted_line(pd->format, cmd_list[i].entry, i,
                                   config.filter);
      }
    }
    helper_tokenize_free(tokens);
    dmenu_mode_free(&dmenu_mode);
    g_free(input);
    return TRUE;
  }
  find_arg_str("-p", &(dmenu_mode.display_name));
  RofiViewState *state =
      rofi_view_create(&dmenu_mode, input, menu_flags, dmenu_finalize);

  if (find_arg("-keep-right") >= 0) {
    rofi_view_ellipsize_start(state);
  }
  rofi_view_set_selected_line(state, pd->selected_line);
  rofi_view_set_active(state);
  if (pd->loading) {
    rofi_view_set_overlay(state, "Loading.. ");
  }

  return FALSE;
}

void print_dmenu_options(void) {
  int is_term = isatty(fileno(stdout));
  print_help_msg(
      "-mesg", "[string]",
      "Print a small user message under the prompt (uses pango markup)", NULL,
      is_term);
  print_help_msg("-p", "[string]", "Prompt to display left of entry field",
                 NULL, is_term);
  print_help_msg("-selected-row", "[integer]", "Select row", NULL, is_term);
  print_help_msg("-format", "[string]", "Output format string", "s", is_term);
  print_help_msg("-u", "[list]", "List of row indexes to mark urgent", NULL,
                 is_term);
  print_help_msg("-a", "[list]", "List of row indexes to mark active", NULL,
                 is_term);
  print_help_msg("-l", "[integer] ", "Number of rows to display", NULL,
                 is_term);
  print_help_msg("-window-title", "[string] ", "Set the dmenu window title",
                 NULL, is_term);
  print_help_msg("-i", "", "Set filter to be case insensitive", NULL, is_term);
  print_help_msg("-only-match", "",
                 "Force selection to be given entry, disallow no match", NULL,
                 is_term);
  print_help_msg("-no-custom", "", "Don't accept custom entry, allow no match",
                 NULL, is_term);
  print_help_msg("-select", "[string]", "Select the first row that matches",
                 NULL, is_term);
  print_help_msg("-password", "",
                 "Do not show what the user inputs. Show '*' instead.", NULL,
                 is_term);
  print_help_msg("-markup-rows", "",
                 "Allow and render pango markup as input data.", NULL, is_term);
  print_help_msg("-sep", "[char]", "Element separator.", "'\\n'", is_term);
  print_help_msg("-input", "[filename]",
                 "Read input from file instead from standard input.", NULL,
                 is_term);
  print_help_msg("-sync", "",
                 "Force dmenu to first read all input data, then show dialog.",
                 NULL, is_term);
  print_help_msg("-w", "windowid", "Position over window with X11 windowid.",
                 NULL, is_term);
  print_help_msg("-keep-right", "", "Set ellipsize to end.", NULL, is_term);
  print_help_msg("-display-columns", "", "Only show the selected columns", NULL,
                 is_term);
  print_help_msg("-display-column-separator", "\t",
                 "Separator to use to split columns (regex)", NULL, is_term);
  print_help_msg("-ballot-selected-str", "\t",
                 "When multi-select is enabled prefix this string when element "
                 "is selected.",
                 NULL, is_term);
  print_help_msg("-ballot-unselected-str", "\t",
                 "When multi-select is enabled prefix this string when element "
                 "is not selected.",
                 NULL, is_term);
}
