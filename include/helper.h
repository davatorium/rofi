/*
 * rofi
 *
 * MIT/X11 License
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

#ifndef ROFI_HELPER_H
#define ROFI_HELPER_H
#include <cairo.h>
#include "rofi-types.h"
G_BEGIN_DECLS

/**
 * @defgroup HELPERS Helpers
 */
/**
 * @defgroup HELPER Helper
 * @ingroup HELPERS
 *
 * @{
 */

/**
 * @param string The input string.
 * @param output Pointer to 2 dimensional array with parsed string.
 * @param length Length of 2 dimensional array.
 * @param ...    Key, value parse. Replaces the string Key with value.
 *
 * Parses a string into arguments. While replacing keys with values.
 *
 * @returns TRUE when successful, FALSE when failed.
 */
int helper_parse_setup ( char * string, char ***output, int *length, ... );

/**
 * @param input The input string.
 * @param case_sensitive Whether case is significant.
 *
 * Tokenize the string on spaces.
 *
 * @returns a newly allocated array of matching objects
 */
rofi_int_matcher **helper_tokenize ( const char *input, int case_sensitive );

/**
 * @param tokens Array of regex objects
 *
 * Frees the array of matching objects.
 */
void helper_tokenize_free ( rofi_int_matcher ** tokens );

/**
 * @param key The key to search for
 * @param val Pointer to the string to set to the key value (if found)
 *
 * Parse command line argument 'key' to character.
 * This one supports character escaping.
 *
 * @returns TRUE if key was found and val was set.
 */
int find_arg_char ( const char * const key, char *val );

/**
 * @param key The key to search for
 * @param val Pointer to the string to set to the key value (if found)
 *
 * Parse command line argument 'key' to unsigned int.
 *
 * @returns TRUE if key was found and val was set.
 */
int find_arg_uint ( const char * const key, unsigned int *val );

/**
 * @param key The key to search for
 * @param val Pointer to the string to set to the key value (if found)
 *
 * Parse command line argument 'key' to int.
 *
 * @returns TRUE if key was found and val was set.
 */
int find_arg_int ( const char * const key, int *val );

/**
 * @param key The key to search for
 * @param val Pointer to the string to set to the key value (if found)
 *
 * Parse command line argument 'key' to string.
 *
 * @returns TRUE if key was found and val was set.
 */
int find_arg_str ( const char * const key, char** val );

/**
 * @param key The key to search for
 *
 * Parse all command line options 'key' to string vector.
 *
 * @returns str vector. user should free array.
 */
const char ** find_arg_strv ( const char *const key );
/**
 * @param key The key to search for
 *
 * Check if key is passed as argument.
 *
 * @returns return position of string or -1 if not found.
 */
int find_arg ( const char * const key );

/**
 * @param tokens  List of (input) tokens to match.
 * @param input   The entry to match against.
 *
 * Tokenized match, match tokens to line input.
 *
 * @returns TRUE when matches, FALSE otherwise
 */
int helper_token_match ( rofi_int_matcher * const *tokens, const char *input );
/**
 * @param cmd The command to execute.
 *
 * Execute cmd using config.run_command and outputs the result (stdout) to the opened file
 * descriptor.
 *
 * @returns a valid file descriptor on success, or -1 on failure.
 */
int execute_generator ( const char * cmd ) __attribute__( ( nonnull ) );

/**
 * @param pidfile The pidfile to create.
 *
 * returns file descriptor (or -1 when failed)
 */
int create_pid_file ( const char *pidfile );

/**
 * Remove pid file
 */
void remove_pid_file ( int fd );

/**
 * Do some input validation, especially the first few could break things.
 * It is good to catch them beforehand.
 *
 * This functions exits the program with 1 when it finds an invalid configuration.
 */
int config_sanity_check ( void );

/**
 * @param arg string to parse.
 *
 * Parses a string into an character.
 *
 * @returns character.
 */
char helper_parse_char ( const char *arg );

/**
 * @param argc number of arguments.
 * @param argv Array of arguments.
 *
 * Set the application arguments.
 */
void cmd_set_arguments ( int argc, char **argv );

/**
 * @param input The path to expand
 *
 * Expand path, both `~` and `~<user>`
 *
 * @returns path
 */
char *rofi_expand_path ( const char *input );

/**
 * @param needle The string to find match weight off
 * @param needlelen The length of the needle
 * @param haystack The string to match against
 * @param haystacklen The length of the haystack
 *
 * UTF-8 aware levenshtein distance calculation
 *
 * @returns the levenshtein distance between needle and haystack
 */
unsigned int levenshtein ( const char *needle, const glong needlelen, const char *haystack, const glong haystacklen );

/**
 * @param data the unvalidated character array holding possible UTF-8 data
 * @param length the length of the data array
 *
 * Convert string to valid utf-8, replacing invalid parts with replacement character.
 *
 * @returns the converted UTF-8 string
 */
char * rofi_force_utf8 ( const gchar *data, ssize_t length );

/**
 * @param input the char array holding latin text
 * @param length the length of the data array
 *
 * Converts latin to UTF-8.
 *
 * @return the UTF-8 representation of data
 */
char * rofi_latin_to_utf8_strdup ( const char *input, gssize length );

/**
 * @param text the string to escape
 *
 * Escape XML markup from the string. text is freed.
 *
 * @return the escaped string
 */
gchar *rofi_escape_markup ( gchar *text );

/**
 * @param pattern   The user input to match against.
 * @param plen      Pattern length.
 * @param str       The input to match against pattern.
 * @param slen      Length of str.
 *
 *  rofi_scorer_fuzzy_evaluate implements a global sequence alignment algorithm to find the maximum accumulated score by
 *  aligning `pattern` to `str`. It applies when `pattern` is a subsequence of `str`.
 *
 *  Scoring criteria
 *  - Prefer matches at the start of a word, or the start of subwords in CamelCase/camelCase/camel123 words. See WORD_START_SCORE/CAMEL_SCORE.
 *  - Non-word characters matter. See NON_WORD_SCORE.
 *  - The first characters of words of `pattern` receive bonus because they usually have more significance than the rest.
 *  See PATTERN_START_MULTIPLIER/PATTERN_NON_START_MULTIPLIER.
 *  - Superfluous characters in `str` will reduce the score (gap penalty). See GAP_SCORE.
 *  - Prefer early occurrence of the first character. See LEADING_GAP_SCORE/GAP_SCORE.
 *
 *  The recurrence of the dynamic programming:
 *  dp[i][j]: maximum accumulated score by aligning pattern[0..i] to str[0..j]
 *  dp[0][j] = leading_gap_penalty(0, j) + score[j]
 *  dp[i][j] = max(dp[i-1][j-1] + CONSECUTIVE_SCORE, max(dp[i-1][k] + gap_penalty(k+1, j) + score[j] : k < j))
 *
 *  The first dimension can be suppressed since we do not need a matching scheme, which reduces the space complexity from
 *  O(N*M) to O(M)
 *
 * @returns the sorting weight.
 */
int rofi_scorer_fuzzy_evaluate ( const char *pattern, glong plen, const char *str, glong slen );
/*@}*/

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
int utf8_strncmp ( const char *a, const char* b, size_t n ) __attribute__( ( nonnull ( 1, 2 ) ) );

/**
 * The startup notification context of the application to launch
 */
typedef struct
{
    /** The name of the application */
    const gchar *name;
    /** The binary name of the application */
    const gchar *binary;
    /** The description of the launch */
    const gchar *description;
    /** The icon name of the application */
    const gchar *icon;
    /** The application id (desktop file with the .desktop suffix) */
    const gchar *app_id;
    /** The window manager class of the application */
    const gchar *wmclass;
    /** The command we run */
    const gchar *command;
} RofiHelperExecuteContext;

/**
 * @param wd   The working directory.
 * @param args The arguments of the command to exec.
 * @param error_precmd Prefix to error message command.
 * @param error_cmd Error message command
 * @param context The startup notification context, if any
 *
 * Executes the command
 *
 * @returns TRUE when successful, FALSE when failed.
 */
gboolean helper_execute ( const char *wd, char **args, const char *error_precmd, const char *error_cmd, RofiHelperExecuteContext *context );

/**
 * @param wd The work directory (optional)
 * @param cmd The cmd to execute
 * @param run_in_term Indicate if command should be run in a terminal
 * @param context The startup notification context, if any
 *
 * Execute command.
 * If needed members of context are NULL, they will be filled.
 *
 * @returns FALSE On failure, TRUE on success
 */
gboolean helper_execute_command ( const char *wd, const char *cmd, gboolean run_in_term, RofiHelperExecuteContext *context );

/**
 * @param file The file path
 * @param height The wanted height
 * Gets a surface from an svg path
 *
 * @returns a cairo surface from an svg path
 */
cairo_surface_t *cairo_image_surface_create_from_svg ( const gchar* file, int height );

/**
 * Ranges.
 */

/**
 * @param input String to parse
 * @param list  List of ranges
 * @param length Length of list.
 *
 * ranges
 */
void parse_ranges ( char *input, rofi_range_pair **list, unsigned int *length );

/**
 * @param format The format string used. See below for possible syntax.
 * @param string The selected entry.
 * @param selected_line The selected line index.
 * @param filter The entered filter.
 *
 * Function that outputs the selected line in the user-specified format.
 * Currently the following formats are supported:
 *   * i: Print the index (0-(N-1))
 *   * d: Print the index (1-N)
 *   * s: Print input string.
 *   * q: Print quoted input string.
 *   * f: Print the entered filter.
 *   * F: Print the entered filter, quoted
 *
 * This functions outputs the formatted string to stdout, appends a newline (\n) character and
 * calls flush on the file descriptor.
 */
void rofi_output_formatted_line ( const char *format, const char *string, int selected_line, const char *filter );

/**
 * @param string The string with elements to be replaced
 * @param ...    Set of {key}, value that will be replaced, terminated by  a NULL
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
char *helper_string_replace_if_exists ( char * string, ... );
G_END_DECLS

/**@} */
#endif // ROFI_HELPER_H
