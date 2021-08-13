# V1.0.0 - Welcome in 2016

The new release, after a long development cycle, is finally out. It has a lot of changes that are hopefully invisible to
the user. On top of that we decided it is time to deprecate some old stuff left from the simpleswitcher era.
The biggest changes in this version are two major rewrites in the hope to improve maintainability and code quality.

The biggest changes in this release are:

## XLib to xcb

With the help of SardemFF7 **rofi** we broke free from the massif xlib and moved to xcb. This resulted in cleaner and
faster code. To offer all the functionality offered by the xlib version, we do depend on a relatively new xkbcommon
(0.5.0), luckily most distributions latest releases should be supporting this. In this move we manage to squash several
long standing bugs and open a possibility to fix more in the future.

## GLib Mainloop

Beside the large xcb move, we also started using a Glib Mainloop. This allowed us to remove several complex code
structures. This change should be mostly invisible for the user, beside the fact that the cursor in the entry box now
blinks.

## Config file

Not everybody seems to like configuration via Xresources, so for those people we now support a configuration file in
`XDG_CONFIG_HOME/rofi/config`, or passed from the commandline via the `-config` option. Settings in the config file will
override Xresources and is read on each startup.

## Version scheme

Rofi now no longer uses 0.year.month as release number, but switches to a more common scheme:
**major**.**minor**.**patch**. The release number 1.0.0 has no significant meaning, but was a logic followup on 0.15.12.
We added features and broke backwards compatibility.

## Better locale and UTF-8 handling

In our continious effort of making **rofi** handle UTF-8 and locales correctly we made the run dialog convert to and
from the locale filesystem encoding correctly, made the dmenu input parser more robust for handling invalid UTF-8 and do
we try to convert coming from Xorg.

## "Regression"

There where also some victims of the big rewrite, we decided to remove an old remnants from the simpleswitcher era,
namely daemon mode. In our opinion this is duplicate functionality, if you are using **rofi** it is very likely you are
either running a window manager (like i3) that implements global hotkey functionality, or running a keyboard daemon like
sxhkd.

A second victim, that had been marked deprecated for more then a year, is the old method of specifying themes where
every color option had one commandline flag. This method was very verbose and incomplete. With the [theme
repository](https://github.com/DaveDavenport/rofi-themes/) and the online [theme
generator](https://davedavenport.github.io/rofi/p11-Generator.html) using and creating new themes should be easy enough.

# Changelog

Below is a more complete changelog between the 0.15.12 and the 1.0.0 release.

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
* Run dialog: Try to convert between locale, fs encoding and utf8. Fixing problems with non-utf8 filesystem encodings.
* Try to display non-utf8 strings as good as possible.
* Autocomplete bang hint in combi mode (#380)
* Remove magic line length limits by switching to getline from fgets.
* Print git version for git builds in version string.

## Bug fixes

* Fix subpixel rendering. (#303)
* Fix basic tests on OpenBSD (#272)
* Fix wrong use of memcpy (thx to Jasperia).
* Work around for sigwaitinfo on OpenBSD.
* Ignore invalid entries (non-utf8) in dmenu mode.
* Glib signal handling.
* Fix connecting to i3 on bsd.
* Be able to distinguish between empty and cancel in dmenu mode. (#323)
* Fix memcpy on single memory region. (#312)
* Fix opening file with mode a+ and using fseek to overwrite on bsd.


## Regressions

* Removal of old themeing method. Given it was incomplete.
* Removal of daemon mode, given this duplicates Window Manager functionality.
