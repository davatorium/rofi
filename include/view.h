#ifndef ROFI_VIEW_H
#define ROFI_VIEW_H

/**
 * @defgroup View View
 *
 * The rofi Menu view.
 *
 * @{
 */
typedef struct RofiViewState   RofiViewState;
typedef enum
{
    MENU_NORMAL   = 0,
    MENU_PASSWORD = 1
} MenuFlags;

/**
 * @param sw the Mode to show.
 * @param lines An array of strings to display.
 * @param num_lines Length of the array with strings to display.
 * @param input A pointer to a string where the inputted data is placed.
 * @param prompt The prompt to show.
 * @param shift pointer to integer that is set to the state of the shift key.
 * @param mmc Menu menu match callback, used for matching user input.
 * @param mmc_data data to pass to mmc.
 * @param selected_line pointer to integer holding the selected line.
 * @param message Extra message to display.
 * @param flags   Flags indicating state of the menu.
 *
 * Main menu callback.
 *
 * @returns The command issued (see MenuReturn)
 */
RofiViewState *rofi_view_create ( Mode *sw,
                                  const char *input, char *prompt,
                                  const char *message, MenuFlags flags )
__attribute__ ( ( nonnull ( 1, 2, 3  ) ) );
/**
 * @param state The Menu Handle
 *
 * Check if a finalize function is set, and if sets executes it.
 */
void rofi_view_finalize ( RofiViewState *state );

RofiViewState * rofi_view_state_create ( void );

MenuReturn rofi_view_get_return_value ( const RofiViewState *state );
unsigned int rofi_view_get_selected_line ( const RofiViewState *state );
unsigned int rofi_view_get_next_position ( const RofiViewState *state );
void rofi_view_itterrate ( RofiViewState *state, XEvent *event );
unsigned int rofi_view_get_completed ( const RofiViewState *state );
const char * rofi_view_get_user_input ( const RofiViewState *state );
void rofi_view_free ( RofiViewState *state );
void rofi_view_restart ( RofiViewState *state );
void rofi_view_set_selected_line ( RofiViewState *state, unsigned int selected_line );
void rofi_view_queue_redraw ( void );
void rofi_view_set_active ( RofiViewState *state );

void rofi_view_call_thread ( gpointer data, gpointer user_data );

void rofi_view_update ( RofiViewState *state );
void rofi_view_setup_fake_transparency ( Display *display, RofiViewState *state );

void rofi_view_cleanup ( void );
/** @} */
#endif
