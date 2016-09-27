#ifndef ROFI_SCROLLBAR_H
#define ROFI_SCROLLBAR_H
#include <cairo.h>
#include "widget.h"

/**
 * @defgroup Scrollbar Scrollbar
 * @ingroup Widgets
 *
 * @{
 */
/**
 * Internal structure for the scrollbar.
 */
typedef struct _scrollbar
{
    Widget       widget;
    unsigned int length;
    unsigned int pos;
    unsigned int pos_length;
} scrollbar;

/**
 * @param x     The x coordinate (relative to parent) to position the new scrollbar
 * @param y     The y coordinate (relative to parent) to position the new scrollbar
 * @param w     The width of the scrollbar
 * @param h     The height of the scrollbar
 *
 * Create a new scrollbar
 *
 * @returns the scrollbar object.
 */
scrollbar *scrollbar_create ( short x, short y, short w, short h );

/**
 * @param sb scrollbar object
 * @param pos_length new length
 *
 * set the length of the handle relative to the max value of bar.
 */
void scrollbar_set_handle_length ( scrollbar *sb, unsigned int pos_length );

/**
 * @param sb scrollbar object
 * @param pos new position
 *
 * set the position of the handle relative to the set max value of bar.
 */
void scrollbar_set_handle ( scrollbar *sb, unsigned int pos );

/**
 * @param sb scrollbar object
 * @param max the new max
 *
 * set the max value of the bar.
 */
void scrollbar_set_max_value ( scrollbar *sb, unsigned int max );

/**
 * @param sb scrollbar object
 * @param y  clicked position
 *
 * Calculate the position of the click relative to the max value of bar
 */
unsigned int scrollbar_clicked ( const scrollbar *sb, int y );

/**
 * @param sb scrollbar object
 * @param w  new width in pixels
 * @param h  new height in pixels
 *
 * Resize the scrollbar.
 */
void scrollbar_resize ( scrollbar *sb, int w, int h );

/*@}*/
#endif // ROFI_SCROLLBAR_H
