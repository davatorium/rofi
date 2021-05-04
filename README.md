[![Codacy Badge](https://api.codacy.com/project/badge/Grade/ca0310962a7c4b829d0c57f1ab023531)](https://app.codacy.com/app/davatorium/rofi?utm_source=github.com\&utm_medium=referral\&utm_content=davatorium/rofi\&utm_campaign=Badge_Grade_Settings)
[![Build Status](https://travis-ci.org/davatorium/rofi.svg?branch=master)](https://travis-ci.org/davatorium/rofi)
[![codecov.io](https://codecov.io/github/davatorium/rofi/coverage.svg?branch=master)](https://codecov.io/github/davatorium/rofi?branch=master)
[![Issues](https://img.shields.io/github/issues/davatorium/rofi.svg)](https://github.com/davatorium/rofi/issues)
[![Forks](https://img.shields.io/github/forks/davatorium/rofi.svg)](https://github.com/davatorium/rofi/network)
[![Stars](https://img.shields.io/github/stars/davatorium/rofi.svg)](https://github.com/davatorium/rofi/stargazers)
[![Downloads](https://img.shields.io/github/downloads/davatorium/rofi/total.svg)](https://github.com/davatorium/rofi/releases)
[![Coverity](https://scan.coverity.com/projects/3850/badge.svg)](https://scan.coverity.com/projects/davedavenport-rofi)
[![Forum](https://img.shields.io/badge/forum-online-green.svg)](https://reddit.com/r/qtools/)
[![Packages](https://repology.org/badge/tiny-repos/rofi.svg)](https://repology.org/metapackage/rofi/versions)

# A window switcher, Application launcher and dmenu replacement

**Rofi** started as a clone of simpleswitcher, written by [Sean Pringle](http://github.com/seanpringle/simpleswitcher) - a
popup window switcher roughly based on [superswitcher](http://code.google.com/p/superswitcher/).
Simpleswitcher laid the foundations, and therefore Sean Pringle deserves most of the credit for this tool. **Rofi**
(renamed, as it lost the *simple* property) has been extended with extra features, like an application launcher and
ssh-launcher, and can act as a drop-in dmenu replacement, making it a very versatile tool.

**Rofi**, like dmenu, will provide the user with a textual list of options where one or more can be selected.
This can either be running an application, selecting a window, or options provided by an external script.

Its main features are:

*   Fully configurable keyboard navigation
*   Type to filter
    *   Tokenized: type any word in any order to filter
    *   Case insensitive (togglable)
    *   Support for fuzzy-, regex-, and glob matching
*   UTF-8 enabled
    *   UTF-8-aware string collating
    *   International keyboard support (\`e -> è)
*   RTL language support
*   Cairo drawing and Pango font rendering
*   Built-in modes:
    *   Window switcher mode
        *   EWMH compatible WM
    *   Application launcher
    *   Desktop file application launcher
    *   SSH launcher mode
    *   Combi mode, allowing several modes to be merged into one list
*   History-based ordering — last 25 choices are ordered on top based on use (optional)
*   Levenshtein distance or fzf like sorting of matches (optional)
*   Drop-in dmenu replacement
    *   Many added improvements
*   Easily extensible using scripts and plugins
*   Advanced Theming

**Rofi** has several built-in modi implementing common use cases and can be extended by scripts (either called from
**Rofi** or calling **Rofi**) or plugins.

Below is a list of the different modi:

* **run**: launch applications from $PATH, with option to launch in terminal.
* **drun**: launch applications based on desktop files. It tries to be compliant to the XDG standard.
* **window**: Switch between windows on an EWMH compatible window manager.
* **ssh**: Connect to a remote host via ssh.
* **file-browser**: A basic file-browser for opening files.
* **keys**: list internal keybindings.
* **script**: Write (limited) custom mode using simple scripts.
* **combi**: Combine multiple modi into one.

**Rofi** is known to work on Linux and BSD.


# Screenshots

![screenshot](https://raw.githubusercontent.com/davatorium/rofi/next/releasenotes/1.6.0/icons.png)
![screenshot2](https://raw.githubusercontent.com/davatorium/rofi/next/releasenotes/1.6.0/icons2.png)
![default](https://raw.githubusercontent.com/davatorium/rofi/next/releasenotes/1.4.0/rofi-no-fzf.png)

# Manpage

For more up to date information, please see the [manpage](doc/rofi.1.markdown), the [wiki](https://github.com/davatorium/rofi/wiki), or the [forum](https://reddit.com/r/qtools/).

# Installation

Please see the [installation guide](https://github.com/davatorium/rofi/blob/next/INSTALL.md) for instructions on how to
install **Rofi**.

# What is rofi not?

Rofi is not:

*   A preview application. In other words, it will not show a (small) preview of images, movies or other files.
*   A UI toolkit.
*   A library to be used in other applications.
*   An application that can support every possible use-case. It tries to be generic enough to be usable by everybody.
    * Specific functionality can be added using scripts or plugins.
*   Just a dmenu replacement. The dmenu functionality is a nice 'extra' to **rofi**, not its main purpose.
