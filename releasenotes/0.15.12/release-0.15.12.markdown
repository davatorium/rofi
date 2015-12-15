# Release 0.15.12

The 0.15.12 release of **rofi** focussed on improving overal user experience.
This includes fixing problems with complex keyboard layouts, locale aware matching and more. This release also include
(some experimental) speedups. If enabled, **rofi** will make full use of all available cores to speedup searching.
The final improvement, it related to creating and distributing of themes. Thanks to the great work of SardemFF7 we now
have an online themenator.
Below I will highlight some of the bigger changes in more details

## Speedups

## Themes

### Theme repository

### Themenator

### Dump themes

To be removed:
0.15.12 (unreleased):
    New features:
        - Initial `-dump` command for dmenu mode. (#216)
        - Threading support.
            - Parallel checking for ASCII.
            - Parallel string matching.
            - Autodetect number of HW-threads.
            - Disabled by default.
        - Highlight multiple selected rows (#287,#293)
        - Dmenu can read from file instead of stdin.
        - Regex matching (#113)
        - Take Screenshot of rofi using keybinding.
        - Hotkey for sorting: (#298)
		- Option to set scrollbar width.
    Improvements:
		- Fix return code of multi-select.
        - Update manpage (#289, #291)
        - Improve speed of reading stdin in dmenu mode.
        - Correctly handle modifier keys now. Should now support most weird keyboard layouts and switching between them.
        (#268, #265, #286)
        - Correctly set locale, fixing issues with entering special characters. (#282)
        - DRun mode support `NoDisplay` setting (#283)
        - Pango markup is matched when filtering. (#273)
    Bug fixes:
        - Improve error message (#290)
        - Correctly switch to next entry on multi-select when list is filtered (#292)
        - Switch __FUNCTION__ for __func__. (#288)
        - Fix segfault on empty list and moving through it. (#256,#275)
        - Fix one off of colors (#269)
        - Drun fix uninitialized memory (#285)
