# rofi-sensible-terminal 1 rofi-sensible-terminal

## NAME

**rofi-sensible-terminal** -  launches $TERMINAL with fallbacks

## SYNOPSIS

rofi-sensible-terminal [arguments]

## DESCRIPTION

rofi-sensible-terminal is invoked in the rofi default config to start a terminal. This
wrapper script is necessary since there is no distribution-independent terminal launcher
(but for example Debian has x-terminal-emulator). Distribution packagers are responsible for
shipping this script in a way which is appropriate for the distribution.

It tries to start one of the following (in that order):

* `$TERMINAL` (this is a non-standard variable)
* x-terminal-emulator
* urxvt
* rxvt
* st
* terminology
* qterminal
* Eterm
* aterm
* uxterm
* xterm
* roxterm
* xfce4-terminal.wrapper
* mate-terminal
* lxterminal
* konsole
* alacritty
* kitty


## SEE ALSO

rofi(1)

## AUTHORS

Dave Davenport and contributors

Copied script from i3:
Michael Stapelberg and contributors
