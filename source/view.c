/**
 * rofi
 *
 * MIT/X11 License
 * Modified 2016 Qball Cow <qball@gmpclient.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <locale.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <glib-unix.h>

#include <cairo.h>
#include <cairo-xlib.h>

#define SN_API_NOT_YET_FROZEN
#include <libsn/sn.h>

#include "settings.h"
#include "mode.h"
#include "rofi.h"
#include "helper.h"
#include "textbox.h"
#include "scrollbar.h"
#include "x11-helper.h"
#include "x11-event-source.h"
#include "xrmoptions.h"
#include "dialogs/dialogs.h"
#include "rofi.h"

extern RofiViewState *current_active_menu;
extern Display       *display;
extern unsigned int  num_modi;
extern Window        main_window;

#include "view.h"

void rofi_view_queue_redraw ( void )
{
    if ( current_active_menu ) {
        current_active_menu->update = TRUE;
        XClearArea ( display, main_window, 0, 0, 1, 1, True );
        XFlush ( display );
    }
}

void rofi_view_restart ( RofiViewState *state )
{
    state->quit = FALSE;
    state->retv = MENU_CANCEL;
}
void rofi_view_set_active ( RofiViewState *state )
{
    g_assert ( ( current_active_menu == NULL && state != NULL ) || ( current_active_menu != NULL && state == NULL ) );
    current_active_menu = state;
}

void rofi_view_set_selected_line ( RofiViewState *state, unsigned int selected_line )
{
    state->selected_line = selected_line;
    // Find the line.
    state->selected = 0;
    for ( unsigned int i = 0; ( ( state->selected_line ) ) < UINT32_MAX && !state->selected && i < state->filtered_lines; i++ ) {
        if ( state->line_map[i] == ( state->selected_line ) ) {
            state->selected = i;
            break;
        }
    }

    state->update = TRUE;
}

void rofi_view_free ( RofiViewState *state )
{
    // Do this here?
    // Wait for final release?
    if ( !state->skip_absorb ) {
        XEvent ev;
        do {
            XNextEvent ( display, &ev );
        } while ( ev.type != KeyRelease );
    }
    textbox_free ( state->text );
    textbox_free ( state->prompt_tb );
    textbox_free ( state->case_indicator );
    scrollbar_free ( state->scrollbar );

    for ( unsigned int i = 0; i < state->max_elements; i++ ) {
        textbox_free ( state->boxes[i] );
    }

    g_free ( state->boxes );
    g_free ( state->line_map );
    g_free ( state->distance );
    g_free ( state->lines_not_ascii );
    g_free ( state );
    // Free the switcher boxes.
    // When state is free'ed we should no longer need these.
    if ( config.sidebar_mode == TRUE ) {
        for ( unsigned int j = 0; j < state->num_modi; j++ ) {
            textbox_free ( state->modi[j] );
            state->modi[j] = NULL;
        }
        g_free ( state->modi );
        state->num_modi = 0;
    }
}

MenuReturn rofi_view_get_return_value ( const RofiViewState *state )
{
    return state->retv;
}
unsigned int rofi_view_get_selected_line ( const RofiViewState *state )
{
    return state->selected_line;
}

unsigned int rofi_view_get_next_position ( const RofiViewState *state )
{
    unsigned int next_pos = state->selected_line;
    if ( ( state->selected + 1 ) < state->num_lines ) {
        ( next_pos ) = state->line_map[state->selected + 1];
    }
    return next_pos;
}

unsigned int rofi_view_get_completed ( const RofiViewState *state )
{
    return state->quit;
}
void rofi_view_itterrate ( RofiViewState *state, XEvent *event )
{
    state->x11_event_loop ( state, event );
}
const char * rofi_view_get_user_input ( const RofiViewState *state )
{
    if ( state->text ) {
        return state->text->text;
    }
    return NULL;
}

/**
 * Create a new, 0 initialized RofiViewState structure.
 *
 * @returns a new 0 initialized RofiViewState
 */
RofiViewState * rofi_view_state_create ( void )
{
    return g_malloc0 ( sizeof ( RofiViewState ) );
}

/**
 * @param state The Menu Handle
 *
 * Check if a finalize function is set, and if sets executes it.
 */
void rofi_view_finalize ( RofiViewState *state )
{
    if ( state && state->finalize ) {
        state->finalize ( state );
    }
}
