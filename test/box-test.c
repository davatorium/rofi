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
#include <theme.h>
#include <widgets/box.h>
#include <widgets/widget.h>
#include <widgets/widget-internal.h>
#include "rofi.h"
#include "xrmoptions.h"
#include "helper.h"
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

#define TASSERTW( a, b )    {                                                                            \
        if ( ( a ) == ( b ) ) {                                                                          \
            printf ( "Test %i passed (%s == %s) (%p == %p)\n", ++test, # a, # b, (void *)a, (void *)b ); \
        }else {                                                                                          \
            printf ( "Test %i failed (%s == %s) (%p != %p)\n", ++test, # a, # b, (void *)a, (void *)b ); \
            abort ( );                                                                                   \
        }                                                                                                \
}

void config_parse_set_property ( G_GNUC_UNUSED const Property *p )
{
}
char * rofi_expand_path ( G_GNUC_UNUSED const char *path )
{
    return NULL;
}

char * helper_get_theme_path ( const char *file )
{
    return g_strdup ( file );
}
void rofi_add_error_message ( G_GNUC_UNUSED GString *msg )
{
}
int textbox_get_estimated_char_height ( void );
int textbox_get_estimated_char_height ( void )
{
    return 16;
}
void rofi_view_get_current_monitor ( G_GNUC_UNUSED int *width, G_GNUC_UNUSED int *height )
{

}


int main ( G_GNUC_UNUSED int argc, G_GNUC_UNUSED char **argv )
{
    {
        box *b = box_create ( "box", ROFI_ORIENTATION_HORIZONTAL );
        //box_set_padding ( b, 5 );
        widget_resize ( WIDGET (b), 100, 20);

        widget *wid1 = g_malloc0(sizeof(widget));
        box_add ( b , WIDGET( wid1 ), TRUE, 0 );
        // Widget not enabled.  no width allocated.
        TASSERTE ( wid1->h, 0);
        TASSERTE ( wid1->w, 0 );
        widget_enable ( WIDGET ( wid1 ) );
        widget_update ( WIDGET ( b ) ) ;
        // Widget enabled.  so width allocated.
        TASSERTE ( wid1->h, 20);
        TASSERTE ( wid1->w, 100 );
        widget *wid2 = g_malloc0(sizeof(widget));
        widget_enable ( WIDGET ( wid2 ) );
        box_add ( b , WIDGET( wid2 ), TRUE, 1 );
        TASSERTE ( wid1->h, 20);
        TASSERTE ( wid1->w, 49);
        TASSERTE ( wid2->h, 20);
        TASSERTE ( wid2->w, 49);

        widget *wid3 = g_malloc0(sizeof(widget));
        widget_enable ( WIDGET ( wid3 ) );
        box_add ( b , WIDGET( wid3 ), FALSE, 2 );
        TASSERTE ( wid1->h, 20);
        TASSERTE ( wid1->w, 48);
        TASSERTE ( wid2->h, 20);
        TASSERTE ( wid2->w, 48);

        widget_resize ( WIDGET (wid3) , 20, 10 );
        // TODO should this happen automagically?
        widget_update ( WIDGET ( b ) ) ;
        TASSERTE ( wid1->h, 20);
        TASSERTE ( wid1->w, 38);
        TASSERTE ( wid2->h, 20);
        TASSERTE ( wid2->w, 38);
        TASSERTE ( wid3->h, 20);
        TASSERTE ( wid3->w, 20);

        widget_resize ( WIDGET (b ), 200, 20 );
        TASSERTE ( wid1->h, 20);
        TASSERTE ( wid1->w, 88);
        TASSERTE ( wid2->h, 20);
        TASSERTE ( wid2->w, 88);
        TASSERTE ( wid3->h, 20);
        TASSERTE ( wid3->w, 20);
//        TASSERTE ( box_get_fixed_pixels ( b ) , 24 );

        widget *wid4 = g_malloc0(sizeof(widget));
        widget_enable ( WIDGET ( wid4 ) );
        widget_resize ( WIDGET ( wid4 ), 20, 20 );
        box_add ( b , WIDGET( wid4 ), FALSE, 5 );
        TASSERTE ( wid4->x, 200-20);
        widget *wid5 = g_malloc0(sizeof(widget));
        widget_enable ( WIDGET ( wid5 ) );
        widget_resize ( WIDGET ( wid5 ), 20, 20 );
        box_add ( b , WIDGET( wid5 ), TRUE, 6 );
        TASSERTE ( wid5->x, 149);
        widget_free ( WIDGET ( b ) );
    }
    {
        box *b = box_create ( "box", ROFI_ORIENTATION_VERTICAL );
        widget_resize ( WIDGET (b), 20, 100);
        //box_set_padding ( b, 5 );

        widget *wid1 = g_malloc0(sizeof(widget));
        box_add ( b , WIDGET( wid1 ), TRUE, 0 );
        // Widget not enabled.  no width allocated.
        TASSERTE ( wid1->h, 0);
        TASSERTE ( wid1->w, 0 );
        widget_enable ( WIDGET ( wid1 ) );
        widget_update ( WIDGET ( b ) ) ;
        // Widget enabled.  so width allocated.
        TASSERTE ( wid1->h, 100);
        TASSERTE ( wid1->w, 20 );
        widget *wid2 = g_malloc0(sizeof(widget));
        widget_enable ( WIDGET ( wid2 ) );
        box_add ( b , WIDGET( wid2 ), TRUE, 1 );
        TASSERTE ( wid1->w, 20);
        TASSERTE ( wid1->h, 49);
        TASSERTE ( wid2->w, 20);
        TASSERTE ( wid2->h, 49);

        widget *wid3 = g_malloc0(sizeof(widget));
        widget_enable ( WIDGET ( wid3 ) );
        box_add ( b , WIDGET( wid3 ), FALSE, 2 );
        TASSERTE ( wid1->w, 20);
        TASSERTE ( wid1->h, 48);
        TASSERTE ( wid2->w, 20);
        TASSERTE ( wid2->h, 48);

        widget_resize ( WIDGET (wid3) , 10, 20 );
        // TODO should this happen automagically?
        widget_update ( WIDGET ( b ) ) ;
        TASSERTE ( wid1->w, 20);
        TASSERTE ( wid1->h, 38);
        TASSERTE ( wid2->w, 20);
        TASSERTE ( wid2->h, 38);
        TASSERTE ( wid3->w, 20);
        TASSERTE ( wid3->h, 20);

        widget_resize ( WIDGET (b ), 20, 200 );
        TASSERTE ( wid1->w, 20);
        TASSERTE ( wid1->h, 88);
        TASSERTE ( wid2->w, 20);
        TASSERTE ( wid2->h, 88);
        TASSERTE ( wid3->w, 20);
        TASSERTE ( wid3->h, 20);
//        TASSERTE ( box_get_fixed_pixels ( b ) , 4 );
        widget *wid4 = g_malloc0(sizeof(widget));
        widget_enable ( WIDGET ( wid4 ) );
        widget_resize ( WIDGET ( wid4 ), 20, 20 );
        box_add ( b , WIDGET( wid4 ), FALSE, 5 );
        TASSERTE ( wid4->y, 180);
        widget *wid5 = g_malloc0(sizeof(widget));
        widget_enable ( WIDGET ( wid5 ) );
        widget_resize ( WIDGET ( wid5 ), 20, 20 );
        box_add ( b , WIDGET( wid5 ), TRUE, 6 );
        TASSERTE ( wid5->y, 149);
        widget_free ( WIDGET ( b ) );
    }
    {
        box *b = box_create ( "box", ROFI_ORIENTATION_VERTICAL );
        widget_resize ( WIDGET (b), 20, 90);
        //box_set_padding ( b, 5 );
        widget *wid1 = g_malloc0(sizeof(widget));
        wid1->type = 1;
        widget_enable(wid1);
        box_add ( b , WIDGET( wid1 ), TRUE, 0 );
        widget *wid2 = g_malloc0(sizeof(widget));
        wid2->type = 1;
        widget_enable(wid2);
        box_add ( b , WIDGET( wid2 ), TRUE, 1 );
        widget *wid3 = g_malloc0(sizeof(widget));
        wid3->type = 2;
        widget_enable(wid3);
        box_add ( b , WIDGET( wid3 ), TRUE, 2 );

        gint x = 10;
        gint y = 50;
        TASSERTW ( widget_find_mouse_target ( WIDGET(b), 1, x, y ), WIDGET(wid2) );

        y = 30;
        TASSERTW ( widget_find_mouse_target ( WIDGET(b), 1, x, y ), WIDGET(wid2) );
        y = 27;
        TASSERTW ( widget_find_mouse_target ( WIDGET(b), 1, x, y ), WIDGET(wid1) );
        widget_disable ( wid2 );
        y = 40;
        TASSERTW ( widget_find_mouse_target ( WIDGET(b), 1, x, y ), WIDGET(wid1) );
        widget_disable ( wid1 );
        widget_enable ( wid2 );
        TASSERTW ( widget_find_mouse_target ( WIDGET(b), 1, x, y ), WIDGET(wid2) );
        TASSERTW ( widget_find_mouse_target ( WIDGET(b), 2, x, y ), NULL );
        y = 55;
        TASSERTW ( widget_find_mouse_target ( WIDGET(b), 2, x, y ), WIDGET(wid3) );
        widget_free ( WIDGET ( b ) );
    }
}
