#ifndef ROFI_VIEW_H
#define ROFI_VIEW_H

#include "xkb.h"

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
    /** ERROR dialog */
    MENU_ERROR_DIALOG  = 4,
    /** INDICATOR */
    MENU_INDICATOR     = 8,
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
RofiViewState *rofi_view_create ( Mode *sw, const char *input, const char *message, MenuFlags flags, void ( *finalize )(
                                      RofiViewState * ) )
__attribute__ ( ( nonnull ( 1, 2, 5 ) ) );

/**
 * @param state The Menu Handle
 *
 * Check if a finalize function is set, and if sets executes it.
 */
void rofi_view_finalize ( RofiViewState *state );

MenuReturn rofi_view_get_return_value ( const RofiViewState *state );
unsigned int rofi_view_get_next_position ( const RofiViewState *state );
void rofi_view_itterrate ( RofiViewState *state, xcb_generic_event_t *event, xkb_stuff *xkb );
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

gboolean rofi_view_trigger_action ( RofiViewState *state, KeyBindingAction action );

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
int rofi_view_error_dialog ( const char *msg, int markup  );

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
 * @param state The handle to the view
 *
 * Get the mode currently displayed by the view.
 *
 * @returns the mode currently displayed by the view
 */
Mode * rofi_view_get_mode ( RofiViewState *state );

/**
 * Unmap the current view.
 */
void rofi_view_hide ( void );

/**
 * Indicate the current view needs to reload its data.
 * This can only be done when *more* information is available.
 *
 * The reloading happens 'lazy', multiple calls might be handled at once.
 */
void rofi_view_reload ( void  );

/**
 * @param state The handle to the view
 * @param mode The new mode to display
 *
 * Change the current view to show a different mode.
 */
void rofi_view_switch_mode ( RofiViewState *state, Mode *mode );

/**
 * @param state The handle to the view
 * @param text An UTF-8 encoded character array with the text to overlay.
 *
 * Overlays text over the current view. Passing NULL for text hides the overlay.
 */
void rofi_view_set_overlay ( RofiViewState *state, const char *text );

/**
 * @param menu_flags The state of the new window.
 *
 * Creates the internal 'Cached' window that gets reused between views.
 * @TODO: Internal call to view exposed.
 */
void __create_window ( MenuFlags menu_flags );
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
/**
 * Initialize the threadpool
 */
void rofi_view_workers_initialize ( void );
/**
 * Stop all threads and free the resources used by the threadpool
 */
void rofi_view_workers_finalize ( void );

/**@}*/
#endif
