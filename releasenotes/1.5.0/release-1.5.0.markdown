# V1.5.0: The Hoff uses it.

After the last release turned out to be fairly large we are hopefully back to more regular, smaller releases.
This release focuses on squashing some bugs and hopefully improving the user experience.
Nevertheless this release also includes some new features.

Big thanks to [SardemFF7](https://www.sardemff7.net/), without whose help and contributions this release would not have been possible.


## New features

### Specify matching field

What field rofi should match on the drun view has been a source of [long discussions](https://github.com/DaveDavenport/rofi/pull/690),
which resulted in new options to specify, which fields rofi should match against in either drun or window mode.

`-drun-match-fields` *field1*,*field2*,...

When using drun, match only with the specified Desktop entry fields.
The different fields are:

* **name**: the application's name
* **generic**: the application's generic name
* **exec**: the application's executable
* **categories**: the application's categories
* **comment**: the application comment
* **all**: all of the above

    Default: *name,generic,exec,categories*

`-window-match-fields` *field1*,*field2*,...

When using window mode, match only with the specified fields.
The different fields are:

* **title**: window's title
* **class**: window's class
* **role**: window's role
* **name**: window's name
* **desktop**: window's current desktop
* **all**: all of the above

Default: *all*

### Pass extra properties in script mode

To further improve script mode, support for passing properties has been added.
You can now set the `prompt`, a `message` or use `markup` and `active`/`urgent` colors from the script itself.

E.g. to set the prompt from a bash mode script:

```bash
echo -en "\x00prompt\x1ftesting\n"
```

Or to mark the first 4 rows urgent and add a message:
```bash
echo -en "\x00urgent\x1f0-3\n"
echo -en "\0message\x1fSpecial <b>bold</b> message\n"
```

The `urgent` and `active` syntax is identical to the dmenu command-line argument.

### Negated matching

The matching engine has been extended. It’s now possible to negate parts of the query. Searching for `deconz -sh` will list all
fields that match `deconz` but do not contain `sh`.

![match](rofi-match.png)

![match negated](rofi-neg-match.png)

### Hashtag rofi?

In themes the '#' prefix before the element name is now optional.
As well as not being needed, it made the multi-selector look weird.

Example:

```css
entry,prompt {
    background-color: DarkRed;
    text-color:       White;
}
```

## Backward incompatible changes

### Mouse bindings

Mouse button and scroll bindings are now separated and naming has changed.

For the 3 base buttons:

- `Mouse1` is now `MousePrimary`
- `Mouse2` is now `MouseMiddle`
- `Mouse3` is now `MouseSecondary`

For the scroll wheel:

- `Mouse4` is now `ScrollUp`
- `Mouse5` is now `ScrollDown`
- `Mouse6` is now `ScrollLeft`
- `Mouse7` is now `ScrollRight`

For extra buttons:

- `Mouse8` is now `MouseBack`
- `Mouse9` is now `MouseForward`
- Above 10, you have to use the platform-specific `MouseExtra<number>` (replace `<number>`).

## Bug fixes

### Prompt colon

This is a controversial one, abeing the cause of heated [discussions](https://github.com/DaveDavenport/rofi/issues/637) in the past.
The prompt string of rofi is now left unmodified. Themes, like the default theme, can re-add the colon if desired.

```css
inputbar {
    children:   [ prompt,textbox-prompt-colon,entry,case-indicator ];
}
textbox-prompt-colon {
    expand:     false;
    str:        ":";
    margin:     0px 0.3em 0em 0em ;
}
```

Results in:

![rofi colon](rofi-colon.png)

### History size

By frequent request, you can now tweak the size of the history each modi keeps. While not recommended to change it as it
can cause performance issues, this allows power users to tweak it to their liking.

```
rofi.max-history-size: 500
```

## Full Changelog
 - [Theme] Accept integer notation for double properties. (#752)
 - [View] Theme textboxes are vertically sized and horizontal wrapped. (#754)
 - Rofi 1.4.2 doesn't capture ←, ↑, →, ↓ binding to keys to work in combination with Mode_switch (#744)
 - Add konsole to list of sensible terminals. (#739)
 - Allow drun to filter based on comment field. (#733)
 - Add prompt widget to default theme.
 - Add manpage for rofi-theme-selector.
 - Dump theme without # prefix and separator .
 - Fix issue with xnomad and -4 placing. (#683)
 - DRun obey OnlyShowIn and NotShowIn properties.
 - Store default theme in rofi binary using GResources.
 - Add extra margin between prompt and entry.
 - Remove colon from prompt. (#637)
 - Add support for passing extra properties in script mode.
 - Better error message on invalid argb syntax.
 - Fix default theme border.
 - Make '#' in the parser optional.
 - Update themes.
 - Add -drun/window-match-fields option (thx to Askrenteam) for drun/window menu. (#690/#656)
 - Implement negated match. (#665)
 - Fix parsing of non-existing fields. (#700)
 - rofi-theme-selector fixes.
 - Fix spelling error (thx to jmkjaer)
 - Fix test on i686/arm. (#692)
 - Fix error in theme manpage. (#686)
 - Allow history size to be specified. (#613)
 - Fix drun history implementation. (#579)
 - Add gentoo install instruction. (#685)
