# Release 0.15.12

The 0.15.12 release of **rofi** focussed on improving overall user experience.  These improvements mostly focussed on
three things, first we (tried to) fix the problems with complex keyboard layouts, secondly we tried to make themeing of
**rofi** easier and last we added several speedups.  Below I will highlight these bigger changes in more details

From the next release on, now that **rofi** reached an acceptable maturity level, we will start using more common
version numbering (instead of `0.year.month` that is currently used). Depending on the amount of new bugs discovered I
might decide to make a `1.0.0` release.

## Keyboard Layouts

**Rofi** used to have problems with keyboard layouts that used modifier keys to switch between different layouts (unsure
how this is called). These problems should now belong to the past.

**Note** the syntax for binding keys has slightly changed. The *Mod1*, *Mod2*, etc. keywords are no longer available.
There was no good way to detect how these keys where mapped and if they could be used as modifiers. E.g. if the right
alt (say *Mod3*) is configured to switch between layouts, it cannot work as modifier key to make a `Mod3-p` keybinding.
**Rofi** will now check if the current layout has the *SuperR*,*SuperL*,*AltGr*,*HyperL*,*HyperR* keys available. If they
are available they can be used for keybindings, if not, the user gets a warning.

![Rofi Keyboard Warning](rofi-warning.png)

## Speedups

### DMenu reading from stdin

**Rofi** used to have a custom `fgets` implementation that supported custom separators. The first version in here was
slow and was improved. After a hint of it existence it has now been replaced by the Posix 2008 `getdelim`, this one is
almost as fast as the new custom implementation.

Overall this gave a speedup of 6x.

*check speedup against previous release*

### Multi-Core power

Still disabled by default, **rofi** can now spawn multiple threads for filtering rows. Depending on the underlying
hardware we saw a 1.5 times speedup running on a dual core ARM  and up to a 3.5 speedup on a quadcore (8 thread) i7.
It uses Glib's GThreadPool and will therefor spawn threads when needed, and clean them up again when unused.

To enable this option pass `-threads 0` option, this will autodetect the number number of hw-threads. Pass `-threads 4`
to force it to use 4 threads. Setting the number to 1, disables it.


## Themes

**Rofi** color theme can be specified in a lot of detail, using transparency to get the desired results. However this is
not always the easiest to write down and testing it can be a hassle. To solve this we added a web frontend for writing
(and live previewing) themes and a theme repository.

To help with this, you can now take screenshots of **rofi** from within **rofi** with a simple keybinding.

![Rofi Internal Screenshot](rofi-screenshot.png)
 
### Theme repository

This has been requested several times and started somewhat on the website. This however got outdated quickly and neither
the themes or the screenshots are correct anymore. To ease this the new repository allows themes to be easily added: you
can export your current configuration directly using **rofi --dump-xresources-theme**, place it in the theme directory
of the repository and the update script will automatically generate screenshots and update the page.

The repository can be found [here](https://github.com/DaveDavenport/rofi-themes/)

![Rofi Theme Site](rofi-theme-site.png)

### Themenator

The second tool is a website allowing you to easily create themes and preview all changes life, the [themenator]()
Big thanks goes to `SardemFF7` who got tired of me complaining, took the very rough prototype and turned it into
something beautiful. 

Hopefully people will make beautiful themes and submit them to the [theme
repository](https://github.com/DaveDavenport/rofi-themes/).

![Rofi Themenator](rofi-themenator.png)

## Full ChangeLog

### New features:
- Initial `-dump` command for dmenu mode. (#216)
- Threading support.
    - Parallel checking for ASCII.
    - Parallel string matching.
    - Autodetect number of HW-threads.
    - Disabled by default.
- Highlight multiple selected rows (#287,#293)
- Dmenu can read from file instead of stdin.
- Regex matching (#113)
- Take Screenshot of rofi using keybinding.
- Hotkey for sorting: (#298)
- Option to set scrollbar width.

### Improvements:

- Fix return code of multi-select.
- Update manpage (#289, #291)
- Improve speed of reading stdin in dmenu mode.
- Correctly handle modifier keys now. Should now support most weird keyboard layouts and switching between them.
(#268, #265, #286)
- Correctly set locale, fixing issues with entering special characters. (#282)
- DRun mode support `NoDisplay` setting (#283)
- Pango markup is matched when filtering. (#273)

### Bug fixes:

- Improve error message (#290)
- Correctly switch to next entry on multi-select when list is filtered (#292)
- Switch __FUNCTION__ for __func__. (#288)
- Fix segfault on empty list and moving through it. (#256,#275)
- Fix one off of colors (#269)
- Drun fix uninitialized memory (#285)
