# Basic Organization 

Each widget has:

## Class

Class: Type of widget. 

Example: textbox, scrollbar, separator

Class are prefixed with a `@`


List of classes in **rofi**:

 * @textbox
 * @scrollbar
 * @window
 * @separator
 * @listview
 * @box

## Name

Name: Internal name of the widget.

Sub-widgets are {Parent}.{Child}.

Example: listview, listview.element, listview.scrollbar

Names are prefixed with a `#`

List of names in **rofi**:

 * #window
 * #mainbox
   * #mainbox.box: The main vertical @box
 * #inputbar
   * #inputbar.box: The horizontal @box packing the widgets.
   * #inputbar.separator: The separator under/above the inputbar.
   * #inputbar.case-indicator: The case/sort indicator @textbox
   * #inputbar.prompt: The prompt @textbox
   * #inputbar.entry: The main entry @textbox
 * #listview
    * #listview.scrollbar: The listview scrollbar
    * #listview.element: The entries in the listview
 * #sidebar
   * #sidebar.box: The main horizontal @box packing the buttons.
   * #sidebar.button: The buttons @textbox for each mode.
   * #sidebar.separator: The separator under/above the sidebar.
 * #message
   * #message.textbox: The message textbox.
   * #message.separator: The separator under/above the sidebar.

## State

State: State of widget

Optional flag(s) indicating state. 

These are appended after the name or class of the widget.

`@textbox selected.normal { }` 

`#listview.element selected.urgent { }` 

Currently only the @entrybox has states:

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
@textbox selected.active { 
    background: #003642;
    foreground: #008ed4;
}
```

Sets all selected textboxes marked active to the given foreground and background color.

# File structure

The file is structured as follows

```
/* Global properties, that apply as default to all widgets. */
{list of properties}

@{class} {optional state} {
    {list of properties}
}
@{name}.{name} {optional state} {
    {list of properties}
}
```

The global properties has to be at the top of the file, the rest can freeĺy be mixed.
