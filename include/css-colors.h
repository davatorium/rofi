#ifndef ROFI_INCLUDE_CSS_COLORS_H
#define ROFI_INCLUDE_CSS_COLORS_H

#include <stdint.h>
/**
 * @defgroup CSSCOLORS CssColors
 * @ingroup HELPERS
 *
 * Lookup table for CSS 4.0 named colors. Like `Navo`.
 *
 * @{
 */

/**
 * Structure of colors.
 */
typedef struct CSSColor {
  /** CSS name of the color. */
  char *name;
  /** BGRA 8 bit color components. */
  uint8_t b, g, r, a;
} CSSColor;

/**
 * Array with all the named colors. Of type #CSSColor, there are #num_CSSColors
 * items in this array.
 */
extern const CSSColor CSSColors[];
/**
 * Number of named colors.
 */
extern const unsigned int num_CSSColors;
/** @} */
#endif // ROFI_INCLUDE_CSS_COLORS_H
