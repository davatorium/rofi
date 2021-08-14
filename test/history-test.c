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

#include <unistd.h>

#include <stdio.h>
#include <assert.h>
#include <glib.h>
#include <history.h>
#include <string.h>

static unsigned int test = 0;

#define TASSERT( a )    {                                \
        assert ( a );                                    \
        printf ( "Test %u passed (%s)\n", ++test, # a ); \
}

const char *file = "text";

static void history_test ( void )
{
    unlink ( file );

    // Empty list.
    unsigned int length = 0;
    char         **retv = history_get_list ( file, &length );

    TASSERT ( retv == NULL );
    TASSERT ( length == 0 );

    // 1 item
    history_set ( file, "aap" );

    retv = history_get_list ( file, &length );

    TASSERT ( retv != NULL );
    TASSERT ( length == 1 );
    TASSERT ( strcmp ( retv[0], "aap" ) == 0 );

    g_strfreev ( retv );

    // Remove entry
    history_remove ( file, "aap" );

    length = 0;
    retv   = history_get_list ( file, &length );

    TASSERT ( retv == NULL );
    TASSERT ( length == 0 );

    // 2 items
    history_set ( file, "aap" );
    history_set ( file, "aap" );

    retv = history_get_list ( file, &length );

    TASSERT ( retv != NULL );
    TASSERT ( length == 1 );

    TASSERT ( strcmp ( retv[0], "aap" ) == 0 );

    g_strfreev ( retv );

    for ( unsigned int in = length + 1; in < 26; in++ ) {
        char *p = g_strdup_printf ( "aap%u", in );
        printf ( "%s- %u\n", p, in );
        history_set ( file, p );
        retv = history_get_list ( file, &length );

        TASSERT ( retv != NULL );
        TASSERT ( length == ( in ) );

        g_strfreev ( retv );

        g_free ( p );
    }
    // Max 25 entries.
    history_set ( file, "blaat" );
    retv = history_get_list ( file, &length );

    TASSERT ( retv != NULL );
    TASSERT ( length == 25 );
    for ( unsigned int in = 0; in < 24; in++ ) {
        char *p = g_strdup_printf ( "aap%i", in + 2 );
        TASSERT ( g_strcmp0 ( retv[in], p ) == 0 );

        g_free ( p );
    }

    g_strfreev ( retv );

    unlink ( file );
}

int main ( G_GNUC_UNUSED int argc, G_GNUC_UNUSED char **argv )
{
    history_test ();

    return 0;
}
