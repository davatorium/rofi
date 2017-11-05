# V1.5.0: P rofi table. 

After the large previous release, we are hopefully back to more regular smaller releases.
This release is focused on squashing some bugs and hopefully improving the user experience.
But to satify people hunkering for new things, we also included a few new features.

## New features

### Specify matching field

### Pass extra properties in script mode

### Negated matching

## Bug fixes

### Hashtag rofi?

We have updated the theme format so that the '#' prefix before the element name is now optional.
Beside being unneeded, it made the multi-selector look weird.

```css
entry,prompt {
    background-color: DarkRed;
    text-color:       White;
}
```

### Prompt colon

### History size



## Full Changelog

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
