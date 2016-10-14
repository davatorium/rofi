#ifndef ROFI_DIALOG_DMENU_H
#define ROFI_DIALOG_DMENU_H

/**
 * @defgroup DMENU DMenu
 * @ingroup MODES
 *
 *
 * @{
 */
/**
 * dmenu dialog.
 *
 * @returns TRUE if script was successful.
 */
int dmenu_switcher_dialog ( void );

/**
 * Print dmenu mode commandline options to stdout, for use in help menu.
 */
void print_dmenu_options ( void );

/*@}*/
#endif // ROFI_DIALOG_DMENU_H
