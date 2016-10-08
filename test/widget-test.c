#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <glib.h>
#include <string.h>
#include <widgets/widget.h>
unsigned int test =0;
#define TASSERT( a )    {                                 \
        assert ( a );                                     \
        printf ( "Test %3i passed (%s)\n", ++test, # a ); \
}


int main ( G_GNUC_UNUSED int argc, G_GNUC_UNUSED char **argv )
{
//    box 20 by 40
    widget widget  = { 10,10,20,40 };

    // Left of box
    TASSERT ( widget_intersect ( &widget, 0, 0) == 0 );
    TASSERT ( widget_intersect ( &widget, 0, 10) == 0 );
    TASSERT ( widget_intersect ( &widget, 0, 25) == 0 );
    TASSERT ( widget_intersect ( &widget, 0, 40) == 0 );
    TASSERT ( widget_intersect ( &widget, 0, 50) == 0 );
    TASSERT ( widget_intersect ( &widget, 9, 0) == 0 );
    TASSERT ( widget_intersect ( &widget, 9, 10) == 0 );
    TASSERT ( widget_intersect ( &widget, 9, 25) == 0 );
    TASSERT ( widget_intersect ( &widget, 9, 40) == 0 );
    TASSERT ( widget_intersect ( &widget, 9, 50) == 0 );
    TASSERT ( widget_intersect ( &widget, 10, 0) == 0 );
    TASSERT ( widget_intersect ( &widget, 10, 10) == 1 );
    TASSERT ( widget_intersect ( &widget, 10, 25) == 1 );
    TASSERT ( widget_intersect ( &widget, 10, 40) == 1 );
    TASSERT ( widget_intersect ( &widget, 10, 50) == 0 );

    // Middle

    TASSERT ( widget_intersect ( &widget, 25, 0) == 0 );
    TASSERT ( widget_intersect ( &widget, 25, 10) == 1 );
    TASSERT ( widget_intersect ( &widget, 25, 25) == 1 );
    TASSERT ( widget_intersect ( &widget, 25, 40) == 1 );
    TASSERT ( widget_intersect ( &widget, 25, 50) == 0 );

    // Right
    TASSERT ( widget_intersect ( &widget, 29, 0) == 0 );
    TASSERT ( widget_intersect ( &widget, 29, 10) == 1 );
    TASSERT ( widget_intersect ( &widget, 29, 25) == 1 );
    TASSERT ( widget_intersect ( &widget, 29, 40) == 1 );
    TASSERT ( widget_intersect ( &widget, 29, 50) == 0 );

    TASSERT ( widget_intersect ( &widget, 30, 0) == 0 );
    TASSERT ( widget_intersect ( &widget, 30, 10) == 0 );
    TASSERT ( widget_intersect ( &widget, 30, 25) == 0 );
    TASSERT ( widget_intersect ( &widget, 30, 40) == 0 );
    TASSERT ( widget_intersect ( &widget, 30, 50) == 0 );

    widget_move ( &widget, 30, 30);
    // Left of box
    TASSERT ( widget_intersect ( &widget, 10, 20) == 0 );
    TASSERT ( widget_intersect ( &widget, 10, 30) == 0 );
    TASSERT ( widget_intersect ( &widget, 10, 45) == 0 );
    TASSERT ( widget_intersect ( &widget, 10, 60) == 0 );
    TASSERT ( widget_intersect ( &widget, 10, 70) == 0 );
    TASSERT ( widget_intersect ( &widget, 19, 20) == 0 );
    TASSERT ( widget_intersect ( &widget, 19, 30) == 0 );
    TASSERT ( widget_intersect ( &widget, 19, 45) == 0 );
    TASSERT ( widget_intersect ( &widget, 19, 60) == 0 );
    TASSERT ( widget_intersect ( &widget, 19, 70) == 0 );
    TASSERT ( widget_intersect ( &widget, 30, 20) == 0 );
    TASSERT ( widget_intersect ( &widget, 30, 30) == 1 );
    TASSERT ( widget_intersect ( &widget, 30, 45) == 1 );
    TASSERT ( widget_intersect ( &widget, 30, 60) == 1 );
    TASSERT ( widget_intersect ( &widget, 30, 70) == 0 );

    // Middle

    TASSERT ( widget_intersect ( &widget, 20+25,20+ 0) == 0 );
    TASSERT ( widget_intersect ( &widget, 20+25,20+ 10) == 1 );
    TASSERT ( widget_intersect ( &widget, 20+25,20+ 25) == 1 );
    TASSERT ( widget_intersect ( &widget, 20+25,20+ 40) == 1 );
    TASSERT ( widget_intersect ( &widget, 20+25,20+ 50) == 0 );

    TASSERT ( widget_intersect ( &widget, 20+29, 20+0) == 0 );
    TASSERT ( widget_intersect ( &widget, 20+29, 20+10) == 1 );
    TASSERT ( widget_intersect ( &widget, 20+29, 20+25) == 1 );
    TASSERT ( widget_intersect ( &widget, 20+29, 20+40) == 1 );
    TASSERT ( widget_intersect ( &widget, 20+29, 20+50) == 0 );

    TASSERT ( widget_intersect ( &widget, 20+30, 20+0) == 0 );
    TASSERT ( widget_intersect ( &widget, 20+30, 20+10) == 0 );
    TASSERT ( widget_intersect ( &widget, 20+30, 20+25) == 0 );
    TASSERT ( widget_intersect ( &widget, 20+30, 20+40) == 0 );
    TASSERT ( widget_intersect ( &widget, 20+30, 20+50) == 0 );

    // Right
    TASSERT ( widget_intersect ( &widget, 20+29, 20+0) == 0 );
    TASSERT ( widget_intersect ( &widget, 20+29, 20+10) == 1 );
    TASSERT ( widget_intersect ( &widget, 20+29, 20+25) == 1 );
    TASSERT ( widget_intersect ( &widget, 20+29, 20+40) == 1 );
    TASSERT ( widget_intersect ( &widget, 20+29, 20+50) == 0 );

    TASSERT ( widget_intersect ( &widget, 20+30, 20+0) == 0 );
    TASSERT ( widget_intersect ( &widget, 20+30, 20+10) == 0 );
    TASSERT ( widget_intersect ( &widget, 20+30, 20+25) == 0 );
    TASSERT ( widget_intersect ( &widget, 20+30, 20+40) == 0 );
    TASSERT ( widget_intersect ( &widget, 20+30, 20+50) == 0 );

    TASSERT ( widget_intersect ( NULL, 0, 0) == 0 );

    TASSERT ( widget_intersect ( &widget, -100, -100) == 0);
    TASSERT ( widget_intersect ( &widget, INT_MIN, INT_MIN) == 0);
    TASSERT ( widget_intersect ( &widget, INT_MAX, INT_MAX) == 0);
}
