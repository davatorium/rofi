# 1.7.0: Iggy 2024 

Rofi 1.7.0 is another bug-fix release that also removes a lot of deprecated features.
This has been needed to keep **rofi** maintainable.


## FileBrowser

## Clickable button and icons

## File Completion

## Timeout actions

## Background image and gradients


# Changelog

* ADD: -steal-focus option.
* ADD: [Config] Add nested configuration option support.
* ADD: [Config] Support for handling dynamic config files.
* ADD: [DRun] Add fallback icon option.
* ADD: [DRun] Add support for passing file (using file-browser) completer for desktop files that support his.
* ADD: [DRun] Support for service files.
* ADD: [File Browser] Allow setting startup directory (#1325)
* ADD: [File Browser]: Add sorting-method. (#1340)
* ADD: [FileBrowser] Add option to group directories ahead of files. (#1352)
* ADD: [Filtering] Add prefix matching method. (#1237)
* ADD: [Icon] Add option to square the widget. 
* ADD: [Icon|Button] Make action available on icon, button and keybinding name. 
* ADD: [KeyBinding] Add Ctrl-Shift-Enter option. (#874)
* ADD: [ListView]-hover-select option. (#1234)
* ADD: [Run] Add support for passing file (using file-browser) completer.
* ADD: [Textbox] Allow theme to force markup on text widget.
* ADD: [Theme] Remove backwards compatiblity hack. 
* ADD: [Theme] theme validation option. (`-rasi-validate`) 
* ADD: [View] Add support for user timeout and keybinding action.
* ADD: [Widget] Add cursor property (#1313)
* ADD: [Widget] Add scaling option to background-image.
* ADD: [Widget] Add support background-image and lineair gradient option.
* ADD: [Window] Add pango markup for window format (#1288)
* FIX: [DSL] Move theme reset into grammar parser from lexer.
* FIX: [Drun]: fix sorting on broken desktop files. (thanks to nick87720z) 
* FIX: [File Browser]: Fix escaping of paths.
* FIX: [ListView] Fix wrong subwidget name.
* FIX: [Script] Don't enable custom keybindings by default.
* FIX: [TextBox] Fix height estimation.
* FIX: [Theme] widget state and inherited properties. This should help fixing some old themes with changes from 1.6.1.
* FIX: [Widget] Fix rendering of border and dashes. (Thanks to nick87720z)
* REMOVE: -dump-xresources
* REMOVE: -fullscreen
* REMOVE: -show-match
* REMOVE: Old xresources based configuration file.
* REMOVE: fake transparency/background option, part of theme now.
* REMOVE: xresources parsing via Xserver
* DOC: Update changes to manpages


# Thanks

Big thanks to:

* Quentin Glidic
* a1346054
* Ian C
* TonCherAmi
* nick87720z
* Markus Grab
* Zachary Freed
* nickofolas
* unisgn
* Jas
* rahulaggarwal965
* Awal Garg
* Eduard Lucena
* Lars Wendler

Apologis if I mistyped or missed anybody.
