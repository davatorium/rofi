# Theme format 3.0

Rofi is now at the 3rd version of it theming format. Where previous formats was a basic version with an extension. This
is a full rewrite. The new format is loosely modelled after [css](https://en.wikipedia.org/wiki/Cascading_Style_Sheets).
This will hopefully be familiar and make it easier for people to get started with theming.

This file is organized as follow, first we give the specification of the file format.
In the second part we will list the possible options. In the final section a few examples are shown.

## File Format Specification

### Encoding

The encoding of the file is ascii.
Both unix ('\n') and windows ('\r\n') newlines format are supported. But unix is preferred. 

### Comments

C and C++ file comments are support.

* Anything after  `// ` and before a newline is considered a comment.
* Everything between `/*` and `*/` are a comment.

Comments can be nested and inline.

The following is valid:
```css
// Magic comment.
property: /* comment */ value;
```

However this is not:
```css
prop/*comment*/erty: value;
```

### White space

White space and newlines, like comments, are ignored by the parser.

This:

```css
property: name;
```

Is identical to:

```css
property             :
name

;
```

### Basic Structure

The file is structured like:

```
/* Global properties section */
* {
    {list of properties}
}

/* Element theme section. */ 
#{element path} {
    {list of properties}
}
```

#### Global properties section

Each theme has one, optional, global properties list.
If present, the global properties section has the be the first section in the file.

The global properties section is special, as the properties here denote the defaults for each element.
Reference properties (see properties section) can only link to properties in this section.

The section may only contain a '*' before the brace open..

#### Element theme section 

A theme can have multiple element theme sections.

The element path can consist of multiple names separated by whitespace or dots.
Each element may contain any number of letters, numbers and '-'. 
The first element should always start with a '#'.

This is a valid element name:

```css
#window mainbox listview element normal.normal {
}
```

And is identical to:

```css
#window.mainbox.listview.element normal.normal {
}
```

Each section inherits the properties of it parents and it own properties are added. If the property is specified both in
the parent as in the child, the childs property has priority.
So `#window mainbox` will contain all properties of `#window` and `#window mainbox`.

In the following example:
```css
#window {
 a: 1;
 b: 2;
}
#window mainbox {
    b: 4;
    c: 8;
}
```

The element `#window mainbox` will have the following set of properties:

```css
a: 1;
b: 4;
c: 8;
```


#### Properties

The properties in a section consist of:

```css
{identifier}: {value};
```

The `identifier` names the property specified. Identifier can consist of any combination of numbers, letters and '-'. It
may not contain any whitespace.

The current theme format support different type of properties:

 * a string.
 * an integer positive number.
 * a positive fractional number.
 * a boolean value.
 * a color.
 * text style.
 * line style.
 * a distance.
 * a padding.
 * a border.
 * a position.
 * a reference.

##### String

* Format:  `"[:print:]"`

A string is always surrounded by quotes ('"'), between the quotes it can have any printable character.

For example:

```css
font: "Awasome 12";
```

###### Integer

* Format: "[:digit:]"

An integer may contain any number.

For examples:

```css
lines: 12;
```

##### Real

* Format: `[:digit:]+(\.[:digit:]+)?`

A real is an integer with an optional fraction.

For example:

```css
real: 3.4;
```

##### Boolean

* Format: `(true|false)`

For example:

```css
dynamic: false;
```

##### Color

* Format: '#{HEX}{6}'
* Format: '#{HEX}{8}'
* Format: 'argb:{HEX}{8}' 
* Format: 'rgb({INTEGER},{INTEGER},{INTEGER})'
* Format: 'rgba({INTEGER},{INTEGER},{INTEGER}, {REAL})'

Where '{HEX}' is a hexidecimal number ('0-9a-f'). The '{INTEGER}' value can be between 0 and 255, the '{Real}' value
between 0.0 and 1.0.


The first formats specify the color as RRGGBB (R = red, G = green, B = Blue), the second adds an alpha (A) channel:
AARRGGB. 

For example:

```css
background: #FF0000;
foreground: rgba(0,0,1, 0.5);
```

##### Text style

* Format: (bold|italic|underline|none)

Text style indicates how the text should be displayed.  None indicates no style should be applied.

##### Line style

* Format: `(dash|solid)`

Indicates how a line should be drawn.


##### Distance

* Format: '{Integer}px'
* Format: '{Real}em'
* Format: '{Real}%'

##### Padding

* Format: '{Integer}'
* Format: '{Distance}'
* Format: '{Distance} {Distance}'
* Format: '{Distance} {Distance} {Distance}'
* Format: '{Distance} {Distance} {Distance} {Distance}'

###### Border

* Format: '{Integer}'
* Format: '{Distance}'
* Format: '{Distance} {Distance}'
* Format: '{Distance} {Distance} {Distance}'
* Format: '{Distance} {Distance} {Distance} {Distance}'
* Format: '{Distance} {Line style}'
* Format: '{Distance} {Line style} {Distance} {Line style}'
* Format: '{Distance} {Line style} {Distance} {Line style} {Distance} {Line style}'
* Format: '{Distance} {Line style} {Distance} {Line style} {Distance} {Line style} {Distance} {Line style}'

###### Position

* Format: '(center|east|north|west|northeast|northweast|south|southwest|southeast)'

###### Reference

* Format: '@{PROPERTY NAME}'

A reference can point to another reference. Currently the maximum number of redirects is 20.

> REST NEEDS updating.

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
           * `#window.mainbox.inputbar.case-indicator`: The case/sort indicator @textbox
           * `#window.mainbox.inputbar.prompt`: The prompt @textbox
           * `#window.mainbox.inputbar.entry`: The main entry @textbox
         * `#window.mainbox.listview`
            * `#window.mainbox.listview.box`: The listview container. 
            * `#window.mainbox.listview.scrollbar`: The listview scrollbar
            * `#window.mainbox.listview.element`: The entries in the listview
         * `#window.mainbox.sidebar`
           * `#window.mainbox.sidebar.box`: The main horizontal @box packing the buttons.
           * `#window.mainbox.sidebar.button`: The buttons @textbox for each mode.
         * `#window.mainbox.message`
           * `#window.mainbox.message.textbox`: The message textbox.
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
 * position: (center|north|south|east|west|northeast|northwest|southwest|southeast)
 * highlight-style:  (none|bold|underline|italic)

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
    * position:       position
        The place of the anchor on the monitor.
    * anchor:         anchor
        The anchor position on the window.

  * scrollbar
    * foreground:      color
    * handle-width:    distance
    * handle-color:    color 
    * foreground:      color

  * box
    * spacing:         distance

  * textbox:
    *  background:      color
    *  foreground:      color
    *  text:            The text color to use (falls back to foreground if not set)
    * highlight:        highlight {color}

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

