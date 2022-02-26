# 1.7.1: Turtley amazing!

This release focusses on fixing bugs found in 1.7.0. The most important ones
are fixing sizing bug, fix nested media statements and broken close-on-delete.
There are a few new features to help themeing: We now support
`env(ENV,default)` statement and when dumping a theme theme-names are resolved
(f.e. `green`, `lightblue`, etc.).

Thanks to everybody reporting bugs, providing patches that made this release possible.

For a more complete list of changes see below.

# Changelog

* [Theme] Fix highlight with only theme.
* Updated documentation and landing page (Thanks to RaZ0rr-Two)
* [Combi] Fix nesting combi modi (#1510)
* [DMenu] Fix crash dmenu mode when no entry is available. (#1504)
* [Run|Drun] Only initialize file-completer on first use.
* [FileBrowser] Reduce number of re-allocs.
* [Readme] Remove generating readme.html for dist.
* [Dmenu] Fix uninitialized memory (non-selectable)
* [FileBrowser] Try to convert invalid encoded text. (#1471)
* [FileBrowser] Don't crash on invalid file filenames. (#1471)
* [Theme] print known colors as its color name.
* [CMD] If failed to convert commandline to new option, do not stop. (#1425)
* [Theme] Fix parsing of nested media blocks. (#1442)
* [Widgets] Fix sizing logic widgets. (#1437)
* [Window] Try to fix auto-sizing of desktop names for non-i3 desktops. (#1439)
* [Window] Fix broken close-on-delete. (#1421)
* [Listview] Correctly check if selected item is highlighted. (#1423)
* [Entry] Allow action to be taken on input change. (#1405)
* [Theme] Don't truncate double values. (#1419)
* [Grammar] Add support for env(ENV,default).
* [Documentation] documentation fixes.
* [Theme] fix dmenu theme ( #1396).

# Thanks

Big thanks to everybody reporting issues.
Special thanks goes to:

* Iggy
* RaZ0rr-Two
* Morgane Glidic
* Danny Colin
* Tuure Piitulainen

Apologies if I mistyped or missed anybody.
