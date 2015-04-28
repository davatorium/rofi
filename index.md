---
layout: default
github: DaveDavenport
title: Rofi
subtitle: A window switcher, run dialog and dmenu replacement
---

A popup window switcher roughly based on [superswitcher](http://code.google.com/p/superswitcher/),
requiring only xlib and pango.
This version started off as a clone of simpleswitcher, the version from [Sean
Pringle](http://github.com/seanpringle/simpleswitcher). All credit for this great tool should go to him.
Rofi developed extra features, like a run-dialog, ssh-launcher and can act as a drop-in dmenu
replacement, making it a very versatile tool.

Using Rofi is a lot like dmenu, but extended for an improved work flow.
It main features are:

* Full keyboard navigation.
* Type to filter
    - Tokenized: Type any word in any order to filter.
    - Case insensitive
* UTF-8 enabled.
    - UTF-8 aware string collating.
* Pango font rendering.
* RTL language support.
* Window Switcher.
    - I3 support.
* Run dialog.
* SSH launcher.
* History based ordering last 25 choices are ordered on top based on use. (optional)
* Levenshtein distance ordering of matches. (optional)
* Drop in dmenu replacement.
* Can be easily extended using scripts.

The 4 Main functions of rofi are:

## Window Switcher

![Window List](images/rofi/window-list.png)

The window switcher shows the following informations in columns:

1. Desktop number (optional, not shown in i3 mode)
2. Window class.
3. Window title.

If compiled with I3 support, it should autodetect if I3 window manager is running and switch into
I3 compatibility mode. This will disable Desktop numbers and hide the i3-bar, also it sends an IPC
message to I3 to change focus.

## Run dialog

![run dialog](images/rofi/run-dialog.png)

The run dialog allows the user to quickly search and launch a program.
It offers the following features:

  - Shift-Return to run the selected program in a terminal.
  - Favorites list, frequently used programs are sorted on top.

## SSH launcher

![SSH Launcher](images/rofi/ssh-dialog.png)

Quickly ssh into remote machines

  - Parses ~/.ssh/config to find hosts.


## dmenu replacement

![DMENU replacement (running teiler)](images/rofi/dmenu-replacement.png)

Drop in dmenu replacement. (Screenshot shows rofi used by
[teiler](https://github.com/carnager/teiler) ).

# Usage

If used with `-show [mode]`, rofi will immediately open in the specified [mode]

If used with `-dmenu`, rofi will use data from STDIN to let the user select an option.

If use with neither of those options, rofi will launch in daemon-mode, waiting for a key (configured beforehand) to launch.

## Single-shot

Show a run dialog with some font / color options:

  `rofi -show run -font "snap 10" -fg "#505050" -bg "#000000" -hlfg "#ffb964" -hlbg "#000000" -o 85`

Show a ssh dialog:

  `rofi -show ssh`

## dmenu

If passed the `-dmenu` option, or ran as `dmenu` (ie, /usr/bin/dmenu is symlinked to /usr/bin/rofi),
rofi will use the data passed from STDIN.

`~/scripts/my_script.sh | rofi -dmenu`
`echo -e "Option #1\nOption #2\nOption #3\n" | rofi -dmenu`

In both cases, rofi will output the user's selection to STDOUT.

## Daemon mode

Let rofi sit in the background, waiting for the user to press `F12` to open the window run dialog:

`rofi -key-run F12`

## Switching Between Modi

Type `Shift-Right` to switch from Window list mode to Run mode and back.


## Keybindings

| Key                         | Action                                                             |
|:----------------------------|:-------------------------------------------------------------------|
|`Ctrl-v, Insert`             | Paste clipboard |
|`Ctrl-Shift-v, Shift-Insert` | Paste primary selection |
|`Ctrl-u`                     | Clear the line |
|`Ctrl-a`                     | Beginning of line |
|`Ctrl-e`                     | End of line |
|`Ctrl-f, Right`              | Forward one character |
|`Alt-f`                      | Forward one word |
|`Ctrl-b, Left`               | Back one character |
|`Alt-b`                      | Back one word |
|`Ctrl-d, Delete`             | Delete character |
|`Ctrl-Alt-d`                 | Delete word |
|`Ctrl-h, Backspace`          | Backspace (delete previous character) |
|`Ctrl-Alt-h`                 | Delete previous word |
|`Ctrl-j,Ctrl-m,Enter`        | Accept entry |
|`Ctrl-n,Down`                | Select next entry |
|`Ctrl-p,Up`                  | Select previous entry |
|`Page Up`                    | Go to the previous page |
|`Page Down`                  | Go to the next page |
|`Ctrl-Page Up`               | Go to the previous column |
|`Ctrl-Page Down`             | Go to the next column |
|`Ctrl-Enter`                 | Use entered text as command (in ssh/run modi) |
|`Shift-Enter`                | Launch the application in a terminal (in run mode) |
|`Shift-Enter`                | Return the selected entry and move to the next item while keeping Rofi open. (in dmenu) |
|`Shift-Right`                | Switch to the next modi. The list can be customized with the -switchers argument. |
|`Shift-Left`                 | Switch to the previous modi. The list can be customized with the -switchers argument. |
|`Ctrl-Tab`                   | Switch to the next modi. The list can be customized with the -switchers argument. |
|`Ctrl-Shift-Tab`             | Switch to the previous modi. The list can be customized with the -switchers argument. |
|`Ctrl-space`                 | Set selected item as input text. |
|`Shift-Del`                  | Delete entry from history. |
|`Ctrl-grave`                 | Toggle case sensitivity. |


# Configuration

There are currently three methods of setting configuration options:

 * Compile time: edit config.c. This method is strongly discouraged.
 * Xresources: A method of storing key values in the Xserver. See
   [here](https://en.wikipedia.org/wiki/X_resources) for more information.
 * Commandline options: Arguments passed to **rofi**.

The Xresources options and the commandline options are aliased. So to set option X you would set:

`rofi.X: value`

In the Xresources file, and to (override) this via the commandline you would pass the same key
prefixed with a '-':

    rofi -X value

To get a list of available options, formatted as Xresources entries run:

    rofi -dump-Xresources

The configuration system supports the following types:

 * String
 * Integer (signed and unsigned)
 * Char
 * Boolean

The boolean option has a non-default commandline syntax, to enable option X  you do:

	rofi -X

to disable it:

	rofi -no-X

# Installation

Please see the [installation guide](INSTALL.html) for instruction on how to install *rofi*.

# Contact

Issues, feature requests can be filed at the github [issue
tracker](https://github.com/DaveDavenport/rofi/issues). Please read the *reporting bugs* section
below.

If you need to ask a direct question or get support installing, please find us on IRC: #gmpc on
[freenode.net](https://webchat.freenode.net/?channels=#gmpc).


# Reporting Bugs

When reporting bugs keep in mind that the people working on it do this unpaid, in their free time
and as a hobby. So be polite and helpful. Bug reports that *demand*, contain *insults* to this
or other projects, or have a general unfriendly tone will be closed without discussion. Keep in mind
that everybody has it own way of working; What might be the *norm* for you, might not be for others.
