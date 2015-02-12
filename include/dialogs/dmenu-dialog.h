#ifndef __DMENU_DIALOG_H__
#define __DMENU_DIALOG_H__

/**
 * Prompt used in dmenu.
 */
extern char *dmenu_prompt;
extern int  dmenu_selected_line;


/**
 * @param input Pointer to the user-input string.
 *
 * dmenu dialog.
 *
 * @returns TRUE if script was successful.
 */
int dmenu_switcher_dialog ( char **input );

#endif
