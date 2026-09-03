# 2.1.0

## GIO launch


## Authors

Thanks to all the people making this release possible:

 * SardemF77
 * Alex190291
 * Benny
 * Bjoernab
 * Colin
 * E-tho
 * Hanssen0
 * Hanssen
 * Istvan Petres
 * Jakob
 * Justin Faber
 * Killertofus
 * Lbonn
 * Lucas Ritzdorf
 * Milad Alizadeh
 * Mtoon
 * Nick H
 * Prithveerarya345
 * Ruedoux
 * Tomoron
 * Zebra2711


## Changelog

* Fix row icons and previews that fail to render (#2323)
* [Info] Dump used XDG directories in the '-info' command.
* [Display] Add some checks so we do not resolve NULL ptr. (#2320)
* [DOC] fix minor spelling typos (defintions, seperated, layed) (#2319)
* replace malloc and realloc with corresponding glib functions (#2316)
* Fix null pointer deref and skip misformed entry (#2315)
* Add fzf-v2 sorting method (#2306)
* [DOC] Minor spelling fixes (#2311)
* [DOC] fixing docs typo
* [Wayland] Fix IME positioning and display refresh
* [DOC] Describe `{window}` placeholder behavior under Wayland
* [WaylandWindow] Map ext to wlr toplevels; support window-command
* [WaylandWindow] Add protocol skeleton for ext-foreign-toplevel-list-v1
* [WaylandWindow] Rename wlr toplevel stuff in prep for ext toplevel
* [DOC] Update keybind descriptions for window, windowcd modes
* [WaylandWindow] Support accept-custom action to run entered commands
* [WaylandWindow] Honor window.close-on-delete config option
* [Listview] Remove weird x-offset (#2301)
* [Wayland] fix: sigbus with shm file (#2292)
* [Theme] Try to detect recursive importing and skip file (#2294)
* [rofi] Print warning messages using g_warning
* [DOC] Clarify that -show-icons applies to default theme.
* [Combi] makes combi compatible with `mode_preprocess_input` (#2288)
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
