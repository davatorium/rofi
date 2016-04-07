#include <config.h>
#include <string.h>
#include "rofi.h"
#include "x11-helper.h"
#include "xrmoptions.h"

typedef struct
{
    unsigned int modmask;
    xkb_keysym_t keysym;
} KeyBinding;

typedef struct
{
    const char *name;
    char       *keystr;
    int        num_bindings;
    KeyBinding *kb;
} ActionBindingEntry;

typedef struct
{
    KeyBindingAction id;
    char             *name;
    char             *keybinding;
} DefaultBinding;

ActionBindingEntry abe[NUM_ABE];

/**
 * LIST OF DEFAULT SETTINGS
 */
DefaultBinding bindings[NUM_ABE] =
{
    { .id = PASTE_PRIMARY,           .name = "kb-primary-paste",           .keybinding = "Control+Shift+v,Shift+Insert",        },
    { .id = PASTE_SECONDARY,         .name = "kb-secondary-paste",         .keybinding = "Control+v,Insert",                    },
    { .id = CLEAR_LINE,              .name = "kb-clear-line",              .keybinding = "Control+u",                           },
    { .id = MOVE_FRONT,              .name = "kb-move-front",              .keybinding = "Control+a",                           },
    { .id = MOVE_END,                .name = "kb-move-end",                .keybinding = "Control+e",                           },
    { .id = MOVE_WORD_BACK,          .name = "kb-move-word-back",          .keybinding = "Alt+b",                               },
    { .id = MOVE_WORD_FORWARD,       .name = "kb-move-word-forward",       .keybinding = "Alt+f",                               },
    { .id = MOVE_CHAR_BACK,          .name = "kb-move-char-back",          .keybinding = "Left,Control+b"                             },
    { .id = MOVE_CHAR_FORWARD,       .name = "kb-move-char-forward",       .keybinding = "Right,Control+f"                            },
    { .id = REMOVE_WORD_BACK,        .name = "kb-remove-word-back",        .keybinding = "Control+Alt+h",                       },
    { .id = REMOVE_WORD_FORWARD,     .name = "kb-remove-word-forward",     .keybinding = "Control+Alt+d",                       },
    { .id = REMOVE_CHAR_FORWARD,     .name = "kb-remove-char-forward",     .keybinding = "Delete,Control+d",                    },
    { .id = REMOVE_CHAR_BACK,        .name = "kb-remove-char-back",        .keybinding = "BackSpace,Control+h",                 },
    { .id = ACCEPT_ENTRY,            .name = "kb-accept-entry",            .keybinding = "Control+j,Control+m,Return,KP_Enter", },
    { .id = ACCEPT_CUSTOM,           .name = "kb-accept-custom",           .keybinding = "Control+Return,Shift+Return",         },
    { .id = MODE_NEXT,               .name = "kb-mode-next",               .keybinding = "Shift+Right,Control+Tab"                    },
    { .id = MODE_PREVIOUS,           .name = "kb-mode-previous",           .keybinding = "Shift+Left,Control+Shift+Tab"               },
    { .id = TOGGLE_CASE_SENSITIVITY, .name = "kb-toggle-case-sensitivity", .keybinding = "grave,dead_grave"                           },
    { .id = DELETE_ENTRY,            .name = "kb-delete-entry",            .keybinding = "Shift+Delete"                               },
    { .id = ROW_LEFT,                .name = "kb-row-left",                .keybinding = "Control+Page_Up"                            },
    { .id = ROW_RIGHT,               .name = "kb-row-right",               .keybinding = "Control+Page_Down"                          },
    { .id = ROW_UP,                  .name = "kb-row-up",                  .keybinding = "Up,Control+p,Shift+Tab,Shift+ISO_Left_Tab"  },
    { .id = ROW_DOWN,                .name = "kb-row-down",                .keybinding = "Down,Control+n"                             },
    { .id = ROW_TAB,                 .name = "kb-row-tab",                 .keybinding = "Tab"                                        },
    { .id = PAGE_PREV,               .name = "kb-page-prev",               .keybinding = "Page_Up"                                    },
    { .id = PAGE_NEXT,               .name = "kb-page-next",               .keybinding = "Page_Down"                                  },
    { .id = ROW_FIRST,               .name = "kb-row-first",               .keybinding = "Home,KP_Home"                               },
    { .id = ROW_LAST,                .name = "kb-row-last",                .keybinding = "End,KP_End"                                 },
    { .id = ROW_SELECT,              .name = "kb-row-select",              .keybinding = "Control+space"                              },
    { .id = CANCEL,                  .name = "kb-cancel",                  .keybinding = "Escape,Control+bracketleft"                 },
    { .id = CUSTOM_1,                .name = "kb-custom-1",                .keybinding = "Alt+1"                                      },
    { .id = CUSTOM_2,                .name = "kb-custom-2",                .keybinding = "Alt+2"                                      },
    { .id = CUSTOM_3,                .name = "kb-custom-3",                .keybinding = "Alt+3"                                      },
    { .id = CUSTOM_4,                .name = "kb-custom-4",                .keybinding = "Alt+4"                                      },
    { .id = CUSTOM_5,                .name = "kb-custom-5",                .keybinding = "Alt+5"                                      },
    { .id = CUSTOM_6,                .name = "kb-custom-6",                .keybinding = "Alt+6"                                      },
    { .id = CUSTOM_7,                .name = "kb-custom-7",                .keybinding = "Alt+7"                                      },
    { .id = CUSTOM_8,                .name = "kb-custom-8",                .keybinding = "Alt+8"                                      },
    { .id = CUSTOM_9,                .name = "kb-custom-9",                .keybinding = "Alt+9"                                      },
    { .id = CUSTOM_10,               .name = "kb-custom-10",               .keybinding = "Alt+0"                                      },
    { .id = CUSTOM_11,               .name = "kb-custom-11",               .keybinding = "Alt+Shift+1"                                },
    { .id = CUSTOM_12,               .name = "kb-custom-12",               .keybinding = "Alt+Shift+2"                                },
    { .id = CUSTOM_13,               .name = "kb-custom-13",               .keybinding = "Alt+Shift+3"                                },
    { .id = CUSTOM_14,               .name = "kb-custom-14",               .keybinding = "Alt+Shift+4"                                },
    { .id = CUSTOM_15,               .name = "kb-custom-15",               .keybinding = "Alt+Shift+5"                                },
    { .id = CUSTOM_16,               .name = "kb-custom-16",               .keybinding = "Alt+Shift+6"                                },
    { .id = CUSTOM_18,               .name = "kb-custom-18",               .keybinding = "Alt+Shift+8"                                },
    { .id = CUSTOM_17,               .name = "kb-custom-17",               .keybinding = "Alt+Shift+7"                                },
    { .id = CUSTOM_19,               .name = "kb-custom-19",               .keybinding = "Alt+Shift+9"                                },
    { .id = SCREENSHOT,              .name = "kb-screenshot",              .keybinding = "Alt+Shift+S"                                },
    { .id = TOGGLE_SORT,             .name = "kb-toggle-sort",             .keybinding = "Alt+grave"                                  },
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

        config_parser_add_option ( xrm_String, abe[id].name, (void * *) &( abe[id].keystr ), "Keybinding" );
    }
}

gboolean parse_keys_abe ( void )
{
    for ( int iter = 0; iter < NUM_ABE; iter++ ) {
        char *keystr = g_strdup ( abe[iter].keystr );
        char *sp     = NULL;

        g_free ( abe[iter].kb );
        abe[iter].kb           = NULL;
        abe[iter].num_bindings = 0;

        // Iter over bindings.
        const char *const sep = ",";
        for ( char *entry = strtok_r ( keystr, sep, &sp ); entry != NULL; entry = strtok_r ( NULL, sep, &sp ) ) {
            abe[iter].kb = g_realloc ( abe[iter].kb, ( abe[iter].num_bindings + 1 ) * sizeof ( KeyBinding ) );
            KeyBinding *kb = &( abe[iter].kb[abe[iter].num_bindings] );
            if ( !x11_parse_key ( entry, &( kb->modmask ), &( kb->keysym ) ) ) {
                g_free ( keystr );
                return FALSE;
            }
            abe[iter].num_bindings++;
        }

        g_free ( keystr );
    }
    return TRUE;
}

void cleanup_abe ( void )
{
    for ( int iter = 0; iter < NUM_ABE; iter++ ) {
        g_free ( abe[iter].kb );
        abe[iter].kb           = NULL;
        abe[iter].num_bindings = 0;
    }
}

static gboolean abe_test_action ( KeyBindingAction action, unsigned int mask, xkb_keysym_t key )
{
    ActionBindingEntry *akb = &( abe[action] );

    for ( int iter = 0; iter < akb->num_bindings; iter++ ) {
        const KeyBinding * const kb = &( akb->kb[iter] );
        if ( ( kb->keysym == key ) && ( kb->modmask == mask ) ) {
            return TRUE;
        }
    }

    return FALSE;
}

KeyBindingAction abe_find_action ( unsigned int mask, xkb_keysym_t key )
{
    KeyBindingAction action;

    for ( action = 0 ; action < NUM_ABE ; ++action ) {
        if ( abe_test_action ( action, mask, key ) ) {
            break;
        }
    }

    return action;
}
