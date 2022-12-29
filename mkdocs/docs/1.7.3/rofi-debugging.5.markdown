# ROFI DEBUGGING 5 rofi debugging

## NAME

Debugging rofi.

When reporting an issue with rofi crashing, or misbehaving. It helps to do some small test
to help pin-point the problem.

First try disabling your custom configuration: `-no-config`

This disables the parsing of the configuration files. This runs rofi in *stock* mode.

If you run custom C plugins, you can disable the plugins using: `-no-plugins`


## Get the relevant information for an issue

Please pastebin the output of the following commands:

```bash
rofi -help
rofi -dump-config
rofi -dump-theme
```

`rofi -help`  provides us with the configuration files parsed, the exact version, monitor layout
and more useful information.

The `rofi -dump-config` and `rofi -dump-theme` output gives us `rofi`
interpretation of your configuration and theme.

Please check the output for identifiable information and remove this.


## Timing traces

To get a timing trace, enable the **Timings** debug domain.

```bash
G_MESSAGES_DEBUG=Timings rofi -show drun
```

It will show a trace with (useful) timing information at relevant points during the execution.
This will help debugging when rofi is slow to start.

Example trace:

```
(process:14942): Timings-DEBUG: 13:47:39.335: 0.000000 (0.000000): Started
(process:14942): Timings-DEBUG: 13:47:39.335: 0.000126 (0.000126): ../source/rofi.c:main:786 
(process:14942): Timings-DEBUG: 13:47:39.335: 0.000163 (0.000037): ../source/rofi.c:main:819 
(process:14942): Timings-DEBUG: 13:47:39.336: 0.000219 (0.000056): ../source/rofi.c:main:826 Setup Locale
(process:14942): Timings-DEBUG: 13:47:39.337: 0.001235 (0.001016): ../source/rofi.c:main:828 Collect MODI
(process:14942): Timings-DEBUG: 13:47:39.337: 0.001264 (0.000029): ../source/rofi.c:main:830 Setup MODI
(process:14942): Timings-DEBUG: 13:47:39.337: 0.001283 (0.000019): ../source/rofi.c:main:834 Setup mainloop
(process:14942): Timings-DEBUG: 13:47:39.337: 0.001369 (0.000086): ../source/rofi.c:main:837 NK Bindings
(process:14942): Timings-DEBUG: 13:47:39.337: 0.001512 (0.000143): ../source/xcb.c:display_setup:1177 Open Display
(process:14942): Timings-DEBUG: 13:47:39.337: 0.001829 (0.000317): ../source/xcb.c:display_setup:1192 Setup XCB
(process:14942): Timings-DEBUG: 13:47:39.346: 0.010650 (0.008821): ../source/rofi.c:main:844 Setup Display
(process:14942): Timings-DEBUG: 13:47:39.346: 0.010715 (0.000065): ../source/rofi.c:main:848 Setup abe
(process:14942): Timings-DEBUG: 13:47:39.350: 0.015101 (0.004386): ../source/rofi.c:main:883 Load cmd config 
(process:14942): Timings-DEBUG: 13:47:39.351: 0.015275 (0.000174): ../source/rofi.c:main:907 Setup Modi
(process:14942): Timings-DEBUG: 13:47:39.351: 0.015291 (0.000016): ../source/view.c:rofi_view_workers_initialize:1922 Setup Threadpool, start
(process:14942): Timings-DEBUG: 13:47:39.351: 0.015349 (0.000058): ../source/view.c:rofi_view_workers_initialize:1945 Setup Threadpool, done
(process:14942): Timings-DEBUG: 13:47:39.367: 0.032018 (0.016669): ../source/rofi.c:main:1000 Setup late Display
(process:14942): Timings-DEBUG: 13:47:39.367: 0.032080 (0.000062): ../source/rofi.c:main:1003 Theme setup
(process:14942): Timings-DEBUG: 13:47:39.367: 0.032109 (0.000029): ../source/rofi.c:startup:668 Startup
(process:14942): Timings-DEBUG: 13:47:39.367: 0.032121 (0.000012): ../source/rofi.c:startup:677 Grab keyboard
(process:14942): Timings-DEBUG: 13:47:39.368: 0.032214 (0.000093): ../source/view.c:__create_window:701 xcb create window
(process:14942): Timings-DEBUG: 13:47:39.368: 0.032235 (0.000021): ../source/view.c:__create_window:705 xcb create gc
(process:14942): Timings-DEBUG: 13:47:39.368: 0.033136 (0.000901): ../source/view.c:__create_window:714 create cairo surface
(process:14942): Timings-DEBUG: 13:47:39.369: 0.033286 (0.000150): ../source/view.c:__create_window:723 pango cairo font setup
(process:14942): Timings-DEBUG: 13:47:39.369: 0.033351 (0.000065): ../source/view.c:__create_window:761 configure font
(process:14942): Timings-DEBUG: 13:47:39.381: 0.045896 (0.012545): ../source/view.c:__create_window:769 textbox setup
(process:14942): Timings-DEBUG: 13:47:39.381: 0.045944 (0.000048): ../source/view.c:__create_window:781 setup window attributes
(process:14942): Timings-DEBUG: 13:47:39.381: 0.045955 (0.000011): ../source/view.c:__create_window:791 setup window fullscreen
(process:14942): Timings-DEBUG: 13:47:39.381: 0.045966 (0.000011): ../source/view.c:__create_window:797 setup window name and class
(process:14942): Timings-DEBUG: 13:47:39.381: 0.045974 (0.000008): ../source/view.c:__create_window:808 setup startup notification
(process:14942): Timings-DEBUG: 13:47:39.381: 0.045981 (0.000007): ../source/view.c:__create_window:810 done
(process:14942): Timings-DEBUG: 13:47:39.381: 0.045992 (0.000011): ../source/rofi.c:startup:679 Create Window
(process:14942): Timings-DEBUG: 13:47:39.381: 0.045999 (0.000007): ../source/rofi.c:startup:681 Parse ABE
(process:14942): Timings-DEBUG: 13:47:39.381: 0.046113 (0.000114): ../source/rofi.c:startup:684 Config sanity check
(process:14942): Timings-DEBUG: 13:47:39.384: 0.048229 (0.002116): ../source/dialogs/run.c:get_apps:216 start
(process:14942): Timings-DEBUG: 13:47:39.390: 0.054626 (0.006397): ../source/dialogs/run.c:get_apps:336 stop
(process:14942): Timings-DEBUG: 13:47:39.390: 0.054781 (0.000155): ../source/dialogs/drun.c:get_apps:634 Get Desktop apps (start)
(process:14942): Timings-DEBUG: 13:47:39.391: 0.055264 (0.000483): ../source/dialogs/drun.c:get_apps:641 Get Desktop apps (user dir)
(process:14942): Timings-DEBUG: 13:47:39.418: 0.082884 (0.027620): ../source/dialogs/drun.c:get_apps:659 Get Desktop apps (system dirs)
(process:14942): Timings-DEBUG: 13:47:39.418: 0.082944 (0.000060): ../source/dialogs/drun.c:get_apps_history:597 Start drun history
(process:14942): Timings-DEBUG: 13:47:39.418: 0.082977 (0.000033): ../source/dialogs/drun.c:get_apps_history:617 Stop drun history
(process:14942): Timings-DEBUG: 13:47:39.419: 0.083638 (0.000661): ../source/dialogs/drun.c:get_apps:664 Sorting done.
(process:14942): Timings-DEBUG: 13:47:39.419: 0.083685 (0.000047): ../source/view.c:rofi_view_create:1759 
(process:14942): Timings-DEBUG: 13:47:39.419: 0.083700 (0.000015): ../source/view.c:rofi_view_create:1783 Startup notification
(process:14942): Timings-DEBUG: 13:47:39.419: 0.083711 (0.000011): ../source/view.c:rofi_view_create:1786 Get active monitor
(process:14942): Timings-DEBUG: 13:47:39.420: 0.084693 (0.000982): ../source/view.c:rofi_view_refilter:1028 Filter start
(process:14942): Timings-DEBUG: 13:47:39.421: 0.085992 (0.001299): ../source/view.c:rofi_view_refilter:1132 Filter done
(process:14942): Timings-DEBUG: 13:47:39.421: 0.086090 (0.000098): ../source/view.c:rofi_view_update:982 
(process:14942): Timings-DEBUG: 13:47:39.421: 0.086123 (0.000033): ../source/view.c:rofi_view_update:1002 Background
(process:14942): Timings-DEBUG: 13:47:39.428: 0.092864 (0.006741): ../source/view.c:rofi_view_update:1008 widgets
```


## Debug domains

To further debug the plugin, you can get a trace with (lots of) debug information. This debug output can be enabled for
multiple parts in rofi using the glib debug framework. Debug domains can be enabled by setting the G\_MESSAGES\_DEBUG
environment variable. At the time of creation of this page, the following debug domains exist:

 * all: Show debug information from all domains.
 * X11Helper: The X11 Helper functions.
 * View: The main window view functions.
 * Widgets.Box: The Box widget.
 * Modes.DMenu: The dmenu mode.
 * Modes.Run: The run mode.
 * Modes.DRun: The desktop file run mode.
 * Modes.Window: The window mode.
 * Modes.Script: The script mode.
 * Modes.Combi: The script mode.
 * Modes.Ssh: The ssh mode.
 * Rofi: The main application.
 * Timings: Get timing output.
 * Theme: Theme engine debug output. (warning lots of output).
 * Widgets.Icon: The Icon widget.
 * Widgets.Box: The box widget.
 * Widgets.Container: The container widget.
 * Widgets.Window: The window widget.
 * Helpers.IconFetcher: Information about icon lookup.

For full list see `man rofi`.

Example: `G_MESSAGES_DEBUG=Dialogs.DRun rofi -show drun` To get specific output from the Desktop file run dialog.

To redirect the debug output to a file (`~/rofi.log`) add:

```
rofi -show drun -log ~/rofi.log
```

Specifying the logfile automatically enabled all log domains.
This can be useful when rofi is launched from a window manager.


## Creating a backtrace.

First make sure you compile **rofi** with debug symbols:

```bash
make CFLAGS="-O0 -g3" clean rofi
```

Getting a backtrace using GDB is not very handy. Because if rofi get stuck, it grabs keyboard and
mouse. So if it crashes in GDB you are stuck.
The best way to go is to enable core file. (ulimit -c unlimited in bash) then make rofi crash. You
can then load the core in GDB.

```bash
gdb rofi core
```

Then type inside gdb:

```
thread apply all bt
```

The output trace is useful when reporting crashes.

Some distribution have `systemd-coredump`, this way you can easily get a backtrace via `coredumpctl`.

## SEE ALSO

**rofi-sensible-terminal(1)**, **dmenu(1)**, **rofi-debugging(5)**, **rofi-theme(5)**, **rofi-script(5)**, **rofi-keys(5)**,**rofi-theme-selector(1)**

## AUTHOR

* Qball Cow <qball@blame.services>
