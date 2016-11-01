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

/**
 * Abstract handle to the separator widget internal state.
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

typedef enum
{
    S_LINE_NONE,
    S_LINE_SOLID,
    S_LINE_DASH
} separator_line_style;

/**
 * @param type The type of separator.
 * @param sw The thickness of the separator.
 *
 * Create a horizontal separator with height h.
 *
 * @returns a new separator, free with ::widget_free
 */
separator *separator_create ( separator_type type, short sw );

/**
 * @param sp The separator widget handle.
 * @param style_str String representation of the style.
 *
 * Sets the line style based on the string style_str
 */
void separator_set_line_style_from_string ( separator *sp, const char *style_str );

/**
 * @param sp The separator widget handle.
 * @param style The new style.
 *
 * Sets the line style.
 */
void separator_set_line_style ( separator *sp, separator_line_style style );
/*@}*/
#endif // ROFI_SEPARATOR_H
