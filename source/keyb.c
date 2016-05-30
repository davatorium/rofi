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

ActionBindingEntry abe[NUM_ABE];

/**
 * LIST OF DEFAULT SETTINGS
 */
DefaultBinding bindings[NUM_ABE] =
{
    { .id = PASTE_PRIMARY,           .name = "kb-primary-paste",           .keybinding = "Control+Shift+v,Shift+Insert",              .comment = "Keybinding paste primary clipboard"   },
    { .id = PASTE_SECONDARY,         .name = "kb-secondary-paste",         .keybinding = "Control+v,Insert",                          .comment = "Keybinding paste secondary clipboard" },
    { .id = CLEAR_LINE,              .name = "kb-clear-line",              .keybinding = "Control+u",                                 .comment = "Keybinding clear input line"          },
    { .id = MOVE_FRONT,              .name = "kb-move-front",              .keybinding = "Control+a",                                 .comment = "Keybinding move cursor to front"      },
    { .id = MOVE_END,                .name = "kb-move-end",                .keybinding = "Control+e",                                 .comment = "Keybinding move cursor to end"        },
    { .id = MOVE_WORD_BACK,          .name = "kb-move-word-back",          .keybinding = "Alt+b",                                     .comment = "Keybinding move word back"            },
    { .id = MOVE_WORD_FORWARD,       .name = "kb-move-word-forward",       .keybinding = "Alt+f",                                     .comment = "Keybinding"                           },
    { .id = MOVE_CHAR_BACK,          .name = "kb-move-char-back",          .keybinding = "Left,Control+b",                            .comment = "Keybinding"                           },
    { .id = MOVE_CHAR_FORWARD,       .name = "kb-move-char-forward",       .keybinding = "Right,Control+f",                           .comment = "Keybinding"                           },
    { .id = REMOVE_WORD_BACK,        .name = "kb-remove-word-back",        .keybinding = "Control+Alt+h",                             .comment = "Keybinding"                           },
    { .id = REMOVE_WORD_FORWARD,     .name = "kb-remove-word-forward",     .keybinding = "Control+Alt+d",                             .comment = "Keybinding"                           },
    { .id = REMOVE_CHAR_FORWARD,     .name = "kb-remove-char-forward",     .keybinding = "Delete,Control+d",                          .comment = "Keybinding"                           },
    { .id = REMOVE_CHAR_BACK,        .name = "kb-remove-char-back",        .keybinding = "BackSpace,Control+h",                       .comment = "Keybinding"                           },
    { .id = ACCEPT_ENTRY,            .name = "kb-accept-entry",            .keybinding = "Control+j,Control+m,Return,KP_Enter",       .comment = "Keybinding"                           },
    { .id = ACCEPT_CUSTOM,           .name = "kb-accept-custom",           .keybinding = "Control+Return,Shift+Return",               .comment = "Keybinding"                           },
    { .id = MODE_NEXT,               .name = "kb-mode-next",               .keybinding = "Shift+Right,Control+Tab",                   .comment = "Keybinding"                           },
    { .id = MODE_PREVIOUS,           .name = "kb-mode-previous",           .keybinding = "Shift+Left,Control+Shift+Tab",              .comment = "Keybinding"                           },
    { .id = TOGGLE_CASE_SENSITIVITY, .name = "kb-toggle-case-sensitivity", .keybinding = "grave,dead_grave",                          .comment = "Keybinding"                           },
    { .id = DELETE_ENTRY,            .name = "kb-delete-entry",            .keybinding = "Shift+Delete",                              .comment = "Keybinding"                           },
    { .id = ROW_LEFT,                .name = "kb-row-left",                .keybinding = "Control+Page_Up",                           .comment = "Keybinding"                           },
    { .id = ROW_RIGHT,               .name = "kb-row-right",               .keybinding = "Control+Page_Down",                         .comment = "Keybinding"                           },
    { .id = ROW_UP,                  .name = "kb-row-up",                  .keybinding = "Up,Control+p,Shift+Tab,Shift+ISO_Left_Tab", .comment = "Keybinding"                           },
    { .id = ROW_DOWN,                .name = "kb-row-down",                .keybinding = "Down,Control+n",                            .comment = "Keybinding"                           },
    { .id = ROW_TAB,                 .name = "kb-row-tab",                 .keybinding = "Tab",                                       .comment = "Keybinding"                           },
    { .id = PAGE_PREV,               .name = "kb-page-prev",               .keybinding = "Page_Up",                                   .comment = "Keybinding"                           },
    { .id = PAGE_NEXT,               .name = "kb-page-next",               .keybinding = "Page_Down",                                 .comment = "Keybinding"                           },
    { .id = ROW_FIRST,               .name = "kb-row-first",               .keybinding = "Home,KP_Home",                              .comment = "Keybinding"                           },
    { .id = ROW_LAST,                .name = "kb-row-last",                .keybinding = "End,KP_End",                                .comment = "Keybinding"                           },
    { .id = ROW_SELECT,              .name = "kb-row-select",              .keybinding = "Control+space",                             .comment = "Keybinding"                           },
    { .id = CANCEL,                  .name = "kb-cancel",                  .keybinding = "Escape,Control+bracketleft",                .comment = "Keybinding"                           },
    { .id = CUSTOM_1,                .name = "kb-custom-1",                .keybinding = "Alt+1",                                     .comment = "Keybinding"                           },
    { .id = CUSTOM_2,                .name = "kb-custom-2",                .keybinding = "Alt+2",                                     .comment = "Keybinding"                           },
    { .id = CUSTOM_3,                .name = "kb-custom-3",                .keybinding = "Alt+3",                                     .comment = "Keybinding"                           },
    { .id = CUSTOM_4,                .name = "kb-custom-4",                .keybinding = "Alt+4",                                     .comment = "Keybinding"                           },
    { .id = CUSTOM_5,                .name = "kb-custom-5",                .keybinding = "Alt+5",                                     .comment = "Keybinding"                           },
    { .id = CUSTOM_6,                .name = "kb-custom-6",                .keybinding = "Alt+6",                                     .comment = "Keybinding"                           },
    { .id = CUSTOM_7,                .name = "kb-custom-7",                .keybinding = "Alt+7",                                     .comment = "Keybinding"                           },
    { .id = CUSTOM_8,                .name = "kb-custom-8",                .keybinding = "Alt+8",                                     .comment = "Keybinding"                           },
    { .id = CUSTOM_9,                .name = "kb-custom-9",                .keybinding = "Alt+9",                                     .comment = "Keybinding"                           },
    { .id = CUSTOM_10,               .name = "kb-custom-10",               .keybinding = "Alt+0",                                     .comment = "Keybinding"                           },
    { .id = CUSTOM_11,               .name = "kb-custom-11",               .keybinding = "Alt+Shift+1",                               .comment = "Keybinding"                           },
    { .id = CUSTOM_12,               .name = "kb-custom-12",               .keybinding = "Alt+Shift+2",                               .comment = "Keybinding"                           },
    { .id = CUSTOM_13,               .name = "kb-custom-13",               .keybinding = "Alt+Shift+3",                               .comment = "Keybinding"                           },
    { .id = CUSTOM_14,               .name = "kb-custom-14",               .keybinding = "Alt+Shift+4",                               .comment = "Keybinding"                           },
    { .id = CUSTOM_15,               .name = "kb-custom-15",               .keybinding = "Alt+Shift+5",                               .comment = "Keybinding"                           },
    { .id = CUSTOM_16,               .name = "kb-custom-16",               .keybinding = "Alt+Shift+6",                               .comment = "Keybinding"                           },
    { .id = CUSTOM_18,               .name = "kb-custom-18",               .keybinding = "Alt+Shift+8",                               .comment = "Keybinding"                           },
    { .id = CUSTOM_17,               .name = "kb-custom-17",               .keybinding = "Alt+Shift+7",                               .comment = "Keybinding"                           },
    { .id = CUSTOM_19,               .name = "kb-custom-19",               .keybinding = "Alt+Shift+9",                               .comment = "Keybinding"                           },
    { .id = SCREENSHOT,              .name = "kb-screenshot",              .keybinding = "Alt+Shift+S",                               .comment = "Keybinding"                           },
    { .id = TOGGLE_SORT,             .name = "kb-toggle-sort",             .keybinding = "Alt+grave",                                 .comment = "Keybinding"                           },
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
            if ( !x11_parse_key ( entry, &( kb->modmask ), &( kb->keysym ), &( kb->release ) ) ) {
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
    RofiViewState    *state;
    KeyBindingAction action;

    state = rofi_view_get_active ( );
    for ( action = 0; action < NUM_ABE; ++action ) {
        if ( _abe_trigger_on_release[action] ) {
            rofi_view_trigger_action ( state, action );
            _abe_trigger_on_release[action] = FALSE;
        }
    }

    rofi_view_update ( state );
}
