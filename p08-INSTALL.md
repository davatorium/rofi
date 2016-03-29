---
layout: default
github: DaveDavenport
title: Installation guide
---


## DEPENDENCY

### For building:

* C compiler that supports the c99 standard. (gcc or clang)
* make
* autoconf
* automake (1.11.3 or up)
* pkg-config
* Developer packages of the external libraries

### External libraries

* libxinerama
* libxft2
* libpango
* libpangocairo
* libcairo
* libcairo-xlib
* libglib2.0
* libx11
* libstartup-notification-1.0

On debian based systems, the developer packages are in the form of: `<package>-dev` on rpm based
'<package>-devel'.


### Optional:

* For i3 support, you need at least i3 version 4.5 or up.
Make sure that 'i3/ipc.h' is included. If it fails please check 
config.log.



## Install from a release

Check dependencies and configure build system:

```
./configure
```

Build Rofi:

```
make
```

The actual install, execute as root (if needed):

```
make install
```


## Install a checkout from git

The GitHub Pages version of these directions may be out of date.  Please use
[INSTALL.md from the online repo][master-install] or your local repository.

[master-install]: https://github.com/DaveDavenport/rofi/blob/master/INSTALL.md#install-a-checkout-from-git

Generate build system:

```
autoreconf -i
```

Create a build directory:

```
mkdir build
```

Enter build directory:

```
cd build
```

Check dependencies and configure build system:

```
../configure
```

Build rofi:

```
make
```

The actual install, execute as root (if needed):

```
make install
```


## Options for configure

When you run the configure step there are several you can configure. (To see the full list type
`./configure --help` ).

The most useful one to set the installation prefix:

```
./configure --prefix=<installation path>
```

f.e.

```
./configure --prefix=/usr/
```

### Install locally

or to install locally:

```
./configure --prefix=${HOME}/.local/
```

###  I3 workaround

 If i3 is installed in a non-standard prefix, point it to the right location using:

```
CFLAGS="-I/weird/i3/path/include/" ../configure
CFLAGS="-I/weird/i3/path/include/" make
```

## Options for make

When you run make you can tweak the build process a little.

### Verbose output

Show the commands called:

```
make V=1
```

### Debug build

Compile with debug symbols and no optimization

```
make CFLAGS="-O0 -g3" clean rofi
```

### Get a backtrace

Getting a backtrace using GDB is not very handy. Because if rofi get stuck, it grabs keyboard and
mouse. So if it crashes in GDB you are stuck.
The best way to go is to enable core file. (ulimit -c unlimited in bash) then make rofi crash. You
can then load the core in GDB.

```
gdb rofi core
```
