# simpleswitcher

A popup window switcher roughly based on [superswitcher](http://code.google.com/p/superswitcher/), requiring only xlib and xft.

* Switcher centered on screen (or monitor for multi-head).

* Vertical list with three columns:
	2. Window class.
	3. Window title.

* Controls like dmenu:
	* Type to filter windows.
	* Arrows to highlight selections.
	* Return to select.

* Runs in background or once-off.

For i3, run with -i3
e.g.
  bindsym $mod+Tab exec simpleswitcher -i3 -now -font "snap-10" -fg "#505050" -bg "#000000" -hlfg "#ffb964" -hlbg "#000000" -o 85
