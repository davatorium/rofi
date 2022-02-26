# 1.7.0: Iggy 2024

![Iggy](./iggy.jpg)

Rofi 1.7.0 is another bug-fix release that also removes a lot of deprecated features.   One of the biggest changes is
the removal of the (deprecated) xresources based configuration setup. With this removal, also a lot of hack-ish code has
been removed that tried to patch old config setups.  While the deprecation might be frustrating for people who have not
yet converted to the new format, I hope for some understanding. Even though this move might not be popular, the
deprecation in previous releases and consequential removal of these options is needed for two reasons.
The most important one is to keep rofi maintainable and secondary to open possibility to overhaul the config system in
the future and with that fixing some long standing bugs and add new options that
where hindered by the almost 10 year old system, the new system has been around for more than 4 years.

Beside mostly bug-fixes and removal of deprecated options, we also improved the theming and added features to help in
some of the more 'off-script' use of rofi.

This release was made possible by many contributors, see below for a full list. Big thanks again to SardemFF7 and
TonCherAmi.


## Default theme loading

In older version of **rofi** the default theme was (almost) always loaded based on some unclear rules, sometimes 
some random patch code was loaded and sometimes no theme was loaded before loading another theme.

The current version of rofi this is hopefully more logic.  It loads the default
theme by default using the default configuration. (Can be disabled by `-no-default-config`).
Using `-theme`, or `@theme` primitive will discard the theme completely.

So the below css completely removes the default theme, and loads `iggy`.

```css
configuration {


}

@theme "iggy"

element {
    children: [element-icon, element-text];
}
```

## File Browser

TonCherAmi made several very nice usability improvements to the file-browser. His changes allow you to define sorting
and ordering of the entries and changing the default start directory.

These options can be set using the new 'nested' configuration format that we are testing in rofi:

```css
configuration {
   filebrowser {
      /** Directory the file browser starts in. */
      directory: "/some/directory";
      /**
        * Sorting method. Can be set to:
        *   - "name"
        *   - "mtime" (modification time)
        *   - "atime" (access time)
        *   - "ctime" (change time)
        */
      sorting-method: "name";
      /** Group directories before files. */
      directories-first: true;
   }
}
```


## File Completion

In rofi 1.7.0 a long awaited patch I wrote many years ago landed into the rofi.
This patch adds some basic completion support by chaining modi. Currently it
only supports chaining the FileBrowser mode. This allows you to launch an
application with a file as argument.  This is currently supported in the Run
and the DRun modi by pressing the `Control-l` keybinding.  For the Run mode it
will add it as first argument, in DRun it only works if the Desktop file
indicates support for this.

![completer](./complete.gif)

This is not the final implementation, but a first investigation in how to
add/extend this feature. Ideally you can have multiple completers (including
custom ones) you can choose from.


## Timeout actions

You can now configure an action to be taken when rofi has not been interacted
with for a certain amount of seconds. You can specify a keybinding to trigger
after X seconds.

This option can be set using the new 'nested' configuration format that we are
testing in rofi:

```css
configuration {
  timeout {
      delay:  15;
      action: "kb-cancel";
  }
}
```

This setting will close rofi after 15 seconds of no interaction.

```css
configuration {
  timeout {
      delay:  5;
      action: "kb-accept";
  }
}
```
This setting will accept the current selection after 5 seconds of no
interaction.

## Background image and gradients

To improve theming the `background-image` property was added with support for
setting images `url()` or a gradient `linear-gradient()`.

```css
window {
    background-image: url("/tmp/i3.png", both);
}
element {
    children: [element-icon, element-text];
    background-image: linear-gradient(to bottom, black/20%, white/20%, black/10%);
}
```

The below screenshot shows both background image and gradients.

![background image](./background-image.png)

Or a more subtle change is the gradient on the tabs here:

![iggy-theme](./iggy-theme.png)


## Clickable button and icons

```css
icon-paste {
    expand: false;
    filename: "gtk-paste";
    size: 24;
    vertical-align: 0.5;
    action: "kb-primary-paste";
}
```

```css
button-paste {
    expand: false;
    content: "My Clickable Message";
    vertical-align: 0.5;
    action: "kb-primary-paste";
}
```

The screenshot below shows a non-squared image and clickable buttons (the close icon in the top right)

![rofi icons](./rofi-icons.png)


# Changelog

* ADD: -steal-focus option.

Explicitly steal focus from from the current window and restore it on closing.
Enabling this might break the window switching mode.

* ADD: [Config] Add nested configuration option support.

Allow for nested configuration options, this allows for options to be grouped.

```css
configuration {
  timeout {
      delay:  15;
      action: "kb-cancel";
  }
  combi {
    display-name: "Combi";
  }
}

```


* ADD: [Config] Support for handling dynamic config options.

A quick work-around for handling old-style dynamic options. This should be resolved when all options are
converted to the new (internal) config system.

* ADD: [DRun] Add fallback icon option.
This option allows you to set a fallback icon from applications.
```css
configuration {
  application-fallback-icon: "my-icon";
}
```

* ADD: [IconFetcher] Find images shipped with the theme.

If you have an icon widget you can specify an image that exists in the theme directory.
```css

window {
  background-image: url("iggy.jpg", width);
}
```

* ADD: [DRun] Add support for passing file (using file-browser) completer for desktop files that support his.

See above.

* ADD: [DRun] Support for service files.

Support KDE service desktop files.

* ADD: [FileBrowser] Allow setting startup directory (#1325)
* ADD: [FileBrowser]: Add sorting-method. (#1340)
* ADD: [FileBrowser] Add option to group directories ahead of files. (#1352)

See above.

* ADD: [Filtering] Add prefix matching method. (#1237)

This matching method matches each entered word to start of words in the target
entry.

* ADD: [Icon] Add option to square the widget.

By default all icons are squared, this can now be disabled. The icon will
occupy the actual space the image occupies.

* ADD: [Icon|Button] Make action available on icon, button and keybinding name.

See above.

* ADD: [KeyBinding] Add Ctrl-Shift-Enter option. (#874)

This combines the custom and alt keybinding. Allowing a custom command to be
launched in terminal.

* ADD: [ListView]-hover-select option. (#1234)

Automatically select the entry under the mouse cursor.

* ADD: [Run] Add support for passing file (using file-browser) completer.

See above.

* ADD: [Textbox] Allow theme to force markup on text widget.

Force markup on text widgets.

* ADD: [Theme] theme validation option. (`-rasi-validate`)
* ADD: [View] Add support for user timeout and keybinding action.
* ADD: [Widget] Add cursor property (#1313)

Add support for setting the mouse cursor on widgets.
For example the entry cursor on the textbox, or click hand cursor on the entry.

```css
element,element-text,element-icon, button {
    cursor: pointer;
}

```

* ADD: [Widget] Add scaling option to background-image.

Allows you to scale the `background-image` on width, height and both.
See above example.

* ADD: [Widget] Add support background-image and lineair gradient option.

See above.

* ADD: [Window] Add pango markup for window format (#1288)

Allows you to use pango-markup in the window format option.


* ADD: [Window] Allow rofi to stay open after closing window.

```css
configuration {
  window {
      close-on-delete: false;
  }
}

```

* FIX: [DSL] Move theme reset into grammar parser from lexer.

Given how the lexer and the grammar parser interact, the reset did not happen at
the right point in the parsing process, causing unexpected behaviour.

* FIX: [Drun]: fix sorting on broken desktop files. (thanks to nick87720z)

Broken desktop files could cause a rofi crash.

* FIX: [File Browser]: Fix escaping of paths.

Fix opening files with special characters that needs to be escaped.

* FIX: [ListView] Fix wrong subwidget name.

Fixes theming of `element-index`.

* FIX: [Script] Don't enable custom keybindings by default.

The quick switch between modi was broken when on a script mode. This now by default works,
unless the mode overrides this.

* FIX: [TextBox] Fix height estimation.

This should fix themes that mix differently sized fonts.

* FIX: [Theme] widget state and inherited properties. This should help fixing some old themes with changes from 1.6.1.

An old pre-1.6.1 rasi theme should work with the following section added:

```css
element-text {
    background-color: inherit;
    text-color:       inherit;
}
```

* FIX: [Widget] Fix rendering of border and dashes. (Thanks to nick87720z)

This fixes the long broken feature of dashed borders.

```css
message {
    padding:      1px ;
    border-color: var(separatorcolor);
    border:       2px dash 0px 0px ;
}

```

* FIX: [Build] Fix CI.

* FIX: [Theme] Discard old theme, when explicitly passing one on command line.

In previous version there was a bug when passing `-theme` on commandline did not discard old theme.
This caused problems loading themes (as it merged two themes instead of loading them).

To get old behaviour on commandline do:

```bash

rofi -theme-str '@import "mytheme"' -show drun
````

* REMOVE: -dump-xresources
* REMOVE: -fullscreen
* REMOVE: -show-match
* REMOVE: Old xresources based configuration file.
* REMOVE: fake transparency/background option, part of theme now.
* REMOVE: xresources parsing via Xserver
* Remove: [Theme] Remove backwards compatiblity hack.
* DOC: Update changes to manpages


# Thanks

Big thanks to:

* Morgane Glidic
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

Apologies if I mistyped or missed anybody.
