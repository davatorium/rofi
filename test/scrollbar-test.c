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
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <glib.h>
#include <string.h>
#include <widgets/scrollbar.h>
#include <widgets/widget.h>
#include <widgets/widget-internal.h>
unsigned int test =0;
#define TASSERT( a )    {                                 \
        assert ( a );                                     \
        printf ( "Test %3i passed (%s)\n", ++test, # a ); \
}

#define TASSERTE( a, b )    {                                                            \
        if ( ( a ) == ( b ) ) {                                                          \
            printf ( "Test %i passed (%s == %s) (%u == %u)\n", ++test, # a, # b, a, b ); \
        }else {                                                                          \
            printf ( "Test %i failed (%s == %s) (%u != %u)\n", ++test, # a, # b, a, b ); \
            abort ( );                                                                   \
        }                                                                                \
}
void config_parse_set_property ( const void *p )
{
}
void rofi_add_error_message ( GString *msg )
{}

char * rofi_expand_path ( const char *path )
{

}
int textbox_get_estimated_char_height ( void );
int textbox_get_estimated_char_height ( void )
{
    return 16;
}
void color_separator ( G_GNUC_UNUSED void *d )
{

}

void rofi_view_get_current_monitor ( int *width, int *height )
{

}

int main ( G_GNUC_UNUSED int argc, G_GNUC_UNUSED char **argv )
{
    scrollbar * sb = scrollbar_create ( "scrollbar" );
    widget_resize ( WIDGET (sb), 10, 100);

    scrollbar_set_handle ( NULL, 10213);
    scrollbar_set_max_value ( NULL, 10 );
    scrollbar_set_handle_length ( NULL , 1000);

    scrollbar_set_max_value ( sb, 10000);
    TASSERTE ( sb->length, 10000 );
    scrollbar_set_handle_length ( sb, 10);
    TASSERTE ( sb->pos_length, 10 );
    scrollbar_set_handle ( sb , 5000 );
    TASSERTE ( sb->pos, 5000 );
    scrollbar_set_handle ( sb , 15000 );
    TASSERTE ( sb->pos, 10000 );
    scrollbar_set_handle ( sb , UINT32_MAX );
    TASSERTE ( sb->pos, 10000 );
    scrollbar_set_handle_length ( sb, 15000);
    TASSERTE ( sb->pos_length, 10000 );
    scrollbar_set_handle_length ( sb, 0);
    TASSERTE ( sb->pos_length, 1 );


    unsigned int cl = scrollbar_clicked ( sb, 10 );
    TASSERTE ( cl, 1010);
    cl = scrollbar_clicked ( sb, 20 );
    TASSERTE ( cl, 2020);
    cl = scrollbar_clicked ( sb, 0 );
    TASSERTE ( cl, 0);
    cl = scrollbar_clicked ( sb, 99 );
    TASSERTE ( cl, 9999);
    scrollbar_set_handle_length ( sb, 1000);
    cl = scrollbar_clicked ( sb, 10 );
    TASSERTE ( cl, 555);
    cl = scrollbar_clicked ( sb, 20 );
    TASSERTE ( cl, 1666);
    cl = scrollbar_clicked ( sb, 0 );
    TASSERTE ( cl, 0);
    cl = scrollbar_clicked ( sb, 99 );
    TASSERTE ( cl, 9999);

    widget_free( WIDGET (sb ) );
}
