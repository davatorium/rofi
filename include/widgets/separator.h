#ifndef ROFI_SEPARATOR_H
#define ROFI_SEPARATOR_H
#include <cairo.h>
#include "widget.h"

/**
 * @defgroup separator separator
 * @ingroup widgets
 *
 * @{
 */
typedef struct _separator   separator;

/**
 * @param h The height of the separator.
 *
 * Create a horizontal separator with height h.
 *
 * @returns a new separator, free with ::widget_free
 */
separator *separator_create ( short h );

/*@}*/
#endif // ROFI_SEPARATOR_H
