#ifndef ROFI_WIDGET_H
#define ROFI_WIDGET_H

/**
 * @defgroup Widgets Widgets
 *
 * Generic Widget class
 *
 * @{
 */
typedef struct
{
    /** X position relative to parent */
    short x;
    /** Y position relative to parent */
    short y;
    /** Width of the widget */
    short w;
    /** Height of the widget */
    short h;
} Widget;

/** Macro to get widget from an implementation (e.g. textbox/scrollbar) */
#define WIDGET( a )    ( a != NULL ? &( a->widget ) : NULL )

/**
 * @param widget The widget to check
 * @param x The X position relative to parent window
 * @param y the Y position relative to parent window
 *
 * Check if x,y falls within the widget.
 *
 * @return TRUE if x,y falls within the widget
 */
int widget_intersect ( const Widget *widget, int x, int y );

/**
 * @param widget The widget to move
 * @param x The new X position relative to parent window
 * @param y The new Y position relative to parent window
 *
 * Moves the widget.
 */
void widget_move ( Widget *widget, short x, short y );

/*@}*/
#endif // ROFI_WIDGET_H
