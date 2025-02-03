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

#ifndef ROFI_SETTINGS_H
#define ROFI_SETTINGS_H

#include <glib.h>

/**
 * Enumeration indicating the matching method to use.
 *
 * @ingroup CONFIGURATION
 */
typedef enum {
  MM_NORMAL = 0,
  MM_REGEX = 1,
  MM_GLOB = 2,
  MM_FUZZY = 3,
  MM_PREFIX = 4,
  MM_NUM_MATCHERS = 5
} MatchingMethod;

/**
 * Possible sorting methods for listview.
 */
typedef enum { SORT_NORMAL = 0, SORT_FZF = 1 } SortingMethod;

/**
 * Settings structure holding all (static) configurable options.
 * @ingroup CONFIGURATION
 */
typedef struct {
  /** List of enabled modes */
  char *modes;
  /** Font string (pango format) */
  char *menu_font;

  /** Whether to load and show icons  */
  gboolean show_icons;
  
  /** Custom command to generate preview icons */
  char *preview_cmd;

  /** Custom command to call when menu selection changes */
  char *on_selection_changed;
  /** Custom command to call when menu mode changes */
  char *on_mode_changed;
  /** Custom command to call when menu entry is accepted */
  char *on_entry_accepted;
  /** Custom command to call when menu is canceled */
  char *on_menu_canceled;
  /** Custom command to call when menu finds errors */
  char *on_menu_error;
  /** Custom command to call when menu screenshot is taken */
  char *on_screenshot_taken;
  /** Terminal to use  */
  char *terminal_emulator;
  /** SSH client to use */
  char *ssh_client;
  /** Command to execute when ssh session is selected */
  char *ssh_command;
  /** Command for executing an application */
  char *run_command;
  /** Command for executing an application in a terminal */
  char *run_shell_command;
  /** Command for listing executables */
  char *run_list_command;
  /** Command for window */
  char *window_command;
  /** Window fields to match in window mode */
  char *window_match_fields;
  /** Theme for icons */
  char *icon_theme;

  /** Windows location/gravity */
  WindowLocation location;
  /** Y offset */
  int y_offset;
  /** X offset */
  int x_offset;
  /** Always should config.menu_lines lines, even if less lines are available */
  unsigned int fixed_num_lines;
  /** Do not use history */
  unsigned int disable_history;
  /** Programs ignored for history */
  char *ignored_prefixes;
  /** Toggle to enable sorting. */
  unsigned int sort;
  /** Sorting method. */
  SortingMethod sorting_method_enum;
  /** Sorting method. */
  char *sorting_method;

  /** Desktop entries to match in drun */
  char *drun_match_fields;
  /** Only show entries in this category */
  char *drun_categories;
  /** Desktop entry show actions */
  unsigned int drun_show_actions;
  /** Desktop format display */
  char *drun_display_format;
  /** Desktop Link launch command */
  char *drun_url_launcher;

  /** Search case sensitivity */
  unsigned int case_sensitive;
  /** Smart case sensitivity like vim */
  unsigned int case_smart;
  /** Cycle through in the element list */
  unsigned int cycle;
  /** Height of an element in number of rows */
  int element_height;
  /** Sidebar mode, show the modes */
  unsigned int sidebar_mode;
  /** Mouse hover automatically selects */
  gboolean hover_select;
  /** Lazy filter limit. */
  unsigned int lazy_filter_limit;
  /** Auto select. */
  unsigned int auto_select;
  /** Hosts file parsing */
  unsigned int parse_hosts;
  /** Knonw_hosts file parsing */
  unsigned int parse_known_hosts;
  /** Combi Modes */
  char *combi_modes;
  char *matching;
  MatchingMethod matching_method;
  unsigned int tokenize;
  /** Monitors */
  char *monitor;
  /** filter */
  char *filter;
  /** dpi */
  int dpi;
  /** Number threads (1 to disable) */
  unsigned int threads;
  unsigned int scroll_method;

  char *window_format;
  /** Click outside the window to exit */
  int click_to_exit;

  char *theme;
  /** Path where plugins can be found. */
  char *plugin_path;

  /** Maximum history length per mode. */
  unsigned int max_history_size;
  gboolean combi_hide_mode_prefix;
  /** Combi format display */
  char *combi_display_format;

  char matching_negate_char;

  /** Cache directory. */
  char *cache_dir;

  /** Window Thumbnails */
  gboolean window_thumbnail;

  /** drun cache */
  gboolean drun_use_desktop_cache;
  gboolean drun_reload_desktop_cache;

  /** Benchmark */
  gboolean benchmark_ui;

  gboolean normalize_match;
  /** Steal focus */
  gboolean steal_focus;
  /** fallback icon */
  char *application_fallback_icon;

  /** refilter timeout limit, when more then these entries,go into timeout mode.
   */
  unsigned int refilter_timeout_limit;

  /** workaround for broken xserver (#300 on xserver, #611) */
  gboolean xserver_i300_workaround;
  /** completer mode */
  char *completer_mode;
} Settings;

/** Default number of lines in the list view */
#define DEFAULT_MENU_LINES 15
/** Default number of columns in the list view */
#define DEFAULT_MENU_COLUMNS 1
/** Default window width */
#define DEFAULT_MENU_WIDTH 50.0f

/** Global Settings structure. */
extern Settings config;
#endif // ROFI_SETTINGS_H
