/**
 * rofi
 *
 * MIT/X11 License
 * Copyright 2013-2015 Qball Cow <qball@gmpclient.org>
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
#include <X11/X.h>
#include <X11/Xresource.h>
#include "rofi.h"
#include "xrmoptions.h"
#include "helper.h"

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
    }          value;
    char       *mem;
    const char *comment;
} XrmOption;
/**
 * Map X resource and commandline options to internal options
 * Currently supports string, boolean and number (signed and unsigned).
 */
static XrmOption xrmOptions[] = {
    { xrm_String,  "switchers",            { .str  = &config.modi               }, NULL, ""                                            },
    { xrm_String,  "modi",                 { .str  = &config.modi               }, NULL, "Enabled modi"                                },
    { xrm_Number,  "opacity",              { .num  = &config.window_opacity     }, NULL, "Window opacity"                              },
    { xrm_SNumber, "width",                { .snum = &config.menu_width         }, NULL, "Window width"                                },
    { xrm_Number,  "lines",                { .num  = &config.menu_lines         }, NULL, "Number of lines"                             },
    { xrm_Number,  "columns",              { .num  = &config.menu_columns       }, NULL, "Number of columns"                           },

    { xrm_String,  "font",                 { .str  = &config.menu_font          }, NULL, "Font to use"                                 },
    /* Foreground color */
    { xrm_String,  "foreground",           { .str  = &config.menu_fg            }, NULL, ""                                            },
    { xrm_String,  "fg",                   { .str  = &config.menu_fg            }, NULL, "Foreground color"                            },
    { xrm_String,  "background",           { .str  = &config.menu_bg            }, NULL, ""                                            },
    { xrm_String,  "bg",                   { .str  = &config.menu_bg            }, NULL, "Background color"                            },

    { xrm_String,  "fg-active",            { .str  = &config.menu_fg_active     }, NULL, "Foreground color for active row"             },
    { xrm_String,  "fg-urgent",            { .str  = &config.menu_fg_urgent     }, NULL, "Foreground color for urgent row"             },
    { xrm_String,  "hlfg-active",          { .str  = &config.menu_hlfg_active   }, NULL, "Foreground color for highlighted active row" },
    { xrm_String,  "hlfg-urgent",          { .str  = &config.menu_hlfg_urgent   }, NULL, "Foreground color for highlighted urgent row" },

    { xrm_String,  "bg-active",            { .str  = &config.menu_bg_active     }, NULL, "Background color for active row"             },
    { xrm_String,  "bg-urgent",            { .str  = &config.menu_bg_urgent     }, NULL, "Background color for urgent row"             },
    { xrm_String,  "hlbg-active",          { .str  = &config.menu_hlbg_active   }, NULL, "Background color for highlighted active row" },
    { xrm_String,  "hlbg-urgent",          { .str  = &config.menu_hlbg_urgent   }, NULL, "Background color for highlighted urgent row" },

    { xrm_String,  "background-alternate", { .str  = &config.menu_bg_alt        }, NULL, ""                                            },
    { xrm_String,  "bgalt",                { .str  = &config.menu_bg_alt        }, NULL, "Background color for alternating row"        },

    { xrm_String,  "highlightfg",          { .str  = &config.menu_hlfg          }, NULL, ""                                            },
    { xrm_String,  "hlfg",                 { .str  = &config.menu_hlfg          }, NULL, "Foreground color for highlighted row"        },

    { xrm_String,  "highlightbg",          { .str  = &config.menu_hlbg          }, NULL, ""                                            },
    { xrm_String,  "hlbg",                 { .str  = &config.menu_hlbg          }, NULL, "Background color for highlighted row"        },

    { xrm_String,  "bordercolor",          { .str  = &config.menu_bc            }, NULL, ""                                            },
    { xrm_String,  "bc",                   { .str  = &config.menu_bc            }, NULL, "Border color"                                },
    { xrm_Boolean, "color-enabled",        { .num  = &config.color_enabled      }, NULL, "Use extended color scheme"                   },
    { xrm_String,  "color-normal",         { .str  = &config.color_normal       }, NULL, "Color scheme for normal row"                 },
    { xrm_String,  "color-urgent",         { .str  = &config.color_urgent       }, NULL, "Color scheme for urgent row"                 },
    { xrm_String,  "color-active",         { .str  = &config.color_active       }, NULL, "Color scheme for active row"                 },
    { xrm_String,  "color-window",         { .str  = &config.color_window       }, NULL, "Color scheme window"                         },

    { xrm_Number,  "borderwidth",          { .num  = &config.menu_bw            }, NULL, ""                                            },
    { xrm_Number,  "bw",                   { .num  = &config.menu_bw            }, NULL, "Border width"                                },

    { xrm_Number,  "location",             { .num  = &config.location           }, NULL, "Location on screen"                          },

    { xrm_Number,  "padding",              { .num  = &config.padding            }, NULL, "Padding"                                     },
    { xrm_SNumber, "yoffset",              { .snum = &config.y_offset           }, NULL, "Y-offset relative to location"               },
    { xrm_SNumber, "xoffset",              { .snum = &config.x_offset           }, NULL, "X-offset relative to location"               },
    { xrm_Boolean, "fixed-num-lines",      { .num  = &config.fixed_num_lines    }, NULL, "Always show number of lines"                 },

    { xrm_String,  "terminal",             { .str  = &config.terminal_emulator  }, NULL, "Terminal to use"                             },
    { xrm_String,  "ssh-client",           { .str  = &config.ssh_client         }, NULL, "Ssh client to use"                           },
    { xrm_String,  "ssh-command",          { .str  = &config.ssh_command        }, NULL, "Ssh command to execute"                      },
    { xrm_String,  "run-command",          { .str  = &config.run_command        }, NULL, "Run command to execute"                      },
    { xrm_String,  "run-list-command",     { .str  = &config.run_list_command   }, NULL, "Command to get extra run targets"            },
    { xrm_String,  "run-shell-command",    { .str  = &config.run_shell_command  }, NULL, "Run command to execute that runs in shell"   },

    { xrm_Boolean, "disable-history",      { .num  = &config.disable_history    }, NULL, "Disable history in run/ssh"                  },
    { xrm_Boolean, "levenshtein-sort",     { .num  = &config.levenshtein_sort   }, NULL, "Use levenshtein sorting"                     },
    { xrm_Boolean, "case-sensitive",       { .num  = &config.case_sensitive     }, NULL, "Set case-sensitivity"                        },
    { xrm_Boolean, "sidebar-mode",         { .num  = &config.sidebar_mode       }, NULL, "Enable sidebar-mode"                         },
    { xrm_Number,  "lazy-filter-limit",    { .num  = &config.lazy_filter_limit  }, NULL, "Set lazy filter limit"                       },
    { xrm_SNumber, "eh",                   { .snum = &config.element_height     }, NULL, "Row height (in chars)"                       },
    { xrm_Boolean, "auto-select",          { .num  = &config.auto_select        }, NULL, "Enable auto select mode"                     },
    { xrm_Boolean, "parse-hosts",          { .num  = &config.parse_hosts        }, NULL, "Parse hosts file for ssh mode"               },
    { xrm_Boolean, "parse-known-hosts",    { .num  = &config.parse_known_hosts  }, NULL, "Parse known_hosts file for ssh mode"         },
    { xrm_String,  "combi-modi",           { .str  = &config.combi_modi         }, NULL, "Set the modi to combine in combi mode"       },
    { xrm_Boolean, "fuzzy",                { .num  = &config.fuzzy              }, NULL, "Do a more fuzzy matching"                    },
    { xrm_Boolean, "glob",                 { .num  = &config.glob               }, NULL, "Use glob matching"                           },
    { xrm_Boolean, "regex",                { .num  = &config.regex              }, NULL, "Use regex matching"                          },
    { xrm_Boolean, "tokenize",             { .num  = &config.tokenize           }, NULL, "Tokenize input string"                       },
    { xrm_Number,  "monitor",              { .snum = &config.monitor            }, NULL, ""                                            },
    /* Alias for dmenu compatibility. */
    { xrm_SNumber, "m",                    { .snum = &config.monitor            }, NULL, "Monitor id to show on"                       },
    { xrm_Number,  "line-margin",          { .num  = &config.line_margin        }, NULL, "Margin between rows"                         },
    { xrm_String,  "filter",               { .str  = &config.filter             }, NULL, "Pre-set filter"                              },
    { xrm_String,  "separator-style",      { .str  = &config.separator_style    }, NULL, "Separator style (none, dash, solid)"         },
    { xrm_Boolean, "hide-scrollbar",       { .num  = &config.hide_scrollbar     }, NULL, "Hide scroll-bar"                             },
    { xrm_Boolean, "markup-rows",          { .num  = &config.markup_rows        }, NULL, "Show markup"                                 },
    { xrm_Boolean, "fullscreen",           { .num  = &config.fullscreen         }, NULL, "Fullscreen"                                  },
    { xrm_Boolean, "fake-transparency",    { .num  = &config.fake_transparency  }, NULL, "Fake transparency"                           },
    { xrm_SNumber, "dpi",                  { .snum = &config.dpi                }, NULL, "DPI"                                         },
    { xrm_Number,  "threads",              { .num  = &config.threads            }, NULL, "Threads to use for string matching"          },
    { xrm_Number,  "scrollbar-width",      { .num  = &config.scrollbar_width    }, NULL, "Scrollbar width"                             }
};

// Dynamic options.
XrmOption    *extra_options    = NULL;
unsigned int num_extra_options = 0;

void config_parser_add_option ( XrmOptionType type, const char *key, void **value, const char *comment )
{
    extra_options = g_realloc ( extra_options, ( num_extra_options + 1 ) * sizeof ( XrmOption ) );

    extra_options[num_extra_options].type          = type;
    extra_options[num_extra_options].name          = key;
    extra_options[num_extra_options].value.pointer = value;
    extra_options[num_extra_options].comment       = comment;
    if ( type == xrm_String ) {
        extra_options[num_extra_options].mem = ( (char *) ( *value ) );
    }
    else {
        extra_options[num_extra_options].mem = NULL;
    }

    num_extra_options++;
}

static void config_parser_set ( XrmOption *option, XrmValue *xrmValue )
{
    if ( option->type == xrm_String ) {
        if ( ( option )->mem != NULL ) {
            g_free ( option->mem );
            option->mem = NULL;
        }
        *( option->value.str ) = g_strndup ( xrmValue->addr, xrmValue->size );

        // Memory
        ( option )->mem = *( option->value.str );
    }
    else if ( option->type == xrm_Number ) {
        *( option->value.num ) = (unsigned int) strtoul ( xrmValue->addr, NULL, 10 );
    }
    else if ( option->type == xrm_SNumber ) {
        *( option->value.snum ) = (int) strtol ( xrmValue->addr, NULL, 10 );
    }
    else if ( option->type == xrm_Boolean ) {
        if ( xrmValue->size > 0 &&
             g_ascii_strncasecmp ( xrmValue->addr, "true", xrmValue->size ) == 0 ) {
            *( option->value.num ) = TRUE;
        }
        else{
            *( option->value.num ) = FALSE;
        }
    }
    else if ( option->type == xrm_Char ) {
        *( option->value.charc ) = helper_parse_char ( xrmValue->addr );
    }
}

void config_parse_xresource_options ( Display *display )
{
    char *xRMS;
    // Map Xresource entries to rofi config options.
    XrmInitialize ();
    xRMS = XResourceManagerString ( display );

    if ( xRMS == NULL ) {
        return;
    }
    XrmDatabase xDB = XrmGetStringDatabase ( xRMS );

    char        * xrmType;
    XrmValue    xrmValue;
    const char  * namePrefix  = "rofi";
    const char  * classPrefix = "rofi";

    for ( unsigned int i = 0; i < sizeof ( xrmOptions ) / sizeof ( XrmOption ); ++i ) {
        char *name, *class;

        name  = g_strdup_printf ( "%s.%s", namePrefix, xrmOptions[i].name );
        class = g_strdup_printf ( "%s.%s", classPrefix, xrmOptions[i].name );

        if ( XrmGetResource ( xDB, name, class, &xrmType, &xrmValue ) ) {
            config_parser_set ( &( xrmOptions[i] ), &xrmValue );
        }

        g_free ( class );
        g_free ( name );
    }
    XrmDestroyDatabase ( xDB );
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
        find_arg_uint ( key, option->value.num );
        break;
    case xrm_SNumber:
        find_arg_int (  key, option->value.snum );
        break;
    case xrm_String:
        if ( find_arg_str (  key, option->value.str ) == TRUE ) {
            if ( option->mem != NULL ) {
                g_free ( option->mem );
                option->mem = NULL;
            }
        }
        break;
    case xrm_Boolean:
        if ( find_arg (  key ) >= 0 ) {
            *( option->value.num ) = TRUE;
        }
        else {
            g_free ( key );
            key = g_strdup_printf ( "-no-%s", option->name );
            if ( find_arg (  key ) >= 0 ) {
                *( option->value.num ) = FALSE;
            }
        }
        break;
    case xrm_Char:
        find_arg_char (  key, option->value.charc );
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
}

void config_parse_cmd_options_dynamic ( void )
{
    for ( unsigned int i = 0; i < num_extra_options; ++i ) {
        XrmOption *op = &( extra_options[i] );
        config_parse_cmd_option ( op  );
    }
}

void config_parse_xresource_options_dynamic ( Display *display )
{
    char *xRMS;
    // Map Xresource entries to rofi config options.
    XrmInitialize ();
    xRMS = XResourceManagerString ( display );

    if ( xRMS == NULL ) {
        return;
    }
    XrmDatabase xDB = XrmGetStringDatabase ( xRMS );

    char        * xrmType;
    XrmValue    xrmValue;
    const char  * namePrefix  = "rofi";
    const char  * classPrefix = "rofi";

    for ( unsigned int i = 0; i < num_extra_options; ++i ) {
        char *name, *class;

        name  = g_strdup_printf ( "%s.%s", namePrefix, extra_options[i].name );
        class = g_strdup_printf ( "%s.%s", classPrefix, extra_options[i].name );
        if ( XrmGetResource ( xDB, name, class, &xrmType, &xrmValue ) ) {
            config_parser_set ( &( extra_options[i] ), &xrmValue );
        }

        g_free ( class );
        g_free ( name );
    }
    XrmDestroyDatabase ( xDB );
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

static void xresource_dump_entry ( const char *namePrefix, XrmOption *option )
{
    printf ( "! %s\n", option->comment );
    printf ( "%s.%s: %*s", namePrefix, option->name,
             (int) ( 30 - strlen ( option->name ) ), "" );
    switch ( option->type )
    {
    case xrm_Number:
        printf ( "%u", *( option->value.num ) );
        break;
    case xrm_SNumber:
        printf ( "%i", *( option->value.snum ) );
        break;
    case xrm_String:
        if ( ( *( option->value.str ) ) != NULL ) {
            printf ( "%s", *( option->value.str ) );
        }
        break;
    case xrm_Boolean:
        printf ( "%s", ( *( option->value.num ) == TRUE ) ? "true" : "false" );
        break;
    case xrm_Char:
        if ( *( option->value.charc ) > 32 && *( option->value.charc ) < 127 ) {
            printf ( "%c", *( option->value.charc ) );
        }
        else {
            printf ( "\\x%02X", *( option->value.charc ) );
        }
        break;
    default:
        break;
    }
    printf ( "\n" );
}

void xresource_dump ( void )
{
    const char   * namePrefix = "rofi";
    unsigned int entries      = sizeof ( xrmOptions ) / sizeof ( *xrmOptions );
    for ( unsigned int i = 0; i < entries; ++i ) {
        // Skip duplicates.
        if ( ( i + 1 ) < entries ) {
            if ( xrmOptions[i].value.str == xrmOptions[i + 1].value.str ) {
                continue;
            }
        }
        xresource_dump_entry ( namePrefix, &( xrmOptions[i] ) );
    }
    for ( unsigned int i = 0; i < num_extra_options; i++ ) {
        xresource_dump_entry ( namePrefix, &( extra_options[i] ) );
    }
}

static void print_option_string ( XrmOption *xo, int is_term )
{
    int l = strlen ( xo->name );
    if ( is_term ) {
        printf ( "\t"color_bold "-%s"color_reset " [string]%-*c%s\n", xo->name, 30 - l, ' ', xo->comment );
        printf ( "\t\t"color_italic "%s"color_reset "\n", ( *( xo->value.str ) == NULL ) ? "(unset)" : ( *( xo->value.str ) ) );
    }
    else {
        printf ( "\t-%s [string]%-*c%s\n", xo->name, 30 - l, ' ', xo->comment );
        printf ( "\t\t%s\n", ( *( xo->value.str ) == NULL ) ? "(unset)" : ( *( xo->value.str ) ) );
    }
}
static void print_option_number ( XrmOption *xo, int is_term )
{
    int l = strlen ( xo->name );
    if ( is_term ) {
        printf ( "\t"color_bold "-%s"color_reset " [number]%-*c%s\n", xo->name, 30 - l, ' ', xo->comment );
        printf ( "\t\t"color_italic "%u"color_reset "\n", *( xo->value.num ) );
    }
    else {
        printf ( "\t-%s [number]%-*c%s\n", xo->name, 30 - l, ' ', xo->comment );
        printf ( "\t\t%u\n", *( xo->value.num ) );
    }
}
static void print_option_snumber ( XrmOption *xo, int is_term )
{
    int l = strlen ( xo->name );
    if ( is_term ) {
        printf ( "\t"color_bold "-%s"color_reset " [number]%-*c%s\n", xo->name, 30 - l, ' ', xo->comment );
        printf ( "\t\t"color_italic "%d"color_reset "\n", *( xo->value.snum ) );
    }
    else {
        printf ( "\t-%s [number]%-*c%s\n", xo->name, 30 - l, ' ', xo->comment );
        printf ( "\t\t%d\n", *( xo->value.snum ) );
    }
}
static void print_option_char ( XrmOption *xo, int is_term )
{
    int l = strlen ( xo->name );
    if ( is_term ) {
        printf ( "\t"color_bold "-%s"color_reset " [character]%-*c%s\n", xo->name, 30 - l, ' ', xo->comment );
        printf ( "\t\t"color_italic "%c"color_reset "\n", *( xo->value.charc ) );
    }
    else {
        printf ( "\t-%s [character]%-*c%s\n", xo->name, 30 - l, ' ', xo->comment );
        printf ( "\t\t%c\n", *( xo->value.charc ) );
    }
}
static void print_option_boolean ( XrmOption *xo, int is_term )
{
    int l = strlen ( xo->name );
    if ( is_term ) {
        printf ( "\t"color_bold "-[no-]%s"color_reset " %-*c%s\n", xo->name, 33 - l, ' ', xo->comment );
        printf ( "\t\t"color_italic "%s"color_reset "\n", ( *( xo->value.snum ) ) ? "True" : "False" );
    }
    else {
        printf ( "\t-[no-]%s %-*c%s\n", xo->name, 33 - l, ' ', xo->comment );
        printf ( "\t\t%s\n", ( *( xo->value.snum ) ) ? "True" : "False" );
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

void print_xresources_theme ( void )
{
    if ( config.color_enabled != TRUE ) {
        printf ( "! Dumping theme only works for the extended color scheme.\n" );
        return;
    }
    printf ( "! ------------------------------------------------------------------------------\n" );
    printf ( "! ROFI Color theme\n" );
    printf ( "! ------------------------------------------------------------------------------\n" );
    const char   * namePrefix = "rofi";
    unsigned int entries      = sizeof ( xrmOptions ) / sizeof ( *xrmOptions );
    for ( unsigned int i = 0; i < entries; ++i ) {
        if ( strncmp ( xrmOptions[i].name, "color-", 6 ) == 0 ) {
            xresource_dump_entry ( namePrefix, &xrmOptions[i] );
        }
        else if ( strcmp ( xrmOptions[i].name, "separator-style" ) == 0 ) {
            xresource_dump_entry ( namePrefix, &xrmOptions[i] );
        }
    }
}
