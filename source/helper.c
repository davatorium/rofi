#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h>
#include <helper.h>
#include <config.h>
#include <rofi.h>

/**
 *  `fgets` implementation with custom separator.
 */
char* fgets_s ( char* s, int n, FILE *iop, char sep )
{
    // Map these to registers.
    register int c;
    register char* cs;
    cs = s;
    // read until EOF or buffer is full.
    while ( --n > 0 && ( c = getc ( iop ) ) != EOF ) {
        // put the input char into the current pointer position, then increment it
        // if a newline entered, break
        if ( ( *cs++ = c ) == sep ) {
            // Whipe separator
            cs[-1] = '\0';
            break;
        }
    }
    // Always, 0 terminate the buffer.
    *cs = '\0';
    // if last read was end of file and current index is start, we are done:
    // Return NULL.
    return ( c == EOF && cs == s ) ? NULL : s;
}

/**
 * @param info The Match informati  on.
 * @param res  The string being generated.
 * @param data User data
 *
 * Replace the entries. This function gets called by g_regex_replace_eval.
 *
 * @returns TRUE to stop replacement, FALSE to continue
 */
static gboolean helper_eval_cb ( const GMatchInfo *info,
                                 GString          *res,
                                 gpointer data )
{
    gchar *match;
    // Get the match
    match = g_match_info_fetch ( info, 0 );
    if ( match != NULL ) {
        // Lookup the match, so we can replace it.
        gchar *r = g_hash_table_lookup ( (GHashTable *) data, match );
        if ( r != NULL ) {
            // Append the replacement to the string.
            g_string_append ( res, r );
        }
        // Free match.
        g_free ( match );
    }
    // Continue replacement.
    return FALSE;
}

int helper_parse_setup ( char * string, char ***output, int *length, ... )
{
    GError     *error = NULL;
    GHashTable *h;
    h = g_hash_table_new ( g_str_hash, g_str_equal );
    // By default, we insert terminal and ssh-client
    g_hash_table_insert ( h, "{terminal}", config.terminal_emulator );
    g_hash_table_insert ( h, "{ssh-client}", config.ssh_client );
    // Add list from variable arguments.
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
    // Free regex.
    g_regex_unref ( reg );
    // Destroy key-value storage.
    g_hash_table_destroy ( h );
    // Parse the string into shell arguments.
    if ( g_shell_parse_argv ( res, length, output, &error ) ) {
        g_free ( res );
        return TRUE;
    }
    g_free ( res );
    // Throw error if shell parsing fails.
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

char *token_collate_key ( const char *token, int case_sensitive )
{
    char *tmp, *compk;

    if ( case_sensitive ) {
        tmp = g_strdup ( token );
    }
    else {
        tmp = g_utf8_casefold ( token, -1 );
    }

    compk = g_utf8_collate_key ( tmp, -1 );
    g_free ( tmp );

    return compk;
}

char **tokenize ( const char *input, int case_sensitive )
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
        retv                 = g_realloc ( retv, sizeof ( char* ) * ( num_tokens + 2 ) );
        retv[num_tokens]     = token_collate_key ( token, case_sensitive );
        retv[num_tokens + 1] = NULL;
        num_tokens++;
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
int find_arg_str_alloc ( const int argc, char * const argv[], const char * const key, char** val )
{
    int i = find_arg ( argc, argv, key );

    if ( val != NULL && i > 0 && i < argc - 1 ) {
        *val = g_strdup ( argv[i + 1] );
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
        // If the length is 1, it is not escaped.
        if ( len == 1 ) {
            *val = argv[i + 1][0];
        }
        // If the length is 2 and the first character is '\', we unescape it.
        else if ( len == 2 && argv[i + 1][0] == '\\' ) {
            // New line
            if ( argv[i + 1][1] == 'n' ) {
                *val = '\n';
            }
            // Bell
            else if ( argv[i + 1][1] == 'a' ) {
                *val = '\a';
            }
            // Backspace
            else if ( argv[i + 1][1] == 'b' ) {
                *val = '\b';
            }
            // Tab
            else if ( argv[i + 1][1] == 't' ) {
                *val = '\t';
            }
            // Vertical tab
            else if ( argv[i + 1][1] == 'v' ) {
                *val = '\v';
            }
            // Form feed
            else if ( argv[i + 1][1] == 'f' ) {
                *val = '\f';
            }
            // Carriage return
            else if ( argv[i + 1][1] == 'r' ) {
                *val = '\r';
            }
            // Forward slash
            else if ( argv[i + 1][1] == '\\' ) {
                *val = '\\';
            }
            // Otherwise it is not valid and throw error
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

/**
 * Shared 'token_match' function.
 * Matches tokenized.
 */
int token_match ( char **tokens, const char *input, int case_sensitive,
                  __attribute__( ( unused ) ) int index,
                  __attribute__( ( unused ) ) void *data )
{
    int  match  = 1;
    char *compk = token_collate_key ( input, case_sensitive );

    // Do a tokenized match.
    if ( tokens ) {
        for ( int j = 0; match && tokens[j]; j++ ) {
            match = ( strstr ( compk, tokens[j] ) != NULL );
        }
    }
    g_free ( compk );
    return match;
}

int execute_generator ( char * cmd )
{
    char **args = NULL;
    int  argv   = 0;
    helper_parse_setup ( config.run_command, &args, &argv, "{cmd}", cmd, NULL );

    int    fd     = -1;
    GError *error = NULL;
    g_spawn_async_with_pipes ( NULL,
                               args,
                               NULL,
                               G_SPAWN_SEARCH_PATH,
                               NULL,
                               NULL,
                               NULL,
                               NULL, &fd, NULL,
                               &error );

    if ( error != NULL ) {
        char *msg = g_strdup_printf ( "Failed to execute: '%s'\nError: '%s'", cmd,
                                      error->message );
#ifdef error_dialog
        error_dialog ( msg );
#else
        fputs ( msg, stderr );
#endif
        g_free ( msg );
        // print error.
        g_error_free ( error );
        fd = -1;
    }
    g_strfreev ( args );
    return fd;
}
