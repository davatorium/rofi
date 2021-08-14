# V1.3.0: Dan vs. Greg: The never ending story.

This release mostly focussed on cleaning up and refactoring the internals. One of the big changes is that the gui is no
longer based on hard-coded positions. We introduced a widget system with boxes, scrollbars, separators, listview and
textboxes.  The boxes (vertical and horizontal) allows us to nice structure the layout and have them resize according to
rules when the window changes size.

![structure](structure.png)

This allowed us to make changes at run-time. In the future I hope we can use this to improve the theming (think css like
themes).  The cleanup and refactoring is not finished and will continue for the next release.

Beside this we still managed to get some new features in:

## Dynamic window size

Enabled by the refactoring, rofi can now resize the window to fit the number of visible entries, so as you type and the
list of options becomes small, so does the windows (this is disabled by default). Rofi will try to keep the text box at
the same place so you don't have to move focus, this means that at the bottom of the screen the window layout is
reversed so the textbox is at the bottom.

![Resize action](resize.gif)

### Theme selector

To make it easier to get a good looking rofi, we included a theme-selector script and ship rofi with a set of themes.
The script allows you to preview themes, and make them the default theme.

![Theme Selector](theme-selector.gif)

It seems byzanz (to record the gif) changes the colors a bit. It looks better in real-life.

### Fuzzy parser

On many request, the fuzzy matcher has been re-added:

![fuzzy](fuzzy.png)

### ASync DMENU

Rofi can read input data for dmenu asynchronous from displaying. So if you have something that takes a time to produce,
you can see the progress, start filtering and selecting entries before it finishes.
This can be very useful when searching through large data sets.
In the below screenshot it keeps feeding rofi the content of the directory. Rofi indicates it is still receiving data by
the `loading...` text.

![async](dmenu-async.png)

Async mode is not always possible, and will be disabled if not possible.

### Drawing improvements

In this release rofi is more efficient in drawing its content to the X11 window. The basic redraw on expose has been
speedup by a factor 1000. This was obtained by using a server side copy of the internal surface, instead of painting it
with cairo each time. Especially for large (4k and higher) screens, in fullscreen mode, this reduces redrawing from +-
180ms to 0.1 ms. A second improvement was made by configuring X11 not repaint the window background when resized, this
caused some flickering (it was painted black before being drawn). It now keeps the old content and tells rofi to
repaint. This causes trailing, but overall looks nicer.

## Removals

We also removed a deprecated option, `-opacity`. Did option did full window opacity, basically it makes the whole window
and text transparent. A very ugly effect. The current argb colors in the theme allow a nice transparency, where only the
background of the window is transparent but not the text (you can still reproduce the old style in the new theme format,
by making all colors transparent).


## Detailed Changelog

### New Features

- Use randr for getting monitor layout. Fallback to xinerama if not available.
- Re-add fuzzy matcher.
- Restructure internal code to use dynamic sizing widgets. (hbox, vbox and lists)
- Async mode for dmenu.
- Add theme selector script.
- Include 21 themes.
- Dynamically sizing window with content.
- When placed at bottom of screen re-order screen to have entry at bottom.

### Improvements

- Fix pasting secondary clipboard. (#516)
- By default use all cores to filter entries.
- Make sure drawing priority is higher then reading input data.
- Improve resizing of window, don't make X whipe background.
- Improve close window (shift-delete) action, by sending NET_WM_CLOSE instead of destroying window.
- Create cache and run directory on startup. (#497)
- Fix unneeded redraws on cursor blinking. (#491)
- Improve time till grabbing keyboard. (#494)
- Better error message when failing to parse keybindings, also continue parsing on error.
- Fix problem with custom layouts (#485)
- Speedup drawing of screen. Works well now for 4k and 8k screens. (factor 1000+ speedup for the flipping of buffer)
  (#496)
- DRun mode more compatible with specification.
- Debug output via g_log.
- Fix password entry cursor position.
- Use bash instead of sh for get_git_rev.sh (#445)
- Add Control+G for cancel (#452)
- Add padding option to textbox (#449)
- Ctrl-click does alternate accept entry. (#429)
- Hide window decoration in normal window mode.
- Click to exit option. (#460)
- Fix cursor blinking on moving. (#462)
- Remove entry from history if fails to execute (#466)
- Fix margins. (#467)
- Improved documentation of functions in code.
- DRun: Set work directory when executing file. (#482)
- Memory leak fixes.
- Improve scrollbar behaviour.

### Removals

- opacity option. The transparency support in the theme can do the same and more.
