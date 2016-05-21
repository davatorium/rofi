/**
 * rofi
 *
 * MIT/X11 License
 * Copyright (c) 2012 Sean Pringle <sean.pringle@gmail.com>
 * Modified 2013-2016 Qball Cow <qball@gmpclient.org>
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
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <pwd.h>
#include <ctype.h>
#include <xcb/xcb.h>
#include <pango/pango.h>
#include <pango/pango-fontmap.h>
#include <pango/pangocairo.h>
#include "helper.h"
#include "settings.h"
#include "x11-helper.h"
#include "rofi.h"
#include "view.h"

extern xcb_connection_t *xcb_connection;
extern xcb_screen_t     *xcb_screen;
static int              stored_argc   = 0;
static char             **stored_argv = NULL;

void cmd_set_arguments ( int argc, char **argv )
{
    stored_argc = argc;
    stored_argv = argv;
}

/**
 * @param info To Match information on.
 * @param res  The string being generated.
 * @param data User data
 *
 * Replace the entries. This function gets called by g_regex_replace_eval.
 *
 * @returns TRUE to stop replacement, FALSE to continue
 */
static gboolean helper_eval_cb ( const GMatchInfo *info, GString *res, gpointer data )
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
    char   *res = g_regex_replace_eval ( reg, string, -1, 0, 0, helper_eval_cb, h, NULL );
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
        char *msg = g_strdup_printf ( "Failed to parse: '%s'\nError: '%s'", string, error->message );
        rofi_view_error_dialog ( msg, FALSE );
        g_free ( msg );
        // print error.
        g_error_free ( error );
    }
    return FALSE;
}

void tokenize_free ( char ** tokens )
{
    for ( size_t i = 0; tokens && tokens[i]; i++ ) {
        g_regex_unref ( (GRegex *) tokens[i] );
    }
    g_free ( tokens );
}

static gchar *glob_to_regex ( const char *input )
{
    gchar  *r    = g_regex_escape_string ( input, -1 );
    size_t str_l = strlen ( r );
    for ( size_t i = 0; i < str_l; i++ ) {
        if ( r[i] == '\\' ) {
            if ( r[i + 1] == '*' ) {
                r[i] = '.';
            }
            else if ( r[i + 1] == '?' ) {
                r[i + 1] = 'S';
            }
            i++;
        }
    }
    return r;
}
static GRegex * create_regex ( const char *input, int case_sensitive )
{
#define R( s )    g_regex_new ( s, G_REGEX_OPTIMIZE | ( ( case_sensitive ) ? 0 : G_REGEX_CASELESS ), G_REGEX_MATCH_PARTIAL, NULL )
    GRegex *retv = NULL;
    if ( config.glob ) {
        gchar *r = glob_to_regex ( input );
        retv = R ( r );
        g_free ( r );
    }
    else if ( config.regex ) {
        retv = R ( input );
        if ( retv == NULL ) {
            gchar *r = g_regex_escape_string ( input, -1 );
            retv = R ( r );
            g_free ( r );
        }
    }
    else if ( config.fuzzy ) {
        gchar *r = g_regex_escape_string ( input, -1 );
        // TODO either strip fuzzy, or fix this pattern.
        gchar *str = g_strdup_printf ( "[%s]+", r );
        retv = R ( str );
        g_free ( r );
        g_free ( str );
    }
    else{
        // TODO; regex should be default?
        gchar *r = g_regex_escape_string ( input, -1 );
        retv = R ( r );
        g_free ( r );
    }
    return retv;
}
char **tokenize ( const char *input, int case_sensitive )
{
    if ( input == NULL || strlen(input) == 0 ) {
        return NULL;
    }

    char *saveptr = NULL, *token;
    char **retv   = NULL;
    if ( !config.tokenize ) {
        retv    = g_malloc0 ( sizeof ( char* ) * 2 );
        retv[0] = (char *) create_regex ( input, case_sensitive );
        return retv;
    }

    // First entry is always full (modified) stringtext.
    int num_tokens = 0;

    // Copy the string, 'strtok_r' modifies it.
    char *str = g_strdup ( input );

    // Iterate over tokens.
    // strtok should still be valid for utf8.
    const char * const sep = " ";
    for ( token = strtok_r ( str, sep, &saveptr ); token != NULL; token = strtok_r ( NULL, sep, &saveptr ) ) {
        retv                 = g_realloc ( retv, sizeof ( char* ) * ( num_tokens + 2 ) );
        retv[num_tokens]     = (char *) create_regex ( token, case_sensitive );
        retv[num_tokens + 1] = NULL;
        num_tokens++;
    }
    // Free str.
    g_free ( str );
    return retv;
}

// cli arg handling
int find_arg ( const char * const key )
{
    int i;

    for ( i = 0; i < stored_argc && strcasecmp ( stored_argv[i], key ); i++ ) {
        ;
    }

    return i < stored_argc ? i : -1;
}
int find_arg_str ( const char * const key, char** val )
{
    int i = find_arg ( key );

    if ( val != NULL && i > 0 && i < stored_argc - 1 ) {
        *val = stored_argv[i + 1];
        return TRUE;
    }
    return FALSE;
}

int find_arg_int ( const char * const key, int *val )
{
    int i = find_arg ( key );

    if ( val != NULL && i > 0 && i < ( stored_argc - 1 ) ) {
        *val = strtol ( stored_argv[i + 1], NULL, 10 );
        return TRUE;
    }
    return FALSE;
}
int find_arg_uint ( const char * const key, unsigned int *val )
{
    int i = find_arg ( key );

    if ( val != NULL && i > 0 && i < ( stored_argc - 1 ) ) {
        *val = strtoul ( stored_argv[i + 1], NULL, 10 );
        return TRUE;
    }
    return FALSE;
}

char helper_parse_char ( const char *arg )
{
    int len = strlen ( arg );
    // If the length is 1, it is not escaped.
    if ( len == 1 ) {
        return arg[0];
    }
    // If the length is 2 and the first character is '\', we unescape it.
    if ( len == 2 && arg[0] == '\\' ) {
        // New line
        if ( arg[1] == 'n' ) {
            return '\n';
        }
        // Bell
        else if ( arg[1] == 'a' ) {
            return '\a';
        }
        // Backspace
        else if ( arg[1] == 'b' ) {
            return '\b';
        }
        // Tab
        else if ( arg[1] == 't' ) {
            return '\t';
        }
        // Vertical tab
        else if ( arg[1] == 'v' ) {
            return '\v';
        }
        // Form feed
        else if ( arg[1] == 'f' ) {
            return '\f';
        }
        // Carriage return
        else if ( arg[1] == 'r' ) {
            return '\r';
        }
        // Forward slash
        else if ( arg[1] == '\\' ) {
            return '\\';
        }
        else if ( arg[1] == '0' ) {
            return '\0';
        }
    }
    if ( len > 2 && arg[0] == '\\' && arg[1] == 'x' ) {
        return (char) strtol ( &arg[2], NULL, 16 );
    }
    fprintf ( stderr, "Failed to parse character string: \"%s\"\n", arg );
    // for now default to newline.
    return '\n';
}

int find_arg_char ( const char * const key, char *val )
{
    int i = find_arg ( key );

    if ( val != NULL && i > 0 && i < ( stored_argc - 1 ) ) {
        *val = helper_parse_char ( stored_argv[i + 1] );
        return TRUE;
    }
    return FALSE;
}

static int regex_token_match ( char **tokens, const char *input, G_GNUC_UNUSED int not_ascii, G_GNUC_UNUSED int case_sensitive )
{
    int match = 1;

    // Do a tokenized match.
    if ( tokens ) {
        for ( int j = 0; match && tokens[j]; j++ ) {
            match = g_regex_match ( (GRegex *) tokens[j], input, G_REGEX_MATCH_PARTIAL, NULL );
        }
    }
    return match;
}

static PangoAttrList *regex_token_match_get_pango_attr ( char **tokens, const char *input, PangoAttrList *retv )
{
    // Do a tokenized match.
    if ( tokens ) {
        for ( int j = 0; tokens[j]; j++ ) {
            GMatchInfo *gmi = NULL;
            g_regex_match ( (GRegex *) tokens[j], input, G_REGEX_MATCH_PARTIAL, &gmi );
            while ( g_match_info_matches ( gmi ) ) {
                int            start, end;
                g_match_info_fetch_pos ( gmi, 0, &start, &end );
                PangoAttribute *pa = pango_attr_underline_new ( PANGO_UNDERLINE_SINGLE );
                pa->start_index = start;
                pa->end_index   = end;
                pango_attr_list_insert ( retv, pa );
                g_match_info_next ( gmi, NULL );
            }
            g_match_info_free ( gmi );
        }
    }
    return retv;
}
PangoAttrList *token_match_get_pango_attr ( char **tokens, const char *input, PangoAttrList *retv )
{
    return regex_token_match_get_pango_attr ( tokens, input, retv );
}

int token_match ( char **tokens, const char *input, int not_ascii, int case_sensitive )
{
    return regex_token_match ( tokens, input, not_ascii, case_sensitive );
}

int execute_generator ( const char * cmd )
{
    char **args = NULL;
    int  argv   = 0;
    helper_parse_setup ( config.run_command, &args, &argv, "{cmd}", cmd, NULL );

    int    fd     = -1;
    GError *error = NULL;
    g_spawn_async_with_pipes ( NULL, args, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL, &fd, NULL, &error );

    if ( error != NULL ) {
        char *msg = g_strdup_printf ( "Failed to execute: '%s'\nError: '%s'", cmd, error->message );
        fputs ( msg, stderr );
        fputs ( "\n", stderr );
        rofi_view_error_dialog ( msg, FALSE );
        g_free ( msg );
        // print error.
        g_error_free ( error );
        fd = -1;
    }
    g_strfreev ( args );
    return fd;
}

int create_pid_file ( const char *pidfile )
{
    if ( pidfile == NULL ) {
        return -1;
    }

    int fd = g_open ( pidfile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR );
    if ( fd < 0 ) {
        fprintf ( stderr, "Failed to create pid file." );
        return -1;
    }
    // Set it to close the File Descriptor on exit.
    int flags = fcntl ( fd, F_GETFD, NULL );
    flags = flags | FD_CLOEXEC;
    if ( fcntl ( fd, F_SETFD, flags, NULL ) < 0 ) {
        fprintf ( stderr, "Failed to set CLOEXEC on pidfile." );
        remove_pid_file ( fd );
        return -1;
    }
    // Try to get exclusive write lock on FD
    int retv = flock ( fd, LOCK_EX | LOCK_NB );
    if ( retv != 0 ) {
        fprintf ( stderr, "Failed to set lock on pidfile: Rofi already running?\n" );
        fprintf ( stderr, "Got error: %d %s\n", retv, strerror ( errno ) );
        remove_pid_file ( fd );
        return -1;
    }
    if ( ftruncate ( fd, (off_t) 0 ) == 0 ) {
        // Write pid, not needed, but for completeness sake.
        char    buffer[64];
        int     length = snprintf ( buffer, 64, "%i", getpid () );
        ssize_t l      = 0;
        while ( l < length ) {
            l += write ( fd, &buffer[l], length - l );
        }
    }
    return fd;
}

void remove_pid_file ( int fd )
{
    if ( fd >= 0 ) {
        if ( close ( fd ) ) {
            fprintf ( stderr, "Failed to close pidfile: '%s'\n", strerror ( errno ) );
        }
    }
}

/**
 * Do some input validation, especially the first few could break things.
 * It is good to catch them beforehand.
 *
 * This functions exits the program with 1 when it finds an invalid configuration.
 */
int config_sanity_check ( void )
{
    int     found_error = FALSE;
    GString *msg        = g_string_new (
        "<big><b>The configuration failed to validate:</b></big>\n" );
    if ( config.element_height < 1 ) {
        g_string_append_printf ( msg, "\t<b>config.element_height</b>=%d is invalid. An element needs to be atleast 1 line high.\n",
                                 config.element_height );
        config.element_height = 1;
        found_error           = TRUE;
    }
    if ( config.menu_columns == 0 ) {
        g_string_append_printf ( msg, "\t<b>config.menu_columns</b>=%d is invalid. You need at least one visible column.\n",
                                 config.menu_columns );
        config.menu_columns = 1;
        found_error         = TRUE;
    }
    if ( config.menu_width == 0 ) {
        g_string_append_printf ( msg, "<b>config.menu_width</b>=0 is invalid. You cannot have a window with no width." );
        config.menu_columns = 50;
        found_error         = TRUE;
    }
    if ( !( config.location >= WL_CENTER && config.location <= WL_WEST ) ) {
        g_string_append_printf ( msg, "\t<b>config.location</b>=%d is invalid. Value should be between %d and %d.\n",
                                 config.location, WL_CENTER, WL_WEST );
        config.location = WL_CENTER;
        found_error     = 1;
    }

    // Check size
    {
        int ssize = monitor_get_smallest_size ( );
        if ( config.monitor >= 0 ) {
            workarea mon;
            if ( monitor_get_dimension ( config.monitor, &mon ) ) {
                ssize = MIN ( mon.w, mon.h );
            }
            else{
                g_string_append_printf ( msg, "\t<b>config.monitor</b>=%d Could not find monitor.\n", config.monitor );
                ssize = 0;
            }
        }
        // Have todo an estimate here.
        if ( ( 2 * ( config.padding + config.menu_bw ) ) > ( 0.9 * ssize ) ) {
            g_string_append_printf ( msg, "\t<b>config.padding+config.menu_bw</b>=%d is too big for the minimum size of the monitor: %d.\n",
                                     ( config.padding + config.menu_bw ), ssize );

            config.padding = 0;
            config.menu_bw = 0;
            found_error    = TRUE;
        }
    }

    if ( config.menu_font ) {
        PangoFontDescription *pfd = pango_font_description_from_string ( config.menu_font );
        const char           *fam = pango_font_description_get_family ( pfd );
        int                  size = pango_font_description_get_size ( pfd );
        if ( fam == NULL || size == 0 ) {
            g_string_append_printf ( msg, "Pango failed to parse font: '%s'\n", config.menu_font );
            g_string_append_printf ( msg, "Got font family: <b>%s</b> at size <b>%d</b>\n", fam ? fam : "{unknown}", size );
            config.menu_font = NULL;
            found_error      = TRUE;
        }
        pango_font_description_free ( pfd );
    }

    if ( config.monitor == -3 ) {
        // On -3, set to location 1.
        config.location   = 1;
        config.fullscreen = 0;
    }

    if ( found_error ) {
        g_string_append ( msg, "Please update your configuration." );
        rofi_view_error_dialog ( msg->str, TRUE );
        return TRUE;
    }

    g_string_free ( msg, TRUE );
    return FALSE;
}

char *rofi_expand_path ( const char *input )
{
    char **str = g_strsplit ( input, G_DIR_SEPARATOR_S, -1 );
    for ( unsigned int i = 0; str && str[i]; i++ ) {
        // Replace ~ with current user homedir.
        if ( str[i][0] == '~' && str[i][1] == '\0' ) {
            g_free ( str[i] );
            str[i] = g_strdup ( g_get_home_dir () );
        }
        // If other user, ask getpwnam.
        else if ( str[i][0] == '~' ) {
            struct passwd *p = getpwnam ( &( str[i][1] ) );
            if ( p != NULL ) {
                g_free ( str[i] );
                str[i] = g_strdup ( p->pw_dir );
            }
        }
        else if ( i == 0 ) {
            char * s = str[i];
            if ( input[0] == G_DIR_SEPARATOR ) {
                str[i] = g_strdup_printf ( "%s%s", G_DIR_SEPARATOR_S, s );
                g_free ( s );
            }
        }
    }
    char *retv = g_build_filenamev ( str );
    g_strfreev ( str );
    return retv;
}

#define MIN3( a, b, c )    ( ( a ) < ( b ) ? ( ( a ) < ( c ) ? ( a ) : ( c ) ) : ( ( b ) < ( c ) ? ( b ) : ( c ) ) )

unsigned int levenshtein ( const char *needle, const char *haystack )
{
    unsigned int x, y, lastdiag, olddiag;
    size_t       needlelen   = g_utf8_strlen ( needle, -1 );
    size_t       haystacklen = g_utf8_strlen ( haystack, -1 );
    unsigned int column[needlelen + 1];
    for ( y = 0; y <= needlelen; y++ ) {
        column[y] = y;
    }
    for ( x = 1; x <= haystacklen; x++ ) {
        const char *needles = needle;
        column[0] = x;
        gunichar   haystackc = g_utf8_get_char ( haystack );
        if ( !config.case_sensitive ) {
            haystackc = g_unichar_tolower ( haystackc );
        }
        for ( y = 1, lastdiag = x - 1; y <= needlelen; y++ ) {
            gunichar needlec = g_utf8_get_char ( needles );
            if ( !config.case_sensitive ) {
                needlec = g_unichar_tolower ( needlec );
            }
            olddiag   = column[y];
            column[y] = MIN3 ( column[y] + 1, column[y - 1] + 1, lastdiag + ( needlec == haystackc ? 0 : 1 ) );
            lastdiag  = olddiag;
            needles   = g_utf8_next_char ( needles );
        }
        haystack = g_utf8_next_char ( haystack );
    }
    return column[needlelen];
}

char * rofi_latin_to_utf8_strdup ( const char *input, gssize length )
{
    gsize slength = 0;
    return g_convert_with_fallback ( input, length, "UTF-8", "latin1", "\uFFFD", NULL, &slength, NULL );
}

char * rofi_force_utf8 ( gchar *start )
{
    if ( start == NULL ) {
        return NULL;
    }
    const char *data = start;
    const char *end;
    gsize      length = strlen ( data );
    GString    *string;

    if ( g_utf8_validate ( data, length, &end ) ) {
        return start;
    }
    string = g_string_sized_new ( length + 16 );

    do {
        /* Valid part of the string */
        g_string_append_len ( string, data, end - data );
        /* Replacement character */
        g_string_append ( string, "\uFFFD" );
        length -= ( end - data ) + 1;
        data    = end + 1;
    } while ( !g_utf8_validate ( data, length, &end ) );

    if ( length ) {
        g_string_append_len ( string, data, length );
    }

    // Free input string.
    g_free ( start );
    return g_string_free ( string, FALSE );
}
