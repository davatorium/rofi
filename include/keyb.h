#ifndef __KEYB_H__
#define __KEYB_H__

typedef enum _KeyBindingAction
{
    PASTE_PRIMARY = 0,
    PASTE_SECONDARY,
    CLEAR_LINE,
    MOVE_FRONT,
    MOVE_END,
    MOVE_WORD_BACK,
    MOVE_WORD_FORWARD,
    MOVE_CHAR_BACK,
    MOVE_CHAR_FORWARD,
    REMOVE_WORD_BACK,
    REMOVE_WORD_FORWARD,
    REMOVE_CHAR_FORWARD,
    REMOVE_CHAR_BACK,
    ACCEPT_ENTRY,
    ACCEPT_CUSTOM,
    ACCEPT_ENTRY_CONTINUE,
    MODE_NEXT,
    MODE_PREVIOUS,
    TOGGLE_CASE_SENSITIVITY,
    DELETE_ENTRY,
    NUM_ABE
} KeyBindingAction;


/**
 * Parse the keybindings.
 * This should be called after the setting system is initialized.
 */
void parse_keys_abe ( void );

/**
 * Setup the keybindings
 * This adds all the entries to the settings system.
 */
void setup_abe ( void );

/**
 * Cleanup.
 */
void cleanup_abe ( void );

/**
 * Check if this key has been triggered.
 * @returns TRUE if key combo matches, FALSE otherwise.
 */
int abe_test_action ( KeyBindingAction action, unsigned int mask, KeySym key );
#endif // __KEYB_H__
