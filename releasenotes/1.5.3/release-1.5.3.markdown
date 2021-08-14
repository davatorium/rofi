# 1.5.3: Time delayed progress

Rofi 1.5.3 is another bug-fix release in the 1.5 series.

There is one breaking change in the theme naming and there are a few small new features (tweaks) in this release.

## Sidebar renamed to mode-switcher

The selection buttons to select between the different modi was still called `sidebar` a remnant from the past.
This has now been renamed to `mode-switcher`.

## Icons in dmenu

Just like in window,drun and script modi you can add icons to the list in dmenu mode.

The syntax is similar to the script modi:

```bash
echo -en "Firefox\0icon\x1ffirefox\ngimp\0icon\x1fgimp" | rofi -dmenu -no-config -show-icons
```

![dmenu icons](rofi-dmenu-icons.png)


The entries are separated by a `\n` newline (normal dmenu behaviour).
The extra parameters can be added after a `\0` null character, the key and value are separated by a `\x1f` unit
separator character.


## Ellipsizing listview entries

If you have very long entries in your view that get ellipsized (cut off at the end indicated by ...) you can now select,
at runtime, where they are cut off (start, middle or end).

You cycle through the options with the `alt+.` keybinding.

Start:

![dmenu ellipsize](rofi-ellipsize-start.png)


Middle:

![dmenu ellipsize](rofi-ellipsize-middle.png)


End:

![dmenu ellipsize](rofi-ellipsize-end.png)


## Full Changelog

The full list of fixes and updates:

* Update manpage with missing entry. (#937)
* Rename sidebar widget to mode-switcher and allow configuration from theme.
* Timing: Moving timing output to glib debug system.
* SSH: Fix unitialized variable issue.
* SSH: resolve ':' conflict in history entries.
* SSH: be case-insensitive when parsing keywords.
* RASI Lexer: Fix nested () in variable default field.
* USABILITY: When mode not found, show in gui not just on commandline.
* ICON: Allow aligning image in icon widget.
* Meson build system: cleanups and improvements.
* Meson build system: add documentation (#943)
* Window: Fix default formatting and remove (invalid) deprecation warning.
* DMenu: Add support for showing icons infront of displayed list.
* Overlay: Fix overlay widget to correctly integrate in new theme format.
* Update libnkutils, libgwater.
* DMENU: Add format option to strip pango markup from return value.
* ListView: allow user to change ellipsizing displayed value at run-time.
