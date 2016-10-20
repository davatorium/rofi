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

#define TASSERT( a )        {                            \
        assert ( a );                                    \
        printf ( "Test %i passed (%s)\n", ++test, # a ); \
}

int rofi_view_error_dialog ( const char *msg, G_GNUC_UNUSED int markup )
{
    fputs ( msg, stderr );
    return TRUE;
}

int show_error_message ( const char *msg, int markup )
{
    fputs ( msg, stderr );
    return 0;
}
xcb_screen_t          *xcb_screen;
xcb_ewmh_connection_t xcb_ewmh;
int                   xcb_screen_nbr;
#include <x11-helper.h>

int main ( int argc, char ** argv )
{
    if ( setlocale ( LC_ALL, "" ) == NULL ) {
        fprintf ( stderr, "Failed to set locale.\n" );
        return EXIT_FAILURE;
    }
    // Pid test.
    // Tests basic functionality of writing it, locking, seeing if I can write same again
    // And close/reopen it again.
    {
        tokenize_free ( NULL );
    }
    {
        config.matching_method = MM_NORMAL;
        GRegex **tokens = tokenize ( "noot", FALSE );

        TASSERT ( token_match ( tokens, "aap noot mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nootap mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap Noot mies") == TRUE );
        TASSERT ( token_match ( tokens, "Nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "noOTap mies") == TRUE );

        tokenize_free ( tokens );

        tokens = tokenize ( "noot", TRUE );

        TASSERT ( token_match ( tokens, "aap noot mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nootap mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap Noot mies") == FALSE );
        TASSERT ( token_match ( tokens, "Nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "noOTap mies") == FALSE );

        tokenize_free ( tokens );
        tokens = tokenize ( "no ot", FALSE );
        TASSERT ( token_match ( tokens, "aap noot mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nootap mies") == TRUE );
        TASSERT ( token_match ( tokens, "noap miesot") == TRUE );
        tokenize_free ( tokens );
    }
    {
        config.matching_method = MM_GLOB;
        GRegex **tokens = tokenize ( "noot", FALSE );

        TASSERT ( token_match ( tokens, "aap noot mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nootap mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap Noot mies") == TRUE );
        TASSERT ( token_match ( tokens, "Nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "noOTap mies") == TRUE );

        tokenize_free ( tokens );

        tokens = tokenize ( "noot", TRUE );

        TASSERT ( token_match ( tokens, "aap noot mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nootap mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap Noot mies") == FALSE );
        TASSERT ( token_match ( tokens, "Nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "noOTap mies") == FALSE );
        tokenize_free ( tokens );

        tokens = tokenize ( "no ot", FALSE );
        TASSERT ( token_match ( tokens, "aap noot mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nootap mies") == TRUE );
        TASSERT ( token_match ( tokens, "noap miesot") == TRUE );
        tokenize_free ( tokens );
        
        tokens = tokenize ( "n?ot", FALSE );
        TASSERT ( token_match ( tokens, "aap noot mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nootap mies") == TRUE );
        TASSERT ( token_match ( tokens, "noap miesot") == FALSE);
        tokenize_free ( tokens );
        tokens = tokenize ( "n*ot", FALSE );
        TASSERT ( token_match ( tokens, "aap noot mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nootap mies") == TRUE );
        TASSERT ( token_match ( tokens, "noap miesot") == TRUE);
        tokenize_free ( tokens );

        tokens = tokenize ( "n* ot", FALSE );
        TASSERT ( token_match ( tokens, "aap noot mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nootap mies") == TRUE );
        TASSERT ( token_match ( tokens, "noap miesot") == TRUE);
        TASSERT ( token_match ( tokens, "ot nap mies") == TRUE);
        tokenize_free ( tokens );
    }
    {
        config.matching_method = MM_FUZZY;
        GRegex **tokens = tokenize ( "noot", FALSE );

        TASSERT ( token_match ( tokens, "aap noot mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nootap mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap Noot mies") == TRUE );
        TASSERT ( token_match ( tokens, "Nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "noOTap mies") == TRUE );

        tokenize_free ( tokens );

        tokens = tokenize ( "noot", TRUE );

        TASSERT ( token_match ( tokens, "aap noot mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nootap mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap Noot mies") == FALSE );
        TASSERT ( token_match ( tokens, "Nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "noOTap mies") == FALSE );
        tokenize_free ( tokens );

        tokens = tokenize ( "no ot", FALSE );
        TASSERT ( token_match ( tokens, "aap noot mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nootap mies") == TRUE );
        TASSERT ( token_match ( tokens, "noap miesot") == TRUE );
        tokenize_free ( tokens );
        
        tokens = tokenize ( "n ot", FALSE );
        TASSERT ( token_match ( tokens, "aap noot mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nootap mies") == TRUE );
        TASSERT ( token_match ( tokens, "noap miesot") == TRUE);
        tokenize_free ( tokens );
        tokens = tokenize ( "ont", FALSE );
        TASSERT ( token_match ( tokens, "aap noot mies") == FALSE);
        TASSERT ( token_match ( tokens, "aap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nootap nmiest") == TRUE );
        tokenize_free ( tokens );

        tokens = tokenize ( "o n t", FALSE );
        TASSERT ( token_match ( tokens, "aap noot mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nootap mies") == TRUE );
        TASSERT ( token_match ( tokens, "noap miesot") == TRUE);
        TASSERT ( token_match ( tokens, "ot nap mies") == TRUE);
        tokenize_free ( tokens );
    }
    {
        config.matching_method = MM_REGEX;
        GRegex **tokens = tokenize ( "noot", FALSE );

        TASSERT ( token_match ( tokens, "aap noot mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nootap mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap Noot mies") == TRUE );
        TASSERT ( token_match ( tokens, "Nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "noOTap mies") == TRUE );

        tokenize_free ( tokens );

        tokens = tokenize ( "noot", TRUE );

        TASSERT ( token_match ( tokens, "aap noot mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nootap mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap Noot mies") == FALSE );
        TASSERT ( token_match ( tokens, "Nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "noOTap mies") == FALSE );
        tokenize_free ( tokens );

        tokens = tokenize ( "no ot", FALSE );
        TASSERT ( token_match ( tokens, "aap noot mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nootap mies") == TRUE );
        TASSERT ( token_match ( tokens, "noap miesot") == TRUE );
        tokenize_free ( tokens );
        
        tokens = tokenize ( "n.?ot", FALSE );
        TASSERT ( token_match ( tokens, "aap noot mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nootap mies") == TRUE );
        TASSERT ( token_match ( tokens, "noap miesot") == FALSE);
        tokenize_free ( tokens );
        tokens = tokenize ( "n[oa]{2}t", FALSE );
        TASSERT ( token_match ( tokens, "aap noot mies") == TRUE );
        TASSERT ( token_match ( tokens, "aap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nootap mies") == TRUE );
        TASSERT ( token_match ( tokens, "noat miesot") == TRUE);
        TASSERT ( token_match ( tokens, "noaat miesot") == FALSE);
        tokenize_free ( tokens );

        tokens = tokenize ( "^(aap|noap)\\sMie.*", FALSE );
        TASSERT ( token_match ( tokens, "aap noot mies") == FALSE );
        TASSERT ( token_match ( tokens, "aap mies") == TRUE);
        TASSERT ( token_match ( tokens, "nooaap mies") == FALSE );
        TASSERT ( token_match ( tokens, "nootap mies") == FALSE );
        TASSERT ( token_match ( tokens, "noap miesot") == TRUE);
        TASSERT ( token_match ( tokens, "ot nap mies") == FALSE );
        tokenize_free ( tokens );
    }
}
