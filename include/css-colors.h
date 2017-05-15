#ifndef ROFI_INCLUDE_CSS_COLORS_H
#define ROFI_INCLUDE_CSS_COLORS_H

typedef struct CSSColor
{
    char    *name;
    uint8_t b, g, r, a;
}CSSColor;

extern const CSSColor     CSSColors[];
extern const unsigned int num_CSSColors;
#endif // ROFI_INCLUDE_CSS_COLORS_H
