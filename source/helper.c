/**
 * rofi
 *
 * MIT/X11 License
 * Copyright (c) 2012 Sean Pringle <sean.pringle@gmail.com>
 * Modified 2013-2015 Qball Cow <qball@gmpclient.org>
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
#include "helper.h"
#include "rofi.h"

static int  stored_argc   = 0;
static char **stored_argv = NULL;

void cmd_set_arguments ( int argc, char **argv )
{
    stored_argc = argc;
    stored_argv = argv;
}

/**
 *  `fgets` implementation with custom separator.
 */
char* fgets_s ( char* s, unsigned int n, FILE *iop, char sep )
{
    // Map these to registers.
    register int c = EOF;
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
    return ( c == EOF && cs == s ) ? NULL : cs;
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
        error_dialog ( msg, FALSE );
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
    for ( token = strtok_r ( str, " ", &saveptr ); token != NULL; token = strtok_r ( NULL, " ", &saveptr ) ) {
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
    char retv = -1;
    int  len  = strlen ( arg );
    // If the length is 1, it is not escaped.
    if ( len == 1 ) {
        retv = arg[0];
    }
    // If the length is 2 and the first character is '\', we unescape it.
    else if ( len == 2 && arg[0] == '\\' ) {
        // New line
        if ( arg[1] == 'n' ) {
            retv = '\n';
        }
        // Bell
        else if ( arg[1] == 'a' ) {
            retv = '\a';
        }
        // Backspace
        else if ( arg[1] == 'b' ) {
            retv = '\b';
        }
        // Tab
        else if ( arg[1] == 't' ) {
            retv = '\t';
        }
        // Vertical tab
        else if ( arg[1] == 'v' ) {
            retv = '\v';
        }
        // Form feed
        else if ( arg[1] == 'f' ) {
            retv = '\f';
        }
        // Carriage return
        else if ( arg[1] == 'r' ) {
            retv = '\r';
        }
        // Forward slash
        else if ( arg[1] == '\\' ) {
            retv = '\\';
        }
        else if ( arg[1] == '0' ) {
            retv = '\0';
        }
    }
    else if ( len > 2 && arg[0] == '\\' && arg[1] == 'x' ) {
        retv = (char) strtol ( &arg[2], NULL, 16 );
    }
    if ( retv < 0 ) {
        fprintf ( stderr, "Failed to parse character string: \"%s\"\n", arg );
        // for now default to newline.
        retv = '\n';
    }
    return retv;
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

/**
 * Shared 'token_match' function.
 * Matches tokenized.
 */
static int fuzzy_token_match ( char **tokens, const char *input, __attribute__( (unused) ) int not_ascii,  int case_sensitive )
{
    int  match  = 1;
    char *compk = token_collate_key ( input, case_sensitive );
    // Do a tokenized match.
    // TODO: this doesn't work for unicode input, because it may split a codepoint which is over two bytes.
    // TODO this does not use the non-ascii speed-up either.
    if ( tokens ) {
        for ( int j = 0; match && tokens[j]; j++ ) {
            char *t        = compk;
            int  token_len = strlen ( tokens[j] );
            for ( int id = 0; match && t != NULL && id < token_len; id++ ) {
                match = ( ( t = strchr ( t, tokens[j][id] ) ) != NULL );
                // next should match the next character.
                if ( t != NULL ) {
                    t++;
                }
            }
        }
    }
    g_free ( compk );
    return match;
}
static int normal_token_match ( char **tokens, const char *input, int not_ascii, int case_sensitive )
{
    int  match  = 1;
    char *compk = not_ascii ? token_collate_key ( input, case_sensitive ) : (char *) input;

    // Do a tokenized match.

    if ( tokens ) {
      char *(*comparison)(const char *, const char *);
      comparison = (case_sensitive || not_ascii) ? strstr : strcasestr;
      for ( int j = 0; match && tokens[j]; j++ ) {
        match = (comparison( compk, tokens[j] ) != NULL );
      }
    }

    if (not_ascii) g_free ( compk );

    return match;
}
int token_match ( char **tokens, const char *input, int not_ascii, int case_sensitive,
                  __attribute__( ( unused ) ) unsigned int index,
                  __attribute__( ( unused ) ) Switcher *data )
{
    if ( config.fuzzy ) {
        return fuzzy_token_match ( tokens, input, not_ascii, case_sensitive );
    }
    return normal_token_match ( tokens, input, not_ascii, case_sensitive );
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
        error_dialog ( msg, FALSE );
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
void config_sanity_check (  )
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
        show_error_message ( "<b>config.menu_width</b>=0 is invalid. You cannot have a window with no width.", TRUE );
        config.menu_columns = 50;
        found_error         = TRUE;
    }
    if ( !( config.location >= WL_CENTER && config.location <= WL_WEST ) ) {
        g_string_append_printf ( msg, "\t<b>config.location</b>=%d is invalid. Value should be between %d and %d.\n",
                                 config.location, WL_CENTER, WL_WEST );
        config.location = WL_CENTER;
        found_error     = 1;
    }
    if ( 0 ) {
        if ( !( config.line_margin <= 50 ) ) {
            g_string_append_printf ( msg, "\t<b>config.line_margin</b>=%d is invalid. Value should be between %d and %d.\n",
                                     config.line_margin, 0, 50 );
            config.line_margin = 2;
            found_error        = 1;
        }
    }
    if ( found_error ) {
        g_string_append ( msg, "Please update your configuration." );
        show_error_message ( msg->str, TRUE );
    }

    g_string_free ( msg, TRUE );
    // If alternative row is not set, copy the normal background color.
    if ( config.menu_bg_alt == NULL ) {
        config.menu_bg_alt = config.menu_bg;
    }
}

int is_not_ascii ( const char * str )
{
   while (*str > 0) {
     str++;
   }
   if (*str) return 1;
   return 0;
}
