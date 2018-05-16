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
    // Pid test.
    // Tests basic functionality of writing it, locking, seeing if I can write same again
    // And close/reopen it again.
    {
        const char *tmpd = g_get_tmp_dir ();
        char *path = g_build_filename (tmpd, "rofi-pid.pid", NULL);
        TASSERT( create_pid_file ( NULL ) == -1 );
        int fd = create_pid_file ( path );
        TASSERT( fd >= 0 );
        int fd2 = create_pid_file ( path );
        TASSERT ( fd2 <  0 );

        remove_pid_file ( fd );
        fd = create_pid_file ( path );
        TASSERT( fd >= 0 );
        remove_pid_file ( fd );
        free ( path );
    }
}
