# 1.7.7: Time Dilation

A quick-fix release following reports after releasing the 1.7.6 release.

## Issues

### Drawing issue

After the previous release some users experienced rofi being rendered to screen
as a black box. The issue seems to be only hit on certain video cards/drivers
see the github issue [#2068](https://github.com/davatorium/rofi/issues/2068).It turned out on these systems using
`cairo_push/pop_group` results in a black screen when the backing surface is
using the XCB library. When using other drivers, or rendering to a cairo image
surface.
Hopefully reverting to `cairo_store` and `cairo_restore` fixes this issue for everybody.

Issue [#2068](https://github.com/davatorium/rofi/issues/2068)

### Window mode missing some windows

Because of a wrong check some windows where misidentified as 'skip pager' and
hidden from the view. This should now be resolved.

Issue [#2071](https://github.com/davatorium/rofi/issues/2071)

### 'Character' in config file broken

A previous patch to make using strings more easy to use in the configuration
file broke settings that use the 'character' type as setting. Because not a lot
of options use the 'Character' type , this has been 'resolved' by using the
'String' type and picking the first ASCII character. 

Issue [#2070](https://github.com/davatorium/rofi/issues/2070)

## improvements

Beside these issues, also some improvements are included in this release. 

## Resolve 'rasinc' for @imports

When importing a theme file like this:

```css
@import "mytheme"
```

Rofi would find any file 'mytheme.rasi' in any of the theme paths.
This was missing the new extension for shared include files 'rasinc'.
This is now added when resolving.

Issue [#2069](https://github.com/davatorium/rofi/issues/2069)

## Desktop file DBus activation

Some desktop files did not launch correctly because it did not implement the
DBusActivation feature. While most desktop files did provide a fallback `Exec`
entry, this seems to not always be the case. Rofi should now first try
DBusActivation and fall back to the Exec if it does not succeed.

Issue [#1924](https://github.com/davatorium/rofi/issues/1924)

## Resolve `-config` identical to `-theme`

if you pass an alternative config file, this is now resolved using the same
logic as the `-theme` argument. Making it easier to have multiple, alternative,
configuration files.

Issue [#2040](https://github.com/davatorium/rofi/issues/2040)

## Changelog

* [Widget] Don't use cairo_push/pop_group as it causes issues. (#2068)
* Revert "[window] Check bitmask, not full comparison".  (#2071)
* [Config] Remove character data type as it aliases with string. (#2070)
* [Doc] Refer to releasenotes for updates in Changelog file. (#2069)
* [Doc] Update theme documentation with import resolving update. (#2069)
* [Themes] Update themes to import without rasi(nc) extensions. (#2069)
* [Theme] Fix resolving of 'rasinc' extension when no extension is given. (#2069)
* Be more diligent trying to resolve -config. (#2040)
* Use FALSE instead of FALSE.
* Resolve -config argument identical to a -theme argument. (#2040)
* [DRun] If indicated by .desktop file, launch via dbus activation. (#1924)
* [Website] Update website links and headers.
