# Installation guide

This guide explains how to install rofi using its build system and how you can make debug builds.

Rofi uses autotools (GNU Build system), for more information see
[here](https://www.gnu.org/software/automake/manual/html_node/Autotools-Introduction.html).

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
* libxcb (sometimes split, you need libxcb, libxcb-xkb and libxcb-randr libxcb-xinerama)
* xcb-util
* xcb-util-wm (sometimes split as libxcb-ewmh and libxcb-icccm)
* xcb-util-xrm [new module might not be available in your distribution. The source can be found
  here](https://github.com/Airblader/xcb-util-xrm/)

On debian based systems, the developer packages are in the form of: `<package>-dev` on rpm based
`<package>-devel`.

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

The default installation prefix is: `/usr/local/` use `./configure --prefix={prefix}` to install into another location.

## Install a checkout from git

The GitHub Pages version of these directions may be out of date.  Please use
[INSTALL.md from the online repo][master-install] or your local repository.

[master-install]: https://github.com/DaveDavenport/rofi/blob/master/INSTALL.md#install-a-checkout-from-git

Make a checkout:

```
git clone https://github.com/DaveDavenport/rofi
cd rofi/
```


Pull in dependencies

```
git submodule update --init
```

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


## Options for make

When you run make you can tweak the build process a little.

### Verbose output

Show the commands called:

```
make V=1
```

### Debug build

Compile with debug symbols and no optimization, this is useful for making backtraces:

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

> Where the core file is located and what its exact name is different on each distributions. Please consult the
> relevant documentation.

## Install distribution

### Debian or Ubuntu

```
apt install rofi
```

#### Ubuntu 16.04 Xenial

**Please note that the latest version of rofi in Ubuntu 16.04 is extremely outdated (v0.15.11)** 

This will cause issues with newer scripts (i.e. with clerk) and misses important updates and bug-fixes.
Newer versions of Rofi however requires versions of xcb-util-xrm and libxkbcommon that are not available in the 16.04 repositories.
These need to be manually installed before rofi can be installed either via source code or Zesty version from the [ubuntu's launchpad page for rofi](https://launchpad.net/ubuntu/+source/rofi).

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
