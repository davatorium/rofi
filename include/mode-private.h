#ifndef ROFI_MODE_PRIVATE_H
#define ROFI_MODE_PRIVATE_H

typedef void ( *_mode_free )( Mode *data );

typedef char * ( *_mode_get_display_value )( const Mode *sw, unsigned int selected_line, int *state, int get_entry );

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

typedef int ( *__mode_init )( Mode *sw );

typedef unsigned int ( *__mode_get_num_entries )( const Mode *sw );

typedef void ( *__mode_destroy )( Mode *sw );

typedef ModeMode ( *_mode_result )( Mode *sw, int menu_retv, char **input, unsigned int selected_line );

typedef char* ( *_mode_preprocess_input )( Mode *sw, const char *input );

/**
 * Structure defining a switcher.
 * It consists of a name, callback and if enabled
 * a textbox for the sidebar-mode.
 */
struct rofi_mode
{
    /** Name (max 31 char long) */
    char name[32];
    char cfg_name_key[128];
    char cfg_close_key[128];
    char *display_name;
    char *display_close;

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
    /** Get the 'completed' entry. */
    _mode_get_completion    _get_completion;

    _mode_preprocess_input  _preprocess_input;

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
};
#endif // ROFI_MODE_PRIVATE_H
