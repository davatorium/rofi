# V1.4.0: I reject your truth and trumpstitute my own

> This release contains some major changes. One of them being a new theme engine. The migration from older versions
> to this version might not go flawless. 

With more then 700 commits since the last version, this is one of the biggest releases so far.
In this version we used the groundwork laid in v1.3.0 and went completely nuts. This release should satisfy the die-hard
desktop [ricers](https://www.reddit.com/r/unixporn/) with a brand new, CSS based, theme engine.

The great work of [SardemFF7](https://github.com/SardemFF7/) simplified the code base and improved the key and mouse
handling. It also made it possible to add a often requested feature of icons (correctly using the icon-theme). A feature
I never expected to be added.

A last big addition, is support for plugins. Allowing the addition of some weird/fancy features. Currently two plugins
are available, [blezz](https://gitcrate.org/qtools/rofi-blezz) a quick launch menu with it own menu definition and
[top](https://gitcrate.org/qtools/rofi-top/) displaying running processes.

Beside these major changes, this release includes a lot of bug-fixes and small improvements. See the bottom of this
release notes for the full list of changes.

## CSS Like Theme engine

The biggest new feature of this release is the theme engine. It has a completely new theme format modelled after CSS.
This allows for a lot more customizations. 

### Colors

You can now set the color on each widget independent in most of the CSS supported color formats (hsl, cmyk, rgb, etc.).

> TODO: add picture of rofi rainbow colors.

### Borders

On every widget we can now configure a border for each of the four sides, the color of the border, the style of the
border and the radius of the corners.

> TODO: Add picture of crazy borders.

This combined with (fake) transparency can make for a very nice looking, rounded rofi experience.

> TODO: Rounded corner rofi.

### Fonts

An often made request was support for different fonts for the entry box and the list. With the new theme, it is possible
to change the font and size of all widgets.

> TODO: picture of mixture of fonts.

### Flexible layout

To top all these changes, as an advanced feature the whole layout of the window can be changed. Making it possible to
mimic the original dmenu view, or make it appear as a minimal context menu.

> TODO: insert two screenshot.

## Icons

Another often made request, I never expected to be implemented, was icon support. But with the help of SardemFF7 an
implementation was possible that correctly follows the XDG icon specification and does not negatively impact the
performance.

> TODO: Screenshot with icons.


## More flexible key and mouse bindings

## Fuzzy Matching

## Initial Plugin support

## Screenshot

## Detailed Changelog


* DMENU tty detection.
* Startup notification of launched application support.
* Meson support.
* Fuzzy matching with fzf based sorting algorithm.
* Auto-detect DPI.
