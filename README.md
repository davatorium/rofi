[![Codacy Badge](https://api.codacy.com/project/badge/Grade/ca0310962a7c4b829d0c57f1ab023531)](https://app.codacy.com/app/davatorium/rofi?utm_source=github.com&utm_medium=referral&utm_content=davatorium/rofi&utm_campaign=Badge_Grade_Settings)
[![Build Status](https://travis-ci.org/davatorium/rofi.svg?branch=master)](https://travis-ci.org/davatorium/rofi)
[![codecov.io](https://codecov.io/github/davatorium/rofi/coverage.svg?branch=master)](https://codecov.io/github/davatorium/rofi?branch=master)
[![Issues](https://img.shields.io/github/issues/davatorium/rofi.svg)](https://github.com/davatorium/rofi/issues)
[![Forks](https://img.shields.io/github/forks/davatorium/rofi.svg)](https://github.com/davatorium/rofi/network)
[![Stars](https://img.shields.io/github/stars/davatorium/rofi.svg)](https://github.com/davatorium/rofi/stargazers)
[![Downloads](https://img.shields.io/github/downloads/davatorium/rofi/total.svg)](https://github.com/davatorium/rofi/releases)
[![Coverity](https://scan.coverity.com/projects/3850/badge.svg)](https://scan.coverity.com/projects/davedavenport-rofi)
[![Forum](https://img.shields.io/badge/forum-online-green.svg)](https://reddit.com/r/qtools/)
 [![Packages](https://repology.org/badge/tiny-repos/rofi.svg)](https://repology.org/metapackage/rofi/versions)

# A window switcher, Application launcher and dmenu replacement

**Rofi** started as a clone of simpleswitcher, written by [Sean Pringle](http://github.com/seanpringle/simpleswitcher) - a
popup window switcher roughly based on [superswitcher](http://code.google.com/p/superswitcher/).
Simpleswitcher laid the foundations, and therefore Sean Pringle deserves most of the credit for this tool. **Rofi**
(renamed, as it lost the *simple* property) has been extended with extra features, like an application launcher and 
ssh-launcher, and can act as a drop-in dmenu replacement, making it a very versatile tool.

**Rofi**, like dmenu, will provide the user with a textual list of options where one or more can be selected.
This can either be running an application, selecting a window, or options provided by an external script.

Its main features are:

 * Fully configurable keyboard navigation
 * Type to filter
    - Tokenized: type any word in any order to filter
    - Case insensitive (togglable)
    - Support for fuzzy-, regex-, and glob matching
 * UTF-8 enabled
    - UTF-8-aware string collating
    - International keyboard support (`e -> è)
 * RTL language support
 * Cairo drawing and Pango font rendering
 * Built-in modes:
    - Window switcher mode
        - EWMH compatible WM
    - Application launcher
    - Desktop file application launcher
    - SSH launcher mode
    - Combi mode, allowing several modes to be merged into one list
 * History-based ordering — last 25 choices are ordered on top based on use (optional)
 * Levenshtein distance ordering of matches (optional)
 * Drop-in dmenu replacement
    - Many added improvements
 * Easily extensible using scripts
 * Theming

**Rofi** has several built-in modes implementing common use cases and can be extended by scripts (either called from
**Rofi** or calling **Rofi**).

Below is a list of the different modes:

## Window Switcher

![Window List](https://davatorium.github.io/rofi/images/rofi/window-list.png)

The window switcher shows the following informations in columns (can be customized):

1. Desktop name
2. Window class
3. Window title

Window mode features:

 * Closing applications with `Shift-Delete`
 * Custom command with `Shift-Return`


## Application launcher

![run mode](https://davatorium.github.io/rofi/images/rofi/run-dialog.png)

The run mode allows users to quickly search for and launch a program.

Run mode features:

 * `Shift-Return` to run the selected program in a terminal
 * Favorites list, with frequently used programs sorted on top
 * Custom entries, like aliases, added by executing a command


## Desktop File Application launcher

The desktop run mode allows users to quickly search and launch an application from the *freedesktop.org* Desktop
Entries. These are used by most Desktop Environments to populate launchers and menus.
Drun mode features:

 * Favorites list, with frequently used programs sorted on top
 * Auto starting terminal applications in a terminal

## SSH launcher

![SSH Launcher](https://davatorium.github.io/rofi/images/rofi/ssh-dialog.png)

Quickly `ssh` into remote machines. Parses `~/.ssh/config` and `~/.ssh/known_hosts` to find hosts.

## Script mode

Loads external scripts to add modes to **Rofi**, for example a file-browser.

```
rofi  -show fb -modi fb:../Examples/rofi-file-browser.sh
```

## COMBI mode

Combine multiple modes in one view. This is especially useful when merging the window and run mode into one view.
Allowing to quickly switch to an application, either by switching to it when it is already running or starting it.

Example to combine Desktop run and the window switcher:

```
rofi -combi-modi window,drun -show combi -modi combi
```

## dmenu replacement

![DMENU replacement (running teiler)](https://davatorium.github.io/rofi/images/rofi/dmenu-replacement.png)

Drop in dmenu replacement. (Screenshot shows rofi used by
[teiler](https://github.com/carnager/teiler) ).

**Rofi** features several improvements over dmenu to improve usability. There is the option to add
an extra message bar (`-mesg`), pre-entering of text (`-filter`), or selecting entries based on a
pattern (`-select`). Also highlighting (`-u` and `-a`) options and modi to force user to select one
provided option (`-only-match`). In addition to this, rofi's dmenu mode can select multiple lines and
write them to stdout.

# Usage

If used with `-show [mode]`, rofi will immediately open in the specified [mode].

If used with `-dmenu`, rofi will use data from STDIN to let the user select an option.

For example, to show a run dialog:

  `rofi -show run`

To show a ssh dialog:

  `rofi -show ssh`

## dmenu

If rofi is passed the `-dmenu` option, or run as `dmenu` (ie, /usr/bin/dmenu is symlinked to /usr/bin/rofi),
it will use the data passed from STDIN.

```
~/scripts/my_script.sh | rofi -dmenu
echo -e "Option #1\nOption #2\nOption #3" | rofi -dmenu
```

In both cases, rofi will output the user's selection to STDOUT.

## Switching Between Modi

Type `Shift-/Left/Right` to switch between active modi.


## Key bindings

| Key                                  | Action                                                             |
|:-------------------------------------|:-------------------------------------------------------------------|
|`Ctrl-v, Insert`                      | Paste from clipboard |
|`Ctrl-Shift-v, Shift-Insert`          | Paste primary selection |
|`Ctrl-w`                              | Clear the line |
|`Ctrl-u`                              | Delete till the start of line |
|`Ctrl-a`                              | Move to beginning of line |
|`Ctrl-e`                              | Move to end of line |
|`Ctrl-f, Right`                       | Move forward one character |
|`Alt-f, Ctrl-Right`                   | Move forward one word |
|`Ctrl-b, Left`                        | Move back one character |
|`Alt-b, Ctrl-Left`                    | Move back one word |
|`Ctrl-d, Delete`                      | Delete character |
|`Ctrl-Alt-d`                          | Delete word |
|`Ctrl-h, Backspace, Shift-Backspace`  | Backspace (delete previous character) |
|`Ctrl-Alt-h`                          | Delete previous word |
|`Ctrl-j,Ctrl-m,Enter`                 | Accept entry |
|`Ctrl-n,Down`                         | Select next entry |
|`Ctrl-p,Up`                           | Select previous entry |
|`Page Up`                             | Go to the previous page |
|`Page Down`                           | Go to the next page |
|`Ctrl-Page Up`                        | Go to the previous column |
|`Ctrl-Page Down`                      | Go to the next column |
|`Ctrl-Enter`                          | Use entered text as a command (in `ssh/run modi`) |
|`Shift-Enter`                         | Launch the application in a terminal (in run mode) |
|`Shift-Enter`                         | Return the selected entry and move to the next item while keeping Rofi open. (in dmenu) |
|`Shift-Right`                         | Switch to the next modi. The list can be customized with the -modi option. |
|`Shift-Left`                          | Switch to the previous modi. The list can be customized with the -modi option. |
|`Ctrl-Tab`                            | Switch to the next modi. The list can be customized with the -modi option. |
|`Ctrl-Shift-Tab`                      | Switch to the previous modi. The list can be customized with the -modi option. |
|`Ctrl-space`                          | Set selected item as input text. |
|`Shift-Del`                           | Delete entry from history. |
|`grave`                               | Toggle case sensitivity. |
|`Alt-grave`                           | Toggle levenshtein sort. |
|`Alt-Shift-S`                         | Take a screenshot and store it in the Pictures directory. |

For the full list of key bindings, see: `rofi -show keys` or `rofi -help`.

# Configuration

There are currently three methods of setting configuration options:

 * Local configuration. Normally, depending on XDG, in `~/.config/rofi/config`. This uses the Xresources format.
 * Xresources: A method of storing key values in the Xserver. See
   [here](https://en.wikipedia.org/wiki/X_resources) for more information.
 * Command line options: Arguments are passed to **Rofi**.

A distribution can ship defaults in `/etc/rofi.conf`.

The Xresources options and the command line options are aliased. To define option X set:

    `rofi.X: value`

In the Xresources file. To set/override this from command line pass the same key
prefixed with '-':

    `rofi -X value`

To get a list of available options formatted as Xresources entries, run:

    `rofi -dump-Xresources`

or in a more readable format:

    `rofi -help`

The configuration system supports the following types:

 * String
 * Integer (signed and unsigned)
 * Char
 * Boolean

The Boolean option has a non-default command line syntax, to enable option X you do:

    `rofi -X`

to disable it:

    `rofi -no-X`

# Manpage

For more detailed information, please see the [manpage](doc/rofi.1.markdown), the [wiki](https://github.com/davatorium/rofi/wiki), or the [forum](https://reddit.com/r/qtools/).

# Installation

Please see the [installation guide](https://github.com/davatorium/rofi/blob/next/INSTALL.md) for instructions on how to
install **Rofi**.

# What is rofi not?

Rofi is not:

 * A preview application. In other words, it will not show a (small) preview of images, movies or other files.
 * A UI toolkit.
 * A library to be used in other applications.
 * An application that can support every possible use-case. It tries to be generic enough to be usable by everybody.
   Specific functionality can be added using scripts.
 * Just a dmenu replacement. The dmenu functionality is a nice 'extra' to **rofi**, not its main purpose.
