# V1.0.0 - Welcome in 2016

The current release, after a long development cycle, is finally out. This version features two major rewrites in the
hope to improve maintainability and code quality.

## XLib to xcb

With the help of SardemFF7 **rofi** we broke free from the massif xlib and moved to xcb. This resulted in cleaner and
faster code. 

## GLib Mainloop

Beside the large xcb move, we also started using a Glib Mainloop. This allowed us to remove several complex code
structures. This change should be mostly invisible for the user, beside the fact that the cursor in the entry box now
blinks.

## Config file

Not everybody seems to like configuration via Xresources, so for those people we now support a configuration file in
`XDG_CONFIG_HOME/rofi/config`, or passed from the commandline via the `-config` option.

## "Regression"

There where also some victims of the big rewrite, we decided to remove an old remnants from the simpleswitcher era,
namely daemon mode. In our opinion this is duplicate functionality, if you are using **rofi** it is very likely you are
either running a window manager (like i3) that implements global hotkey functionality, or running a keyboard daemon like
sxhkd. 

Also the old method of specifying themes has been removed.

# Changelog

## New Features

* Blinking cursor
* Separate configuration file
* History in drun mode (#343)
* Context menu mode, show **rofi** at the mouse pointer

## Improvement

* auto select and single item on dmenu mode (#281)
* Unlimited window title length.
* Correctly follow the active desktop, instead of active window.
* If requesting modi that is not enabled, show it anyway.
* DMenu password mode. (#315)
* Levenshtein sort is now UTF-8 aware.
* Use xcb instead of large xlib library.
* Use GLib mainloop for cleaner code and easier external event based handling in future.

## Bug fixes

* Fix subpixel rendering. (#303)
* Fix basic tests on OpenBSD  (#272)
* Fix wrong use of memcpy (thx to Jasperia).
* Work around for sigwaitinfo on OpenBSD.
* Ignore invalid (non-utf8) in dmenu mode.
* Glib signal handling.
* Fix connecting to i3 on bsd.
* Be able to distinguish between empty and cancel. (#323)
* Fix memcpy on single memory region. (#312)
* Fix that opening file with mode a+ does not allow fseek on bsd.


## Regressions

* Removal of old themeing method. Given it was incomplete.
* Removal of daemon mode, given this duplicates Window Manager functionality.

