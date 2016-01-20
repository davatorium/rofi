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

/**
 * @defgroup Main Main
 * @{
 */
/**
 * Pointer to xdg cache directory.
 */
extern const char *cache_dir;
typedef struct MenuState   MenuState;

/**
 * @param msg The error message to show.
 * @param markup The error message uses pango markup.
 *
 * The error message to show.
 */
void error_dialog ( const char *msg, int markup  );

typedef enum
{
    MENU_NORMAL   = 0,
    MENU_PASSWORD = 1
} MenuFlags;

/**
 * @param sw the Mode to show.
 * @param lines An array of strings to display.
 * @param num_lines Length of the array with strings to display.
 * @param input A pointer to a string where the inputted data is placed.
 * @param prompt The prompt to show.
 * @param shift pointer to integer that is set to the state of the shift key.
 * @param mmc Menu menu match callback, used for matching user input.
 * @param mmc_data data to pass to mmc.
 * @param selected_line pointer to integer holding the selected line.
 * @param message Extra message to display.
 * @param flags   Flags indicating state of the menu.
 *
 * Main menu callback.
 *
 * @returns The command issued (see MenuReturn)
 */
MenuState *menu ( Mode *sw,
                  char *input, char *prompt,
                  const char *message, MenuFlags flags )
__attribute__ ( ( nonnull ( 1, 2, 3  ) ) );

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

MenuReturn menu_state_get_return_value ( const MenuState *state );
unsigned int menu_state_get_selected_line ( const MenuState *state );
unsigned int menu_state_get_next_position ( const MenuState *state );
void menu_state_itterrate ( MenuState *state, XEvent *event );
unsigned int menu_state_get_completed ( const MenuState *state );
const char * menu_state_get_user_input ( const MenuState *state );
void menu_state_free ( MenuState *state );
void menu_state_restart ( MenuState *state );
void menu_state_set_selected_line ( MenuState *state, unsigned int selected_line );
/*@}*/
#endif
