# Installation guide

This guide explains how to install rofi using its build system and how you can
make debug builds.

Rofi uses autotools (GNU Build system), for more information see
[here](https://www.gnu.org/software/automake/manual/html_node/Autotools-Introduction.html).
You can also use [Meson](https://mesonbuild.com/) as an alternative.

## DEPENDENCY

### For building

-   C compiler that supports the c99 standard. (gcc or clang)

-   make

-   autoconf

-   automake (1.11.3 or up)

-   pkg-config

-   flex 2.5.39 or higher

-   bison

-   check (Can be disabled using the `--disable-check` configure flag)
    check is used for build-time tests and does not affect functionality.

-   Developer packages of the external libraries

-   glib-compile-resources

### External libraries

-   libpango >= 1.50

-   libpangocairo

-   libcairo

-   libcairo-xcb

-   libglib2.0 >= 2.72
    - gmodule-2.0
    - gio-unix-2.0

-   libgdk-pixbuf-2.0

-   libstartup-notification-1.0

-   libxkbcommon >= 0.4.1

-   libxkbcommon-x11

-   libxcb (sometimes split, you need libxcb, libxcb-xkb and libxcb-randr
    libxcb-xinerama)

-   xcb-util

-   xcb-util-wm (sometimes split as libxcb-ewmh and libxcb-icccm)

-   xcb-util-cursor

-   xcb-imdkit  (optional, 1.0.3 or up preferred)

On debian based systems, the developer packages are in the form of:
`<package>-dev` on rpm based `<package>-devel`.

## Install from a release

When downloading from the github release page, make sure to grab the archive
`rofi-{version}.tar.[g|x]z`. The auto-attached files `source code (zip|tar.gz)`
by github do not contain a valid release. It misses a setup build system and
includes irrelevant files.

### Autotools

Create a build directory and enter it:

```bash
    mkdir build && cd build
```

Check dependencies and configure build system:

```bash
    ../configure
```

Build Rofi:

```bash
    make
```

The actual install, execute as root (if needed):

```bash
    make install
```

The default installation prefix is: `/usr/local/` use `./configure
--prefix={prefix}` to install into another location.

### Meson

Check dependencies and configure build system:

```bash
    meson setup build
```

Build Rofi:

```bash
    ninja -C build
```

The actual install, execute as root (if needed):

```bash
    ninja -C build install
```

The default installation prefix is: `/usr/local/` use `meson setup build
--prefix={prefix}` to install into another location.

## Install a checkout from git

The GitHub Pages version of these directions may be out of date.  Please use
[INSTALL.md from the online repo][master-install] or your local repository.

If you don't have a checkout:

```bash
    git clone --recursive https://github.com/DaveDavenport/rofi
    cd rofi/
```

If you already have a checkout:

```bash
    cd rofi/
    git pull
    git submodule update --init
```

For Autotools you have an extra step, to generate build system:

```bash
    autoreconf -i
```

From this point, use the same steps you use for a release.

## Options for configure

When you run the configure step there are several options you can configure.

For Autotools, you can see the full list with `./configure --help`.

For Meson, before the initial setup, you can see rofi options in
`meson_options.txt` and Meson options with `meson setup --help`. Meson's
built-in options can be set using regular command line arguments, like so:
`meson setup build --option=value`. Rofi-specific options can be set using the
`-D` argument, like so: `meson setup build -Doption=value`. After the build dir
is set up by `meson setup build`, the `meson configure build` command can be
used to configure options, by the same means.

The most useful one to set is the installation prefix:

```bash
    # Autotools
    ../configure --prefix=<installation path>

    # Meson
    meson setup build --prefix <installation path>
```

f.e.

```bash
    # Autotools
    ../configure --prefix=/usr/

    # Meson
    meson setup build --prefix /usr
```

### Install locally

or to install locally:

```bash
    # Autotools
    ../configure --prefix=${HOME}/.local/

    # Meson
    meson setup build --prefix ${HOME}/.local
```

## Options for make

When you run make you can tweak the build process a little.

### Verbose output

Show the commands called:

```bash
    # Autotools
    make V=1

    # Meson
    ninja -C build -v
```

### Debug build

Compile with debug symbols and no optimization, this is useful for making
backtraces:

```bash
    # Autotools
    make CFLAGS="-O0 -g3" clean rofi

    # Meson
    meson configure build --debug
    ninja -C build
```

### Get a backtrace

Getting a backtrace using GDB is not very handy. Because if rofi get stuck, it
grabs keyboard and mouse. So if it crashes in GDB you are stuck. The best way
to go is to enable core file. (ulimit -c unlimited in bash) then make rofi
crash. You can then load the core in GDB.

```bash
    # Autotools
    gdb rofi core

    # Meson (because it uses a separate build directory)
    gdb build/rofi core
```

> Where the core file is located and what its exact name is different on each
> distributions. Please consult the relevant documentation.

For more information see the rofi-debugging(5) manpage.

## Install distribution

### Debian or Ubuntu

```bash
    apt install rofi
```

### Fedora

```bash
    dnf install rofi
```

### ArchLinux

```bash
    pacman -S rofi
```

### Gentoo

An ebuild is available, `x11-misc/rofi`. It's up to date, but you may need to
enable ~arch to get the latest release:

```bash
    echo 'x11-misc/rofi ~amd64' >> /etc/portage/package.accept_keywords
```

for amd64 or:

```bash
    echo 'x11-misc/rofi ~x86' >> /etc/portage/package.accept_keywords
```

for i386.

To install it, simply issue `emerge rofi`.

### openSUSE

On both openSUSE Leap and openSUSE Tumbleweed rofi can be installed using:

```bash
    sudo zypper install rofi
```

### FreeBSD

```bash
    sudo pkg install rofi
```

### macOS

On macOS rofi can be installed via [MacPorts](https://www.macports.org):

```bash
    sudo port install rofi
```

[master-install]: https://github.com/DaveDavenport/rofi/blob/master/INSTALL.md#install-a-checkout-from-git
