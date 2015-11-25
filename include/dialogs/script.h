#ifndef __SCRIPT_DIALOG_H__
#define __SCRIPT_DIALOG_H__

/**
 * @param str   The input string to parse
 *
 * Parse an argument string into the right ScriptOptions data object.
 * This is off format: <Name>:<Script>
 *
 * @returns NULL when it fails, a newly allocated ScriptOptions when successful.
 */
Mode *script_switcher_parse_setup ( const char *str );
#endif
