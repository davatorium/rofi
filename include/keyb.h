#ifndef ROFI_KEYB_H
#define ROFI_KEYB_H

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
    ROW_LEFT,
    ROW_RIGHT,
    ROW_UP,
    ROW_DOWN,
    ROW_TAB,
    PAGE_PREV,
    PAGE_NEXT,
    ROW_FIRST,
    ROW_LAST,
    ROW_SELECT,
    CANCEL,
    CUSTOM_1,
    CUSTOM_2,
    CUSTOM_3,
    CUSTOM_4,
    CUSTOM_5,
    CUSTOM_6,
    CUSTOM_7,
    CUSTOM_8,
    CUSTOM_9,
    CUSTOM_10,
    CUSTOM_11,
    CUSTOM_12,
    CUSTOM_13,
    CUSTOM_14,
    CUSTOM_15,
    CUSTOM_16,
    CUSTOM_17,
    CUSTOM_18,
    CUSTOM_19,
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
#endif // ROFI_KEYB_H
