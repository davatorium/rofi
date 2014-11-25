#ifndef __SCRIPT_DIALOG_H__
#define __SCRIPT_DIALOG_H__

/**
 * Structure holds the arguments for the script_switcher.
 */
typedef struct
{
    /** Prompt to display. */
    char *name;
    /** The script */
    char *script_path;
} ScriptOptions;


/**
 * @param input Pointer to the user-input string.
 * @param data  Custom data pointer for callback.
 *
 * Run the script dialog
 *
 * @returns SwitcherMode selected by user.
 */
SwitcherMode script_switcher_dialog ( char **input, void *data );

/**
 * @param str   The input string to parse
 *
 * Parse an argument string into the right ScriptOptions data object.
 * This is off format: <Name>:<Script>
 *
 * @returns NULL when it fails, a newly allocated ScriptOptions when successful.
 */
ScriptOptions *script_switcher_parse_setup ( const char *str );

/**
 * @param sw Handle to the ScriptOption
 *
 * Free the ScriptOptions block.
 */
void script_switcher_free_options ( ScriptOptions *sw );
#endif
