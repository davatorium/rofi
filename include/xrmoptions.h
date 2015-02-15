#ifndef __XRMOPTIONS_H__
#define __XRMOPTIONS_H__

// Big thanks to Sean Pringle for this code.
// This maps xresource options to config structure.
typedef enum
{
    xrm_String  = 0,
    xrm_Number  = 1,
    xrm_SNumber = 2,
    xrm_Boolean = 3
} XrmOptionType;

/**
 * @param display Handler of the display to fetch the settings from.
 *
 * Parse the rofi related X resource options of the
 * connected X server.
 */
void config_parse_xresource_options ( Display *display );

/**
 * @param display Handler of the display to fetch the settings from.
 *
 * Parse the rofi related X resource options of the
 * connected X server.
 */
void config_parse_xresource_options_dynamic ( Display *display );

/**
 * Free any allocated memory.
 */
void config_xresource_free ( void );

/**
 * Dump the settings in a Xresources compatible way to
 * stdout.
 */
void xresource_dump ( void );

void config_parser_add_option ( XrmOptionType type, const char *key, void **value );
#endif
