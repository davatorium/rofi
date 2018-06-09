#ifndef ROFI_ICON_FETCHER_H
#define ROFI_ICON_FETCHER_H

#include <glib.h>
#include <stdint.h>
#include <cairo.h>
#include "nkutils-xdg-theme.h"

void rofi_icon_fetcher_init ( void );


void rofi_icon_fetcher_destroy ( void );


uint32_t rofi_icon_fetcher_query ( const char *name, const int size );


cairo_surface_t * rofi_icon_fetcher_get ( const uint32_t uid );


#endif // ROFI_ICON_FETCHER_H
