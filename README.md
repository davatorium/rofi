# simpleswitcher

A popup window switcher roughly based on [superswitcher](http://code.google.com/p/superswitcher/), requiring only xlib and xft.
This version is based on the version from [Sean
Pringle](http://github.com/seanpringle/simpleswitcher). All credit for this great tool should go to him.

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

If compiled with I3 support, it should autodetect if I3 window manager is running. 

Usage
-----

e.g.

  bindsym $mod+Tab exec simpleswitcher -now -font "snap-10" -fg "#505050" -bg "#000000" -hlfg "#ffb964" -hlbg "#000000" -o 85


Compilation
-----------

Type `make I3=` to disable compiling with i3 support.
If during compilation it complains about not finding i3/ipc.h either disable i3 support
or install the headers.

Type `make PREFIX=<path> install` to install in a different prefix.

Archlinux
---------

This version of simpleswitcher has been made available on the
[AUR](https://aur.archilinux.org/packages/simpleswitcher-dd-git/) by 1ace.
