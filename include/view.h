#ifndef ROFI_VIEW_H
#define ROFI_VIEW_H

/**
 * @defgroup View View
 *
 * The rofi Menu view.
 *
 * @{
 */

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

void menu_update ( RofiViewState *state );
void menu_setup_fake_transparency ( Display *display, RofiViewState *state );

void rofi_view_cleanup ( void );
/** @} */
#endif
