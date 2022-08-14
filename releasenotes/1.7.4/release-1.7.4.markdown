# 1.7.4: Preshellected Optimizations

Another maintenance and small features expansion release. A lot of small
annoyances have been fixed and ignored errors are now more visually flagged to
the user. In the past typos in the theme could result into broken themes
without any warning to the user (except in debug mode), if an unknown link is
found it will now throw an error. To help the user find the right
documentation, the manpages are further split up into sub-pages and are
expanded

We now have:

* rofi(1)
* rofi-theme-selector(1)
* rofi-keys(5)
* rofi-theme(5)
* rofi-debugging(5)
* rofi-dmenu(5)
* rofi-script(5)

Another improvement made that can have huge impact on the user-experience is a
significant speedup in the async input reading of dmenu. It turned out glib's
GInputStream async methods are very slow. On large input sets where it was
supposed to improve the user experience, it made it unusable. To resolve this a
custom implementation has been made. Background loading is now close to the
same speed as loading at start before displaying. A million item list is now
near instant. On very large lists, the instant filtering automatically changes
to be postponed until the user stops typing. This severely reduces system load
and interface blocking.

A few long standing feature requests and bug reports have been implemented or fixed:

* Listview flow. You can now change the flow in the listview from vertical first
  to horizontal first. Making it mimic tables.

```bash
for i in {1..90}; do echo $i; done | rofi -dmenu -no-config -theme-str 'listview { columns: 3; flow: vertical; }'
```
![Vertical](./vertical.png)

```bash
for i in {1..90}; do echo $i; done | rofi -dmenu -no-config -theme-str 'listview { columns: 3; flow: horizontal; }'
```
![Horizontal](./horizontal.png)

* You can set a custom fallback icon for each mode.

```css
configuration {
  run,drun {
    fallback-icon: "application-x-addon";
  }
}
```

* In dmenu mode (and script) you can now make (some) changes to the theme, for
  example modifying the background color of the entry box.

```bash
echo -en "\0theme\x1felement-text { background-color: red;}\n"
```

* User scripts (for script mode) into `$XDG_CONFIG_HOME/rofi/scripts` directory
  are automatically available in rofi.

```bash
rofi -h
<snip>
Detected user scripts:
        • hc (/home/qball/.config/rofi/scripts/hc.sh)
</snip>
```

This script can now by shows by running `rofi -show hc`.

* You can now render text as icons, this allows you to use glyphs icon fonts as
  icons.

```bash
echo -en "testing\0icon\x1f<span color='green'>Test</span>" | rofi -dmenu
```

* Hide listview when unfiltered. (#1079) 

```css
listview {
    require-input: true;
}
```

* You can now add a separate icon or textbox widget to the UI that displays the
  current selected item. As an example see the included `sidebar-v2`.

* A bug was fixed that caused problems with newer xkeyboard-config versions and
  different keyboard layouts.

Below is a more complete list of changes:

# Changelog

* [Doc] Add `-config` to `-help` output. (#1665)
* [Dmenu] Fix multi-select, use text as indicator.
* [filebrowser] Fix building on Mac. (#1662,#1663)
* [textbox] Implement text-transform option. (#1010)
* [script] Add `new-selection` (#1064).
* [Script] Add keep-selection flag that keeps the current selection. (#1064)
* [Debug]  Add '-log' flag to redirect debug output to a file.
* [XCB]    Try to deduce rotated monitors.
* [Doc]    Add rofi-dmenu to 'See also' in rofi(1).
* [Options] Mark offset(s) as deprecated.
* [Modes] Support loading multiple icon sizes.
* [View]  Add textbox|icon-current-entry widget.
* [Doc]   Add ascii manpage to rofi-script(5).
* [Script] Print user-scripts in -h
* [Script] Look into $XDG_CONFIG_HOME/rofi/scripts/ for user scripts.
* [Dmenu|Script] Allow (some) theme modification from script/dmenu.
* [Textbox] Fix some pango alignment.
* [FileBrowser] Bind 'kb-accept-custom-alt' to directory up.
* [Build] Add desktop files as per complaint (rofi theme selector and rofi itself).
* [Dmenu] Code cleanups.
* [Themes] Remove broken themes.
* [Modes] Allow fallback icon per mode. (#1633)
* [View] Fix broken anchoring behaviour. (#1630)
* [Rofi] Move error message on commandline to UI.
* [Listview] Option to hide listview elements when not filtered. (#1622,#1079)
* [DMenu] Speed up reading async in of large files from stdin.
* [FileBrowser] Make accept-alt open folder if selected.
* [Helper] Add XDG_DATA_DIRS to the theme search path. (#1619)
* [Doc] Split up manpages and extend them with examples.
* [Doc] Highlight use of `-dump-config` in config. (#1609)
* [Config] Workaround for in data type passed to string option.
* [Listview] Allow flow of elements to be set. (#1058)
* [Script] Add data field that gets passed to next execution. (#1601)
* Change modi to modes to avoid confusion.
* [Theme] When links are unresolvable throw an error to the user.
* [DMenu] Document the `display-columns` and `display-column-separator` option.
* [Theme] Media now supports `enabled` that supports an environment variable.
* [IconFetcher] Support rendering fonts as icon.
* [xcb] Remove work-around to fix use with new xkeyboard-config (#1642)

# Thanks

Big thanks to everybody reporting issues.
Special thanks goes to:

* Iggy
* Morgane Glidic
* Danny Colin

Apologies if I mistyped or missed anybody.
