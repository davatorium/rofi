/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2020 Qball Cow <qball@gmpclient.org>
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
 *
 */

#ifndef ROFI_VIEW_INTERNAL_H
#define ROFI_VIEW_INTERNAL_H
#include "widgets/container.h"
#include "widgets/widget.h"
#include "widgets/textbox.h"
#include "widgets/box.h"
#include "widgets/icon.h"
#include "widgets/listview.h"
#include "keyb.h"
#include "xcb.h"
#include "theme.h"

#ifdef ENABLE_WAYLAND
#include "wayland.h"
#endif

/**
 * @ingroup ViewHandle
 *
 * @{
 */
// State of the menu.

struct RofiViewState
{
    /** #Mode bound to to this view. */
    Mode             *sw;

    /** Flag indicating if view needs to be refiltered. */
    int              refilter;
    /** Widget representing the main container. */
    box              *main_window;
    /** #textbox showing the prompt in the input bar. */
    textbox          *prompt;
    /** #textbox with the user input in the input bar. */
    textbox          *text;
    /** #textbox showing the state of the case sensitive and sortng. */
    textbox          *case_indicator;

    /** #listview holding the displayed elements. */
    listview         *list_view;
    /** #textbox widget showing the overlay. */
    textbox          *overlay;
    /** #container holding the message box */
    container        *mesg_box;
    /** #textbox containing the message entry */
    textbox          *mesg_tb;

    /** Array with the levenshtein distance for each eleemnt. */
    int              *distance;
    /** Array with the translation between the filtered and unfiltered list. */
    unsigned int     *line_map;
    /** number of (unfiltered) elements to show. */
    unsigned int     num_lines;

    /** number of (filtered) elements to show. */
    unsigned int     filtered_lines;

    /** Previously called key action. */
    KeyBindingAction prev_action;
    /** Time previous key action was executed. */
    xcb_timestamp_t  last_button_press;

    /** Indicate view should terminate */
    int              quit;
    /** Indicate if we should absorb the key release */
    int              skip_absorb;
    /** The selected line (in the unfiltered list) */
    unsigned int     selected_line;
    /** The return state of the view */
    MenuReturn       retv;
    /** Monitor #workarea the view is displayed on */
    workarea         mon;

    /** #box holding the different modi buttons */
    box              *sidebar_bar;
    /** number of modi to display */
    unsigned int     num_modi;
    /** Array of #textbox that act as buttons for switching modi */
    textbox          **modi;

    /** Total rows. */
    textbox          *tb_total_rows;
    /** filtered rows */
    textbox          *tb_filtered_rows;

    /** Settings of the menu */
    MenuFlags        menu_flags;
    /** If mouse was within view previously */
    int              mouse_seen;
    /** Flag indicating if view needs to be reloaded. */
    int              reload;
    /** The function to be called when finalizing this view */
    void             ( *finalize )( struct RofiViewState *state );

    /** Width of the view */
    int              width;
    /** Height of the view */
    int              height;
    /** X position of the view */
    int              x;
    /** Y position of the view */
    int              y;

#ifdef ENABLE_WAYLAND
    /** wayland */
    display_buffer_pool *pool;
    gboolean frame_callback;
#endif

    /** Position and target of the mouse. */
    struct
    {
        /** X position */
        int    x;
        /** Y position */
        int    y;
        /** Widget being targetted. */
        widget *motion_target;
    }                mouse;

    /** Regexs used for matching */
    rofi_int_matcher **tokens;
};
/** @} */

typedef struct _view_proxy {
    void (*update) ( RofiViewState *state, gboolean qr );
    void (*maybe_update) ( RofiViewState *state );
    void (*temp_configure_notify) ( RofiViewState *state, xcb_configure_notify_event_t *xce );
    void (*temp_click_to_exit) ( RofiViewState *state, xcb_window_t target );
    void (*frame_callback) ( void );

    void (*queue_redraw) ( void );

    void (*set_window_title) ( const char * title );
    void (*calculate_window_position) ( RofiViewState *state );
    void (*calculate_window_width) ( RofiViewState *state );
    int (*calculate_window_height) ( RofiViewState *state );
    void (*window_update_size) ( RofiViewState *state );

    void (*cleanup) ( void );
    void (*hide) ( void );
    void (*reload) ( void  );
    void (*__create_window) ( MenuFlags menu_flags );
    xcb_window_t (*get_window) ( void );

    void (*get_current_monitor) ( int *width, int *height );
    void (*capture_screenshot) ( void );

    void (*set_size) ( RofiViewState * state, gint width, gint height );
    void (*get_size) ( RofiViewState * state, gint *width, gint *height );
} view_proxy;

/**
 * Structure holding cached state.
 */
struct _rofi_view_cache_state
{
    /** main x11 windows */
    xcb_window_t       main_window;
    /** Main flags */
    MenuFlags          flags;
    /** List of stacked views */
    GQueue             views;
};
extern struct _rofi_view_cache_state CacheState;

void rofi_view_update ( struct RofiViewState *state, gboolean qr );
void rofi_view_calculate_window_position ( struct RofiViewState *state );
void rofi_view_calculate_window_width ( struct RofiViewState *state );
int rofi_view_calculate_window_height ( struct RofiViewState *state );
void rofi_view_window_update_size ( struct RofiViewState * state );
void rofi_view_refilter ( struct RofiViewState *state );
void rofi_view_set_window_title ( const char * title );

#endif
