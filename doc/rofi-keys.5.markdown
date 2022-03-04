# ROFI-KEYS 5 rofi-keys

## NAME

**rofi keys** - Rofi Key and Mouse bindings 


## DESCRIPTION

**rofi** supports overriding of any of it key and mouse binding.

## Setting binding

Bindings can be done on the commandline (-{bindingname}):

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


## Keyboard Bindings

### **kb-primary-paste**:
Paste primary selection

  **Default**: 	Control+V,Shift+Insert 

### **kb-secondary-paste**
Paste clipboard

**Default**: 	Control+v,Insert 

### **kb-clear-line**
Clear input line

**Default**: 	Control+w 

### **kb-move-front**
Beginning of line

**Default**: 	Control+a 

### **kb-move-end**
End of line

**Default**: 	Control+e 

### **kb-move-word-back**
Move back one word

**Default**: 	Alt+b,Control+Left 

### **kb-move-word-forward**
Move forward one word

**Default**: 	Alt+f,Control+Right 

### **kb-move-char-back**
Move back one char

**Default**: 	Left,Control+b 

### **kb-move-char-forward**
Move forward one char

**Default**: 	Right,Control+f 

### **kb-remove-word-back**
Delete previous word

**Default**: 	Control+Alt+h,Control+BackSpace 

### **kb-remove-word-forward**
Delete next word

**Default**: 	Control+Alt+d 

### **kb-remove-char-forward**
Delete next char

**Default**: 	Delete,Control+d 

### **kb-remove-char-back**
Delete previous char

**Default**: 	BackSpace,Shift+BackSpace,Control+h 

### **kb-remove-to-eol**
Delete till the end of line

**Default**: 	Control+k 

### **kb-remove-to-sol**
Delete till the start of line

**Default**: 	Control+u 

### **kb-accept-entry**
Accept entry

**Default**: 	Control+j,Control+m,Return,KP_Enter 

### **kb-accept-custom**
Use entered text as command (in ssh/run modes)

**Default**: 	Control+Return 

### **kb-accept-custom-alt**
Use entered text as command (in ssh/run modes)

**Default**: 	Control+Shift+Return 

### **kb-accept-alt**
Use alternate accept command.

**Default**: 	Shift+Return 

### **kb-delete-entry**
Delete entry from history

**Default**: 	Shift+Delete 

### **kb-mode-next**
Switch to the next mode.

**Default**: 	Shift+Right,Control+Tab 

### **kb-mode-previous**
Switch to the previous mode.

**Default**: 	Shift+Left,Control+ISO_Left_Tab 

### **kb-mode-complete**
Start completion for mode.

**Default**: 	Control+l 

### **kb-row-left**
Go to the previous column

**Default**: 	Control+Page_Up 

### **kb-row-right**
Go to the next column

**Default**: 	Control+Page_Down 

### **kb-row-up**
Select previous entry

**Default**: 	Up,Control+p 

### **kb-row-down**
Select next entry

**Default**: 	Down,Control+n 

### **kb-row-tab**
Go to next row, if one left, accept it, if no left next mode.

**Default**: 	

### **kb-element-next**
Go to next row.

**Default**: Tab	

### **kb-element-prev**
Go to previous row.

**Default**: ISO_Left_Tab

### **kb-page-prev**
Go to the previous page

**Default**: 	Page_Up 

### **kb-page-next**
Go to the next page

**Default**: 	Page_Down 

### **kb-row-first**
Go to the first entry

**Default**: 	Home,KP_Home 

### **kb-row-last**
Go to the last entry

**Default**: 	End,KP_End 

### **kb-row-select**
Set selected item as input text

**Default**: 	Control+space 

### **kb-screenshot**
Take a screenshot of the rofi window

**Default**: 	Alt+S 

### **kb-ellipsize**
Toggle between ellipsize modes for displayed data

**Default**: 	Alt+period 

### **kb-toggle-case-sensitivity**
Toggle case sensitivity

**Default**: 	grave,dead_grave 

### **kb-toggle-sort**
Toggle sort

**Default**: 	Alt+grave 

### **kb-cancel**
Quit rofi

**Default**: 	Escape,Control+g,Control+bracketleft 

### **kb-custom-1**
Custom keybinding 1

**Default**: 	Alt+1 

### **kb-custom-2**
Custom keybinding 2

**Default**: 	Alt+2 

### **kb-custom-3**
Custom keybinding 3

**Default**: 	Alt+3 

### **kb-custom-4**
Custom keybinding 4

**Default**: 	Alt+4 

### **kb-custom-5**
Custom Keybinding 5

**Default**: 	Alt+5 

### **kb-custom-6**
Custom keybinding 6

**Default**: 	Alt+6 

### **kb-custom-7**
Custom Keybinding 7

**Default**: 	Alt+7 

### **kb-custom-8**
Custom keybinding 8

**Default**: 	Alt+8 

### **kb-custom-9**
Custom keybinding 9

**Default**: 	Alt+9 

### **kb-custom-10**
Custom keybinding 10

**Default**: 	Alt+0 

### **kb-custom-11**
Custom keybinding 11

**Default**: 	Alt+exclam 

### **kb-custom-12**
Custom keybinding 12

**Default**: 	Alt+at 

### **kb-custom-13**
Custom keybinding 13

**Default**: 	Alt+numbersign 

### **kb-custom-14**
Custom keybinding 14

**Default**: 	Alt+dollar 

### **kb-custom-15**
Custom keybinding 15

**Default**: 	Alt+percent 

### **kb-custom-16**
Custom keybinding 16

**Default**: 	Alt+dead_circumflex 

### **kb-custom-17**
Custom keybinding 17

**Default**: 	Alt+ampersand 

### **kb-custom-18**
Custom keybinding 18

**Default**: 	Alt+asterisk 

### **kb-custom-19**
Custom Keybinding 19

**Default**: 	Alt+parenleft 

### **kb-select-1**
Select row 1

**Default**: 	Super+1 

### **kb-select-2**
Select row 2

**Default**: 	Super+2 

### **kb-select-3**
Select row 3

**Default**: 	Super+3 

### **kb-select-4**
Select row 4

**Default**: 	Super+4 

### **kb-select-5**
Select row 5

**Default**: 	Super+5 

### **kb-select-6**
Select row 6

**Default**: 	Super+6 

### **kb-select-7**
Select row 7

**Default**: 	Super+7 

### **kb-select-8**
Select row 8

**Default**: 	Super+8 

### **kb-select-9**
Select row 9

**Default**: 	Super+9 

### **kb-select-10**
Select row 10

**Default**: 	Super+0 

## Mouse Bindings

### **ml-row-left**
Go to the previous column

**Default**: 	ScrollLeft 

### **ml-row-right**
Go to the next column

**Default**: 	ScrollRight 

### **ml-row-up**
Select previous entry

**Default**: 	ScrollUp 

### **ml-row-down**
Select next entry

**Default**: 	ScrollDown 

### **me-select-entry**
Select hovered row

 **Default**: 	MousePrimary 

### **me-accept-entry**
Accept hovered row

**Default**: 	MouseDPrimary 

### **me-accept-custom**
Accept hovered row with custom action

**Default**: 	Control+MouseDPrimary 


## SEE ALSO

rofi(1), rofi-sensible-terminal(1), rofi-theme(5), rofi-script(5) 

## AUTHOR

Qball Cow <qball@gmpclient.org>

Rasmus Steinke <rasi@xssn.at>

Morgane Glidic <sardemff7+rofi@sardemff7.net>


Original code based on work by: Sean Pringle <sean.pringle@gmail.com>

For a full list of authors, check the AUTHORS file.
