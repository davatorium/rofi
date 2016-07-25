# V1.2.0 - Something Something Darkside 

## New Features

### Underline Match

A new, hopefully welcome, addition is that **Rofi** now  highlights the match in each row:

![Rofi Underline](./rofi-underline-match.png)

To accomplish this, now all matching is done using *GRegex*, as this returns the exact location in the string of each match.

## Multiline Select

While already existing, we now improved the multi-line select in **dmenu** mode. It will allow you to select and unselect rows, selected rows are highlighted with a dot and a small counter indicated the amount of rows selected.

![Rofi Multi Select](./rofi-multi-select.png)

## Customize Window string

You can now specify what the window switcher will show.
It allows for some markup to nicely line up the entries.

For example if the with of the window is specified in characters, this would right align the class name

```
rofi.window-format: {t:-16} ({c:10})
```

![Rofi Window title align](./rofi-window-align.png)

## Bug Fixes


* Fix current desktop window selector
* Fix launching application in terminal
* Support ```#include``` in config file.
* **XLib** dependency. The last hard dependency on **Xlib** has been removed by the use of
  **[xcb-util-xrm](https://github.com/Airblader/xcb-util-xrm)**.

## Remove features

* Removed fuzzy finder
* Remove **[i3](http://www.i3wm.org)** workarounds. As **i3** has, for more than a year now, native support for EWMH.
