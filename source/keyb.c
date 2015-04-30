#include "rofi.h"
#include <X11/keysym.h>
#include "x11-helper.h"
#include "xrmoptions.h"

ActionBindingEntry abe[NUM_ABE];
// Use this so we can ignore numlock mask.
// TODO: maybe use something smarter here..
extern unsigned int NumlockMask;
const char          *KeyBindingActionName[NUM_ABE] =
{
    // PASTE_PRIMARY
    "primary-paste",
    // PASTE_SECONDARY
    "secondary-paste",
    // CLEAR_LINE
    "clear-line",
    // MOVE_FRONT
    "move-front",
    // MOVE_END
    "move-end",
    // REMOVE_WORD_BACK
    "remove-word-back",
    // REMOVE_WORD_FORWARD
    "remove-word-forward",
    // REMOVE_CHAR_FORWARD
    "remove-char-forward",
    // REMOVE_CHAR_BACK
    "remove-char-back",
    // MOVE_WORD_BACK
    "move-word-back",
    // MOVE_WORD_FORWARD
    "move-word-forward",
};

char *KeyBindingActionDefault[NUM_ABE] =
{
    // PASTE_PRIMARY
    "Control+Shift+v,Shift+Insert",
    // PASTE_SECONDARY
    "Control+v,Insert",
    // CLEAR_LINE
    "Control+u",
    // MOVE_FRONT
    "Control+a",
    // MOVE_END
    "Control+e",
    // REMOVE_WORD_BACK
    "Control+Alt+h",
    // REMOVE_WORD_FORWARD
    "Control+Alt+d",
    // REMOVE_CHAR_FORWARD
    "Delete,Control+d",
    // REMOVE_CHAR_BACK
    "BackSpace,Control+h",
    // MOVE_WORD_BACK
    "Alt+b",
    // MOVE_WORD_FORWARD
    "Alt+f",
};

void setup_abe ( void )
{
    for ( int iter = 0; iter < NUM_ABE; iter++ ) {
        // set pointer to name.
        abe[iter].name         = KeyBindingActionName[iter];
        abe[iter].keystr       = g_strdup ( KeyBindingActionDefault[iter] );
        abe[iter].num_bindings = 0;
        abe[iter].kb           = NULL;

        config_parser_add_option ( xrm_String,
                                   abe[iter].name, (void **)&( abe[iter].keystr ) );
    }
}

void parse_keys_abe ( void )
{
    for ( int iter = 0; iter < NUM_ABE; iter++ ) {
        char *keystr = g_strdup ( abe[iter].keystr );
        char *sp     = NULL;

        g_free ( abe[iter].kb );
        abe[iter].num_bindings = 0;

        // Iter over bindings.
        for ( char *entry = strtok_r ( keystr, ",", &sp ); entry != NULL; entry = strtok_r ( NULL, ",", &sp ) ) {
            abe[iter].kb = g_realloc ( abe[iter].kb, ( abe[iter].num_bindings + 1 ) * sizeof ( KeyBinding ) );
            KeyBinding *kb = &( abe[iter].kb[abe[iter].num_bindings] );
            x11_parse_key ( entry, &( kb->modmask ), &( kb->keysym ) );
            abe[iter].num_bindings++;
        }

        g_free ( keystr );
    }
}

void cleanup_abe ( void )
{
    for ( int iter = 0; iter < NUM_ABE; iter++ ) {
        g_free ( abe[iter].kb );
        abe[iter].kb           = NULL;
        abe[iter].num_bindings = 0;
    }
}

int abe_test_action ( KeyBindingAction action, unsigned int mask, KeySym key )
{
    ActionBindingEntry *akb = &( abe[action] );

    for ( int iter = 0; iter < akb->num_bindings; iter++ ) {
        const KeyBinding * const kb = &( akb->kb[iter] );
        if ( kb->keysym == key ) {
            if ( ( mask & ~NumlockMask ) == kb->modmask ) {
                return TRUE;
            }
        }
    }

    return FALSE;
}
