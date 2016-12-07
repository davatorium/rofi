[![Build Status](https://travis-ci.org/DaveDavenport/rofi.svg?branch=master)](https://travis-ci.org/DaveDavenport/rofi)
[![codecov.io](https://codecov.io/github/DaveDavenport/rofi/coverage.svg?branch=master)](https://codecov.io/github/DaveDavenport/rofi?branch=master)
[![Issues](https://img.shields.io/github/issues/DaveDavenport/rofi.svg)](https://github.com/DaveDavenport/rofi/issues)
[![Forks](https://img.shields.io/github/forks/DaveDavenport/rofi.svg)](https://github.com/DaveDavenport/rofi/network)
[![Stars](https://img.shields.io/github/stars/DaveDavenport/rofi.svg)](https://github.com/DaveDavenport/rofi/stargazers)
[![Downloads](https://img.shields.io/github/downloads/DaveDavenport/rofi/total.svg)](https://github.com/DaveDavenport/rofi/releases)
[![Coverity](https://scan.coverity.com/projects/3850/badge.svg)](https://scan.coverity.com/projects/davedavenport-rofi)
[![Forum](https://img.shields.io/badge/forum-online-green.svg)](https://forum.qtools.org)

# A window switcher, run dialog and dmenu replacement

**Rofi** started as clone of simpleswitcher, written by [Sean Pringle](http://github.com/seanpringle/simpleswitcher) a
popup window switcher roughly based on [superswitcher](http://code.google.com/p/superswitcher/).
Simpleswitcher laid the foundations and therefor Sean Pringle deserves most of the credit for this tool. **Rofi**,
renamed as it lost the *simple* property, has been extended with extra features, like a run-dialog, ssh-launcher and
can act as a drop-in dmenu replacement, making it a very versatile tool.

**Rofi**, like dmenu, will provide the user with a textual list of options where one or more can be selected.
This can either be, running an application, selecting a window or options provided by an external script.

It main features are:

* Fully configurable keyboard navigation.
* Type to filter
    - Tokenized: Type any word in any order to filter.
    - (toggable) Case insensitive.
    - Supports fuzzy, regex and glob matching.
* UTF-8 enabled.
    - UTF-8 aware string collating.
    - intl. keyboard support (`e -> è)
* RTL language support.
* Cairo drawing and Pango font rendering.
* Build in modes:
    - Window switcher mode.
        - EWMH compatible WM.
    - Run mode.
    - Desktop File Run mode.
    - SSH launcher mode.
    - Combi mode, allow several modes to be merged into one list.
* History based ordering last 25 choices are ordered on top based on use. (optional)
* Levenshtein distance ordering of matches. (optional)
* Drop in dmenu replacement.
    - With many added improvements.
* Can be easily extended using scripts.
* Themeing.

**Rofi** has several buildin modes implementing common use-cases and can be exteneded by scripts (either called from
**Rofi** or calling **Rofi**).

Below the different modes are listed:

## Window Switcher

![Window List](https://davedavenport.github.io/rofi/images/rofi/window-list.png)

The window switcher shows the following informations in columns (can be customized):

1. Desktop name
2. Window class.
3. Window title.

Window mode features:

  - Closing applications by hitting `Shift-Delete`.
  - Custom command by `Shift-Return`


## Run mode

![run mode](https://davedavenport.github.io/rofi/images/rofi/run-dialog.png)

The run mode allows users to quickly search and launch a program.

Run mode features:

  - Shift-Return to run the selected program in a terminal.
  - Favorites list, frequently used programs are sorted on top.
  - Execute command to add custom entries, like aliases.


## DRun mode

The desktop run mode allows users to quickly search and launch an application from the *freedesktop.org* Desktop
Entries. These are used by most common Desktop Environments to populate launchers and menus.
Drun mode features:

  - Favorites list, frequently used programs are sorted on top.
  - Auto starting terminal applications in a terminal.

## SSH launcher

![SSH Launcher](https://davedavenport.github.io/rofi/images/rofi/ssh-dialog.png)

Quickly ssh into remote machines

  - Parses ~/.ssh/config to find hosts.

## Script mode

Loads external scripts to add modes to **Rofi**, for example a file-browser.

```
rofi  -show fb -modi fb:../Examples/rofi-file-browser.sh
```

## COMBI mode

Combine multiple modes in one view. This is especially usefull when merging the window and run mode into one view.
Allowing to quickly switch to an application, either by switching to it when it is already running or starting it.

Example to combine Desktop run and the window switcher:

```
rofi -combi-modi window,drun -show combi -modi combi
```

## dmenu replacement

![DMENU replacement (running teiler)](https://davedavenport.github.io/rofi/images/rofi/dmenu-replacement.png)

Drop in dmenu replacement. (Screenshot shows rofi used by
[teiler](https://github.com/carnager/teiler) ).

**Rofi** features several improvements over dmenu to improve usability. There is the option to add
an extra message bar (`-mesg`), pre-entering of text (`-filter`) or selecting entries based on a
pattern (`-select`). Also highlighting (`-u` and `-a`) options and modi to force user to select one
provided option (`-only-match`). In addition to this rofi's dmenu mode can select multiple lines and
write them to stdout.

# Usage

If used with `-show [mode]`, rofi will immediately open in the specified [mode]

If used with `-dmenu`, rofi will use data from STDIN to let the user select an option.

For example to show a run dialog:

  `rofi -show run`

Show a ssh dialog:

  `rofi -show ssh`

## dmenu

If passed the `-dmenu` option, or ran as `dmenu` (ie, /usr/bin/dmenu is symlinked to /usr/bin/rofi),
rofi will use the data passed from STDIN.

```
~/scripts/my_script.sh | rofi -dmenu
echo -e "Option #1\nOption #2\nOption #3" | rofi -dmenu
```

In both cases, rofi will output the user's selection to STDOUT.

## Switching Between Modi

Type `Shift-/Left/Right` to switch between active modi.


## Keybindings

| Key                         | Action                                                             |
|:----------------------------|:-------------------------------------------------------------------|
|`Ctrl-v, Insert`             | Paste clipboard |
|`Ctrl-Shift-v, Shift-Insert` | Paste primary selection |
|`Ctrl-w`                     | Clear the line |
|`Ctrl-u`                     | Delete till the start of line |
|`Ctrl-a`                     | Move to beginning of line |
|`Ctrl-e`                     | Move to end of line |
|`Ctrl-f, Right`              | Move forward one character |
|`Alt-f`                      | Move forward one word |
|`Ctrl-b, Left`               | Move back one character |
|`Alt-b`                      | Move back one word |
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
|`Shift-Right`                | Switch to the next modi. The list can be customized with the -modi option. |
|`Shift-Left`                 | Switch to the previous modi. The list can be customized with the -modi option. |
|`Ctrl-Tab`                   | Switch to the next modi. The list can be customized with the -modi option. |
|`Ctrl-Shift-Tab`             | Switch to the previous modi. The list can be customized with the -modi option. |
|`Ctrl-space`                 | Set selected item as input text. |
|`Shift-Del`                  | Delete entry from history. |
|`grave`                      | Toggle case sensitivity. |
|`Alt-grave`                  | Toggle levenshtein  sort. |
|`Alt-Shift-S`                | Take a screenshot and store this in the Pictures directory. |

For the full list of keybindings see: `rofi -show keys` or `rofi -help`.

# Configuration

There are currently three methods of setting configuration options:

 * Local configuration. Normally, depending on XDG, in `~/.local/rofi/config`. This uses the Xresources format.
 * Xresources: A method of storing key values in the Xserver. See
   [here](https://en.wikipedia.org/wiki/X_resources) for more information.
 * Commandline options: Arguments passed to **Rofi**.

A distribution can ship defaults in `/etc/rofi.conf`.

The Xresources options and the commandline options are aliased. To define option X set:

    rofi.X: value

In the Xresources file. To set/override this from commandline pass the same key
prefixed with '-':

    rofi -X value

To get a list of available options, formatted as Xresources entries run:

    rofi -dump-Xresources

or in a more readable format

    rofi -help

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

Please see the [installation guide](https://davedavenport.github.io/rofi/p08-INSTALL.html) for instruction on how to
install **Rofi**.
