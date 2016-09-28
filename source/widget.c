#include <glib.h>
#include "widget.h"

static void _widget_free ( Widget *widget )
{
    g_free (widget);
}

Widget *widget_create (void )
{
    Widget *g = g_malloc0 ( sizeof ( Widget ));
    g->enabled = TRUE;
    g->free = _widget_free;
    return g;
}
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

void widget_resize ( Widget *widget, short w, short h )
{
    if ( widget != NULL  )
    {
        if ( widget->resize != NULL) {
            widget->resize ( widget,  w, h ); 
        } else {
            widget->w = w; 
            widget->h = h;
        }
    }
}
void widget_move ( Widget *widget, short x, short y )
{
    if ( widget != NULL ) {
        widget->x = x;
        widget->y = y;
    }
}

gboolean widget_enabled ( Widget *widget )
{
    if ( widget != NULL ) {
        return widget->enabled;
    }
    return FALSE;
}

void widget_enable ( Widget *widget )
{
    if ( widget ) {
        widget->enabled = TRUE;
    }
}
void widget_disable ( Widget *widget )
{
    if ( widget ) {
        widget->enabled = FALSE;
    }
}
void widget_draw ( Widget *widget, cairo_t *d )
{
    // Check if enabled and if draw is implemented.
    if ( widget && widget->enabled && widget->draw ) {
        widget->draw ( widget, d );
    }
}
void widget_free ( Widget *widget )
{
    if ( widget ) {
        widget->free ( widget );
    }
}

int widget_get_height ( Widget *widget )
{
    if ( widget ){
        if ( widget->get_height ) {
            return widget->get_height ( widget );
        }
        return widget->h;
    }
    return -1;
}
int widget_get_width ( Widget *widget )
{
    if ( widget ){
        if ( widget->get_width ) {
            return widget->get_width ( widget );
        }
        return widget->w;
    }
    return -1;
}
