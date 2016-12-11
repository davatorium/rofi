#ifndef ROFI_HBOX_H
#define ROFI_HBOX_H

#include "widget.h"

/**
 * @defgroup box box
 * @ingroup widget
 *
 * Widget used to pack multiple widgets either horizontally or vertically.
 * It supports packing widgets horizontally or vertically. Child widgets are always
 * expanded to the maximum size in the oposite direction of the packing direction.
 * e.g. vertically packed widgets use the full box width.
 *
 * @{
 */

/**
 * Abstract handle to the box widget internal state.
 */
typedef struct _box   box;

/**
 * The packing direction of the box
 */
typedef enum
{
    /** Pack widgets horizontal */
    BOX_HORIZONTAL,
    /** Pack widgets vertical */
    BOX_VERTICAL
} boxType;

/**
 * @param name The name of the widget.
 * @param type The packing direction of the newly created box.
 * @param x    The x position of the box relative to its parent.
 * @param y    The y position of the box relative to its parent.
 * @param w    The width of the box.
 * @param h    The height of the box.
 *
 * @returns a newly created box, free with #widget_free
 */
box * box_create ( const char *name, boxType type, short x, short y, short w, short h );

/**
 * @param box   Handle to the box widget.
 * @param child Handle to the child widget to pack.
 * @param expand If the child widget should expand and use all available space.
 * @param end    If the child widget should be packed at the end.
 *
 * Add a widget to the box.
 */
void box_add ( box *box, widget *child, gboolean expand, gboolean end );

/**
 * @param box Handle to the box widget.
 *
 * Obtains the minimal size required to display all widgets. (expanding widgets are not counted, except for their
 * padding)
 *
 * @returns the minimum size in pixels.
 */
int box_get_fixed_pixels ( box *box );
/*@}*/
#endif // ROFI_HBOX_H
