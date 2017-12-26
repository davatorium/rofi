# V1.5.0: The Hoff uses it.

After the large previous release, we are hopefully back to more regular smaller releases.
This release is focused on squashing some bugs and hopefully improving the user experience.
But to satisfy people hunkering for new things, we also included a few new features.

## New features

### Specify matching field

What field rofi should match on the drun view has been a source of long discussions. Some people only want to match the
name, others want to include the field with the tooltip text. In this release you can now set what fields are used for
matching for both the drun as window browser.

### Pass extra properties in script mode

As a first step in improving script mode, support for passing properties where added.
You can now, from the script, set the `prompt`, a `message`, if the row contain `markup` and `active`, `urgent`.


### Negated matching

The matching engine has been extended so, part of, queries can be negated. Searching for `terminal -vim` will list all
fields that match `terminal` but do not contain `vim`.

### Hashtag rofi?

We have updated the theme format so that the '#' prefix before the element name is now optional.
Beside being unneeded, it made the multi-selector look weird.

```css
entry,prompt {
    background-color: DarkRed;
    text-color:       White;
}
```

## Backward incompatible changes

### Mouse bindings

Mouse button and scroll bindings are now separated, and names have changed.

For the 3 base buttons:

- `Mouse1` is now `MousePrimary`
- `Mouse2` is now `MouseMiddle`
- `Mouse3` is `MouseSecondary`

For the scroll wheel:

- `Mouse4` is `ScrollUp`
- `Mouse5` is `ScrollDown`
- `Mouse6` is `ScrollLeft`
- `Mouse7` is `ScrollRight`

For extra buttons:

- `Mouse8` is `MouseBack`
- `Mouse9` is `MouseForward`
- Above 10, you have to use the platform-specific `MouseExtra<number>` (replace `<number>`). Under X11, these buttons will go on from 10.

## Bug fixes

### Prompt colon

This is a controversial one, being the cause of heated issues in the past. The prompt string of rofi is now left
unmodified. Themes, like the default theme, can re-add the colon if desired. 

### History size

On frequent request, you can now tweak the size of the history each modi keeps. While not recommended to change as it
can cause performance issues, this allows power users to tweak it to their liking.


## Full Changelog

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
