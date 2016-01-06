#ifndef ROFI_DIALOG_SSH_H
#define ROFI_DIALOG_SSH_H

/**
 * @defgroup SSHMode SSH
 *
 * SSH Mode, returns a list of known SSH hosts the user can log into.
 * It does this by parsing the SSH config file and optional the known host  and host list
 * It also keeps history of the last choosen hosts.
 *
 * This mode uses the following options from the #config object:
 *  * #_Settings::ssh_command
 *  * #_Settings::parse_known_hosts
 *  * #_Settings::parse_hosts
 *
 * @{
 */

/**
 * SSH Mode object.
 */
extern Mode ssh_mode;
/*@}*/
#endif // ROFI_DIALOG_SSH_H
