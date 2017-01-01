#ifndef ROFI_WINDOW_H
#define ROFI_WINDOW_H

#include "widget.h"

/**
 * @defgroup window window
 * @ingroup widget
 *
 *
 * @{
 */

/**
 * Abstract handle to the window widget internal state.
 */
typedef struct _window   window;

/**
 * @param name The name of the widget.
 * @param type The packing direction of the newly created window.
 *
 * @returns a newly created window, free with #widget_free
 */
window * window_create ( const char *name );

/**
 * @param window   Handle to the window widget.
 * @param child Handle to the child widget to pack.
 *
 * Add a widget to the window.
 */
void window_add ( window *window, widget *child );

/**
 * @param window Handle to the window widget.
 * 
 * Get the border width of the widget.
 *
 * @returns the border width (times 2) of the widget.
 */
int window_get_border_width ( const window *window );
/*@}*/
#endif // ROFI_WINDOW_H
