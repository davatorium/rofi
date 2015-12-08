#include <assert.h>
#include <locale.h>
#include <glib.h>
#include <stdio.h>
#include <helper.h>
#include <string.h>

static int test = 0;

#define TASSERT( a )    {                                \
        assert ( a );                                    \
        printf ( "Test %i passed (%s)\n", ++test, # a ); \
}

void error_dialog ( const char *msg, G_GNUC_UNUSED int markup )
{
    fputs ( msg, stderr );
}

int show_error_message ( const char *msg, int markup )
{
    error_dialog ( msg, markup );
    return 0;
}
#include <x11-helper.h>
int monitor_get_smallest_size ( G_GNUC_UNUSED Display *d )
{
    return 0;
}
int monitor_get_dimension ( G_GNUC_UNUSED Display *d, G_GNUC_UNUSED Screen *screen, G_GNUC_UNUSED int monitor, G_GNUC_UNUSED workarea *mon )
{
    return 0;
}

int main ( int argc, char ** argv )
{
    cmd_set_arguments ( argc, argv );

    if ( setlocale ( LC_ALL, "" ) == NULL ) {
        fprintf ( stderr, "Failed to set locale.\n" );
        return EXIT_FAILURE;
    }
    char **list     = NULL;
    int  llength    = 0;
    char * test_str =
        "{host} {terminal} -e bash -c \"{ssh-client} {host}; echo '{terminal} {host}'\"";
    helper_parse_setup ( test_str, &list, &llength, "{host}", "chuck",
                         "{terminal}", "x-terminal-emulator", NULL );

    TASSERT ( llength == 6 );
    TASSERT ( strcmp ( list[0], "chuck" ) == 0 );
    TASSERT ( strcmp ( list[1], "x-terminal-emulator" ) == 0 );
    TASSERT ( strcmp ( list[2], "-e" ) == 0 );
    TASSERT ( strcmp ( list[3], "bash" ) == 0 );
    TASSERT ( strcmp ( list[4], "-c" ) == 0 );
    TASSERT ( strcmp ( list[5], "ssh chuck; echo 'x-terminal-emulator chuck'" ) == 0 );
    g_strfreev ( list );

    /**
     * Test some path functions. Not easy as not sure what is right output on travis.
     */
    // Test if root is preserved.
    char *str = rofi_expand_path ( "/" );
    TASSERT ( strcmp ( str, "/" ) == 0 );
    g_free ( str );
    // Test is relative path is preserved.
    str = rofi_expand_path ( "../AUTHORS" );
    TASSERT ( strcmp ( str, "../AUTHORS" ) == 0 );
    g_free ( str );
    // Test another one.
    str = rofi_expand_path ( "/bin/false" );
    TASSERT ( strcmp ( str, "/bin/false" ) == 0 );
    g_free ( str );
    // See if user paths get expanded in full path.
    str = rofi_expand_path ( "~/" );
    const char *hd = g_get_home_dir ();
    TASSERT ( strcmp ( str, hd ) == 0 );
    g_free ( str );
    str = rofi_expand_path ( "~root/" );
    TASSERT ( str[0] == '/' );
    g_free ( str );

    /**
     * Collating.
     */
    char *res = token_collate_key ( "€ Sign", FALSE );
    TASSERT ( strcmp ( res, "€ sign" ) == 0 );
    g_free ( res );

    res = token_collate_key ( "éÉêèë Sign", FALSE );
    TASSERT ( strcmp ( res, "ééêèë sign" ) == 0 );
    g_free ( res );
    res = token_collate_key ( "éÉêèë³ Sign", TRUE );
    TASSERT ( strcmp ( res, "éÉêèë3 Sign" ) == 0 );
    g_free ( res );

    /**
     * Char function
     */

    TASSERT ( helper_parse_char ( "\\n" ) == '\n' );
    TASSERT ( helper_parse_char ( "\\a" ) == '\a' );
    TASSERT ( helper_parse_char ( "\\b" ) == '\b' );
    TASSERT ( helper_parse_char ( "\\t" ) == '\t' );
    TASSERT ( helper_parse_char ( "\\v" ) == '\v' );
    TASSERT ( helper_parse_char ( "\\f" ) == '\f' );
    TASSERT ( helper_parse_char ( "\\r" ) == '\r' );
    TASSERT ( helper_parse_char ( "\\\\" ) == '\\' );
    TASSERT ( helper_parse_char ( "\\0" ) == 0 );
    TASSERT ( helper_parse_char ( "\\x77" ) == 'w' );
    TASSERT ( helper_parse_char ( "\\x0A" ) == '\n' );

    /**
     * tokenize
     */
    config.regex = FALSE;
    config.glob  = FALSE;
    char ** retv = tokenize ( "aAp nOoT MieS 12", FALSE );
    TASSERT ( retv[0] && strcmp ( retv[0], "aap" ) == 0 );
    TASSERT ( retv[1] && strcmp ( retv[1], "noot" ) == 0 );
    TASSERT ( retv[2] && strcmp ( retv[2], "mies" ) == 0 );
    TASSERT ( retv[3] && strcmp ( retv[3], "12" ) == 0 );
    tokenize_free ( retv );
    retv = tokenize ( "blub³ bOb bEp bEE", TRUE );
    TASSERT ( retv[0] && strcmp ( retv[0], "blub3" ) == 0 );
    TASSERT ( retv[1] && strcmp ( retv[1], "bOb" ) == 0 );
    TASSERT ( retv[2] && strcmp ( retv[2], "bEp" ) == 0 );
    TASSERT ( retv[3] && strcmp ( retv[3], "bEE" ) == 0 );
    tokenize_free ( retv );
}
