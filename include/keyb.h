#ifndef ROFI_KEYB_H
#define ROFI_KEYB_H

/**
 * @defgroup KEYB KeyboardBindings
 *
 * @{
 */

typedef enum
{
    /** Paste from primary clipboard */
    PASTE_PRIMARY = 0,
    /** Paste from secondary clipboard */
    PASTE_SECONDARY,
    /** Clear the entry box. */
    CLEAR_LINE,
    /** Move to front of text */
    MOVE_FRONT,
    /** Move to end of text */
    MOVE_END,
    /** Move on word back */
    MOVE_WORD_BACK,
    /** Move on word forward */
    MOVE_WORD_FORWARD,
    /** Move character back */
    MOVE_CHAR_BACK,
    /** Move character forward */
    MOVE_CHAR_FORWARD,
    /** Remove previous word */
    REMOVE_WORD_BACK,
    /** Remove next work */
    REMOVE_WORD_FORWARD,
    /** Remove next character */
    REMOVE_CHAR_FORWARD,
    /** Remove previous character */
    REMOVE_CHAR_BACK,
    /** Accept the current selected entry */
    ACCEPT_ENTRY,
    ACCEPT_CUSTOM,
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
    SCREENSHOT,
    TOGGLE_SORT,
    NUM_ABE
} KeyBindingAction;

/**
 * Parse the keybindings.
 * This should be called after the setting system is initialized.
 */
gboolean parse_keys_abe ( void );

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
 * Find if a binding has been triggered.
 * @returns NUM_ABE if no key combo matches, a valid action otherwise.
 */
KeyBindingAction abe_find_action ( unsigned int mask, xkb_keysym_t key );

/*@}*/
#endif // ROFI_KEYB_H
