/**
 * rofi
 *
 * MIT/X11 License
 * Modified 2013-2015 Qball  Cow <qball@gmpclient.org>
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
#include <stdio.h>
#include <stdlib.h>
#include "rofi.h"

Settings config = {
    /** List of enabled switchers. */
    /** -switchers */
    .switchers         = "window,run,ssh",
    /** Set the default window opacity. */
    /** This option only works when running a composite manager. */
    /** -o */
    .window_opacity    =                                       100,
    /** Border width around the window. */
    .menu_bw           =                                         1,
    /** The width of the switcher. (0100 in % > 100 in pixels) */
    .menu_width        =                                        50,
    /** Maximum number of options to show. */
    .menu_lines        =                                        15,
    /** Number of columns */
    .menu_columns      =                                         1,
    /** Font */
    .menu_font         = "mono 12",

    /** Row colors */
    // Enable new color
    .color_enabled = FALSE,
    .color_normal  = "#fdf6e3,#002b36,#eee8d5,#586e75,#eee8d5",
    .color_urgent  = "#fdf6e3,#dc322f,#eee8d5,#dc322f,#fdf6e3",
    .color_active  = "#fdf6e3,#268bd2,#eee8d5,#268bd2,#fdf6e3",
    .color_window  = "#fdf6e3,#002b36",

    /** Background color */
    .menu_bg           = "#FDF6E3",
    /** Border color. */
    .menu_bc           = "#002B36",
    /** Foreground color */
    .menu_fg           = "#002B36",
    /** Text color used for urgent windows */
    .menu_fg_urgent    = "#DC322F",
    /** Text color used for active window */
    .menu_fg_active = "#268BD2",
    .menu_bg_urgent = "#FDF6E3",
    .menu_bg_active = "#FDF6E3",
    /** Background color alternate row */
    .menu_bg_alt       = NULL,                                     //"#EEE8D5",
    /** Foreground color (selected) */
    .menu_hlfg        = "#EEE8D5",
    .menu_hlfg_urgent = "#FDF6E3",
    .menu_hlfg_active = "#FDF6E3",
    /** Background color (selected) */
    .menu_hlbg        = "#586E75",
    .menu_hlbg_urgent = "#DC322F",
    .menu_hlbg_active = "#268BD2",
    /** Terminal to use. (for ssh and open in terminal) */
    .terminal_emulator = "x-terminal-emulator",
    .ssh_client        = "ssh",
    /** Command when executing ssh. */
    .ssh_command       = "{terminal} -e {ssh-client} {host}",
    /** Command when running */
    .run_command       = "{cmd}",
    /** Command used to list executable commands. empty -> internal */
    .run_list_command  = "",
    /** Command executed when running application in terminal */
    .run_shell_command = "{terminal} -e {cmd}",
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
    .location          = WL_CENTER,
    /** Padding between elements */
    .padding           =                                         5,
    /** Y offset */
    .y_offset          =                                         0,
    /** X offset */
    .x_offset          =                                         0,
    /** Always should config.menu_lines lines, even if less lines are available */
    .fixed_num_lines   = FALSE,
    /** Do not use history */
    .disable_history   = FALSE,
    /** Use levenshtein sorting when matching */
    .levenshtein_sort  = TRUE,
    /** Case sensitivity of the search */
    .case_sensitive    = FALSE,
    /** Separator to use for dmenu mode */
    .separator         = '\n',
    /** Height of an element in #chars */
    .element_height    =                                         1,
    /** Sidebar mode, show the switchers */
    .sidebar_mode      = FALSE,
    /** Lazy mode setting */
    .lazy_filter_limit =                                      5000,
    /** auto select */
    .auto_select       = FALSE,
    /** Parse /etc/hosts file in ssh view. */
    .parse_hosts       = FALSE,
    /** Modi to combine into one view. */
    .combi_modi        = "window,run"
};

