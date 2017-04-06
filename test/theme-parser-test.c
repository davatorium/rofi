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
#include "widgets/widget-internal.h"

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
    // C++ style comments with nesting.
    rofi_theme_parse_string ( "// Random comments // /*test */");
    TASSERT ( rofi_theme != NULL );
    rofi_theme_free ( rofi_theme );
    rofi_theme = NULL;
    rofi_theme_parse_string ( "/* test /* aap */ */");
    TASSERT ( rofi_theme != NULL );
    rofi_theme_free ( rofi_theme );
    rofi_theme = NULL;
    // C++ comments multiple lines.
    rofi_theme_parse_string ( "// Random comments\n// /*test */");
    TASSERT ( rofi_theme != NULL );
    rofi_theme_free ( rofi_theme );
    rofi_theme = NULL;
    rofi_theme_parse_string ( "/* test \n*\n*/* aap */ */");
    TASSERT ( rofi_theme != NULL );
    rofi_theme_free ( rofi_theme );
    rofi_theme = NULL;


    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    /** Boolean property */
    rofi_theme_parse_string ( "*{ test: true; test2:/* inline */false; }");
    TASSERT ( rofi_theme_get_boolean ( &wid, "test", FALSE) == TRUE );
    TASSERT ( rofi_theme_get_boolean ( &wid, "test2", TRUE) == FALSE );
    TASSERT ( rofi_theme != NULL );
    rofi_theme_free ( rofi_theme );
    rofi_theme = NULL;

    /** reference. */
    error = 0;
    rofi_theme_parse_string ( "* { test: true; test2:/* inline */false;} *{ a:@test; b:@test2;}");
    TASSERT ( error == 0 );
    TASSERT ( rofi_theme_get_boolean ( &wid, "test", FALSE) == TRUE );
    TASSERT ( rofi_theme_get_boolean ( &wid, "b", TRUE) == FALSE );
    TASSERT ( rofi_theme != NULL );
    rofi_theme_free ( rofi_theme );
    rofi_theme = NULL;
    {
        error = 0;
        rofi_theme_parse_string ( "* { test: 10em;}");
        TASSERT ( error == 0 );
        Distance d = (Distance){ 1, PW_PX, SOLID};
        Padding pi = (Padding){d,d,d,d};
        Padding p = rofi_theme_get_padding ( &wid, "test", pi);
        TASSERT (  p.left.distance == 10 );
        TASSERT (  p.left.type == PW_EM );
        TASSERT ( rofi_theme != NULL );
        rofi_theme_free ( rofi_theme );
        rofi_theme = NULL;
    }

    {
        error = 0;
        rofi_theme_parse_string ( "* { test: 10px;}");
        TASSERT ( error == 0 );
        Distance d = (Distance){ 1, PW_PX, SOLID};
        Padding pi = (Padding){d,d,d,d};
        Padding p = rofi_theme_get_padding ( &wid, "test", pi);
        TASSERT (  p.left.distance == 10 );
        TASSERT (  p.left.type == PW_PX );
        TASSERT ( rofi_theme != NULL );
        rofi_theme_free ( rofi_theme );
        rofi_theme = NULL;
    }
    {
        error = 0;
        rofi_theme_parse_string ( "* { test: 10%;}");
        TASSERT ( error == 0 );
        Distance d = (Distance){ 1, PW_PX, SOLID};
        Padding pi = (Padding){d,d,d,d};
        Padding p = rofi_theme_get_padding ( &wid, "test", pi);
        TASSERT (  p.left.distance == 10 );
        TASSERT (  p.left.type == PW_PERCENT );
        TASSERT ( rofi_theme != NULL );
        rofi_theme_free ( rofi_theme );
        rofi_theme = NULL;
    }
}
