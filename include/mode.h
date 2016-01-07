#ifndef ROFI_MODE_H
#define ROFI_MODE_H

/**
 * @defgroup MODE Mode
 *
 * The 'object' that makes a mode in rofi.
 * @{
 */
typedef struct _Mode   Mode;

/**
 * Enum used to sum the possible states of ROFI.
 */
typedef enum
{
    /** Exit. */
    MODE_EXIT       = 1000,
    /** Skip to the next cycle-able dialog. */
    NEXT_DIALOG     = 1001,
    /** Reload current DIALOG */
    RELOAD_DIALOG   = 1002,
    /** Previous dialog */
    PREVIOUS_DIALOG = 1003
} ModeMode;

/**
 * State returned by the rofi window.
 */
typedef enum
{
    /** Entry is selected. */
    MENU_OK           = 0x00010000,
    /** User canceled the operation. (e.g. pressed escape) */
    MENU_CANCEL       = 0x00020000,
    /** User requested a mode switch */
    MENU_NEXT         = 0x00040000,
    /** Custom (non-matched) input was entered. */
    MENU_CUSTOM_INPUT = 0x00080000,
    /** User wanted to delete entry from history. */
    MENU_ENTRY_DELETE = 0x00100000,
    /** User wants to jump to another switcher. */
    MENU_QUICK_SWITCH = 0x00200000,
    /** Go to the previous menu. */
    MENU_PREVIOUS     = 0x00400000,
    /** Modifiers */
    MENU_SHIFT        = 0x10000000,
    /** Mask */
    MENU_LOWER_MASK   = 0x0000FFFF
} MenuReturn;

/**
 * @param mode The mode to initialize
 *
 * Initialize mode
 */
void mode_init ( Mode *mode );

/**
 * @param mode The mode to destroy
 *
 * Destroy the mode
 */
void mode_destroy ( Mode *mode );

/**
 * @param mode The mode to query
 *
 * Get the number of entries in the mode.
 *
 * @returns an unsigned in with the number of entries.
 */
unsigned int mode_get_num_entries ( const Mode *sw );

/**
 * @param mode The mode to query
 * @param selected_line The entry to query
 * @param state The state of the entry [out]
 * @param get_entry If the should be returned.
 *
 * Returns the string as it should be displayed for the entry and the state of how it should be displayed.
 *
 * @returns allocated new string and state when get_entry is TRUE otherwise just the state.
 */
char * mode_get_display_value ( const Mode *mode, unsigned int selected_line, int *state, int get_entry );
/*@}*/
#endif
