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

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include "rofi-types.h"
#include "settings.h"

Settings config = {
    /** List of enabled modi. */
    /** -modi */
#ifdef WINDOW_MODE
    .modi                      = "window,run,ssh",
#else
    .modi                      = "run,ssh",
#endif
    /** Border width around the window. */
    .menu_bw                   = 1,
    /** The width of the switcher. (0100 in % > 100 in pixels) */
    .menu_width                = 50,
    /** Maximum number of options to show. */
    .menu_lines                = 15,
    /** Number of columns */
    .menu_columns              = 1,
    /** Font */
    .menu_font                 = "mono 12",

    /** Whether to load and show icons */
    .show_icons                = FALSE,

    /** Terminal to use. (for ssh and open in terminal) */
    .terminal_emulator = "rofi-sensible-terminal",
    .ssh_client        = "ssh",
    /** Command when executing ssh. */
    .ssh_command               = "{terminal} -e {ssh-client} {host} [-p {port}]",
    /** Command when running */
    .run_command               = "{cmd}",
    /** Command used to list executable commands. empty -> internal */
    .run_list_command          = "",
    /** Command executed when running application in terminal */
    .run_shell_command         = "{terminal} -e {cmd}",
    /** Command executed on accep-entry-custom for window modus */
    .window_command            = "wmctrl -i -R {window}",
    /** No default icon theme, we search Adwaita and gnome as fallback */
    .icon_theme                = NULL,
    /**
     * Location of the window.
     * Enumeration indicating location or gravity of window.
     *
     * WL_NORTH_WEST      WL_NORTH      WL_NORTH_EAST
     *
     * WL_EAST            WL_CENTER     WL_EAST
     *
     * WL_SOUTH_WEST      WL_SOUTH      WL_SOUTH_EAST
     *
     */
    .location                  = WL_CENTER,
    /** Padding between elements */
    .padding                   = 5,
    /** Y offset */
    .y_offset                  = 0,
    /** X offset */
    .x_offset                  = 0,
    /** Always show config.menu_lines lines, even if less lines are available */
    .fixed_num_lines           = TRUE,
    /** Do not use history */
    .disable_history           = FALSE,
    /** Programs ignored for history */
    .ignored_prefixes          = "",
    /** Sort the displayed list */
    .sort                      = FALSE,
    /** Use levenshtein sorting when matching */
    .sorting_method            = "normal",
    /** Case sensitivity of the search */
    .case_sensitive            = FALSE,
    /** Cycle through in the element list */
    .cycle                     = TRUE,
    /** Height of an element in #chars */
    .element_height            = 1,
    /** Sidebar mode, show the modi */
    .sidebar_mode              = FALSE,
    /** auto select */
    .auto_select               = FALSE,
    /** Parse /etc/hosts file in ssh view. */
    .parse_hosts               = FALSE,
    /** Parse ~/.ssh/known_hosts file in ssh view. */
    .parse_known_hosts         = TRUE,
    /** Modi to combine into one view. */
    .combi_modi      = "window,run",
    .tokenize        = TRUE,
    .matching        = "normal",
    .matching_method = MM_NORMAL,

    /** Desktop entries to match in drun */
    .drun_match_fields         = "name,generic,exec,categories,keywords",
    /** Only show entries in this category */
    .drun_categories           = NULL,
    /** Desktop entry show actions */
    .drun_show_actions         = FALSE,
    /** Desktop format display */
    .drun_display_format       = "{name} [<span weight='light' size='small'><i>({generic})</i></span>]",
    /** Desktop Link launch command */
    .drun_url_launcher         = "xdg-open",

    /** Window fields to match in window mode*/
    .window_match_fields       = "all",
    /** Monitor */
    .monitor                   = "-5",
    /** set line margin */
    .line_margin  = 2,
    .line_padding = 1,
    /** Set filter */
    .filter                    = NULL,
    /** Separator style: dash/solid */
    .separator_style           = "dash",
    /** Hide scrollbar */
    .hide_scrollbar         = FALSE,
    .fake_transparency      = FALSE,
    .dpi                    = -1,
    .threads                = 0,
    .scroll_method          = 0,
    .scrollbar_width        = 8,
    .fake_background        = "screenshot",
    .window_format          = "{w}    {c}   {t}",
    .click_to_exit          = TRUE,
    .theme                  = NULL,
    .color_normal           = NULL,
    .color_active           = NULL,
    .color_urgent           = NULL,
    .color_window           = NULL,
    .plugin_path            = PLUGIN_PATH,
    .max_history_size       = 25,
    .combi_hide_mode_prefix = FALSE,

    .matching_negate_char      = '-',

    .cache_dir        = NULL,
    .window_thumbnail = FALSE,

    /** drun cache */
    .drun_use_desktop_cache    = FALSE,
    .drun_reload_desktop_cache = FALSE,

    /** Benchmarks */
    .benchmark_ui              = FALSE,

    /** normalize match */
    .normalize_match           = FALSE,
    /** steal focus */
    .steal_focus               = FALSE 
};
