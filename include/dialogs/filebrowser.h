/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2020 Qball Cow <qball@gmpclient.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef ROFI_DIALOG_FILE_BROWSER_H
#define ROFI_DIALOG_FILE_BROWSER_H

/**
 * @defgroup FileBrowserMode FileBrowser
 * @ingroup MODES
 *
 *
 * @{
 */
/** #Mode object representing the run dialog. */
extern Mode file_browser_mode;

/**
 * Create a new filebrowser.
 * @returns a new filebrowser structure.
 */
Mode *create_new_file_browser ( void );
/**
 * @param sw Mode object.
 * @param mretv return value passed in.
 * @param input The user input string.
 * @param selected_line The user selected line.
 * @param path The full path as output.
 *
 * @returns the state the user selected.
 */
ModeMode file_browser_mode_completer ( Mode *sw, int mretv, char **input, unsigned int selected_line, char **path );
/**@}*/
#endif // ROFI_DIALOG_FILE_BROWSER_H
