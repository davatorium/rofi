
Each widget has:

* Class: Type of widget. 

Example: textbox, scrollbar, separator

Class are prefixed with a `@`

* Name: Internal name of the widget.

Sub-widgets are {Parent}.{Child}.

Example: Listview, Listview.Even, Listview.Uneven, Listview.Scrollbar

Names are prefixed with a `#`

* State: State of widget

Optional flag(s) indicating state. 
Multiple flags can be set.

Example: Highlight Active Urgent 

States are prefixed with a `!` 

So to set color of Even entry in listview that is highlighted and urgent:

`@textbox #Listview.Even !Urgent !Highlight` 

Or to indicate all textboxes

`@textbox !Highlight`

Class is manditory, name is optional. Name is split on .s.


# Internally:

The theme is represented like a tree:

class --> name --> name --> state -> state

The states are sorted alphabetically

So  `@textbox #Listview.Even !Urgent !Highlight` becomes:

textbox->listview->even -> highlight -> urgent.

When searching for entries the tree is traversed until deepest node is found.
Missing states are skipped.
Then from there properties are searched going up again.

Properties are in the form of:

`name: value`
Each property ends with `;` 
Each property has a type. (Boolean, Integer, String, Color)

A block is enclosed by `{}`

```
@textbox #Listview.Even !Urgent !Highlight {
   padding: 3;
   foreground: #aarrggbb; 
}
```
