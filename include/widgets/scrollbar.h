#ifndef ROFI_SCROLLBAR_H
#define ROFI_SCROLLBAR_H
#include <cairo.h>
#include "widgets/widget.h"
#include "widgets/widget-internal.h"

/**
 * @defgroup Scrollbar Scrollbar
 * @ingroup widget
 *
 * @{
 */
/**
 * Internal structure for the scrollbar.
 */
typedef struct _scrollbar
{
    widget       widget;
    unsigned int length;
    unsigned int pos;
    unsigned int pos_length;
} scrollbar;

/**
 * @param name  The name of the widget.
 * @param width The width of the scrollbar
 *
 * Create a new scrollbar
 *
 * @returns the scrollbar object.
 */
scrollbar *scrollbar_create ( const char *name, int width );

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

void scrollbar_set_width ( scrollbar *sb, int width );
/*@}*/
#endif // ROFI_SCROLLBAR_H
