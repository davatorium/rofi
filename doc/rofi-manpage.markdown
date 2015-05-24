# ROFI 1 rofi

## NAME

**rofi** - A window switcher, run launcher, ssh dialog and dmenu replacement

## SYNOPSIS

**rofi**
[ -width *pct_scr* ]
[ -lines *lines* ]
[ -columns *columns* ]
[ -font *pangofont* ]
[ -fg *color* ]
[ -fg-urgent *color* ]
[ -fg-active *color* ]
[ -bg-urgent *color* ]
[ -bg-active *color* ]
[ -bg *color* ]
[ -bgalt *color* ]
[ -hlfg *color* ]
[ -hlbg *color* ]
[ -key-**mode** *combo* ]
[ -terminal *terminal* ]
[ -location *position* ]
[ -fixed-num-lines ]
[ -padding *padding* ]
[ -opacity *opacity%* ]
[ -display *display* ]
[ -bc *color* ]
[ -bw *width* ]
[ -dmenu [ -p *prompt* ] [ -sep *separator* ] [ -l *selected line* ] [ -mesg ] [ -filter ] [ -select ] ]
[ -ssh-client *client* ]
[ -ssh-command *command* ]
[ -disable-history ]
[ -levenshtein-sort ]
[ -case-sensitive ]
[ -show *mode* ]
[ -modi *mode1,mode2* ]
[ -eh *element height* ]
[ -lazy-filter-limit *limit* ]
[ -e *message*]
[ -a *row* ]
[ -u *row* ]
[ -pid *path* ]
[ -now ]
[ -rnow ]
[ -snow ]
[ -version ]
[ -help]
[ -dump-Xresources ]
[ -auto-select ]
[ -parse-hosts ]
[ -combi-modi *mode1,mode2* ]
[ -quiet ]

## DESCRIPTION

**rofi** is an X11 popup window switcher, run dialog, dmenu replacement and more. It focuses on
being fast to use and have minimal distraction. It supports keyboard and mouse navigation, type to
filter, tokenized search and more.


## USAGE

**rofi** can be used in three ways, single-shot; executes once and directly exits when done, as a
daemon listening to specific key-combination or emulating dmenu.

### Single-shot mode

To launch **rofi** directly in a certain mode, specifying `rofi -show <mode>`.
To show the run dialog:

```
    rofi -show run
```

### Daemon mode

To launch **rofi** in daemon mode don't specify a mode to show (`-show <mode>`), instead you can
bind keys to launch a certain mode. To have run mode open when pressing `F2` start **rofi** like:

```
    rofi -key-run F2
```
Keybindings can also be specified in the `Xresources` file.

### Emulating dmenu

**rofi** can emulate `dmenu` (a dynamic menu for X) when launched with the `-dmenu` flag.

The official website for `dmenu` can be found [here](http://tools.suckless.org/dmenu/).

## OPTIONS

There are currently three methods of setting configuration options:

 * Compile time: edit config.c. This method is strongly discouraged.
 * Xresources: A method of storing key values in the Xserver. See
   [here](https://en.wikipedia.org/wiki/X_resources) for more information.
 * Command-line options: Arguments passed to **rofi**.

The Xresources options and the command-line options are aliased. So to set option X you would set:

    rofi.X: value

In the Xresources file, and to (override) this via the command-line you would pass the same key
prefixed with a '-':

    rofi -X value

To get a list of available options, formatted as Xresources entries run:

    rofi -dump-Xresources

The configuration system supports the following types:

 * String
 * Integer (signed and unsigned)
 * Char
 * Boolean

The boolean option has a non-default command-line syntax, to enable option X  you do:

    -X

to disable it:

    -no-X

Below is a list of the most important options:

### General

`-key-{mode}` **KEY**

  Set the key combination to display a {mode} in daemon mode.


      rofi -key-run F12
      rofi -key-ssh control+shift+s
      rofi -key-window mod1+Tab

`-dmenu`

  Run **rofi** in dmenu mode. Allowing it to be used for user interaction in scripts.

  In `dmenu` mode, **rofi** will read input from STDIN, and will output to STDOUT by default.

  Example to let the user choose between three pre-defined options:

        echo -e "Option #1\nOption #2\nOption #3" | rofi -dmenu

  Or get the options from a script:

        ~/my_script.sh | rofi -dmenu

  Pressing `shift-enter` will open the selected entries and move to the next entry.

`-show` *mode*

  Open **rofi** in a certain mode.

  For example to show the run-dialog:

        rofi -show run

  This function deprecates -rnow,-snow and -now

`-switchers` *mode1,mode1*
`-modi` *mode1,mode1*

  Give a comma separated list of modes to enable, in what order.

  For example to only show the run and ssh launcher (in that order):

        rofi -modi "run,ssh" -show run

  Custom modes can be added using the internal 'script' mode. Each mode has two parameters:

        <name>:<script>

  So to have a mode 'Workspaces' using the `i3_switch_workspace.sh` script type:

        rofi -modi "window,run,ssh,Workspaces:i3_switch_workspaces.sh" -show Workspaces

`-case-sensitive`

  Start in case sensitive mode.

`-quiet`

  Do not print any message when starting in daemon mode.


### Theming

`-bg`

`-bg-active`

`-bg-urgent`

  Set the background text color (X11 named color or hex #rrggbb) for the menu.

      rofi -bg "#222222"

  Default: *#f2f1f0*

`-bgalt`

  Set the background text color  for alternating rows (X11 named color or hex #rrggbb) for the menu.

      rofi -bgalt "#222222"

  Default: *#f2f1f0*

`-bc`

  Set the border color (X11 named color or hex #rrggbb) for the menu.

      rofi -bc black

  Default: *black*

`-bw`

  Set the border width in pixels.

      rofi -bw 1

  Default: *1*

`-fg`

`-fg-urgent`

`-fg-active`

  Set the foreground text color (X11 named color or hex #rrggbb) for the menu.

      rofi -fg "#cccccc"

  Default: *#222222*


`-hlbg`

`-hlbg-active`

`-hlbg-urgent`

  Set the background text color (X11 named color or hex #rrggbb) for the highlighted item in the
  menu.

      rofi -hlbg "#005577"

  Default: *#005577*

`-hlfg`

`-hlfg-active`

`-hlfg-urgent`

  Set the foreground text color (X11 named color or hex #rrggbb) for the highlighted item in the
  menu.

      rofi -hlfg "#ffffff"

  Default: *#FFFFFF*

`-font`

  Pango font name for use by the menu.


      rofi -font monospace\ 14

  Default: *mono 12*

`-opacity`

  Set the window opacity (0-100).

      rofi -opacity "75"

  Default: *100*

`-eh` *element height*

  The height of a field in lines. e.g.

            echo -e "a\n3|b\n4|c\n5" | rofi -sep '|' -eh 2 -dmenu

  Default: *1*

The following options are further explained in the theming section:

`-color-enabled`

    Enable the exteneded coloring options.

`-color-window` *background* *border color*

    Set window background and border color.

`-color-normal` *background,foreground,background alt, highlight background, highlight foreground*
`-color-urgent` *background,foreground,background alt, highlight background, highlight foreground*
`-color-active` *background,foreground,background alt, highlight background, highlight foreground*

    Specify the colors used in a row per state (normal, active, urgent).

### Layout

`-lines`

  Maximum number of lines the menu may show before scrolling.

      rofi -lines 25

  Default: *15*

`-columns`

  The number of columns the menu may show before scrolling.

      rofi -columns 2

  Default: *1*

`-width` [value]

  Set the width of the menu as a percentage of the screen width.

      rofi -width 60

  If value is larger then 100, the size is set in pixels. e.g. to span a full hd monitor:

      rofi -width 1920

  If the value is negative, it tries to estimates a character width. To show 30 characters on a row:

      rofi -width -30

  Character width is a rough estimation, and might not be correct, but should work for most monospaced fonts.

  Default: *50*

`-location`

  Specify where the window should be located. The numbers map to the following location on the
  monitor:

      1 2 3
      8 0 4
      7 6 5

  Default: *0*

`-fixed-num-lines`

  Keep a fixed number of visible lines (See the `-lines` option.)

`-padding`

  Define the inner margin of the window.

  Default: *5*

`-sidebar-mode`

    Go into side-bar mode, it will show list of modi at the bottom.
    To show sidebar use:

        rofi -show run -sidebar-mode -lines 0

`-lazy-filter-limit` *limit*

   The number of entries required for **rofi** to go into lazy filter mode.
   In lazy filter mode, it won't re-filter the list on each keypress, but only after **rofi** been
   idle for 250ms. Experiments shows that the default (5000 lines) works well, set to 0 to always
   enable.

   Default: *5000*

`-auto-select`

    When one entry is left, automatically select this.

### PATTERN setting

`-terminal`

  Specify what terminal to start.

      rofi -terminal xterm

  Pattern: *{terminal}*
  Default: *x-terminal-emulator*

`-ssh-client` *client*

  Override the used ssh client.

  Pattern: *{ssh-client}*
  Default: *ssh*


### SSH settings

`-ssh-command` *cmd*

  Set the command to execute when starting a ssh session.
  The pattern *{host}* is replaced by the selected ssh entry.

  Default: *{terminal} -e {ssh-client} {host}*

`-parse-hosts`

    Parse the `/etc/hosts` files for entries.


### Run settings

`-run-command` *cmd*

  Set the command (*{cmd}*) to execute when running an application.
  See *PATTERN*.

  Default: *{cmd}*

`-run-shell-command` *cmd*

  Set the command to execute when running an application in a shell.
  See *PATTERN*.

  Default: *{terminal} -e {cmd}*

`-run-list-command` *cmd*

  If set, use an external tool to generate list of executable commands. Uses 'run-command'

  Default: *""*

### Combi settings

`-combi-modi` *mode1,mode2*

    The modi to combine in the combi mode.
    For syntax to see `-modi`.
    To get one merge view, of window,run,ssh:

            rofi -show combi -combi-mode "window,run,ssh"

### History and Sorting

`-disable-history`
`-no-disable-history` (re-enable history)

  Disable history

`-levenshtein-sort` to enable
`-no-levenshtein-sort` to disable

  When searching sort the result based on levenshtein distance.

### Dmenu specific

`-sep` *separator*

    Separator for dmenu. For example to show list a to e with '|' as separator:

            echo "a|b|c|d|e" | rofi -sep '|' -dmenu

`-p` *prompt*

    Specify the prompt to show in dmenu mode. E.g. select monkey a,b,c,d or e.

            echo "a|b|c|d|e" | rofi -sep '|' -dmenu -p "monkey:"

    Default: *dmenu*

`-l` *selected line*

    Select a certain line.

    Default: *0*

`-i`

    Number mode, return the index of the selected row. (starting at 0)

`-a` *X*

    Active row, mark row X as active. (starting at 0)
    You can specify single element: -a 3
    A range: -a 3-8
    or a set of rows: -a 0,2
    Or any combination: -a 0,2-3,9

`-u` *X*

    Urgent row, mark row X as urgent. (starting at 0)
    You can specify single element: -u 3
    A range: -u 3-8
    or a set of rows: -u 0,2
    Or any combination: -u 0,2-3,9

`-only-match`

    Only return a selected item, do not allow custom entry.
    This mode always returns an entry, or returns directly when no entries given.

`-format` *format*

    Allows the output of dmenu to be customized (N is total number of input entries):

     * 's' selected string.
     * 'i' index (0 - (N-1)).
     * 'd' index (1 - N).
     * 'q' quote string.
     * 'f' filter string (user input).
     * 'F' quoted filter string (user input).

    Default: 's'

`-filter` *filter*

    Preset user filter to *filter* in the entry box and pre-filter the list.

`-select` *string*

    Select first line that matches the given string

`-mesg` *string*

    Add a message line beneath filter. Supports pango markdown.

### Message dialog

`-e` *message*

    Popup a message dialog (used internally for showing errors) with *message*.
    Message can be multi-line.

### Other

'-pid' *path*

    Make **rofi** create a pid file and check this on startup. Avoiding multiple copies running
    simultaneous. This is useful when running **rofi** from a keybinding daemon.

### Debug

`-dump-Xresources`

  Dump the current active configuration in Xresources format to the command-line.

## PATTERN

To launch commands (e.g. when using the ssh launcher) the user can enter the used command-line,
the following keys can be used that will be replaced at runtime:

* `{host}`: The host to connect to.
* `{terminal}`: The configured terminal (See -terminal-emulator)
* `{ssh-client}`: The configured ssh client (See -ssh-client)
* `{cmd}`: The command to execute.

## DMENU REPLACEMENT

If `argv[0]` (calling command) is dmenu, **rofi** will start in dmenu mode.
This way it can be used as a drop-in replacement for dmenu. just copy or symlink **rofi** to dmenu in `$PATH`.


    ln -s /usr/bin/dmenu /usr/bin/rofi

## SIGNALS

`HUP`

    If in daemon mode, reload the configuration from Xresources. (commandline arguments still
override Xresources).

## THEMING

With **rofi** 0.15.4 we have a new way of specifying colors, the old settings still apply (for now).
To enable the new setup, set `rofi.color-enabled` to true. The new setup allows you to specify
colors per state, similar to **i3**
Currently 3 states exists:

* **normal** Normal row.
* **urgent** Highlighted row (urgent)
* **active** Highlighted row (active)

For each state the following 5 colors must be set:

* **bg** Background color row
* **fg** Text color
* **bgalt** Background color alternating row
* **hlfg** Foreground color selected row
* **hlbg** Background color selected row

The window background and border color should be specified separate. The key `color-window` contains
a pair `background,border`.
An example for `Xresources` file:

```
! State:           'bg',     'fg',     'bgalt',  'hlbg',   'hlfg'
rofi.color-normal: #fdf6e3,  #002b36,  #eee8d5,  #586e75,  #eee8d5
rofi.color-urgent: #fdf6e3,  #dc322f,  #eee8d5,  #dc322f,  #fdf6e3
rofi.color-active: #fdf6e3,  #268bd2,  #eee8d5,  #268bd2,  #fdf6e3

!                  'background', 'border'
rofi.color-window: #fdf6e3,      #002b36
```

Same settings can also be specified on command-line:

```
rofi -color-normal "#fdf6e3,#002b36,#eee8d5,#586e75,#eee8d5"
```

## COLORS

Rofi has an experimental mode for a 'nicer' transparency. The idea is that you can make the
background of the window transparent but the text not. This way, in contrast to the `-opacity`
option, the text is still fully visible and readable.
To use this there are 2 requirements: 1. Your Xserver supports TrueColor, 2. You are running a
composite manager. If this is satisfied you can use the following format for colors:

   argb:FF444444

The first two fields specify the alpha level. This determines how much the background shines through
the color (00 everything, FF nothing). E.g. 'argb:00FF0000' gives you a bright red color with the
background shining through. If you want a dark greenish transparent color use: 'argb:dd2c3311'. This
can be done for any color; it is therefore possible to have solid borders,  the selected row solid,
and the others slightly transparent.

## KEYBINDINGS

**rofi** has the following key-bindings:

* `Ctrl-v, Insert`: Paste clipboard
* `Ctrl-Shift-v, Shift-Insert`: Paste primary selection
* `Ctrl-u`: Clear the line
* `Ctrl-a`: Beginning of line
* `Ctrl-e`: End of line
* `Ctrl-f, Right`: Forward one character
* `Alt-f`: Forward one word
* `Ctrl-b, Left`: Back one character
* `Alt-b`: Back one word
* `Ctrl-d, Delete`: Delete character
* `Ctrl-Alt-d': Delete word
* `Ctrl-h, Backspace`: Backspace (delete previous character)
* `Ctrl-Alt-h`: Delete previous word
* `Ctrl-j,Ctrl-m,Enter`: Accept entry
* `Ctrl-n,Down`: Select next entry
* `Ctrl-p,Up`: Select previous entry
* `Page Up`: Go to the previous page
* `Page Down`: Go to the next page
* `Ctrl-Page Up`: Go to the previous column
* `Ctrl-Page Down`: Go to the next column
* `Ctrl-Enter`: Use entered text as command (in ssh/run modi)
* `Shift-Enter`: Launch the application in a terminal (in run mode)
* `Shift-Enter`: Return the selected entry and move to the next item while keeping Rofi open. (in dmenu)
* `Shift-Right`: Switch to the next modi. The list can be customized with the `-switchers` argument.
* `Shift-Left`: Switch to the previous modi. The list can be customized with the `-switchers` argument.
* `Ctrl-Tab`: Switch to the next modi. The list can be customized with the `-switchers` argument.
* `Ctrl-Shift-Tab`: Switch to the previous modi. The list can be customized with the `-switchers` argument.
* `Ctrl-space`: Set selected item as input text.
* `Shift-Del`: Delete entry from history.
* `Ctrl-grave`: Toggle case sensitivity.

To get a full list of keybindings, see `rofi -dump-xresources | grep kb-`.
Keybindings can be modified using the configuration systems.

## FAQ

`Text in window switcher is not nicely lined out`

    Try using a mono-space font.

`**rofi** is completely black.`

    Check quotes used on the commandline: e.g. used “ instead of ".

## LICENSE

    MIT/X11

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

## WEBSITE

**rofi** website can be found at [here](https://davedavenport.github.io/rofi/)

**rofi** bugtracker can be found [here](https://github.com/DaveDavenport/rofi/issues)

## AUTHOR

Qball Cow <qball@gmpclient.org>

Rasmus Steinke

Original code based on work by: Sean Pringle <sean.pringle@gmail.com>

For a full list of authors, check the AUTHORS file.

