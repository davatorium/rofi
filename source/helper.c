#include <glib.h>
#include <rofi.h>
#include <helper.h>
#include <config.h>

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
#endif
        g_free ( msg );
        // print error.
        g_error_free ( error );
    }
    return FALSE;
}
