#ifndef ROFI_MAIN_H
#define ROFI_MAIN_H
#include <xcb/xcb.h>
#include <xkbcommon/xkbcommon.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include "keyb.h"
#include "mode.h"
#include "view.h"

/**
 * @defgroup Main Main
 * @{
 */

/**
 * Pointer to xdg cache directory.
 */
extern const char *cache_dir;

/**
 * Get the number of enabled modi.
 *
 * @returns the number of enabled modi.
 */
unsigned int rofi_get_num_enabled_modi ( void );

/**
 * @param index The mode to return. (should be smaller then rofi_get_num_enabled_mode)
 *
 * Get an enabled mode handle.
 *
 * @returns a Mode handle.
 */
const Mode * rofi_get_mode ( unsigned int index );

/**
 * @param str A GString with an error message to display.
 *
 * Queue an error.
 */
void rofi_add_error_message ( GString *str );

/**
 * @param code the code to return
 *
 * Return value are used for integrating dmenu rofi in scripts.
 * This function sets the code that rofi will return on exit.
 */
void rofi_set_return_code ( int code );

/**
 * @param name Search for mode with this name.
 *
 * @return returns Mode * when found, NULL if not.
 */
Mode * rofi_collect_modi_search ( const char *name );
/** Reset terminal */
#define  color_reset     "\033[0m"
/** Set terminal text bold */
#define  color_bold      "\033[1m"
/** Set terminal text italic */
#define  color_italic    "\033[2m"
/** Set terminal foreground text green */
#define color_green      "\033[0;32m"
/** Set terminal foreground text red */
#define color_red        "\033[0;31m"

/** Appends instructions on how to report an error. */
#define ERROR_MSG( a )    a "\n"                                       \
    "If you suspect this is caused by a bug in rofi,\n"                \
    "please report the following information to rofi's github page:\n" \
    " * The generated commandline output when the error occored.\n"    \
    " * Output of -dump-xresource\n"                                   \
    " * Steps to reproduce\n"                                          \
    " * The version of rofi you are running\n\n"                       \
    " <i>https://github.com/DaveDavenport/rofi/</i>"
/** Indicates if ERROR_MSG uses pango markup */
#define ERROR_MSG_MARKUP    TRUE
/*@}*/
#endif
