#ifndef __RUN_DIALOG_H__
#define __RUN_DIALOG_H__

/**
 * @param input Pointer to the user-input string.
 * @param data  Custom data pointer for callback.
 *
 * Run the run dialog
 *
 * @returns SwitcherMode selected by user.
 */
SwitcherMode run_switcher_dialog ( char **input, void *data );

#endif
