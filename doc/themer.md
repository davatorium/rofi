# Basic Organization

Each widget has:

## Name

Name: Internal name of the widget.

Sub-widgets are {Parent}.{Child}.

Example: window, window.mainbox.listview, window.mainbox.listview.element

Names are prefixed with a `#`

List of names in **rofi**:

 * `#window`
    * `#window.overlay`: The overlay widget.
    * `#window.mainbox`
        * `#window.mainbox.box`: The main vertical @box
         * `#window.mainbox.inputbar`
           * `#window.mainbox.inputbar.box`: The horizontal @box packing the widgets.
           * `#window.mainbox.inputbar.separator`: The separator under/above the inputbar.
           * `#window.mainbox.inputbar.case-indicator`: The case/sort indicator @textbox
           * `#window.mainbox.inputbar.prompt`: The prompt @textbox
           * `#window.mainbox.inputbar.entry`: The main entry @textbox
         * `#window.mainbox.listview`
            * `#window.mainbox.listview.scrollbar`: The listview scrollbar
            * `#window.mainbox.listview.element`: The entries in the listview
         * `#window.mainbox.sidebar`
           * `#window.mainbox.sidebar.box`: The main horizontal @box packing the buttons.
           * `#window.mainbox.sidebar.button`: The buttons @textbox for each mode.
           * `#window.mainbox.sidebar.separator`: The separator under/above the sidebar.
         * `#window.mainbox.message`
           * `#window.mainbox.message.textbox`: The message textbox.
           * `#window.mainbox.message.separator`: The separator under/above the sidebar.
           * `#window.mainbox.message.box`: The box containing the message.

## State

State: State of widget

Optional flag(s) indicating state.

These are appended after the name or class of the widget.

`#window.mainbox.sidebar.button selected.normal { }`

`#window.mainbox.listview.element selected.urgent { }`

Currently only the entrybox and scrollbar has states:

`{visible modifier}.{state}`

Where `visible modifier` can be:
 * normal: No modification.
 * selected: The entry is selected/highlighted by user.
 * alternate: The entry is at an alternating row. (uneven row)

Where `state` is:
 * normal: No modification.
 * urgent: This entry is marked urgent.
 * activE: This entry is marked active.

These can be mixed.

Example:
```
#name.to.textbox selected.active {
    background: #003642;
    foreground: #008ed4;
}
```

Sets all selected textboxes marked active to the given foreground and background color.

The scrollbar when drawing uses the `handle` state when drawing the small scrollbar handle.
Allowing overriding of color.

# File structure

The file is structured as follows

```
/* Global properties, that apply as default to all widgets. */
{list of properties}

#{name} {optional state} {
    {list of properties}
}
#{name}.{optional state} {
    {list of properties}
}
```

The global properties an freeĺy be mixed between entries.
Name and state can be separated by a comman, or joined using a dot.

Each property is constructed like:
```
{key} : {value} ;
```
Key is a simple ascii string.
Separated from value by a colon ':';
Value supports the following formats:

 * string:  `"{string}"`
 * integer: `[0-9]+`
 * double:  `[0-9]+\.[0-9]`
 * boolean: `true|false`
 * color:
    * `#[0-9a-fA-F]{6}`: hexidecimal rgb color.
    * `#[0-9a-fA-F]{8}`:  hexidecimal argb color.
    * `argb:[0-0a-fA-F]{8}`: Old **rofi** argb color style.
    * `rgba\([0-9]{1,3},[0-9]{1,3}, [0-9]{1,3}, {double}\)`: css style rgba color.
    * `rgb\([0-9]{1,3},[0-9]{1,3}, [0-9]{1,3}\)`: css style rgb color.
 * distance:
    * `{size}{unit} {line-style}`
       * unit can be px,em,%. for px `{size}` is an integer number, for %,em it is a positive real.
       * {line-style} can be `dash` or `solid` and is optional.
 * padding: `({distance}){1,4}`

Each property is closed by a semi-colon `;`;

The following properties are currently supports:

 * all widgets:
    * padding:         distance
    * margin:          distance
    * border:          distance
    * background:      color
    * foreground:      color
    * end:             boolean

 * window:
    * font:            string
    * transparency:    string
        - real
        - background
        - screenshot
        - Path to png file

  * scrollbar
    * foreground:      color
  * scrollbar handle
    * foreground:      color

  * box
    * spacing:         distance

  * textbox:
    *  background:      color
    *  foreground:      color

  * listview:
    * columns:         integer
    * fixed-height:    boolean
    * dynamic:         boolean
    * scrollbar:       boolean
    * scrollbar-width: distance
    * cycle:           boolean
    * spacing:         distance


## Resolving properties

It tries to find the longest match down the dependency tree.

