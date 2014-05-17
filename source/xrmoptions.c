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
} XrmOption;
XrmOption xrmOptions[] = {
    { xrm_Number, "opacity",            { .num = &config.window_opacity    } },
    { xrm_Number, "width",              { .num = &config.menu_width        } },
    { xrm_Number, "lines",              { .num = &config.menu_lines        } },
    { xrm_String, "font",               { .str = &config.menu_font         } },
    { xrm_String, "foreground",         { .str = &config.menu_fg           } },
    { xrm_String, "background",         { .str = &config.menu_bg           } },
    { xrm_String, "highlightfg",        { .str = &config.menu_hlfg         } },
    { xrm_String, "highlightbg",        { .str = &config.menu_hlbg         } },
    { xrm_String, "bordercolor",        { .str = &config.menu_bc           } },
    { xrm_Number, "padding",            { .num = &config.padding           } },
    { xrm_Number, "borderwidth",        { .num = &config.menu_bw           } },
    { xrm_String, "terminal",           { .str = &config.terminal_emulator } },
    { xrm_Number, "location",           { .num = &config.location          } },
    { xrm_Number, "yoffset",            { .num = &config.y_offset          } },
    { xrm_Number, "xoffset",            { .num = &config.x_offset          } },
    { xrm_Number, "fixed_num_lines",    { .num = &config.fixed_num_lines } },
};


void parse_xresource_options ( Display *display )
{
    // Map Xresource entries to simpleswitcher config options.
    XrmInitialize ();
    char * xRMS = XResourceManagerString ( display );

    if ( xRMS != NULL )
    {
        XrmDatabase xDB = XrmGetStringDatabase ( xRMS );

        char        * xrmType;
        XrmValue    xrmValue;
        // TODO: update when we have new name.
        const char  * namePrefix  = "rofi";
        const char  * classPrefix = "Simpleswitcher";

        for ( unsigned int i = 0; i < sizeof ( xrmOptions ) / sizeof ( *xrmOptions ); ++i )
        {
            char *name = ( char * ) allocate ( ( strlen ( namePrefix ) + 1 + strlen ( xrmOptions[i].name ) ) *
                                               sizeof ( char ) + 1 );
            char *class = ( char * ) allocate ( ( strlen ( classPrefix ) + 1 + strlen ( xrmOptions[i].name ) ) *
                                                sizeof ( char ) + 1 );
            sprintf ( name, "%s.%s", namePrefix, xrmOptions[i].name );
            sprintf ( class, "%s.%s", classPrefix, xrmOptions[i].name );

            if ( XrmGetResource ( xDB, name, class, &xrmType, &xrmValue ) )
            {
                if ( xrmOptions[i].type == xrm_String )
                {
                    *xrmOptions[i].str = ( char * ) allocate ( xrmValue.size * sizeof ( char ) );
                    strncpy ( *xrmOptions[i].str, xrmValue.addr, xrmValue.size );
                }
                else if ( xrmOptions[i].type == xrm_Number )
                {
                    *xrmOptions[i].num = strtol ( xrmValue.addr, NULL, 10 );
                }
            }

            free ( name );
            free ( class );
        }
    }
}
