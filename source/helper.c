#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h>
#include <rofi.h>
#include <helper.h>
#include <config.h>


char* fgets_s ( char* s, int n, FILE *iop, char sep )
{
    register int c;
    register char* cs;
    cs = s;

    while ( --n > 0 && ( c = getc ( iop ) ) != EOF ) {
        // put the input char into the current pointer position, then increment it
        // if a newline entered, break
        if ( ( *cs++ = c ) == sep ) {
            // Whipe separator
            cs[-1] = '\0';
            break;
        }
    }

    *cs = '\0';
    return ( c == EOF && cs == s ) ? NULL : s;
}

/**
 * Replace the entries
 */
static gboolean helper_eval_cb ( const GMatchInfo *info,
                                 GString          *res,
                                 gpointer data )
{
    gchar *match;
    gchar *r;

    match = g_match_info_fetch ( info, 0 );
    r     = g_hash_table_lookup ( (GHashTable *) data, match );
    if ( r != NULL ) {
        g_string_append ( res, r );
        g_free ( match );
    }

    return FALSE;
}

int helper_parse_setup ( char * string, char ***output, int *length, ... )
{
    GError     *error = NULL;
    GHashTable *h;
    h = g_hash_table_new ( g_str_hash, g_str_equal );
    g_hash_table_insert ( h, "{terminal}", config.terminal_emulator );
    g_hash_table_insert ( h, "{ssh-client}", config.ssh_client );
    // Add list.
    va_list ap;
    va_start ( ap, length );
    while ( 1 ) {
        char * key = va_arg ( ap, char * );
        if ( key == NULL ) {
            break;
        }
        char *value = va_arg ( ap, char * );
        if ( value == NULL ) {
            break;
        }
        g_hash_table_insert ( h, key, value );
    }
    va_end ( ap );

    // Replace hits within {-\w+}.
    GRegex *reg = g_regex_new ( "{[-\\w]+}", 0, 0, NULL );
    char   *res = g_regex_replace_eval ( reg,
                                         string, -1,
                                         0, 0, helper_eval_cb, h,
                                         NULL );
    g_regex_unref ( reg );
    g_hash_table_destroy ( h );
    if ( g_shell_parse_argv ( res, length, output, &error ) ) {
        g_free ( res );
        return TRUE;
    }
    g_free ( res );
    if ( error ) {
        char *msg = g_strdup_printf ( "Failed to parse: '%s'\nError: '%s'", string,
                                      error->message );
#ifdef error_dialog
        error_dialog ( msg );
#else
        fputs ( msg, stderr );
#endif
        g_free ( msg );
        // print error.
        g_error_free ( error );
    }
    return FALSE;
}

char **tokenize ( const char *input )
{
    if ( input == NULL ) {
        return NULL;
    }

    char *saveptr = NULL, *token;
    char **retv   = NULL;
    // First entry is always full (modified) stringtext.
    int  num_tokens = 0;

    // Copy the string, 'strtok_r' modifies it.
    char *str = g_strdup ( input );

    // Iterate over tokens.
    // strtok should still be valid for utf8.
    for ( token = strtok_r ( str, " ", &saveptr );
          token != NULL;
          token = strtok_r ( NULL, " ", &saveptr ) ) {
        // Get case insensitive version of the string.
        char *tmp = g_utf8_casefold ( token, -1 );

        retv                 = g_realloc ( retv, sizeof ( char* ) * ( num_tokens + 2 ) );
        retv[num_tokens + 1] = NULL;
        // Create compare key from the case insensitive version.
        retv[num_tokens] = g_utf8_collate_key ( tmp, -1 );
        num_tokens++;
        g_free ( tmp );
    }
    // Free str.
    g_free ( str );
    return retv;
}

// cli arg handling
int find_arg ( const int argc, char * const argv[], const char * const key )
{
    int i;

    for ( i = 0; i < argc && strcasecmp ( argv[i], key ); i++ ) {
        ;
    }

    return i < argc ? i : -1;
}
int find_arg_str ( const int argc, char * const argv[], const char * const key, char** val )
{
    int i = find_arg ( argc, argv, key );

    if ( val != NULL && i > 0 && i < argc - 1 ) {
        *val = argv[i + 1];
        return TRUE;
    }
    return FALSE;
}

int find_arg_int ( const int argc, char * const argv[], const char * const key, int *val )
{
    int i = find_arg ( argc, argv, key );

    if ( val != NULL && i > 0 && i < ( argc - 1 ) ) {
        *val = strtol ( argv[i + 1], NULL, 10 );
        return TRUE;
    }
    return FALSE;
}
int find_arg_uint ( const int argc, char * const argv[], const char * const key, unsigned int *val )
{
    int i = find_arg ( argc, argv, key );

    if ( val != NULL && i > 0 && i < ( argc - 1 ) ) {
        *val = strtoul ( argv[i + 1], NULL, 10 );
        return TRUE;
    }
    return FALSE;
}

int find_arg_char ( const int argc, char * const argv[], const char * const key, char *val )
{
    int i = find_arg ( argc, argv, key );

    if ( val != NULL && i > 0 && i < ( argc - 1 ) ) {
        int len = strlen ( argv[i + 1] );
        if ( len == 1 ) {
            *val = argv[i + 1][0];
        }
        else if ( len == 2 && argv[i + 1][0] == '\\' ) {
            if ( argv[i + 1][1] == 'n' ) {
                *val = '\n';
            }
            else if ( argv[i + 1][1] == 'a' ) {
                *val = '\a';
            }
            else if ( argv[i + 1][1] == 'b' ) {
                *val = '\b';
            }
            else if ( argv[i + 1][1] == 't' ) {
                *val = '\t';
            }
            else if ( argv[i + 1][1] == 'v' ) {
                *val = '\v';
            }
            else if ( argv[i + 1][1] == 'f' ) {
                *val = '\f';
            }
            else if ( argv[i + 1][1] == 'r' ) {
                *val = '\r';
            }
            else if ( argv[i + 1][1] == '\\' ) {
                *val = '\\';
            }
            else {
                fprintf ( stderr, "Failed to parse command-line argument." );
                exit ( 1 );
            }
        }
        else{
            fprintf ( stderr, "Failed to parse command-line argument." );
            exit ( 1 );
        }
        return TRUE;
    }
    return FALSE;
}

