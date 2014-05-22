---
layout: default
github: DaveDavenport
title: Screenshots
---


This place lists a set of nice setups for rofi.
You can easily get your current configuration by typing:

    rofi -dump-xresources

## Dropdown menu

[ ![Rofi in dropdown menu mode](images/rofi/screenshots/dropdown.png)
](images/rofi/screenshots/dropdown.png)

Xresources file:

        rofi.opacity: 90
        rofi.width: 50
        rofi.lines: 5
        rofi.columns: 1
        rofi.font: SourceCodePro-9
        rofi.fg: #1aa
        rofi.bg: #333
        rofi.hlfg: #333
        rofi.hlbg: #1aa
        rofi.bc: #1aa
        rofi.bw: 2
        rofi.loc: 0
        rofi.padding: 2
        rofi.yoffset: -2
        rofi.xoffset: 0
        rofi.fixed-num-lines: false
        rofi.hmode: false
        rofi.terminal: x-terminal-emulator
        rofi.ssh-set-title: true
        rofi.key: F12
        rofi.rkey: mod1+F2
        rofi.skey: mod1+F3

## DMenu mode: Well hidden

[ ![Rofi dmenu mode: Well Hidden](images/rofi/screenshots/dmenu_greg.jpg)
](images/rofi/screenshots/dmenu_greg.jpg)

Config file `config/config.c`:

    Settings config = {
        // Set the default window opacity.
        // This option only works when running a composite manager.
        // -o
        .window_opacity = 100,
        // Border width around the window.
        .menu_bw = 1,
        // The width of the switcher. (0-100 in % > 100 in pixels)
        .menu_width = 100,
        // Maximum number of options to show.
        .menu_lines = 15,
        // Font
        .menu_font = "Inconsolata:pixelsize=14",
         
        // Foreground color
        .menu_fg = "#eee8d5",
        // Background color
        .menu_bg = "#073642",
        // Foreground color (selected)
        .menu_hlfg = "#ffffff",
        // Background color (selected)
        .menu_hlbg = "#cb4b16",
        // Border color.
        .menu_bc = "b58900",
        // Directly select when only 1 choice is left
        .zeltak_mode = 0,
        // Terminal to use. (for ssh and open in terminal)
        .terminal_emulator = "x-terminal-emulator",
        #ifdef I3
        // Auto-detected. no longer used.
        .i3_mode = 0,
        #endif
        // Key binding
        .window_key = "F12",
        .run_key = "mod1+F2",
        .ssh_key = "mod1+F3",
        // Location of the window. WL_CENTER, WL_NORTH_WEST, WL_NORTH, WL_NORTH_EAST, etc.
        .location = WL_SOUTH,
        // Mode of window, list (Vertical) or dmenu like (Horizontal)
        .wmode = HORIZONTAL,
        // Padding of the window.
        .padding = 1,
        .show_title = 1,
        .y_offset = 0,
        .x_offset = 0,
        .fixed_num_lines = 0
    };
