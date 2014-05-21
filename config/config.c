/**
 * rofi
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
#include <stdio.h>
#include <stdlib.h>
#include "rofi.h"

Settings config = {
    // Set the default window opacity.
    // This option only works when running a composite manager.
    // -o
    .window_opacity    =                   100,
    // Border width around the window.
    .menu_bw           =                     1,
    // The width of the switcher. (0100 in % > 100 in pixels)
    .menu_width        =                    50,
    // Maximum number of options to show.
    .menu_lines        =                    15,
    // Number of columns
    .menu_columns      =                     1,
    // Font
    .menu_font         = "mono12",
    // Foreground color
    .menu_fg           = "#222222",
    // Background color
    .menu_bg           = "#f2f1f0",
    // Foreground color (selected)
    .menu_hlfg         = "#ffffff",
    // Background color (selected)
    .menu_hlbg         = "#005577",
    // Border color.
    .menu_bc           = "black",
    // Directly select when only 1 choice is left
    .zeltak_mode       =                     0,
    // Terminal to use. (for ssh and open in terminal)
    .terminal_emulator = "x-terminal-emulator",
#ifdef I3
    // Auto-detected. no longer used.
    .i3_mode           =                     0,
#endif
    // Key binding
    .window_key = "F12",
    .run_key    = "mod1+F2",
    .ssh_key    = "mod1+F3",
    // Location of the window.   WL_CENTER, WL_NORTH_WEST, WL_NORTH,WL_NORTH_EAST, etc.
    .location          = WL_CENTER,
    // Mode of window, list (Vertical) or dmenu like (Horizontal)
    .hmode             = FALSE,
    // Padding of the window.
    .padding           = 5,
    .show_title        = 1,
    .y_offset          = 0,
    .x_offset          = 0,
    .fixed_num_lines   = FALSE
};

/**
 * Do some input validation, especially the first few could break things.
 * It is good to catch them beforehand.
 *
 * This functions exits the program with 1 when it finds an invalid configuration.
 */
void config_sanity_check( void )
{
    if ( config.menu_lines == 0 ) {
        fprintf(stderr, "config.menu_lines is invalid. You need at least one visible line.\n");
        exit(1);
    }
    if ( config.menu_columns == 0 ) {
        fprintf(stderr, "config.menu_columns is invalid. You need at least one visible column.\n");
        exit(1);
    }

    if ( config.menu_width == 0 ) {
        fprintf(stderr, "config.menu_width is invalid. You cannot have a window with no width.\n");
        exit(1);
    }

    if ( !( config.location >= WL_CENTER &&  config.location <= WL_WEST ) )
    {
        fprintf(stderr, "config.location is invalid. ( %d >= %d >= %d) does not hold.\n",
                WL_WEST, config.location, WL_CENTER);
        exit(1);
    }

    if ( !( config.hmode == TRUE || config.hmode == FALSE ) )
    {
        fprintf(stderr, "config.hmode is invalid.\n");
        exit(1);
    }
}

/**
 * Print out the current configuration.
 */
void config_print( void )
{
    printf("Windows opacity:                      %3d%%\n",   config.window_opacity);
    printf("Border width:                         %3d\n",     config.menu_bw);
    printf("Padding:                              %3d\n",     config.padding);
    printf("Width:                               %4d%s\n",    config.menu_width,
            config.menu_width > 100? "px":"%");
    printf("offset (x,y):                     (%2d,%2d)px\n", config.x_offset, config.y_offset);
    printf("Location:                      ");
    switch(config.location)
    {
        case WL_CENTER:     printf("    Center\n"); break;
        case WL_NORTH_WEST: printf("North West\n"); break;
        case WL_NORTH:      printf("     North\n"); break;
        case WL_NORTH_EAST: printf("North East\n"); break;
        case WL_EAST:       printf("      East\n"); break;
        case WL_EAST_SOUTH: printf("East South\n"); break;
        case WL_SOUTH:      printf("     South\n"); break;
        case WL_SOUTH_WEST: printf("South West\n"); break;
        case WL_WEST:       printf("      West\n"); break;
        default:            printf("   Invalid\n"); break;
    }
    printf("# Lines:                              %3d\n",     config.menu_lines);
    printf("# Columns:                            %3d\n",     config.menu_columns);
    printf("Fixed number of lines:              %5s\n",       config.fixed_num_lines?"true":"false");
    printf("Horizontal model:                   %5s\n",       config.hmode == TRUE?"true":"false");


    printf("Font: %35s\n",                          config.menu_font);
    /* Colors */
    printf("FG Color:                         %7s\n",         config.menu_fg);
    printf("BG Color:                         %7s\n",         config.menu_bg);
    printf("Highlight FG Color:               %7s\n",         config.menu_hlfg);
    printf("Highlight BG Color:               %7s\n",         config.menu_hlbg);
    printf("Border color:                     %7s\n",         config.menu_bc);

    /* Terminal */
    printf("Terminal emulator: %22s\n",             config.terminal_emulator);
    /* Keybindings. */
    printf("Window switcher key:              %7s\n",         config.window_key);
    printf("Run dialog key:                   %7s\n",         config.run_key);
    printf("SSH dialog key:                   %7s\n",         config.ssh_key);
}
