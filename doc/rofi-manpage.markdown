# ROFI 1 rofi

## NAME

rofi - A window switcher, run dialog and dmenu replacement

## SYNOPSIS

**rofi** [ -width *pct_scr* ] [ -lines *lines* ] [ -columns *columns* ] [ -font *xftfont* ] [ -fg *color* ]
[ -bg *color* ] [ -hlfg *color* ] [ -hlbg *color* ] [ -key *combo* ] [ -dkey *comdo* ] [ -rkey *comdo* ]
[ -terminal *terminal* ] [ -location *position* ] [ -hmode ] [ -fixed-num-lines ] [ -padding *padding* ]
[ -opacity *opacity%* ] [ -display *display* ] [ -bc *color* ] [ -bw *width* ] [ -dmenu [ -p *prompt* ] ]
[ -ssh-set-title *true|false* ] [ -now ] [ -rnow ] [ -snow ] [ -version ] [ -help] [ -dump-xresources ]
[ -disable-history ] [ -levenshtein-sort ] [ -show *mode* ] [ -switcher *mode1,mode2* ]

## DESCRIPTION

**rofi** is an X11 popup window switcher. A list is displayed center-screen showing open window titles, WM_CLASS, and desktop number.
The user may filter the list by typing, navigate with Up/Down or Tab keys, and select a window with Return (Enter). Escape cancels.

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

   Show run-dialog.


## OPTIONS

`-key`

  Change the key combination to display all windows (default: F12).

      rofi -key F12
      rofi -key control+shift+s
      rofi -key mod1+Tab


`-rkey`

  Change the key combination to display the run dialog (default: mod1-F2).


      rofi -rkey F11
      rofi -rkey control+shift+d
      rofi -rkey mod1+grave (grave=backtick)


`-skey`

  Change the key combination to display the ssh dialog (default: Alt-F3).


      rofi -skey F10
      rofi -skey control+shift+s
      rofi -skey mod1+grave (grave=backtick)


`-now`

  Run rofi in all-windows mode once then exit. Does not bind any keys.

`-rnow`

  Run rofi in run-dialog mode once then exit. Does not bind any keys.

`-snow`

  Run rofi in ssh mode once then exit. Does not bind any keys.

`-bg`

  Set the background text color (X11 named color or hex #rrggbb) for the menu (default: #222222).

      rofi -fg "#222222"


`-bc`

  Set the border color (X11 named color or hex #rrggbb) for the menu (default: #000000).

      rofi -bc black


`-bw`

  Set the border width in pixels (default: 1).

      rofi -bw 1


`-fg`

  Set the foreground text color (X11 named color or hex #rrggbb) for the menu (default: #cccccc).

      rofi -fg "#cccccc"

`-font`

  Xft font name for use by the menu (default: mono-14).


      rofi -font monospace-14:medium


`-hlbg`

  Set the background text color (X11 named color or hex #rrggbb) for the highlighted item in the
  menu (default: #005577).

      rofi -fg "#005577"


`-hlfg`

  Set the foreground text color (X11 named color or hex #rrggbb) for the highlighted item in the
  menu (default: #ffffff).

      rofi -fg "#ffffff"


`-opacity`

  Set the window opacity (0-100).

      rofi -opacity "75"

`-lines`

  Maximum number of lines the menu may show before scrolling (default: 25).

      rofi -lines 25

`-columns`

  The number of columns the menu may show before scrolling (default: 25).

      rofi -columns 2

`-width`

  Set the width of the menu as a percentage of the screen width (default: 60).

      rofi -width 60

`-terminal`

  Specify what terminal to start (default x-terminal-emulator)

      rofi -terminal xterm

`-location`

  Specify where the window should be located. The numbers map to the following location on the
  monitor:

      1 2 3
      8 0 4
      7 6 5

`-hmode`

  Switch to horizontal mode (ala dmenu). The number of elements is the number of `lines` times the
  number of `columns`.

`-fixed-num-lines`

  Keep a fixed number of visible lines (See the `-lines` option.)

`-padding`

  Define the inner margin of the window. Default is 5 pixels.

  To make rofi look like dmenu:

      rofi -hmode -padding 0

`-dmenu`

  Run rofi in dmenu mode. Allowing it to be used for user interaction in scripts.

  Pressing `shift-enter` will open the selected entries and move to the next entry.


`-dump-xresources`

  Dump the current active configuration in xresources format to the command-line.

`-ssh-set-title` *true|false*

  SSH dialogs tries to set 'ssh hostname' of the spawned terminal.
  Not all terminals support this.
  Default value is true.

`-disable-history`

  Disable history

`-levenshtein-sort`

  When searching sort the result based on levenshtein distance.

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


## Switch between modi

Type '?' *enter* to switch between window list, run and ssh mode. The list can be customized with
the `-switchers` argument.

## WEBSITE

**rofi** website can be found at [here](https://davedavenport.github.io/rofi/)

## Keybinding

* `Ctrl-v, Insert`: Paste clipboard
* `Ctrl-Shift-v, Shift-Insert`: Paste primary selection
* `Ctrl-u`: Clear the line
* `Ctrl-a`: Beginning of line
* `Ctrl-e`: End of line
* `Ctrl-f, Right`: Forward one character
* `Ctrl-b, Left`: Back one character
* `Ctrl-d, Delete`: Delete character
* `Ctrl-h, Backspace`: Backspace (delete previous character)
* `Ctrl-j,Ctrl-m,Enter`: Accept entry
* `Ctrl-n,Down`: Select next entry
* `Ctrl-p,Up`: Select previous entry
* `Page Up`: Go to the previous page
* `Page Down`: Go to the next page
* `Ctrl-Page Up`: Go to the previous column
* `Ctrl-Page Down`: Go to the next column

## AUTHOR

Qball Cow <qball@gmpclient.org>

Original code based on work by: Sean Pringle <sean.pringle@gmail.com>
