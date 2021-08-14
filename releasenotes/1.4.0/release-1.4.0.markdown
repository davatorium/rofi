# V1.4.0: I reject your truth and trumpstitute my own

> This release contains some major changes. One of them being a new theme
> engine. The migration from older versions to this version might not go
> flawless.

With more then 750 commits since the last version, this is one of the biggest
releases so far.  In this version we used the groundwork laid in v1.3.0 and went
completely nuts with it. Hopefully this release should satisfy the die-hard
desktop [ricers](https://www.reddit.com/r/unixporn/) with a brand new theme
engine.  Lot of different colors, border, multiple fonts everything is now
possible.

Because of The great work done by [SardemFF7](https://github.com/SardemFF7/) the
code base is simplified and the key and mouse handling improved. The libraries
provided by SardemFF7 also made it possible to add a often requested feature of
icons (correctly using the icon-theme). A feature I never expected to be added.
To top this off, SardemFF7 added support to build rofi using
[meson](http://mesonbuild.com/).

A last big addition and still in beta, is support for plugins. Allowing the
addition of some weird/fancy features.  Currently two plugins are available,
[blezz](https://gitcrate.org/qtools/rofi-blezz) a quick launch menu with it own
menu definition and [top](https://gitcrate.org/qtools/rofi-top/) displaying
running processes.

Beside these major changes, this release includes a lot of bug-fixes and small
improvements. See the bottom of this release notes for a more complete list of
changes.


## Theme engine

The biggest new feature of this release is the theme engine. Building on the
changes made in v1.3.0 we implemented a new theme engine and it has a completely
new theme format. While the themes are a lot more verbose now, it does allow for
a lot of extra customizations.

It is now possible to theme each widget in rofi independently:

### Colors

You can now set the color on each widget independent in most of the CSS
supported color formats (hsl, cmyk, rgb, etc.) and each color can have a
transparency. There are three colors that can be set on each widget:

*  **background-color**
   Used to draw the background of the widget. Each widget is drawn on top of it
   parent, if the background is transparent, you will see the parents widget.
*  **border-color**
   Used to draw the borders.
*  **text-color**
   Used to draw text. If not set the foreground color is used.

![rainbox](rofi-rainbow.png)

### Borders

On every widget we can now configure a border for each of the four sides, the
color of the border, the style of the border (solid or dashed) and the radius of
the corners can be set.

![border1](rofi-border.png)

This combined with (fake) transparency can make for a very nice looking, rounded
rofi experience.

![border2](rofi-border-transp.png)

### Fonts

An often made request was support for different fonts for the entry box and the
list. With the new theme, it is possible to change the font and size of all
widgets.

![fonts](rofi-fonts.png)

> Note that opening a fonts is one of the slowest operations during rofi
> startup; having multiple fonts could have a significant impact on startup
> times.

### Flexible layout

To top all these changes, as an advanced feature the whole layout of the window
can be changed. Making it possible to mimic the original dmenu view, or make it
appear as a minimal context menu.

![dmenu](rofi-dmenu.png)


## Error reporting

The new theme parser will also be more verbose when encountering parsing errors,
hopefully helping debugging and modifying themes.

For example when forgetting a trailing ';' will report where it failed, and what
it expected (a ';').

![rofi-error](rofi-error.png)

## Importing

The new theme parser also support importing and overriding. This allow you to
make make modifications to an existing theme, without having to completely copy
it. For example, I want to use the `arthur` theme (shipped with rofi) but use
fake transparency, change the font off the result list and import a set of
overriding colors from `mycolors`.

```css
// Import the default arthur theme
@theme "arthur"

// Load in overriding of colors from mycolors.
@import "mycolors"

/* on the window widget, set transparency to use a screenshot of the screen. */
#window {
    transparency: "screenshot";
}
/* Override the font on the listview elements */
#element {
    font: "Ubuntu Mono 18";
}
```

## Icons

Another often made request, I never expected to be implemented, was icon
support. With the help of SardemFF7 an implementation was possible that
correctly follows the XDG icon specification and does not negatively impact the
performance. Currently the drun and the window switcher can application icons.
To enable icons, set the `show-icons` property to `true`.


![icons](rofi-icons.png)


## More flexible key and mouse bindings

Thanks to another great work of SardemFF7 you can now configure how the mouse
behave and bind modifier keys. It also improves on error messages. For example
it will now detect duplicate bindings.

For example to select an entry on single click, instead of double click:

```
rofi -show run -me-select-entry '' -me-accept-entry 'Mouse1'
```


## Fuzzy Matching

With thanks to Fangrui Song, the `fuzzy` matcher will now use a ranking similar
to `fzf` to sort the results.  This should hopefully to improve the results,
something a lot of users will appreciate.

Without:

![rofi no fzf](rofi-no-fzf.png)

With:

![rofi fzf](rofi-fzf.png)


## Initial Plugin support

> This feature is still in beta stage.

It is now possible to add custom mode via C plugins. This is allows interactive
modi to be added to rofi.  For example it is possible to have `top` (display
linux processes) mode:

![rofi top](rofi-top.png)

This mode allows sorting of the result on different keys (cpu usage, memory,
etc.) to be selected, programs to be killed and refreshes the results every 2
seconds.


See [here](https://gitcrate.org/qtools/rofi-top).

## Configuration File

> This feature is in alpha stage.

The new theme format can now (as an alpha) feature be used to set rofi's
configuration. In the future, when we add wayland support, we want to get rid of
the current Xresources (X11) based configuration format.  You can see how this
would look using: `rofi -dump-config`.

## Detailed Changelog

* Improved error messages
  * Theme parsing.
  * Keybinding. Duplicate bindings will now be reported.
  * Invalid commandline options.
  * Etc.
* Customizable highlight, allowing underline, strikethrough, italic, bold, small
  caps and the color to be set.
* Give up when keyboard is not grabbed in first 5 seconds.
* Improve manpage
    * rofi (1)
    * rofi-themes (5)
* Super-{1..10} hotkeys for selecting the first 10 rows.
* Allow x-align/y-align on textbox.
* Support matching bangs on multiple characters in combi mode. (#542)
* Set WM_CLASS (#549)
* Async pre-read 25 rows for improving user experience. (#550)
* Improve handling in floating window manager by always setting window size.
* DRun speedups.
* Make lazy-grab default.
* Remove extra layer in textbox. (#553)
* Ignore fonts that result in a family name or size 0. (#554)
* [Combi] Allow bang to match multiple modes. (#552)
* Add detection of window manager and work around quirks.
* Support dynamic plugins.
* DMENU tty detection.
* Support for icons in drun, combi and window mode.
* Startup notification of launched application support.
* Meson support.
* Fuzzy matching with fzf based sorting algorithm.
* Auto-detect DPI.
* Set cursor at the end of the input field. (#662)
* Meson support (thx to SardemFF7).
* [Script] parse the command as if it was commandline. (#650)
* Don't enable asan by meson. (#642)
* Allow text widgets to be added in theme and string to be set.
* [Dmenu] Support the -w flag.
* Allow window (via window id) to be location for rofi window.
* [Dmenu] Allow multi-select mode in `-no-custom` mode.
* Flex/Bison based parser for new theme format.
* Meson build support.
* Initial plugin support, exporting of pkg-config file for rofi.
* Improved positioning support for placing window on monitor.
* Allow rofi to be placed above window based on window id.
* Support different font per textbox.
  * Keep cache of previous used fonts.
