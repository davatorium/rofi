# ROFI-SCRIPT 5 rofi-script

## NAME

**rofi script mode** - Rofi format for scriptable modi. 


## DESCRIPTION

**rofi** supports modes that use simple scripts in the background to generate a list and process the result from user
actions.  This provide a simple interface to make simple extensions to rofi.


## USAGE

To specify a script mode, set a mode with the following syntax: "{name}:{executable}"

For example:

```
rofi -show fb -modi "fb:file_browser.sh"
```

The name should be unique.

## API

Rofi calls the executable without arguments on startup.  This should generate a list of options, separated by a newline
(`\n`).
If the user selects an option, rofi calls the executable with the text of that option as the first argument.
If the script returns no entries, rofi quits.

A simple script would be:

```bash
#!/usr/bin/env bash

if [ x"$@" = x"quit" ]
then
    exit 0
fi
echo "reload"
echo "quit"

```

This shows two entries, reload and quit. When the quit entry is selected, rofi closes.

## Passing mode options

Extra options, like setting the prompt, can be set by the script.
Extra options are lines that start with a NULL character (`\0`) followed by a key, separator (`\x1f`) and value.

For example to set the prompt:

```bash
    echo -en "\0prompt\x1fChange prompt\n"
```

The following extra options exists:

 * **prompt**:      Update the prompt text.
 * **message**:     Update the message text.
 * **markup-rows**: If 'true' renders markup in the row.
 * **urgent**:      Mark rows as urgent. (for syntax see the urgent option in dmenu mode)
 * **active**:      Mark rows as active. (for syntax see the active option in dmenu mode)

## Parsing row options

Extra options for individual rows can be set.
The extra option can be specified following the same syntax as mode option, but following the entry.

For example:

```bash
    echo -en "aap\0icon\x1ffolder\n"
```

The following options are supported:

 * **icon**: Set the icon for that row.
 * **meta**: Specify invisible search terms.
 * **nonselectable**: If true the row cannot activated.



## SEE ALSO

rofi-sensible-terminal(1), dmenu(1), rofi-theme(5), rofi-theme-selector(1)

## AUTHOR

Qball Cow <qball@gmpclient.org>

Rasmus Steinke <rasi@xssn.at>

Quentin Glidic <sardemff7+rofi@sardemff7.net>


Original code based on work by: Sean Pringle <sean.pringle@gmail.com>

For a full list of authors, check the AUTHORS file.
