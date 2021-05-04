/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2020 Qball Cow <qball@gmpclient.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xkb.h>
#include <xcb/xcb_xrm.h>
#include <glib.h>
#include "xcb.h"
#include "xcb-internal.h"
#include "rofi.h"
#include "xrmoptions.h"
#include "settings.h"
#include "helper.h"
#include "rofi-types.h"

/** Different sources of configuration. */
const char * const ConfigSourceStr[] = {
    "Default",
    "File",
    "Rasi File",
    "Commandline",
};
/** Enumerator of different sources of configuration. */
enum ConfigSource
{
    CONFIG_DEFAULT    = 0,
    CONFIG_FILE       = 1,
    CONFIG_FILE_THEME = 2,
    CONFIG_CMDLINE    = 3
};

typedef struct
{
    int        type;
    const char * name;
    union
    {
        unsigned int * num;
        int          * snum;
        char         ** str;
        void         *pointer;
        char         * charc;
    }                 value;
    char              *mem;
    const char        *comment;
    enum ConfigSource source;
} XrmOption;
/**
 * Map X resource and commandline options to internal options
 * Currently supports string, boolean and number (signed and unsigned).
 */
static XrmOption xrmOptions[] = {
    { xrm_String,  "switchers",                 { .str   = &config.modi                                 }, NULL,
      "", CONFIG_DEFAULT },
    { xrm_String,  "modi",                      { .str   = &config.modi                                 }, NULL,
      "Enabled modi", CONFIG_DEFAULT },
    { xrm_SNumber, "width",                     { .snum  = &config.menu_width                           }, NULL,
      "Window width", CONFIG_DEFAULT },
    { xrm_Number,  "lines",                     { .num   = &config.menu_lines                           }, NULL,
      "Number of lines", CONFIG_DEFAULT },
    { xrm_Number,  "columns",                   { .num   = &config.menu_columns                         }, NULL,
      "Number of columns", CONFIG_DEFAULT },

    { xrm_String,  "font",                      { .str   = &config.menu_font                            }, NULL,
      "Font to use", CONFIG_DEFAULT },
    { xrm_Number,  "borderwidth",               { .num   = &config.menu_bw                              }, NULL,
      "", CONFIG_DEFAULT },
    { xrm_Number,  "bw",                        { .num   = &config.menu_bw                              }, NULL,
      "Border width", CONFIG_DEFAULT },

    { xrm_Number,  "location",                  { .num   = &config.location                             }, NULL,
      "Location on screen", CONFIG_DEFAULT },

    { xrm_Number,  "padding",                   { .num   = &config.padding                              }, NULL,
      "Padding", CONFIG_DEFAULT },
    { xrm_SNumber, "yoffset",                   { .snum  = &config.y_offset                             }, NULL,
      "Y-offset relative to location", CONFIG_DEFAULT },
    { xrm_SNumber, "xoffset",                   { .snum  = &config.x_offset                             }, NULL,
      "X-offset relative to location", CONFIG_DEFAULT },
    { xrm_Boolean, "fixed-num-lines",           { .num   = &config.fixed_num_lines                      }, NULL,
      "Always show number of lines", CONFIG_DEFAULT },

    { xrm_Boolean, "show-icons",                { .snum  = &config.show_icons                           }, NULL,
      "Whether to load and show icons", CONFIG_DEFAULT },

    { xrm_String,  "terminal",                  { .str   = &config.terminal_emulator                    }, NULL,
      "Terminal to use", CONFIG_DEFAULT },
    { xrm_String,  "ssh-client",                { .str   = &config.ssh_client                           }, NULL,
      "Ssh client to use", CONFIG_DEFAULT },
    { xrm_String,  "ssh-command",               { .str   = &config.ssh_command                          }, NULL,
      "Ssh command to execute", CONFIG_DEFAULT },
    { xrm_String,  "run-command",               { .str   = &config.run_command                          }, NULL,
      "Run command to execute", CONFIG_DEFAULT },
    { xrm_String,  "run-list-command",          { .str   = &config.run_list_command                     }, NULL,
      "Command to get extra run targets", CONFIG_DEFAULT },
    { xrm_String,  "run-shell-command",         { .str   = &config.run_shell_command                    }, NULL,
      "Run command to execute that runs in shell", CONFIG_DEFAULT },
    { xrm_String,  "window-command",            { .str   = &config.window_command                       }, NULL,
      "Command to executed when -kb-accept-alt binding is hit on selected window ", CONFIG_DEFAULT },
    { xrm_String,  "window-match-fields",       { .str   = &config.window_match_fields                  }, NULL,
      "Window fields to match in window mode", CONFIG_DEFAULT },
    { xrm_String,  "icon-theme",                { .str   = &config.icon_theme                           }, NULL,
      "Theme to use to look for icons", CONFIG_DEFAULT },

    { xrm_String,  "drun-match-fields",         { .str   = &config.drun_match_fields                    }, NULL,
      "Desktop entry fields to match in drun", CONFIG_DEFAULT },
    { xrm_String,  "drun-categories",           { .str   = &config.drun_categories                      }, NULL,
      "Only show Desktop entry from these categories", CONFIG_DEFAULT },
    { xrm_Boolean, "drun-show-actions",         { .num   = &config.drun_show_actions                    }, NULL,
      "Desktop entry show actions.", CONFIG_DEFAULT },
    { xrm_String,  "drun-display-format",       { .str   = &config.drun_display_format                  }, NULL,
      "DRUN format string. (Supports: generic,name,comment,exec,categories)", CONFIG_DEFAULT },
    { xrm_String,  "drun-url-launcher",         { .str   = &config.drun_url_launcher                    }, NULL,
      "Command to open an Desktop Entry that is a Link.", CONFIG_DEFAULT },

    { xrm_Boolean, "disable-history",           { .num   = &config.disable_history                      }, NULL,
      "Disable history in run/ssh", CONFIG_DEFAULT },
    { xrm_String,  "ignored-prefixes",          { .str   = &config.ignored_prefixes                     }, NULL,
      "Programs ignored for history", CONFIG_DEFAULT },
    { xrm_Boolean, "sort",                      { .num   = &config.sort                                 }, NULL,
      "Use sorting", CONFIG_DEFAULT },
    { xrm_String,  "sorting-method",            { .str   = &config.sorting_method                       }, NULL,
      "Choose the strategy used for sorting: normal (levenshtein) or fzf.", CONFIG_DEFAULT },
    { xrm_Boolean, "case-sensitive",            { .num   = &config.case_sensitive                       }, NULL,
      "Set case-sensitivity", CONFIG_DEFAULT },
    { xrm_Boolean, "cycle",                     { .num   = &config.cycle                                }, NULL,
      "Cycle through the results list", CONFIG_DEFAULT },
    { xrm_Boolean, "sidebar-mode",              { .num   = &config.sidebar_mode                         }, NULL,
      "Enable sidebar-mode", CONFIG_DEFAULT },
    { xrm_Boolean, "hover-select",              { .snum   = &config.hover_select                         }, NULL,
      "Enable hover-select", CONFIG_DEFAULT },
    { xrm_SNumber, "eh",                        { .snum  = &config.element_height                       }, NULL,
      "Row height (in chars)", CONFIG_DEFAULT },
    { xrm_Boolean, "auto-select",               { .num   = &config.auto_select                          }, NULL,
      "Enable auto select mode", CONFIG_DEFAULT },
    { xrm_Boolean, "parse-hosts",               { .num   = &config.parse_hosts                          }, NULL,
      "Parse hosts file for ssh mode", CONFIG_DEFAULT },
    { xrm_Boolean, "parse-known-hosts",         { .num   = &config.parse_known_hosts                    }, NULL,
      "Parse known_hosts file for ssh mode", CONFIG_DEFAULT },
    { xrm_String,  "combi-modi",                { .str   = &config.combi_modi                           }, NULL,
      "Set the modi to combine in combi mode", CONFIG_DEFAULT },
    { xrm_String,  "matching",                  { .str   = &config.matching                             }, NULL,
      "Set the matching algorithm. (normal, regex, glob, fuzzy)", CONFIG_DEFAULT },
    { xrm_Boolean, "tokenize",                  { .num   = &config.tokenize                             }, NULL,
      "Tokenize input string", CONFIG_DEFAULT },
    { xrm_String,  "monitor",                   { .str   = &config.monitor                              }, NULL,
      "", CONFIG_DEFAULT },
    /* Alias for dmenu compatibility. */
    { xrm_String,  "m",                         { .str   = &config.monitor                              }, NULL,
      "Monitor id to show on", CONFIG_DEFAULT },
    { xrm_Number,  "line-margin",               { .num   = &config.line_margin                          }, NULL,
      "Margin between rows *DEPRECATED*", CONFIG_DEFAULT },
    { xrm_Number,  "line-padding",              { .num   = &config.line_padding                         }, NULL,
      "Padding within rows *DEPRECATED*", CONFIG_DEFAULT },
    { xrm_String,  "filter",                    { .str   = &config.filter                               }, NULL,
      "Pre-set filter", CONFIG_DEFAULT },
    { xrm_String,  "separator-style",           { .str   = &config.separator_style                      }, NULL,
      "Separator style (none, dash, solid) *DEPRECATED*", CONFIG_DEFAULT },
    { xrm_Boolean, "hide-scrollbar",            { .num   = &config.hide_scrollbar                       }, NULL,
      "Hide scroll-bar *DEPRECATED*", CONFIG_DEFAULT },
    { xrm_Boolean, "fake-transparency",         { .num   = &config.fake_transparency                    }, NULL,
      "Fake transparency *DEPRECATED*", CONFIG_DEFAULT },
    { xrm_SNumber, "dpi",                       { .snum  = &config.dpi                                  }, NULL,
      "DPI", CONFIG_DEFAULT },
    { xrm_Number,  "threads",                   { .num   = &config.threads                              }, NULL,
      "Threads to use for string matching", CONFIG_DEFAULT },
    { xrm_Number,  "scrollbar-width",           { .num   = &config.scrollbar_width                      }, NULL,
      "Scrollbar width *DEPRECATED*", CONFIG_DEFAULT },
    { xrm_Number,  "scroll-method",             { .num   = &config.scroll_method                        }, NULL,
      "Scrolling method. (0: Page, 1: Centered)", CONFIG_DEFAULT },
    { xrm_String,  "fake-background",           { .str   = &config.fake_background                      }, NULL,
      "Background to use for fake transparency. (background or screenshot) *DEPRECATED*", CONFIG_DEFAULT },
    { xrm_String,  "window-format",             { .str   = &config.window_format                        }, NULL,
      "Window Format. w (desktop name), t (title), n (name), r (role), c (class)", CONFIG_DEFAULT },
    { xrm_Boolean, "click-to-exit",             { .snum  = &config.click_to_exit                        }, NULL,
      "Click outside the window to exit", CONFIG_DEFAULT },
    { xrm_String,  "theme",                     { .str   = &config.theme                                }, NULL,
      "New style theme file", CONFIG_DEFAULT },
    { xrm_String,  "color-normal",              { .str   = &config.color_normal                         }, NULL,
      "Color scheme for normal row", CONFIG_DEFAULT },
    { xrm_String,  "color-urgent",              { .str   = &config.color_urgent                         }, NULL,
      "Color scheme for urgent row", CONFIG_DEFAULT },
    { xrm_String,  "color-active",              { .str   = &config.color_active                         }, NULL,
      "Color scheme for active row", CONFIG_DEFAULT },
    { xrm_String,  "color-window",              { .str   = &config.color_window                         }, NULL,
      "Color scheme window", CONFIG_DEFAULT },
    { xrm_Number,  "max-history-size",          { .num   = &config.max_history_size                     }, NULL,
      "Max history size (WARNING: can cause slowdowns when set to high).", CONFIG_DEFAULT },
    { xrm_Boolean, "combi-hide-mode-prefix",    { .snum  = &config.combi_hide_mode_prefix               }, NULL,
      "Hide the prefix mode prefix on the combi view.", CONFIG_DEFAULT },
    { xrm_Char,    "matching-negate-char",      { .charc = &config.matching_negate_char                 }, NULL,
      "Set the character used to negate the matching. ('\\0' to disable)", CONFIG_DEFAULT },
    { xrm_String,  "cache-dir",                 { .str   = &config.cache_dir                            }, NULL,
      "Directory where history and temporary files are stored.", CONFIG_DEFAULT },
    { xrm_Boolean, "window-thumbnail",          { .snum  = &config.window_thumbnail                     }, NULL,
      "Show window thumbnail (if available) as icon in window switcher.", CONFIG_DEFAULT },
    { xrm_Boolean, "drun-use-desktop-cache",    { .snum  = &config.drun_use_desktop_cache               }, NULL,
      "DRUN: build and use a cache with desktop file content.", CONFIG_DEFAULT },
    { xrm_Boolean, "drun-reload-desktop-cache", { .snum  = &config.drun_reload_desktop_cache            }, NULL,
      "DRUN: If enabled, reload the cache with desktop file content.", CONFIG_DEFAULT },
    { xrm_Boolean, "normalize-match",           { .snum  = &config.normalize_match                      }, NULL,
      "Normalize string when matching (disables match highlighting).", CONFIG_DEFAULT },
    { xrm_Boolean, "steal-focus",               { .snum  = &config.steal_focus                          }, NULL,
      "Steal focus on launch and restore to window that had it on rofi start on close .", CONFIG_DEFAULT },
};

/** Dynamic array of extra options */
XrmOption    *extra_options = NULL;
/** Number of entries in extra options array */
unsigned int num_extra_options = 0;

void config_parser_add_option ( XrmOptionType type, const char *key, void **value, const char *comment )
{
    extra_options = g_realloc ( extra_options, ( num_extra_options + 1 ) * sizeof ( XrmOption ) );

    extra_options[num_extra_options].type          = type;
    extra_options[num_extra_options].name          = key;
    extra_options[num_extra_options].value.pointer = value;
    extra_options[num_extra_options].comment       = comment;
    extra_options[num_extra_options].source        = CONFIG_DEFAULT;
    switch ( type )
    {
    case xrm_String:
        extra_options[num_extra_options].mem = ( (char *) ( *value ) );
        break;
    default:
        extra_options[num_extra_options].mem = NULL;
        break;
    }

    num_extra_options++;
}

static void config_parser_set ( XrmOption *option, char *xrmValue, enum ConfigSource source )
{
    switch ( option->type )
    {
    case xrm_String:
        if ( ( option )->mem != NULL ) {
            g_free ( option->mem );
            option->mem = NULL;
        }
        *( option->value.str ) = g_strchomp ( g_strdup ( xrmValue ) );

        // Memory
        ( option )->mem = *( option->value.str );
        break;
    case xrm_Number:
        *( option->value.num ) = (unsigned int) g_ascii_strtoull ( xrmValue, NULL, 10 );
        break;
    case xrm_SNumber:
        *( option->value.snum ) = (int) g_ascii_strtoll ( xrmValue, NULL, 10 );
        break;
    case xrm_Boolean:
        if ( strlen ( xrmValue ) > 0 &&
             g_ascii_strcasecmp ( xrmValue, "true" ) == 0 ) {
            *( option->value.num ) = TRUE;
        }
        else{
            *( option->value.num ) = FALSE;
        }
        break;
    case xrm_Char:
        *( option->value.charc ) = helper_parse_char ( xrmValue );
        break;
    }
    option->source = source;
}

static void __config_parse_xresource_options ( xcb_xrm_database_t *xDB, enum ConfigSource source )
{
    const char * namePrefix = "rofi";

    for ( unsigned int i = 0; i < sizeof ( xrmOptions ) / sizeof ( XrmOption ); ++i ) {
        char *name = g_strdup_printf ( "%s.%s", namePrefix, xrmOptions[i].name );

        char *xrmValue = NULL;
        if ( xcb_xrm_resource_get_string ( xDB, name, NULL, &xrmValue ) == 0 ) {
            config_parser_set ( &( xrmOptions[i] ), xrmValue, source );
        }
        if ( xrmValue ) {
            free ( xrmValue );
        }

        g_free ( name );
    }
}
static void __config_parse_xresource_options_dynamic ( xcb_xrm_database_t *xDB, enum ConfigSource source )
{
    const char * namePrefix = "rofi";

    for ( unsigned int i = 0; i < num_extra_options; ++i ) {
        char *name;

        name = g_strdup_printf ( "%s.%s", namePrefix, extra_options[i].name );
        char *xrmValue = NULL;
        if ( xcb_xrm_resource_get_string ( xDB, name, NULL, &xrmValue ) == 0 ) {
            config_parser_set ( &( extra_options[i] ), xrmValue, source );
        }
        if ( xrmValue ) {
            free ( xrmValue );
        }

        g_free ( name );
    }
}

void config_parse_xresource_options_file ( const char *filename )
{
    if ( !filename ) {
        return;
    }
    // Map Xresource entries to rofi config options.
    xcb_xrm_database_t *xDB = xcb_xrm_database_from_file ( filename );
    if ( xDB == NULL ) {
        return;
    }
    __config_parse_xresource_options ( xDB, CONFIG_FILE );
    __config_parse_xresource_options_dynamic ( xDB, CONFIG_FILE );
    xcb_xrm_database_free ( xDB );
}

/**
 * Parse an option from the commandline vector.
 */
static void config_parse_cmd_option ( XrmOption *option )
{
    // Prepend a - to the option name.
    char *key = g_strdup_printf ( "-%s", option->name );
    switch ( option->type )
    {
    case xrm_Number:
        if ( find_arg_uint ( key, option->value.num ) == TRUE ) {
            option->source = CONFIG_CMDLINE;
        }
        break;
    case xrm_SNumber:
        if ( find_arg_int (  key, option->value.snum ) == TRUE ) {
            option->source = CONFIG_CMDLINE;
        }
        break;
    case xrm_String:
        if ( find_arg_str (  key, option->value.str ) == TRUE ) {
            if ( option->mem != NULL ) {
                g_free ( option->mem );
                option->mem = NULL;
            }
            option->source = CONFIG_CMDLINE;
        }
        break;
    case xrm_Boolean:
        if ( find_arg (  key ) >= 0 ) {
            *( option->value.num ) = TRUE;
            option->source         = CONFIG_CMDLINE;
        }
        else {
            g_free ( key );
            key = g_strdup_printf ( "-no-%s", option->name );
            if ( find_arg (  key ) >= 0 ) {
                *( option->value.num ) = FALSE;
                option->source         = CONFIG_CMDLINE;
            }
        }
        break;
    case xrm_Char:
        if ( find_arg_char (  key, option->value.charc ) == TRUE ) {
            option->source = CONFIG_CMDLINE;
        }
        break;
    default:
        break;
    }
    g_free ( key );
}

void config_parse_cmd_options ( void )
{
    for ( unsigned int i = 0; i < sizeof ( xrmOptions ) / sizeof ( XrmOption ); ++i ) {
        XrmOption *op = &( xrmOptions[i] );
        config_parse_cmd_option ( op );
    }
    for ( unsigned int i = 0; i < num_extra_options; ++i ) {
        XrmOption *op = &( extra_options[i] );
        config_parse_cmd_option ( op  );
    }
}

static gboolean __config_parser_set_property ( XrmOption *option, const Property *p, char **error  )
{
    if ( option->type == xrm_String ) {
        if ( p->type != P_STRING && p->type != P_LIST ) {
            *error = g_strdup_printf ( "Option: %s needs to be set with a string not a %s.", option->name, PropertyTypeName[p->type] );
            return TRUE;
        }
        gchar *value = NULL;
        if ( p->type == P_LIST ) {
            for ( GList *iter = p->value.list; iter != NULL; iter = g_list_next ( iter ) ) {
                if ( value == NULL ) {
                    value = g_strdup ( (char *) ( iter->data ) );
                }
                else {
                    char *nv = g_strjoin ( ",", value, (char *) ( iter->data ), NULL );
                    g_free ( value );
                    value = nv;
                }
            }
        }
        else {
            value = g_strdup ( p->value.s );
        }
        if ( ( option )->mem != NULL ) {
            g_free ( option->mem );
            option->mem = NULL;
        }
        *( option->value.str ) = value;

        // Memory
        ( option )->mem = *( option->value.str );
        option->source  = CONFIG_FILE_THEME;
    }
    else if ( option->type == xrm_Number ) {
        if ( p->type != P_INTEGER ) {
            *error = g_strdup_printf ( "Option: %s needs to be set with a number not a %s.", option->name, PropertyTypeName[p->type] );
            return TRUE;
        }
        *( option->value.snum ) = p->value.i;
        option->source          = CONFIG_FILE_THEME;
    }
    else if ( option->type == xrm_SNumber ) {
        if ( p->type != P_INTEGER ) {
            *error = g_strdup_printf ( "Option: %s needs to be set with a number not a %s.", option->name, PropertyTypeName[p->type] );
            return TRUE;
        }
        *( option->value.num ) = (unsigned int ) ( p->value.i );
        option->source         = CONFIG_FILE_THEME;
    }
    else if ( option->type == xrm_Boolean ) {
        if ( p->type != P_BOOLEAN ) {
            *error = g_strdup_printf ( "Option: %s needs to be set with a boolean not a %s.", option->name, PropertyTypeName[p->type] );
            return TRUE;
        }
        *( option->value.num ) = ( p->value.b );
        option->source         = CONFIG_FILE_THEME;
    }
    else if ( option->type == xrm_Char ) {
        if ( p->type != P_CHAR ) {
            *error = g_strdup_printf ( "Option: %s needs to be set with a character not a %s.", option->name, PropertyTypeName[p->type] );
            return TRUE;
        }
        *( option->value.charc ) = ( p->value.c );
        option->source           = CONFIG_FILE_THEME;
    }
    else {
        // TODO add type
        *error = g_strdup_printf ( "Option: %s is not of a supported type: %s.", option->name, PropertyTypeName[p->type] );
        return TRUE;
    }
    return FALSE;
}

gboolean config_parse_set_property ( const Property *p, char **error )
{
    for ( unsigned int i = 0; i < sizeof ( xrmOptions ) / sizeof ( XrmOption ); ++i ) {
        XrmOption *op = &( xrmOptions[i] );
        if ( g_strcmp0 ( op->name, p->name ) == 0 ) {
            return __config_parser_set_property ( op, p, error );
        }
    }
    for ( unsigned int i = 0; i < num_extra_options; ++i ) {
        XrmOption *op = &( extra_options[i] );
        if ( g_strcmp0 ( op->name, p->name ) == 0 ) {
            return __config_parser_set_property ( op, p, error );
        }
    }
    *error = g_strdup_printf ( "Option: %s is not found.", p->name );
    return TRUE;
}

void config_xresource_free ( void )
{
    for ( unsigned int i = 0; i < ( sizeof ( xrmOptions ) / sizeof ( *xrmOptions ) ); ++i ) {
        if ( xrmOptions[i].mem != NULL ) {
            g_free ( xrmOptions[i].mem );
            xrmOptions[i].mem = NULL;
        }
    }
    for ( unsigned int i = 0; i < num_extra_options; ++i ) {
        if ( extra_options[i].mem != NULL ) {
            g_free ( extra_options[i].mem );
            extra_options[i].mem = NULL;
        }
    }
    if ( extra_options != NULL ) {
        g_free ( extra_options );
    }
}

static void config_parse_dump_config_option ( FILE *out, XrmOption *option )
{
    if ( option->type == xrm_Char || option->source == CONFIG_DEFAULT ) {
        fprintf ( out, "/*" );
    }
    fprintf ( out, "\t%s: ", option->name );
    switch ( option->type )
    {
    case xrm_Number:
        fprintf ( out, "%u", *( option->value.num ) );
        break;
    case xrm_SNumber:
        fprintf ( out, "%i", *( option->value.snum ) );
        break;
    case xrm_String:
        if ( ( *( option->value.str ) ) != NULL ) {
            // TODO should this be escaped?
            fprintf ( out, "\"%s\"", *( option->value.str ) );
        }
        break;
    case xrm_Boolean:
        fprintf ( out, "%s", ( *( option->value.num ) == TRUE ) ? "true" : "false" );
        break;
    case xrm_Char:
        // TODO
        if ( *( option->value.charc ) > 32 && *( option->value.charc ) < 127 ) {
            fprintf ( out, "'%c'", *( option->value.charc ) );
        }
        else {
            fprintf ( out, "'\\x%02X'", *( option->value.charc ) );
        }
        fprintf ( out, " /* unsupported */" );
        break;
    default:
        break;
    }

    fprintf ( out, ";" );
    if ( option->type == xrm_Char || option->source == CONFIG_DEFAULT ) {
        fprintf ( out, "*/" );
    }
    fprintf ( out, "\n" );
}

void config_parse_dump_config_rasi_format ( FILE *out, gboolean changes )
{
    fprintf ( out, "configuration {\n" );

    unsigned int entries = sizeof ( xrmOptions ) / sizeof ( *xrmOptions );
    for ( unsigned int i = 0; i < entries; ++i ) {
        // Skip duplicates.
        if ( ( i + 1 ) < entries ) {
            if ( xrmOptions[i].value.str == xrmOptions[i + 1].value.str ) {
                continue;
            }
        }
        if ( !changes || xrmOptions[i].source != CONFIG_DEFAULT ) {
            config_parse_dump_config_option ( out, &( xrmOptions[i] ) );
        }
    }
    for ( unsigned int i = 0; i < num_extra_options; i++ ) {
        if ( !changes || extra_options[i].source != CONFIG_DEFAULT ) {
            config_parse_dump_config_option ( out, &( extra_options[i] ) );
        }
    }

    fprintf ( out, "}\n" );
}

static void print_option_string ( XrmOption *xo, int is_term )
{
    int l = strlen ( xo->name );
    if ( is_term ) {
        printf ( "\t"color_bold "-%s"color_reset " [string]%-*c%s\n", xo->name, 30 - l, ' ', xo->comment );
        printf ( "\t"color_italic "%s"color_reset, ( *( xo->value.str ) == NULL ) ? "(unset)" : ( *( xo->value.str ) ) );
        printf ( " "color_green "(%s)"color_reset "\n", ConfigSourceStr[xo->source] );
    }
    else {
        printf ( "\t-%s [string]%-*c%s\n", xo->name, 30 - l, ' ', xo->comment );
        printf ( "\t\t%s", ( *( xo->value.str ) == NULL ) ? "(unset)" : ( *( xo->value.str ) ) );
        printf ( " (%s)\n", ConfigSourceStr[xo->source] );
    }
}
static void print_option_number ( XrmOption *xo, int is_term )
{
    int l = strlen ( xo->name );
    if ( is_term ) {
        printf ( "\t"color_bold "-%s"color_reset " [number]%-*c%s\n", xo->name, 30 - l, ' ', xo->comment );
        printf ( "\t"color_italic "%u"color_reset, *( xo->value.num ) );
        printf ( " "color_green "(%s)"color_reset "\n", ConfigSourceStr[xo->source] );
    }
    else {
        printf ( "\t-%s [number]%-*c%s\n", xo->name, 30 - l, ' ', xo->comment );
        printf ( "\t\t%u", *( xo->value.num ) );
        printf ( " (%s)\n", ConfigSourceStr[xo->source] );
    }
}
static void print_option_snumber ( XrmOption *xo, int is_term )
{
    int l = strlen ( xo->name );
    if ( is_term ) {
        printf ( "\t"color_bold "-%s"color_reset " [number]%-*c%s\n", xo->name, 30 - l, ' ', xo->comment );
        printf ( "\t"color_italic "%d"color_reset, *( xo->value.snum ) );
        printf ( " "color_green "(%s)"color_reset "\n", ConfigSourceStr[xo->source] );
    }
    else {
        printf ( "\t-%s [number]%-*c%s\n", xo->name, 30 - l, ' ', xo->comment );
        printf ( "\t\t%d", *( xo->value.snum ) );
        printf ( " (%s)\n", ConfigSourceStr[xo->source] );
    }
}
static void print_option_char ( XrmOption *xo, int is_term )
{
    int l = strlen ( xo->name );
    if ( is_term ) {
        printf ( "\t"color_bold "-%s"color_reset " [character]%-*c%s\n", xo->name, 30 - l, ' ', xo->comment );
        printf ( "\t"color_italic "%c"color_reset, *( xo->value.charc ) );
        printf ( " "color_green "(%s)"color_reset "\n", ConfigSourceStr[xo->source] );
    }
    else {
        printf ( "\t-%s [character]%-*c%s\n", xo->name, 30 - l, ' ', xo->comment );
        printf ( "\t\t%c", *( xo->value.charc ) );
        printf ( " (%s)\n", ConfigSourceStr[xo->source] );
    }
}
static void print_option_boolean ( XrmOption *xo, int is_term )
{
    int l = strlen ( xo->name );
    if ( is_term ) {
        printf ( "\t"color_bold "-[no-]%s"color_reset " %-*c%s\n", xo->name, 33 - l, ' ', xo->comment );
        printf ( "\t"color_italic "%s"color_reset, ( *( xo->value.snum ) ) ? "True" : "False" );
        printf ( " "color_green "(%s)"color_reset "\n", ConfigSourceStr[xo->source] );
    }
    else {
        printf ( "\t-[no-]%s %-*c%s\n", xo->name, 33 - l, ' ', xo->comment );
        printf ( "\t\t%s", ( *( xo->value.snum ) ) ? "True" : "False" );
        printf ( " (%s)\n", ConfigSourceStr[xo->source] );
    }
}

static void print_option ( XrmOption *xo, int is_term )
{
    switch ( xo->type )
    {
    case xrm_String:
        print_option_string ( xo, is_term );
        break;
    case xrm_Number:
        print_option_number ( xo, is_term );
        break;
    case xrm_SNumber:
        print_option_snumber ( xo, is_term );
        break;
    case xrm_Boolean:
        print_option_boolean ( xo, is_term );
        break;
    case xrm_Char:
        print_option_char ( xo, is_term );
        break;
    default:
        break;
    }
}
void print_options ( void )
{
    // Check output filedescriptor
    int          is_term = isatty ( fileno ( stdout ) );
    unsigned int entries = sizeof ( xrmOptions ) / sizeof ( *xrmOptions );
    for ( unsigned int i = 0; i < entries; ++i ) {
        if ( ( i + 1 ) < entries ) {
            if ( xrmOptions[i].value.str == xrmOptions[i + 1].value.str ) {
                continue;
            }
        }
        print_option ( &xrmOptions[i], is_term );
    }
    for ( unsigned int i = 0; i < num_extra_options; i++ ) {
        print_option ( &extra_options[i], is_term );
    }
}

void print_help_msg ( const char *option, const char *type, const char*text, const char *def, int isatty )
{
    int l = 37 - strlen ( option ) - strlen ( type );
    if ( isatty ) {
        printf ( "\t%s%s%s %s %-*c%s\n", color_bold, option, color_reset, type, l, ' ', text );
        if ( def != NULL ) {
            printf ( "\t\t%s%s%s\n", color_italic, def, color_reset );
        }
    }
    else{
        printf ( "\t%s %s %-*c%s\n", option, type, l, ' ', text );
        if ( def != NULL ) {
            printf ( "\t\t%s\n", def );
        }
    }
}

static char * config_parser_return_display_help_entry ( XrmOption *option, size_t l )
{
    int ll = (int) l;
    switch ( option->type )
    {
    case xrm_Number:
        return g_markup_printf_escaped ( "<b%-*s</b> (%u) <span style='italic' size='small'>%s</span>",
                                         ll, option->name, *( option->value.num ), option->comment );
    case xrm_SNumber:
        return g_markup_printf_escaped ( "<b%-*s</b> (%d) <span style='italic' size='small'>%s</span>",
                                         ll, option->name, *( option->value.snum ), option->comment );
    case xrm_String:
        return g_markup_printf_escaped ( "<b>%-*s</b> (%s) <span style='italic' size='small'>%s</span>",
                                         ll, option->name,
                                         ( *( option->value.str ) != NULL ) ? *( option->value.str ) : "null",
                                         option->comment
                                         );
    case xrm_Boolean:
        return g_markup_printf_escaped ( "<b>%-*s</b> (%s) <span style='italic' size='small'>%s</span>",
                                         ll, option->name, ( *( option->value.num ) == TRUE ) ? "true" : "false", option->comment );
    case xrm_Char:
        if ( *( option->value.charc ) > 32 && *( option->value.charc ) < 127 ) {
            return g_markup_printf_escaped ( "<b>%-*s</b> (%c) <span style='italic' size='small'>%s</span>",
                                             ll, option->name, *( option->value.charc ), option->comment );
        }
        else {
            return g_markup_printf_escaped ( "<b%-*s</b> (\\x%02X) <span style='italic' size='small'>%s</span>",
                                             ll, option->name, *( option->value.charc ), option->comment );
        }
    default:
        break;
    }

    return g_strdup ( "failed" );
}

char ** config_parser_return_display_help ( unsigned int *length )
{
    unsigned int entries = sizeof ( xrmOptions ) / sizeof ( *xrmOptions );
    char         **retv  = NULL;
    /**
     * Get length of name
     */
    size_t max_length = 0;
    for ( unsigned int i = 0; i < entries; ++i ) {
        size_t l = strlen ( xrmOptions[i].name );
        max_length = MAX ( max_length, l );
    }
    for ( unsigned int i = 0; i < num_extra_options; i++ ) {
        size_t l = strlen ( extra_options[i].name );
        max_length = MAX ( max_length, l );
    }
    /**
     * Generate entries
     */
    for ( unsigned int i = 0; i < entries; ++i ) {
        if ( ( i + 1 ) < entries ) {
            if ( xrmOptions[i].value.str == xrmOptions[i + 1].value.str ) {
                continue;
            }
        }
        if ( strncmp ( xrmOptions[i].name, "kb", 2 ) != 0 && strncmp ( xrmOptions[i].name, "ml", 2 ) != 0 && strncmp ( xrmOptions[i].name, "me", 2 ) != 0 ) {
            continue;
        }

        retv = g_realloc ( retv, ( ( *length ) + 2 ) * sizeof ( char* ) );

        retv[( *length )] = config_parser_return_display_help_entry ( &xrmOptions[i], max_length );
        ( *length )++;
    }
    for ( unsigned int i = 0; i < num_extra_options; i++ ) {
        if ( strncmp ( extra_options[i].name, "kb", 2 ) != 0 && strncmp ( extra_options[i].name, "ml", 2 ) != 0 && strncmp ( extra_options[i].name, "me", 2 ) != 0 ) {
            continue;
        }
        retv              = g_realloc ( retv, ( ( *length ) + 2 ) * sizeof ( char* ) );
        retv[( *length )] = config_parser_return_display_help_entry ( &extra_options[i], max_length );
        ( *length )++;
    }
    if ( ( *length ) > 0 ) {
        retv[( *length )] = NULL;
    }
    return retv;
}
