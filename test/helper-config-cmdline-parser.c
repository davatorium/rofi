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

int main ( int argc, char ** argv )
{

    if ( setlocale ( LC_ALL, "" ) == NULL ) {
        fprintf ( stderr, "Failed to set locale.\n" );
        return EXIT_FAILURE;
    }
    char **list     = NULL;
    int  llength    = 0;
    char * test_str =
        "{host} {terminal} -e bash -c \"{ssh-client} {host}; echo '{terminal} {host}'\" -i -3 -u 4";
    helper_parse_setup ( test_str, &list, &llength, "{host}", "chuck",
                         "{terminal}", "x-terminal-emulator", NULL );

    TASSERT ( llength == 10);
    TASSERT ( strcmp ( list[0], "chuck" ) == 0 );
    TASSERT ( strcmp ( list[1], "x-terminal-emulator" ) == 0 );
    TASSERT ( strcmp ( list[2], "-e" ) == 0 );
    TASSERT ( strcmp ( list[3], "bash" ) == 0 );
    TASSERT ( strcmp ( list[4], "-c" ) == 0 );
    TASSERT ( strcmp ( list[5], "ssh chuck; echo 'x-terminal-emulator chuck'" ) == 0 );
    TASSERT ( strcmp ( list[6], "-i" ) == 0 );
    TASSERT ( strcmp ( list[7], "-3" ) == 0 );
    TASSERT ( strcmp ( list[8], "-u" ) == 0 );
    TASSERT ( strcmp ( list[9], "4" ) == 0 );

    cmd_set_arguments ( llength, list);
    TASSERT( find_arg ( "-e")  == 2 );
    TASSERT( find_arg ( "-x")  == -1 );
    char *str;
    TASSERT( find_arg_str ( "-e", &str)  == TRUE );
    TASSERT ( str == list[3] );
    TASSERT( find_arg_str ( "-x", &str)  == FALSE );
    // Should be unmodified.
    TASSERT ( str == list[3] );

    unsigned int u = 1234;
    unsigned int i = -1234;
    TASSERT ( find_arg_uint ( "-x", &u ) == FALSE );
    TASSERT ( u == 1234 );
    TASSERT ( find_arg_int ( "-x", &i ) == FALSE );
    TASSERT ( i == -1234 );
    TASSERT ( find_arg_uint ( "-u", &u ) == TRUE );
    TASSERT ( u == 4 );
    TASSERT ( find_arg_uint ( "-i", &u ) == TRUE );
    TASSERT ( u == 4294967293 );
    TASSERT ( find_arg_int ( "-i", &i ) == TRUE );
    TASSERT ( i == -3 );

    g_strfreev ( list );

}
