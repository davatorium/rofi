#ifndef ROFI_HBOX_H
#define ROFI_HBOX_H

#include "widget.h"

/**
 * @defgroup box box
 * @ingroup widgets
 *
 * @{
 */
typedef struct _box   box;
typedef enum
{
    BOX_HORIZONTAL,
    BOX_VERTICAL
} boxType;

box * box_create ( boxType type, short x, short y, short w, short h );

void box_add ( box *box, widget *child, gboolean expand, gboolean end );

int box_get_fixed_pixels ( box *box );
/*@}*/
#endif // ROFI_HBOX_H
