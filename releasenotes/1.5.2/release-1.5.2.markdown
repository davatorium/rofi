# 1.5.2: Procrastination in progress

Rofi 1.5.2 is another bug-fix release in the 1.5 series.


## Fix border drawing

Issue: #792, #783

There turned out to be a bug in how borders are drawn. It would overlap parts of the border on corners, screwing up
transparency.

![broken border](border-issue.png)

This is now fixed.

## Improve Icon handling

Issue: #860

Several bugs around Icon handling have been fixed:

* Failing to load multiple (identical icons) on initial load.
* Preload user-set icon theme.
* Use the common threadpool in rofi for the icon fetching, instead of spawning a custom one.


## New sort syntax

Because of all the changes to the sorting methods in rofi, the command-line options for it where very confusing.
To fix this they have been changed.

The `sort` option is now used to enable/disable sorting. (This can also be changed at run-time using the hotkey)

The `sorting-method` allows you to set the sorting method. Currently it supports **normal** (levenshtein) and **fzf**.

## Documentation updates

Issue: #879, #867, #837, #831, #804

Thanks to all the people highlighting or providing fixes to the documentation.

## Improving the ssh known hosts file parser

Issue: #820

The original known hosts parser was very limited. The parser has been extended to a bit more robust.

## Additions

For some reason I can never make a release without adding more features to it. (Feature creep?).

### Option to change the negate character

Issue: #877

The option to negate a query: `foo -bar` e.g. search for all items matching `foo` but not `bar` caused a lot of
confusion. It seems people often use rofi to also add arguments to applications (that start with a -).

To help with this, the negate character (`-`) can be changed, or disabled.

To disable:

```
rofi -matching-negate-char '\0'
```


### Modify the DRUN display string

Issue: #858

An often requested feature is the ability to change the display string for the drun modi.
The `-drun-display-format` option is added that allows just this.

> -drun-display-format
>
> The format string for the drun dialog:
> * name: the application's name
> * generic: the application's generic name
> * exec: the application's executable
> * categories: the application's categories
> * comment: the application comment
>
> Default: {name} [({generic})]

Items between `[]` are only displayed when the field within is set. So in the above example, the `()` are omitted when
`{generic}` is not set.


### Theme format now supports environment variables

You can use environment variables as part of your theme/configuration file property value.
Environment variables start with `$` and the name is surrounded by `{}`.
So to query the environment `FOO` you can do:

```css
#window {
    background: ${FOO};
}
```

The environment is then parsed as a normal value.
