/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2017 Qball Cow <qball@gmpclient.org>
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
#define TASSERTL( a, b )    {                                                            \
        if ( ( a ) == ( b ) ) {                                                          \
            printf ( "Test %i passed (%s == %s) (%d == %d)\n", ++test, # a, # b, a, b ); \
        }else {                                                                          \
            printf ( "Test %i failed (%s == %s) (%d != %d)\n", ++test, # a, # b, a, b ); \
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

    TASSERT ( levenshtein ( "aap", g_utf8_strlen ( "aap", -1), "aap", g_utf8_strlen ( "aap", -1) ) == 0 );
    TASSERT ( levenshtein ( "aap", g_utf8_strlen ( "aap", -1), "aap ", g_utf8_strlen ( "aap ", -1) ) == 1 );
    TASSERT ( levenshtein ( "aap ", g_utf8_strlen ( "aap ", -1), "aap", g_utf8_strlen ( "aap", -1) ) == 1 );
    TASSERTE ( levenshtein ( "aap", g_utf8_strlen ( "aap", -1), "aap noot", g_utf8_strlen ( "aap noot", -1) ), 5 );
    TASSERTE ( levenshtein ( "aap", g_utf8_strlen ( "aap", -1), "noot aap", g_utf8_strlen ( "noot aap", -1) ), 5 );
    TASSERTE ( levenshtein ( "aap", g_utf8_strlen ( "aap", -1), "noot aap mies", g_utf8_strlen ( "noot aap mies", -1) ), 10 );
    TASSERTE ( levenshtein ( "noot aap mies", g_utf8_strlen ( "noot aap mies", -1), "aap", g_utf8_strlen ( "aap", -1) ), 10 );
    TASSERTE ( levenshtein ( "otp", g_utf8_strlen ( "otp", -1), "noot aap", g_utf8_strlen ( "noot aap", -1) ), 5 );
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
    {
        TASSERT ( utf8_strncmp ( "aapno", "aap€",3) == 0 );
        TASSERT ( utf8_strncmp ( "aapno", "aap€",4) != 0 );
        TASSERT ( utf8_strncmp ( "aapno", "a",4) != 0 );
        TASSERT ( utf8_strncmp ( "a", "aap€",4) != 0 );
//        char in[] = "Valid utf8 until \xc3\x28 we continue here";
//        TASSERT ( utf8_strncmp ( in, "Valid", 3 ) == 0);
    }
    {
        TASSERTL ( rofi_scorer_fuzzy_evaluate ("aap noot mies", 12 , "aap noot mies", 12), -605);
        TASSERTL ( rofi_scorer_fuzzy_evaluate ("anm", 3, "aap noot mies", 12), -155);
        TASSERTL ( rofi_scorer_fuzzy_evaluate ("blu", 3, "aap noot mies", 12), 1073741824);
        config.case_sensitive = TRUE;
        TASSERTL ( rofi_scorer_fuzzy_evaluate ("Anm", 3, "aap noot mies", 12), 1073741754);
        config.case_sensitive = FALSE;
        TASSERTL ( rofi_scorer_fuzzy_evaluate ("Anm", 3, "aap noot mies", 12), -155);
        TASSERTL ( rofi_scorer_fuzzy_evaluate ("aap noot mies", 12,"Anm", 3 ), 1073741824);

    }
}
