# rofi-thumbnails(5)

## NAME

**rofi-thumbnails** - Rofi thumbnails system

## DESCRIPTION

**rofi** is now able to show thumbnails for all file types where an XDG compatible thumbnailer is present in the system.

This is done by default in filebrowser and recursivebrowser mode, if **rofi** is launched with the `-show-icons` argument.

In a custom user script or dmenu mode, it is possible to produce entry icons using XDG thumbnailers by adding the prefix `thumbnail://` to the filename
specified after `\0icon\x1f`, for example:

```bash
echo -en "EntryName\0icon\x1fthumbnail://path/to/file\n" | rofi -dmenu -show-icons
```

### XDG thumbnailers

XDG thumbnailers are files with a ".thumbnailer" suffix and a structure similar to ".desktop" files for launching applications. They are placed in `/usr/share/thumbnailers/` or `$HOME/.local/share/thumbnailers/`, and contain a list of mimetypes, for which is possible to produce the thumbnail image, and a string with the command to create said image. The example below shows the content of `librsvg.thumbnailer`, a thumbnailer for svg files using librsvg:

```
[Thumbnailer Entry]
TryExec=/usr/bin/gdk-pixbuf-thumbnailer
Exec=/usr/bin/gdk-pixbuf-thumbnailer -s %s %u %o
MimeType=image/svg+xml;image/svg+xml-compressed;
```

The images produced are named as the md5sum of the input files and placed, depending on their size, in the XDG thumbnails directories: `$HOME/.cache/thumbnails/{normal,large,x-large,xx-large}`. They are then loaded by **rofi** as entry icons and can also be used by file managers like Thunar, Caja or KDE Dolphin to show their thumbnails. Additionally, if a thumbnail for a file is found in the thumbnails directories (produced previously by **rofi** or a file manager), **rofi** will load it instead of calling the thumbnailer.

If a suitable thumbnailer for a given file is not found, **rofi** will try to use the corresponding mimetype icon from the icon theme. 

### Custom command to create thumbnails

It is possible to use a custom command to generate thumbnails for generic entry names, for example a script that downloads an icon given its url or selects different icons depending on the input. This can be done providing the `-preview-cmd` argument followed by a string with the command to execute, with the following syntax:

```
rofi ... -preview-cmd 'path/to/script_or_cmd "{input}" "{output}" "{size}"'
```

**rofi** will call the script or command substituting `{input}` with the input entry icon name (the string after `\0icon\x1fthumbnail://`), `{output}` with the output filename of the thumbnail and `{size}` with the requested thumbnail size. The script or command is responsible of producing a thumbnail image (if possible respecting the requested size) and saving it in the given `{output}` filename.

### Issues with AppArmor

In Linux distributions using AppArmor (such as Ubuntu and Debian), the default rules shipped can cause issues with thumbnails generation. If that is the case, AppArmor can be disabled by issuing the following commands

```
sudo systemctl stop apparmor
sudo systemctl disable apparmor
```

In alternative, the following apparmor profile con be placed in a file named /etc/apparmor.d/usr.bin.rofi

```
#vim:syntax=apparmor
# AppArmor policy for rofi

#include <tunables/global>

/usr/bin/rofi {
    #include <abstractions/base>

    # TCP/UDP network access for NFS
    network inet  stream,
    network inet6 stream,
    network inet  dgram,
    network inet6 dgram,

    /usr/bin/rofi mr,

    @{HOME}/ r,
    @{HOME}/** rw,
    owner @{HOME}/.cache/thumbnails/** rw,
}
```

then run

```
apparmor_parser  -r /etc/apparmor.d/usr.bin.rofi
```

to reload the rule. This assumes that **rofi** binary is in /usr/bin, that is the case of a standard package installation.
