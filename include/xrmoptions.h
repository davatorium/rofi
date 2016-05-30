#ifndef ROFI_XRMOPTIONS_H
#define ROFI_XRMOPTIONS_H
#include "xcb.h"
// Big thanks to Sean Pringle for this code.

/**
 * @defgroup CONFXResources XResources Configuration
 * @ingroup CONFIGURATION
 *
 * Configuration described in Xresource format. This can be loaded from the X server or file.
 *
 * @defgroup CONFXServer XServer Configuration
 * @ingroup CONFXResources
 *
 * Loads the configuration directly from the X server using the XResources system.
 *
 * @defgroup CONFCommandline Commandline Configuration
 * @ingroup CONFIGURATION
 *
 * Modified the configuration based on commandline arguments
 *
 * @defgroup CONFFile File Configuration
 * @ingroup CONFXResources
 *
 * Loads the configuration from a config file that uses the XResource file format.
 *
 * @defgroup CONFIGURATION Configuration
 *
 * This provides rofi configuration system, supports:
 * * Compiled defaults.
 * * XResource parsing
 * * Config file parsing
 * * Commandline options.
 *
 * @{
 */

// This maps xresource options to config structure.
typedef enum
{
    xrm_String  = 0,
    xrm_Number  = 1,
    xrm_SNumber = 2,
    xrm_Boolean = 3,
    xrm_Char    = 4
} XrmOptionType;

/**
 * @param display Handler of the display to fetch the settings from.
 *
 * Parse the rofi related X resource options of the
 * connected X server.
 *
 * @ingroup CONFXServer
 */
void config_parse_xresource_options ( xcb_stuff *xcb );

/**
 * @ingroup CONFFile
 */
void config_parse_xresource_options_file ( const char *filename );

/**
 * Parse commandline options.
 * @ingroup CONFCommandline
 */
void config_parse_cmd_options ( void );

/**
 * Parse dynamic commandline options.
 * @ingroup CONFCommandline
 */
void config_parse_cmd_options_dynamic ( void );

/**
 * @param display Handler of the display to fetch the settings from.
 *
 * Parse the rofi related X resource options of the
 * connected X server.
 *
 * @ingroup CONFXServer
 */
void config_parse_xresource_options_dynamic ( xcb_stuff *xcb );

/**
 * @ingroup CONFFile
 */
void config_parse_xresource_options_dynamic_file ( const char *filename );

/**
 * Initializes the Xresourced system.
 *
 * @ingroup CONFXResources
 */
void config_parse_xresource_init ( void );
/**
 * Free any allocated memory.
 *
 * @ingroup CONFXResources
 */
void config_xresource_free ( void );

/**
 * Dump the settings in a Xresources compatible way to
 * stdout.
 *
 * @ingroup CONFXResources
 */
void config_parse_xresource_dump ( void );

/**
 * Dump the theme related settings in Xresources compatible way to
 * stdout.
 *
 * @ingroup CONFXResources
 */
void config_parse_xresources_theme_dump ( void );

/**
 * @param type The type of the value
 * @param key  The key refering to this configuration option
 * @param value The value to update based [out][in]
 * @param command Description of this configuration option
 *
 * Add option (at runtime) to the dynamic option parser.
 */
void config_parser_add_option ( XrmOptionType type, const char *key, void **value, const char *comment );

/**
 * Print the current configuration to stdout. Uses bold/italic when printing to terminal.
 */
void print_options ( void );

/**
 * @param option The name of the option
 * @param type   String describing the type
 * @param text   Description of the option
 * @param def    Current value of the option
 * @param isatty If printed to a terminal
 *
 * Function that does the markup for printing an configuration option to stdout.
 */
void print_help_msg ( const char *option, const char *type, const char*text, const char *def, int isatty );

char ** config_parser_return_display_help ( unsigned int *length );

/* @}*/
#endif
