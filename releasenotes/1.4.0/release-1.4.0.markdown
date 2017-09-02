# V1.4.0: I reject your truth and trumpstitute my own

> This release contains some major changes. One of them being a new theme engine. The migration from older versions
> to this version might not go flawless.

With more then 700 commits since the last version, this is one of the biggest releases so far.
In this version we used the groundwork laid in v1.3.0 and went completely nuts. This release should satisfy the die-hard
desktop [ricers](https://www.reddit.com/r/unixporn/) with a brand new, CSS based, theme engine.

The great work of [SardemFF7](https://github.com/SardemFF7/) simplified the code base and improved the key and mouse
handling. It also made it possible to add a often requested feature of icons (correctly using the icon-theme). A feature
I never expected to be added.

A last big addition and still in beta, is support for plugins. Allowing the addition of some weird/fancy features.
Currently two plugins are available, [blezz](https://gitcrate.org/qtools/rofi-blezz) a quick launch menu with it own
menu definition and [top](https://gitcrate.org/qtools/rofi-top/) displaying running processes.

Beside these major changes, this release includes a lot of bug-fixes and small improvements. See the bottom of this
release notes for the full list of changes.


## CSS Like Theme engine

The biggest new feature of this release is the theme engine. Building on the changes made in v1.3.0 we implemented a new
theme engine and it has a completely new theme format modelled after CSS. We decided to model the theme format after
CSS because many people are familiar with it and it seems to be a reasonable fit. While the themes are a lot more
verbose now, it does allow for a lot of extra customizations.

It is now possible to theme each widget in rofi independently:

### Colors

You can now set the color on each widget independent in most of the CSS supported color formats (hsl, cmyk, rgb, etc.)
and each color can have a transparency. Each widget has three colors: background, foreground and text.

![rainbox](rofi-rainbow.png)

### Borders

On every widget we can now configure a border for each of the four sides, the color of the border, the style of the
border (solid or dashed0) and the radius of the corners can be set.

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


![icons](rofi-icons.png)


## More flexible key and mouse bindings

## Fuzzy Matching

## Initial Plugin support

## Screenshot

## Configuration File

The new theme format can now (as an alpha) feature be used to set rofi's configuration. In the future, when we add
wayland support, we want to get rid of the current Xresources (X11) based configuration format.
You can see how this would look using: `rofi -dump-config`.

## Detailed Changelog


* DMENU tty detection.
* Startup notification of launched application support.
* Meson support.
* Fuzzy matching with fzf based sorting algorithm.
* Auto-detect DPI.
