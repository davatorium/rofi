#ifndef ROFI_HISTORY_H
#define ROFI_HISTORY_H

/**
 * @defgroup HISTORY History
 * @ingroup HELPERS
 *
 * Implements a very simple history module that can be used by a #Mode.
 *
 * This uses the following options from the #config object:
 * * #_Settings::disable_history
 *
 * @{
 */

/**
 * @param filename The filename of the history cache.
 * @param entry    The entry to add/increment
 *
 * Sets the entry in the history, if it exists its use-count is incremented.
 *
 */
void history_set ( const char *filename, const char *entry ) __attribute__( ( nonnull ) );

/**
 * @param filename The filename of the history cache.
 * @param entry    The entry to remove
 *
 * Removes the entry from the history.
 */
void history_remove ( const char *filename, const char *entry ) __attribute__( ( nonnull ) );

/**
 * @param filename The filename of the history cache.
 * @param length   The length of the returned list.
 *
 * Gets the entries in the list (in order of usage)
 * @returns a list of entries length long. (and NULL terminated).
 */
char ** history_get_list ( const char *filename, unsigned int * length ) __attribute__( ( nonnull ) );

/*@}*/
#endif // ROFI_HISTORY_H
