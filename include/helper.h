#ifndef __HELPER_H__
#define __HELPER_H__

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
 * Implementation of fgets with custom separator.
 */
char* fgets_s ( char* s, int n, FILE *iop, char sep );

/**
 * @param input The input string.
 *
 * Tokenize the string on spaces.
 *
 * @returns a newly allocated 2 dimensional array of strings.
 */
char **tokenize ( const char *input );

/**
 * @param argc Number of arguments.
 * @param argv 2 dimensional array of arguments.
 * @param key The key to search for
 * @param val Pointer to the string to set to the key value (if found)
 *
 * Parse command line argument 'key' to character.
 * This one supports character escaping.
 *
 * @returns TRUE if key was found and val was set.
 */
int find_arg_char ( const int argc, char * const argv[], const char * const key, char *val );

/**
 * @param argc Number of arguments.
 * @param argv 2 dimensional array of arguments.
 * @param key The key to search for
 * @param val Pointer to the string to set to the key value (if found)
 *
 * Parse command line argument 'key' to unsigned int.
 *
 * @returns TRUE if key was found and val was set.
 */
int find_arg_uint ( const int argc, char * const argv[], const char * const key, unsigned int *val );

/**
 * @param argc Number of arguments.
 * @param argv 2 dimensional array of arguments.
 * @param key The key to search for
 * @param val Pointer to the string to set to the key value (if found)
 *
 * Parse command line argument 'key' to int.
 *
 * @returns TRUE if key was found and val was set.
 */
int find_arg_int ( const int argc, char * const argv[], const char * const key, int *val );


/**
 * @param argc Number of arguments.
 * @param argv 2 dimensional array of arguments.
 * @param key The key to search for
 * @param val Pointer to the string to set to the key value (if found)
 *
 * Parse command line argument 'key' to string.
 *
 * @returns TRUE if key was found and val was set.
 */
int find_arg_str ( const int argc, char * const argv[], const char * const key, char** val );

/**
 * @param argc Number of arguments.
 * @param argv 2 dimensional array of arguments.
 * @param key The key to search for
 *
 * Check if key is passed as argument.
 *
 * @returns return position of string or -1 if not found.
 */
int find_arg ( const int argc, char * const argv[], const char * const key );

/**
 * @params tokens
 * @param tokens  List of (input) tokens to match.
 * @param input   The entry to match against.
 * @param index   The current selected index.
 * @param data    User data.
 *
 * Tokenized match, match tokens to line input.
 *
 * @returns 1 when matches, 0 otherwise
 */
int token_match ( char **tokens, const char *input,
                  __attribute__( ( unused ) ) int index,
                  __attribute__( ( unused ) ) void *data );

#endif // __HELPER_H__
