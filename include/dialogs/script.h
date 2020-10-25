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

#ifndef ROFI_DIALOG_SCRIPT_H
#define ROFI_DIALOG_SCRIPT_H

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
Mode *script_switcher_parse_setup ( const char *str );

/**
 * @param token The modi str to check
 *
 * Check if token could be a valid script modi.
 *
 * @returns true when valid.
 */
gboolean script_switcher_is_valid ( const char *token );
/**@}*/
#endif // ROFI_DIALOG_SCRIPT_H
