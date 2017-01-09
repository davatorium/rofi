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
void rofi_add_error_message ( GString *msg )
{

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

    /**
     * Char function
     */

    TASSERT ( helper_parse_char ( "\\n" ) == '\n' );
    TASSERT ( helper_parse_char ( "\\a" ) == '\a' );
    TASSERT ( helper_parse_char ( "\\b" ) == '\b' );
    TASSERT ( helper_parse_char ( "\\t" ) == '\t' );
    TASSERT ( helper_parse_char ( "\\v" ) == '\v' );
    TASSERT ( helper_parse_char ( "\\f" ) == '\f' );
    TASSERT ( helper_parse_char ( "\\r" ) == '\r' );
    TASSERT ( helper_parse_char ( "\\\\" ) == '\\' );
    TASSERT ( helper_parse_char ( "\\0" ) == 0 );
    TASSERT ( helper_parse_char ( "\\x77" ) == 'w' );
    TASSERT ( helper_parse_char ( "\\x0A" ) == '\n' );

    /**
     * tokenize
     */

    TASSERT ( levenshtein ( "aap", "aap" ) == 0 );
    TASSERT ( levenshtein ( "aap", "aap " ) == 1 );
    TASSERT ( levenshtein ( "aap ", "aap" ) == 1 );
    TASSERTE ( levenshtein ( "aap", "aap noot" ), 5 );
    TASSERTE ( levenshtein ( "aap", "noot aap" ), 5 );
    TASSERTE ( levenshtein ( "aap", "noot aap mies" ), 10 );
    TASSERTE ( levenshtein ( "noot aap mies", "aap" ), 10 );
    TASSERTE ( levenshtein ( "otp", "noot aap" ), 5 );
    /**
     * Quick converision check.
     */
    {
        char *str = rofi_latin_to_utf8_strdup ( "\xA1\xB5", 2 );
        TASSERT ( g_utf8_collate ( str, "¡µ" ) == 0 );
        g_free ( str );
    }

    {
        char *str = rofi_force_utf8 ( "Valid utf8", 10 );
        TASSERT ( g_utf8_collate ( str, "Valid utf8" ) == 0 );
        g_free ( str );
        char in[] = "Valid utf8 until \xc3\x28 we continue here";
        TASSERT ( g_utf8_validate ( in, -1, NULL ) == FALSE );
        str = rofi_force_utf8 ( in, strlen ( in ) );
        TASSERT ( g_utf8_validate ( str, -1, NULL ) == TRUE );
        TASSERT ( g_utf8_collate ( str, "Valid utf8 until �( we continue here" ) == 0 );
        g_free ( str );
    }
    // Pid test.
    // Tests basic functionality of writing it, locking, seeing if I can write same again
    // And close/reopen it again.
    {
        const char *path = "/tmp/rofi-test.pid";
        TASSERT ( create_pid_file ( NULL ) == -1 );
        int        fd = create_pid_file ( path );
        TASSERT ( fd >= 0 );
        int        fd2 = create_pid_file ( path );
        TASSERT ( fd2 < 0 );

        remove_pid_file ( fd );
        fd = create_pid_file ( path );
        TASSERT ( fd >= 0 );
        remove_pid_file ( fd );
    }
}
