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




/**
 * Structure describing a cairo color.
 */
typedef struct
{
    /** red channel */
    double red;
    /** green channel */
    double green;
    /** blue channel */
    double blue;
    /**  alpha channel */
    double alpha;
} Color;

/**
 * @param name    String representing the color.
 *
 * Allocate a pixel value for an X named color
 */
Color color_get ( const char *const name );


/**
 * @param mask the mask to check for key
 * @param key the key to check in mask
 *
 * Check if key is in the modifier mask.
 *
 * @returns TRUE if key is in the modifier mask
 */
int x11_modifier_active ( unsigned int mask, int key );

/*@}*/
#endif
