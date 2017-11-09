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
#include "display.h"
#include "xcb.h"
#include "xcb-internal.h"
#include "rofi.h"
#include "settings.h"

static int       test = 0;

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
void rofi_add_error_message ( G_GNUC_UNUSED GString *msg )
{
}
int rofi_view_error_dialog ( const char *msg, G_GNUC_UNUSED int markup )
{
    fputs ( msg, stderr );
    return TRUE;
}

int monitor_active ( G_GNUC_UNUSED workarea *mon )
{
    return 0;
}

void display_startup_notification ( G_GNUC_UNUSED RofiHelperExecuteContext *context, G_GNUC_UNUSED GSpawnChildSetupFunc *child_setup, G_GNUC_UNUSED gpointer *user_data )
{
}

int main ( G_GNUC_UNUSED int argc, G_GNUC_UNUSED char ** argv )
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
    int d = -1234;
    TASSERT ( find_arg_uint ( "-x", &u ) == FALSE );
    TASSERT ( u == 1234 );
    TASSERT ( find_arg_int ( "-x", &d ) == FALSE );
    TASSERT ( d == -1234 );
    TASSERT ( find_arg_uint ( "-u", &u ) == TRUE );
    TASSERT ( u == 4 );
    TASSERT ( find_arg_uint ( "-i", &u ) == TRUE );
    TASSERT ( u == 4294967293 );
    TASSERT ( find_arg_int ( "-i", &d ) == TRUE );
    TASSERT ( d == -3 );

    g_strfreev ( list );

}
