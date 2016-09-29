#ifndef ROFI_WIDGET_H
#define ROFI_WIDGET_H
#include <glib.h>
#include <cairo.h>
/**
 * @defgroup widgets widgets
 *
 * Generic widget class
 *
 * @{
 */
typedef struct _widget
{
    /** X position relative to parent */
    short    x;
    /** Y position relative to parent */
    short    y;
    /** Width of the widget */
    short    w;
    /** Height of the widget */
    short    h;
    /** enabled or not */
    gboolean enabled;
    /** Information about packing. */
    gboolean expand;
    gboolean end;

    struct _widget *parent;
    /** Function prototypes */

    void ( *draw )( struct _widget *widget, cairo_t *draw );
    void ( *free )( struct _widget *widget );
    void ( *resize )( struct _widget *, short, short );
    int ( *get_width )( struct _widget * );
    int ( *get_height )( struct _widget * );
    void ( *update ) ( struct _widget * );
} widget;

/** Macro to get widget from an implementation (e.g. textbox/scrollbar) */
#define WIDGET( a )    ( a != NULL ? (widget *) ( a ) : NULL )

/**
 * @param widget The widget to check
 * @param x The X position relative to parent window
 * @param y the Y position relative to parent window
 *
 * Check if x,y falls within the widget.
 *
 * @return TRUE if x,y falls within the widget
 */
int widget_intersect ( const widget *widget, int x, int y );

/**
 * @param widget The widget to move
 * @param x The new X position relative to parent window
 * @param y The new Y position relative to parent window
 *
 * Moves the widget.
 */
void widget_move ( widget *widget, short x, short y );

gboolean widget_enabled ( widget *widget );
void widget_disable ( widget *widget );
void widget_enable ( widget *widget );

/**
 * @param tb  Handle to the widget
 * @param draw The cairo object used to draw itself.
 *
 * Render the textbox.
 */
void widget_draw ( widget *widget, cairo_t *d );

/**
 * @param tb  Handle to the widget
 *
 * Free the widget and all allocated memory.
 */
void widget_free ( widget *widget );

/**
 * @param widget The widget toresize
 * @param w The new width
 * @param h The new height
 *
 * Resizes the widget.
 */
void widget_resize ( widget *widget, short w, short h );

int widget_get_height ( widget *widget );
int widget_get_width ( widget *widget );
widget *widget_create ( void );

void widget_update ( widget *widget );
/*@}*/
#endif // ROFI_WIDGET_H
