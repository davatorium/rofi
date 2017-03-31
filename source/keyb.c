#include <config.h>
#include <string.h>
#include "rofi.h"
#include "x11-helper.h"
#include "xrmoptions.h"

typedef struct
{
    unsigned int modmask;
    xkb_keysym_t keysym;
    gboolean     release;
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
    char             *comment;
} DefaultBinding;

/**
 * Data structure holding all the action keybinding.
 */
ActionBindingEntry abe[NUM_ABE];

/**
 * LIST OF DEFAULT SETTINGS
 */
DefaultBinding bindings[NUM_ABE] =
{
    { .id = PASTE_PRIMARY,           .name = "kb-primary-paste",           .keybinding = "Control+V,Shift+Insert",               .comment = "Paste primary selection"                                         },
    { .id = PASTE_SECONDARY,         .name = "kb-secondary-paste",         .keybinding = "Control+v,Insert",                     .comment = "Paste clipboard"                                                 },
    { .id = CLEAR_LINE,              .name = "kb-clear-line",              .keybinding = "Control+w",                            .comment = "Clear input line"                                                },
    { .id = MOVE_FRONT,              .name = "kb-move-front",              .keybinding = "Control+a",                            .comment = "Beginning of line"                                               },
    { .id = MOVE_END,                .name = "kb-move-end",                .keybinding = "Control+e",                            .comment = "End of line"                                                     },
    { .id = MOVE_WORD_BACK,          .name = "kb-move-word-back",          .keybinding = "Alt+b",                                .comment = "Move back one word"                                              },
    { .id = MOVE_WORD_FORWARD,       .name = "kb-move-word-forward",       .keybinding = "Alt+f",                                .comment = "Move forward one word"                                           },
    { .id = MOVE_CHAR_BACK,          .name = "kb-move-char-back",          .keybinding = "Left,Control+b",                       .comment = "Move back one char"                                              },
    { .id = MOVE_CHAR_FORWARD,       .name = "kb-move-char-forward",       .keybinding = "Right,Control+f",                      .comment = "Move forward one char"                                           },
    { .id = REMOVE_WORD_BACK,        .name = "kb-remove-word-back",        .keybinding = "Control+Alt+h,Control+BackSpace",      .comment = "Delete previous word"                                            },
    { .id = REMOVE_WORD_FORWARD,     .name = "kb-remove-word-forward",     .keybinding = "Control+Alt+d",                        .comment = "Delete next word"                                                },
    { .id = REMOVE_CHAR_FORWARD,     .name = "kb-remove-char-forward",     .keybinding = "Delete,Control+d",                     .comment = "Delete next char"                                                },
    { .id = REMOVE_CHAR_BACK,        .name = "kb-remove-char-back",        .keybinding = "BackSpace,Control+h",                  .comment = "Delete previous char"                                            },
    { .id = REMOVE_TO_EOL,           .name = "kb-remove-to-eol",           .keybinding = "Control+k",                            .comment = "Delete till the end of line"                                     },
    { .id = REMOVE_TO_SOL,           .name = "kb-remove-to-sol",           .keybinding = "Control+u",                            .comment = "Delete till the start of line"                                   },
    { .id = ACCEPT_ENTRY,            .name = "kb-accept-entry",            .keybinding = "Control+j,Control+m,Return,KP_Enter",  .comment = "Accept entry"                                                    },
    { .id = ACCEPT_CUSTOM,           .name = "kb-accept-custom",           .keybinding = "Control+Return",                       .comment = "Use entered text as command (in ssh/run modi)"                   },
    { .id = ACCEPT_ALT,              .name = "kb-accept-alt",              .keybinding = "Shift+Return",                         .comment = "Use alternate accept command."                                   },
    { .id = DELETE_ENTRY,            .name = "kb-delete-entry",            .keybinding = "Shift+Delete",                         .comment = "Delete entry from history"                                       },
    { .id = MODE_NEXT,               .name = "kb-mode-next",               .keybinding = "Shift+Right,Control+Tab",              .comment = "Switch to the next mode."                                        },
    { .id = MODE_PREVIOUS,           .name = "kb-mode-previous",           .keybinding = "Shift+Left,Control+ISO_Left_Tab",      .comment = "Switch to the previous mode."                                    },
    { .id = ROW_LEFT,                .name = "kb-row-left",                .keybinding = "Control+Page_Up",                      .comment = "Go to the previous column"                                       },
    { .id = ROW_RIGHT,               .name = "kb-row-right",               .keybinding = "Control+Page_Down",                    .comment = "Go to the next column"                                           },
    { .id = ROW_UP,                  .name = "kb-row-up",                  .keybinding = "Up,Control+p,ISO_Left_Tab",            .comment = "Select previous entry"                                           },
    { .id = ROW_DOWN,                .name = "kb-row-down",                .keybinding = "Down,Control+n",                       .comment = "Select next entry"                                               },
    { .id = ROW_TAB,                 .name = "kb-row-tab",                 .keybinding = "Tab",                                  .comment = "Go to next row, if one left, accept it, if no left next mode."   },
    { .id = PAGE_PREV,               .name = "kb-page-prev",               .keybinding = "Page_Up",                              .comment = "Go to the previous page"                                         },
    { .id = PAGE_NEXT,               .name = "kb-page-next",               .keybinding = "Page_Down",                            .comment = "Go to the next page"                                             },
    { .id = ROW_FIRST,               .name = "kb-row-first",               .keybinding = "Home,KP_Home",                         .comment = "Go to the first entry"                                           },
    { .id = ROW_LAST,                .name = "kb-row-last",                .keybinding = "End,KP_End",                           .comment = "Go to the last entry"                                            },
    { .id = ROW_SELECT,              .name = "kb-row-select",              .keybinding = "Control+space",                        .comment = "Set selected item as input text"                                 },
    { .id = SCREENSHOT,              .name = "kb-screenshot",              .keybinding = "Alt+S",                                .comment = "Take a screenshot of the rofi window"                            },
    { .id = TOGGLE_CASE_SENSITIVITY, .name = "kb-toggle-case-sensitivity", .keybinding = "grave,dead_grave",                     .comment = "Toggle case sensitivity"                                         },
    { .id = TOGGLE_SORT,             .name = "kb-toggle-sort",             .keybinding = "Alt+grave",                            .comment = "Toggle sort"                                                     },
    { .id = CANCEL,                  .name = "kb-cancel",                  .keybinding = "Escape,Control+g,Control+bracketleft", .comment = "Quit rofi"                                                       },
    { .id = CUSTOM_1,                .name = "kb-custom-1",                .keybinding = "Alt+1",                                .comment = "Custom keybinding 1"                                             },
    { .id = CUSTOM_2,                .name = "kb-custom-2",                .keybinding = "Alt+2",                                .comment = "Custom keybinding 2"                                             },
    { .id = CUSTOM_3,                .name = "kb-custom-3",                .keybinding = "Alt+3",                                .comment = "Custom keybinding 3"                                             },
    { .id = CUSTOM_4,                .name = "kb-custom-4",                .keybinding = "Alt+4",                                .comment = "Custom keybinding 4"                                             },
    { .id = CUSTOM_5,                .name = "kb-custom-5",                .keybinding = "Alt+5",                                .comment = "Custom Keybinding 5"                                             },
    { .id = CUSTOM_6,                .name = "kb-custom-6",                .keybinding = "Alt+6",                                .comment = "Custom keybinding 6"                                             },
    { .id = CUSTOM_7,                .name = "kb-custom-7",                .keybinding = "Alt+7",                                .comment = "Custom Keybinding 7"                                             },
    { .id = CUSTOM_8,                .name = "kb-custom-8",                .keybinding = "Alt+8",                                .comment = "Custom keybinding 8"                                             },
    { .id = CUSTOM_9,                .name = "kb-custom-9",                .keybinding = "Alt+9",                                .comment = "Custom keybinding 9"                                             },
    { .id = CUSTOM_10,               .name = "kb-custom-10",               .keybinding = "Alt+0",                                .comment = "Custom keybinding 10"                                            },
    { .id = CUSTOM_11,               .name = "kb-custom-11",               .keybinding = "Alt+exclam",                           .comment = "Custom keybinding 11"                                            },
    { .id = CUSTOM_12,               .name = "kb-custom-12",               .keybinding = "Alt+at",                               .comment = "Custom keybinding 12"                                            },
    { .id = CUSTOM_13,               .name = "kb-custom-13",               .keybinding = "Alt+numbersign",                       .comment = "Csutom keybinding 13"                                            },
    { .id = CUSTOM_14,               .name = "kb-custom-14",               .keybinding = "Alt+dollar",                           .comment = "Custom keybinding 14"                                            },
    { .id = CUSTOM_15,               .name = "kb-custom-15",               .keybinding = "Alt+percent",                          .comment = "Custom keybinding 15"                                            },
    { .id = CUSTOM_16,               .name = "kb-custom-16",               .keybinding = "Alt+dead_circumflex",                  .comment = "Custom keybinding 16"                                            },
    { .id = CUSTOM_17,               .name = "kb-custom-17",               .keybinding = "Alt+ampersand",                        .comment = "Custom keybinding 17"                                            },
    { .id = CUSTOM_18,               .name = "kb-custom-18",               .keybinding = "Alt+asterisk",                         .comment = "Custom keybinding 18"                                            },
    { .id = CUSTOM_19,               .name = "kb-custom-19",               .keybinding = "Alt+parenleft",                        .comment = "Custom Keybinding 19"                                            },
    { .id = SELECT_ELEMENT_1,        .name = "kb-select-1",                .keybinding = "Super+1",                              .comment = "Select row 1"                                                    },
    { .id = SELECT_ELEMENT_2,        .name = "kb-select-2",                .keybinding = "Super+2",                              .comment = "Select row 2"                                                    },
    { .id = SELECT_ELEMENT_3,        .name = "kb-select-3",                .keybinding = "Super+3",                              .comment = "Select row 3"                                                    },
    { .id = SELECT_ELEMENT_4,        .name = "kb-select-4",                .keybinding = "Super+4",                              .comment = "Select row 4"                                                    },
    { .id = SELECT_ELEMENT_5,        .name = "kb-select-5",                .keybinding = "Super+5",                              .comment = "Select row 5"                                                    },
    { .id = SELECT_ELEMENT_6,        .name = "kb-select-6",                .keybinding = "Super+6",                              .comment = "Select row 6"                                                    },
    { .id = SELECT_ELEMENT_7,        .name = "kb-select-7",                .keybinding = "Super+7",                              .comment = "Select row 7"                                                    },
    { .id = SELECT_ELEMENT_8,        .name = "kb-select-8",                .keybinding = "Super+8",                              .comment = "Select row 8"                                                    },
    { .id = SELECT_ELEMENT_9,        .name = "kb-select-9",                .keybinding = "Super+9",                              .comment = "Select row 9"                                                    },
    { .id = SELECT_ELEMENT_10,       .name = "kb-select-10",               .keybinding = "Super+0",                              .comment = "Select row 10"                                                   },
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

        config_parser_add_option ( xrm_String, abe[id].name, (void * *) &( abe[id].keystr ), bindings[iter].comment );
    }
}

gboolean parse_keys_abe ( void )
{
    GString *error_msg = g_string_new ( "" );
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
            memset ( kb, 0, sizeof ( KeyBinding ) );
            if ( x11_parse_key ( entry, &( kb->modmask ), &( kb->keysym ), &( kb->release ), error_msg ) ) {
                abe[iter].num_bindings++;
            }
            else {
                char *name = g_markup_escape_text ( abe[iter].name, -1 );
                g_string_append_printf ( error_msg, "Failed to set binding for: <b>%s</b>\n\n", name );
                g_free ( name );
            }
        }

        g_free ( keystr );
    }
    if ( error_msg->len > 0 ) {
        rofi_view_error_dialog ( error_msg->str, TRUE );
        g_string_free ( error_msg, TRUE );
        return FALSE;
    }
    g_string_free ( error_msg, TRUE );
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

/**
 * Array holding actions that should be trigger on release.
 */
static gboolean _abe_trigger_on_release[NUM_ABE] = { 0 };

static gboolean abe_test_action ( KeyBindingAction action, unsigned int mask, xkb_keysym_t key )
{
    ActionBindingEntry *akb = &( abe[action] );

    for ( int iter = 0; iter < akb->num_bindings; iter++ ) {
        const KeyBinding * const kb = &( akb->kb[iter] );
        if ( ( kb->keysym == key ) && ( kb->modmask == mask ) ) {
            if ( kb->release ) {
                _abe_trigger_on_release[action] = TRUE;
            }
            else {
                return TRUE;
            }
        }
    }

    return FALSE;
}

KeyBindingAction abe_find_action ( unsigned int mask, xkb_keysym_t key )
{
    KeyBindingAction action;

    for ( action = 0; action < NUM_ABE; ++action ) {
        if ( abe_test_action ( action, mask, key ) ) {
            break;
        }
    }

    return action;
}

void abe_trigger_release ( void )
{
    RofiViewState *state;

    state = rofi_view_get_active ( );
    if ( state ) {
        for ( KeyBindingAction action = 0; action < NUM_ABE; ++action ) {
            if ( _abe_trigger_on_release[action] ) {
                rofi_view_trigger_action ( state, action );
                _abe_trigger_on_release[action] = FALSE;
            }
        }
    }
}
