#ifndef ROFI_VIEW_H
#define ROFI_VIEW_H

/**
 * @defgroup View View
 *
 * The rofi Menu view.
 *
 * @defgroup ViewHandle ViewHandle
 * @ingroup View
 *
 * @{
 */
typedef struct RofiViewState   RofiViewState;
typedef enum
{
    /** Create a menu for entering text */
    MENU_NORMAL        = 0,
    /** Create a menu for entering passwords */
    MENU_PASSWORD      = 1,
    /** Create amanaged window. */
    MENU_NORMAL_WINDOW = 2,
} MenuFlags;

/**
 * @param sw the Mode to show.
 * @param input A pointer to a string where the inputted data is placed.
 * @param prompt The prompt to show.
 * @param message Extra message to display.
 * @param flags   Flags indicating state of the menu.
 * @param finalize the finailze callback
 *
 * Main menu callback.
 *
 * @returns The command issued (see MenuReturn)
 */
RofiViewState *rofi_view_create ( Mode *sw, const char *input, char *prompt, const char *message, MenuFlags flags, void ( *finalize )(
                                      RofiViewState * ) )
__attribute__ ( ( nonnull ( 1, 2, 3, 6 ) ) );

/**
 * @param state The Menu Handle
 *
 * Check if a finalize function is set, and if sets executes it.
 */
void rofi_view_finalize ( RofiViewState *state );

MenuReturn rofi_view_get_return_value ( const RofiViewState *state );
unsigned int rofi_view_get_next_position ( const RofiViewState *state );
void rofi_view_itterrate ( RofiViewState *state, XEvent *event );
unsigned int rofi_view_get_completed ( const RofiViewState *state );
const char * rofi_view_get_user_input ( const RofiViewState *state );

/**
 * @param state The Menu Handle
 * @param selected_line The line to select.
 *
 * Select a line.
 */
void rofi_view_set_selected_line ( RofiViewState *state, unsigned int selected_line );

/**
 * @param state The Menu Handle
 *
 * Get the selected line.
 *
 * @returns the selected line or UINT32_MAX if none selected.
 */
unsigned int rofi_view_get_selected_line ( const RofiViewState *state );
/**
 * @param state The Menu Handle
 *
 * Restart the menu so it can be displayed again.
 * Resets RofiViewState::quit and RofiViewState::retv.
 */
void rofi_view_restart ( RofiViewState *state );

/**
 * @param state The handle to the view
 *
 * Update the state of the view. This involves filter state.
 */
void rofi_view_update ( RofiViewState *state );

/**
 * @param display Connection to the X server.
 * @param state The handle to the view
 *
 * Enables fake transparancy on this view.
 */
void rofi_view_setup_fake_transparency ( Display *display, RofiViewState *state );

/**
 * @param state The handle to the view
 *
 * Free's the memory allocated for this handle.
 * After a call to this function, state is invalid and can no longer be used.
 */
void rofi_view_free ( RofiViewState *state );
/** @} */
/**
 * @defgroup ViewGlobal ViewGlobal
 * @ingroup View
 *
 * Global menu view functions.
 * These do not work on the view itself but modifies the global state.
 * @{
 */

/**
 * Get the current active view Handle.
 *
 * @returns the active view handle or NULL
 */
RofiViewState * rofi_view_get_active ( void );

/**
 * @param state the new active view handle, NULL to clear.
 *
 * Set the current active view Handle.
 *
 */
void rofi_view_set_active ( RofiViewState *state );

/**
 * @param msg The error message to show.
 * @param markup The error message uses pango markup.
 *
 * The error message to show.
 */
void rofi_view_error_dialog ( const char *msg, int markup  );

/**
 * Queue a redraw.
 * This triggers a X11 Expose Event.
 */
void rofi_view_queue_redraw ( void );

/**
 * Cleanup internal data of the view.
 */
void rofi_view_cleanup ( void );

/**
 * @param data A thread_state object.
 * @param user_data User data to pass to thread_state callback
 *
 * Small wrapper function that is internally used to pass a job to a worker.
 */
void rofi_view_call_thread ( gpointer data, gpointer user_data );

Mode * rofi_view_get_mode ( RofiViewState *state );
/** @} */
/***
 * @defgroup ViewThreadPool ViewThreadPool
 * @ingroup View
 *
 * The view can (optionally) keep a set of worker threads around to parallize work.
 * This includes filtering and sorting.
 *
 * @{
 */
void rofi_view_workers_initialize ( void );
void rofi_view_workers_finalize ( void );

/**@}*/
#endif
