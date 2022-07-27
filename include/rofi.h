/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2022 Qball Cow <qball@gmpclient.org>
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

#ifndef ROFI_MAIN_H
#define ROFI_MAIN_H
#include "keyb.h"
#include "mode.h"
#include "rofi-types.h"
#include "view.h"
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>
#include <xkbcommon/xkbcommon.h>

/**
 * @defgroup Main Main
 * @{
 */

/**
 * Pointer to xdg cache directory.
 */
extern const char *cache_dir;

/**
 * Get the number of enabled modes.
 *
 * @returns the number of enabled modes.
 */
unsigned int rofi_get_num_enabled_modes(void);

/**
 * @param index The mode to return. (should be smaller then
 * rofi_get_num_enabled_mode)
 *
 * Get an enabled mode handle.
 *
 * @returns a Mode handle.
 */
const Mode *rofi_get_mode(unsigned int index);

/**
 * @param str A GString with an error message to display.
 *
 * Queue an error.
 */
void rofi_add_error_message(GString *str);

/**
 * Clear the list of stored error messages.
 */
void rofi_clear_error_messages(void);

/**
 * @param str A GString with an warning message to display.
 *
 * Queue an warning.
 */
void rofi_add_warning_message(GString *str);

/**
 * Clear the list of stored warning messages.
 */
void rofi_clear_warning_messages(void);
/**
 * @param code the code to return
 *
 * Return value are used for integrating dmenu rofi in scripts.
 * This function sets the code that rofi will return on exit.
 */
void rofi_set_return_code(int code);

void rofi_quit_main_loop(void);

/**
 * @param name Search for mode with this name.
 *
 * @return returns Mode * when found, NULL if not.
 */
Mode *rofi_collect_modes_search(const char *name);
/** Reset terminal */
#define color_reset "\033[0m"
/** Set terminal text bold */
#define color_bold "\033[1m"
/** Set terminal text italic */
#define color_italic "\033[2m"
/** Set terminal foreground text green */
#define color_green "\033[0;32m"
/** Set terminal foreground text red */
#define color_red "\033[0;31m"

/** Appends instructions on how to report an error. */
#define ERROR_MSG(a)                                                           \
  a "\n"                                                                       \
    "If you suspect this is caused by a bug in rofi,\n"                        \
    "please report the following information to rofi's github page:\n"         \
    " * The generated commandline output when the error occurred.\n"           \
    " * Output of -dump-xresource\n"                                           \
    " * Steps to reproduce\n"                                                  \
    " * The version of rofi you are running\n\n"                               \
    " <i>https://github.com/davatorium/rofi/</i>"
/** Indicates if ERROR_MSG uses pango markup */
#define ERROR_MSG_MARKUP TRUE
/**@}*/
#endif
