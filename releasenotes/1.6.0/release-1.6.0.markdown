# 1.6.0: The Masked Launcher

More then 2 years after the 1.5.0 release and a year after 1.5.4, we present rofi 1.6.0. This release
is again focusses bug-fixing and improving the experience for themers and
script developers. The script mode has been extended with many small requested tweaks to get it more
on par with dmenu mode. For themers the listview has been made more flexible, allowing more fancy themes,
for examples mimicking Gnomes application launcher or [albert](https://github.com/albertlauncher/albert).

Big thanks to [SardemFF7](https://www.sardemff7.net/) and all the other
contributors, without their support and contributions this release would not
have been possible.


## Script mode

Rofi now communicates some information back to the script using environment variables.
The most important one, is `ROFI_RETV`, this is equal to the return value in dmenu mode.
It can have the following values:

 * **0**: Initial call of script.
 * **1**: Selected an entry.
 * **2**: Selected a custom entry.
 * **10-28**: Custom keybinding 1-19


To fully read up on all features of script mode, there is now a `rofi-script(5)` manpage.

Some of the new features are:

 * Search invisible text
 * Pass extra information back on selection
 * Support for a custom delimiter
 * Support for dmenus no-custom option
 * Detect if launched from rofi


To test some of the features:

```bash
#!/usr/bin/env bash

if [ -z "${ROFI_OUTSIDE}" ]
then
    echo "run this script in rofi".
    exit
fi

echo -en "\x00no-custom\x1ftrue\n"
echo -en "${ROFI_RETV}\x00icon\x1ffirefox\x1finfo\x1ftest\n"

if [ -n "${ROFI_INFO}" ]
then
    echo "my info: ${ROFI_INFO} "
fi
```


## Theme

There have been a set of tweaks to the theme format, making it more flexible and hopefully more themer-friendly.


### Listview flexibility

This is one of the biggest change, instead of the listview having a hacked
textbox as elements. It now re-uses existing widgets like box, icon and
textbox.  This way you can re-structure how it looks. For example put the icon
above the text.


![Icons](./icons.png)

With theme:

```css
element {
  orientation: vertical;
}
```

This will make the box `element` put `element-icon` and `element-text` in a vertical list.

or change the ordering to show icon on the right:

```css
element {
  children: [element-text, element-icon];
}
```

![Icons vertical](./icons2.png)


![icon warning](./warning.png)
This causes a breaking change for themes, to modify the highlighting, this should be set to `element-text`.
Or inherited. `element-text { highlight: inherit; }`.

### Calculation support in theme format.

Rofi adds CSS like calculations in the CSS format for distances.
You can now set the width of the window to the screen width minus a 10 pixels.

```css
window {
    width: calc(100% - 10px);
}
```

It supports: `-`, `+`, `/`, `*` and `%` operators and they should be surrounded by whitespace.


### Initial media support

This is a very initial implementation of CSS like `@media` support. This allows you to modify the theme
based on screen size or ratio.

We currently support: minimum width, minimum height, maximum width, maximum
height, monitor id, minimum acpect ratio or maximum acpect ratio.


For example, go to fullscreen mode on screens smaller then full HD:

```
@media (max-width: 1920 ) {
  window {
    fullscreen: true;
  }
}
```


## List of Changes

* Add `themes/` directory in the users rofi config directory to the theme search path. (#1001)
* Split listview element into box widget holding icon and textbox. Supporting more dynamic themes. (#949)
* Fix default theme.
* Add -upgrade-config option.
* Add `ROFI_PLUGIN_PATH` variable.
* Add check for running rofi inside a Script mode.
* Remove gnome-terminal from rofi-sensible-terminal (#1074)
* Set window title based on mode name. (#969)
* Subpixel rendering workaround. (#303)
* Support character type in configuration {} block . (#1131)
* Use `XDG_CONFIG_DIRS` (#1133)
* [Box] Bug fix update propagation.
* [Build] Fix meson build with meson 0.55.
* [DMenu] Add `-keep-right` flag. (#1089)
* [DMenu] Don't match markup when filtering. (#579,#1128)
* [DRUN] Support Type=Link (#1166)
* [DRun] Add % to escape variable.
* [DRun] Add an optional cache for desktop files. (#1040)
* [DRun] Add keywords as default match item. (#1061)
* [DRun] Don't run custom commands.
* [DRun] Match keywords field.
* [DRun] Only show selected categories. (#817)
* [Dmenu|Script] Add non-selectable entries. (#1024)
* [Doc] Update documentation.
* [IconFetcher] Add jpeg support.
* [Icon] Set default size to 1.2 CH.
* [Icon] support distance for size.
* [Listview] Add widget to show keybinding index. (#1050)
* [Listview] Fix distribution of remaining space.
* [Listview] Fix left-to-right scrolling. (#1028)
* [Listview] Fix updating elements. (#1032)
* [Matching] Make Fuzzy matching non greedy.
* [Script] Add delimiter option. (#1041)
* [Script] Add environment variable indicating state.
* [Script] Add extra matchign field (meta). (#1052)
* [Script] Add info option, hidden field that gets passed to script via `ROFI_INFO` environment.
* [Script] Add no-custom option.
* [Textbox] Add cursor blinking option.
* [Textbox] Add placeholder. (#1020)
* [Theme] Add `calc()` support. (#1105)
* [Theme] Add alpha channel to highlight color. (#1033)
* [Theme] Add sidebar as mode-switcher alias.
* [Theme] Add some initial @media support. (#893)
* [Theme] Support buttons in the UI.
* [View] Add two widgets. One showing number of rows, other number of filtered rows. (#1026)
* [Window] Add window thumbnail option.
* [Window] Remove arbitrary # window limit. (#1047)
* [Window] check buffer overflow.
