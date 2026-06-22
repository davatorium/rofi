# 2.1.0

This release mainly includes small improvements and bug fixes.

## GIO launch

## Wayland: click-to-exit

Click-to-exit is finally enabled in Wayland by default.
Due to the way it is implemented, it has some side-effects in multi-monitor setup: rofi windows will not spill into other screens but be cropped out.

Thanks to @benny-e for helping out with this feature!

## Wayland: improved wayland window mode

TODO

## Changelog

* fixing docs typo
* Wayland: Fix IME positioning and display refresh
* Fix glib inclusion order
* [DOC] Describe `{window}` placeholder behavior under Wayland
* [WaylandWindow] Map ext to wlr toplevels; support window-command
* [WaylandWindow] Actually handle ext-foreign-toplevel-list-v1 messages
* [WaylandWindow] Add protocol skeleton for ext-foreign-toplevel-list-v1
* [WaylandWindow] Rename wlr toplevel stuff in prep for ext toplevel
* Remove CodeQL
* [DOC] Update keybind descriptions for window, windowcd modes
* [WaylandWindow] Support accept-custom action to run entered commands
* [WaylandWindow] Honor window.close-on-delete config option
* [Listview] Remove weird x-offset
* [Wayland] fix: sigbus with shm file (#2292)
* [Theme] Try to detect recursive importing and skip file
* [rofi] Print warning messages using g_warning
* [DOC] Clarify that -show-icons applies to default theme.
* [comb] makes combi compatible with `mode_preprocess_input` (#2288)
* Add transparent theme  (#2115)
* [Icon] Fix drawing size of icon.
* [DRUN] Change dbus to gio launch (#2275)
* [Wayland] Add click-to-exit
* [Wayland] Fix memory leak buffer pools
* [doc] Indicate smart-case is disabled by default.
* Add right-click cancel binding (#2244)
* [SSH] Updates to the SSH mode history saving/restoring (#2253)
* [DOC] fixup! use 'match:namespace' instead of 'match:class' for layerrules (#2251)
* [Script] Add extra checks for safety
* [xrmoptions] Re-structure struct to have less padding
* [XCB] Remove unreachable code.
* [Theme] Remove unreachable code.
* [ROFI] Put a (very high) limit on read from stdin
* [History] Check for buffer, not buffer_length
* [DRUN] Do some extra boundary checking.
* [SSH] Don't make new entries for aliases, put them on a single line. (#2240)
* [Help] Indicate that a plugin is 'external' in overview.
* Fix global_kb option description
* [Test] Add explicit rounding, making it behave same on 64/32
* [Doc] Clarify 'tokenize' option. (#2218)
* [Script] fix missing and wrong free
* [Helper] do NULL check on rofi_expand_path
* [filebrowser] sort based on natural ordering (#2201)
* [Script] add switch-mode option to the script mode (#2196)
* [Dmenu][Script] Add support for fallback icons (#2122)
* [XCB] Re-add signals for substructure notifications (#2194)
* [icon] Allow icons to be greyscaled and tinted with color (#2193, #2175)
* [dmenu] Fix async mode for multi-select (#2191)
* [Textbox] Add transpose keybinding (#2189)
* [Wayland] Request utf-8 charset on paste (#2190)
* [Wayland] wayland layer config option
* [Wayland] enables input method support for Rofi under Wayland.
* [View] Fix constness of rofi_view_handle_text.
* [script]: `ROFI_INPUT` for custom scripts to read the user input (#2187)
* [drun]: history not recorded for dbus-launched entries.
* [recursivebrowser] Add check for visited directory (#2181)
* Fixed -transient-window (#2178)
