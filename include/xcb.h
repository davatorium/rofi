/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2017 Qball Cow <qball@gmpclient.org>
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

#ifndef ROFI_XCB_H
#define ROFI_XCB_H

/**
 * xcb data structure type declaration.
 */
typedef struct _xcb_stuff   xcb_stuff;

/**
 * Global pointer to xcb_stuff instance.
 */
extern xcb_stuff *xcb;

/**
 * @param xcb the xcb data structure
 *
 * Get the root window.
 *
 * @returns the root window.
 */
xcb_window_t xcb_stuff_get_root_window ( xcb_stuff *xcb );
/**
 * @param xcb The xcb data structure.
 *
 * Disconnect and free all xcb connections and references.
 */
void xcb_stuff_wipe ( xcb_stuff *xcb );
#endif
