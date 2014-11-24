#ifndef __SSH_DIALOG_H__
#define __SSH_DIALOG_H__

/**
 * @param input Pointer to the user-input string.
 * @param data  Custom data pointer for callback.
 *
 * Run the ssh dialog
 *
 * @returns SwitcherMode selected by user.
 */
SwitcherMode ssh_switcher_dialog ( char **input, void *data );

#endif
