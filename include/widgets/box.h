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
 *
 * @returns a newly created box, free with #widget_free
 */
box * box_create ( const char *name, boxType type );

/**
 * @param box   Handle to the box widget.
 * @param child Handle to the child widget to pack.
 * @param expand If the child widget should expand and use all available space.
 * @param index The position index.
 *
 * Add a widget to the box.
 */
void box_add ( box *box, widget *child, gboolean expand, int index );
/*@}*/
#endif // ROFI_HBOX_H
