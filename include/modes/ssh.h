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

#ifndef ROFI_MODE_SSH_H
#define ROFI_MODE_SSH_H
#include "mode.h"
/**
 * @defgroup SSHMode SSH
 * @ingroup MODES
 *
 * SSH Mode, returns a list of known SSH hosts the user can log into.
 * It does this by parsing the SSH config file and optional the known host  and
 * host list It also keeps history of the last chosen hosts.
 *
 * This mode uses the following options from the #config object:
 *  * #Settings::ssh_command
 *  * #Settings::parse_known_hosts
 *  * #Settings::parse_hosts
 *
 * @{
 */

/** #Mode object representing the ssh mode. */
extern Mode ssh_mode;
/**@}*/
#endif // ROFI_MODE_SSH_H
