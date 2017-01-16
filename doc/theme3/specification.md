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
* Everything between `/*` and `*/` is a comment.

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

### File extension

The prefered file extension for the new theme format is *rasi*. This is an abbriviation for **r**ofi **a**advanced
**s**tyle **i**nformation (Or the nick of the user that helped form the new specification).

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
 * an integer number.
 * a fractional number.
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

* Format:  `"[:print:]+"`

A string is always surrounded by quotes ('"'), between the quotes it can have any printable character.

For example:

```css
font: "Awasome 12";
```

###### Integer

* Format: `[-+]?[:digit:]+`

An integer may contain any number.

For examples:

```css
lines: 12;
```

##### Real

* Format: `[-+]?[:digit:]+(\.[:digit:]+)?`

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

* Format: `#{HEX}{6}`
* Format: `#{HEX}{8}`
* Format: `argb:{HEX}{8}`
* Format: `rgb({INTEGER},{INTEGER},{INTEGER})`
* Format: `rgba({INTEGER},{INTEGER},{INTEGER}, {REAL})`

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

* Format: `(bold|italic|underline|none)`

Text style indicates how the text should be displayed.  None indicates no style should be applied.

##### Line style

* Format: `(dash|solid)`

Indicates how a line should be drawn.


##### Distance

* Format: `{Integer}px`
* Format: `{Real}em`
* Format: `{Real}%`

##### Padding

* Format: `{Integer}`
* Format: `{Distance}`
* Format: `{Distance} {Distance}`
* Format: `{Distance} {Distance} {Distance}`
* Format: `{Distance} {Distance} {Distance} {Distance}`

###### Border

* Format: `{Integer}`
* Format: `{Distance}`
* Format: `{Distance} {Distance}`
* Format: `{Distance} {Distance} {Distance}`
* Format: `{Distance} {Distance} {Distance} {Distance}`
* Format: `{Distance} {Line style}`
* Format: `{Distance} {Line style} {Distance} {Line style}`
* Format: `{Distance} {Line style} {Distance} {Line style} {Distance} {Line style}`
* Format: `{Distance} {Line style} {Distance} {Line style} {Distance} {Line style} {Distance} {Line style}`

###### Position

* Format: `(center|east|north|west|northeast|northweast|south|southwest|southeast)`

###### Reference

* Format: `@{PROPERTY NAME}`

A reference can point to another reference. Currently the maximum number of redirects is 20.


## Elements Paths

Element paths exists of two parts, the first part refers to the actual widget by name.
Some widgets have an extra state.

For example:

```css
#window mainbox listview element .selected {
}
```

Here `#window mainbox listview element` is the name of the widget, `selected` is the state of the widget.

The difference between dots and spaces is purely cosmetic. These are all the same:
```css
#window mainbox listview element .selected {
}
#window.mainbox.listview.element.selected {
}
#window mainbox listview element selected {
}
```

### Name

The current widgets exist in **rofi**:

* `#window`
  * `#window.box`: The container holding the window.
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

### State

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


### Supported properties

The following properties are currently supports:

 * all widgets:
    * padding:         padding
      Padding on the inside of the widget.
    * margin:          padding
      Margin on the outside of the widget.
    * border:          border
      Border around the widget (between padding and margin)/
    * background:      color
      Background color.
    * foreground:      color
      Foreground color.
    * index:           integer  (This one does not inherits it value from the parent widget)

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
    * fullscreen:     boolean
      Window is fullscreen.
    * x-offset:  distance
    * y-offset:  distance
        The offset of the window to the anchor point.
        Allowing you to push the window left/right/up/down.


  * scrollbar
    * foreground:      color
    * handle-width:    distance
    * handle-color:    color
    * foreground:      color

  * box
    * spacing:         distance

  * textbox:
    * background:      color
    * foreground:      color
    * text:            The text color to use (falls back to foreground if not set)
    * highlight:        highlight {color}

  * listview:
    * columns:         integer
    * fixed-height:    boolean
    * dynamic:         boolean
    * scrollbar:       boolean
    * scrollbar-width: distance
    * cycle:           boolean
    * spacing:         distance


## Examples

### Arthur

A simple theme showing basic theming and transparent background:

```css
* {
    foreground:  #ffeedd;
    background:  rgba(0,0,0,0);
    dark: #1c1c1c;
    // Black
    black:       #3d352a;
    lightblack:  #554444;
    //
    // Red
    red:         #cd5c5c;
    lightred:    #cc5533;
    //
    // Green
    green:       #86af80;
    lightgreen:  #88cc22;
    //
    // Yellow
    yellow:      #e8ae5b;
    lightyellow:     #ffa75d;
    //
    // Blue
    blue:      #6495ed;
    lightblue:     #87ceeb;
    //
    // Magenta
    magenta:      #deb887;
    lightmagenta:     #996600;
    //
    // Cyan
    cyan:      #b0c4de;
    lightcyan:     #b0c4de;
    //
    // White
    white:      #bbaa99;
    lightwhite:     #ddccbb;
    //
    // Bold, Italic, Underline
    highlight:     bold #ffffff;
}
#window {
    location: center;
    anchor:   north;
    y-offset: -5em;
}
#window box {
    padding: 10px;
    border:  1px;
    foreground: @magenta;
    background: #cc1c1c1c;
}

#window mainbox inputbar {
    background: @lightblack;
    text: @lightgreen;
    padding: 4px;
}
#window mainbox inputbar box {
    border: 0px 0px 2px 0px;
}
#window mainbox box {
    spacing: 0.3em;
}

#window mainbox listview {
    dynamic: false;
}
#window mainbox listview element selected  normal {
    background: @blue;
}
#window mainbox listview element normal active {
    foreground: @lightblue;
}
#window mainbox listview element normal urgent {
    foreground: @lightred;
}
#window mainbox listview element alternate normal {
}
#window mainbox listview element alternate active {
    foreground: @lightblue;
}
#window mainbox listview element alternate urgent {
    foreground: @lightred;
}
#window mainbox listview element selected active {
    background: @lightblue;
    foreground: @dark;
}
#window mainbox listview element selected urgent {
    background: @lightred;
    foreground: @dark;
}
#window mainbox listview element normal normal {

}
```

### Sidebar

The previous theme modified to behave like a sidebar, positioned on the left of the screen.

```css
* {
    foreground:  #ffeedd;
    background:  rgba(0,0,0,0);
    dark: #1c1c1c;
    // Black
    black:       #3d352a;
    lightblack:  #554444;
    //
    // Red
    red:         #cd5c5c;
    lightred:    #cc5533;
    //
    // Green
    green:       #86af80;
    lightgreen:  #88cc22;
    //
    // Yellow
    yellow:      #e8ae5b;
    lightyellow:     #ffa75d;
    //
    // Blue
    blue:      #6495ed;
    lightblue:     #87ceeb;
    //
    // Magenta
    magenta:      #deb887;
    lightmagenta:     #996600;
    //
    // Cyan
    cyan:      #b0c4de;
    lightcyan:     #b0c4de;
    //
    // White
    white:      #bbaa99;
    lightwhite:     #ddccbb;
    //
    // Bold, Italic, Underline
    highlight:     bold #ffffff;
}
#window {
    width: 30em;
    location: west;
    anchor:   west;
}
#window box {
    border:  0px 2px 0px 0px;
    foreground: @lightwhite;
    background: #ee1c1c1c;
}

#window mainbox sidebar box {
    border: 2px 0px 0px 0px;
    background: @lightblack;
    padding: 10px;
}
#window mainbox sidebar selected {
    foreground: @lightgreen;
    text: @lightgreen;
}
#window mainbox inputbar {
    background: @lightblack;
    text: @lightgreen;
    padding: 4px;
}
#window mainbox inputbar box {
    border: 0px 0px 2px 0px;
}
#window mainbox box {
    spacing: 1em;
}
#window mainbox listview box {
    padding: 0em 0.4em 0em 1em;
}
#window mainbox listview {
    dynamic: false;
    lines: 0;
}
#window mainbox listview element selected  normal {
    background: @blue;
}
#window mainbox listview element normal active {
    foreground: @lightblue;
}
#window mainbox listview element normal urgent {
    foreground: @lightred;
}
#window mainbox listview element alternate normal {
}
#window mainbox listview element alternate active {
    foreground: @lightblue;
}
#window mainbox listview element alternate urgent {
    foreground: @lightred;
}
#window mainbox listview element selected active {
    background: @lightblue;
    foreground: @dark;
}
#window mainbox listview element selected urgent {
    background: @lightred;
    foreground: @dark;
}
#window mainbox listview element normal normal {

}

```

### Paper Float

A theme that shows a floating inputbar. 

> TODO: cleanup this theme.

```css

* {
    blue:  #0000FF;
    white: #FFFFFF;
    black: #000000;
    grey:  #eeeeee;

    spacing: 2;
    background: #00000000;
    anchor: north;
    location: center;
}
#window {
    transparency: "screenshot";
    background: #00000000;
    border: 0;
    padding: 0% 0% 1em 0%;
    foreground: #FF444444;
    x-offset: 0;
    y-offset: -10%;
}
#window.mainbox {
    padding: 0px;
}
#window.mainbox.box {
    border: 0;
    spacing: 1%;
}
#window.mainbox.message.box {
    border: 2px; 
    padding: 1em; 
    background: @white; 
    foreground: @back;
}
#window.mainbox.message.normal {
    foreground: #FF002B36;
    padding: 0;
    border: 0;
}
#window.mainbox.listview {
    fixed-height: 1;
    border: 2px;
    padding: 1em;
    reverse: false;

    columns: 1;
    background: @white; 
}
#window.mainbox.listview.element {
    border: 0;
    padding: 2px;
    highlight: bold ;
}
#window.mainbox.listview.element.normal.normal {
    foreground: #FF002B36;
    background: #00F5F5F5;
}
#window.mainbox.listview.element.normal.urgent {
    foreground: #FFD75F00;
    background: #FFF5F5F5;
}
#window.mainbox.listview.element.normal.active {
    foreground: #FF005F87;
    background: #FFF5F5F5;
}
#window.mainbox.listview.element.selected.normal {
    foreground: #FFF5F5F5;
    background: #FF4271AE;
}
#window.mainbox.listview.element.selected.urgent {
    foreground: #FFF5F5F5;
    background: #FFD75F00;
}
#window.mainbox.listview.element.selected.active {
    foreground: #FFF5F5F5;
    background: #FF005F87;
}
#window.mainbox.listview.element.alternate.normal {
    foreground: #FF002B36;
    background: #FFD0D0D0;
}
#window.mainbox.listview.element.alternate.urgent {
    foreground: #FFD75F00;
    background: #FFD0D0D0;
}
#window.mainbox.listview.element.alternate.active {
    foreground: #FF005F87;
    background: #FFD0D0D0;
}
#window.mainbox.listview.scrollbar {
    border: 0;
    padding: 0;
}
#window.mainbox.inputbar {
    spacing: 0;
}
#window.mainbox.inputbar.box {
    border: 2px;
    padding: 0.5em 1em;
    background: @grey;
    index: 0;
}
#window.mainbox.inputbar.normal {
    foreground: #FF002B36;
    background: #00F5F5F5;
}

#window.mainbox.sidebar.box {
    border: 2px;
    padding: 0.5em 1em;
    background: @grey;
    index: 10;
}
#window.mainbox.sidebar.button selected {
    text: #FF4271AE;
}
```
