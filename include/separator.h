#ifndef ROFI_SEPARATOR_H
#define ROFI_SEPARATOR_H
#include <cairo.h>
#include "widget.h"

/**
 * @defgroup Scrollbar Scrollbar
 * @ingroup Widgets
 *
 * @{
 */
/**
 * Internal structure for the separator.
 */
typedef struct _separator 
{
    Widget       widget;
} separator;

separator *separator_create ( short h );

/*@}*/
#endif // ROFI_SEPARATOR_H
