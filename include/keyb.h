#ifndef __KEYB_H__
#define __KEYB_H__

typedef enum _KeyBindingAction
{
    PASTE_PRIMARY = 0,
    PASTE_SECONDARY,
    CLEAR_LINE,
    MOVE_FRONT,
    MOVE_END,
    MOVE_WORD_BACK,
    MOVE_WORD_FORWARD,
    REMOVE_WORD_BACK,
    REMOVE_WORD_FORWARD,
    REMOVE_CHAR_FORWARD,
    REMOVE_CHAR_BACK,
    NUM_ABE
} KeyBindingAction;


typedef struct _KeyBinding
{
    unsigned int modmask;
    KeySym       keysym;
} KeyBinding;

typedef struct _ActionBindingEntry
{
    const char *name;
    char       *keystr;
    int        num_bindings;
    KeyBinding *kb;
} ActionBindingEntry;


void parse_keys_abe ( void );
void setup_abe ( void );

void cleanup_abe ( void );

extern ActionBindingEntry abe[NUM_ABE];
int abe_test_action ( KeyBindingAction action, unsigned int mask, KeySym key );
#endif // __KEYB_H__
