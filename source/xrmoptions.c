/**
 * rofi
 *
 * MIT/X11 License
 * Copyright 2013-2014 Qball  Cow <qball@gmpclient.org>
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
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/X.h>
#include <X11/Xresource.h>
#include "rofi.h"
#include "xrmoptions.h"

// Big thanks to Sean Pringle for this code.
// This maps xresource options to config structure.
typedef enum
{
    xrm_String = 0,
    xrm_Number = 1
} XrmOptionType;

typedef struct
{
    int  type;
    char * name;
    union
    {
        unsigned int * num;
        char         ** str;
    };
    char *mem;
} XrmOption;
/**
 * Map X resource settings to internal options
 * Currently supports string and number.
 */
static XrmOption xrmOptions[] = {
    { xrm_Number, "opacity",         { .num = &config.window_opacity    }, NULL },
    { xrm_Number, "width",           { .num = &config.menu_width        }, NULL },
    { xrm_Number, "lines",           { .num = &config.menu_lines        }, NULL },
    { xrm_String, "font",            { .str = &config.menu_font         }, NULL },
    { xrm_String, "foreground",      { .str = &config.menu_fg           }, NULL },
    { xrm_String, "background",      { .str = &config.menu_bg           }, NULL },
    { xrm_String, "highlightfg",     { .str = &config.menu_hlfg         }, NULL },
    { xrm_String, "highlightbg",     { .str = &config.menu_hlbg         }, NULL },
    { xrm_String, "bordercolor",     { .str = &config.menu_bc           }, NULL },
    { xrm_Number, "padding",         { .num = &config.padding           }, NULL },
    { xrm_Number, "borderwidth",     { .num = &config.menu_bw           }, NULL },
    { xrm_String, "terminal",        { .str = &config.terminal_emulator }, NULL },
    { xrm_Number, "location",        { .num = &config.location          }, NULL },
    { xrm_Number, "yoffset",         { .num = &config.y_offset          }, NULL },
    { xrm_Number, "xoffset",         { .num = &config.x_offset          }, NULL },
    { xrm_Number, "fixed_num_lines", { .num = &config.fixed_num_lines   }, NULL },
    { xrm_Number, "columns",         { .num = &config.menu_columns      }, NULL },
    { xrm_Number, "hmode",           { .num = &config.hmode             }, NULL },
    /* Key bindings */
    { xrm_String, "key",             { .str = &config.window_key        }, NULL },
    { xrm_String, "rkey",            { .str = &config.run_key           }, NULL },
    { xrm_String, "skey",            { .str = &config.ssh_key           }, NULL },
};


void parse_xresource_options ( Display *display )
{
    char *xRMS;
    // Map Xresource entries to rofi config options.
    XrmInitialize ();
    xRMS = XResourceManagerString ( display );

    if ( xRMS == NULL )
    {
        return;
    }
    XrmDatabase xDB = XrmGetStringDatabase ( xRMS );

    char        * xrmType;
    XrmValue    xrmValue;
    const char  * namePrefix  = "rofi";
    const char  * classPrefix = "rofi";

    for ( unsigned int i = 0; i < sizeof ( xrmOptions ) / sizeof ( *xrmOptions ); ++i )
    {
        char *name, *class;
        if ( asprintf ( &name, "%s.%s", namePrefix, xrmOptions[i].name ) == -1 )
        {
            continue;
        }
        if ( asprintf ( &class, "%s.%s", classPrefix, xrmOptions[i].name ) > 0 )
        {
            if ( XrmGetResource ( xDB, name, class, &xrmType, &xrmValue ) )
            {
                if ( xrmOptions[i].type == xrm_String )
                {
                    if ( xrmOptions[i].mem != NULL )
                    {
                        free ( xrmOptions[i].mem );
                        xrmOptions[i].mem = NULL;
                    }
                    //TODO this leaks memory.
                    *xrmOptions[i].str = ( char * ) malloc ( xrmValue.size * sizeof ( char ) );
                    strncpy ( *xrmOptions[i].str, xrmValue.addr, xrmValue.size );

                    // Memory
                    xrmOptions[i].mem = ( *xrmOptions[i].str );
                }
                else if ( xrmOptions[i].type == xrm_Number )
                {
                    *xrmOptions[i].num = strtol ( xrmValue.addr, NULL, 10 );
                }
            }

            free ( class );
        }
        free ( name );
    }
    XrmDestroyDatabase ( xDB );
}

void parse_xresource_free ( void )
{
    for ( unsigned int i = 0; i < sizeof ( xrmOptions ) / sizeof ( *xrmOptions ); ++i )
    {
        if ( xrmOptions[i].mem != NULL )
        {
            free ( xrmOptions[i].mem );
            xrmOptions[i].mem = NULL;
        }
    }
}
