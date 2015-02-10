# ROFI 1 rofi

## NAME

rofi - A window switcher, run dialog and dmenu replacement

## SYNOPSIS

**rofi**
[ -width *pct_scr* ]
[ -lines *lines* ]
[ -columns *columns* ]
[ -font *pangofont* ]
[ -fg *color* ]
[ -bg *color* ]
[ -bgalt *color* ]
[ -hlfg *color* ]
[ -hlbg *color* ]
[ -key *combo* ]
[ -dkey *comdo* ]
[ -rkey *comdo* ]
[ -terminal *terminal* ]
[ -location *position* ]
[ -fixed-num-lines ]
[ -padding *padding* ]
[ -opacity *opacity%* ]
[ -display *display* ]
[ -bc *color* ]
[ -bw *width* ]
[ -dmenu [ -p *prompt* ] [ -sep *separator* ] [ -l *selected line* ] ]
[ -ssh-client *client* ]
[ -ssh-command *command* ]
[ -disable-history ]
[ -levenshtein-sort ]
[ -case-sensitive ]
[ -show *mode* ]
[ -switcher *mode1,mode2* ]
[ -eh *element height* ]
[ -lazy-filter-limit *limit* ]
[ -e *message*]
[ -pid *path* ]
[ -now ]
[ -rnow ]
[ -snow ]
[ -version ]
[ -help]
[ -dump-xresources ]

## DESCRIPTION

**rofi** is an X11 popup window switcher, run dialog, dmenu replacement and more. It focusses on
being fast to use and have minimal distraction. It supports keyboard and mouse navigation, type to
filter, tokenized search and more.

## License

MIT/X11

## USAGE

**rofi** can be used in two ways, single-shot; executes once and directly exits when done or as
daemon listening to specific key-combinations.

The default key combinations are:

* `F12`

   Show all windows on all desktops.

* `mod1-F2`

   Show run-dialog.

* `mod1-F3`

   Show ssh-dialog.


## OPTIONS


`-key`

  Change the key combination to display all windows.


      rofi -key F12
      rofi -key control+shift+s
      rofi -key mod1+Tab

  Default: *F12*


`-rkey`

  Change the key combination to display the run dialog.


      rofi -rkey F11
      rofi -rkey control+shift+d
      rofi -rkey mod1+grave (grave=backtick)

  Default: *Alt-F2*

`-skey`

  Change the key combination to display the ssh dialog.


      rofi -skey F10
      rofi -skey control+shift+s
      rofi -skey mod1+grave (grave=backtick)

  Default: *Alt-F3*

`-now`

  Run rofi in all-windows mode once then exit. Does not bind any keys.

`-rnow`

  Run rofi in run-dialog mode once then exit. Does not bind any keys.

`-snow`

  Run rofi in ssh mode once then exit. Does not bind any keys.

`-dmenu`

  Run rofi in dmenu mode. Allowing it to be used for user interaction in scripts.

  Pressing `shift-enter` will open the selected entries and move to the next entry.

`-show` *mode*

  Open rofi in a certain mode.

  For example to show the run-dialog:

        rofi -show run

  This function deprecates -rnow,-snow and -now

`-switchers` *mode1,mode1*

  Give a comma separated list of modes to enable, in what order.

  For example to only show the run and ssh dialog (in that order):

        rofi -switchers "run,ssh" -show run

  Custom modes can be added using the internal 'script' mode. Each mode has two parameters:

        <name>:<script>

  So to have a mode 'Workspaces' using the `i3_switch_workspace.sh` script type:

        rofi -switchers "window,run,ssh,Workspaces:i3_switch_workspaces.sh" -show Workspaces

`-case-sensitive`

  Start in case sensitive mode.


### Theming

`-bg`

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

  Set the foreground text color (X11 named color or hex #rrggbb) for the menu.

      rofi -fg "#cccccc"

  Default: *#222222*

`-hlbg`

  Set the background text color (X11 named color or hex #rrggbb) for the highlighted item in the
  menu.

      rofi -hlbg "#005577"

  Default: *#005577*

`-hlfg`

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

        rofi -rnow -sidebar-mode -lines 0

`-lazy-filter-limit` *limit*

   The number of entries required for Rofi to go into lazy filter mode.
   In lazy filter mode, it won't refilter the list on each keypress, but only after rofi been idle
   for 250ms. Experiments shows that the default (5000 lines) works well, set to 0 to always enable.

   Default: *5000*

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

### History and Sorting

`-disable-history`

  Disable history

`-levenshtein-sort`

  When searching sort the result based on levenshtein distance.

  Note that levenshtein sort is disabled in dmenu mode.

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

### Message dialog

`-e` *message*

    Popup a message dialog (used internally for showing errors) with *message*.
    Message can be multi-line.

### Other

'-pid' *path*

    Make **rofi** create a pid file and check this on startup. Avoiding multiple copies running
    simultaneous. This is useful when running rofi from a keybinding daemon. 

### Debug

`-dump-xresources`

  Dump the current active configuration in xresources format to the command-line.

## PATTERN

To launch commands (e.g. when using the ssh dialog) the user can enter the used commandline,
the following keys can be used that will be replaced at runtime:

* `{host}`: The host to connect to.
* `{terminal}`: The configured terminal (See -terminal-emulator)
* `{ssh-client}`: The configured ssh client (See -ssh-client)
* `{cmd}`: The command to execute.

## Dmenu replacemnt

If `argv[0]` (calling command) is dmenu, **rofi** will start in dmenu mode.
This way it can be used as a drop-in replacement for dmenu. just copy or symlink **rofi** to dmenu in `$PATH`.


    ln -s /usr/bin/dmenu /usr/bin/rofi

## Signals

`HUP`

    If in daemon mode, reload the configuration from Xresources. (commandline arguments still
override xresources).


## Colors

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

## Keybindings

Rofi supports the following keybindings:

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
* `Ctrl-Enter`: Use entered text as command (in ssh/run dialog)
* `Shift-Enter`: Launch the application in a terminal (in run dialog)
* `Shift-Enter`: Return the selected entry and move to the next item while keeping Rofi open. (in dmenu)
* `Shift-Right`: Switch to the next modi. The list can be customized with the `-switchers` argument.
* `Shift-Left`: Switch to the previous modi. The list can be customized with the `-switchers` argument.
* `Ctrl-space`: Set selected item as input text.
* `Shift-Del`: Delete entry from history.
* `grave`: Toggle case sensitivity.

## FAQ

`Text in window switcher is not nicely lined out`

    Try using a mono-space font.

`Rofi is completely black.`

    Check quotes used on the commandline: e.g. used “ instead of ".

## WEBSITE

**rofi** website can be found at [here](https://davedavenport.github.io/rofi/)

**rofi** bugtracker can be found [here](https://github.com/DaveDavenport/rofi/issues)

## AUTHOR

Qball Cow <qball@gmpclient.org>

Original code based on work by: Sean Pringle <sean.pringle@gmail.com>
