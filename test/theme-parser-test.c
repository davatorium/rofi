#include <assert.h>
#include <locale.h>
#include <glib.h>
#include <stdio.h>
#include <helper.h>
#include <string.h>
#include <xcb/xcb_ewmh.h>
#include "xcb-internal.h"
#include "rofi.h"
#include "settings.h"
#include "theme.h"

static int       test = 0;
struct xcb_stuff *xcb;

#define TASSERT( a )        {                            \
        assert ( a );                                    \
        printf ( "Test %i passed (%s)\n", ++test, # a ); \
}
#define TASSERTE( a, b )    {                                                            \
        if ( ( a ) == ( b ) ) {                                                          \
            printf ( "Test %i passed (%s == %s) (%u == %u)\n", ++test, # a, # b, a, b ); \
        }else {                                                                          \
            printf ( "Test %i failed (%s == %s) (%u != %u)\n", ++test, # a, # b, a, b ); \
            abort ( );                                                                   \
        }                                                                                \
}
gboolean error = FALSE;
void rofi_add_error_message ( GString *msg )
{

    error = TRUE;
    g_string_free ( msg, TRUE );
}

int rofi_view_error_dialog ( const char *msg, G_GNUC_UNUSED int markup )
{
    fputs ( msg, stderr );
    return TRUE;
}

int show_error_message ( const char *msg, int markup )
{
    rofi_view_error_dialog ( msg, markup );
    return 0;
}
void rofi_view_get_current_monitor ( int *width, int *height )
{
    if ( width ) {
        *width = 1920; 
    }
    if ( height ) {
        *height = 1080; 
    }
}
double textbox_get_estimated_char_height ( void )
{
    return 16.0;
}
xcb_screen_t          *xcb_screen;
xcb_ewmh_connection_t xcb_ewmh;
int                   xcb_screen_nbr;
#include <x11-helper.h>

int main ( int argc, char ** argv )
{
    cmd_set_arguments ( argc, argv );

    if ( setlocale ( LC_ALL, "" ) == NULL ) {
        fprintf ( stderr, "Failed to set locale.\n" );
        return EXIT_FAILURE;
    }
    // EMPTY string.
    rofi_theme_parse_string ( "");
    TASSERT ( rofi_theme != NULL );
    rofi_theme_free ( rofi_theme );
    rofi_theme = NULL;
    // EMPTY global.
    rofi_theme_parse_string ( " *{} ");
    TASSERT ( rofi_theme != NULL );
    rofi_theme_free ( rofi_theme );
    rofi_theme = NULL;
    // EMPTY section.
    rofi_theme_parse_string ( " #{} ");
    TASSERT ( rofi_theme != NULL );
    rofi_theme_free ( rofi_theme );
    rofi_theme = NULL;
    rofi_theme_parse_string ( " Blaat ");
    TASSERT ( rofi_theme != NULL );
    TASSERT ( error == TRUE );
    rofi_theme_free ( rofi_theme );
    rofi_theme = NULL;
    error = FALSE;
}
