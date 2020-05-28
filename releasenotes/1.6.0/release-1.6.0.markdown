# 1.6.0: 




##  Log

* Add `themes/` directory in the users rofi config directory to the theme search path. (#1001)
* Split listview element into box widget holding icon and textbox. Supporting more dynamic themes. (#949)
* Fix default theme.
* [DRun] Only show selected categories. (#817)
* [DMenu] Don't match markup when filtering. (#579,#1128)
* [DRun] Don't run custom commands.
* Set window title based on mode name. (#969)
* [Icon] support distance for size.
* [Icon] Set default size to 1.2 CH.
* [DRun] Match keywords field.
* [Theme] Add sidebar as mode-switcher alias.
* [Theme] Support buttons in the UI.
* [Theme] Add some initial @media support. (#893)
* [Window] Add window thumbnail option.
* [Textbox] Add cursor blinking option.
* [View] Add two widgets. One showing number of rows, other number of filtered rows. (#1026)
* [Box] Bug fix update propagation.
* [Listview] Fix left-to-right scrolling. (#1028)
* [Theme] Add alpha channel to highlight color. (#1033)
* [Listview] Fix updating elements. (#1032)
* [Doc] Update documentation.
* [Window] Remove arbitrary # window limit. (#1047)
* [Textbox] Add placeholder. (#1020)
* [Listview] Add widget to show keybinding index. (#1050)
* [Listview] Fix distribution of remaining space.
* Add -upgrade-config option.
* [DRun] Add an optional cache for desktop files. (#1040)
* [Script] Add extra matchign field (meta). (#1052)
* [Dmenu|Script] Add non-selectable entries. (#1024)
* [IconFetcher] Add jpeg support.
* [DRun] Add keywords as default match item. (#1061)
* Remove gnome-terminal from rofi-sensible-terminal (#1074)
* [Script] Add delimiter option. (#1041)
* [Script] Add environment variable indicating state.
* [DRun] Add % to escape variable.
* [DMenu] Add `-keep-right` flag. (#1089)
* Add ROFI_PLUGIN_PATH variable.
* Use XDG_CONFIG_DIRS (#1133)
* [Script] Add no-custom option.
* [Theme] Add `calc()` support. (#1105)
* Subpixel rendering workaround. (#303)
* Support character type in configuration {} block . (#1131)
* Add check for running rofi inside a Script mode.
* [Window] check buffer overflow.
* [Script] Add info option, hidden field that gets passed to script via ROFI_INFO environment.

