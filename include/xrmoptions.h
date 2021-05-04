/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2020 Qball Cow <qball@gmpclient.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef ROFI_XRMOPTIONS_H
#define ROFI_XRMOPTIONS_H
#include "xcb.h"
#include "theme.h"
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

/**
 *  Type of the config options.
 */
typedef enum
{
    /** Config option is string */
    xrm_String  = 0,
    /** Config option is an unsigned number */
    xrm_Number  = 1,
    /** Config option is a signed number */
    xrm_SNumber = 2,
    /** Config option is a boolean (true/false) value*/
    xrm_Boolean = 3,
    /** Config option is a character */
    xrm_Char    = 4
} XrmOptionType;

/**
 * @param filename The xresources file to parse
 *
 * Parses filename and updates the config
 * @ingroup CONFFile
 */
void config_parse_xresource_options_file ( const char *filename );

/**
 * Parse commandline options.
 * @ingroup CONFCommandline
 */
void config_parse_cmd_options ( void );

/**
 * Free any allocated memory.
 *
 * @ingroup CONFXResources
 */
void config_xresource_free ( void );

/**
 * @param type The type of the value
 * @param key  The key referring to this configuration option
 * @param value The value to update based [out][in]
 * @param comment Description of this configuration option
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

/**
 * @param length the length of the returned array
 *
 * Creates an array with a strings describing each keybinding.
 *
 * @returns an array of string with length elements
 */
char ** config_parser_return_display_help ( unsigned int *length );

/**
 * @brief Set config option.
 *
 * Sets both the static as  dynamic config option.
 *
 * @param p Property to set
 * @param error Error msg when not found.
 *
 * @returns true when failed to set property.
 */
gboolean config_parse_set_property ( const Property *p, char **error );

/**
 * @param out The destination.
 * @param changes Only print the changed options.
 *
 * @brief Dump configuration in rasi format.
 */
void config_parse_dump_config_rasi_format ( FILE *out, gboolean changes );
/** @}*/
#endif
