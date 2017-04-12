#ifndef ROFI_MODE_PRIVATE_H
#define ROFI_MODE_PRIVATE_H

#include <gmodule.h>

/** ABI version to check if loaded plugin is compatible. */
#define ABI_VERSION    0x00000004

/**
 * @param data Pointer to #Mode object.
 *
 * Mode free function.
 */
typedef void ( *_mode_free )( Mode *data );

/**
 * @param sw The #Mode pointer
 * @param selected_line The selected line
 * @param state The state to display [out]
 * @param attribute_list List of extra (pango) attribute to apply when displaying. [out][null]
 * @param get_entry if it should only return the state
 *
 * Get the value for displaying.
 *
 * @return the string and state for displaying.
 */
typedef char * ( *_mode_get_display_value )( const Mode *sw, unsigned int selected_line, int *state, GList **attribute_list, int get_entry );

/**
 * @param sw The #Mode pointer
 * @param selected_line The selected line
 *
 * Obtains the icon if available
 *
 * @return Get the icon
 */
typedef cairo_surface_t * ( *_mode_get_icon )( const Mode *sw, unsigned int selected_line, int height );

/**
 * @param sw The #Mode pointer
 * @param selected_line The selected line
 *
 * Obtains the string to complete.
 *
 * @return Get the completion string
 */
typedef char * ( *_mode_get_completion )( const Mode *sw, unsigned int selected_line );

/**
 * @param tokens  List of (input) tokens to match.
 * @param input   The entry to match against.
 * @param case_sensitive Whether case is significant.
 * @param index   The current selected index.
 * @param data    User data.
 *
 * Function prototype for the matching algorithm.
 *
 * @returns 1 when it matches, 0 if not.
 */
typedef int ( *_mode_token_match )( const Mode *data, GRegex **tokens, unsigned int index );

/**
 * @param sw The #Mode pointer
 *
 * Initialize the mode.
 *
 * @returns TRUE is successfull
 */
typedef int ( *__mode_init )( Mode *sw );

/**
 * @param sw The #Mode pointer
 *
 * Get the number of entries.
 *
 * @returns the number of entries
 */
typedef unsigned int ( *__mode_get_num_entries )( const Mode *sw );

/**
 * @param sw The #Mode pointer
 *
 * Destroy the current mode. Still ready to restart.
 *
 */
typedef void ( *__mode_destroy )( Mode *sw );

/**
 * @param sw The #Mode pointer
 * @param menu_retv The return value
 * @param input The input string
 * @param selected_line The selected line
 *
 * Handle the user accepting an entry.
 *
 * @returns the next action to take
 */
typedef ModeMode ( *_mode_result )( Mode *sw, int menu_retv, char **input, unsigned int selected_line );

/**
 * @param sw The #Mode pointer
 * @param input The input string
 *
 * Preprocess the input for sorting.
 *
 * @returns Entry stripped from markup for sorting
 */
typedef char* ( *_mode_preprocess_input )( Mode *sw, const char *input );

/**
 * @param sw The #Mode pointer
 *
 * Message to show in the message bar.
 *
 * @returns the (valid pango markup) message to display.
 */
typedef char * ( *_mode_get_message )( const Mode *sw );

/**
 * Structure defining a switcher.
 * It consists of a name, callback and if enabled
 * a textbox for the sidebar-mode.
 */
struct rofi_mode
{
    /** Used for external plugins. */
    unsigned int abi_version;
    /** Name (max 31 char long) */
    char         name[32];
    char         cfg_name_key[128];
    char         *display_name;

    /**
     * A switcher normally consists of the following parts:
     */
    /** Initialize the Mode */
    __mode_init             _init;
    /** Destroy the switcher, e.g. free all its memory. */
    __mode_destroy          _destroy;
    /** Get number of entries to display. (unfiltered). */
    __mode_get_num_entries  _get_num_entries;
    /** Process the result of the user selection. */
    _mode_result            _result;
    /** Token match. */
    _mode_token_match       _token_match;
    /** Get the string to display for the entry. */
    _mode_get_display_value _get_display_value;
    /** Get the icon for the entry. */
    _mode_get_icon          _get_icon;
    /** Get the 'completed' entry. */
    _mode_get_completion    _get_completion;

    _mode_preprocess_input  _preprocess_input;

    _mode_get_message       _get_message;

    /** Pointer to private data. */
    void                    *private_data;

    /**
     * Free SWitcher
     * Only to be used when the switcher object itself is dynamic.
     * And has data in `ed`
     */
    _mode_free free;
    /** Extra fields for script */
    void       *ed;

    /** Module */
    GModule    *module;
};
#endif // ROFI_MODE_PRIVATE_H
