# 1.7.4:  


# Changelog

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

# Thanks

Big thanks to everybody reporting issues.
Special thanks goes to:

* Iggy
* Morgane Glidic
* Danny Colin

Apologies if I mistyped or missed anybody.
