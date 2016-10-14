#ifndef ROFI_DIALOG_SSH_H
#define ROFI_DIALOG_SSH_H

/**
 * @defgroup SSHMode SSH
 * @ingroup MODES
 *
 * SSH Mode, returns a list of known SSH hosts the user can log into.
 * It does this by parsing the SSH config file and optional the known host  and host list
 * It also keeps history of the last choosen hosts.
 *
 * This mode uses the following options from the #config object:
 *  * #Settings::ssh_command
 *  * #Settings::parse_known_hosts
 *  * #Settings::parse_hosts
 *
 * @{
 */

/** #Mode object representing the ssh dialog. */
extern Mode ssh_mode;
/*@}*/
#endif // ROFI_DIALOG_SSH_H
