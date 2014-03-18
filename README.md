# Rofi 

A popup window switcher roughly based on [superswitcher](http://code.google.com/p/superswitcher/), requiring only xlib and xft.
This version is based on the version from [Sean
Pringle](http://github.com/seanpringle/simpleswitcher). All credit for this great tool should go to him.

Some of the features of rofi are:

* Switcher centered on screen (or monitor for multi-head).

* Vertical list with two columns:
    1. Desktop number (optional, not shown in i3 mode)
	2. Window class.
	3. Window title.

* Controls like dmenu:
	* Type to filter windows.
    * Tokonized filter. Type any word in any order to filter.
	* Arrows to highlight selections.
	* Return to select.

* Rudimentary run dialog
    * Type to filter.
    * Tokonized filter. Type any word in any order to filter.
	* Arrows to highlight selections.
	* Return to select.
    * Shift-Return to run in terminal.
    * Favorites list.

* Quickly ssh into remote machines
    * Parses ~/.ssh/config to find hosts.

* Runs in background or once-off.

* Drop in dmenu replacement.

If compiled with I3 support, it should autodetect if I3 window manager is running. 

## Usage

e.g.

  bindsym $mod+Tab exec rofi -now -font "snap-10" -fg "#505050" -bg "#000000" -hlfg "#ffb964" -hlbg "#000000" -o 85

## Switching Between Modi

Type '?' (enter)  to switch from Window list mode to Run mode and back.

## Compilation

If compiling from GIT, first run to generate the needed build files:

    autoreconf --install

To build rofi, run the following steps:

    mkdir build/
    cd build/
    ../configure
    make
    make install

The build system will autodetect the i3 header file during compilation. If it fails, make sure you 
have i3/ipc.h installed. Check config.log for more information. 

## Dependencies

Rofi requires the following tools and libraries to be installed:

 * libx11
 * libxinerama
 * libxdg-basedir
 * libxft 

## Configuration

There are 3 ways to configure rofi:

### 1. Pre-compile time

You can change the default behavior by modifying config/config.c

### 2. Xresources

Another solution is to configure it via X resources, e.g. add the following to your 
Xresources file:

    rofi.background: #333
    rofi.foreground: #1aa
    rofi.highlightbg: #1aa
    rofi.highlightfg: #111
    rofi.bordercolor: #277
    rofi.font: times-10
    rofi.padding: 3
    rofi.lines: 5
    rofi.borderwidth: 3

### 3. Runtime

All the above settings can be overridden by rofi's commandline flags.

## Archlinux

This version of rofi has been made available on the
[AUR](https://aur.archlinux.org/packages/rofi-git/).

# Contact

Issues, feature requests can be filed at the github [issue
tracker](https://github.com/DaveDavenport/rofi/issues).

If you need to ask a direct question or get support installing, please find us on IRC: #gmpc on
[freenode.net](https://webchat.freenode.net/?channels=#gmpc).

