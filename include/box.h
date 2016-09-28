#ifndef ROFI_HBOX_H
#define ROFI_HBOX_H

#include "widget.h"

typedef struct _Box Box;
typedef enum {
    BOX_HORIZONTAL,
    BOX_VERTICAL
} BoxType;

Box * box_create ( BoxType type,  short x, short y, short w, short h );

void box_add ( Box *box, Widget *child, gboolean expand, gboolean end );

void box_update ( Box *box );

int box_get_fixed_pixels ( Box *box );
#endif // ROFI_HBOX_H
