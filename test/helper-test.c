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
     * Collating.
     */
    char *res = token_collate_key ( "€ Sign", FALSE );
    TASSERT ( strcmp ( res, "€ sign" ) == 0 );
    g_free ( res );

    res = token_collate_key ( "éÉêèë Sign", FALSE );
    TASSERT ( strcmp ( res, "ééêèë sign" ) == 0 );
    g_free ( res );
    res = token_collate_key ( "éÉêèë³ Sign", TRUE );
    TASSERT ( strcmp ( res, "éÉêèë3 Sign" ) == 0 );
    g_free ( res );

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
    config.regex = FALSE;
    config.glob  = FALSE;
    char ** retv = tokenize ( "aAp nOoT MieS 12", FALSE );
    TASSERT ( retv[0] && strcmp ( retv[0], "aap" ) == 0 );
    TASSERT ( retv[1] && strcmp ( retv[1], "noot" ) == 0 );
    TASSERT ( retv[2] && strcmp ( retv[2], "mies" ) == 0 );
    TASSERT ( retv[3] && strcmp ( retv[3], "12" ) == 0 );
    tokenize_free ( retv );
    retv = tokenize ( "blub³ bOb bEp bEE", TRUE );
    TASSERT ( retv[0] && strcmp ( retv[0], "blub3" ) == 0 );
    TASSERT ( retv[1] && strcmp ( retv[1], "bOb" ) == 0 );
    TASSERT ( retv[2] && strcmp ( retv[2], "bEp" ) == 0 );
    TASSERT ( retv[3] && strcmp ( retv[3], "bEE" ) == 0 );
    tokenize_free ( retv );

    TASSERT ( levenshtein ( "aap", "aap" ) == 0 );
    TASSERT ( levenshtein ( "aap", "aap " ) == 1 );
    TASSERT ( levenshtein ( "aap ", "aap" ) == 1 );
    TASSERTE ( levenshtein ( "aap", "aap noot" ), 5 );
    TASSERTE ( levenshtein ( "aap", "noot aap" ), 5 );
    TASSERTE ( levenshtein ( "aap", "noot aap mies" ), 10 );
    TASSERTE ( levenshtein ( "noot aap mies", "aap" ), 10 );
    TASSERTE ( levenshtein ( "otp", "noot aap" ), 5 );
}
