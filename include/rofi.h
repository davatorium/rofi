#ifndef ROFI_MAIN_H
#define ROFI_MAIN_H
#include <X11/X.h>
#include <X11/Xlib.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include "timings.h"
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
 * @param key the Key to match
 * @param modstate the modifier state to match
 *
 * Match key and modifier state against modi.
 *
 * @return the index of the switcher that matches the key combination
 * specified by key and modstate. Returns -1 if none was found
 */
int locate_switcher ( KeySym key, unsigned int modstate );

/** Reset terminal */
#define  color_reset     "\033[0m"
/** Set terminal text bold */
#define  color_bold      "\033[1m"
/** Set terminal text italic */
#define  color_italic    "\033[2m"
/** Set terminal foreground text green */
#define color_green      "\033[0;32m"

/**
 * @param msg The error message to show.
 * @param markup If the message contains pango markup.
 *
 * Create a dialog showing the msg.
 *
 * @returns EXIT_FAILURE if failed to create dialog, EXIT_SUCCESS if succesfull
 */
int show_error_message ( const char *msg, int markup );

#define ERROR_MSG( a )    a "\n"                                       \
    "If you suspect this is caused by a bug in rofi,\n"                \
    "please report the following information to rofi's github page:\n" \
    " * The generated commandline output when the error occored.\n"    \
    " * Output of -dump-xresource\n"                                   \
    " * Steps to reproduce\n"                                          \
    " * The version of rofi you are running\n\n"                       \
    " <i>https://github.com/DaveDavenport/rofi/</i>"
#define ERROR_MSG_MARKUP    TRUE
/*@}*/
#endif
