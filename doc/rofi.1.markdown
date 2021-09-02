# ROFI 1 rofi

## NAME

**rofi** - A window switcher, application launcher, ssh dialog, dmenu replacement and more

## SYNOPSIS

**rofi** [ -show *mode* ]|[ -dmenu ]|[ -e *msg* ] [ CONFIGURATION ]


## DESCRIPTION

**rofi** is an X11 pop-up window switcher, run dialog, dmenu replacement, and more. It focuses on
being fast to use and have minimal distraction. It supports keyboard and mouse navigation, type to
filter, tokenized search and more.


## USAGE

**rofi**'s main functionality is to assist in your workflow, allowing you to quickly switch
between windows, start applications or log into a remote machine via `ssh`.
There are different *modi* for different types of actions.

**rofi** can also function as (drop-in) replacement for **dmenu(1)**.

### Running rofi

To launch **rofi** directly in a certain mode, specify a mode with `rofi -show <mode>`.
To show the `run` dialog:

    rofi -show run

### Emulating dmenu

**rofi** can emulate **dmenu(1)** (a dynamic menu for X11) when launched with the `-dmenu` flag.

The website for `dmenu` can be found [here](http://tools.suckless.org/dmenu/).

**rofi** does not aim to be 100% compatible with `dmenu`. There are simply too many flavors of `dmenu`.
The idea is that the basic usage command-line flags are obeyed, theme-related flags are not.
Besides, **rofi** offers some extended features (like multi-select, highlighting, message bar, extra key bindings).

### Display Error message

**rofi** error dialog can also be called from the command line.

    rofi -e "my message"

Markup support can be enabled, see CONFIGURATION options.

## CONFIGURATION

There are currently three methods of setting configuration options (evaluated in order below):

 * System configuration file  (for example `/etc/rofi.rasi`).
   It first checks `XDG_CONFIG_DIRS`, and then `SYSCONFDIR` (that is passed at compile time).
   It loads the first config file it finds, it does not merge multiple system configuration files.
 * Rasi theme file: The new *theme* format can be used to set configuration values.
 * Command-line options: Arguments passed to **rofi**.

To get a template config file, run: `rofi -dump-config > config.rasi`

This will contain (commented) all current configuration options, modified options are uncommented.

The configuration system supports the following types:

 * string
 * integer (signed and unsigned)
 * char
 * boolean
 * lists

For the syntax of these options, see the **rofi-theme(5)** manpage.

For use on the command line, Boolean options have a non-default command-line
syntax. Example to enable option X:

    -X

To disable option X:

    -no-X

Below is a list of the most important options:

### General

`-help`

The help option shows the full list of command-line options and the current set values.
These include dynamic (run-time generated) options.

`-version`

Show the **rofi** version and exit.

`-dump-config`

Dump the current active configuration, in rasi format, to stdout and exit.
Information about the rasi format can be found in the **rofi-theme(5)** manpage.

`-dump-theme`

Dump the current active theme, in rasi format, to stdout and exit.

`-rasi-validate` *filename*

Try to parse the file and return 0 when successful, non-zero when failed.

`-threads` *num*

Specify the number of threads **rofi** should use:

  * 0: Autodetect the number of supported hardware threads.
  * 1: Disable threading
  * 2..n: Specify the maximum number of threads to use in the thread pool.

    Default:  Autodetect

`-display` *display*

The X server to contact. Default is `$DISPLAY`.

`-dmenu`

Run **rofi** in dmenu mode. This allows for interactive scripts.
In `dmenu` mode, **rofi** reads from STDIN, and output to STDOUT.
A simple example, displaying three pre-defined options:

    echo -e "Option #1\nOption #2\nOption #3" | rofi -dmenu

Or get the options from a script:

    ~/my_script.sh | rofi -dmenu

`-show` *mode*

Open **rofi** in a certain mode. Available modes are `window`, `run`, `drun`, `ssh`, `combi`.
The special argument `keys` can be used to open a searchable list of supported key bindings
(see *KEY BINDINGS*)

To show the run-dialog:

    rofi -show run

If `-show` is the last option passed to rofi, the first enabled modi is shown.

`-modi` *mode1,mode2*

Specify an ordered, comma-separated list of modes to enable.
Enabled modes can be changed at runtime. Default key is `Ctrl+Tab`.
If no modes are specified, all configured modes will be enabled.
To only show the `run` and `ssh` launcher:

    rofi -modi "run,ssh" -show run

Custom modes can be added using the internal `script` mode. Each such mode has two parameters:

    <name>:<script>

Example: Have a mode called 'Workspaces' using the `i3_switch_workspaces.sh` script:

    rofi -modi "window,run,ssh,Workspaces:i3_switch_workspaces.sh" -show Workspaces

Notes: The i3 window manager dislikes commas in the command when specifying an exec command.
For that case, `#` can be used as a separator.

**TIP**: The name is allowed to contain spaces:

    rofi -modi "My File Browser:fb.sh" -show "My File Browser"

`-case-sensitive`

Start in case-sensitive mode.
This option can be changed at run-time using the `-kb-toggle-case-sensitivity` key binding.

`-cycle`

Cycle through the result list. Default is 'true'.

`-filter` *filter*

Filter the list by setting text in input bar to *filter*

`-config` *filename*

Load an alternative configuration file.

`-cache-dir` *filename*

Directory that is used to place temporary files, like history.

`-scroll-method` *method*

Select the scrolling method. 0: Per page, 1: continuous.

`-normalize-match`

Normalize the string before matching, so `o` will match `ö`, and `é` matches `e`.  
This is not a perfect implementation, but works. For now, it disables highlighting of the matched part.

`-no-lazy-grab`

Disables lazy grab, this forces the keyboard being grabbed before gui is shown.

`-no-plugins`

Disable plugin loading.

`-plugin-path` *directory*

Specify the directory where **rofi** should look for plugins.

`-show-icons`

Show application icons in `drun` and `window` modes.

`-icon-theme`

Specify icon theme to be used.
If not specified default theme from DE is used, *Adwaita* and *gnome* themes act as
fallback themes.

`-fallback-application-icon`

Specify an icon to be used when the application icon in run/drun are not yet loaded or is not available.

`-markup`

Use Pango markup to format output wherever possible.

`-normal-window`

Make **rofi** react like a normal application window. Useful for scripts like Clerk that are basically an application.

`-[no-]steal-focus`

Make rofi steal focus on launch and restore close to window that held it when launched.

### Matching

`-matching` *method*

Specify the matching algorithm used.
Currently, the following methods are supported:

* **normal**: match the int string
* **regex**: match a regex input
* **glob**: match a glob pattern
* **fuzzy**: do a fuzzy match
* **prefix**: match prefix

   Default: *normal*

Note: glob matching might be slow for larger lists

`-tokenize`

Tokenize the input.

`-drun-categories` *category1*,*category2*

Only show desktop files that are present in the listed categories.

`-drun-match-fields` *field1*,*field2*,...

When using `drun`, match only with the specified Desktop entry fields.
The different fields are:

* **name**: the application's name
* **generic**: the application's generic name
* **exec**: the application's  executable
* **categories**: the application's categories
* **comment**: the application comment
* **all**: all the above

    Default: *name,generic,exec,categories,keywords*

`-drun-display-format`

The format string for the `drun` dialog:

* **name**: the application's name
* **generic**: the application's generic name
* **exec**: the application's  executable
* **categories**: the application's categories
* **comment**: the application comment

Pango markup can be used to formatting the output.

    Default: {name} [<span weight='light' size='small'><i>({generic})</i></span>]

Note: Only fields enabled in `-drun-match-fields` can be used in the format string.

`-[no-]drun-show-actions`

Show actions present in the Desktop files.

    Default: false

`-window-match-fields` *field1*,*field2*,...

When using window mode, match only with the specified fields.
The different fields are:

* **title**: window's title
* **class**: window's class
* **role**: window's role
* **name**: window's name
* **desktop**: window's current desktop
* **all**: all the above

    Default: *all*

`-matching-negate-char` *char*

Set the character used to negate the query (i.e. if it does **not** match the next keyword).
Set to '\x0' to disable.

    Default: '-'


### Layout and Theming

**IMPORTANT:**
  In newer **rofi** releases, all the theming options have been moved into the new theme format. They are no longer normal
  **rofi** options that can be passed directly on the command line (there are too many).
  Small snippets can be passed on the command line: `rofi -theme-str 'window {width: 50%;}'` to override a single
  setting. They are merged into the current theme.
  They can also be appended at the end of the **rofi** config file to override parts of the theme.

Most of the following options are **deprecated** and should not be used. Please use the new theme format to customize
**rofi**. More information about the new format can be found in the **rofi-theme(5)** manpage.

`-location`

Specify where the window should be located. The numbers map to the following locations on screen:

      1 2 3
      8 0 4
      7 6 5

Default: *0*

`-fixed-num-lines`

Keep a fixed number of visible lines. 

`-sidebar-mode`

Open in sidebar-mode. In this mode, a list of all enabled modes is shown at the bottom.
(See `-modi` option)
To show sidebar, use:

    rofi -show run -sidebar-mode 

`-hover-select`

Automatically select the entry the mouse is hovering over. This option is best combined with custom mouse bindings.
To utilize hover-select and accept an entry in a single click, use:

    rofi -show run -hover-select -me-select-entry '' -me-accept-entry MousePrimary

`-eh` *number*

Set row height (in chars)
Default: *1*

`-auto-select`

When one entry is left, automatically select it.

`-m` *num*

`-m` *name*

`-monitor` *num*

`-monitor` *name*

Select monitor to display **rofi** on.
It accepts as input: *primary* (if primary output is set), the *xrandr* output name, or integer number (in order of
detection). Negative numbers are handled differently:

 *  **-1**: the currently focused monitor.
 *  **-2**: the currently focused window (that is, **rofi** will be displayed on top of the focused window).
 *  **-3**: Position of mouse (overrides the location setting to get normal context menu
    behavior.)
 *  **-4**: the monitor with the focused window.
 *  **-5**: the monitor that shows the mouse pointer.

    Default: *-5*

See `rofi -h` output for the detected monitors, their position, and size.


`-theme` *filename*

Path to the new theme file format. This overrides the old theme settings.

`-theme-str` *string*

Allow theme parts to be specified on the command line as an override.

For example:

    rofi -theme-str '#window { fullscreen: true; }'

This option can be specified multiple times.
This is now the method to tweak the theme via the command line.

`-dpi`  *number*

Override the default DPI setting.

* If set to `0`, it tries to auto-detect based on X11 screen size (similar to i3 and GTK).
* If set to `1`, it tries to auto-detect based on the size of the monitor that **rofi** is displayed on (similar to latest Qt 5).

`-selected-row` *selected row*

Select a certain row.

Default: *0*

### PATTERN setting

`-terminal`

Specify which terminal to start.

    rofi -terminal xterm

Pattern: *{terminal}*

Default: *x-terminal-emulator*

`-ssh-client` *client*

Override the used `ssh` client.

Pattern: *{ssh-client}*

Default: *ssh*

### SSH settings

`-ssh-command` *cmd*

Set the command to execute when starting an ssh session.
The pattern *{host}* is replaced by the selected ssh entry.

Pattern: *{ssh-client}*

Default: *{terminal} -e {ssh-client} {host}*

`-parse-hosts`

Parse the `/etc/hosts` file for entries.

Default: *disabled*

`-parse-known-hosts`
`-no-parse-known-hosts`

Parse the `~/.ssh/known_hosts` file for entries.

Default: *enabled*

### Run settings

`-run-command` *cmd*

Set command (*{cmd}*) to execute when running an application.
See *PATTERN*.

Default: *{cmd}*

`-run-shell-command` *cmd*

Set command to execute when running an application in a shell.
See *PATTERN*.

Default: *{terminal} -e {cmd}*

`-run-list-command` *cmd*

If set, use an external tool to generate a list of executable commands. Uses `run-command`.

Default: *{cmd}*

### Window switcher settings

`-window-format` *format*

Format what is being displayed for windows.

*format*: {field[:len]}

*field*:

 * **w**: desktop name
 * **t**: title of window
 * **n**: name
 * **r**: role
 * **c**: class

*len*: maximum field length (0 for auto-size). If length and window *width* are negative, field length is *width - len*.  
If length is positive, the entry will be truncated or padded to fill that length.


default: {w}  {c}   {t}

`-window-command` *cmd*

Set command to execute on selected window for an alt action (`-kb-accept-alt`).
See *PATTERN*.

Default: *"wmctrl -i -R {window}"*


`-window-thumbnail`

Show window thumbnail (if available) as icon in the window switcher.


You can stop rofi from exiting when closing a window (allowing multiple to be closed in a row).

```css
configuration {
  window {
      close-on-delete: false;
  }
}
```

### Combi settings

`-combi-modi` *mode1*,*mode2*

The modi to combine in combi mode.
For syntax to `-combi-modi`, see `-modi`.
To get one merge view, of `window`,`run`, and `ssh`:

    rofi -show combi -combi-modi "window,run,ssh" -modi combi

**NOTE**: The i3 window manager dislikes commas in the command when specifying an exec command.
For that case, `#` can be used as a separator.

### History and Sorting

`-disable-history`
`-no-disable-history` (re-enable history)

Disable history

`-sort` to enable
`-no-sort` to disable

Enable, disable sorting.
This setting can be changed at runtime (see `-kb-toggle-sort`).

`-sorting-method` 'method' to specify the sorting method.

There are 2 sorting methods:

 * levenshtein (Default)
 * fzf sorting.

`-max-history-size` *number*

Maximum number of entries to store in history. Defaults to 25. (WARNING: can cause slowdowns when set too high)

### Dmenu specific

`-sep` *separator*

Separator for `dmenu`. Example: To show a list of 'a' to 'e' with '|' as a separator:

    echo "a|b|c|d|e" | rofi -sep '|' -dmenu

`-p` *prompt*

Specify the prompt to show in `dmenu` mode. For example, select 'monkey', a,b,c,d, or e.

    echo "a|b|c|d|e" | rofi -sep '|' -dmenu -p "monkey"

Default: *dmenu*

`-l` *number of lines to show*

Maximum number of lines the menu may show before scrolling.

    rofi -dmenu -l 25

Default: *15*

`-i`

Makes `dmenu` searches case-insensitive

`-a` *X*

Active row, mark *X* as active. Where *X* is a comma-separated list of python(1)-style indices and ranges, e.g.  indices start at 0, -1 refers to the last row with -2 preceding it, ranges are left-open and right-close, and so on. You can specify:

  * A single row: '5'
  * A range of (last 3) rows: '-3:'
  * 4 rows starting from row 7: '7:11' (or in legacy notation: '7-10')
  * A set of rows: '2,0,-9'
  * Or any combination: '5,-3:,7:11,2,0,-9'

`-u` *X*

Urgent row, mark *X* as urgent. See `-a` option for details.

`-only-match`

Only return a selected item, do not allow custom entry.
This mode always returns an entry. It will not return if no matching entry is
selected.

`-no-custom`

Only return a selected item, do not allow custom entry.
This mode returns directly when no entries given.

`-format` *format*

Allows the output of dmenu to be customized (N is the total number of input entries):

  * 's' selected string
  * 'i' index (0 - (N-1))
  * 'd' index (1 - N)
  * 'q' quote string
  * 'p' Selected string stripped from Pango markup (Needs to be a valid string)
  * 'f' filter string (user input)
  * 'F' quoted filter string (user input)

Default: 's'

`-select` *string*

Select first line that matches the given string

`-mesg` *string*

Add a message line below the filter entry box. Supports Pango markup.
For more information on supported markup, see [here](https://developer.gnome.org/pygtk/stable/pango-markup-language.html)

`-dump`

Dump the filtered list to stdout and quit.
This can be used to get the list as **rofi** would filter it.
Use together with `-filter` command.

`-input` *file*

Reads from *file* instead of stdin.

`-password`

Hide the input text. This should not be considered secure!

`-markup-rows`

Tell **rofi** that DMenu input is Pango markup encoded, and should be rendered.
See [here](https://developer.gnome.org/pygtk/stable/pango-markup-language.html) for details about Pango markup.


`-multi-select`

Allow multiple lines to be selected. Adds a small selection indicator to the left of each entry.

`-sync`

Force **rofi** mode to first read all data from stdin before showing the selection window. This is original dmenu behavior.

Note: the default asynchronous mode will also be automatically disabled if used with conflicting options,
such as `-dump`, `-only-match` or `-auto-select`.

`-async-pre-read` *number*

Reads the first *number* entries blocking, then switches to async mode.
This makes it feel more 'snappy'.

*default*: 25

`-window-title` *title*

Set name used for the window title. Will be shown as Rofi - *title*

`-w` *windowid*

Position **rofi** over the window with the given X11 window ID.

`-keep-right`

Set ellipsize mode to start. So, the end of the string is visible.


### Message dialog

`-e` *message*

Pops up a message dialog (used internally for showing errors) with *message*.
Message can be multi-line.

### File browser settings

File browser behavior can be controlled via the following options:

```css
configuration {
   filebrowser {
      /** Directory the file browser starts in. */
      directory: "/some/directory";
      /**
        * Sorting method. Can be set to:
        *   - "name"
        *   - "mtime" (modification time)
        *   - "atime" (access time)
        *   - "ctime" (change time)
        */
      sorting-method: "name";
      /** Group directories before files. */
      directories-first: true;
   }
}
```

### Other

`-drun-use-desktop-cache`

Build and use a cache with the content of desktop files. Usable for systems with slow hard drives.

`-drun-reload-desktop-cache`

If `drun-use-desktop-cache` is enabled, rebuild a cache with the content of desktop files.

`-drun-url-launcher` *command*

Command to open a Desktop Entry that is a Link.

`-pid` *path*

Make **rofi** create a pid file and check this on startup. The pid file prevents multiple **rofi** instances from running simultaneously. This is useful when running **rofi** from a key-binding daemon.

`-display-{mode}` *string*

Set the name to use for mode. This is used as prompt and in combi-browser.

It is now preferred to use the configuration file:

```css
configuration {
  {mode} {
    display-name: *string*;
  }
}
```


`-click-to-exit`
`-no-click-to-exit`

Click the mouse outside the **rofi** window to exit.

Default: *enabled*

## PATTERN

To launch commands (for example, when using the ssh launcher), the user can enter the used command-line. The following keys can be used that will be replaced at runtime:

  * `{host}`: the host to connect to
  * `{terminal}`: the configured terminal (see -terminal-emulator)
  * `{ssh-client}`: the configured ssh client (see -ssh-client)
  * `{cmd}`: the command to execute
  * `{window}`: the window ID of the selected window (in `window-command`)

## DMENU REPLACEMENT

If `argv[0]` (calling command) is dmenu, **rofi** will start in dmenu mode.
This way, it can be used as a drop-in replacement for dmenu. Just copy or symlink **rofi** to dmenu in `$PATH`.

    ln -s /usr/bin/rofi /usr/bin/dmenu

## THEMING

Please see **rofi-theme(5)** manpage for more information on theming.

## KEY BINDINGS

**rofi** has the following key bindings:

  * `Control-v, Insert`: Paste from clipboard
  * `Control-Shift-v, Shift-Insert`: Paste primary selection
  * `Control-u`: Clear the line
  * `Control-a`: Beginning of line
  * `Control-e`: End of line
  * `Control-f, Right`: Forward one character
  * `Alt-f, Control-Right`: Forward one word
  * `Control-b, Left`: Back one character
  * `Alt-b, Control-Left`: Back one word
  * `Control-d, Delete`: Delete character
  * `Control-Alt-d`: Delete word
  * `Control-h, Backspace, Shift-Backspace`: Backspace (delete previous character)
  * `Control-Alt-h`: Delete previous word
  * `Control-j,Control-m,Enter`: Accept entry
  * `Control-n,Down`: Select next entry
  * `Control-p,Up`: Select previous entry
  * `Page Up`: Go to previous page
  * `Page Down`: Go to next page
  * `Control-Page Up`: Go to previous column
  * `Control-Page Down`: Go to next column
  * `Control-Enter`: Use entered text as a command (in `ssh/run modi`)
  * `Shift-Enter`: Launch the application in a terminal (in run mode)
  * `Control-Shift-Enter`: As Control-Enter and run the command in terminal (in run mode)
  * `Shift-Enter`: Return the selected entry and move to the next item while keeping **rofi** open. (in dmenu)
  * `Shift-Right`: Switch to the next mode. The list can be customized with the `-modi` argument.
  * `Shift-Left`: Switch to the previous mode. The list can be customized with the `-modi` argument.
  * `Control-Tab`: Switch to the next mode. The list can be customized with the `-modi` argument.
  * `Control-Shift-Tab`: Switch to the previous mode. The list can be customized with the `-modi` argument.
  * `Control-space`: Set selected item as input text.
  * `Shift-Del`: Delete entry from history.
  * `grave`: Toggle case sensitivity.
  * `Alt-grave`: Toggle sorting.
  * `Alt-Shift-S`: Take a screenshot and store it in the Pictures directory.
  * `Control-l`: File complete for run dialog.

This list might not be complete, to get a full list of all key bindings
supported in your rofi, see `rofi -h`. The options starting with `-kb` are keybindings.

Key bindings can be modified using the configuration systems. Multiple keys can be bound
to one action by comma separating them. For example `-kb-primary-paste "Conctrol+v,Insert"`

To get a searchable list of key bindings, run `rofi -show keys`.

A key binding starting with `!` will act when all keys have been released.

You can bind certain events to key-actions:

### Timeout

You can configure an action to be taken when rofi has not been interacted
with for a certain amount of seconds. You can specify a keybinding to trigger
after X seconds.

```css
configuration {
  timeout {
      delay:  15;
      action: "kb-cancel";
  }
}
```

### Input change

When the input of the textbox changes:

```css
configuration {
  inputchange {
      action: "kb-row-first";
  }
}
```


## Available Modi

### window

Show a list of all the windows and allow switching between them.
Pressing the `delete-entry` binding (`shift-delete`) will close the window.
Pressing the `accept-custom` binding (`control-enter` or `shift-enter`) will run a command on the window.
(See option `window-command` );

If there is no match, it will try to launch the input.

### windowcd

Shows a list of the windows on the current desktop and allows switching between them.
Pressing the `delete-entry` binding (`shift-delete`) will kill the window.
Pressing the `accept-custom` binding (`control-enter` or `shift-enter`) will run a command on the window.
(See option `window-command` );

If there is no match, it will try to launch the input.

### run

Shows a list of executables in `$PATH` and can launch them (optional in a terminal).
Pressing the `delete-entry` binding (`shift-delete`) will remove this entry from the run history.
Pressing the `accept-custom` binding (`control-enter`) will run the command as entered in the entry box.
Pressing the `accept-alt` binding (`shift-enter`) will run the command in a terminal.

When pressing the `mode-complete` binding (`Control-l`), you can use the File Browser mode to launch the application
with a file as the first argument.

### drun

Same as the **run** launches, but the list is created from the installed desktop files. It automatically launches them
in a terminal if specified in the Desktop File.
Pressing the `delete-entry` binding (`shift-delete`) will remove this entry from the run history.
Pressing the `accept-custom` binding (`control-enter`) will run the command as entered in the entry box.
Pressing the `accept-alt` binding (`shift-enter`) will run the command in a terminal.

When pressing the `mode-complete` binding (`Control-l`), you can use the File Browser mode to launch the application
passing a file as argument if specified in the desktop file.


The DRUN mode tries to follow the [XDG Desktop Entry
Specification](https://freedesktop.org/wiki/Specifications/desktop-entry-spec/) and should be compatible with
applications using this standard.  Some applications create invalid desktop files, **rofi** will discard these entries.
See the debugging section for more info on DRUN mode, this will print why desktop files are
discarded.

There are two advanced options to tweak the behaviour:

```css
configuration {
   drun {
      /** Parse user desktop files. */
      parse-user:   true;
      /** Parse system desktop files. */
      parse-system: false;
   }
}
```



### ssh

Shows a list of SSH targets based on your `ssh` config file, and allows to quickly `ssh` into them.

### keys

Shows a searchable list of key bindings.

### script

Allows custom scripted Modi to be added, see the **rofi-script(5)** manpage for more information.

### combi

Combines multiple modi in one list. Specify which modi are included with the `-combi-modi` option.

When using the combi mode, a *!bang* can be used to filter the results by modi.
All modi that match the bang as a prefix are included.
For example, say you have specified `-combi-modi run,window,windowcd`. If your
query begins with the bang `!w`, only results from the `window` and `windowcd`
modi are shown, even if the rest of the input text would match results from `run`.

If no match, the input is handled by the first combined modi.

## FAQ

### The text in the window switcher is not nicely aligned.

Try using a mono-space font.

### The window is completely black.

Check quotes used on the command-line: you might have used `“` ("smart quotes") instead of `"` ("machine quotes").

### What does the icon in the top right show?

The indicator shows:

    ` ` Case insensitive and no sorting.
    `-` Case sensitivity enabled, no sorting.
    `+` Case insensitive and Sorting enabled
    `±` Sorting and Case sensitivity enabled"

## EXAMPLES

Some basic usage examples of **rofi**:

Show the run dialog:

    rofi -modi run -show run

Show the run dialog, and allow switching to Desktop File run dialog (`drun`):

    rofi -modi run,drun -show run

Combine the run and Desktop File run dialog (`drun`):

    rofi -modi combi -show combi -combi-modi run,drun

Combine the run and Desktop File run dialog (`drun`), and allow switching to window switcher:

    rofi -modi combi,window -show combi -combi-modi run,drun

Pop up a text message claiming that this is the end:

    rofi -e "This is the end"

Pop up a text message in red, bold font claiming that this is still the end:

    rofi -e "<span color='red'><b>This is still the end</b></span>" -markup

Show all key bindings:

    rofi -show keys

Use `qalc` to get a simple calculator in **rofi**:

     rofi -show calc -modi "calc:qalc +u8 -nocurrencies"

## i3

In [i3](http://i3wm.org/) you want to bind **rofi** to be launched on key release. Otherwise, it cannot grab the keyboard.
See also the i3 [manual](http://i3wm.org/docs/userguide.html):

Some tools (such as `import` or `xdotool`) might be unable to run upon a KeyPress event, because the keyboard/pointer is
still grabbed. For these situations, the `--release` flag can be used, as it will execute the command after the keys have
been released.

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

**rofi** website can be found [here](https://davedavenport.github.io/rofi/)

## SUPPORT

**rofi** support can be obtained:
 * [GitHub Discussions](https://github.com/davatorium/rofi/discussions)
 * [Forum (Reddit)](https://reddit.com/r/qtools//)
 * [IRC](irc://irc.libera.chat:6697/#rofi) (#rofi on irc.libera.chat),

## DEBUGGING

To debug, it is smart to first try disabling your custom configuration:
`-no-config`

Disable parsing of configuration. This runs rofi in *stock* mode.

If you run custom C plugins, you can disable them using:

`-no-plugins`

Disables the loading of plugins.

To further debug the plugin, you can get a trace with (lots of) debug information.  This debug output can be enabled for
multiple parts in rofi using the glib debug framework. Debug domains can be enabled by setting the G_MESSAGES_DEBUG
environment variable. At the time of creation of this page, the following debug domains exist:

 * all: Show debug information from all domains.
 * X11Helper: The X11 Helper functions.
 * View: The main window view functions.
 * Widgets.Box: The Box widget.
 * Dialogs.DMenu: The dmenu mode.
 * Dialogs.Run: The run mode.
 * Dialogs.DRun: The desktop file run mode.
 * Dialogs.Window: The window mode.
 * Dialogs.Script: The script mode.
 * Dialogs.Combi: The script mode.
 * Dialogs.Ssh: The ssh mode.
 * Rofi: The main application.
 * Timings: Get timing output.
 * Theme: Theme engine debug output. (warning lots of output).
 * Widgets.Icon: The Icon widget.
 * Widgets.Box: The box widget.
 * Widgets.Container: The container widget.
 * Widgets.Window: The window widget.
 * Helpers.IconFetcher: Information about icon lookup.

The output of this can provide useful information when writing an issue.

More information (possibly outdated) see [this](https://github.com/DaveDavenport/rofi/wiki/Debugging%20Rofi) wiki entry.

## ISSUE TRACKER

The **rofi** issue tracker can be found [here](https://github.com/DaveDavenport/rofi/issues)

When creating an issue, please read [this](https://github.com/DaveDavenport/rofi/blob/master/.github/CONTRIBUTING.md)
first.

## SEE ALSO

**rofi-sensible-terminal(1)**, **dmenu(1)**, **rofi-theme(5)**, **rofi-script(5)**, **rofi-theme-selector(1)**

## AUTHOR

* Qball Cow <qball@blame.services>
* Rasmus Steinke <rasi@xssn.at>
* Quentin Glidic <sardemff7+rofi@sardemff7.net>

Original code based on work by: [Sean Pringle](https://github.com/seanpringle/simpleswitcher) <sean.pringle@gmail.com>

For a full list of authors, check the `AUTHORS` file.
