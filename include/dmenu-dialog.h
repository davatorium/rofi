#ifndef __DMENU_DIALOG_H__
#define __DMENU_DIALOG_H__

extern char *dmenu_prompt;
/**
 * Returns TRUE when success, FALSE when canceled.
 */
int dmenu_switcher_dialog ( char **input );

#endif
