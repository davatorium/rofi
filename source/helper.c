/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2012 Sean Pringle <sean.pringle@gmail.com>
 * Copyright © 2013-2020 Qball Cow <qball@gmpclient.org>
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

/** The log domain for this helper. */
#define G_LOG_DOMAIN    "Helper"

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
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
#include <pango/pango.h>
#include <pango/pango-fontmap.h>
#include <pango/pangocairo.h>
#include "display.h"
#include "xcb.h"
#include "helper.h"
#include "helper-theme.h"
#include "settings.h"
#include "rofi.h"
#include "view.h"

/**
 * Textual description of positioning rofi.
 */
const char *const monitor_position_entries[] = {
    "on focused monitor",
    "on focused window",
    "at mouse pointer",
    "on monitor with focused window",
    "on monitor that has mouse pointer"
};
/** copy of the argc for use in commandline argument parser. */
static int        stored_argc = 0;
/** copy of the argv pointer for use in the commandline argument parser */
static char       **stored_argv = NULL;

char *helper_string_replace_if_exists_v ( char * string, GHashTable *h );

void cmd_set_arguments ( int argc, char **argv )
{
    stored_argc = argc;
    stored_argv = argv;
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
        if ( key == (char *) 0 ) {
            break;
        }
        char *value = va_arg ( ap, char * );
        if ( value == (char *) 0 ) {
            break;
        }
        g_hash_table_insert ( h, key, value );
    }
    va_end ( ap );

    char *res = helper_string_replace_if_exists_v ( string, h );
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

void helper_tokenize_free ( rofi_int_matcher ** tokens )
{
    for ( size_t i = 0; tokens && tokens[i]; i++ ) {
        g_regex_unref ( (GRegex *) tokens[i]->regex );
        g_free ( tokens[i] );
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
static gchar *fuzzy_to_regex ( const char * input )
{
    GString *str = g_string_new ( "" );
    gchar   *r   = g_regex_escape_string ( input, -1 );
    gchar   *iter;
    int     first = 1;
    for ( iter = r; iter && *iter != '\0'; iter = g_utf8_next_char ( iter ) ) {
        if ( first ) {
            g_string_append ( str, "(" );
        }
        else {
            g_string_append ( str, ".*?(" );
        }
        if ( *iter == '\\' ) {
            g_string_append_c ( str, '\\' );
            iter = g_utf8_next_char ( iter );
            // If EOL, break out of for loop.
            if ( ( *iter ) == '\0' ) {
                break;
            }
        }
        g_string_append_unichar ( str, g_utf8_get_char ( iter ) );
        g_string_append ( str, ")" );
        first = 0;
    }
    g_free ( r );
    char *retv = str->str;
    g_string_free ( str, FALSE );
    return retv;
}

static char *utf8_helper_simplify_string ( const char *s )
{
    gunichar buf2[G_UNICHAR_MAX_DECOMPOSITION_LENGTH] = { 0, };
    char     buf[6]                                   = { 0, };
    // Compose the string in maximally composed form.
    char     * str    = g_malloc0 ( ( g_utf8_strlen ( s, 0 ) * 6 + 2 ) );
    char     *striter = str;
    for ( const char *iter = s; iter && *iter; iter = g_utf8_next_char ( iter ) ) {
        gunichar uc = g_utf8_get_char ( iter );
        int      l  = 0;
        gsize    dl = g_unichar_fully_decompose ( uc, FALSE, buf2, G_UNICHAR_MAX_DECOMPOSITION_LENGTH );
        if ( dl ) {
            l = g_unichar_to_utf8 ( buf2[0], buf );
        }
        else {
            l = g_unichar_to_utf8 ( uc, buf );
        }
        memcpy ( striter, buf, l );
        striter += l;
    }

    return str;
}

// Macro for quickly generating regex for matching.
static inline GRegex * R ( const char *s, int case_sensitive  )
{
    if ( config.normalize_match ) {
        char   *str = utf8_helper_simplify_string ( s );

        GRegex *r = g_regex_new ( str, G_REGEX_OPTIMIZE | ( ( case_sensitive ) ? 0 : G_REGEX_CASELESS ), 0, NULL );

        g_free ( str );
        return r;
    }
    else {
        return g_regex_new ( s, G_REGEX_OPTIMIZE | ( ( case_sensitive ) ? 0 : G_REGEX_CASELESS ), 0, NULL );
    }
}

static rofi_int_matcher * create_regex ( const char *input, int case_sensitive )
{
    GRegex           * retv = NULL;
    gchar            *r;
    rofi_int_matcher *rv = g_malloc0 ( sizeof ( rofi_int_matcher ) );
    if ( input && input[0] == config.matching_negate_char ) {
        rv->invert = 1;
        input++;
    }
    switch ( config.matching_method )
    {
    case MM_GLOB:
        r    = glob_to_regex ( input );
        retv = R ( r, case_sensitive );
        g_free ( r );
        break;
    case MM_REGEX:
        retv = R ( input, case_sensitive );
        if ( retv == NULL ) {
            r    = g_regex_escape_string ( input, -1 );
            retv = R ( r, case_sensitive );
            g_free ( r );
        }
        break;
    case MM_FUZZY:
        r    = fuzzy_to_regex ( input );
        retv = R ( r, case_sensitive );
        g_free ( r );
        break;
    default:
        r    = g_regex_escape_string ( input, -1 );
        retv = R ( r, case_sensitive );
        g_free ( r );
        break;
    }
    rv->regex = retv;
    return rv;
}
rofi_int_matcher **helper_tokenize ( const char *input, int case_sensitive )
{
    if ( input == NULL ) {
        return NULL;
    }
    size_t len = strlen ( input );
    if ( len == 0 ) {
        return NULL;
    }

    char             *saveptr = NULL, *token;
    rofi_int_matcher **retv = NULL;
    if ( !config.tokenize ) {
        retv    = g_malloc0 ( sizeof ( rofi_int_matcher* ) * 2 );
        retv[0] = create_regex ( input, case_sensitive );
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
        retv                 = g_realloc ( retv, sizeof ( rofi_int_matcher* ) * ( num_tokens + 2 ) );
        retv[num_tokens]     = create_regex ( token, case_sensitive );
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

const char ** find_arg_strv ( const char *const key )
{
    const char **retv = NULL;
    int        length = 0;
    for ( int i = 0; i < stored_argc; i++ ) {
        if (  i < ( stored_argc - 1 ) && strcasecmp ( stored_argv[i], key ) == 0 ) {
            length++;
        }
    }
    if ( length > 0 ) {
        retv = g_malloc0 ( ( length + 1 ) * sizeof ( char* ) );
        int index = 0;
        for ( int i = 0; i < stored_argc; i++ ) {
            if ( i < ( stored_argc - 1 ) && strcasecmp ( stored_argv[i], key ) == 0 ) {
                retv[index++] = stored_argv[i + 1];
            }
        }
    }
    return retv;
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
    const size_t len = strlen ( arg );
    // If the length is 1, it is not escaped.
    if ( len == 1 ) {
        return arg[0];
    }
    // If the length is 2 and the first character is '\', we unescape it.
    if ( len == 2 && arg[0] == '\\' ) {
        switch ( arg[1] )
        {
        // New line
        case 'n': return '\n';
        // Bell
        case  'a': return '\a';
        // Backspace
        case 'b': return '\b';
        // Tab
        case  't': return '\t';
        // Vertical tab
        case  'v': return '\v';
        // Form feed
        case  'f': return '\f';
        // Carriage return
        case  'r': return '\r';
        // Forward slash
        case  '\\': return '\\';
        // 0 line.
        case  '0': return '\0';
        default:
            break;
        }
    }
    if ( len > 2 && arg[0] == '\\' && arg[1] == 'x' ) {
        return (char) strtol ( &arg[2], NULL, 16 );
    }
    g_warning ( "Failed to parse character string: \"%s\"", arg );
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

PangoAttrList *helper_token_match_get_pango_attr ( RofiHighlightColorStyle th, rofi_int_matcher**tokens, const char *input, PangoAttrList *retv )
{
    // Disable highlighting for normalize match, not supported atm.
    if ( config.normalize_match ) {
        return retv;
    }
    // Do a tokenized match.
    if ( tokens ) {
        for ( int j = 0; tokens[j]; j++ ) {
            GMatchInfo *gmi = NULL;
            if ( tokens[j]->invert ) {
                continue;
            }
            g_regex_match ( tokens[j]->regex, input, G_REGEX_MATCH_PARTIAL, &gmi );
            while ( g_match_info_matches ( gmi ) ) {
                int count = g_match_info_get_match_count ( gmi );
                for ( int index = ( count > 1 ) ? 1 : 0; index < count; index++ ) {
                    int start, end;
                    g_match_info_fetch_pos ( gmi, index, &start, &end );
                    if ( th.style & ROFI_HL_BOLD ) {
                        PangoAttribute *pa = pango_attr_weight_new ( PANGO_WEIGHT_BOLD );
                        pa->start_index = start;
                        pa->end_index   = end;
                        pango_attr_list_insert ( retv, pa );
                    }
                    if ( th.style & ROFI_HL_UNDERLINE ) {
                        PangoAttribute *pa = pango_attr_underline_new ( PANGO_UNDERLINE_SINGLE );
                        pa->start_index = start;
                        pa->end_index   = end;
                        pango_attr_list_insert ( retv, pa );
                    }
                    if ( th.style & ROFI_HL_STRIKETHROUGH ) {
                        PangoAttribute *pa = pango_attr_strikethrough_new ( TRUE );
                        pa->start_index = start;
                        pa->end_index   = end;
                        pango_attr_list_insert ( retv, pa );
                    }
                    if ( th.style & ROFI_HL_SMALL_CAPS ) {
                        PangoAttribute *pa = pango_attr_variant_new ( PANGO_VARIANT_SMALL_CAPS );
                        pa->start_index = start;
                        pa->end_index   = end;
                        pango_attr_list_insert ( retv, pa );
                    }
                    if ( th.style & ROFI_HL_ITALIC ) {
                        PangoAttribute *pa = pango_attr_style_new ( PANGO_STYLE_ITALIC );
                        pa->start_index = start;
                        pa->end_index   = end;
                        pango_attr_list_insert ( retv, pa );
                    }
                    if ( th.style & ROFI_HL_COLOR ) {
                        PangoAttribute *pa = pango_attr_foreground_new (
                            th.color.red * 65535,
                            th.color.green * 65535,
                            th.color.blue * 65535 );
                        pa->start_index = start;
                        pa->end_index   = end;
                        pango_attr_list_insert ( retv, pa );

                        if ( th.color.alpha < 1.0 ) {
                            pa              = pango_attr_foreground_alpha_new ( th.color.alpha * 65535 );
                            pa->start_index = start;
                            pa->end_index   = end;
                            pango_attr_list_insert ( retv, pa );
                        }
                    }
                }
                g_match_info_next ( gmi, NULL );
            }
            g_match_info_free ( gmi );
        }
    }
    return retv;
}

int helper_token_match ( rofi_int_matcher* const *tokens, const char *input )
{
    int match = TRUE;
    // Do a tokenized match.
    if ( tokens ) {
        if ( config.normalize_match ) {
            char *r = utf8_helper_simplify_string ( input );
            for ( int j = 0; match && tokens[j]; j++ ) {
                match  = g_regex_match ( tokens[j]->regex, r, 0, NULL );
                match ^= tokens[j]->invert;
            }
            g_free ( r );
        }
        else {
            for ( int j = 0; match && tokens[j]; j++ ) {
                match  = g_regex_match ( tokens[j]->regex, input, 0, NULL );
                match ^= tokens[j]->invert;
            }
        }
    }
    return match;
}

int execute_generator ( const char * cmd )
{
    char **args = NULL;
    int  argv   = 0;
    helper_parse_setup ( config.run_command, &args, &argv, "{cmd}", cmd, (char *) 0 );

    int    fd     = -1;
    GError *error = NULL;
    g_spawn_async_with_pipes ( NULL, args, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL, &fd, NULL, &error );

    if ( error != NULL ) {
        char *msg = g_strdup_printf ( "Failed to execute: '%s'\nError: '%s'", cmd, error->message );
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
        g_warning ( "Failed to create pid file: '%s'.", pidfile );
        return -1;
    }
    // Set it to close the File Descriptor on exit.
    int flags = fcntl ( fd, F_GETFD, NULL );
    flags = flags | FD_CLOEXEC;
    if ( fcntl ( fd, F_SETFD, flags, NULL ) < 0 ) {
        g_warning ( "Failed to set CLOEXEC on pidfile." );
        remove_pid_file ( fd );
        return -1;
    }
    // Try to get exclusive write lock on FD
    int retv = flock ( fd, LOCK_EX | LOCK_NB );
    if ( retv != 0 ) {
        g_warning ( "Failed to set lock on pidfile: Rofi already running?" );
        g_warning ( "Got error: %d %s", retv, g_strerror ( errno ) );
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
            g_warning ( "Failed to close pidfile: '%s'", g_strerror ( errno ) );
        }
    }
}

gboolean helper_validate_font ( PangoFontDescription *pfd, const char *font )
{
    const char *fam = pango_font_description_get_family ( pfd );
    int        size = pango_font_description_get_size ( pfd );
    if ( fam == NULL || size == 0 ) {
        g_debug ( "Pango failed to parse font: '%s'", font );
        g_debug ( "Got family: <b>%s</b> at size: <b>%d</b>", fam ? fam : "{unknown}", size );
        return FALSE;
    }
    return TRUE;
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

    if ( config.sorting_method ) {
        if ( g_strcmp0 ( config.sorting_method, "normal" ) == 0 ) {
            config.sorting_method_enum = SORT_NORMAL;
        }
        else if ( g_strcmp0 ( config.sorting_method, "levenshtein" ) == 0 ) {
            config.sorting_method_enum = SORT_NORMAL;
        }
        else if ( g_strcmp0 ( config.sorting_method, "fzf" ) == 0 ) {
            config.sorting_method_enum = SORT_FZF;
        }
        else {
            g_string_append_printf ( msg, "\t<b>config.sorting_method</b>=%s is not a valid sorting strategy.\nValid options are: normal or fzf.\n",
                                     config.sorting_method );
            found_error = 1;
        }
    }

    if ( config.matching ) {
        if ( g_strcmp0 ( config.matching, "regex" ) == 0 ) {
            config.matching_method = MM_REGEX;
        }
        else if ( g_strcmp0 ( config.matching, "glob" ) == 0 ) {
            config.matching_method = MM_GLOB;
        }
        else if ( g_strcmp0 ( config.matching, "fuzzy" ) == 0 ) {
            config.matching_method = MM_FUZZY;
        }
        else if ( g_strcmp0 ( config.matching, "normal" ) == 0 ) {
            config.matching_method = MM_NORMAL;;
        }
        else {
            g_string_append_printf ( msg, "\t<b>config.matching</b>=%s is not a valid matching strategy.\nValid options are: glob, regex, fuzzy or normal.\n",
                                     config.matching );
            found_error = 1;
        }
    }

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
    if ( !( config.location >= 0 && config.location <= 8 ) ) {
        g_string_append_printf ( msg, "\t<b>config.location</b>=%d is invalid. Value should be between %d and %d.\n",
                                 config.location, 0, 8 );
        config.location = WL_CENTER;
        found_error     = 1;
    }

    // Check size
    {
        workarea mon;
        if ( !monitor_active ( &mon ) ) {
            const char *name = config.monitor;
            if ( name && name[0] == '-' ) {
                int index = name[1] - '0';
                if ( index < 5 && index > 0 ) {
                    name = monitor_position_entries[index - 1];
                }
            }
            g_string_append_printf ( msg, "\t<b>config.monitor</b>=%s Could not find monitor.\n", name );
            found_error = TRUE;
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

    if ( g_strcmp0 ( config.monitor, "-3" ) == 0 ) {
        // On -3, set to location 1.
        config.location   = 1;
    }

    if ( found_error ) {
        g_string_append ( msg, "Please update your configuration." );
        rofi_add_error_message ( msg );
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

/** Return the minimum value of a,b,c */
#define MIN3( a, b, c )    ( ( a ) < ( b ) ? ( ( a ) < ( c ) ? ( a ) : ( c ) ) : ( ( b ) < ( c ) ? ( b ) : ( c ) ) )

unsigned int levenshtein ( const char *needle, const glong needlelen, const char *haystack, const glong haystacklen )
{
    if ( needlelen == G_MAXLONG ) {
        // String to long, we cannot handle this.
        return UINT_MAX;
    }
    unsigned int column[needlelen + 1];
    for ( glong y = 0; y < needlelen; y++ ) {
        column[y] = y;
    }
    // Removed out of the loop, otherwise static code analyzers think it is unset.. silly but true.
    // old loop: for ( glong y = 0; y <= needlelen; y++)
    column[needlelen] = needlelen;
    for ( glong x = 1; x <= haystacklen; x++ ) {
        const char *needles = needle;
        column[0] = x;
        gunichar   haystackc = g_utf8_get_char ( haystack );
        if ( !config.case_sensitive ) {
            haystackc = g_unichar_tolower ( haystackc );
        }
        for ( glong y = 1, lastdiag = x - 1; y <= needlelen; y++ ) {
            gunichar needlec = g_utf8_get_char ( needles );
            if ( !config.case_sensitive ) {
                needlec = g_unichar_tolower ( needlec );
            }
            unsigned int olddiag = column[y];
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

gchar *rofi_escape_markup ( gchar *text )
{
    if ( text == NULL ) {
        return NULL;
    }
    gchar *ret = g_markup_escape_text ( text, -1 );
    g_free ( text );
    return ret;
}

char * rofi_force_utf8 ( const gchar *data, ssize_t length )
{
    if ( data == NULL ) {
        return NULL;
    }
    const char *end;
    GString    *string;

    if ( g_utf8_validate ( data, length, &end ) ) {
        return g_memdup ( data, length + 1 );
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

    return g_string_free ( string, FALSE );
}

/****
 * FZF like scorer
 */

/** Max length of input to score. */
#define FUZZY_SCORER_MAX_LENGTH         256
/** minimum score */
#define MIN_SCORE                       ( INT_MIN / 2 )
/** Leading gap score */
#define LEADING_GAP_SCORE               -4
/** gap score */
#define GAP_SCORE                       -5
/** start of word score */
#define WORD_START_SCORE                50
/** non-word score */
#define NON_WORD_SCORE                  40
/** CamelCase score */
#define CAMEL_SCORE                     ( WORD_START_SCORE + GAP_SCORE - 1 )
/** Consecutive score */
#define CONSECUTIVE_SCORE               ( WORD_START_SCORE + GAP_SCORE )
/** non-start multiplier */
#define PATTERN_NON_START_MULTIPLIER    1
/** start multiplier */
#define PATTERN_START_MULTIPLIER        2

/**
 * Character classification.
 */
enum CharClass
{
    /* Lower case */
    LOWER,
    /* Upper case */
    UPPER,
    /* Number */
    DIGIT,
    /* non word character */
    NON_WORD
};

/**
 * @param c The character to determine class of
 *
 * @returns the class of the character c.
 */
static enum CharClass rofi_scorer_get_character_class ( gunichar c )
{
    if ( g_unichar_islower ( c ) ) {
        return LOWER;
    }
    if ( g_unichar_isupper ( c ) ) {
        return UPPER;
    }
    if ( g_unichar_isdigit ( c ) ) {
        return DIGIT;
    }
    return NON_WORD;
}

/**
 * @param prev The previous character.
 * @param curr The current character
 *
 * Scrore the transition.
 *
 * @returns score of the transition.
 */
static int rofi_scorer_get_score_for ( enum CharClass prev, enum CharClass curr )
{
    if ( prev == NON_WORD && curr != NON_WORD ) {
        return WORD_START_SCORE;
    }
    if ( ( prev == LOWER && curr == UPPER ) ||
         ( prev != DIGIT && curr == DIGIT ) ) {
        return CAMEL_SCORE;
    }
    if ( curr == NON_WORD ) {
        return NON_WORD_SCORE;
    }
    return 0;
}

int rofi_scorer_fuzzy_evaluate ( const char *pattern, glong plen, const char *str, glong slen )
{
    if ( slen > FUZZY_SCORER_MAX_LENGTH ) {
        return -MIN_SCORE;
    }
    glong    pi, si;
    // whether we are aligning the first character of pattern
    gboolean pfirst = TRUE;
    // whether the start of a word in pattern
    gboolean pstart = TRUE;
    // score for each position
    int      *score = g_malloc_n ( slen, sizeof ( int ) );
    // dp[i]: maximum value by aligning pattern[0..pi] to str[0..si]
    int      *dp = g_malloc_n ( slen, sizeof ( int ) );
    // uleft: value of the upper left cell; ulefts: maximum value of uleft and cells on the left. The arbitrary initial
    // values suppress warnings.
    int            uleft = 0, ulefts = 0, left, lefts;
    const gchar    *pit = pattern, *sit;
    enum CharClass prev = NON_WORD;
    for ( si = 0, sit = str; si < slen; si++, sit = g_utf8_next_char ( sit ) ) {
        enum CharClass cur = rofi_scorer_get_character_class ( g_utf8_get_char ( sit ) );
        score[si] = rofi_scorer_get_score_for ( prev, cur );
        prev      = cur;
        dp[si]    = MIN_SCORE;
    }
    for ( pi = 0; pi < plen; pi++, pit = g_utf8_next_char ( pit ) ) {
        gunichar pc = g_utf8_get_char ( pit ), sc;
        if ( g_unichar_isspace ( pc ) ) {
            pstart = TRUE;
            continue;
        }
        lefts = MIN_SCORE;
        for ( si = 0, sit = str; si < slen; si++, sit = g_utf8_next_char ( sit ) ) {
            left  = dp[si];
            lefts = MAX ( lefts + GAP_SCORE, left );
            sc    = g_utf8_get_char ( sit );
            if ( config.case_sensitive
                 ? pc == sc
                 : g_unichar_tolower ( pc ) == g_unichar_tolower ( sc ) ) {
                int t = score[si] * ( pstart ? PATTERN_START_MULTIPLIER : PATTERN_NON_START_MULTIPLIER );
                dp[si] = pfirst
                         ? LEADING_GAP_SCORE * si + t
                         : MAX ( uleft + CONSECUTIVE_SCORE, ulefts + t );
            }
            else {
                dp[si] = MIN_SCORE;
            }
            uleft  = left;
            ulefts = lefts;
        }
        pfirst = pstart = FALSE;
    }
    lefts = MIN_SCORE;
    for ( si = 0; si < slen; si++ ) {
        lefts = MAX ( lefts + GAP_SCORE, dp[si] );
    }
    g_free ( score );
    g_free ( dp );
    return -lefts;
}

/**
 * @param a    UTF-8 string to compare
 * @param b    UTF-8 string to compare
 * @param n    Maximum number of characters to compare
 *
 * Compares the `G_NORMALIZE_ALL_COMPOSE` forms of the two strings.
 *
 * @returns less than, equal to, or greater than zero if the first `n` characters (not bytes) of `a`
 *          are found, respectively, to be less than, to match, or be greater than the first `n`
 *          characters (not bytes) of `b`.
 */
int utf8_strncmp ( const char* a, const char* b, size_t n )
{
    char *na = g_utf8_normalize ( a, -1, G_NORMALIZE_ALL_COMPOSE );
    char *nb = g_utf8_normalize ( b, -1, G_NORMALIZE_ALL_COMPOSE );
    *g_utf8_offset_to_pointer ( na, n ) = '\0';
    *g_utf8_offset_to_pointer ( nb, n ) = '\0';
    int r = g_utf8_collate ( na, nb );
    g_free ( na );
    g_free ( nb );
    return r;
}

gboolean helper_execute ( const char *wd, char **args, const char *error_precmd, const char *error_cmd, RofiHelperExecuteContext *context )
{
    gboolean             retv   = TRUE;
    GError               *error = NULL;

    GSpawnChildSetupFunc child_setup = NULL;
    gpointer             user_data   = NULL;

    display_startup_notification ( context, &child_setup, &user_data );

    g_spawn_async ( wd, args, NULL, G_SPAWN_SEARCH_PATH, child_setup, user_data, NULL, &error );
    if ( error != NULL ) {
        char *msg = g_strdup_printf ( "Failed to execute: '%s%s'\nError: '%s'", error_precmd, error_cmd, error->message );
        rofi_view_error_dialog ( msg, FALSE  );
        g_free ( msg );
        // print error.
        g_error_free ( error );
        retv = FALSE;
    }

    // Free the args list.
    g_strfreev ( args );
    return retv;
}

gboolean helper_execute_command ( const char *wd, const char *cmd, gboolean run_in_term, RofiHelperExecuteContext *context )
{
    char **args = NULL;
    int  argc   = 0;

    if ( run_in_term ) {
        helper_parse_setup ( config.run_shell_command, &args, &argc, "{cmd}", cmd, (char *) 0 );
    }
    else {
        helper_parse_setup ( config.run_command, &args, &argc, "{cmd}", cmd, (char *) 0 );
    }

    if ( args == NULL ) {
        return FALSE;
    }

    if ( context != NULL ) {
        if ( context->name == NULL ) {
            context->name = args[0];
        }
        if ( context->binary == NULL ) {
            context->binary = args[0];
        }
        if ( context->description == NULL ) {
            gsize l            = strlen ( "Launching '' via rofi" ) + strlen ( cmd ) + 1;
            gchar *description = g_newa ( gchar, l );

            g_snprintf ( description, l, "Launching '%s' via rofi", cmd );
            context->description = description;
        }
        if ( context->command == NULL ) {
            context->command = cmd;
        }
    }

    return helper_execute ( wd, args, "", cmd, context );
}

char *helper_get_theme_path ( const char *file )
{
    char *filename = rofi_expand_path ( file );
    g_debug ( "Opening theme, testing: %s\n", filename );
    if ( g_file_test ( filename, G_FILE_TEST_EXISTS ) ) {
        return filename;
    }
    g_free ( filename );

    if ( g_str_has_suffix ( file, ".rasi" ) ) {
        filename = g_strdup ( file );
    }
    else {
        filename = g_strconcat ( file, ".rasi", NULL );
    }
    // Check config's themes directory.
    const char *cpath = g_get_user_config_dir ();
    if ( cpath ) {
        char *themep = g_build_filename ( cpath, "rofi", "themes", filename, NULL );
        g_debug ( "Opening theme, testing: %s\n", themep );
        if ( themep && g_file_test ( themep, G_FILE_TEST_EXISTS ) ) {
            g_free ( filename );
            return themep;
        }
        g_free ( themep );
    }
    // Check config directory.
    if ( cpath ) {
        char *themep = g_build_filename ( cpath, "rofi", filename, NULL );
        g_debug ( "Opening theme, testing: %s\n", themep );
        if ( g_file_test ( themep, G_FILE_TEST_EXISTS ) ) {
            g_free ( filename );
            return themep;
        }
        g_free ( themep );
    }
    const char * datadir = g_get_user_data_dir ();
    if ( datadir ) {
        char *theme_path = g_build_filename ( datadir, "rofi", "themes", filename, NULL );
        g_debug ( "Opening theme, testing: %s\n", theme_path );
        if ( theme_path ) {
            if ( g_file_test ( theme_path, G_FILE_TEST_EXISTS ) ) {
                g_free ( filename );
                return theme_path;
            }
            g_free ( theme_path );
        }
    }

    char *theme_path = g_build_filename ( THEME_DIR, filename, NULL );
    if ( theme_path ) {
        g_debug ( "Opening theme, testing: %s\n", theme_path );
        if ( g_file_test ( theme_path, G_FILE_TEST_EXISTS ) ) {
            g_free ( filename );
            return theme_path;
        }
        g_free ( theme_path );
    }
    return filename;
}

static gboolean parse_pair ( char  *input, rofi_range_pair  *item )
{
    // Skip leading blanks.
    while ( input != NULL && isblank ( *input ) ) {
        ++input;
    }

    if ( input == NULL ) {
        return FALSE;
    }

    const char *sep[]   = { "-", ":" };
    int        pythonic = ( strchr ( input, ':' ) || input[0] == '-' ) ? 1 : 0;
    int        index    = 0;

    for (  char *token = strsep ( &input, sep[pythonic] ); token != NULL; token = strsep ( &input, sep[pythonic] ) ) {
        if ( index == 0 ) {
            item->start = item->stop = (int) strtol ( token, NULL, 10 );
            index++;
            continue;
        }

        if ( token[0] == '\0' ) {
            item->stop = -1;
            continue;
        }

        item->stop = (int) strtol ( token, NULL, 10 );
        if ( pythonic ) {
            --item->stop;
        }
    }
    return TRUE;
}
void parse_ranges ( char *input, rofi_range_pair **list, unsigned int *length )
{
    char *endp;
    if ( input == NULL ) {
        return;
    }
    const char *const sep = ",";
    for ( char *token = strtok_r ( input, sep, &endp ); token != NULL; token = strtok_r ( NULL, sep, &endp ) ) {
        // Make space.
        *list = g_realloc ( ( *list ), ( ( *length ) + 1 ) * sizeof ( struct rofi_range_pair ) );
        // Parse a single pair.
        if ( parse_pair ( token, &( ( *list )[*length] ) ) ) {
            ( *length )++;
        }
    }
}
void rofi_output_formatted_line ( const char *format, const char *string, int selected_line, const char *filter )
{
    for ( int i = 0; format && format[i]; i++ ) {
        if ( format[i] == 'i' ) {
            fprintf ( stdout, "%d", selected_line );
        }
        else if ( format[i] == 'd' ) {
            fprintf ( stdout, "%d", ( selected_line + 1 ) );
        }
        else if ( format[i] == 's' ) {
            fputs ( string, stdout );
        }
        else if ( format[i] == 'p' ) {
            char *esc = NULL;
            pango_parse_markup ( string, -1, 0, NULL, &esc, NULL, NULL );
            if ( esc ) {
                fputs ( esc, stdout );
                g_free ( esc );
            }
            else {
                fputs ( "invalid string", stdout );
            }
        }
        else if ( format[i] == 'q' ) {
            char *quote = g_shell_quote ( string );
            fputs ( quote, stdout );
            g_free ( quote );
        }
        else if ( format[i] == 'f' ) {
            if ( filter ) {
                fputs ( filter, stdout );
            }
        }
        else if ( format[i] == 'F' ) {
            if ( filter ) {
                char *quote = g_shell_quote ( filter );
                fputs ( quote, stdout );
                g_free ( quote );
            }
        }
        else {
            fputc ( format[i], stdout );
        }
    }
    fputc ( '\n', stdout );
    fflush ( stdout );
}

static gboolean helper_eval_cb2 ( const GMatchInfo *info, GString *res, gpointer data )
{
    gchar *match;
    // Get the match
    int   num_match = g_match_info_get_match_count ( info );
    // Just {text} This is inside () 5.
    if ( num_match == 5 ) {
        match = g_match_info_fetch ( info, 4 );
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
    }
    // {} with [] guard around it.
    else if ( num_match == 4 ) {
        match = g_match_info_fetch ( info, 2 );
        if ( match != NULL ) {
            // Lookup the match, so we can replace it.
            gchar *r = g_hash_table_lookup ( (GHashTable *) data, match );
            if ( r != NULL ) {
                // Add (optional) prefix
                gchar *prefix = g_match_info_fetch ( info, 1 );
                g_string_append ( res, prefix );
                g_free ( prefix );
                // Append the replacement to the string.
                g_string_append ( res, r );
                // Add (optional) postfix
                gchar *post = g_match_info_fetch ( info, 3 );
                g_string_append ( res, post );
                g_free ( post );
            }
            // Free match.
            g_free ( match );
        }
    }
    // Else we have an invalid match.
    // Continue replacement.
    return FALSE;
}

char *helper_string_replace_if_exists ( char * string, ... )
{
    GHashTable *h;
    h = g_hash_table_new ( g_str_hash, g_str_equal );
    va_list    ap;
    va_start ( ap, string );
    // Add list from variable arguments.
    while ( 1 ) {
        char * key = va_arg ( ap, char * );
        if ( key == (char *) 0 ) {
            break;
        }
        char *value = va_arg ( ap, char * );
        g_hash_table_insert ( h, key, value );
    }
    char *retv = helper_string_replace_if_exists_v ( string, h );
    va_end ( ap );
    // Destroy key-value storage.
    g_hash_table_destroy ( h );
    return retv;
}
/**
 * @param string The string with elements to be replaced
 * @param h      Hash table with set of {key}, value that will be replaced, terminated by  a NULL
 *
 * Items {key} are replaced by the value if '{key}' is passed as key/value pair, otherwise removed from string.
 * If the {key} is in between []  all the text between [] are removed if {key}
 * is not found. Otherwise key is replaced and [ & ] removed.
 *
 * This allows for optional replacement, f.e.   '{ssh-client} [-t  {title}] -e
 * "{cmd}"' the '-t {title}' is only there if {title} is set.
 *
 * @returns a new string with the keys replaced.
 */
char *helper_string_replace_if_exists_v ( char * string, GHashTable *h )
{
    GError *error = NULL;
    char   *res   = NULL;

    // Replace hits within {-\w+}.
    GRegex *reg = g_regex_new ( "\\[(.*)({[-\\w]+})(.*)\\]|({[\\w-]+})", 0, 0, &error );
    if ( error == NULL ) {
        res = g_regex_replace_eval ( reg, string, -1, 0, 0, helper_eval_cb2, h, &error );
    }
    // Free regex.
    g_regex_unref ( reg );
    // Throw error if shell parsing fails.
    if ( error != NULL ) {
        char *msg = g_strdup_printf ( "Failed to parse: '%s'\nError: '%s'", string, error->message );
        rofi_view_error_dialog ( msg, FALSE );
        g_free ( msg );
        // print error.
        g_error_free ( error );
        g_free ( res );
        return NULL;
    }
    return res;
}
