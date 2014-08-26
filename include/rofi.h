#ifndef __SIMPLESWITCHER_H__
#define __SIMPLESWITCHER_H__
#include <X11/X.h>
#include <glib.h>

#define OVERLAP( a, b, c, d )                      ( ( ( a ) == ( c ) && ( b ) == ( d ) ) || MIN ( ( a ) + ( b ), ( c ) + ( d ) ) - MAX ( ( a ), ( c ) ) > 0 )
#define INTERSECT( x, y, w, h, x1, y1, w1, h1 )    ( OVERLAP ( ( x ), ( w ), ( x1 ), ( w1 ) ) && OVERLAP ( ( y ), ( h ), ( y1 ), ( h1 ) ) )

extern const char *cache_dir;


/**
 * Enum used to sum the possible states of ROFI.
 */
typedef enum
{
    /** Exit. */
    MODE_EXIT     = 1000,
    /** Skip to the next cycle-able dialog. */
    NEXT_DIALOG   = 1001,
    /** Reload current DIALOG */
    RELOAD_DIALOG = 1002
} SwitcherMode;

// switcher callback
typedef SwitcherMode ( *switcher_callback )( char **input, void *data );

/**
 * State returned by the rofi window.
 */
typedef enum
{
    /** Entry is selected. */
    MENU_OK           = 0,
    /** User canceled the operation. (e.g. pressed escape) */
    MENU_CANCEL       = -1,
    /** User requested a mode switch */
    MENU_NEXT         = -2,
    /** Custom (non-matched) input was entered. */
    MENU_CUSTOM_INPUT = -3,
    /** User wanted to delete entry from history. */
    MENU_ENTRY_DELETE = -4,
    MENU_QUICK_SWITCH = -5,
} MenuReturn;


/**
 * Function prototype for the matching algorithm.
 */
typedef int ( *menu_match_cb )( char **tokens, const char *input, int index, void *data );

/**
 * @param lines An array of strings to display.
 * @param num_lines Length of the array with strings to display.
 * @param input A pointer to a string where the inputted data is placed.
 * @param prompt The prompt to show.
 * @param time The current time (used for window interaction.)
 * @param shift pointer to integer that is set to the state of the shift key.
 * @param mmc Menu menu match callback, used for matching user input.
 * @param mmc_data data to pass to mmc.
 * @param selected_line pointer to integer holding the selected line.
 *
 * Main menu callback.
 *
 * @returns The command issued (see MenuReturn)
 */
MenuReturn menu ( char **lines, unsigned int num_lines, char **input, char *prompt,
                  Time *time, int *shift,
                  menu_match_cb mmc, void *mmc_data,
                  int *selected_line, int sorting ) __attribute__ ( ( nonnull ( 1, 3, 4, 9 ) ) );

void catch_exit ( __attribute__( ( unused ) ) int sig );

typedef enum _WindowLocation
{
    WL_CENTER     = 0,
    WL_NORTH_WEST = 1,
    WL_NORTH      = 2,
    WL_NORTH_EAST = 3,
    WL_EAST       = 4,
    WL_EAST_SOUTH = 5,
    WL_SOUTH      = 6,
    WL_SOUTH_WEST = 7,
    WL_WEST       = 8
} WindowLocation;

/**
 * Settings
 */

typedef struct _Settings
{
    char           *switchers;
    // Window settings
    unsigned int   window_opacity;
    // Menu settings
    unsigned int   menu_bw;
    int            menu_width;
    unsigned int   menu_lines;
    unsigned int   menu_columns;
    char           * menu_font;
    char           * menu_fg;
    char           * menu_bg;
    char           * menu_hlfg;
    char           * menu_hlbg;
    char           * menu_bc;
    // Behavior
    char           * terminal_emulator;

    // Key bindings
    char           * window_key;
    char           * run_key;
    char           * ssh_key;
    WindowLocation location;
    unsigned int   hmode;
    unsigned int   padding;
    int            y_offset;
    int            x_offset;

    unsigned int   ssh_set_title;
    unsigned int   fixed_num_lines;

    unsigned int   disable_history;

    unsigned int   levenshtein_sort;
} Settings;

extern Settings config;


int token_match ( char **tokens, const char *input,
                  __attribute__( ( unused ) ) int index,
                  __attribute__( ( unused ) ) void *data );

#endif
