# Convert old themes to the new format

> This is now invalid, old theme support has been removed and will be part of a separate tool.

**Rofi** 1.4 can still read in and convert the old theme format. To read the old format, convert it, and dump it into a
new file run:

```bash
rofi -config {old theme} -dump-theme > new_theme.rasi
```

This gives a very basic theme, with the colors from the old theme.
You can preview the theme using:

```bash
rofi -theme new_theme.rasi -show run
```


