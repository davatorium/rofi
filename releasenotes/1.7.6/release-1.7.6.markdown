# 1.7.6: 


# Changelog

 * The `-e` message argument now accepts `-` to read from stdin (blocking).
 * [ConfigParser] Ignore extremely long input arguments. (#1855)
 * [Doc] Fix doxygen documentation.
 * Print detected window manager in the  `-help` output.
 * [Recursive Browser] Add an alternative file browser that can be used for completion.
 * [Mode] Rework modes to indicate type and add completer field for future flexibility.
 * [DRun] Option to scan desktop directory for desktop files. (#1831)
 * [IconFetcher] Fix memory leak.
 * [MKDocs] Add website.
 * [DMenu] Fix row initial tab if non-first column is shown first (#1834)
 * [Textbox] Drawing fixes to textbox.
 * [Matching] Better unicode normalization with `-normalize-match` (#1813)
 * [Theme] Fix some wrong grammar in the calculation grammer. (#1803)
 * [Dmenu] disable async mode in multi-select.
 * [View] Fix wrong bitmask checking.
 * [rofi-theme-selector] prepend newline before specifying new line (#1791)
 * [Script] Strip pango markup when matching rows (#1795)
 * Add an entry history option, so you can return to your previous entered input.
 * [Doc] Documentation fixes.
 * [Textbox] Replace 'space' with a space. (#1784)
 * [Textbox] Draw text after cursor (#1777)
 * [FileBrowser] Allow a custom launch command.
 * [Theme] Add theme to show of features.
 * [window] When no window title set, handle this more gracefully.
 * [Dmenu] Add per row urgen/active option.
 * set & realpath workaround for bsd and darwin os (#1775)
 * [filebrowser] add option to return 1 on cancel.
 * [Helper] Fix for wrong pointer dereference.
 * [Theme] Support 'rasinc' as theme extension for include files.
 * [listview] Don't calculate infinite rows on empty height. (#1769)
 * [CI] CI fixes. (#1767)
 * [Textbox] Add text-outline style.
 * [Textbox] Add cursor-width and cursor-color option (#1753)
 * [Doc] Add example run command with cgroup support (#1752)
 * [Dmenu] add `-ellipsize-mode` option.
 * [Listview] Set ellipsize mode on creation of textbox (#1746) 
 * [Doc] Clarify documentation (#1744)
 * [Window] Fix reloading windowcd based on xserver request.
 * [IMDKit] Add (optional) option to add imdkit support (#1735)  Experimental.
 * [Window] Make sure there is a trailing 0 on the workspace strings. (#1739)
 * [Filebrowser]  Bind `kb-delete-entry` to toggle show hidden.
 * [Textbox] Add a 'get_cursor_x_pos' function.
 * [Window] Small fixes to prefer-icon-theme option (#1702) (thanks to Kafva)
 * [Drun] Only pass path on launch, if one is set.
 * [Filebrowser] Add option to show hidden files (#1715)
 * [Script] Split mode string only once on :, allowing : in right part. (#1718)
 * [Window] Check bitmask, not full comparison.
 * [Keyb] Add -list-keybindings commandline option.
 * Fix sed binary call with variable (#1707)
 * [Listview] ADd check before resolving pointer. (#1703)
 * [Textbox] Add 'placeholder-markup' (#1689)
 * [Theme] if no theme loaded, load default. (#1686)
 * [DMenu] Reset variable correctly so keep-selection is initially off. (#1686)
 * [View|Xcb] Add support to copy current selected item to clipboard. (#378)
 * Include `sys/stat.h` for S_IWUSR (#1685)
 * [View] Tweak error message and instant/delayed switching.
 * [View] Change refilter timeout limit to be in time units (ms) (#1683)
 * [Combi] Fix memory leaks.
 * [View] Increase default refilter-timeout-limit. (#1683)
