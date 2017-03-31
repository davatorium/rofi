#ifndef X11_ROFI_HELPER_H
#define X11_ROFI_HELPER_H
#include <glib.h>
#include <cairo.h>
#include <xkbcommon/xkbcommon.h>

#include "xkb.h"



/**
 * @param combo String representing the key combo
 * @param mod [out]  The modifier specified (or AnyModifier if not specified)
 * @param key [out]  The key specified
 * @param release [out] If it should react on key-release, not key-press
 *
 * Parse key from user input string.
 */
gboolean x11_parse_key ( const char *combo, unsigned int *mod, xkb_keysym_t *key, gboolean *release, GString * );

/*@}*/
#endif
