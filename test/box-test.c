#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <glib.h>
#include <string.h>
#include <widgets/box.h>
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

static gboolean test_widget_clicked ( G_GNUC_UNUSED widget *wid, G_GNUC_UNUSED xcb_button_press_event_t* xce, G_GNUC_UNUSED void *data )
{
    return TRUE;
}

int main ( G_GNUC_UNUSED int argc, G_GNUC_UNUSED char **argv )
{
    {
        box *b = box_create ( BOX_HORIZONTAL, 0, 0, 100, 20 );
        box_set_padding ( b, 5 );

        widget *wid1 = g_malloc0(sizeof(widget));
        box_add ( b , WIDGET( wid1 ), TRUE, FALSE );
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
        box_add ( b , WIDGET( wid2 ), TRUE, FALSE );
        TASSERTE ( wid1->h, 20);
        TASSERTE ( wid1->w, 47);
        TASSERTE ( wid2->h, 20);
        TASSERTE ( wid2->w, 48);

        widget *wid3 = g_malloc0(sizeof(widget));
        widget_enable ( WIDGET ( wid3 ) );
        box_add ( b , WIDGET( wid3 ), FALSE, FALSE );
        TASSERTE ( wid1->h, 20);
        TASSERTE ( wid1->w, 45);
        TASSERTE ( wid2->h, 20);
        TASSERTE ( wid2->w, 45);

        widget_resize ( WIDGET (wid3) , 20, 10 );
        // TODO should this happen automagically?
        widget_update ( WIDGET ( b ) ) ;
        TASSERTE ( wid1->h, 20);
        TASSERTE ( wid1->w, 35);
        TASSERTE ( wid2->h, 20);
        TASSERTE ( wid2->w, 35);
        TASSERTE ( wid3->h, 20);
        TASSERTE ( wid3->w, 20);

        widget_resize ( WIDGET (b ), 200, 20 );
        TASSERTE ( wid1->h, 20);
        TASSERTE ( wid1->w, 85);
        TASSERTE ( wid2->h, 20);
        TASSERTE ( wid2->w, 85);
        TASSERTE ( wid3->h, 20);
        TASSERTE ( wid3->w, 20);
        TASSERTE ( box_get_fixed_pixels ( b ) , 30 );

        widget *wid4 = g_malloc0(sizeof(widget));
        widget_enable ( WIDGET ( wid4 ) );
        widget_resize ( WIDGET ( wid4 ), 20, 20 );
        box_add ( b , WIDGET( wid4 ), FALSE, TRUE );
        TASSERTE ( wid4->x, 200-20);
        widget *wid5 = g_malloc0(sizeof(widget));
        widget_enable ( WIDGET ( wid5 ) );
        widget_resize ( WIDGET ( wid5 ), 20, 20 );
        box_add ( b , WIDGET( wid5 ), TRUE, TRUE );
        TASSERTE ( wid5->x, 128);
        widget_free ( WIDGET ( b ) );
    }
    {
        box *b = box_create ( BOX_VERTICAL, 0, 0, 20, 100 );
        box_set_padding ( b, 5 );

        widget *wid1 = g_malloc0(sizeof(widget));
        box_add ( b , WIDGET( wid1 ), TRUE, FALSE );
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
        box_add ( b , WIDGET( wid2 ), TRUE, FALSE );
        TASSERTE ( wid1->w, 20);
        TASSERTE ( wid1->h, 47);
        TASSERTE ( wid2->w, 20);
        TASSERTE ( wid2->h, 48);

        widget *wid3 = g_malloc0(sizeof(widget));
        widget_enable ( WIDGET ( wid3 ) );
        box_add ( b , WIDGET( wid3 ), FALSE, FALSE );
        TASSERTE ( wid1->w, 20);
        TASSERTE ( wid1->h, 45);
        TASSERTE ( wid2->w, 20);
        TASSERTE ( wid2->h, 45);

        widget_resize ( WIDGET (wid3) , 10, 20 );
        // TODO should this happen automagically?
        widget_update ( WIDGET ( b ) ) ;
        TASSERTE ( wid1->w, 20);
        TASSERTE ( wid1->h, 35);
        TASSERTE ( wid2->w, 20);
        TASSERTE ( wid2->h, 35);
        TASSERTE ( wid3->w, 20);
        TASSERTE ( wid3->h, 20);

        widget_resize ( WIDGET (b ), 20, 200 );
        TASSERTE ( wid1->w, 20);
        TASSERTE ( wid1->h, 85);
        TASSERTE ( wid2->w, 20);
        TASSERTE ( wid2->h, 85);
        TASSERTE ( wid3->w, 20);
        TASSERTE ( wid3->h, 20);
        TASSERTE ( box_get_fixed_pixels ( b ) , 30 );
        widget *wid4 = g_malloc0(sizeof(widget));
        widget_enable ( WIDGET ( wid4 ) );
        widget_resize ( WIDGET ( wid4 ), 20, 20 );
        box_add ( b , WIDGET( wid4 ), FALSE, TRUE );
        TASSERTE ( wid4->y, 200-20);
        widget *wid5 = g_malloc0(sizeof(widget));
        widget_enable ( WIDGET ( wid5 ) );
        widget_resize ( WIDGET ( wid5 ), 20, 20 );
        box_add ( b , WIDGET( wid5 ), TRUE, TRUE );
        TASSERTE ( wid5->y, 128);
        widget_free ( WIDGET ( b ) );
    }
    {
        box *b = box_create ( BOX_VERTICAL, 0, 0, 20, 100 );
        box_set_padding ( b, 5 );
        widget *wid1 = g_malloc0(sizeof(widget));
        widget_enable(wid1);
        wid1->clicked = test_widget_clicked;
        box_add ( b , WIDGET( wid1 ), TRUE, FALSE );
        widget *wid2 = g_malloc0(sizeof(widget));
        widget_enable(wid2);
        box_add ( b , WIDGET( wid2 ), TRUE, FALSE );

        xcb_button_press_event_t xce;
        xce.event_x = 10;
        xce.event_y = 60;
        TASSERTE ( widget_clicked ( WIDGET(b), &xce ), 0);

        xce.event_y = 50;
        TASSERTE ( widget_clicked ( WIDGET(b), &xce ), 0);
        xce.event_y = 45;
        TASSERTE ( widget_clicked ( WIDGET(b), &xce ), 1);
        widget_disable ( wid2 );
        xce.event_y = 60;
        TASSERTE ( widget_clicked ( WIDGET(b), &xce ), 1);
        widget_disable ( wid1 );
        widget_enable ( wid2 );
        TASSERTE ( widget_clicked ( WIDGET(b), &xce ), 0);
        widget_free ( WIDGET ( b ) );
    }
}
