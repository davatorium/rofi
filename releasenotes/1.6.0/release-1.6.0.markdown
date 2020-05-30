# 1.6.0: 



## DRun


## Script mode


    * Support for invisible search text
    * Support for passing extra information back on selection
    * Support for custom keybindings
    * Support for custom delimiter
    * Support for dmenus no-custom option




## Theme

There have been a set of tweaks to the theme format, making it more flexible and hopefully more themer-friendly.


### Listview flexibility

Instead of the listview having a hacked textbox as elements. It now re-uses existing widgets like box, icon and textbox.
This way you can re-structure how it looks. For example put the icon above the text.


![Icons](./icons.png)


### Calculation support in theme format.

Rofi adds CSS like calculations in the CSS format for distances. 
You can now set the width of the window to the screen width minus a 10 pixels.

```css
window {
    width: calc(100% - 10px);
}
```

### Initial media support

This is a very initial implementation of CSS like `@media` support. This allows you to modify the theme
based on screen size or ratio.


##  Log

* Add `themes/` directory in the users rofi config directory to the theme search path. (#1001)
* Split listview element into box widget holding icon and textbox. Supporting more dynamic themes. (#949)
* Fix default theme.
* Add -upgrade-config option.
* Add ROFI_PLUGIN_PATH variable.
* Add check for running rofi inside a Script mode.
* Remove gnome-terminal from rofi-sensible-terminal (#1074)
* Set window title based on mode name. (#969)
* Subpixel rendering workaround. (#303)
* Support character type in configuration {} block . (#1131)
* Use XDG_CONFIG_DIRS (#1133)
* [Box] Bug fix update propagation.
* [DMenu] Add `-keep-right` flag. (#1089)
* [DMenu] Don't match markup when filtering. (#579,#1128)
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
* [Script] Add delimiter option. (#1041)
* [Script] Add environment variable indicating state.
* [Script] Add extra matchign field (meta). (#1052)
* [Script] Add info option, hidden field that gets passed to script via ROFI_INFO environment.
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

