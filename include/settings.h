#ifndef ROFI_SETTINGS_H
#define ROFI_SETTINGS_H

/**
 * Enumeration indicating location or gravity of window.
 *
 * \verbatim WL_NORTH_WEST      WL_NORTH      WL_NORTH_EAST \endverbatim
 * \verbatim WL_EAST            WL_CENTER     WL_EAST \endverbatim
 * \verbatim WL_SOUTH_WEST      WL_SOUTH      WL_SOUTH_EAST\endverbatim
 * \endverbatim
 *
 * @ingroup CONFIGURATION
 */
typedef enum _WindowLocation
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
typedef struct _Settings
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
    unsigned int   color_enabled;
    char           * color_normal;
    char           * color_active;
    char           * color_urgent;
    char           * color_window;
    /** Foreground color */
    char           * menu_fg;
    char           * menu_fg_urgent;
    char           * menu_fg_active;
    /** Background color */
    char           * menu_bg;
    char           * menu_bg_urgent;
    char           * menu_bg_active;
    /** Background color alt */
    char           * menu_bg_alt;
    /** Highlight foreground color */
    char           * menu_hlfg;
    char           * menu_hlfg_urgent;
    char           * menu_hlfg_active;
    /** Highlight background color */
    char           * menu_hlbg;
    char           * menu_hlbg_urgent;
    char           * menu_hlbg_active;
    /** Border color */
    char           * menu_bc;
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
    /** Separator to use for dmenu mode */
    char           separator;
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
    /** Fuzzy match */
    unsigned int   fuzzy;
    unsigned int   glob;
    unsigned int   tokenize;
    unsigned int   regex;
    /** Monitors */
    int            monitor;
    /** Line margin */
    unsigned int   line_margin;
    /** filter */
    char           *filter;
    /** style */
    char           *separator_style;
    /** hide scrollbar */
    unsigned int   hide_scrollbar;
    /** show markup in elements. */
    unsigned int   markup_rows;
    /** fullscreen */
    unsigned int   fullscreen;
    /** bg image */
    unsigned int   fake_transparency;
    /** dpi */
    int            dpi;
    /** Number threads (1 to disable) */
    unsigned int   threads;
    unsigned int   scrollbar_width;
} Settings;
/** Global Settings structure. */
extern Settings config;
#endif // ROFI_SETTINGS_H
