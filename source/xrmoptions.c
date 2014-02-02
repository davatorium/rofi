#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/X.h>
#include <X11/Xresource.h>
#include "simpleswitcher.h"
#include "xrmoptions.h"

// Big thanks to Sean Pringle for this code.
// This maps xresource options to config structure.
typedef enum {
    xrm_String = 0,
    xrm_Number = 1
} XrmOptionType;

typedef struct {
    int type;
    char * name ;
    union {
        unsigned int * num;
        char ** str;
    };
} XrmOption;
XrmOption xrmOptions[] = {
    { xrm_Number, "opacity",     { .num = &config.window_opacity } },
    { xrm_Number, "width",       { .num = &config.menu_width } },
    { xrm_Number, "lines",       { .num = &config.menu_lines } },
    { xrm_String, "font",        { .str = &config.menu_font } },
    { xrm_String, "foreground",  { .str = &config.menu_fg } },
    { xrm_String, "background",  { .str = &config.menu_bg } },
    { xrm_String, "highlightfg", { .str = &config.menu_hlfg } },
    { xrm_String, "highlightbg", { .str = &config.menu_hlbg } },
    { xrm_String, "bordercolor", { .str = &config.menu_bc } },
    { xrm_Number, "padding",     { .num = &config.padding } },
    { xrm_Number, "borderwidth", { .num = &config.menu_bw} },
    { xrm_String, "terminal",    { .str = &config.terminal_emulator } },
};


void parse_xresource_options( Display *display )
{
    // Map Xresource entries to simpleswitcher config options.
    XrmInitialize();
    char * xRMS = XResourceManagerString ( display );

    if ( xRMS != NULL ) {
        XrmDatabase xDB = XrmGetStringDatabase ( xRMS );

        char * xrmType;
        XrmValue xrmValue;
        // TODO: update when we have new name.
        const char * namePrefix = "simpleswitcher";
        const char * classPrefix = "Simpleswitcher";

        for ( unsigned int  i = 0; i < sizeof ( xrmOptions ) / sizeof ( *xrmOptions ); ++i ) {
            char *name = ( char* ) allocate( ( strlen ( namePrefix ) + 1 + strlen ( xrmOptions[i].name ) ) *
                                             sizeof ( char ) + 1 );
            char *class = ( char* ) allocate( ( strlen ( classPrefix ) + 1 + strlen ( xrmOptions[i].name ) ) *
                                                      sizeof ( char ) + 1 );
            sprintf ( name, "%s.%s", namePrefix, xrmOptions[i].name );
            sprintf ( class, "%s.%s", classPrefix, xrmOptions[i].name );

            if ( XrmGetResource ( xDB, name, class, &xrmType, &xrmValue ) ) {

                if ( xrmOptions[i].type == xrm_String ) {
                    *xrmOptions[i].str = ( char * ) allocate ( xrmValue.size * sizeof ( char ) );
                    strncpy ( *xrmOptions[i].str, xrmValue.addr, xrmValue.size );
                } else if ( xrmOptions[i].type == xrm_Number ) {
                    *xrmOptions[i].num = strtol ( xrmValue.addr, NULL, 10 );
                }
            }

            free ( name );
            free ( class );
        }

        XFree ( xRMS );
    }

}
