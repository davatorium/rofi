#ifndef ROFI_DIALOG_SCRIPT_H
#define ROFI_DIALOG_SCRIPT_H

/**
 * @defgroup SCRIPTMode Script
 * @ingroup MODES
 *
 * @{
 */
/**
 * @param str   The input string to parse
 *
 * Parse an argument string into the right ScriptOptions data object.
 * This is off format: \<Name\>:\<Script\>
 *
 * @returns NULL when it fails, a newly allocated ScriptOptions when successful.
 */
Mode *script_switcher_parse_setup ( const char *str );
/*@}*/
#endif // ROFI_DIALOG_SCRIPT_H
