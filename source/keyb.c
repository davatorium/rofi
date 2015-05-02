#include "rofi.h"
#include <X11/keysym.h>
#include "x11-helper.h"
#include "xrmoptions.h"

ActionBindingEntry abe[NUM_ABE];
// Use this so we can ignore numlock mask.
// TODO: maybe use something smarter here..
extern unsigned int NumlockMask;

typedef struct _DefaultBinding
{
    KeyBindingAction id;
    char             *name;
    char             *keybinding;
} DefaultBinding;

DefaultBinding bindings[NUM_ABE] =
{
    {
        .id         = PASTE_PRIMARY,
        .name       = "primary-paste",
        .keybinding = "Control+Shift+v,Shift+Insert",
    },
    {
        .id         = PASTE_SECONDARY,
        .name       = "secondary-paste",
        .keybinding = "Control+v,Insert",
    },
    {
        .id         = CLEAR_LINE,
        .name       = "clear-line",
        .keybinding = "Control+u",
    },
    {
        .id         = MOVE_FRONT,
        .name       = "move-front",
        .keybinding = "Control+a",
    },
    {
        .id         = MOVE_END,
        .name       = "move-end",
        .keybinding = "Control+e",
    },
    {
        .id         = MOVE_WORD_BACK,
        .name       = "move-word-back",
        .keybinding = "Alt+b",
    },
    {
        .id         = MOVE_WORD_FORWARD,
        .name       = "move-word-forward",
        .keybinding = "Alt+f",
    },
    {
        .id         = MOVE_CHAR_BACK,
        .name       = "move-char-back",
        .keybinding = "Left,Control+b"
    },
    {
        .id         = MOVE_CHAR_FORWARD,
        .name       = "move-char-forward",
        .keybinding = "Right,Control+f"
    },
    {
        .id         = REMOVE_WORD_BACK,
        .name       = "remove-word-back",
        .keybinding = "Control+Alt+h",
    },
    {
        .id         = REMOVE_WORD_FORWARD,
        .name       = "remove-word-forward",
        .keybinding = "Control+Alt+d",
    },
    {
        .id         = REMOVE_CHAR_FORWARD,
        .name       = "remove-char-forward",
        .keybinding = "Delete,Control+d",
    },
    {
        .id         = REMOVE_CHAR_BACK,
        .name       = "remove-char-back",
        .keybinding = "BackSpace,Control+h",
    },
    {
        .id = ACCEPT_ENTRY,
        // TODO: split Shift return in separate state.
        .name       = "accept-entry",
        .keybinding = "Control+j,Control+m,Return,KP_Enter",
    },
    {
        .id         = ACCEPT_CUSTOM,
        .name       = "accept-custom",
        .keybinding = "Control+Return",
    },
    {
        .id         = ACCEPT_ENTRY_CONTINUE,
        .name       = "accept-entry-continue",
        .keybinding = "Shift+Return",
    },
    {
        .id         = MODE_NEXT,
        .name       = "mode-next",
        .keybinding = "Shift+Right,Control+Tab"
    },
    {
        .id         = MODE_PREVIOUS,
        .name       = "mode-previous",
        .keybinding = "Shift+Left,Control+Shift+Tab"
    },
    {
        .id         = TOGGLE_CASE_SENSITIVITY,
        .name       = "toggle-case-sensitivity",
        .keybinding = "grave,dead_grave"
    },
    {
        .id         = DELETE_ENTRY,
        .name       = "delete-entry",
        .keybinding = "Shift+Delete"
    }
};


void setup_abe ( void )
{
    for ( int iter = 0; iter < NUM_ABE; iter++ ) {
        int id = bindings[iter].id;
        // set pointer to name.
        abe[id].name         = bindings[iter].name;
        abe[id].keystr       = g_strdup ( bindings[iter].keybinding );
        abe[id].num_bindings = 0;
        abe[id].kb           = NULL;

        config_parser_add_option ( xrm_String,
                                   abe[id].name, (void * *) &( abe[id].keystr ) );
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
