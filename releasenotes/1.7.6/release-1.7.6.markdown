# 1.7.6: Traveling Time

## Recursive file browser

An experimental file browser is introduced in this version. This recursively scans through the 
users home directory (this is configurable) to find files.
Its designed to be asynchronous and very fast.

The following settings can be configured:

‘‘‘css
configuration {
   recursivebrowser {
      /** Directory the file browser starts in. */
      directory: "/some/directory";
      /** return 1 on cancel. */
      cancel‐returns‐1: true;
      /** filter entries using regex */
      filter‐regex: "(.*cache.*|.*.o)";
      /** command */
      command: "xdg‐open";
   }
}
‘‘‘


## Copy to clipboard support

Add support to copy current selected item to clipboard.
The added `control-c` binding copies the current selection to the clipboard.
THIS ONLY WORKS WITH CLIPBOARD MANAGER!!! Once rofi is closes, the data is
gone!
    
## entry box history

You can now recall and move through previous queries by using
`kb-entry-history-up` or 'kb-entry-history-down` keys. (`Control-Up`,
`Control-Down`).

The following settings can be configured:

‘‘‘css
configuration {
    entry  {
        max‐history: 30;
    }
}
‘‘‘


## Fix calc

There was a non-parsable grammar in the 'calc' part of the language.
The % operator (modulo) overloaded with percent and could leave to statements
having multiple valid but contradicting interpretations. To resolve this the modulo
operator is now `modulo`. Including in this patch several smaller issues with the
parser where fixed.

## Text outline

## Website

The current documentation is now also available on online at:
[https://davatorium.github.io/rofi/](https://davatorium.github.io/rofi/)

# Thanks to

Big thanks to everybody reporting issues.
Special thanks goes to:

  * a1346054
  * aloispichler
  * Amith Mohanan
  * Christian Friedow
  * cognitiond
  * David Kosorin
  * Dimitris Triantafyllidis
  * duarm
  * Fabian Winter
  * Gutyina Gergő
  * Jasper Lievisse Adriaanse
  * Jorge
  * Martin Weinelt
  * Morgane Glidic
  * Naïm Favier
  * Nikita Zlobin
  * nomoo
  * notuxic
  * Rasmus Steinke
  * Tim Pope
  * TonCherAmi
  * vE5li
  * Yuta Katayama
  * Danny Colin

Apologies if I mistyped or missed anybody.
