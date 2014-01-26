/**
 * simpleswitcher
 *
 * MIT/X11 License
 * Modified 2013-2014 Qball  Cow <qball@gmpclient.org>
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
#include "simpleswitcher.h"

Settings config = {
    // Window settings
    .window_opacity = 100,
    // Menu settings
    .menu_bw        = 1,
    .menu_width     = 50,
    .menu_lines     = 15,
    .menu_font      = "mono-12",
    .menu_fg        = "#222222",
    .menu_bg        = "#f2f1f0",
    .menu_bgalt     = "#e9e8e7",
    .menu_hlfg      = "#ffffff",
    .menu_hlbg      = "#005577",
    .menu_bc        = "black",
    // Behavior
    .zeltak_mode    = 0,
    .terminal_emulator = "x-terminal-emulator",
    .i3_mode        = 0,
    // Key binding
    .window_key     = "F12",
    .run_key        = "mod1+F2",
    .ssh_key        = "mod1+F3",
#ifdef I3
    .mark_key		= "mod1+F5",
#endif
    .location       = CENTER,
    .wmode          = VERTICAL,
    .inner_margin   = 5
};
