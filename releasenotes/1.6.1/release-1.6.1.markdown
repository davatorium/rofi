# 1.6.1: Tortoise Power

A bugfix release with a few minor new features.

## Theme: min/max and nested media support

To make themes more adoptable between different setups, `@media` statements can now be nested.

Support for min/max operation `calc()` has been added.

## FileBrowser

The file-browser plugin is now integrated in rofi.

![File Browser](filebrowser.png)


## ChangeLog
   - Use GdkPixbuf for Icon parsing.
   - Add FileBrowser to default mode.
   - Fix parsing dicts in config file (with " in middle of string.)
   - Add -normalize-match option, that tries to o match ö, é match e. (#1119)
   - [Theme] Add min/max operator support to calc() (#1172)
   - Show error message and return to list instead of closing (#1187)
   - [Theme] Add nested media support. (#1189)
   - [Textbox] Try to fix estimated font height. (#1190)
   - [DRun] Fix broken caching mechanism.
