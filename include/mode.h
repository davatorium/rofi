/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2022 Qball Cow <qball@gmpclient.org>
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

#ifndef ROFI_MODE_H
#define ROFI_MODE_H
#include "rofi-types.h"
#include <cairo.h>
G_BEGIN_DECLS
/**
 * @defgroup MODE Mode
 *
 * The 'object' that makes a mode in rofi.
 * @{
 */

/**
 * Type of a mode.
 * Access should be done via mode_* functions.
 */
typedef struct rofi_mode Mode;

/**
 * Enum used to sum the possible states of ROFI.
 */
typedef enum {
  /** Exit. */
  MODE_EXIT = 1000,
  /** Skip to the next cycle-able dialog. */
  NEXT_DIALOG = 1001,
  /** Reload current DIALOG */
  RELOAD_DIALOG = 1002,
  /** Previous dialog */
  PREVIOUS_DIALOG = 1003,
  /** Reloads the dialog and unset user input */
  RESET_DIALOG = 1004,
} ModeMode;

/**
 * State returned by the rofi window.
 */
typedef enum {
  /** Entry is selected. */
  MENU_OK = 0x00010000,
  /** User canceled the operation. (e.g. pressed escape) */
  MENU_CANCEL = 0x00020000,
  /** User requested a mode switch */
  MENU_NEXT = 0x00040000,
  /** Custom (non-matched) input was entered. */
  MENU_CUSTOM_INPUT = 0x00080000,
  /** User wanted to delete entry from history. */
  MENU_ENTRY_DELETE = 0x00100000,
  /** User wants to jump to another switcher. */
  MENU_QUICK_SWITCH = 0x00200000,
  /** User wants to jump to custom command. */
  MENU_CUSTOM_COMMAND = 0x00800000,
  /** Go to the previous menu. */
  MENU_PREVIOUS = 0x00400000,
  /** Go to the complete. */
  MENU_COMPLETE = 0x01000000,
  /** Bindings specifics */
  MENU_CUSTOM_ACTION = 0x10000000,
  /** Mask */
  MENU_LOWER_MASK = 0x0000FFFF
} MenuReturn;

/**
 * @param mode The mode to initialize
 *
 * Initialize mode
 *
 * @returns FALSE if there was a failure, TRUE if successful
 */
int mode_init(Mode *mode);

/**
 * @param mode The mode to destroy
 *
 * Destroy the mode
 */
void mode_destroy(Mode *mode);

/**
 * @param mode The mode to query
 *
 * Get the number of entries in the mode.
 *
 * @returns an unsigned int with the number of entries.
 */
unsigned int mode_get_num_entries(const Mode *mode);

/**
 * @param mode The mode to query
 * @param selected_line The entry to query
 * @param state The state of the entry [out]
 * @param attribute_list List of extra (pango) attribute to apply when
 * displaying. [out][null]
 * @param get_entry If the should be returned.
 *
 * Returns the string as it should be displayed for the entry and the state of
 * how it should be displayed.
 *
 * @returns allocated new string and state when get_entry is TRUE otherwise just
 * the state.
 */
char *mode_get_display_value(const Mode *mode, unsigned int selected_line,
                             int *state, GList **attribute_list, int get_entry);

/**
 * @param mode The mode to query
 * @param selected_line The entry to query
 * @param height The desired height of the icon.
 *
 * Returns the icon for the selected_line
 *
 * @returns allocated new cairo_surface_t if applicable
 */
cairo_surface_t *mode_get_icon(Mode *mode, unsigned int selected_line,
                               unsigned int height);

/**
 * @param mode The mode to query
 * @param selected_line The entry to query
 *
 * Return a string that can be used for completion. It has should have no
 * markup.
 *
 * @returns allocated string.
 */
char *mode_get_completion(const Mode *mode, unsigned int selected_line);

/**
 * @param mode The mode to query
 * @param menu_retv The menu return value.
 * @param input Pointer to the user input string. [in][out]
 * @param selected_line the line selected by the user.
 *
 * Acts on the user interaction.
 *
 * @returns the next #ModeMode.
 */
ModeMode mode_result(Mode *mode, int menu_retv, char **input,
                     unsigned int selected_line);

/**
 * @param mode The mode to query
 * @param tokens The set of tokens to match against
 * @param selected_line The index of the entry to match
 *
 * Match entry against the set of tokens.
 *
 * @returns TRUE if matches
 */
int mode_token_match(const Mode *mode, rofi_int_matcher **tokens,
                     unsigned int selected_line);

/**
 * @param mode The mode to query
 *
 * Get the name of the mode.
 *
 * @returns the name of the mode.
 */
const char *mode_get_name(const Mode *mode);

/**
 * @param mode The mode to query
 *
 * Free the resources allocated for this mode.
 */
void mode_free(Mode **mode);

/**
 * @param mode The mode to query
 *
 * Helper functions for mode.
 * Get the private data object.
 */
void *mode_get_private_data(const Mode *mode);

/**
 * @param mode The mode to query
 * @param pd   Pointer to private data to attach to the mode.
 *
 * Helper functions for mode.
 * Set the private data object.
 */
void mode_set_private_data(Mode *mode, void *pd);

/**
 * @param mode The mode to query
 *
 * Get the name of the mode as it should be presented to the user.
 *
 * @return the user visible name of the mode
 */
const char *mode_get_display_name(const Mode *mode);

/**
 * @param mode The mode to query
 *
 * Should be called once for each mode. This adds the display-name configuration
 * option for the mode.
 */
void mode_set_config(Mode *mode);

/**
 * @param mode The mode to query
 * @param input The input to process
 *
 * This processes the input so it can be used for matching and sorting.
 * This includes removing pango markup.
 *
 * @returns a newly allocated string
 */
char *mode_preprocess_input(Mode *mode, const char *input);

/**
 * @param mode The mode to query
 *
 * Query the mode for a user display.
 *
 * @return a new allocated (valid pango markup) message to display (user should
 * free).
 */
char *mode_get_message(const Mode *mode);
/**@}*/
G_END_DECLS
#endif
