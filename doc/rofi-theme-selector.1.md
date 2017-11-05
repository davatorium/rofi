# rofi-theme-selector 1 rofi-theme-selector

## NAME

**rofi-theme-selector** - Preview and apply themes for **rofi**

## DESCRIPTION

**rofi-theme-selector** is a bash/rofi script to preview and apply themes for **rofi**.
It's part of any installation of **rofi**.

## USAGE

### Running rofi-theme-selector

**rofi-theme-selector** shows a list of all available themes in a **rofi** window.
It lets you preview each theme with the Enter key and apply the theme to your
**rofi** configuration file with Alt+a.


## Theme directories

**rofi-theme-selector** searches the following directories for themes:

* ${PREFIX}/share/rofi/themes
* $XDG_CONFIG_HOME/rofi
* $XDG_DATA_HOME/share/rofi/themes

${PREFIX} reflects the install location of rofi. In most cases this will be "/usr".
$XDG_CONFIG_HOME is normally unset. Default path is "$HOME/.config".
$XDG_DATA_HOME is normally unset. Default path is "$HOME/.local/share".

SEE ALSO
       rofi(1)

AUTHOR
       Qball Cow qball@gmpclient.org

       Rasmus Steinke rasi@xssn.at
