# V1.2.0 - 8397

## New Features

Despite me saying after every release that it is mostly feature complete;  new **Rofi**, new features.
However these new features are mostly to improve current functionality and debugging.
Below the 4 most important ones.

### Underline Match

A new, hopefully welcome, addition is that **Rofi** now highlights the match in each row:

![Rofi Underline](./rofi-underline-match.png)

To accomplish this, now all matching is done using *GRegex*, as this returns the exact location in the string of each match.
While I don't see a direct use, it is something a lot of other *quick search* tools provide, so **Rofi** could not stay
behind.

## Multiline Select

While already existing in a very rudimentary form, we now improved the multi-line select in **dmenu** mode. It will
allow you to select and unselect rows, selected rows are highlighted with a dot and a small counter indicated the amount
of rows selected.

![Rofi Multi Select](./rofi-multi-select.png)

## Customize Window string

You can now specify what the window switcher will show.
It allows for some markup to nicely line up the entries.

For example if the with of the window is specified in characters, this would right align the class name

```
rofi.window-format: {t:-16} ({c:10})
```

![Rofi Window title align](./rofi-window-align.png)

## Track configuration option origin

**Rofi** now keeps track of how configuration options are set. It will now display if it is the default value, set in
Xresources, configuration file or commandline.

![Rofi configuration tracking](./rofi-options.png)

This should help debugging recent bugs, where people had an invalid `pid` path set in the configuration file.
Additionally if you dump the configuration, for using on another pc, it will comment the options that are set to their
default value. (So f.e. pidfile location won't be overriden).

## Bug Fixes

As no tool is without bugs, and **Rofi** not being the exception, we did manage to squash a few.

* Fix current desktop window selector.
* Fix launching application in terminal.
* Support ```#include``` in config file.
* Fix rofi on 30bit 10 bit per channel display.
* Correct `Control-u` behaviour to remove till begin of line. `Control-w` now whipes the line.
* Add missing `Control-k` keybinding, delete till end of line.

## Remove features

* Removed fuzzy finder
* Remove **[i3](http://www.i3wm.org)** workarounds. As **i3** has, for more than a year now, native support for EWMH.
* Remove **XLib** dependency. The last hard dependency on **Xlib** has been removed by the use of
  **[xcb-util-xrm](https://github.com/Airblader/xcb-util-xrm)**.
