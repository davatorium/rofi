#ifndef __KEYB_H__
#define __KEYB_H__

typedef enum _KeyBindingAction
{
    PASTE_PRIMARY = 0,
    PASTE_SECONDARY,
    CLEAR_LINE,
    MOVE_FRONT,
    MOVE_END,
    REMOVE_WORD_BACK,
    REMOVE_WORD_FORWARD,
    REMOVE_CHAR_FORWARD,
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
    int        num_bindings;
    KeyBinding *kb;
} ActionBindingEntry;


void setup_abe ( void );

extern ActionBindingEntry abe[NUM_ABE];
int abe_test_action ( KeyBindingAction action, unsigned int mask, KeySym key );
#endif // __KEYB_H__
