#ifndef __SIMPLESWITCHER_H__
#define __SIMPLESWITCHER_H__
#include <config.h>
#include <X11/X.h>

#define MAX( a, b )                                ( ( a ) > ( b ) ? ( a ) : ( b ) )
#define MIN( a, b )                                ( ( a ) < ( b ) ? ( a ) : ( b ) )
#define NEAR( a, o, b )                            ( ( b ) > ( a ) - ( o ) && ( b ) < ( a ) + ( o ) )
#define OVERLAP( a, b, c, d )                      ( ( ( a ) == ( c ) && ( b ) == ( d ) ) || MIN ( ( a ) + ( b ), ( c ) + ( d ) ) - MAX ( ( a ), ( c ) ) > 0 )
#define INTERSECT( x, y, w, h, x1, y1, w1, h1 )    ( OVERLAP ( ( x ), ( w ), ( x1 ), ( w1 ) ) && OVERLAP ( ( y ), ( h ), ( y1 ), ( h1 ) ) )

extern const char *cache_dir;

typedef enum
{
    WINDOW_SWITCHER,
    RUN_DIALOG,
    SSH_DIALOG,
    NUM_DIALOGS,
    DMENU_DIALOG,
    MODE_EXIT,
    NEXT_DIALOG
} SwitcherMode;

typedef enum
{
    MENU_OK           = 0,
    MENU_CANCEL       = -1,
    MENU_NEXT         = -2,
    MENU_CUSTOM_INPUT = -3,
    MENU_ENTRY_DELETE = -4
} MenuReturn;


typedef int ( *menu_match_cb )( char **tokens, const char *input, int index, void *data );
MenuReturn menu ( char **lines, char **input, char *prompt,
                  Time *time, int *shift,
                  menu_match_cb mmc, void *mmc_data,
                  int *selected_line );

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

typedef enum
{
    VERTICAL   = 0,
    HORIZONTAL = 1
} WindowMode;
/**
 * Settings
 */

typedef struct _Settings
{
    // Window settings
    unsigned int   window_opacity;
    // Menu settings
    unsigned int   menu_bw;
    unsigned int   menu_width;
    unsigned int   menu_lines;
    unsigned int   menu_columns;
    char           * menu_font;
    char           * menu_fg;
    char           * menu_bg;
    char           * menu_hlfg;
    char           * menu_hlbg;
    char           * menu_bc;
    // Behavior
    unsigned int   zeltak_mode;
    char           * terminal_emulator;

    // Key bindings
    char           * window_key;
    char           * run_key;
    char           * ssh_key;
    WindowLocation location;
    WindowMode     hmode;
    unsigned int   padding;
    int            y_offset;
    int            x_offset;

    unsigned int   show_title;
    unsigned int   fixed_num_lines;

} Settings;

extern Settings config;


int token_match ( char **tokens, const char *input,
                  __attribute__( ( unused ) ) int index,
                  __attribute__( ( unused ) ) void *data );

void config_sanity_check ( void );
void config_print ( void );
#endif
