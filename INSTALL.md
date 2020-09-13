# Installation guide

This guide explains how to install rofi using its build system and how you can make debug builds.

Rofi uses autotools (GNU Build system), for more information see
[here](https://www.gnu.org/software/automake/manual/html_node/Autotools-Introduction.html).
You can also use [Meson](https://mesonbuild.com/) as an alternative.

## DEPENDENCY

### For building:

* C compiler that supports the c99 standard. (gcc or clang)
* make
* autoconf
* automake (1.11.3 or up)
* pkg-config
* flex 2.5.39 or higher
* bison
* check (Can be disabled using the `--disable-check` configure flag)
  check is used for build-time tests and does not affect functionality.
* Developer packages of the external libraries
* glib-compile-resources

### External libraries

* libpango
* libpangocairo
* libcairo
* libcairo-xcb
* libglib2.0 >= 2.40
  * gmodule-2.0
  * gio-unix-2.0
* librsvg2.0
* libstartup-notification-1.0
* libxkbcommon >= 0.4.1
* libxkbcommon-x11
* libjpeg
* libxcb (sometimes split, you need libxcb, libxcb-xkb and libxcb-randr libxcb-xinerama)
* xcb-util
* xcb-util-wm (sometimes split as libxcb-ewmh and libxcb-icccm)
* xcb-util-xrm [new module might not be available in your distribution. The source can be found
  here](https://github.com/Airblader/xcb-util-xrm/)

On debian based systems, the developer packages are in the form of: `<package>-dev` on rpm based
`<package>-devel`.

## Install from a release

### Autotools

Create a build directory and enter it:

```
mkdir build && cd build
```

Check dependencies and configure build system:

```
../configure
```

Build Rofi:

```
make
```

The actual install, execute as root (if needed):

```
make install
```

The default installation prefix is: `/usr/local/` use `./configure --prefix={prefix}` to install into another location.

### Meson

Check dependencies and configure build system:

```
meson setup build
```

Build Rofi:

```
ninja -C build
```

The actual install, execute as root (if needed):

```
ninja -C build install
```

The default installation prefix is: `/usr/local/` use `meson setup build --prefix={prefix}` to install into another location.

## Install a checkout from git

The GitHub Pages version of these directions may be out of date.  Please use
[INSTALL.md from the online repo][master-install] or your local repository.

[master-install]: https://github.com/DaveDavenport/rofi/blob/master/INSTALL.md#install-a-checkout-from-git

If you don't have a checkout:

```
git clone --recursive https://github.com/DaveDavenport/rofi
cd rofi/
```

If you already have a checkout:

```
cd rofi/
git pull
git submodule update --init
```

For Autotools you have an extra step, to generate build system:

```
autoreconf -i
```

From this point, use the same steps you use for a release.



## Options for configure

When you run the configure step there are several options you can configure.
For Autotools, you can see the full list with `./configure --help`.
For Meson, before the initial setup, you can see rofi options in `meson_options.txt` and Meson options with `meson setup --help`.
After the initial setup, use `meson configure build`.

The most useful one to set the installation prefix:

```
# Autotools
../configure --prefix=<installation path>

# Meson
meson setup build --prefix <installation path>
```

f.e.

```
# Autotools
../configure --prefix=/usr/

# Meson
meson setup build --prefix /usr
```

### Install locally

or to install locally:

```
# Autotools
../configure --prefix=${HOME}/.local/

# Meson
meson setup build --prefix ${HOME}/.local
```


## Options for make

When you run make you can tweak the build process a little.

### Verbose output

Show the commands called:

```
# Autotools
make V=1

# Meson
ninja -C build -v
```

### Debug build

Compile with debug symbols and no optimization, this is useful for making backtraces:

```
# Autotools
make CFLAGS="-O0 -g3" clean rofi

# Meson
meson configure build --debug
ninja -C build
```

### Get a backtrace

Getting a backtrace using GDB is not very handy. Because if rofi get stuck, it grabs keyboard and
mouse. So if it crashes in GDB you are stuck.
The best way to go is to enable core file. (ulimit -c unlimited in bash) then make rofi crash. You
can then load the core in GDB.

```
# Autotools
gdb rofi core

# Meson (because it uses a separate build directory)
gdb build/rofi core
```

> Where the core file is located and what its exact name is different on each distributions. Please consult the
> relevant documentation.

## Install distribution

### Debian or Ubuntu

```
apt install rofi
```

### Fedora

rofi from [russianfedora repository](http://ru.fedoracommunity.org/repository)
and also
[Yaroslav's COPR (Cool Other Package Repo)](https://copr.fedorainfracloud.org/coprs/yaroslav/i3desktop/)


### ArchLinux

```
pacman -S rofi
```

### Gentoo

An ebuild is available, `x11-misc/rofi`. It's up to date, but you may need to
enable ~arch to get the latest release:

```
echo 'x11-misc/rofi ~amd64' >> /etc/portage/package.accept_keywords
```

for amd64 or:

```
echo 'x11-misc/rofi ~x86' >> /etc/portage/package.accept_keywords
```

for i386.

To install it, simply issue `emerge rofi`.

### openSUSE

On both openSUSE Leap and openSUSE Tumbleweed rofi can be installed using:

```
sudo zypper install rofi
```

### FreeBSD

```
sudo pkg install rofi
```
