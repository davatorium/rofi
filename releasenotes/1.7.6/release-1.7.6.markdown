# 1.7.6: Traveling Time

## Recursive file browser

An experimental file browser is introduced in this version.
This recursively scans through the users home directory (this is configurable) to find files.
It is designed to be asynchronous and very fast.

The following settings can be configured:

```css
configuration {
   recursivebrowser {
      /** Directory the file browser starts in. */
      directory: "/some/directory";
      /** return 1 on cancel. */
      cancel‐returns‐1: true;
      /** filter entries using regex */
      filter‐regex: "(.*cache.*|.*.o)";
      /** command */
      command: "xdg‐open";
   }
}
```


## Copy to clipboard support

Add support to copy current selected item to clipboard.
The added `control-c` binding copies the current selection to the clipboard.

**THIS ONLY WORKS WITH CLIPBOARD MANAGER!!!** Once rofi is closed, the data is
gone!

## Entry box history

You can now recall and move through previous queries by using
`kb-entry-history-up` or 'kb-entry-history-down` keys. (`Control-Up`,
`Control-Down`).

The following settings can be configured:

```css
configuration {
    entry  {
        max‐history: 30;
    }
}
```

## Fix calc

There was a non-parsable grammar in the 'calc' part of the language.
The % operator (modulo) overloaded with percent and could leave to statements
having multiple valid but contradicting interpretations.
To resolve this the modulo operator is now `modulo`.

Several smaller issues with the parser were also fixed in this patch.

## Text and cursor outline

Three new properties were added to textbox widgets to control text outline:

* `text-outline` boolean to enable outlines
* `text-outline-width` to control size
* `text-outline-color` to control color

![Outlines](./text-outline.png)

Outlines can also be added to cursors, via similarly named
properties (`cursor-outline-*`).

## Dependencies and packaging

In this version, we have bumped the minimal required versions of some
dependencies to keep up with bugs and deprecations while staying compatible
with long-term supported distributions.

* glib: 2.72 or newer
* meson: 0.59.0 or newer

This roughly corresponds to Ubuntu 22.04 Jammy and Debian Bookworm.

Generated man pages were removed from the git repository and now require
`pandoc` to build instead of `go-md2man`. If you compile from git, you
will now need to install `pandoc` to get the man pages.

Release tarballs still contain the files and can be installed without
additional tool.

## Other smaller changes

* new `.rasinc` extension for theme fragments included from other theme files
* `-ellipsize-mode` mode option in dmenu mode can be set to `start`, `middle` or `end`
  to control how long entries are ellipsized
* `-list-keybindings` to print a summary of configured bindings
* `-window-prefer-icon-theme` to force precedence of theme icons over applications'
  custom ones (#1702)
* `-placeholder-markup` to enable pango markup when rendering placeholder text (#1690)
* `urgent` and `active` flags can be controlled for each row in dmenu and script
  modes

## Website

The current documentation is now also available online at:
[https://davatorium.github.io/rofi/](https://davatorium.github.io/rofi/)

# Thanks to

Big thanks to everybody reporting issues.
Special thanks goes to:

  * a1346054
  * aloispichler
  * Amith Mohanan
  * Christian Friedow
  * cognitiond
  * David Kosorin
  * Dimitris Triantafyllidis
  * duarm
  * Fabian Winter
  * Gutyina Gergő
  * Jasper Lievisse Adriaanse
  * Jorge
  * Martin Weinelt
  * Morgane Glidic
  * Naïm Favier
  * Nikita Zlobin
  * nomoo
  * notuxic
  * Rasmus Steinke
  * Tim Pope
  * TonCherAmi
  * vE5li
  * Yuta Katayama
  * Danny Colin

Apologies if I mistyped or missed anybody.

# Changelog

* [View] Work around GThreadPool 1 pointer bug.
* Also fix typo in icon fetcher.
* [Github] Bump checkout to v4
* [Doc] Switch to pandoc and remove generated files (#1955)
* [Build] Add missing dist files from libnkutils
* [IconFetcher] Don't free on removal from thread-pool
* Add an item-free method to the thread-pool
* [Window] write code so clang-check does not complain about leak.
* [script|dmenu] Add option to make row permanent. (#1952)
* [run] fix missing doxygen and add explanation.
* [Run] When passing raw entry, pass it unquoted to history (#1951)
* Replace deprecated g_memdup by g_memdup2
* Bump glib version to 2.72
* [Build] Bump minimal meson version to 0.59.0 (#1947)
* Fix compiler warnings in window mode.
* Fix some compiler warnings.
* [RUN] shell escape command before processing it further.
* [DRun] Drun read url field from cache.
* [Build] Reduce amount of warnings (#1944)
* [View] Don't use xcb surface to render to png, but create surface.
* [Box] When estimating height, set correct width on children (#1943)
* [ThreadPool] Sort items in the queue based on priority
* [Doc] Fix broken ``` guards.
* [Doc] Remove reddit link from config.yml.
* [Doc] Clarify in build instructions what release to use.
* Add extra documentation issue template.
* Fix typo in dynamic_themes.md (#1941)
* [DOC] Add explanation to PATTERN of brackets (#1933)
* [Doc] Update manpage to clarify meta property.
* [View] On mode switch force refilter instead of queuing. (#1928)
* [View] Allow float delay (#1926)
* [View] Always forward motion to the grabbed widget first.
* [IconFetcher] If last step fails to load icon, don't error out make warning
* [Script] Update theme property clarification a bit.
* [Script] Add clarification to theme property.
* [Dmenu][Script] Add 'display' row option to override whats displayed.
* [DRun] Allow url field to be searched and fix c/p error (#1914)
* [DRUN] Add {url} to drun-display-format. (#1914)
* [lexer] Add dmenu as enabled option for media type. (#1903)
* [XCB] Make sure that window maker is 0 terminated before usage.
  Thanks to Omar Polo and bsdmp
* Fix text color when `cursor-color` is set (#1901)
* [XCB] Try to be smarter with where mouse click started. (#1896)
* [View|Textbox] cleanups to drawing code
* Clip text with extents rectangle
  Fonts are not ideal, some characters have mismatch between reported and
  painted size.
* [Rofi] Expand cache-dir (#1892)
* Fix typos in dmenu docs (#1891)
* Support single quotes for strings as in CSS
* [Theme] Fix missing doxygen documentation
* [Theme] Fix opening abs path if no/wrong extension (backward comp.)
* [rofi-theme] fix typo
* [Theme] Try to fix importing of theme.
    - Fix the two place of resolving into one method.
    - Do not accept file in CWD.
    - Prefer file if it exists in same directory as parent file that
      imported it.
    (#1889)
* script: Let script handle empty custom input
* widget_draw: clean useless calls in corner drawing
* Fix border segments stitch
* Fix mm type in description
* Remaining modi words in the code
* Better descriptions for sort options group
  It's unobvious from documentation, that sort only works against filtered menu.
* update man pages without scripts
* [Lexer] Print some more debug info on error. (#1865)
* [Script] Set type on Mode object.
* [window] Quick test of code scanning.
* [ROFI] -e '-' reads from stdin
* [ConfigParser] Don't pass commandline options with very long args.
    This is a quick 'n dirty fix for this unexpected issue.
    (#1855)
* [Build] Fix autotools build system.
* [Doc] Fix some missing/wrong doxygen headers.
* Print window manager in -help output
* Merging in the Recursive file browser.
* Add wezterm to rofi-sensible-terminal (#1838)
* [DRun] Add option to scan desktop directory for desktop files.
* [IconFetcher] Fix small memory leak.
* Small memory leaks fixed and other cleanups.
* [MKDocs] Add logo
* [DMenu] Fix row initial tab if non-first column is shown first. (#1834)
* [Doc] Update theme manpage with remark-lint hints.
* [Doc] More small markdown fixes.
* [DOC] Update rofi-script update with remark-lint remarks.
* Remove unneeded test and extra enforcement of 0 terminated buffer
* [Doc] Update rofi.1.markdown with markdown fixes.
* [DOC] update readme.md with remark-lint updates..
* [DOC] Update INSTALL,md with remark-lint fixes.
* [DOC] Add some remark markdown fixes.
* Fix to pointless or's.
* [UnitTest] Add more tests for environment parsing.
* [Doc] Mention location of scripts in manpage.
* Re-indent the code using clang-format
* Fix typo in template.
* Update issue template to include checkbox for version.
* [Doc] Re-generate manpage
* docs: element children theming (#1818)
* Add support for adding textbox widgets to listview elements (#1792)
* [Textbox] cairo_save/restore does not store path.
    Fix by moving cairo_move_to to after blink.
    Also fix drawing outline.
* More Unicode normalization with `-normalize-match` (#1813)
    Normalize the string to a fully decomposed form, then filter out mark/accent characters.
* #1802: Calc broken fix (#1803)
    * [Theme] First move to double internal calculations.
    * [Theme] Allow float numbers.
    * [Theme] Fix unary - and tighten the grammar parser.
    * [Theme] Rename % to modulo to fix compiler.
    * [Theme] Dump right modulo syntax.
    * [Test] add missing end_test
    * [Grammar] Allow negative numbers as property value
* [Dmenu] Small fix that disabled async mode when multi-select is enabled.
* [View] Fix wrong bitmask checking. (& not |)
* [rofi-theme-selector] prepend newline before specifying new theme (#1791)
    * [rofi-theme-selector] prepend newline before specifying new theme
      If the EOF is not a newline, new theme setting will fail.
    * make sed substitution more readable
    * simplify sed substitution
* [MKDocs] Try to fix link.
* [MKDocs] Add downloads to side menu
* [MKDocs] Add a download page.
* [Script] Strip pango markup when matching rows (#1795)
* [Doc] theme, spelling fix and more textual tweaks.
* [Doc] More tweaks to get the formulation right.
* [Doc] themes manpage, small textual improvement.
* [Doc] Try to fix some markdown, themes.
* [Doc] Try to clarify the children situation for the listview widget.
* [EntryHistory] Disable entry history when dmenu password is set.
* I785 (#1789)
    * [Textbox] Add history to the entrybox.
    * [Textbox] Add comments and move into sub functions.
    * [doc] Add conflicting constraint section to manpage.
    * [Script] Some small memory leak fixes.
    * [Entry History] Add documentation.
    (#785)
* [doc] Add conflicting constraint section to manpage.
* [mkdoc] add link to user scripts
* [Textbox] Replace 'space' with a space (#1784)
* draw text after cursor (#1777)
* [Doc] Small tweak to markdown.
* [Example] Small change in escaping for caday.
* [Doc] Add manpage documentation for pango font string.
* [MKDocs] Add dynamic theme guide.
* [FileBrowser] Allow command to be overwritten
* [theme] Small theme tweak.
* [Theme] Add NO_IMAGE mode to theme.
* [Themes] add fullscreen theme with preview part.
* [window] When no window title set, handle this more gracefully
* [DMenu|Script] Add per row urgent/active option.
    Instead of having a global list of entries to highlight urgent/active,
    you can now to it per row.
* sed & realpath workaround for BSD and Darwin OS
* [filebrowser] Add option to return 1 on cancel.  (#1732)
* [Theme] Small tweak to fancy2 theme
* [MKDocs] Link to rasi files in theme page.
* [Themes] Add fancy2 theme.
* [Themes] Add material theme
* Fix header theme
* [Helper] Quick fix for wrong dereference.
* MKDoc website (#1772)
    * Add initial documentation page using mkdocs
    * Test action
    * Add notes to mkdoc site.
    * Add installation guide
    * Add installation and config guide to mkdocs.
    * Add installation manual
    * Add image to main page
    * [mkdocs] Add plugin guide.
    * [mkdocs] Add plugin to main page and some small fixes.
    * Add shipped themes page
    * [actions] Also rebuild website on the next branch
* [themes] don't use screenshot transparency in shipped themes
* [IconFetcher] Fix for api change
* [Theme] support rasinc for theme include files.
* [listview] Don't calculate infinite rows on empty height. (#1769)
* [Theme] Move some definitions header around for plugin.
* [Textbox] Cursor goes over, not under. allow cursor outline.
* [Textbox] Add text-outline to style
* [Doc] Clarify documentation on `require-input` further.
* make cursor more customizable by adding cursor-width and cursor-color (#1753)
    * make cursor more customizable by adding cursor-width and cursor-color to the theme
    * fix placeholder color
    * add doc entry
    * more documentation
* [XIM] Fix an unitialized value problem.
* [Doc] Add example run command with cgroup support (#1752)
* [Build] Fix test building in makefile.
* [Doc] Add documentation for new functions.
* [Doc] Fix some missing docu.
* [DMenu] Add -ellipsize-mode option.
* [listview] Set ellipsize mode on creation of textbox
  So if rows are added, they behave correctily. (#1746)
* Disable imdkit by default
* Build documentation (#1744)
    * explain how to pass options to meson
    * fix typo in INSTALL.md
* [Build] Use built-in lto option.
* [Window] Fix reloading windowcd from xserver request
* [Build]  Add option to build with lto to meson.
    Fix error in test.
    (#1743)
* [Build] Add option to disable imdkit at compile time. (#1742)
* input method (#1735)
    * input method draft
    * restoring relese event
    * using unused macro, removing debug code, handling disconnection
    * review fixes, new update_im_window_pos method
    * initializing variables correctly
    * initializing im pos queue correctly
    * ime window positioning
    * add widget_get_y_pos() position
    * [Build] Update makefile with imdkit
    * [CI] Add imdkit as dependency.
    * [XCB] rofi_view_paste don't throw warning, print debug.
    * [XCB] rofi_view_paste lower 'failed to convert selection'
    * [Build] Add minimum version check to imdkit
    * new macro XCB_IMDKIT_1_0_3_LOWER
    * [Build] Try to support old version of imdkit in meson/makefile.
    * [Build] Fix typo in meson.build
    * [XIM] Don't set use compound/set use utf8 when on old version.
    * [Build] Allow building without imdkit.
    * [Doc] Add imdkit to dependency list.
* [Window] Make sure their is a trailing 0 on the workspace strings. (#1739)
* [FileBrowser] Bind kb-delete-entry to toggle show-hidden.
* [Textbox] Add a 'get_cursor_x_pos' function.
* [man] re-gen manpage.
* [DOC] Add parsing row options to dmenu manpage (#1734)
* [Build] Fix icon install path for makefile. (#1733)
* [Window] Small fixes to prefer-icon-theme option
  Thanks to Kafva (https://github.com/Kafva) for the original patch.
  (#1702)
* [Window] Add -window-prefer-icon-theme option. (#1702)
* [drun] Only pass path on launch, if one is set
* The mode is filebrowser (not file-browser) (#1723)
* [filebrowser] Add an option to show hidden files. (#1716)
* [Doc] Update rofi-keys manpage with unset section
* Add format option to disable padding with space the "window-format" entries (#1715)
* [Script] Split mode string only once on :, allowing : in right part. (#1718)
* [window] Check bitmask, not full comparison
* Use `command -v` instead of `which` (#1704)
* [Keyb] Add a -list-keybindings command.
* Fix sed binary call with variable (#1707)
* [listview] Add extra checks before resolving pointer. (#1703)
* [Textbox] Add 'placeholder-markup' flag. (#1690)
* [Test][Theme] Update test for downgrade error
* [Theme] If no theme loaded, load default. Downgrade missing theme file to warning. (#1689)
* [DMenu] reset variable correctly so keep-selection is initially off. (#1686)
* Update test for # keybindings.
* [View|Xcb] Add support to copy current selected item to clipboard (#378)
* Include sys/stat.h for S_IWUSR (#1685)
* [View] Tweak error message and instant/delayed switching.
* [View] Change refilter timeout limit to be in time units (ms) (#1683)
* [Combi] Fix possible memory leak.
* [combi] Fix selecting entry with only bang as input.
* [View] Increase default refilter-timeout-limit. (#1683)
