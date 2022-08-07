# ROFI-THEME 5 rofi-theme

## NAME

**rofi-theme** - Rofi theme format files

## GETTING STARTED WITH THEMING 

The easiest way to get started theming rofi is by modifying your existing theme.

Themes can be modified/tweaked by adding theming elements to the end of the  
config file. The default location of this file is `~/.config/rofi/config.rasi`,
if the file does not exists, you can create it.

A basic config:

```css
configuration {
  modes: [ combi ];
  combi-modes: [ window, drun, run ];
}

@theme "gruvbox-light"
 
/* Insert theme modifications after this */
```


For example if we want to change the `Type to filter` text in the entry box we
append the following:

```css
entry {
    placeholder: "Type here";
}
```

In the above section, `entry` indicates the widget, `placeholder` is the
property we want to modify and we set it to the string `"Type here"`.
To find the commonly available widgets in rofi, see the 'Basic structure' section.

To change the mouse over cursor to a pointer, add:

```css
entry {
    placeholder: "Type here";
    cursor: pointer;
}
```

For the next modification, we want to add the icon after each text element and
increase the size. First we start by modifying the `element` widget:

```css

element {
  orientation: horizontal;
  children: [ element-text, element-icon ];
  spacing: 5px;
}

```

Resulting in the following packing:

```
┌─────────────────────────────────────────────────────────────────────┐ 
│ element                                                             │ 
│ ┌─────────────────────────────────────────────┐ ┌─────────────────┐ │ 
│ │element─text                                 │ │ element─icon    │ │ 
│ └─────────────────────────────────────────────┘ └─────────────────┘ │ 
└─────────────────────────────────────────────────────────────────────┘ 
```

The `element` (container) widget hold each entry in the `listview`, we add the
two pre-defined children in the order we want to show. We also specify the
packing direction (`orientation`) and the spacing between the children
(`spacing`). We specify the space between the two children in absolute pixels
(`px`).

To increase the icon-size, we need to modify the `element-icon` widget.

```css
element-icon {
    size: 2.5em;
}
```

```
┌─────────────────────────────────────────────────────────────────────┐ 
│ element                                                             │ 
│ ┌─────────────────────────────────────────────┐ ┌─────────────────┐ │ 
│ │element─text                                 │ │    element      │ │ 
│ │                                             │ │       ─         │ │ 
│ │                                             │ │     icon        │ │ 
│ └─────────────────────────────────────────────┘ └─────────────────┘ │ 
└─────────────────────────────────────────────────────────────────────┘ 
```

In this example we specify the size in the [em](https://www.w3.org/Style/LieBos3e/em) unit.

Now lets change the text color of both the `entry` and the `element-text` widget to red and background to blue.

```css
entry, element-text {
  text-color: red;
  background-color: rgb(0,0,255);
}
```

Here we use two different methods of writing down the color, for `text-color`
we used a named color, for `background-color` we specify it in `rgb`.
We also specify the property for multiple widgets by passing a comma separated
list of widget names.

If you want to center the text relative to the icon, we can set this:

```css
element-text {
    vertical-align: 0.5;
}
```

```
┌─────────────────────────────────────────────────────────────────────┐ 
│ element                                                             │ 
│ ┌─────────────────────────────────────────────┐ ┌─────────────────┐ │ 
│ │                                             │ │    element      │ │ 
│ │element-text                                 │ │       ─         │ │ 
│ │                                             │ │     icon        │ │ 
│ └─────────────────────────────────────────────┘ └─────────────────┘ │ 
└─────────────────────────────────────────────────────────────────────┘ 
```

If you want to see the complete theme, including the modification you can run:

```bash
rofi -dump-theme
```

## DEFAULT THEME LOADING

By default, rofi loads the default theme. This theme is **always** loaded.
The default configuration contains:

```css
@theme "default"
```

To unload the default theme, and load another theme, add the `@theme` statement 
to your `config.rasi` file.

If you have a theme loaded via `@theme` or use the default theme, you can tweak
it by adding overriding elements at the end of your `config.rasi` file.

For the difference between `@import` and `@theme` see the `Multiple file
handling` section in this manpage.

To see the default theme, run the following command:

```bash
rofi -no-config -dump-theme
```

## DESCRIPTION

The need for a new theme format was motivated by the fact that the way rofi handled widgets has changed. From a very
static drawing of lines and text to a nice structured form of packing widgets. This change made it possible to provide a
more flexible theme framework. The old theme format and config file are not flexible enough to expose these options in a
user-friendly way. Therefore, a new file format has been created, replacing the old one.

## FORMAT SPECIFICATION

## Encoding

The encoding of the file is UTF-8. Both unix (`\n`) and windows (`\r\n`) newlines format are supported. But unix is
preferred.

## Comments

C and C++ file comments are supported.

* Anything after  `// ` and before a newline is considered a comment.
* Everything between `/*` and `*/` is a comment, this comment can span multiple lines.

Comments can be nested and the C comments can be inline.

The following is valid:

```css
// Magic comment.
property: /* comment */ value;
```

However, this is not:

```css
prop/*comment*/erty: value;
```

## White space

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

## File extension

The preferred file extension for the new theme format is **rasi**. This is an
abbreviation for **r**ofi **a**dvanced **s**tyle **i**nformation.

## Basic Structure

Each element has a section with defined properties. Global properties can be defined in section `* { }`.
Sub-section names begin with an optional hash symbol `#`.

It is advised to define the *global properties section* on top of the file to
make inheritance of properties clearer.

```css
/* Global properties section */
* {
    // list of properties
}

/* Element theme section. */
{element path} {
    // list of properties
}
{elements... } {
    // list of properties
}
```

If there are multiple sections with the same name, they are merged. Duplicate properties are overwritten and the last
parsed entry kept.

## Global properties section

A theme can have one or more global properties sections. If there is more than one,
they will be merged.

The global properties section denotes the defaults for each element.
Each property of this section can be referenced with `@{identifier}`
(See Properties section)

A global properties section is indicated with a `*` as element path.

## Element theme section

A theme can have multiple element theme sections.

The element path can consist of multiple names separated by whitespace or dots.
Each element may contain any number of letters, numbers and `-`'s.
The first element in the element path can optionally start with a `#` (for
historic reasons). Multiple elements can be specified by a `,`.

This is a valid element name:

```css
element normal.normal {
    background-color: blue;
}
button {
    background-color: blue;
}
```

And is identical to:

```css
element normal normal, button {
    background-color: blue;
}
```

Each section inherits the global properties. Properties can be explicitly
inherited from their parent with the `inherit` keyword.
In the following example:

```css
window {
 a: 1;
 b: 2;
 children: [ mainbox ];
}
mainbox {
    a: inherit;
    b: 4;
    c: 8;
}
```

The element `mainbox` will have the following set of properties (if `mainbox`
is a child of `window`):

```css
a: 1;
b: 4;
c: 8;
```

If multiple sections are defined with the same name, they are merged by the
parser. If multiple properties with the same name are defined in one section,
the last encountered property is used.

## Properties Format

The properties in a section consist of:

```css
{identifier}: {value};
```

Both fields are mandatory for a property.

The `identifier` names the specified property. Identifiers can consist of any
combination of numbers, letters and '-'. It must not contain any whitespace.
The structure of the `value` defines the type of the property. The current
parser does not define or enforce a certain type of a particular `identifier`.
When used, values with the wrong type that cannot be converted are ignored.

The current theme format supports different types:

 * a string
 * an integer number
 * a fractional number
 * a boolean value
 * a color
 * image
 * text style
 * line style
 * a distance
 * a padding
 * a border
 * a position
 * a reference
 * an orientation
 * a cursor
 * a list of keywords
 * an array of values
 * an environment variable
 * Inherit

Some of these types are a combination of other types.

## String

* Format:  `"[:print:]+"`

A string is always surrounded by double quotes (`"`). Between the quotes there can be any printable character.

For example:

```css
font: "Awasome 12";
```

The string must be valid UTF-8, special characters can be escaped:

```css
text {
    content: "Line one\n\tIndented line two";
}
```

The following special characters can be escaped: `\b`, `\f`, `\n`, `\r`, `\t`, `\v`, `\` and `"`.

## Integer

* Format: `[-+]?[:digit:]+`

An integer may contain any number.

For examples:

```css
lines: 12;
```

## Real

* Format: `[-+]?[:digit:]+(\.[:digit:]+)?`

A real is an integer with an optional fraction.

For example:

```css
real: 3.4;
```

The following is not valid: `.3`, `3.` or scientific notation: `3.4e-3`.

## Boolean

* Format: `(true|false)`

Boolean value is either `true` or `false`. This is case-sensitive.


For example:

```css
dynamic: false;
```

## Image

**rofi** support a limited set of background-image formats.

* Format: url("path to image");
* Format: url("path to image", scale);
  where scale is: none, both, width, height
* Format: linear-gradient(stop color,stop1, color, stop2 color, ...);
* Format: linear-gradient(to direction, stop color,stop1, color, stop2 color, ...);
  where direction is:   top,left,right,bottom.
* Format: linear-gradient(angle, stop color,stop1, color, stop2 color, ...);
  Angle in deg,rad,grad (as used in color).

Where the `path` is a string, and `stop` color is of type color.

## Color

**rofi** supports the color formats as specified in the CSS standard (1,2,3 and some of CSS 4)

* Format: `#{HEX}{3}` (rgb)
* Format: `#{HEX}{4}` (rgba)
* Format: `#{HEX}{6}` (rrggbb)
* Format: `#{HEX}{8}` (rrggbbaa)
* Format: `rgb[a]({INTEGER},{INTEGER},{INTEGER}[, {PERCENTAGE}])`
* Format: `rgb[a]({INTEGER}%,{INTEGER}%,{INTEGER}%[, {PERCENTAGE}])`
* Format: `hsl[a]( {ANGLE}, {PERCENTAGE}, {PERCENTAGE} [, {PERCENTAGE}])`
* Format: `hwb[a]( {ANGLE}, {PERCENTAGE}, {PERCENTAGE} [, {PERCENTAGE}])`
* Format: `cmyk( {PERCENTAGE}, {PERCENTAGE}, {PERCENTAGE}, {PERCENTAGE} [, {PERCENTAGE} ])`
* Format: `{named-color} [ / {PERCENTAGE} ]`

The white-space format proposed in CSS4 is also supported.

The different values are:

 * `{HEX}` is a hexadecimal number ('0-9a-f' case insensitive).
 * `{INTEGER}` value can be between 0 and 255 or 0-100 when representing percentage.
 * `{ANGLE}` is the angle on the color wheel, can be in `deg`, `rad`, `grad` or `turn`. When no unit is specified, degrees is assumed.
 * `{PERCENTAGE}` can be between 0-1.0, or 0%-100%
 * `{named-color}` is one of the following colors:

    AliceBlue, AntiqueWhite, Aqua, Aquamarine, Azure, Beige, Bisque, Black, BlanchedAlmond, Blue, BlueViolet, Brown,
    BurlyWood, CadetBlue, Chartreuse, Chocolate, Coral, CornflowerBlue, Cornsilk, Crimson, Cyan, DarkBlue, DarkCyan,
    DarkGoldenRod, DarkGray, DarkGrey, DarkGreen, DarkKhaki, DarkMagenta, DarkOliveGreen, DarkOrange, DarkOrchid, DarkRed,
    DarkSalmon, DarkSeaGreen, DarkSlateBlue, DarkSlateGray, DarkSlateGrey, DarkTurquoise, DarkViolet, DeepPink, DeepSkyBlue,
    DimGray, DimGrey, DodgerBlue, FireBrick, FloralWhite, ForestGreen, Fuchsia, Gainsboro, GhostWhite, Gold, GoldenRod,
    Gray, Grey, Green, GreenYellow, HoneyDew, HotPink, IndianRed, Indigo, Ivory, Khaki, Lavender, LavenderBlush, LawnGreen,
    LemonChiffon, LightBlue, LightCoral, LightCyan, LightGoldenRodYellow, LightGray, LightGrey, LightGreen, LightPink,
    LightSalmon, LightSeaGreen, LightSkyBlue, LightSlateGray, LightSlateGrey, LightSteelBlue, LightYellow, Lime, LimeGreen,
    Linen, Magenta, Maroon, MediumAquaMarine, MediumBlue, MediumOrchid, MediumPurple, MediumSeaGreen, MediumSlateBlue,
    MediumSpringGreen, MediumTurquoise, MediumVioletRed, MidnightBlue, MintCream, MistyRose, Moccasin, NavajoWhite, Navy,
    OldLace, Olive, OliveDrab, Orange, OrangeRed, Orchid, PaleGoldenRod, PaleGreen, PaleTurquoise, PaleVioletRed,
    PapayaWhip, PeachPuff, Peru, Pink, Plum, PowderBlue, Purple, RebeccaPurple, Red, RosyBrown, RoyalBlue, SaddleBrown,
    Salmon, SandyBrown, SeaGreen, SeaShell, Sienna, Silver, SkyBlue, SlateBlue, SlateGray, SlateGrey, Snow, SpringGreen,
    SteelBlue, Tan, Teal, Thistle, Tomato, Turquoise, Violet, Wheat, White, WhiteSmoke, Yellow, YellowGreen,transparent



For example:

```css
background-color: #FF0000;
border-color: rgba(0,0,1, 0.5);
text-color: SeaGreen;
```
or

```css
background-color: transparent;
text-color: Black;
```

## Text style

* Format: `(bold|italic|underline|strikethrough|none)`

Text style indicates how the highlighted text is emphasized. `None` indicates that no emphasis
should be applied.

 * `bold`: make the text thicker then the surrounding text.
 * `italic`: put the highlighted text in script type (slanted).
 * `underline`: put a line under the text.
 * `strikethrough`: put a line through the text.

The following options are available on pango 1.50.0 and up:

 * `uppercase`: Uppercase the text.
 * `lowercase`: Lowercase the text.

 The following option is disabled as pango crashes on this if there is eel
 upsizing or wrapping. This will be re-enabled once fixed:

 * `capitalize`: Capitalize the text.

## Line style

* Format: `(dash|solid)`

Indicates how a line should be drawn.
It currently supports:
 * `dash`:  a dashed line, where the gap is the same width as the dash
 * `solid`: a solid line

## Distance

* Format: `{Integer}px`
* Format: `{Real}em`
* Format: `{Real}ch`
* Format: `{Real}%`
* Format: `{Integer}mm`

A distance can be specified in 3 different units:

* `px`: Screen pixels.
* `em`: Relative to text height.
* `ch`: Relative to width of a single number.
* `mm`: Actual size in millimeters (based on dpi).
* `%`:  Percentage of the **monitor** size.

Distances used in the horizontal direction use the monitor width. Distances in
the vertical direction use the monitor height.
For example:

```css
   padding: 10%;
```
On a full-HD (1920x1080) monitor, it defines a padding of 192 pixels on the left
and right side and 108 pixels on the top and bottom.

### Calculating sizes

Rofi supports some maths in calculating sizes. For this it uses the CSS syntax:

```css
width: calc( 100% - 37px );
```

```css
width: calc( 20% min 512 );
```

It supports the following operations:

* `+`     : Add
* `-`     : Subtract
* `/`     : Divide
* `*`     : Multiply
* `%`     : Modulo
* `min`   : Minimum of lvalue or rvalue;
* `max`   : Maximum of lvalue or rvalue;
* `floor` : Round down lvalue to the next multiple of rvalue 
* `ceil`  : Round up lvalue to the next multiple of rvalue 
* `round` : Round lvalue to the next multiple of rvalue 

It uses the C precedence ordering.

## Padding

* Format: `{Integer}`
* Format: `{Distance}`
* Format: `{Distance} {Distance}`
* Format: `{Distance} {Distance} {Distance}`
* Format: `{Distance} {Distance} {Distance} {Distance}`

If no unit is specified, pixels are assumed.

The different number of fields in the formats are parsed like:

* 1 field: `all`
* 2 fields: `top&bottom` `left&right`
* 3 fields: `top`, `left&right`, `bottom`
* 4 fields: `top`, `right`, `bottom`, `left`


## Border

* Format: `{Integer}`
* Format: `{Distance}`
* Format: `{Distance} {Distance}`
* Format: `{Distance} {Distance} {Distance}`
* Format: `{Distance} {Distance} {Distance} {Distance}`
* Format: `{Distance} {Line style}`
* Format: `{Distance} {Line style} {Distance} {Line style}`
* Format: `{Distance} {Line style} {Distance} {Line style} {Distance} {Line style}`
* Format: `{Distance} {Line style} {Distance} {Line style} {Distance} {Line style} {Distance} {Line style}`

Borders are identical to padding, except that each distance field has a line
style property.

> When no unit is specified, pixels are assumed.


## Position

Indicate a place on the window/monitor.

```
┌─────────────┬─────────────┬─────────────┐
│ north west  │    north    │  north east │
├─────────────┼─────────────┼─────────────┤
│   west      │   center    │     east    │
├─────────────┼─────────────┼─────────────┤
│ south west  │    south    │  south east │
└─────────────┴─────────────┴─────────────┘
```

* Format: `(center|east|north|west|south|north east|north west|south west|south east)`

## Visibility

It is possible to hide widgets:

```css
inputbar {
    enabled: false;
}
```


## Reference

* Format: `@{PROPERTY NAME}`

A reference can point to another reference. Currently, the maximum number of redirects is 20.
A property always refers to another property. It cannot be used for a subpart of the property.
For example, this is not valid:

```css
highlight: bold @pink;
```

But this is:

```css
* {
    myhigh: bold #FAA;
}

window {
    highlight: @myhigh;
}
```

* Format: `var(PROPERTY NAME, DEFAULT)`

A reference can point to another reference. Currently, the maximum number of redirects is 20.
A property always refers to another property. It cannot be used for a subpart of the property.

Example:

```css
window {
    width: var( width, 30%);
}
```

If the property `width` is set globally (`*{}`) that value is used, if the property
`width` is not set, the default value is used.


## Orientation

 * Format: `(horizontal|vertical)`

Specify the orientation of the widget.

## Cursor

 * Format: `(default|pointer|text)`

Specify the type of mouse cursor that is set when the mouse pointer is over the widget.

## List of keywords

* Format: `[ keyword, keyword ]`

A list starts with a '[' and ends with a ']'. The entries in the list are comma-separated.
The `keyword` in the list refers to an widget name.

## List of values

* Format: `[ value, value, ... ]`

An list starts with a '[' and ends with a ']'. The entries in the list are comma-separated.

## Environment variable

* Format: `${:alnum:}`

This will parse the environment variable as the property value. (that then can be any of the above types).
The environment variable should be an alphanumeric string without white-space.

```css
* {
    background-color: ${BG};
}
```

* Format: `env(ENVIRONMENT, default)`

This will parse the environment variable as the property value. (that then can be any of the above types).
The environment variable should be an alphanumeric string without white-space.
If the environment value is not found, the default value is used.

```css
window {
    width: env(WIDTH, 40%);
}
```

If environment WIDTH is set, then that value is parsed, otherwise the default value (`40%`).

## Inherit

 * Format: `inherit`

Inherits the property from its parent widget.

```css
mainbox {
    border-color: inherit;
}
```


## ELEMENTS PATHS

Element paths exists of two parts, the first part refers to the actual widget by name.
Some widgets have an extra state.

For example:

```css
element selected {
}
```

Here `element selected` is the name of the widget, `selected` is the state of the widget.

The difference between dots and spaces is purely cosmetic. These are all the same:

```css
element .selected {

element.selected {
}
element selected {
}
```

## SUPPORTED ELEMENT PATH

## Name

The current widgets available in **rofi**:

* `window`
  * `overlay`: the overlay widget.
  * `mainbox`: The mainbox box.
    * `inputbar`: The input bar box.
      * `box`: the horizontal @box packing the widgets
      * `case-indicator`: the case/sort indicator @textbox
      * `prompt`: the prompt @textbox
      * `entry`: the main entry @textbox
      * `num-rows`: Shows the total number of rows.
      * `num-filtered-rows`: Shows the total number of rows after filtering.
      * `textbox-current-entry`: Shows the text of the currently selected entry.
      * `icon-current-entry`: Shows the icon of the currently selected entry.
    * `listview`: The listview.
       * `scrollbar`: the listview scrollbar
       * `element`: a box in the listview holding the entries
           * `element-icon`: the widget in the listview's entry showing the (optional) icon
           * `element-index`: the widget in the listview's entry keybindable index (1,2,3..0)
           * `element-text`: the widget in the listview's entry showing the text.
    * `mode-switcher`: the main horizontal @box packing the buttons.
      * `button`: the buttons @textbox for each mode
    * `message`: The container holding the textbox.
      * `textbox`: the message textbox

Note that these path names match the default theme. Themes that provide a custom layout will have different
elements, and structure.


## State

State: State of widget

Optional flag(s) indicating state of the widget, used for theming.

These are appended after the name or class of the widget.

### Example:

`button selected.normal { }`

`element selected.urgent { }`

Currently only the entrybox and scrollbar have states:

### Entrybox:

`{visible modifier}.{state}`

Where `visible modifier` can be:
 * normal: no modification
 * selected: the entry is selected/highlighted by user
 * alternate: the entry is at an alternating row (uneven row)

Where `state` is:
 * normal: no modification
 * urgent: this entry is marked urgent
 * active: this entry is marked active

These can be mixed.

Example:

```css
nametotextbox selected.active {
    background-color: #003642;
    text-color: #008ed4;
}
```

Sets all selected textboxes marked active to the given text and background color.
Note that a state modifies the original element, it therefore contains all the properties of that element.

### Scrollbar

The scrollbar uses the `handle` state when drawing the small scrollbar handle.
This allows the colors used for drawing the handle to be set independently.


## SUPPORTED PROPERTIES

The following properties are currently supported:

###  all widgets:

* **enabled**:           enable/disable rendering of the widget
* **padding**:           padding
  Padding on the inside of the widget
* **margin**:            padding
  Margin on the outside of the widget
* **border**:            border
  Border around the widget (between padding and margin)/
* **border-radius**:     padding
  Sets a radius on the corners of the borders.
* **background-color**:  color
  Background color
* **background-image**:  image
  Background image
* **border-color**:      color
  Color of the border
* **cursor**:            cursor
  Type of mouse cursor that is set when the mouse pointer is hovered over the widget.

### window:

* **font**:            string
  The font used in the window

* **transparency**:    string
  Indicating if transparency should be used and what type:
  **real** - True transparency. Only works with a compositor.
  **background** - Take a screenshot of the background image and use that.
  **screenshot** - Take a screenshot of the screen and use that.
  **Path** to png file - Use an image.

* **location**:       position
    The place of the anchor on the monitor
* **anchor**:         anchor
    The anchor position on the window
* **fullscreen**:     boolean
    Window is fullscreen.
* **width**:          distance
    The width of the window
* **x-offset**:       distance
* **y-offset**:       distance
    The offset of the window to the anchor point, allowing you to push the window left/right/up/down


### scrollbar:

* **background-color**:    color
* **handle-width**:        distance
* **handle-color**:        color
* **border-color**:        color

### box:

* **orientation**:      orientation
        Set the direction the elements are packed.
* **spacing**:          distance
        Distance between the packed elements.

### textbox:

* **background-color**:  color
* **border-color**:      the color used for the border around the widget.
* **font**:              the font used by this textbox (string).
* **str**/**content**:   the string to display by this textbox (string).
* **vertical-align**:    Vertical alignment of the text. A number between 0 (top) and 1 (bottom).
* **horizontal-align**:  Horizontal alignment of the text. A number between 0 (left) and 1 (right).
* **text-color**:        the text color to use.
* **text-transform**:    text style {color} for the whole text.
* **highlight**:         text style {color}.
    color is optional, multiple highlight styles can be added like: bold underline italic #000000;
    This option is only available on the `element-text` widget.
* **width**:             override the desired width for the textbox.
* **content**:           Set the displayed text (String).
* **placeholder**:       Set the displayed text (String) when nothing is entered.
* **placeholder-color**: Color of the placeholder text.
* **blink**:             Enable/Disable blinking on an input textbox (Boolean).
* **markup**:            Force markup on, beware that only valid pango markup strings are shown.
* **tab-stops**:         array of distances
    Set the location of tab stops by their distance from the beginning of the line.
    Each distance should be greater than the previous one.
    The text appears to the right of the tab stop position (other alignments are not supported yet).

### listview:
* **columns**:         integer
    Number of columns to show (at least 1)
* **fixed-height**:    boolean
    Always show `lines` rows, even if fewer elements are available.
* **dynamic**:         boolean
    `True` if the size should change when filtering the list, `False` if it should keep the original height.
* **scrollbar**:       boolean
    If the scrollbar should be enabled/disabled.
* **scrollbar-width**: distance
    Width of the scrollbar
* **cycle**:           boolean
    When navigating, it should wrap around
* **spacing**:         distance
    Spacing between the elements (both vertical and horizontal)
* **lines**:           integer
    Number of rows to show in the list view.
* **layout**:           orientation
    Indicate how elements are stacked. Horizontal implements the dmenu style.
* **reverse**:         boolean
    Reverse the ordering (top down to bottom up).
* **flow**:           orientation
    The order the elements are layed out.  Vertical is the original 'column' view.
* **fixed-columns**:    boolean
    Do not reduce the number of columns shown when number of visible elements is not enough to fill them all.
* **require-input**:    boolean
    Listview requires user input to show up.

Each element is a `box` called `element`. Each `element` can contain an `element-icon` and `element-text`.

### listview text highlight:

The `element-text` widget in the `listview` is the one used to show the text.
On this widget set the `highlight` property (only place this property is used) to change
the style of highlighting.
The `highlight` property consist of the `text-style` property and a color.

To disable highlighting:

```css
  element-text {
    highlight: None;
  }
```

To set to red underlined:

```css
  element-text {
    highlight: underline red;
  }
```

## Layout

The new format allows the layout of the **rofi** window to be tweaked extensively.
For each widget, the themer can specify padding, margin, border, font, and more.
It even allows, as an advanced feature, to pack widgets in a custom structure.

### Basic structure

The whole view is made out of boxes that pack other boxes or widgets.
The box can be vertical or horizontal. This is loosely inspired by [GTK](http://gtk.org/).

The current layout of **rofi** is structured as follows:

```
┌────────────────────────────────────────────────────────────────────────────────────┐
│ window {BOX:vertical}                                                              │
│ ┌───────────────────────────────────────────────────────────────────────────────┐  │
│ │ mainbox  {BOX:vertical}                                                       │  │
│ │ ┌───────────────────────────────────────────────────────────────────────────┐ │  │
│ │ │ inputbar {BOX:horizontal}                                                 │ │  │
│ │ │ ┌─────────┐ ┌─┐ ┌───────────────────────────────┐ ┌───┐ ┌───┐ ┌───┐ ┌───┐ │ │  │
│ │ │ │ prompt  │ │:│ │ entry                         │ │#fr│ │ / │ │#ns│ │ci │ │ │  │
│ │ │ └─────────┘ └─┘ └───────────────────────────────┘ └───┘ └───┘ └───┘ └───┘ │ │  │
│ │ └───────────────────────────────────────────────────────────────────────────┘ │  │
│ │                                                                               │  │
│ │ ┌───────────────────────────────────────────────────────────────────────────┐ │  │
│ │ │ message                                                                   │ │  │
│ │ │ ┌───────────────────────────────────────────────────────────────────────┐ │ │  │
│ │ │ │ textbox                                                               │ │ │  │
│ │ │ └───────────────────────────────────────────────────────────────────────┘ │ │  │
│ │ └───────────────────────────────────────────────────────────────────────────┘ │  │
│ │                                                                               │  │
│ │ ┌───────────────────────────────────────────────────────────────────────────┐ │  │
│ │ │ listview                                                                  │ │  │
│ │ │ ┌─────────────────────────────────────────────────────────────────────┐   │ │  │
│ │ │ │ element                                                             │   │ │  │
│ │ │ │ ┌─────────────────┐ ┌─────────────────────────────────────────────┐ │   │ │  │
│ │ │ │ │element─icon     │ │element─text                                 │ │   │ │  │
│ │ │ │ └─────────────────┘ └─────────────────────────────────────────────┘ │   │ │  │
│ │ │ └─────────────────────────────────────────────────────────────────────┘   │ │  │
│ │ └───────────────────────────────────────────────────────────────────────────┘ │  │
│ │                                                                               │  │
│ │ ┌───────────────────────────────────────────────────────────────────────────┐ │  │
│ │ │  mode─switcher {BOX:horizontal}                                           │ │  │
│ │ │ ┌───────────────┐   ┌───────────────┐  ┌──────────────┐ ┌───────────────┐ │ │  │
│ │ │ │ Button        │   │ Button        │  │ Button       │ │ Button        │ │ │  │
│ │ │ └───────────────┘   └───────────────┘  └──────────────┘ └───────────────┘ │ │  │
│ │ └───────────────────────────────────────────────────────────────────────────┘ │  │
│ └───────────────────────────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────────────────────────┘


```
> * ci is the case-indicator
> * fr is the num-filtered-rows
> * ns is the num-rows

### Error message structure

```
┌──────────────────────────────────────────────────────────────────────────────────┐
│ window {BOX:vertical}                                                            │
│ ┌─────────────────────────────────────────────────────────────────────────────┐  │
│ │ error─message {BOX:vertical}                                                │  │
│ │ ┌────────────────────────────────────────────────────────────────────────┐  │  │
│ │ │ textbox                                                                │  │  │
│ │ └────────────────────────────────────────────────────────────────────────┘  │  │
│ └─────────────────────────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────────────────────────┘

```

### Advanced layout

The layout of **rofi** can be tweaked by packing the 'fixed' widgets in a custom structure.

The following widgets are fixed, as they provide core **rofi** functionality:

 * prompt
 * entry
 * overlay
 * case-indicator
 * message
 * listview
 * mode-switcher
 * num-rows
 * num-filtered-rows

The following keywords are defined and can be used to automatically pack a subset of the widgets.
These are used in the default theme as depicted in the figure above.

 * mainbox
   Packs: `inputbar, message, listview, mode-switcher`
 * inputbar
   Packs: `prompt,entry,case-indicator`

Any widget name starting with `textbox` is a textbox widget, others are box widgets and can pack other widgets.

There are several special widgets that can be used by prefixing the name of the widget:

#### textbox

This is a read-only textbox widget. The displayed string can be set with `content`.


Example:

```css
textbox-custom {
  expand: false;
  content: "My Message";
}
```

#### Icon

This is an icon widget. The displayed icon can be set with `filename` and size with `size`.
If the property `action` is set, it acts as a button.
`action` can be set to a keybinding name and completes that action. (see rofi -show keys for a list).

If the `squared` property is set to **false** the widget height and width are not forced to be equal.

Example:

```css
icon-paste {
    expand: false;
    filename: "gtk-paste";
    size: 24;
    vertical-align: 0.5;
    action: "kb-primary-paste";
}
```


#### button

This is a textbox widget that can have a 'clickable' action.
The `action` can be set to:
`keybinding`: accepts a keybinding name and completes that action. (see rofi -show keys for a list).

```css
button-paste {
    expand: false;
    content: "My Clickable Message";
    vertical-align: 0.5;
    action: "kb-primary-paste";
}
```


#### Children

To specify children, set the `children`
property (this always happens on the `box` child, see example below):

```css
inputbar {
  children: [prompt,entry,overlay,case-indicator];
}
```

The theme needs to be updated to match the hierarchy specified.

Below is an example of a theme emulating dmenu:

```css
* {
    background-color:      Black;
    text-color:            White;
    border-color:          White;
    font:            "Times New Roman 12";
}

window {
    anchor:     north;
    location:   north;
    width:      100%;
    padding:    4px;
    children:   [ horibox ];
}

horibox {
    orientation: horizontal;
    children:   [ prompt, entry, listview ];
}

listview {
    layout:     horizontal;
    spacing:    5px;
    lines:      10;
}

entry {
    expand:     false;
    width:      10em;
}

element {
    padding: 0px 2px;
}
element selected {
    background-color: SteelBlue;
}
```


### Padding and margin

Just like CSS, **rofi** uses the box model for each widget.

```
┌──────────────────────────────────────────────────────────────────┐
│ margin                                                           │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │ border                                                     │  │
│  │ ┌────────────────────────────────────────────────────────┐ │  │
│  │ │ padding                                                │ │  │
│  │ │ ┌────────────────────────────────────────────────────┐ │ │  │
│  │ │ │ content                                            │ │ │  │
│  │ │ └────────────────────────────────────────────────────┘ │ │  │
│  │ └────────────────────────────────────────────────────────┘ │  │
│  └────────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────────┘
```

Explanation of the different parts:

 * Content - The content of the widget.
 * Padding - Clears an area around the widget.
   The padding shows the background color of the widget.
 * Border - A border that goes around the padding and content.
   The border use the border-color of the widget.
 * Margin - Clears an area outside the border.
   The margin is transparent.

The box model allows us to add a border around elements, and to define space between elements.

The size of each margin, border, and padding can be set.
For the border, a linestyle and radius can be set.

### Spacing

Widgets that can pack more then one child widget (currently box and listview) have the `spacing` property.
This property sets the distance between the packed widgets (both horizontally and vertically).

```
┌───────────────────────────────────────┐
│ ┌────────┐ s ┌────────┐ s ┌────────┐  │
│ │ child  │ p │ child  │ p │ child  │  │
│ │        │ a │        │ a │        │  │
│ │        │ c │        │ c │        │  │
│ │        │ i │        │ i │        │  │
│ │        │ n │        │ n │        │  │
│ └────────┘ g └────────┘ g └────────┘  │
└───────────────────────────────────────┘
```

### Advanced box packing

More dynamic spacing can be achieved by adding dummy widgets, for example to make one widget centered:

```
┌────────────────────────────────────────────────────┐
│  ┌───────────────┐  ┌────────┐  ┌───────────────┐  │
│  │ dummy         │  │ child  │  │ dummy         │  │
│  │ expand: true; │  │        │  │ expand: true; │  │
│  │               │  │        │  │               │  │
│  │               │  │        │  │               │  │
│  │               │  │        │  │               │  │
│  └───────────────┘  └────────┘  └───────────────┘  │
└────────────────────────────────────────────────────┘
```

If both dummy widgets are set to expand, `child` will be centered. Depending on the `expand` flag of child the
remaining space will be equally divided between both dummy and child widget (expand enabled), or both dummy widgets
(expand disabled).

## DEBUGGING

To get debug information from the parser, run rofi like:

```
G_MESSAGES_DEBUG=Parser rofi -show run
```

Syntax errors are shown in a popup and printed out to command line with the above command.

To see the elements queried during running, run:

```
G_MESSAGES_DEBUG=Theme rofi -show run
```

To test minor changes, part of the theme can be passed on the command line, for example to set it to full-screen:

```
rofi -theme-str 'window { fullscreen:true;}' -show run
```

Another syntax to modify theme properties is:

```bash
rofi -theme+window+fullscreen true -show run
```

To print the current theme, run:

```
rofi -dump-theme
```

## Media support

Parts of the theme can be conditionally loaded, like the CSS `@media` option.

```
@media ( min-width: 120 ) {

}
```

It supports the following keys as constraint:

 * `min-width`:         load when width is bigger or equal then value.
 * `max-width`:         load when width is smaller then value.
 * `min-height`:        load when height is bigger or equal then value.
 * `max-height`:        load when height is smaller then value.
 * `min-aspect-ratio`   load when aspect ratio is over value.
 * `max-aspect-ratio`:  load when aspect ratio is under value.
 * `monitor-id`:        The monitor id, see rofi -help for id's.
 * `enabled`:           Boolean option to enable. Supports environment variable.

@media takes an integer number or a fraction, for integer number `px` can be added.


```
@media ( min-width: 120 px ) {

}
```

```
@media ( enabled: env(DO_LIGHT, false ) {

}
```


## Font Parsing

Rofi uses [pango](https://pango.gnome.org/) for font rendering. The font should be specified in a format that pango
understands.
This normally is the font name followed by the font size. For example:

```
mono 18
```

Or

```
FontAwesome 22
```

## Icon Handling 

Rofi supports 3 ways of specifying an icon:

* Filename
* icon-name, this is looked up via the icon-theme.
* Markup String. It renders a string as an icon.


For the first two options, GdkPixbuf is used to open and render the icons.
This in general gives support for most required image formats.
For the string option it uses Pango to render the string. The string needs to
start with a `<span` tag, that allows you to set color and font.

Markup string:

```bash
echo -en "testing\0icon\x1f<span color='red'>⏻</span>" | ./rofi -dmenu
```

Getting supported icon formats:

```bash
G_MESSAGES_DEBUG=Helpers.IconFetcher rofi
```
This uses the debug framework and prints out a list of supported image  file
extensions.

## Multiple file handling

The rasi file format offers two methods of including other files.
This can be used to modify existing themes, or have multiple variations on a theme.

 * import:  Import and parse a second file.
 * theme:   Discard theme, and load file as a fresh theme.

Syntax:

```
@import "myfile"
@theme "mytheme"
```

The specified file can either by *name*, *filename*,*full path*.

If a filename is provided, it will try to resolve it in the following order:

 * `${XDG_CONFIG_HOME}/rofi/themes/`
 * `${XDG_CONFIG_HOME}/rofi/`
 * `${XDG_DATA_HOME}/rofi/themes/`
 * `${INSTALL PREFIX}/share/rofi/themes/`

A name is resolved as a filename by appending the `.rasi` extension.



## EXAMPLES

Several examples are installed together with **rofi**. These can be found in `{datadir}/rofi/themes/`, where
`{datadir}` is the install path of **rofi** data. When installed using a package manager, this is usually: `/usr/share/`.

## SEE ALSO

rofi(1), rofi-script(5), rofi-theme-selector(1)
