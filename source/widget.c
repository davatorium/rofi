#include <glib.h>
#include "widget.h"

int widget_intersect ( const Widget *widget, int x, int y )
{
    if ( widget == NULL ) {
        return FALSE;
    }

    if ( x >= ( widget->x ) && x < ( widget->x + widget->w ) ) {
        if ( y >= ( widget->y ) && y < ( widget->y + widget->h ) ) {
            return TRUE;
        }
    }
    return FALSE;
}

void widget_move ( Widget *widget, short x, short y )
{
    if ( widget != NULL ) {
        widget->x = x;
        widget->y = y;
    }
}
