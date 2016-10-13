#ifndef ROFI_SEPARATOR_H
#define ROFI_SEPARATOR_H
#include <cairo.h>
#include "widget.h"

/**
 * @defgroup separator separator
 * @ingroup widget
 *
 * Displays a horizontal separator line. The height of the widget determines the line width.
 *
 * @{
 */
typedef struct _separator   separator;

/**
 * Direction of the separator.
 */

typedef enum
{
    S_HORIZONTAL = 0,
    S_VERTICAL   = 1
} separator_type;

/**
 * @param type The type of separator.
 * @param sw The thickness of the separator.
 *
 * Create a horizontal separator with height h.
 *
 * @returns a new separator, free with ::widget_free
 */
separator *separator_create ( separator_type type, short sw );

/*@}*/
#endif // ROFI_SEPARATOR_H
