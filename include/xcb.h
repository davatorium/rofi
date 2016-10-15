#ifndef ROFI_XCB_H
#define ROFI_XCB_H

/**
 * xcb data structure type declaration.
 */
typedef struct _xcb_stuff   xcb_stuff;

/**
 * Global pointer to xcb_stuff instance.
 */
extern xcb_stuff *xcb;

/**
 * @param xcb the xcb data structure
 *
 * Get the root window.
 *
 * @returns the root window.
   xcb_window_t xcb_stuff_get_root_window ( xcb_stuff *xcb );
   /**
 * @param xcb The xcb data structure.
 *
 * Disconnect and free all xcb connections and references.
 */
void xcb_stuff_wipe ( xcb_stuff *xcb );
#endif
