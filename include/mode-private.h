#ifndef ROFI_MODE_PRIVATE_H
#define ROFI_MODE_PRIVATE_H

typedef void ( *switcher_free )( Mode *data );

typedef char * ( *switcher_get_display_value )( const Mode *sw, unsigned int selected_line, int *state, int get_entry );

typedef char * ( *switcher_get_completion )( const Mode *sw, unsigned int selected_line );
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
typedef int ( *switcher_token_match )( const Mode *data, char **tokens, int not_ascii, int case_sensitive, unsigned int index );

typedef void ( *__mode_init )( Mode *sw );

typedef unsigned int ( *__mode_get_num_entries )( const Mode *sw );

typedef void ( *__mode_destroy )( Mode *sw );

typedef ModeMode ( *switcher_result )( Mode *sw, int menu_retv, char **input, unsigned int selected_line );

typedef int ( *switcher_is_not_ascii )( const Mode *sw, unsigned int index );

/**
 * Structure defining a switcher.
 * It consists of a name, callback and if enabled
 * a textbox for the sidebar-mode.
 */
struct _Mode
{
    /** Name (max 31 char long) */
    char         name[32];
    /** Keybindings (keysym and modmask) */
    char         * keycfg;
    char         * keystr;
    KeySym       keysym;
    unsigned int modmask;

    /**
     * A switcher normally consists of the following parts:
     */
    /** Initialize the Mode */
    __mode_init              _init;
    /** Destroy the switcher, e.g. free all its memory. */
    __mode_destroy           _destroy;
    /** Get number of entries to display. (unfiltered). */
    __mode_get_num_entries   _get_num_entries;
    /** Check if the element is ascii. */
    switcher_is_not_ascii      is_not_ascii;
    /** Process the result of the user selection. */
    switcher_result            result;
    /** Token match. */
    switcher_token_match       token_match;
    /** Get the string to display for the entry. */
    switcher_get_display_value mgrv;
    /** Get the 'completed' entry. */
    switcher_get_completion    get_completion;

    /** Pointer to private data. */
    void                       *private_data;

    /**
     * Free SWitcher
     * Only to be used when the switcher object itself is dynamic.
     * And has data in `ed`
     */
    switcher_free free;
    /** Extra fields for script */
    void          *ed;
};
#endif // ROFI_MODE_PRIVATE_H
