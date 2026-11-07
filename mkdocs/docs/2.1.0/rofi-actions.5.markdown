# rofi-actions(5)

## NAME

**rofi-actions** - Custom commands following interaction with rofi menus

## DESCRIPTION

**rofi** allows to set custom commands or scripts to be executed when some actions are performed in the menu, such as changing selection, accepting an entry or canceling.

This makes it possible for example to play sound effects or read aloud menu entries on selection.

## USAGE

Following is the list of rofi flags for specifying custom commands or scripts to execute on supported actions:

`-on-selection-changed` *cmd*

Command or script to run when the current selection changes. Selected text is forwarded to the command replacing the pattern *{entry}*.

`-on-entry-accepted` *cmd*

Command or script to run when a menu entry is accepted. Accepted text is forwarded to the command replacing the pattern *{entry}*.

`-on-mode-changed` *cmd*

Command or script to run when the menu mode (e.g. drun,window,ssh...) is changed.

`-on-menu-canceled` *cmd*

Command or script to run when the menu is canceled.

`-on-menu-error` *cmd*

Command or script to run when an error menu is shown (e.g. `rofi -e "error message"`). Error text is forwarded to the command replacing the pattern *{error}*.

`-on-screenshot-taken` *cmd*

Command or script to run when a screenshot of rofi is taken. Screenshot path is forwarded to the command replacing the pattern *{path}*.

### Example usage

Rofi command line:

```bash
rofi -on-selection-changed "/path/to/select.sh {entry}" \
     -on-entry-accepted "/path/to/accept.sh {entry}" \
     -on-menu-canceled "/path/to/exit.sh" \
     -on-mode-changed "/path/to/change.sh" \
     -on-menu-error "/path/to/error.sh {error}" \
     -on-screenshot-taken "/path/to/camera.sh {path}" \
     -show drun
```

Rofi config file:

```css
configuration {
    on-selection-changed: "/path/to/select.sh {entry}";
    on-entry-accepted: "/path/to/accept.sh {entry}";
    on-menu-canceled: "/path/to/exit.sh";
    on-mode-changed: "/path/to/change.sh";
    on-menu-error: "/path/to/error.sh {error}";
    on-screenshot-taken: "/path/to/camera.sh {path}";
}
```

### Play sound effects

Here's an example bash script that plays a sound effect using `aplay` when the current selection is changed:

```bash
#!/bin/bash

coproc aplay -q $HOME/Music/selecting_an_item.wav
```

The use of `coproc` for playing sounds is suggested, otherwise the rofi process will wait for sounds to end playback before exiting.

### Read aloud

Here's an example bash script that reads aloud currently selected entries using `espeak`:

```bash
#!/bin/bash

killall espeak
echo "selected: $@" | espeak
```
