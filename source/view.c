#include <glib.h>
#include <xcb/xcb.h>
#include "keyb.h"
#include "display.h"

#include "view.h"
#include "view-internal.h"

static const view_proxy *proxy;

void view_init( const view_proxy *view_in )
{
    proxy = view_in;
}

GThreadPool *tpool = NULL;

RofiViewState *rofi_view_create ( Mode *sw, const char *input, MenuFlags menu_flags, void ( *finalize )( RofiViewState * ) ) {
    return proxy->create ( sw, input, menu_flags, finalize );
}

void rofi_view_finalize ( RofiViewState *state ) {
    return proxy->finalize ( state );
}

MenuReturn rofi_view_get_return_value ( const RofiViewState *state ) {
    return proxy->get_return_value ( state );
}

unsigned int rofi_view_get_next_position ( const RofiViewState *state ) {
    return proxy->get_next_position ( state );
}

void rofi_view_handle_text ( RofiViewState *state, char *text ) {
    proxy->handle_text ( state, text );
}

void rofi_view_handle_mouse_motion ( RofiViewState *state, gint x, gint y ) {
    proxy->handle_mouse_motion ( state, x , y );
}

void rofi_view_maybe_update ( RofiViewState *state ) {
    proxy->maybe_update ( state );
}

void rofi_view_temp_configure_notify ( RofiViewState *state, xcb_configure_notify_event_t *xce ) {
    proxy->temp_configure_notify ( state, xce );
}

void rofi_view_temp_click_to_exit ( RofiViewState *state, xcb_window_t target ) {
    proxy->temp_click_to_exit ( state, target );
}

void rofi_view_frame_callback ( void ) {
    proxy->frame_callback ( );
}

unsigned int rofi_view_get_completed ( const RofiViewState *state ) {
    return proxy->get_completed ( state );
}

const char * rofi_view_get_user_input ( const RofiViewState *state ) {
    return proxy->get_user_input ( state );
}

void rofi_view_set_selected_line ( RofiViewState *state, unsigned int selected_line ) {
    proxy->set_selected_line ( state, selected_line );
}

unsigned int rofi_view_get_selected_line ( const RofiViewState *state ) {
    return proxy->get_selected_line ( state );
}

void rofi_view_restart ( RofiViewState *state ) {
    proxy->restart ( state );
}

gboolean rofi_view_trigger_action ( RofiViewState *state, BindingsScope scope, guint action ) {
    return proxy->trigger_action ( state, scope, action );
}

void rofi_view_free ( RofiViewState *state ) {
    return proxy->free ( state );
}

RofiViewState * rofi_view_get_active ( void ) {
    return proxy->get_active ( );
}

void rofi_view_set_active ( RofiViewState *state ) {
    proxy->set_active ( state );
}

int rofi_view_error_dialog ( const char *msg, int markup  ) {
    return proxy->error_dialog ( msg, markup );
}

void rofi_view_queue_redraw ( void ) {
    proxy->queue_redraw ( );
}

void rofi_view_cleanup ( void ) {
    proxy->cleanup ( );
}

Mode * rofi_view_get_mode ( RofiViewState *state ) {
    return proxy->get_mode ( state );
}

void rofi_view_hide ( void ) {
    proxy->hide ( );
}

void rofi_view_reload ( void  ) {
    proxy->reload ( );
}

void rofi_view_switch_mode ( RofiViewState *state, Mode *mode ) {
    proxy->switch_mode ( state, mode );
}

void rofi_view_set_overlay ( RofiViewState *state, const char *text ) {
    proxy->set_overlay ( state, text );
}

void rofi_view_clear_input ( RofiViewState *state ) {
    proxy->clear_input ( state );
}

void __create_window ( MenuFlags menu_flags ) {
    proxy->__create_window ( menu_flags );
}

xcb_window_t rofi_view_get_window ( void ) {
    return proxy->get_window ( );
}

void rofi_view_workers_initialize ( void ) {
    proxy->workers_initialize ( );
}

void rofi_view_workers_finalize ( void ) {
    proxy->workers_finalize ( );
}

void rofi_view_get_current_monitor ( int *width, int *height ) {
    proxy->get_current_monitor ( width, height );
}

void rofi_capture_screenshot ( void ) {
    proxy->capture_screenshot ( );
}

void rofi_view_ellipsize_start ( RofiViewState *state ) {
    proxy->ellipsize_start ( state );
}

void rofi_view_set_size ( RofiViewState * state, gint width, gint height ) {
    proxy->set_size ( state, width, height );
}

void rofi_view_get_size ( RofiViewState * state, gint *width, gint *height ) {
    proxy->get_size ( state, width, height );
}
