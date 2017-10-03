/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2017 Qball Cow <qball@gmpclient.org>
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
typedef enum
{
    MM_NORMAL = 0,
    MM_REGEX  = 1,
    MM_GLOB   = 2,
    MM_FUZZY  = 3
} MatchingMethod;

/**
 * Settings structure holding all (static) configurable options.
 * @ingroup CONFIGURATION
 */
typedef struct
{
    /** List of enabled modi */
    char           *modi;
    /** Border width */
    unsigned int   menu_bw;
    /** Width (0-100 in %, > 100 in pixels, < 0 in char width.) */
    int            menu_width;
    /** # lines */
    unsigned int   menu_lines;
    /** # Columns */
    unsigned int   menu_columns;
    /** Font string (pango format) */
    char           * menu_font;

    /** New row colors */
    char           * color_normal;
    char           * color_active;
    char           * color_urgent;
    char           * color_window;

    /** Whether to load and show icons  */
    gboolean       show_icons;

    /** Terminal to use  */
    char           * terminal_emulator;
    /** SSH client to use */
    char           * ssh_client;
    /** Command to execute when ssh session is selected */
    char           * ssh_command;
    /** Command for executing an application */
    char           * run_command;
    /** Command for executing an application in a terminal */
    char           * run_shell_command;
    /** Command for listing executables */
    char           * run_list_command;
    /** Command for window */
    char           * window_command;
    /** Theme for icons */
    char           * drun_icon_theme;

    /** Windows location/gravity */
    WindowLocation location;
    /** Padding between elements */
    unsigned int   padding;
    /** Y offset */
    int            y_offset;
    /** X offset */
    int            x_offset;
    /** Always should config.menu_lines lines, even if less lines are available */
    unsigned int   fixed_num_lines;
    /** Do not use history */
    unsigned int   disable_history;
    /** Toggle to enable sorting. */
    unsigned int   sort;
    /** Use levenshtein sorting when matching */
    unsigned int   levenshtein_sort;
    /** Search case sensitivity */
    unsigned int   case_sensitive;
    /** Cycle through in the element list */
    unsigned int   cycle;
    /** Height of an element in number of rows */
    int            element_height;
    /** Sidebar mode, show the modi */
    unsigned int   sidebar_mode;
    /** Lazy filter limit. */
    unsigned int   lazy_filter_limit;
    /** Auto select. */
    unsigned int   auto_select;
    /** Hosts file parsing */
    unsigned int   parse_hosts;
    /** Knonw_hosts file parsing */
    unsigned int   parse_known_hosts;
    /** Combi Modes */
    char           *combi_modi;
    char           *matching;
    MatchingMethod matching_method;
    unsigned int   tokenize;
    /** Monitors */
    char           *monitor;
    /** Line margin */
    unsigned int   line_margin;
    unsigned int   line_padding;
    /** filter */
    char           *filter;
    /** style */
    char           *separator_style;
    /** hide scrollbar */
    unsigned int   hide_scrollbar;
    /** fullscreen */
    unsigned int   fullscreen;
    /** bg image */
    unsigned int   fake_transparency;
    /** dpi */
    int            dpi;
    /** Number threads (1 to disable) */
    unsigned int   threads;
    unsigned int   scroll_method;
    unsigned int   scrollbar_width;
    /** Background type */
    char           *fake_background;

    char           *window_format;
    /** Click outside the window to exit */
    int            click_to_exit;
    gboolean       show_match;

    char           *theme;
    /** Path where plugins can be found. */
    char           * plugin_path;

    /** Maximum history length per mode. */
    unsigned int   max_history_size;
} Settings;
/** Global Settings structure. */
extern Settings config;
#endif // ROFI_SETTINGS_H
