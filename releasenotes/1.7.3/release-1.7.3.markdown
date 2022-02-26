# 1.7.3: Sturtled!

A small intermediate release with a few fixes, mostly in documentation and two great additions by Jakub Jiruta:

 * An option to customize the combi mode display format. 
 * To possibility to set tab stops on listview and entry boxes.

# Changelog

v1.7.3:
   - [Help] Print out the parsed config/theme files in -help output.
   - [Keybindings] Fix keybindings being modified by -theme-str
   - [Doc] Add rofi-dmenu manpage.
   - [XCB] Cache lookup of monitor.
   - Add -replace option (#568)
   - Fix memory leak.
   - [1566] Add extra debug for resolving monitors.
   - [Theme] Add round,floor,ceil function in @calc (#1569)
   - [Doc] Explain icon lookup.
   - [Combi] Add -combi-display-format (#1570) (thanks to Jakub)
   - [Theme] Expand list type ([]) for more data types.
   - [Theme] Add support for tab-stops on textbox. (#1571) (thanks to Jakub)
   - [Theme] Testing direct access to widgets via cmdline option (-theme+widget+property value)

# Thanks

Big thanks to everybody reporting issues.
Special thanks goes to:

* Iggy
* Morgane Glidic
* Danny Colin
* Jakub Jirutka

Apologies if I mistyped or missed anybody.
