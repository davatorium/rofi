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

void color_separator ( G_GNUC_UNUSED void *d )
{

}

int main ( G_GNUC_UNUSED int argc, G_GNUC_UNUSED char **argv )
{
    scrollbar * sb = scrollbar_create ( 0, 0, 10, 100);

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
    TASSERTE ( cl, 900);
    cl = scrollbar_clicked ( sb, 20 );
    TASSERTE ( cl, 1900);
    cl = scrollbar_clicked ( sb, 0 );
    TASSERTE ( cl, 0);
    cl = scrollbar_clicked ( sb, 99 );
    TASSERTE ( cl, 9800);

    scrollbar_set_max_value ( sb, 100 );
    for ( unsigned int i = 1; i < 99; i++ ){
        cl = scrollbar_clicked ( sb, i );
        TASSERTE ( cl, i-1);
    }

    scrollbar_set_max_value ( sb, 200 );
    for ( unsigned int i = 1; i < 100; i++ ){
        cl = scrollbar_clicked ( sb, i );
        TASSERTE ( cl, i*2-2);
    }
    widget_free( WIDGET (sb ) );
}
