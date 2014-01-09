# simpleswitcher

A popup window switcher roughly based on [superswitcher](http://code.google.com/p/superswitcher/), requiring only xlib and xft.

* Switcher centered on screen (or monitor for multi-head).

* Vertical list with two columns:
	2. Window class.
	3. Window title.

* Controls like dmenu:
	* Type to filter windows.
    * Tokonized filter. Type any word in any order to filter.
	* Arrows to highlight selections.
	* Return to select.

* Runs in background or once-off.

Hackish support for the I3 window manager:

 For i3, run with -i3

e.g.

  bindsym $mod+Tab exec simpleswitcher -i3 -now -font "snap-10" -fg "#505050" -bg "#000000" -hlfg "#ffb964" -hlbg "#000000" -o 85
