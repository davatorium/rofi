#ifndef ROFI_SETTINGS_H
#define ROFI_SETTINGS_H

/**
 * Enumeration indicating the matching method to use.
 *
 * @ingroup CONFIGURATION
 */
typedef enum
{
    MM_NORMAL = 0,
    MM_REGEX  = 1,
    MM_GLOB   = 2,
    MM_FUZZY  = 3
} MatchingMethod;

/**
 * Enumeration indicating location or gravity of window.
 *
 * \verbatim WL_NORTH_WEST      WL_NORTH      WL_NORTH_EAST \endverbatim
 * \verbatim WL_EAST            WL_CENTER     WL_EAST \endverbatim
 * \verbatim WL_SOUTH_WEST      WL_SOUTH      WL_SOUTH_EAST\endverbatim
 *
 * @ingroup CONFIGURATION
 */
typedef enum
{
    /** Center */
    WL_CENTER     = 0,
    /** Left top corner. */
    WL_NORTH_WEST = 1,
    /** Top middle */
    WL_NORTH      = 2,
    /** Top right */
    WL_NORTH_EAST = 3,
    /** Middle right */
    WL_EAST       = 4,
    /** Bottom right */
    WL_EAST_SOUTH = 5,
    /** Bottom middle */
    WL_SOUTH      = 6,
    /** Bottom left */
    WL_SOUTH_WEST = 7,
    /** Middle left */
    WL_WEST       = 8
} WindowLocation;

/**
 * Settings structure holding all (static) configurable options.
 * @ingroup CONFIGURATION
 */
typedef struct
{
    /** List of enabled modi */
    char           *modi;
    /** Window settings */
    unsigned int   window_opacity;
    /** Border width */
    unsigned int   menu_bw;
    /** Width (0-100 in %, > 100 in pixels, < 0 in char width.) */
    int            menu_width;
    /** # lines */
    unsigned int   menu_lines;
    /** # Columns */
    unsigned int   menu_columns;
    /** Font string (pango format) */
    char           * menu_font;

    /** New row colors */
    char           * color_normal;
    char           * color_active;
    char           * color_urgent;
    char           * color_window;
    /** Terminal to use  */
    char           * terminal_emulator;
    /** SSH client to use */
    char           * ssh_client;
    /** Command to execute when ssh session is selected */
    char           * ssh_command;
    /** Command for executing an application */
    char           * run_command;
    /** Command for executing an application in a terminal */
    char           * run_shell_command;
    /** Command for listing executables */
    char           * run_list_command;
    /** Command for window */
    char           * window_command;

    /** Windows location/gravity */
    WindowLocation location;
    /** Padding between elements */
    unsigned int   padding;
    /** Y offset */
    int            y_offset;
    /** X offset */
    int            x_offset;
    /** Always should config.menu_lines lines, even if less lines are available */
    unsigned int   fixed_num_lines;
    /** Do not use history */
    unsigned int   disable_history;
    /** Use levenshtein sorting when matching */
    unsigned int   levenshtein_sort;
    /** Search case sensitivity */
    unsigned int   case_sensitive;
    /** Cycle through in the element list */
    unsigned int   cycle;
    /** Height of an element in number of rows */
    int            element_height;
    /** Sidebar mode, show the modi */
    unsigned int   sidebar_mode;
    /** Lazy filter limit. */
    unsigned int   lazy_filter_limit;
    /** Auto select. */
    unsigned int   auto_select;
    /** Hosts file parsing */
    unsigned int   parse_hosts;
    /** Knonw_hosts file parsing */
    unsigned int   parse_known_hosts;
    /** Combi Modes */
    char           *combi_modi;
    char           *matching;
    MatchingMethod matching_method;
    unsigned int   tokenize;
    /** Monitors */
    char           *monitor;
    /** Line margin */
    unsigned int   line_margin;
    unsigned int   line_padding;
    /** filter */
    char           *filter;
    /** style */
    char           *separator_style;
    /** hide scrollbar */
    unsigned int   hide_scrollbar;
    /** fullscreen */
    unsigned int   fullscreen;
    /** bg image */
    unsigned int   fake_transparency;
    /** dpi */
    int            dpi;
    /** Number threads (1 to disable) */
    unsigned int   threads;
    unsigned int   scrollbar_width;
    unsigned int   scroll_method;
    /** Background type */
    char           *fake_background;

    char           *window_format;
    /** Click outside the window to exit */
    int            click_to_exit;
} Settings;
/** Global Settings structure. */
extern Settings config;
#endif // ROFI_SETTINGS_H
