# rofi-keys(5)

## NAME

**rofi keys** - Rofi Key and Mouse bindings

## DESCRIPTION

**rofi** supports overriding of any of it key and mouse binding.

## Set a Binding

Bindings can be done on the command line (-{bindingname}):

```bash
rofi -show run -kb-accept-entry 'Control+Shift+space'
```

or via the configuration file:

```css
configuration {
  kb-accept-entry: "Control+Shift+space";
}
```

The key can be set by its name (see above) or its keycode:

```css
configuration {
  kb-accept-entry: "Control+Shift+[65]";
}
```

An easy way to look up keycode is xev(1).

Multiple keys can be specified for an action as a comma separated list:

```css
configuration {
  kb-accept-entry: "Control+Shift+space,Return";
}
```

By Default **rofi** reacts on pressing, to act on the release of all keys
prepend the binding with `!`:

```css
configuration {
  kb-accept-entry: "!Control+Shift+space,Return";
}
```

## Unset a Binding

To unset a binding, pass an empty string.

```css
configuration {
  kb-clear-line: "";
}
```

## Keyboard Bindings

| Keybinding Name              | Description                                 | Default Shortcut(s)                    |
| ---------------------------- | ------------------------------------------- | -------------------------------------- |
| `kb-primary-paste`           | Paste primary selection                     | Control+V, Shift+Insert                |
| `kb-secondary-paste`         | Paste clipboard                             | Control+v, Insert                      |
| `kb-secondary-copy`          | Copy current selection to clipboard         | Control+c                              |
| `kb-clear-line`              | Clear input line                            | Control+w                              |
| `kb-move-front`              | Beginning of line                           | Control+a                              |
| `kb-move-end`                | End of line                                 | Control+e                              |
| `kb-move-word-back`          | Move back one word                          | Alt+b, Control+Left                    |
| `kb-move-word-forward`       | Move forward one word                       | Alt+f, Control+Right                   |
| `kb-move-char-back`          | Move back one char                          | Left, Control+b                        |
| `kb-move-char-forward`       | Move forward one char                       | Right, Control+f                       |
| `kb-remove-word-back`        | Delete previous word                        | Control+Alt+h, Control+BackSpace       |
| `kb-remove-word-forward`     | Delete next word                            | Control+Alt+d                          |
| `kb-remove-char-forward`     | Delete next char                            | Delete, Control+d                      |
| `kb-remove-char-back`        | Delete previous char                        | BackSpace, Shift+BackSpace, Control+h  |
| `kb-remove-to-eol`           | Delete till the end of line                 | Control+k                              |
| `kb-remove-to-sol`           | Delete till the start of line               | Control+u                              |
| `kb-accept-entry`            | Accept entry                                | Control+j, Control+m, Return, KP_Enter |
| `kb-accept-custom`           | Use entered text as command (ssh/run modes) | Control+Return                         |
| `kb-accept-custom-alt`       | Use entered text as command (ssh/run modes) | Control+Shift+Return                   |
| `kb-accept-alt`              | Use alternate accept command                | Shift+Return                           |
| `kb-delete-entry`            | Delete entry from history                   | Shift+Delete                           |
| `kb-mode-next`               | Switch to the next mode                     | Shift+Right, Control+Tab               |
| `kb-mode-previous`           | Switch to the previous mode                 | Shift+Left, Control+ISO_Left_Tab       |
| `kb-mode-complete`           | Start completion for mode                   | Control+l                              |
| `kb-row-left`                | Go to the previous column                   | Control+Page_Up                        |
| `kb-row-right`               | Go to the next column                       | Control+Page_Down                      |
| `kb-row-up`                  | Select previous entry                       | Up, Control+p                          |
| `kb-row-down`                | Select next entry                           | Down, Control+n                        |
| `kb-row-tab`                 | Go to next row or accept/next mode          | —                                      |
| `kb-element-next`            | Go to next row                              | Tab                                    |
| `kb-element-prev`            | Go to previous row                          | ISO_Left_Tab                           |
| `kb-page-prev`               | Go to the previous page                     | Page_Up                                |
| `kb-page-next`               | Go to the next page                         | Page_Down                              |
| `kb-row-first`               | Go to the first entry                       | Home, KP_Home                          |
| `kb-row-last`                | Go to the last entry                        | End, KP_End                            |
| `kb-row-select`              | Set selected item as input text             | Control+space                          |
| `kb-screenshot`              | Take a screenshot of the rofi window        | Alt+S                                  |
| `kb-ellipsize`               | Toggle ellipsize modes                      | Alt+period                             |
| `kb-toggle-case-sensitivity` | Toggle case sensitivity                     | grave, dead_grave                      |
| `kb-toggle-sort`             | Toggle filtered menu sort                   | Alt+grave                              |
| `kb-cancel`                  | Quit rofi                                   | Escape, Control+g, Control+[           |
| `kb-custom-1`                | Custom keybinding 1                         | Alt+1                                  |
| `kb-custom-2`                | Custom keybinding 2                         | Alt+2                                  |
| `kb-custom-3`                | Custom keybinding 3                         | Alt+3                                  |
| `kb-custom-4`                | Custom keybinding 4                         | Alt+4                                  |
| `kb-custom-5`                | Custom Keybinding 5                         | Alt+5                                  |
| `kb-custom-6`                | Custom keybinding 6                         | Alt+6                                  |
| `kb-custom-7`                | Custom Keybinding 7                         | Alt+7                                  |
| `kb-custom-8`                | Custom keybinding 8                         | Alt+8                                  |
| `kb-custom-9`                | Custom keybinding 9                         | Alt+9                                  |
| `kb-custom-10`               | Custom keybinding 10                        | Alt+0                                  |
| `kb-custom-11`               | Custom keybinding 11                        | Alt+exclam                             |
| `kb-custom-12`               | Custom keybinding 12                        | Alt+at                                 |
| `kb-custom-13`               | Custom keybinding 13                        | Alt+numbersign                         |
| `kb-custom-14`               | Custom keybinding 14                        | Alt+dollar                             |
| `kb-custom-15`               | Custom keybinding 15                        | Alt+percent                            |
| `kb-custom-16`               | Custom keybinding 16                        | Alt+dead_circumflex                    |
| `kb-custom-17`               | Custom keybinding 17                        | Alt+ampersand                          |
| `kb-custom-18`               | Custom keybinding 18                        | Alt+asterisk                           |
| `kb-custom-19`               | Custom Keybinding 19                        | Alt+parenleft                          |
| `kb-select-1`                | Select row 1                                | Super+1                                |
| `kb-select-2`                | Select row 2                                | Super+2                                |
| `kb-select-3`                | Select row 3                                | Super+3                                |
| `kb-select-4`                | Select row 4                                | Super+4                                |
| `kb-select-5`                | Select row 5                                | Super+5                                |
| `kb-select-6`                | Select row 6                                | Super+6                                |
| `kb-select-7`                | Select row 7                                | Super+7                                |
| `kb-select-8`                | Select row 8                                | Super+8                                |
| `kb-select-9`                | Select row 9                                | Super+9                                |
| `kb-select-10`               | Select row 10                               | Super+0                                |
| `kb-entry-history-up`        | Go up in the entry history                  | Control+Up                             |
| `kb-entry-history-down`      | Go down in the entry history                | Control+Down                           |
| `kb-matcher-up`              | Select the next matcher                     | Super+equal                            |
| `kb-matcher-down`            | Select the previous matcher                 | Super+minus                            |

## Mouse Bindings

| Mouse Binding Name | Description                           | Default Shortcut(s)   |
| ------------------ | ------------------------------------- | --------------------- |
| `ml-row-left`      | Go to the previous column             | ScrollLeft            |
| `ml-row-right`     | Go to the next column                 | ScrollRight           |
| `ml-row-up`        | Select previous entry                 | ScrollUp              |
| `ml-row-down`      | Select next entry                     | ScrollDown            |
| `me-select-entry`  | Select hovered row                    | MousePrimary          |
| `me-accept-entry`  | Accept hovered row                    | MouseDPrimary         |
| `me-accept-custom` | Accept hovered row with custom action | Control+MouseDPrimary |

## Mouse Key Bindings

The following mouse buttons can be bound:

* `Primary`: Primary (Left) mouse button click.
* `Secondary`:  Secondary (Right) mouse button click.
* `Middle`: Middle mouse button click.
* `Forward`: The forward mouse button.
* `Back`: The back mouse button.
* `ExtraN`: The N'the mouse button. (Depending on mouse support).

The Identifier is constructed as follow:

`Mouse<D><Button>`

* `D` indicates optional Double press.
* `Button` is the button name.

So `MouseDPrimary` is Primary (`Left`) mouse button double click.

## SEE ALSO

rofi(1), rofi-sensible-terminal(1), rofi-theme(5), rofi-script(5)

## AUTHOR

Qball Cow <qball@gmpclient.org>

Rasmus Steinke <rasi@xssn.at>

Morgane Glidic <sardemff7+rofi@sardemff7.net>

Original code based on work by: Sean Pringle <sean.pringle@gmail.com>

For a full list of authors, check the AUTHORS file.
