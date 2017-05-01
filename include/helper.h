/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2017 Qball Cow <qball@gmpclient.org>
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
 * @returns a newly allocated array of regex objest
 */
GRegex **tokenize ( const char *input, int case_sensitive );

/**
 * @param tokens Array of regex objects
 *
 * Frees the array of regex expressions.
 */
void tokenize_free ( GRegex ** tokens );

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
int helper_token_match ( GRegex * const *tokens, const char *input );
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
 * @param pattern   The user input to match against.
 * @param plen      Pattern length.
 * @param str       The input to match against pattern.
 * @param slen      Lenght of str.
 *
 * FZF like fuzzy sorting algorithm.
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
int utf8_strncmp ( const char *a, const char* b, size_t n ) __attribute__((nonnull(1,2)));

/**
 * @param wd The work directory (optional)
 * @param cmd The cmd to execute
 * @param run_in_term Indicate if command should be run in a terminal
 *
 * Execute command.
 *
 * @returns FALSE On failure, TRUE on success
 */
int helper_execute_command ( const char *wd, const char *cmd, int run_in_term );
#endif // ROFI_HELPER_H
