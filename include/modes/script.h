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

#ifndef ROFI_MODE_SCRIPT_H
#define ROFI_MODE_SCRIPT_H

#include "mode.h"

/**
 * @defgroup SCRIPTMode Script
 * @ingroup MODES
 *
 * @{
 */
/**
 * @param str   The input string to parse
 *
 * Parse an argument string into the right ScriptOptions data object.
 * This is off format: \<Name\>:\<Script\>
 *
 * @returns NULL when it fails, a newly allocated ScriptOptions when successful.
 */
Mode *script_mode_parse_setup(const char *str);

/**
 * @param token The modes str to check
 *
 * Check if token could be a valid script modes.
 *
 * @returns true when valid.
 */
gboolean script_mode_is_valid(const char *token);

/**
 * Gather the users scripts from `~/.config/rofi/scripts/`
 */
void script_mode_gather_user_scripts(void);

/**
 * Cleanup memory allocated by `script_mode_gather_user_scripts`
 */
void script_mode_cleanup(void);
/**
 * @param is_term if printed to terminal
 *
 * List the user scripts found.
 */
void script_user_list(gboolean is_term);
/**@}*/
#endif // ROFI_MODE_SCRIPT_H
