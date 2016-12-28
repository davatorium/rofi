#include <glib.h>
#include "widgets/widget.h"
#include "widgets/widget-internal.h"
#include "theme.h"


void widget_init ( widget *widget , const char *name, const char *class_name )
{
    widget->name       = g_strdup(name);
    widget->class_name = g_strdup(class_name);
    widget->pad = rofi_theme_get_padding (widget->class_name, widget->name, NULL, "padding", (Padding){0,0,0,0,FALSE});

}

void widget_set_state ( widget *widget, const char *state )
{
    widget->state = state;
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
    if ( widget && !widget->enabled ) {
        widget->enabled = TRUE;
        widget_update ( widget );
    }
}
void widget_disable ( widget *widget )
{
    if ( widget && widget->enabled ) {
        widget->enabled = FALSE;
        widget_update ( widget );
    }
}
void widget_draw ( widget *widget, cairo_t *d )
{
    // Check if enabled and if draw is implemented.
    if ( widget && widget->enabled && widget->draw ) {
        // Store current state.
        cairo_save ( d );
        // Define a clipmask so we won't draw outside out widget.
        cairo_rectangle ( d, widget->x, widget->y, widget->w, widget->h );
        cairo_clip ( d );
        rofi_theme_get_color ( widget->class_name, widget->name, widget->state, "background", d );
        cairo_paint( d ) ;

        // Set new x/y possition.
        cairo_translate ( d, widget->x, widget->y );

        widget->draw ( widget, d );
        widget->need_redraw = FALSE;

        cairo_restore ( d );
    }
}
void widget_free ( widget *wid )
{
    if ( wid ) {
        if ( wid->name ) {
            g_free ( wid->name );
        }
        if ( wid->class_name ) {
            g_free( wid->class_name );
        }
        if ( wid->free ) {
            wid->free ( wid );
        }
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
    return 0;
}
int widget_get_width ( widget *widget )
{
    if ( widget ) {
        if ( widget->get_width ) {
            return widget->get_width ( widget );
        }
        return widget->w;
    }
    return 0;
}
int widget_get_x_pos ( widget *widget )
{
    if ( widget ) {
        return widget->x;
    }
    return 0;
}
int widget_get_y_pos ( widget *widget )
{
    if ( widget ) {
        return widget->y;
    }
    return 0;
}

void widget_update ( widget *widget )
{
    // When (desired )size of widget changes.
    if ( widget ) {
        if ( widget->update ) {
            widget->update ( widget );
        }
        // Recurse back.
        widget_update ( widget->parent );
    }
}

void widget_queue_redraw ( widget *wid )
{
    if ( wid ) {
        widget *iter = wid;
        // Find toplevel widget.
        while ( iter->parent != NULL ) {
            iter->need_redraw = TRUE;
            iter              = iter->parent;
        }
        iter->need_redraw = TRUE;
    }
}

gboolean widget_need_redraw ( widget *wid )
{
    if ( wid && wid->enabled ) {
        return wid->need_redraw;
    }
    return FALSE;
}
gboolean widget_clicked ( widget *wid, xcb_button_press_event_t *xbe )
{
    if ( wid && wid->clicked ) {
        return wid->clicked ( wid, xbe, wid->clicked_cb_data );
    }
    return FALSE;
}
void widget_set_clicked_handler ( widget *wid, widget_clicked_cb cb, void *udata )
{
    if ( wid ) {
        wid->clicked         = cb;
        wid->clicked_cb_data = udata;
    }
}

gboolean widget_motion_notify ( widget *wid, xcb_motion_notify_event_t *xme )
{
    if ( wid && wid->motion_notify ) {
        wid->motion_notify ( wid, xme );
    }

    return FALSE;
}

void widget_set_name ( widget *wid, const char *name )
{
    if ( wid->name ) {
        g_free(wid);
    }
    wid->name = g_strdup ( name );
}

int widget_padding_get_left ( const widget *wid )
{
    if ( wid->pad.left < 0 ){
        return (wid->w*-wid->pad.left)/100;
    }
    return wid->pad.left;
}
int widget_padding_get_right ( const widget *wid )
{
    if ( wid->pad.right < 0 ){
        return (wid->w*-wid->pad.right)/100;
    }
    return wid->pad.right;
}
int widget_padding_get_top ( const widget *wid )
{
    if ( wid->pad.top < 0 ){
        return (wid->h*-wid->pad.top)/100;
    }
    return wid->pad.top;
}
int widget_padding_get_bottom ( const widget *wid )
{
    if ( wid->pad.bottom < 0 ){
        return (wid->h*-wid->pad.bottom)/100;
    }
    return wid->pad.bottom;
}

int widget_padding_get_remaining_width ( const widget *wid )
{
    int width = wid->w;
    width -= widget_padding_get_left ( wid );
    width -= widget_padding_get_right ( wid );
    return width;
}
int widget_padding_get_remaining_height ( const widget *wid )
{
    int height = wid->h;
    height -= widget_padding_get_top ( wid );
    height -= widget_padding_get_bottom ( wid );
    return height;
}
int widget_padding_get_padding_height ( const widget *wid )
{
    int height = 0; 
    height += widget_padding_get_top ( wid );
    height += widget_padding_get_bottom ( wid );
    return height;
}
int widget_padding_get_padding_width ( const widget *wid )
{
    int width = 0; 
    width += widget_padding_get_left ( wid );
    width += widget_padding_get_right ( wid );
    return width;
}
