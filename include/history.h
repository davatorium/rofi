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

#ifndef ROFI_HISTORY_H
#define ROFI_HISTORY_H

/**
 * @defgroup HISTORY History
 * @ingroup HELPERS
 *
 * Implements a very simple history module that can be used by a #Mode.
 *
 * This uses the following options from the #config object:
 * * #Settings::disable_history
 * * #Settings::ignored_prefixes
 *
 * @{
 */

/**
 * @param filename The filename of the history cache.
 * @param entry    The entry to add/increment
 *
 * Sets the entry in the history, if it exists its use-count is incremented.
 *
 */
void history_set(const char *filename, const char *entry)
    __attribute__((nonnull));

/**
 * @param filename The filename of the history cache.
 * @param entry    The entry to remove
 *
 * Removes the entry from the history.
 */
void history_remove(const char *filename, const char *entry)
    __attribute__((nonnull));

/**
 * @param filename The filename of the history cache.
 * @param length   The length of the returned list.
 *
 * Gets the entries in the list (in order of usage)
 * @returns a list of entries length long. (and NULL terminated).
 */
char **history_get_list(const char *filename, unsigned int *length)
    __attribute__((nonnull));

/**@}*/
#endif // ROFI_HISTORY_H
