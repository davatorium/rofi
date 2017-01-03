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
/*@}*/
#endif // ROFI_WINDOW_H
