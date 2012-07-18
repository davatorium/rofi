# simpleswitcher

A popup window switcher roughly based on [superswitcher](http://code.google.com/p/superswitcher/), requiring only xlib and xft.

* Switcher centered on screen (or monitor for multi-head).
* Vertical list with three columns:
	1. Desktop number.
	2. Window class.
	3. Window title.
* Controls like dmenu:
	* Type to filter windows.
	* Arrows to highlight selections.
	* Return to select.
* Modes to display:
	* All windows on all desktops.
	* Only windows on current desktop.
* Runs in background or once-off.