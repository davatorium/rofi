#include <glib.h>
#include "widget.h"

static void _widget_free ( widget *widget )
{
    g_free ( widget );
}

widget *widget_create ( void )
{
    widget *g = g_malloc0 ( sizeof ( widget ) );
    g->enabled = TRUE;
    g->free    = _widget_free;
    return g;
}
int widget_intersect ( const widget *widget, int x, int y )
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

void widget_resize ( widget *widget, short w, short h )
{
    if ( widget != NULL  ) {
        if ( widget->resize != NULL ) {
            widget->resize ( widget, w, h );
        }
        else {
            widget->w = w;
            widget->h = h;
        }
    }
}
void widget_move ( widget *widget, short x, short y )
{
    if ( widget != NULL ) {
        widget->x = x;
        widget->y = y;
    }
}

gboolean widget_enabled ( widget *widget )
{
    if ( widget != NULL ) {
        return widget->enabled;
    }
    return FALSE;
}

void widget_enable ( widget *widget )
{
    if ( widget ) {
        widget->enabled = TRUE;
    }
}
void widget_disable ( widget *widget )
{
    if ( widget ) {
        widget->enabled = FALSE;
    }
}
void widget_draw ( widget *widget, cairo_t *d )
{
    // Check if enabled and if draw is implemented.
    if ( widget && widget->enabled && widget->draw ) {
        widget->draw ( widget, d );
    }
}
void widget_free ( widget *widget )
{
    if ( widget ) {
        widget->free ( widget );
    }
}

int widget_get_height ( widget *widget )
{
    if ( widget ) {
        if ( widget->get_height ) {
            return widget->get_height ( widget );
        }
        return widget->h;
    }
    return -1;
}
int widget_get_width ( widget *widget )
{
    if ( widget ) {
        if ( widget->get_width ) {
            return widget->get_width ( widget );
        }
        return widget->w;
    }
    return -1;
}
