#ifndef ROFI_I3_H
#define ROFI_I3_H

/**
 * @defgroup I3Support I3Support
 * @ingroup HELPERS
 *
 * These functions are dummies when i3 support is not compiled in.
 *
 * @{
 */

/**
 * @param id          The window to focus on.
 *
 * If we want to switch windows in I3, we use I3 IPC mode.
 * This works more better then sending messages via X11.
 * Hopefully at some point, I3 gets fixed and this is not needed.
 * This function takes the path to the i3 IPC socket, and the XID of the window.
 */
void i3_support_focus_window ( xcb_window_t id );

/**
 * @param xcb The xcb to read the i3 property from.
 *
 * Get the i3 socket from the X root window.
 * @returns TRUE when i3 is running, FALSE when not.
 */

int i3_support_initialize ( xcb_stuff *xcb );

/**
 * Cleanup.
 */
void i3_support_free_internals ( void );
/*@}*/
#endif // ROFI_I3_H
