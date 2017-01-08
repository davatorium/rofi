#ifndef ROFI_CONTAINER_H
#define ROFI_CONTAINER_H

#include "widget.h"

/**
 * @defgroup container container
 * @ingroup widget
 *
 *
 * @{
 */

/**
 * Abstract handle to the container widget internal state.
 */
typedef struct _window   container;

/**
 * @param name The name of the widget.
 *
 * @returns a newly created container, free with #widget_free
 */
container * container_create ( const char *name );

/**
 * @param container   Handle to the container widget.
 * @param child Handle to the child widget to pack.
 *
 * Add a widget to the container.
 */
void container_add ( container *container, widget *child );
/*@}*/
#endif // ROFI_CONTAINER_H
