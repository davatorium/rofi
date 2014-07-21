#ifndef __SCRIPT_DIALOG_H__
#define __SCRIPT_DIALOG_H__

/**
 * Structure holds the arguments for the script_switcher.
 */
typedef struct
{
    // Prompt to display.
    char *name;
    // The script
    char *script_path;
} ScriptOptions;

SwitcherMode script_switcher_dialog ( char **input, void *data );

/**
 * Parse an argument string into the right ScriptOptions data object.
 * This is off format: <Name>:<Script>
 * Return NULL when it fails.
 */
ScriptOptions *script_switcher_parse_setup ( const char *str );

/**
 * Free the ScriptOptions block.
 */
void script_switcher_free_options ( ScriptOptions *sw );
#endif
