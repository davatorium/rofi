<p align="center">
<a href="https://github.com/davatorium/rofi/issues"><img src="https://img.shields.io/github/issues/davatorium/rofi.svg"></a>
<a href="https://img.shields.io/github/forks/davatorium/rofi.svg"><img src="https://img.shields.io/github/forks/davatorium/rofi.svg"></a>
<a href="https://github.com/davatorium/rofi/stargazers"><img src="https://img.shields.io/github/stars/davatorium/rofi.svg"></a>
<a href="https://github.com/davatorium/rofi/releases"><img src="https://img.shields.io/github/downloads/davatorium/rofi/total.svg"></a>
<a href="https://repology.org/metapackage/rofi/versions"><img src="https://repology.org/badge/tiny-repos/rofi.svg"></a>
</p>

**Please match the documentation and scripts to the version of rofi used**

- [next version](https://github.com/davatorium/rofi)
- [1.7.7](https://github.com/davatorium/rofi/tree/1.7.7)
- [1.7.6](https://github.com/davatorium/rofi/tree/1.7.6)
- [1.7.5](https://github.com/davatorium/rofi/tree/1.7.5)
- [1.7.4](https://github.com/davatorium/rofi/tree/1.7.4)
- [1.7.3](https://github.com/davatorium/rofi/tree/1.7.3)
- [1.7.2](https://github.com/davatorium/rofi/tree/1.7.2)
- [1.7.1](https://github.com/davatorium/rofi/tree/1.7.1)
- [1.7.0](https://github.com/davatorium/rofi/tree/1.7.0)

Also see the locally installed documentation (manpages).

<h1 align="center"> Rofi </h1>
<p align="center"><i>A window switcher, Application launcher and dmenu replacement</i>.</p>

<https://user-images.githubusercontent.com/84911063/139428874-fe182dd6-82c6-49b8-8da1-920ddda3d1ed.mp4>

**Rofi** started as a clone of simpleswitcher, written by [Sean
Pringle](http://github.com/seanpringle/simpleswitcher) - a popup window
switcher roughly based on
[superswitcher](http://code.google.com/p/superswitcher/). Simpleswitcher laid
the foundations, and therefore Sean Pringle deserves most of the credit for
this tool. **Rofi** (renamed, as it lost the *simple* property) has been
extended with extra features, like an application launcher and ssh-launcher,
and can act as a drop-in dmenu replacement, making it a very versatile tool.

**Rofi**, like dmenu, will provide the user with a textual list of options
where one or more can be selected.
This can either be running an application, selecting a window, or options
provided by an external script.

### What is rofi not?

Rofi is not:

- A UI toolkit.

- A library to be used in other applications.

- An application that can support every possible use-case. It tries to be
    generic enough to be usable by everybody.
  - Specific functionality can be added using scripts or plugins, many exists.

- Just a dmenu replacement. The dmenu functionality is a nice 'extra' to
    **rofi**, not its main purpose.

## Table of Contents

- [Features](#features)
- [Modes](#modes)
- [Manpages](#manpage)
- [Installation](#installation)
- [Quickstart](#quickstart)
  - [Usage](#usage)
  - [Configuration](#configuration)
  - [Themes](#themes)
- [Screenshots](#screenshots)
- [Wiki](#wiki)
- [Discussion places](#discussion-places)

## Features

Its main features are:

- Fully configurable keyboard navigation

- Type to filter
  - Tokenized: type any word in any order to filter
  - Case insensitive (togglable)
  - Support for fuzzy-, regex-, prefix-, and glob-matching

- UTF-8 enabled
  - UTF-8-aware string collating
  - International keyboard support (\`e -> è)

- RTL language support

- Cairo drawing and Pango font rendering

- Built-in modes:
  - Window switcher mode
    - EWMH compatible WM
    - Work arounds for i3,bspwm

  - Application launcher

  - Desktop file application launcher

  - SSH launcher mode

  - File browser

  - Combi mode, allowing several modes to be merged into one list

- History-based ordering — last 25 choices are ordered on top based on use
    (optional)

- Levenshtein distance or fzf like sorting of matches (optional)

- Drop-in dmenu replacement
  - Many added improvements

- Easily extensible using scripts and plugins

- Advanced Theming

## Modes

**Rofi** has several built-in modes implementing common use cases and can be
extended by scripts (either called from
**Rofi** or calling **Rofi**) or plugins.

Below is a list of the different modes:

- **run**: launch applications from $PATH, with option to launch in terminal.

- **drun**: launch applications based on desktop files. It tries to be
    compliant to the XDG standard.

- **window**: Switch between windows on an EWMH compatible window manager.

- **ssh**: Connect to a remote host via ssh.

- **filebrowser**: A basic file-browser for opening files.

- **keys**: list internal keybindings.

- **script**: Write (limited) custom mode using simple scripts.

- **combi**: Combine multiple modes into one.

**Rofi** is known to work on Linux and BSD.

## Manpage

For more up to date information, please see the manpages. The other sections
and links might have outdated information as they have relatively less
maintainance than the manpages. So, if you come across any issues please
consult manpages, [discussion](https://github.com/davatorium/rofi/discussions)
and [issue tracker](https://github.com/davatorium/rofi/issues?q=) before filing
new issue.

- Manpages:
  - [rofi](doc/rofi.1.markdown)
  - [rofi-theme](doc/rofi-theme.5.markdown)
  - [rofi-debugging](doc/rofi-debugging.5.markdown)
  - [rofi-script](doc/rofi-script.5.markdown)
  - [rofi-theme-selector](doc/rofi-theme-selector.1.markdown)
  - [rofi-thumbnails](doc/rofi-thumbnails.5.markdown)
  - [rofi-keys](doc/rofi-keys.5.markdown)
  - [rofi-dmenu](doc/rofi-dmenu.5.markdown)

## Installation

Please see the [installation
guide](https://github.com/davatorium/rofi/blob/next/INSTALL.md) for
instructions on how to install **Rofi**.

## Quickstart

### Usage

> **This section just gives a brief overview of the various options. To get the
> full set of options see the [manpages](#manpage) section above**

#### Running rofi

To launch **rofi** directly in a certain mode, specify a mode with `rofi -show <mode>`.
To show the `run` dialog:

```bash
    rofi -show run
```

Or get the options from a script:

```bash
    ~/my_script.sh | rofi -dmenu
```

Specify an ordered, comma-separated list of modes to enable. Enabled modes can
be changed at runtime. Default key is `Ctrl+Tab`. If no modes are specified,
all configured modes will be enabled. To only show the `run` and `ssh`
launcher:

```bash
    rofi -modes "run,ssh" -show run
```

The modes to combine in combi mode.
For syntax to `-combi-modes`, see `-modes`.
To get one merge view, of `window`,`run`, and `ssh`:

```bash
 rofi -show combi -combi-modes "window,run,ssh" -modes combi
```

### Configuration

Generate a default configuration file

```bash
mkdir -p ~/.config/rofi
rofi -dump-config > ~/.config/rofi/config.rasi
```

This creates a file called `config.rasi` in the `~/.config/rofi/` folder. You
can modify this file to set configuration settings and modify themes.
`config.rasi` is the file rofi looks to by default.

Please see the [configuration
guide](https://github.com/davatorium/rofi/blob/next/CONFIG.md) for a summary of
configuration options. More detailed options are provided in the manpages.

### Themes

Please see the [themes
manpages](https://github.com/davatorium/rofi/blob/next/doc/rofi-theme.5.markdown)
for a detailed description.

The latest bundled themes can be found
[here](https://github.com/davatorium/rofi/tree/next/themes).

## Screenshots

Rezlooks:

![screenshot](https://raw.githubusercontent.com/davatorium/rofi/next/releasenotes/1.6.0/icons.png)

Arthur:

![screenshot2](https://raw.githubusercontent.com/davatorium/rofi/next/releasenotes/1.6.0/icons2.png)

Default theme:

![default](https://raw.githubusercontent.com/davatorium/rofi/next/releasenotes/1.4.0/rofi-no-fzf.png)

## Wiki

| ❗ **The Wiki is currently unmaintained and might contain outdated data** |
| --- |

[Go to wiki](https://github.com/davatorium/rofi/wiki) .

### Contents

- [User scripts](https://github.com/davatorium/rofi/wiki/User-scripts)
- [Examples](https://github.com/davatorium/rofi/wiki#examples)
- [dmenu Specs](https://github.com/davatorium/rofi/wiki/dmenu_specs)
- [mode Specs](https://github.com/davatorium/rofi/wiki/mode-Specs)
- [F.A.Q.](https://github.com/davatorium/rofi/wiki/Frequently-Asked-Questions).
- [Script mode](https://github.com/davatorium/rofi/wiki/rfc-script-mode)
- [Creating an issue](https://github.com/davatorium/rofi/blob/master/.github/CONTRIBUTING.md)
- [Creating a Pull request](https://github.com/davatorium/rofi/wiki/Creating-a-pull-request)

## Discussion places

The [GitHub Discussions](https://github.com/davatorium/rofi/discussions) is the
preferred location for discussions.

- [GitHub Discussions](https://github.com/davatorium/rofi/discussions)
- IRC (#rofi on irc.libera.chat)

### Stargazers over time

[![Stargazers over time](https://starchart.cc/davatorium/rofi.svg)](https://starchart.cc/davatorium/rofi)
