#ifndef ROFI_HELPER_H
#define ROFI_HELPER_H
#include "rofi.h"

#include <pango/pango.h>
#include <theme.h>
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
int token_match ( GRegex * const *tokens, const char *input );
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
char * rofi_force_utf8 ( gchar *data, ssize_t length );

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
 * @param th The ThemeHighlight
 * @param tokens Array of regexes used for matching
 * @param input The input string to find the matches on
 * @param retv The Attribute list to update with matches
 *
 * Creates a set of pango attributes highlighting the matches found in the input string.
 *
 * @returns the updated retv list.
 */
PangoAttrList *token_match_get_pango_attr ( ThemeHighlight th, GRegex **tokens, const char *input, PangoAttrList *retv );

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
#endif // ROFI_HELPER_H
