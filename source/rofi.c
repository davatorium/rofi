/**
 * rofi
 *
 * MIT/X11 License
 * Copyright (c) 2012 Sean Pringle <sean.pringle@gmail.com>
 * Modified 2013-2015 Qball Cow <qball@gmpclient.org>
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
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "rofi.h"
#include "helper.h"
#include "textbox.h"
#include "scrollbar.h"
#include "x11-helper.h"
#include "xrmoptions.h"
#include "dialogs/dialogs.h"
//
// TEMP
SwitcherMode switcher_run ( char **input, Switcher *sw );

typedef enum _MainLoopEvent
{
    ML_XEVENT,
    ML_TIMEOUT
} MainLoopEvent;

// Pidfile.
char         *pidfile           = NULL;
const char   *cache_dir         = NULL;
Display      *display           = NULL;
char         *display_str       = NULL;

extern Atom  netatoms[NUM_NETATOMS];
Window       main_window        = None;
GC           gc                 = NULL;
Colormap     map                = None;
XVisualInfo  vinfo;

unsigned int normal_window_mode = FALSE;

typedef struct _Mode
{
    Switcher *sw;
    textbox  *tb;
} Mode;
// Array of switchers.
Mode         *switchers = NULL;
// Number of switchers.
unsigned int num_switchers = 0;
// Current selected switcher.
unsigned int curr_switcher = 0;

/**
 * @param name Name of the switcher to lookup.
 *
 * Find the index of the switcher with name.
 *
 * @returns index of the switcher in switchers, -1 if not found.
 */
static int switcher_get ( const char *name )
{
    for ( unsigned int i = 0; i < num_switchers; i++ ) {
        if ( strcmp ( switchers[i].sw->name, name ) == 0 ) {
            return i;
        }
    }
    return -1;
}

/**
 * @param display Connection to the X server.
 * @param x11_fd  File descriptor from the X server to listen on.
 *
 * Function waits for a new XEvent with a timeout.
 */
static inline MainLoopEvent wait_for_xevent_or_timeout ( Display *display, int x11_fd )
{
    // Check if events are pending.
    if ( XPending ( display ) ) {
        return ML_XEVENT;
    }
    // If not, wait for timeout.
    struct timeval tv;
    fd_set         in_fds;
    // Create a File Description Set containing x11_fd
    FD_ZERO ( &in_fds );
    FD_SET ( x11_fd, &in_fds );

    // Set our timer.  200ms is a decent delay
    tv.tv_usec = 200000;
    tv.tv_sec  = 0;
    // Wait for X Event or a Timer
    if ( select ( x11_fd + 1, &in_fds, 0, 0, &tv ) == 0 ) {
        return ML_TIMEOUT;
    }
    return ML_XEVENT;
}


/**
 * Levenshtein Sorting.
 */

static int lev_sort ( const void *p1, const void *p2, void *arg )
{
    const int *a         = p1;
    const int *b         = p2;
    int       *distances = arg;

    return distances[*a] - distances[*b];
}

static int dist ( const char *s, const char *t, int *d, int ls, int lt, int i, int j )
{
    if ( d[i * ( lt + 1 ) + j] >= 0 ) {
        return d[i * ( lt + 1 ) + j];
    }

    int x;
    if ( i == ls ) {
        x = lt - j;
    }
    else if ( j == lt ) {
        x = ls - i;
    }
    else if ( s[i] == t[j] ) {
        x = dist ( s, t, d, ls, lt, i + 1, j + 1 );
    }
    else {
        x = dist ( s, t, d, ls, lt, i + 1, j + 1 );

        int y;
        if ( ( y = dist ( s, t, d, ls, lt, i, j + 1 ) ) < x ) {
            x = y;
        }
        if ( ( y = dist ( s, t, d, ls, lt, i + 1, j ) ) < x ) {
            x = y;
        }
        x++;
    }
    return d[i * ( lt + 1 ) + j] = x;
}
static int levenshtein ( const char *s, const char *t )
{
    int    ls           = strlen ( s ), lt = strlen ( t );
    size_t array_length = ( ls + 1 ) * ( lt + 1 );

    // For some reason Coverity does not get that I initialize the
    // array in for loop.
    int d[array_length];
    for ( size_t i = 0; i < array_length; i++ ) {
        d[i] = -1;
    }

    return dist ( s, t, d, ls, lt, 0, 0 );
}

static Window create_window ( Display *display )
{
    XSetWindowAttributes attr;
    attr.colormap         = map;
    attr.border_pixel     = color_border ( display );
    attr.background_pixel = color_background ( display );

    Window box = XCreateWindow ( display, DefaultRootWindow ( display ),
                                 0, 0, 200, 100, config.menu_bw, vinfo.depth, InputOutput,
                                 vinfo.visual, CWColormap | CWBorderPixel | CWBackPixel, &attr );
    XSelectInput ( display, box, ExposureMask | ButtonPressMask | StructureNotifyMask | FocusChangeMask );

    gc = XCreateGC ( display, box, 0, 0 );
    int line_style = LineOnOffDash;
    if ( strcasecmp ( config.separator_style, "dash" ) == 0 ) {
        line_style = LineOnOffDash;
    }
    else if ( strcasecmp ( config.separator_style, "solid" ) == 0 ) {
        line_style = LineSolid;
    }
    XSetLineAttributes ( display, gc, 2, line_style, CapButt, JoinMiter );
    XSetForeground ( display, gc, color_separator ( display ) );
    // make it an unmanaged window
    if ( !normal_window_mode ) {
        window_set_atom_prop ( display, box, netatoms[_NET_WM_STATE], &netatoms[_NET_WM_STATE_ABOVE], 1 );
        XSetWindowAttributes sattr = { .override_redirect = True };
        XChangeWindowAttributes ( display, box, CWOverrideRedirect, &sattr );
    }
    else{
        window_set_atom_prop ( display, box, netatoms[_NET_WM_WINDOW_TYPE],
                               &netatoms[_NET_WM_WINDOW_TYPE_NORMAL], 1 );
    }

    // Set the WM_NAME
    XStoreName ( display, box, "rofi" );

    x11_set_window_opacity ( display, box, config.window_opacity );
    return box;
}

// State of the menu.

typedef struct MenuState
{
    Switcher     *sw;
    unsigned int menu_lines;
    unsigned int max_elements;
    unsigned int max_rows;
    unsigned int columns;

    // window width,height
    unsigned int w, h;
    int          x, y;
    unsigned int element_width;
    int          top_offset;

    // Update/Refilter list.
    int          update;
    int          refilter;
    int          rchanged;
    int          cur_page;

    // Entries
    textbox      *text;
    textbox      *prompt_tb;
    textbox      *message_tb;
    textbox      *case_indicator;
    textbox      **boxes;
    scrollbar    *scrollbar;
    int          *distance;
    unsigned int *line_map;

    unsigned int num_lines;

    // Selected element.
    unsigned int selected;
    unsigned int filtered_lines;
    // Last offset in paginating.
    unsigned int last_offset;

    KeySym       prev_key;
    Time         last_button_press;

    int          quit;
    int          skip_absorb;
    // Return state
    unsigned int *selected_line;
    MenuReturn   retv;
    char         **lines;
    int          line_height;
}MenuState;

/**
 * @param state Internal state of the menu.
 *
 * Free the allocated fields in the state.
 */
static void menu_free_state ( MenuState *state )
{
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
}

/**
 * @param x [out] the calculated x position.
 * @param y [out] the calculated y position.
 * @param mon     the work area.
 * @param h       the required height of the window.
 * @param w       the required width of the window.
 */
static void calculate_window_position ( MenuState *state, const workarea *mon )
{
    // Default location is center.
    state->y = mon->y + ( mon->h - state->h - config.menu_bw * 2 ) / 2;
    state->x = mon->x + ( mon->w - state->w - config.menu_bw * 2 ) / 2;
    // Determine window location
    switch ( config.location )
    {
    case WL_NORTH_WEST:
        state->x = mon->x;
    case WL_NORTH:
        state->y = mon->y;
        break;

    case WL_NORTH_EAST:
        state->y = mon->y;
    case WL_EAST:
        state->x = mon->x + mon->w - state->w - config.menu_bw * 2;
        break;

    case WL_EAST_SOUTH:
        state->x = mon->x + mon->w - state->w - config.menu_bw * 2;
    case WL_SOUTH:
        state->y = mon->y + mon->h - state->h - config.menu_bw * 2;
        break;

    case WL_SOUTH_WEST:
        state->y = mon->y + mon->h - state->h - config.menu_bw * 2;
    case WL_WEST:
        state->x = mon->x;
        break;

    case WL_CENTER:
    default:
        break;
    }
    // Apply offset.
    state->x += config.x_offset;
    state->y += config.y_offset;
}


/**
 * @param state Internal state of the menu.
 * @param num_lines the number of entries passed to the menu.
 *
 * Calculate the number of rows, columns and elements to display based on the
 * configuration and available data.
 */
static void menu_calculate_rows_columns ( MenuState *state )
{
    state->columns      = config.menu_columns;
    state->max_elements = MIN ( state->menu_lines * state->columns, state->num_lines );

    // Calculate the number or rows. We do this by getting the num_lines rounded up to X columns
    // (num elements is better name) then dividing by columns.
    state->max_rows = MIN ( state->menu_lines,
                            (unsigned int) (
                                ( state->num_lines + ( state->columns - state->num_lines % state->columns ) %
                                  state->columns ) / ( state->columns )
                                ) );
    // Always have at least one row.
    state->max_rows = MAX ( 1, state->max_rows );

    if ( config.fixed_num_lines == TRUE ) {
        state->max_elements = state->menu_lines * state->columns;
        state->max_rows     = state->menu_lines;
        // If it would fit in one column, only use one column.
        if ( state->num_lines < state->max_elements ) {
            state->columns = ( state->num_lines + ( state->max_rows - state->num_lines % state->max_rows ) %
                               state->max_rows ) / state->max_rows;
            state->max_elements = state->menu_lines * state->columns;
        }
        // Sanitize.
        if ( state->columns == 0 ) {
            state->columns = 1;
        }
    }
}

/**
 * @param state Internal state of the menu.
 * @param mon the dimensions of the monitor rofi is displayed on.
 *
 * Calculate the width of the window and the width of an element.
 */
static void menu_calculate_window_and_element_width ( MenuState *state, workarea *mon )
{
    if ( config.menu_width < 0 ) {
        double fw = textbox_get_estimated_char_width ( );
        state->w  = -( fw * config.menu_width );
        state->w += 2 * config.padding + 4; // 4 = 2*SIDE_MARGIN
        // Compensate for border width.
        state->w -= config.menu_bw * 2;
    }
    else{
        // Calculate as float to stop silly, big rounding down errors.
        state->w = config.menu_width < 101 ? ( mon->w / 100.0f ) * ( float ) config.menu_width : config.menu_width;
        // Compensate for border width.
        state->w -= config.menu_bw * 2;
    }

    if ( state->columns > 0 ) {
        state->element_width = state->w - ( 2 * ( config.padding ) );
        // Divide by the # columns
        state->element_width = ( state->element_width - ( state->columns - 1 ) * config.line_margin ) / state->columns;
    }
}

/**
 * Nav helper functions, to avoid duplicate code.
 */
/**
 * @param state The current MenuState
 *
 * Move the selection one page down.
 * - No wrap around.
 * - Clip at top/bottom
 */
inline static void menu_nav_page_next ( MenuState *state )
{
    state->selected += ( state->max_elements );
    if ( state->selected >= state->filtered_lines ) {
        state->selected = state->filtered_lines - 1;
    }
    state->update = TRUE;
}
/**
 * @param state The current MenuState
 *
 * Move the selection one page up.
 * - No wrap around.
 * - Clip at top/bottom
 */
inline static void menu_nav_page_prev ( MenuState * state )
{
    if ( state->selected < state->max_elements ) {
        state->selected = 0;
    }
    else{
        state->selected -= ( state->max_elements );
    }
    state->update = TRUE;
}
/**
 * @param state The current MenuState
 *
 * Move the selection one column to the right.
 * - No wrap around.
 * - Do not move to top row when at start.
 */
inline static void menu_nav_right ( MenuState *state )
{
    if ( ( state->selected + state->max_rows ) < state->filtered_lines ) {
        state->selected += state->max_rows;
        state->update    = TRUE;
    }
    else if ( state->selected < ( state->filtered_lines - 1 ) ) {
        // We do not want to move to last item, UNLESS the last column is only
        // partially filled, then we still want to move column and select last entry.
        // First check the column we are currently in.
        int col = state->selected / state->max_rows;
        // Check total number of columns.
        int ncol = state->filtered_lines / state->max_rows;
        // If there is an extra column, move.
        if ( col != ncol ) {
            state->selected = state->filtered_lines - 1;
            state->update   = TRUE;
        }
    }
}
/**
 * @param state The current MenuState
 *
 * Move the selection one column to the left.
 * - No wrap around.
 */
inline static void menu_nav_left ( MenuState *state )
{
    if ( state->selected >= state->max_rows ) {
        state->selected -= state->max_rows;
        state->update    = TRUE;
    }
}
/**
 * @param state The current MenuState
 *
 * Move the selection one row up.
 * - Wrap around.
 */
inline static void menu_nav_up ( MenuState *state )
{
    // Wrap around.
    if ( state->selected == 0 ) {
        state->selected = state->filtered_lines;
    }

    if ( state->selected > 0 ) {
        state->selected--;
    }
    state->update = TRUE;
}
/**
 * @param state The current MenuState
 *
 * Move the selection one row down.
 * - Wrap around.
 */
inline static void menu_nav_down ( MenuState *state )
{
    state->selected = state->selected < state->filtered_lines - 1 ? MIN (
        state->filtered_lines - 1, state->selected + 1 ) : 0;
    state->update = TRUE;
}
/**
 * @param state The current MenuState
 *
 * Move the selection to first row.
 */
inline static void menu_nav_first ( MenuState * state )
{
    state->selected = 0;
    state->update   = TRUE;
}
/**
 * @param state The current MenuState
 *
 * Move the selection to last row.
 */
inline static void menu_nav_last ( MenuState * state )
{
    state->selected = state->filtered_lines - 1;
    state->update   = TRUE;
}
/**
 * @param key the Key to match
 * @param modstate the modifier state to match
 *
 * Match key and modifier state against switchers.
 *
 * @return the index of the switcher that matches the key combination
 * specified by key and modstate. Returns -1 if none was found
 */
extern unsigned int NumlockMask;
static int locate_switcher ( KeySym key, unsigned int modstate )
{
    // ignore annoying modifiers
    unsigned int modstate_filtered = modstate & ~( LockMask | NumlockMask );
    for ( unsigned int i = 0; i < num_switchers; i++ ) {
        if ( switchers[i].sw->keystr != NULL ) {
            if ( ( modstate_filtered == switchers[i].sw->modmask ) &&
                 switchers[i].sw->keysym == key ) {
                return i;
            }
        }
    }
    return -1;
}
/**
 * @param state Internal state of the menu.
 * @param key the Key being pressed.
 * @param modstate the modifier state.
 *
 * Keyboard navigation through the elements.
 */
static void menu_keyboard_navigation ( MenuState *state, KeySym key, unsigned int modstate )
{
    // pressing one of the global key bindings closes the switcher. This allows fast closing of the
    // menu if an item is not selected
    if ( locate_switcher ( key, modstate ) != -1 || abe_test_action ( CANCEL, modstate, key ) ) {
        state->retv = MENU_CANCEL;
        state->quit = TRUE;
    }
    // Up, Ctrl-p or Shift-Tab
    else if ( abe_test_action ( ROW_UP, modstate, key ) ) {
        menu_nav_up ( state );
    }
    else if ( abe_test_action ( ROW_TAB, modstate, key ) ) {
        if ( state->filtered_lines == 1 ) {
            state->retv               = MENU_OK;
            *( state->selected_line ) = state->line_map[state->selected];
            state->quit               = 1;
            return;
        }

        // Double tab!
        if ( state->filtered_lines == 0 && key == state->prev_key ) {
            state->retv               = MENU_NEXT;
            *( state->selected_line ) = 0;
            state->quit               = TRUE;
        }
        else{
            menu_nav_down ( state );
        }
    }
    // Down, Ctrl-n
    else if ( abe_test_action ( ROW_DOWN, modstate, key ) ) {
        menu_nav_down ( state );
    }
    else if ( abe_test_action ( ROW_LEFT, modstate, key ) ) {
        menu_nav_left ( state );
    }
    else if ( abe_test_action ( ROW_RIGHT, modstate, key ) ) {
        menu_nav_right ( state );
    }
    else if ( abe_test_action ( PAGE_PREV, modstate, key ) ) {
        menu_nav_page_prev ( state );
    }
    else if ( abe_test_action ( PAGE_NEXT, modstate, key ) ) {
        menu_nav_page_next ( state );
    }
    else if  ( abe_test_action ( ROW_FIRST, modstate, key ) ) {
        menu_nav_first ( state );
    }
    else if ( abe_test_action ( ROW_LAST, modstate, key ) ) {
        menu_nav_last ( state );
    }
    else if ( abe_test_action ( ROW_SELECT, modstate, key ) ) {
        // If a valid item is selected, return that..
        if ( state->selected < state->filtered_lines ) {
            textbox_text ( state->text, state->lines[state->line_map[state->selected]] );
            textbox_cursor_end ( state->text );
            state->update   = TRUE;
            state->refilter = TRUE;
        }
    }
    state->prev_key = key;
}

/**
 * @param state Internal state of the menu.
 * @param xbe   The mouse button press event.
 *
 * mouse navigation through the elements.
 *
 */
static void menu_mouse_navigation ( MenuState *state, XButtonEvent *xbe )
{
    // Scroll event
    if ( xbe->button > 3 ) {
        if ( xbe->button == 4 ) {
            menu_nav_up ( state );
        }
        else if ( xbe->button == 5 ) {
            menu_nav_down ( state );
        }
        else if ( xbe->button == 6 ) {
            menu_nav_left ( state );
        }
        else if ( xbe->button == 7 ) {
            menu_nav_right ( state );
        }
        return;
    }
    else {
        if ( state->scrollbar && state->scrollbar->window == xbe->window ) {
            state->selected = scrollbar_clicked ( state->scrollbar, xbe->y );
            state->update   = TRUE;
            return;
        }
        for ( unsigned int i = 0; config.sidebar_mode == TRUE && i < num_switchers; i++ ) {
            if ( switchers[i].tb->window == ( xbe->window ) ) {
                *( state->selected_line ) = 0;
                state->retv               = MENU_QUICK_SWITCH | ( i & MENU_LOWER_MASK );
                state->quit               = TRUE;
                state->skip_absorb        = TRUE;
                return;
            }
        }
        for ( unsigned int i = 0; i < state->max_elements; i++ ) {
            if ( ( xbe->window ) == ( state->boxes[i]->window ) ) {
                // Only allow items that are visible to be selected.
                if ( ( state->last_offset + i ) >= state->filtered_lines ) {
                    break;
                }
                //
                state->selected = state->last_offset + i;
                state->update   = TRUE;
                if ( ( xbe->time - state->last_button_press ) < 200 ) {
                    state->retv               = MENU_OK;
                    *( state->selected_line ) = state->line_map[state->selected];
                    // Quit
                    state->quit        = TRUE;
                    state->skip_absorb = TRUE;
                }
                state->last_button_press = xbe->time;
                break;
            }
        }
    }
}

static void menu_refilter ( MenuState *state, char **lines, int sorting, int case_sensitive )
{
    if ( strlen ( state->text->text ) > 0 ) {
        unsigned int j        = 0;
        char         **tokens = tokenize ( state->text->text, case_sensitive );

        // input changed
        for ( unsigned int i = 0; i < state->num_lines; i++ ) {
            int match = state->sw->token_match ( tokens, lines[i], case_sensitive, i, state->sw );

            // If each token was matched, add it to list.
            if ( match ) {
                state->line_map[j] = i;
                if ( sorting ) {
                    state->distance[i] = levenshtein ( state->text->text, lines[i] );
                }
                j++;
            }
        }
        if ( sorting ) {
            g_qsort_with_data ( state->line_map, j, sizeof ( int ), lev_sort, state->distance );
        }

        // Cleanup + bookkeeping.
        state->filtered_lines = j;
        g_strfreev ( tokens );
    }
    else{
        for ( unsigned int i = 0; i < state->num_lines; i++ ) {
            state->line_map[i] = i;
        }
        state->filtered_lines = state->num_lines;
    }
    state->selected = MIN ( state->selected, state->filtered_lines - 1 );

    if ( config.auto_select == TRUE && state->filtered_lines == 1 && state->num_lines > 1 ) {
        *( state->selected_line ) = state->line_map[state->selected];
        state->retv               = MENU_OK;
        state->quit               = TRUE;
    }

    scrollbar_set_max_value ( state->scrollbar, state->filtered_lines );
    state->refilter = FALSE;
    state->rchanged = TRUE;
}


static void menu_draw ( MenuState *state )
{
    unsigned int i, offset = 0;

    // selected row is always visible.
    // If selected is visible do not scroll.
    if ( ( ( state->selected - ( state->last_offset ) ) < ( state->max_elements ) )
         && ( state->selected >= ( state->last_offset ) ) ) {
        offset = state->last_offset;
    }
    else{
        // Do paginating
        int page = ( state->max_elements > 0 ) ? ( state->selected / state->max_elements ) : 0;
        offset             = page * state->max_elements;
        state->last_offset = offset;
        if ( page != state->cur_page ) {
            state->cur_page = page;
            state->rchanged = TRUE;
        }
        // Set the position.
        scrollbar_set_handle ( state->scrollbar, page * state->max_elements );
    }
    // Re calculate the boxes and sizes, see if we can move this in the menu_calc*rowscolumns
    // Get number of remaining lines to display.
    unsigned int a_lines = MIN ( ( state->filtered_lines - offset ), state->max_elements );

    // Calculate number of columns
    unsigned int columns = ( a_lines + ( state->max_rows - a_lines % state->max_rows ) %
                             state->max_rows ) / state->max_rows;
    columns = MIN ( columns, state->columns );

    // Update the handle length.
    scrollbar_set_handle_length ( state->scrollbar, columns * state->max_rows );
    scrollbar_draw ( state->scrollbar );
    // Element width.
    unsigned int element_width = state->w - ( 2 * ( config.padding ) );
    if ( state->scrollbar != NULL ) {
        element_width -= state->scrollbar->w;
    }
    if ( columns > 0 ) {
        element_width = ( element_width - ( columns - 1 ) * config.line_margin ) / columns;
    }

    int          element_height = state->line_height * config.element_height;
    int          y_offset       = state->top_offset;
    int          x_offset       = config.padding;
    // Calculate number of visible rows.
    unsigned int max_elements = MIN ( a_lines, state->max_rows * columns );

    // Hide now invisible boxes.
    for ( i = max_elements; i < state->max_elements; i++ ) {
        textbox_hide ( state->boxes[i] );
    }
    if ( state->rchanged ) {
        // Move, resize visible boxes and show them.
        for ( i = 0; i < max_elements; i++ ) {
            unsigned int ex = ( ( i ) / state->max_rows ) * ( element_width + config.line_margin );
            unsigned int ey = ( ( i ) % state->max_rows ) * ( element_height + config.line_margin );
            // Move it around.
            textbox_moveresize ( state->boxes[i],
                                 ex + x_offset, ey + y_offset,
                                 element_width, element_height );
            {
                TextBoxFontType type   = ( ( ( i % state->max_rows ) & 1 ) == 0 ) ? NORMAL : ALT;
                int             fstate = 0;
                const char      *text  = state->sw->mgrv ( state->line_map[i + offset], state->sw, &fstate );
                TextBoxFontType tbft   = fstate | ( ( i + offset ) == state->selected ? HIGHLIGHT : type );
                textbox_font ( state->boxes[i], tbft );
                textbox_text ( state->boxes[i], text );
            }
            textbox_show ( state->boxes[i] );
            textbox_draw ( state->boxes[i] );
        }
        state->rchanged = FALSE;
    }
    else{
        // Only do basic redrawing + highlight of row.
        for ( i = 0; i < max_elements; i++ ) {
            TextBoxFontType type   = ( ( ( i % state->max_rows ) & 1 ) == 0 ) ? NORMAL : ALT;
            int             fstate = 0;
            state->sw->mgrv ( state->line_map[i + offset], state->sw, &fstate );
            TextBoxFontType tbft = fstate | ( ( i + offset ) == state->selected ? HIGHLIGHT : type );
            textbox_font ( state->boxes[i], tbft );
            textbox_draw ( state->boxes[i] );
        }
    }
}

static void menu_update ( MenuState *state )
{
    textbox_draw ( state->case_indicator );
    textbox_draw ( state->prompt_tb );
    textbox_draw ( state->text );
    if ( state->message_tb ) {
        textbox_draw ( state->message_tb );
    }
    menu_draw ( state );
    // Why do we need the special -1?
    XDrawLine ( display, main_window, gc, 0,
                state->line_height + ( config.padding ) * 1 + config.line_margin + 1,
                state->w,
                state->line_height + ( config.padding ) * 1 + config.line_margin + 1 );
    if ( state->message_tb ) {
        XDrawLine ( display, main_window, gc,
                    0, state->top_offset - ( config.line_margin ) - 1,
                    state->w, state->top_offset - ( config.line_margin ) - 1 );
    }

    if ( config.sidebar_mode == TRUE ) {
        XDrawLine ( display, main_window, gc,
                    0,
                    state->h - state->line_height - ( config.padding ) * 1 - 1 - config.line_margin,
                    state->w,
                    state->h - state->line_height - ( config.padding ) * 1 - 1 - config.line_margin );
        for ( unsigned int j = 0; j < num_switchers; j++ ) {
            textbox_draw ( switchers[j].tb );
        }
    }

    state->update = FALSE;
}

/**
 * @param state Internal state of the menu.
 * @param xse   X selection event.
 *
 * Handle paste event.
 */
static void menu_paste ( MenuState *state, XSelectionEvent *xse )
{
    if ( xse->property == netatoms[UTF8_STRING] ) {
        gchar *text = window_get_text_prop ( display, main_window, netatoms[UTF8_STRING] );
        if ( text != NULL && text[0] != '\0' ) {
            unsigned int dl = strlen ( text );
            // Strip new line
            while ( dl > 0 && text[dl] == '\n' ) {
                text[dl] = '\0';
                dl--;
            }
            // Insert string move cursor.
            textbox_insert ( state->text, state->text->cursor, text );
            textbox_cursor ( state->text, state->text->cursor + dl  );
            // Force a redraw and refiltering of the text.
            state->update   = TRUE;
            state->refilter = TRUE;
        }
        g_free ( text );
    }
}

static void menu_resize ( MenuState *state )
{
    if ( state->scrollbar ) {
        unsigned int sbw = config.line_margin + 8;
        XMoveWindow ( display, state->scrollbar->window, state->w - config.padding - sbw, state->top_offset );
    }
    if ( config.sidebar_mode == TRUE ) {
        int width = ( state->w - ( 2 * ( config.padding ) +
                                   ( num_switchers - 1 ) * config.line_margin ) ) / num_switchers;
        for ( unsigned int j = 0; j < num_switchers; j++ ) {
            textbox_moveresize ( switchers[j].tb,
                                 config.padding + j * ( width + config.line_margin ),
                                 state->h - state->line_height - config.padding,
                                 width, state->line_height );
            textbox_show ( switchers[j].tb );
            textbox_draw ( switchers[j].tb );
        }
    }
    state->rchanged = TRUE;
    state->update   = TRUE;
}

MenuReturn menu ( Switcher *sw, char **input, char *prompt,
                  unsigned int *selected_line,
                  unsigned int *next_pos, const char *message )
{
    int       shift = FALSE;
    MenuState state = {
        .sw                = sw,
        .selected_line     = selected_line,
        .retv              = MENU_CANCEL,
        .prev_key          =             0,
        .last_button_press =             0,
        .last_offset       =             0,
        .distance          = NULL,
        .quit              = FALSE,
        .skip_absorb       = FALSE,
        .filtered_lines    =             0,
        .max_elements      =             0,
        // We want to filter on the first run.
        .refilter   = TRUE,
        .update     = FALSE,
        .rchanged   = TRUE,
        .cur_page   =            -1,
        .top_offset = 0
    };
    // Request the lines to show.
    state.lines = sw->get_data ( &( state.num_lines ), sw );
    if ( next_pos ) {
        *next_pos = *selected_line;
    }
    workarea mon;

    // Try to grab the keyboard as early as possible.
    // We grab this using the rootwindow (as dmenu does it).
    // this seems to result in the smallest delay for most people.
    int has_keyboard = take_keyboard ( display, DefaultRootWindow ( display ) );

    if ( !has_keyboard ) {
        fprintf ( stderr, "Failed to grab keyboard, even after %d uS.", 500 * 1000 );
        // Break off.
        return MENU_CANCEL;
    }
    // main window isn't explicitly destroyed in case we switch modes. Reusing it prevents flicker
    XWindowAttributes attr;
    if ( main_window == None || XGetWindowAttributes ( display, main_window, &attr ) == 0 ) {
        main_window = create_window ( display );
    }
    // Get active monitor size.
    monitor_active ( display, &mon );

    // we need this at this point so we can get height.
    state.line_height    = textbox_get_estimated_char_height ();
    state.case_indicator = textbox_create ( main_window, &vinfo, map, TB_AUTOWIDTH,
                                            ( config.padding ), ( config.padding ),
                                            0, state.line_height,
                                            NORMAL, "*" );
    // Height of a row.
    if ( config.menu_lines == 0 ) {
        // Autosize it.
        int h = mon.h - config.padding * 2 - config.line_margin - config.menu_bw * 2;
        int r = ( h ) / ( state.line_height * config.element_height ) - 1 - config.sidebar_mode;
        state.menu_lines = r;
    }
    else {
        state.menu_lines = config.menu_lines;
    }
    menu_calculate_rows_columns ( &state );
    menu_calculate_window_and_element_width ( &state, &mon );

    // Prompt box.
    state.prompt_tb = textbox_create ( main_window, &vinfo, map, TB_AUTOWIDTH,
                                       ( config.padding ),
                                       ( config.padding ),
                                       0, state.line_height, NORMAL, prompt );
    // Entry box
    int entrybox_width = state.w
                         - ( 2 * ( config.padding ) )
                         - textbox_get_width ( state.prompt_tb )
                         - textbox_get_width ( state.case_indicator );

    state.text = textbox_create ( main_window, &vinfo, map, TB_EDITABLE,
                                  ( config.padding ) + textbox_get_width ( state.prompt_tb ),
                                  ( config.padding ),
                                  entrybox_width, state.line_height,
                                  NORMAL,
                                  *input );

    state.top_offset = config.padding * 1 + state.line_height + 2 + config.line_margin * 2;

    // Move indicator to end.
    textbox_move ( state.case_indicator,
                   config.padding + textbox_get_width ( state.prompt_tb ) + entrybox_width,
                   config.padding );

    textbox_show ( state.text );
    textbox_show ( state.prompt_tb );

    if ( config.case_sensitive ) {
        textbox_show ( state.case_indicator );
    }

    state.message_tb = NULL;
    if ( message ) {
//        state.top_offset += config.line_margin;
        state.message_tb = textbox_create ( main_window, &vinfo, map, TB_AUTOHEIGHT | TB_MARKUP,
                                            ( config.padding ),
                                            state.top_offset,
                                            state.w - ( 2 * ( config.padding ) ),
                                            -1,
                                            NORMAL,
                                            message );
        textbox_show ( state.message_tb );
        state.top_offset += textbox_get_height ( state.message_tb );
        state.top_offset += config.line_margin * 2 + 2;
    }

    int element_height = state.line_height * config.element_height;
    // filtered list display
    state.boxes = g_malloc0_n ( state.max_elements, sizeof ( textbox* ) );

    int y_offset = state.top_offset;
    int x_offset = config.padding;

    for ( unsigned int i = 0; i < state.max_elements; i++ ) {
        state.boxes[i] = textbox_create ( main_window, &vinfo, map, 0,
                                          x_offset, y_offset,
                                          state.element_width, element_height, NORMAL, "" );
    }
    if ( !config.hide_scrollbar ) {
        unsigned int sbw = config.line_margin + 8;
        state.scrollbar = scrollbar_create ( main_window, &vinfo, map,
                                             state.w - config.padding - sbw,
                                             state.top_offset,
                                             sbw,
                                             ( state.max_rows - 1 ) * ( element_height + config.line_margin ) + element_height );
    }


    scrollbar_set_max_value ( state.scrollbar, state.num_lines );
    // filtered list
    state.line_map = g_malloc0_n ( state.num_lines, sizeof ( unsigned int ) );
    if ( config.levenshtein_sort ) {
        state.distance = (int *) g_malloc0_n ( state.num_lines, sizeof ( int ) );
    }

    // resize window vertically to suit
    // Subtract the margin of the last row.
    state.h  = state.top_offset + ( element_height + config.line_margin ) * ( state.max_rows ) - config.line_margin;
    state.h += config.padding;
    state.h += 0;
    // Add entry
    if ( config.sidebar_mode == TRUE ) {
        state.h += state.line_height + 2 * config.line_margin + 2;
    }

    // Sidebar mode.
    if ( config.menu_lines == 0 ) {
        state.h = mon.h - config.menu_bw * 2;
    }

    // Move the window to the correct x,y position.
    calculate_window_position ( &state, &mon );

    if ( config.sidebar_mode == TRUE ) {
        int width = ( state.w - ( 2 * ( config.padding ) + ( num_switchers - 1 ) * config.line_margin ) ) / num_switchers;
        for ( unsigned int j = 0; j < num_switchers; j++ ) {
            switchers[j].tb = textbox_create ( main_window, &vinfo, map, TB_CENTER,
                                               config.padding + j * ( width + config.line_margin ),
                                               state.h - state.line_height - config.padding,
                                               width, state.line_height, ( j == curr_switcher ) ? HIGHLIGHT : NORMAL, switchers[j].sw->name );
            textbox_show ( switchers[j].tb );
        }
    }

    scrollbar_show ( state.scrollbar );
    // Display it.
    XMoveResizeWindow ( display, main_window, state.x, state.y, state.w, state.h );
    XMapRaised ( display, main_window );

    // if grabbing keyboard failed, fall through
    state.selected = 0;

    state.quit = FALSE;
    menu_refilter ( &state, state.lines, config.levenshtein_sort, config.case_sensitive );

    for ( unsigned int i = 0; ( *( state.selected_line ) ) < UINT32_MAX && !state.selected && i < state.filtered_lines; i++ ) {
        if ( state.line_map[i] == *( state.selected_line ) ) {
            state.selected = i;
            break;
        }
    }

    int x11_fd = ConnectionNumber ( display );
    while ( !state.quit ) {
        // Update if requested.
        if ( state.update ) {
            menu_update ( &state );
        }

        // Wait for event.
        XEvent ev;
        // Only use lazy mode above 5000 lines.
        // Or if we still need to get window.
        MainLoopEvent mle = ML_XEVENT;
        // If we are in lazy mode, or trying to grab keyboard, go into timeout.
        // Otherwise continue like we had an XEvent (and we will block on fetching this event).
        if ( ( state.refilter && state.num_lines > config.lazy_filter_limit ) ) {
            mle = wait_for_xevent_or_timeout ( display, x11_fd );
        }
        // If not in lazy mode, refilter.
        if ( state.num_lines <= config.lazy_filter_limit ) {
            if ( state.refilter ) {
                menu_refilter ( &state, state.lines, config.levenshtein_sort, config.case_sensitive );
                menu_update ( &state );
            }
        }
        else if ( mle == ML_TIMEOUT ) {
            // When timeout (and in lazy filter mode)
            // We refilter then loop back and wait for Xevent.
            if ( state.refilter ) {
                menu_refilter ( &state, state.lines, config.levenshtein_sort, config.case_sensitive );
                menu_update ( &state );
            }
        }
        if ( mle == ML_TIMEOUT ) {
            continue;
        }
        // Get next event. (might block)
        XNextEvent ( display, &ev );
        if ( ev.type == KeymapNotify ) {
            XRefreshKeyboardMapping ( &ev.xmapping );
        }
        else if ( ev.type == ConfigureNotify ) {
            XConfigureEvent xce = ev.xconfigure;
            if ( xce.window == main_window ) {
                if ( state.w != (unsigned int) xce.width ) {
                    state.w = xce.width;
                    menu_resize ( &state );
                }
            }
        }
        else if ( ev.type == FocusIn ) {
            take_keyboard ( display, main_window );
        }
        else if ( ev.type == FocusOut ) {
            release_keyboard ( display );
        }
        // Handle event.
        else if ( ev.type == Expose ) {
            while ( XCheckTypedEvent ( display, Expose, &ev ) ) {
                ;
            }
            state.update = TRUE;
        }
        // Button press event.
        else if ( ev.type == ButtonPress ) {
            while ( XCheckTypedEvent ( display, ButtonPress, &ev ) ) {
                ;
            }
            menu_mouse_navigation ( &state, &( ev.xbutton ) );
        }
        // Paste event.
        else if ( ev.type == SelectionNotify ) {
            do {
                menu_paste ( &state, &( ev.xselection ) );
            } while ( XCheckTypedEvent ( display, SelectionNotify, &ev ) );
        }
        // Key press event.
        else if ( ev.type == KeyPress ) {
            do {
                KeySym key = XkbKeycodeToKeysym ( display, ev.xkey.keycode, 0, 0 );

                // Handling of paste
                if ( abe_test_action ( PASTE_PRIMARY, ev.xkey.state, key ) ) {
                    XConvertSelection ( display, XA_PRIMARY,
                                        netatoms[UTF8_STRING], netatoms[UTF8_STRING], main_window, CurrentTime );
                }
                else if ( abe_test_action ( PASTE_SECONDARY, ev.xkey.state, key ) ) {
                    XConvertSelection ( display, netatoms[CLIPBOARD],
                                        netatoms[UTF8_STRING], netatoms[UTF8_STRING], main_window, CurrentTime );
                }
                else if ( abe_test_action ( MODE_PREVIOUS, ev.xkey.state, key ) ) {
                    state.retv               = MENU_PREVIOUS;
                    *( state.selected_line ) = 0;
                    state.quit               = TRUE;
                    break;
                }
                // Menu navigation.
                else if ( abe_test_action ( MODE_NEXT, ev.xkey.state, key ) ) {
                    state.retv               = MENU_NEXT;
                    *( state.selected_line ) = 0;
                    state.quit               = TRUE;
                    break;
                }
                // Toggle case sensitivity.
                else if ( abe_test_action ( TOGGLE_CASE_SENSITIVITY, ev.xkey.state, key ) ) {
                    config.case_sensitive    = !config.case_sensitive;
                    *( state.selected_line ) = 0;
                    state.refilter           = TRUE;
                    state.update             = TRUE;
                    if ( config.case_sensitive ) {
                        textbox_show ( state.case_indicator );
                    }
                    else {
                        textbox_hide ( state.case_indicator );
                    }
                }
                // Special delete entry command.
                else if ( abe_test_action ( DELETE_ENTRY, ev.xkey.state, key ) ) {
                    if ( state.selected < state.filtered_lines ) {
                        *( state.selected_line ) = state.line_map[state.selected];
                        state.retv               = MENU_ENTRY_DELETE;
                        state.quit               = TRUE;
                        break;
                    }
                }
                else{
                    for ( unsigned int a = CUSTOM_1; a <= CUSTOM_19; a++ ) {
                        if ( abe_test_action ( a, ev.xkey.state, key ) ) {
                            if ( state.selected < state.filtered_lines ) {
                                *( state.selected_line ) = state.line_map[state.selected];
                            }
                            state.retv = MENU_QUICK_SWITCH | ( ( a - CUSTOM_1 ) & MENU_LOWER_MASK );
                            state.quit = TRUE;
                            break;
                        }
                    }
                    // Skip if we detected key before.
                    if ( state.quit ) {
                        continue;
                    }

                    int rc = textbox_keypress ( state.text, &ev );
                    // Row is accepted.
                    if ( rc < 0 ) {
                        shift = ( ( ev.xkey.state & ShiftMask ) == ShiftMask );

                        // If a valid item is selected, return that..
                        if ( state.selected < state.filtered_lines ) {
                            *( state.selected_line ) = state.line_map[state.selected];
                            if ( strlen ( state.text->text ) > 0 && rc == -2 ) {
                                state.retv = MENU_CUSTOM_INPUT;
                            }
                            else {
                                state.retv = MENU_OK;
                            }
                        }
                        else if ( strlen ( state.text->text ) > 0 ) {
                            state.retv = MENU_CUSTOM_INPUT;
                        }
                        else{
                            // Nothing entered and nothing selected.
                            state.retv = MENU_CANCEL;
                        }

                        state.quit = TRUE;
                    }
                    // Key press is handled by entry box.
                    else if ( rc == 1 ) {
                        state.refilter = TRUE;
                        state.update   = TRUE;
                    }
                    else if (  rc == 2 ) {
                        // redraw.
                        state.update = TRUE;
                    }
                    // Other keys.
                    else{
                        // unhandled key
                        menu_keyboard_navigation ( &state, key, ev.xkey.state );
                    }
                }
            } while ( XCheckTypedEvent ( display, KeyPress, &ev ) );
        }
    }
    // Wait for final release?
    if ( !state.skip_absorb ) {
        XEvent ev;
        do {
            XNextEvent ( display, &ev );
        } while ( ev.type != KeyRelease );
    }


    // Update input string.
    g_free ( *input );
    *input = g_strdup ( state.text->text );

    if ( next_pos ) {
        *( next_pos ) = state.selected + 1;
    }

    int retv = state.retv;
    menu_free_state ( &state );

    if ( shift ) {
        retv |= MENU_SHIFT;
    }

    // Free the switcher boxes.
    // When state is free'ed we should no longer need these.
    if ( config.sidebar_mode == TRUE ) {
        for ( unsigned int j = 0; j < num_switchers; j++ ) {
            textbox_free ( switchers[j].tb );
            switchers[j].tb = NULL;
        }
    }

    return retv;
}

void error_dialog ( const char *msg, int markup )
{
    MenuState state = {
        .sw                = NULL,
        .selected_line     = NULL,
        .retv              = MENU_CANCEL,
        .prev_key          =           0,
        .last_button_press =           0,
        .last_offset       =           0,
        .num_lines         =           0,
        .distance          = NULL,
        .quit              = FALSE,
        .skip_absorb       = FALSE,
        .filtered_lines    =           0,
        .columns           =           0,
        .update            = TRUE,
    };
    workarea  mon;

    // Try to grab the keyboard as early as possible.
    // We grab this using the rootwindow (as dmenu does it).
    // this seems to result in the smallest delay for most people.
    int has_keyboard = take_keyboard ( display, DefaultRootWindow ( display ) );

    if ( !has_keyboard ) {
        fprintf ( stderr, "Failed to grab keyboard, even after %d uS.", 500 * 1000 );
        return;
    }
    // Get active monitor size.
    monitor_active ( display, &mon );
    // main window isn't explicitly destroyed in case we switch modes. Reusing it prevents flicker
    XWindowAttributes attr;
    if ( main_window == None || XGetWindowAttributes ( display, main_window, &attr ) == 0 ) {
        main_window = create_window ( display );
    }


    menu_calculate_window_and_element_width ( &state, &mon );
    state.max_elements = 0;

    state.text = textbox_create ( main_window, &vinfo, map, TB_AUTOHEIGHT + ( ( markup ) ? TB_MARKUP : 0 ),
                                  ( config.padding ),
                                  ( config.padding ),
                                  ( state.w - ( 2 * ( config.padding ) ) ),
                                  1,
                                  NORMAL,
                                  ( msg != NULL ) ? msg : "" );
    textbox_show ( state.text );
    state.line_height = textbox_get_height ( state.text );

    // resize window vertically to suit
    state.h = state.line_height + ( config.padding ) * 2;

    // Move the window to the correct x,y position.
    calculate_window_position ( &state, &mon );
    XMoveResizeWindow ( display, main_window, state.x, state.y, state.w, state.h );

    // Display it.
    XMapRaised ( display, main_window );

    while ( !state.quit ) {
        // Update if requested.
        if ( state.update ) {
            textbox_draw ( state.text );
            state.update = FALSE;
        }
        // Wait for event.
        XEvent ev;
        XNextEvent ( display, &ev );
        // Handle event.
        if ( ev.type == Expose ) {
            while ( XCheckTypedEvent ( display, Expose, &ev ) ) {
                ;
            }
            state.update = TRUE;
        }
        // Key press event.
        else if ( ev.type == KeyPress ) {
            while ( XCheckTypedEvent ( display, KeyPress, &ev ) ) {
                ;
            }
            state.quit = TRUE;
        }
    }
    release_keyboard ( display );
    menu_free_state ( &state );
}

/**
 * Do needed steps to start showing the gui
 */
static int setup ()
{
    // Create pid file
    int pfd = create_pid_file ( pidfile );
    if ( pfd >= 0 ) {
        // Request truecolor visual.
        create_visual_and_colormap ( display );
        textbox_setup ( &vinfo, map );
    }
    return pfd;
}
/**
 * Teardown the gui.
 */
static void teardown ( int pfd )
{
    // Cleanup font setup.
    textbox_cleanup ( );

    // Release the window.
    release_keyboard ( display );
    if ( main_window != None ) {
        XUnmapWindow ( display, main_window );
        XDestroyWindow ( display, main_window );
        main_window = None;
    }
    if ( gc != NULL ) {
        XFreeGC ( display, gc );
        gc = NULL;
    }
    if ( map != None ) {
        XFreeColormap ( display, map );
        map = None;
    }
    // Cleanup pid file.
    remove_pid_file ( pfd );
}

/**
 * Start dmenu mode.
 */
static int run_dmenu ()
{
    int ret_state = EXIT_FAILURE;
    int pfd       = setup ();
    if ( pfd < 0 ) {
        return ret_state;
    }

    // Dmenu modi has a return state.
    ret_state = dmenu_switcher_dialog ();

    teardown ( pfd );
    return ret_state;
}

static void run_switcher ( SwitcherMode mode )
{
    int pfd = setup ();
    if ( pfd < 0 ) {
        return;
    }
    // Otherwise check if requested mode is enabled.
    char *input = g_strdup ( config.filter );
    for ( unsigned int i = 0; i < num_switchers; i++ ) {
        switchers[i].sw->init ( switchers[i].sw );
    }
    do {
        SwitcherMode retv;

        curr_switcher = mode;
        retv          = switcher_run ( &input, switchers[mode].sw );
        // Find next enabled
        if ( retv == NEXT_DIALOG ) {
            mode = ( mode + 1 ) % num_switchers;
        }
        else if ( retv == PREVIOUS_DIALOG ) {
            if ( mode == 0 ) {
                mode = num_switchers - 1;
            }
            else {
                mode = ( mode - 1 ) % num_switchers;
            }
        }
        else if ( retv == RELOAD_DIALOG ) {
            // do nothing.
        }
        else if ( retv < MODE_EXIT ) {
            mode = ( retv ) % num_switchers;
        }
        else {
            mode = retv;
        }
    } while ( mode != MODE_EXIT );
    g_free ( input );
    for ( unsigned int i = 0; i < num_switchers; i++ ) {
        switchers[i].sw->destroy ( switchers[i].sw );
    }
    // cleanup
    teardown ( pfd );
}

int show_error_message ( const char *msg, int markup )
{
    int pfd = setup ();
    if ( pfd < 0 ) {
        return EXIT_FAILURE;
    }
    error_dialog ( msg, markup );
    teardown ( pfd );
    return EXIT_SUCCESS;
}

/**
 * Function that listens for global key-presses.
 * This is only used when in daemon mode.
 */
static void handle_keypress ( XEvent *ev )
{
    int    index;
    KeySym key = XkbKeycodeToKeysym ( display, ev->xkey.keycode, 0, 0 );
    index = locate_switcher ( key, ev->xkey.state );
    if ( index >= 0 ) {
        run_switcher ( index );
    }
    else {
        fprintf ( stderr, "Warning: Unhandled keypress in global keyhandler, keycode = %u mask = %u\n", ev->xkey.keycode, ev->xkey.state );
    }
}


/**
 * Help function. This calls man.
 */
static void help ()
{
    int code = execlp ( "man", "man", "-M", MANPAGE_PATH, "rofi", NULL );

    if ( code == -1 ) {
        fprintf ( stderr, "Failed to execute manpage viewer: %s\n", strerror ( errno ) );
    }
}


/**
 * Function bound by 'atexit'.
 * Cleanup globally allocated memory.
 */
static void cleanup ()
{
    // Cleanup
    if ( display != NULL ) {
        if ( main_window != None ) {
            // We should never hit this code.
            release_keyboard ( display );
            XDestroyWindow ( display, main_window );
            main_window = None;
        }
        if ( gc != NULL ) {
            XFreeGC ( display, gc );
            gc = NULL;
        }
        XCloseDisplay ( display );
        display = NULL;
    }


    // Cleaning up memory allocated by the Xresources file.
    config_xresource_free ();
    for ( unsigned int i = 0; i < num_switchers; i++ ) {
        // Switcher keystr is free'ed when needed by config system.
        if ( switchers[i].sw->keycfg != NULL ) {
            g_free ( switchers[i].sw->keycfg );
            switchers[i].sw->keycfg = NULL;
        }
        // only used for script dialog.
        if ( switchers[i].sw->free != NULL ) {
            switchers[i].sw->free ( switchers[i].sw );
        }
    }
    g_free ( switchers );

    // Cleanup the custom keybinding
    cleanup_abe ();
}

/**
 * Parse the switcher string, into internal array of type Switcher.
 *
 * String is split on separator ','
 * First the three build-in modi are checked: window, run, ssh
 * if that fails, a script-switcher is created.
 */
static void setup_switchers ( void )
{
    char *savept = NULL;
    // Make a copy, as strtok will modify it.
    char *switcher_str = g_strdup ( config.switchers );
    // Split token on ','. This modifies switcher_str.
    for ( char *token = strtok_r ( switcher_str, ",", &savept );
          token != NULL;
          token = strtok_r ( NULL, ",", &savept ) ) {
        // Resize and add entry.
        switchers                   = (Mode *) g_realloc ( switchers, sizeof ( Mode ) * ( num_switchers + 1 ) );
        switchers[num_switchers].tb = NULL;

        // Window switcher.
        if ( strcasecmp ( token, "window" ) == 0 ) {
            switchers[num_switchers].sw = &window_mode;
            num_switchers++;
        }
        else if ( strcasecmp ( token, "windowcd" ) == 0 ) {
            switchers[num_switchers].sw = &window_mode_cd;
            num_switchers++;
        }
        // SSh dialog
        else if ( strcasecmp ( token, "ssh" ) == 0 ) {
            switchers[num_switchers].sw = &ssh_mode;
            num_switchers++;
        }
        // Run dialog
        else if ( strcasecmp ( token, "run" ) == 0 ) {
            switchers[num_switchers].sw = &run_mode;
            num_switchers++;
        }
        // combi dialog
        else if ( strcasecmp ( token, "combi" ) == 0 ) {
            switchers[num_switchers].sw = &combi_mode;
            num_switchers++;
        }
        else {
            // If not build in, use custom switchers.
            Switcher *sw = script_switcher_parse_setup ( token );
            if ( sw != NULL ) {
                switchers[num_switchers].sw = sw;
                num_switchers++;
            }
            else{
                // Report error, don't continue.
                fprintf ( stderr, "Invalid script switcher: %s\n", token );
                token = NULL;
            }
        }
        // Keybinding.
    }
    // Free string that was modified by strtok_r
    g_free ( switcher_str );
    // We cannot do this in main loop, as we create pointer to string,
    // and re-alloc moves that pointer.
    for ( unsigned int i = 0; i < num_switchers; i++ ) {
        switchers[i].sw->keycfg = g_strdup_printf ( "key-%s", switchers[i].sw->name );
        config_parser_add_option ( xrm_String,
                                   switchers[i].sw->keycfg,
                                   (void * *) &( switchers[i].sw->keystr ) );
    }
}

/**
 * @param display Pointer to the X connection to use.
 * Load configuration.
 * Following priority: (current), X, commandline arguments
 */
static inline void load_configuration ( Display *display )
{
    // Load in config from X resources.
    config_parse_xresource_options ( display );

    // Parse command line for settings.
    config_parse_cmd_options ( );
}
static inline void load_configuration_dynamic ( Display *display )
{
    // Load in config from X resources.
    config_parse_xresource_options_dynamic ( display );
    config_parse_cmd_options_dynamic (  );
}


static void release_global_keybindings ()
{
    for ( unsigned int i = 0; i < num_switchers; i++ ) {
        if ( switchers[i].sw->keystr != NULL ) {
            // No need to parse key, this should be done when grabbing.
            if ( switchers[i].sw->keysym != NoSymbol ) {
                x11_ungrab_key ( display, switchers[i].sw->modmask, switchers[i].sw->keysym );
            }
        }
    }
}
static int grab_global_keybindings ()
{
    int key_bound = FALSE;
    for ( unsigned int i = 0; i < num_switchers; i++ ) {
        if ( switchers[i].sw->keystr != NULL ) {
            x11_parse_key ( switchers[i].sw->keystr, &( switchers[i].sw->modmask ), &( switchers[i].sw->keysym ) );
            if ( switchers[i].sw->keysym != NoSymbol ) {
                x11_grab_key ( display, switchers[i].sw->modmask, switchers[i].sw->keysym );
                key_bound = TRUE;
            }
        }
    }
    return key_bound;
}
static void print_global_keybindings ()
{
    fprintf ( stdout, "listening to the following keys:\n" );
    for ( unsigned int i = 0; i < num_switchers; i++ ) {
        if ( switchers[i].sw->keystr != NULL ) {
            fprintf ( stdout, "\t* "color_bold "%s"color_reset " on %s\n",
                      switchers[i].sw->name, switchers[i].sw->keystr );
        }
        else {
            fprintf ( stdout, "\t* "color_bold "%s"color_reset " on <unspecified>\n",
                      switchers[i].sw->name );
        }
    }
}

static void reload_configuration ()
{
    if ( find_arg ( "-no-config" ) < 0 ) {
        // We need to open a new connection to X11, otherwise we get old
        // configuration
        Display *temp_display = XOpenDisplay ( display_str );
        if ( temp_display ) {
            load_configuration ( temp_display );
            load_configuration_dynamic ( temp_display );

            // Sanity check
            config_sanity_check ( );
            parse_keys_abe ();
            XCloseDisplay ( temp_display );
        }
        else {
            fprintf ( stderr, "Failed to get a new connection to the X11 server. No point in continuing.\n" );
            abort ();
        }
    }
}

/**
 * Separate thread that handles signals being send to rofi.
 * Currently it listens for three signals:
 *  * HUP: reload the configuration.
 *  * INT: Quit the program.
 *  * USR1: Dump the current configuration to stdout.
 *
 *  These messages are relayed to the main thread via a pipe, the write side of the pipe is passed
 *  as argument to this thread.
 *  When receiving a sig-int the thread quits.
 */
static gpointer rofi_signal_handler_process ( gpointer arg )
{
    int pfd = *( (int *) arg );

    // Create same mask again.
    sigset_t set;
    sigemptyset ( &set );
    sigaddset ( &set, SIGHUP );
    sigaddset ( &set, SIGINT );
    sigaddset ( &set, SIGUSR1 );
    sigaddset ( &set, SIGCHLD );
    // loop forever.
    while ( 1 ) {
        siginfo_t info;
        int       sig = sigwaitinfo ( &set, &info );
        if ( sig < 0 ) {
            perror ( "sigwaitinfo failed, lets restart" );
        }
        else {
            // Send message to main thread.
            if ( sig == SIGHUP ) {
                write ( pfd, "c", 1 );
            }
            if ( sig == SIGUSR1 ) {
                write ( pfd, "i", 1 );
            }
            else if ( sig == SIGINT ) {
                write ( pfd, "q", 1 );
                // Close my end and exit.
                g_thread_exit ( NULL );
            }
            else if ( sig == SIGCHLD ) {
                while ( 0 < waitpid ( -1, NULL, WNOHANG ) ) {
                    ;
                }
            }
        }
    }
}

/**
 * Process X11 events in the main-loop (gui-thread) of the application.
 */
static void main_loop_x11_event_handler ( void )
{
    // X11 produced an event. Consume them.
    while ( XPending ( display ) ) {
        XEvent ev;
        // Read event, we know this won't block as we checked with XPending.
        XNextEvent ( display, &ev );
        // If we get an event that does not belong to a window:
        // Ignore it.
        if ( ev.xany.window == None ) {
            continue;
        }
        // If keypress, handle it.
        if ( ev.type == KeyPress ) {
            handle_keypress ( &ev );
        }
    }
}

/**
 * Process signals in the main-loop (gui-thread) of the application.
 *
 * returns TRUE when mainloop should be stopped.
 */
static int main_loop_signal_handler ( char command, int quiet )
{
    if ( command == 'c' ) {
        if ( !quiet ) {
            fprintf ( stdout, "Reload configuration\n" );
        }
        // Release the keybindings.
        release_global_keybindings ();
        // Reload config
        reload_configuration ();
        // Grab the possibly new keybindings.
        grab_global_keybindings ();
        if ( !quiet ) {
            print_global_keybindings ();
        }
        // We need to flush, otherwise the first key presses are not caught.
        XFlush ( display );
    }
    // Got message to quit.
    else if ( command == 'q' ) {
        // Break out of loop.
        return TRUE;
    }
    // Got message to print info
    else if ( command == 'i' ) {
        xresource_dump ();
    }
    return FALSE;
}


/**
 * Setup signal handling.
 * Block all the signals, start a signal processor thread to handle these events.
 */
static GThread *setup_signal_thread ( int *fd )
{
    // Block all HUP,INT,USR1 signals.
    // In this and other child (they inherit mask)
    sigset_t set;
    sigemptyset ( &set );
    sigaddset ( &set, SIGHUP );
    sigaddset ( &set, SIGINT );
    sigaddset ( &set, SIGUSR1 );
    sigaddset ( &set, SIGCHLD );
    sigprocmask ( SIG_BLOCK, &set, NULL );
    // Create signal handler process.
    // This will use sigwaitinfo to read signals and forward them back to the main thread again.
    return g_thread_new ( "signal_process", rofi_signal_handler_process, (void *) fd );
}

int main ( int argc, char *argv[] )
{
    cmd_set_arguments ( argc, argv );
    // Quiet flag
    int quiet = ( find_arg ( "-quiet" ) >= 0 );
    // catch help request
    if ( find_arg (  "-h" ) >= 0 || find_arg (  "-help" ) >= 0 || find_arg (  "--help" ) >= 0 ) {
        help ();
        exit ( EXIT_SUCCESS );
    }
    // Version
    if ( find_arg (  "-v" ) >= 0 || find_arg (  "-version" ) >= 0 ) {
        fprintf ( stdout, "Version: "VERSION "\n" );
        exit ( EXIT_SUCCESS );
    }

    // Detect if we are in dmenu mode.
    // This has two possible causes.
    // 1 the user specifies it on the command-line.
    int dmenu_mode = FALSE;
    if ( find_arg (  "-dmenu" ) >= 0 ) {
        dmenu_mode = TRUE;
    }
    // 2 the binary that executed is called dmenu (e.g. symlink to rofi)
    else{
        // Get the base name of the executable called.
        char *base_name = g_path_get_basename ( argv[0] );
        dmenu_mode = ( strcmp ( base_name, "dmenu" ) == 0 );
        // Free the basename for dmenu detection.
        g_free ( base_name );
    }

    // Get the path to the cache dir.
    cache_dir = g_get_user_cache_dir ();

    // Create pid file path.
    const char *path = g_get_user_runtime_dir ();
    if ( path ) {
        pidfile = g_build_filename ( path, "rofi.pid", NULL );
    }
    config_parser_add_option ( xrm_String, "pid", (void * *) &pidfile );

    // Register cleanup function.
    atexit ( cleanup );

    // Get DISPLAY, first env, then argument.
    display_str = getenv ( "DISPLAY" );
    find_arg_str (  "-display", &display_str );

    if ( !XSupportsLocale () ) {
        fprintf ( stderr, "X11 does not support locales\n" );
        return 11;
    }
    if ( XSetLocaleModifiers ( "@im=none" ) == NULL ) {
        fprintf ( stderr, "Failed to set locale modifier.\n" );
        return 10;
    }
    if ( !( display = XOpenDisplay ( display_str ) ) ) {
        fprintf ( stderr, "cannot open display!\n" );
        return EXIT_FAILURE;
    }

    // Setup keybinding
    setup_abe ();

    if ( find_arg ( "-no-config" ) < 0 ) {
        load_configuration ( display );
    }
    if ( !dmenu_mode ) {
        // setup_switchers
        setup_switchers ();
    }
    else {
        // Add dmenu options.
        config_parser_add_option ( xrm_Char, "sep", (void * *) &( config.separator ) );
    }
    if ( find_arg ( "-no-config" ) < 0 ) {
        // Reload for dynamic part.
        load_configuration_dynamic ( display );
    }

    x11_setup ( display );

    // Sanity check
    config_sanity_check ( );
    // Dump.
    if ( find_arg (  "-dump-xresources" ) >= 0 ) {
        xresource_dump ();
        exit ( EXIT_SUCCESS );
    }
    // Parse the keybindings.
    parse_keys_abe ();

    char *msg = NULL;
    if ( find_arg_str (  "-e", &( msg ) ) ) {
        int markup = FALSE;
        if ( find_arg ( "-markup" ) >= 0 ) {
            markup = TRUE;
        }
        return show_error_message ( msg, markup );
    }

    // Dmenu mode.
    if ( dmenu_mode == TRUE ) {
        normal_window_mode = find_arg ( "-normal-window" ) >= 0;
        // force off sidebar mode:
        config.sidebar_mode = FALSE;
        int retv = run_dmenu ();

        // User canceled the operation.
        if ( retv == FALSE ) {
            return EXIT_FAILURE;
        }
        else if ( retv >= 10 ) {
            return retv;
        }
        return EXIT_SUCCESS;
    }

    // flags to run immediately and exit
    char *sname = NULL;
    if ( find_arg_str ( "-show", &sname ) == TRUE ) {
        int index = switcher_get ( sname );
        if ( index >= 0 ) {
            run_switcher ( index );
        }
        else {
            fprintf ( stderr, "The %s switcher has not been enabled\n", sname );
        }
    }
    else{
        // Daemon mode, Listen to key presses..
        if ( !grab_global_keybindings () ) {
            fprintf ( stderr, "Rofi was launched in daemon mode, but no key-binding was specified.\n" );
            fprintf ( stderr, "Please check the manpage on how to specify a key-binding.\n" );
            fprintf ( stderr, "The following modi are enabled and keys can be specified:\n" );
            for ( unsigned int i = 0; i < num_switchers; i++ ) {
                fprintf ( stderr, "\t* "color_bold "%s"color_reset ": -key-%s <key>\n",
                          switchers[i].sw->name, switchers[i].sw->name );
            }
            return EXIT_FAILURE;
        }
        if ( !quiet ) {
            fprintf ( stdout, "Rofi is launched in daemon mode.\n" );
            print_global_keybindings ();
        }


        // Create a pipe to communicate between signal thread an main thread.
        int pfds[2];
        if ( pipe ( pfds ) != 0 ) {
            char * msg = g_strdup_printf ( "Failed to start rofi: '<i>%s</i>'", strerror ( errno ) );
            show_error_message ( msg, TRUE );
            g_free ( msg );
            exit ( EXIT_FAILURE );
        }
        GThread *pid_signal_proc = setup_signal_thread ( &( pfds[1] ) );

        // Application Main loop.
        // This listens in the background for any events on the Xserver
        // catching global key presses.
        // It also listens from messages from the signal process.
        XSelectInput ( display, DefaultRootWindow ( display ), KeyPressMask );
        XFlush ( display );
        int x11_fd = ConnectionNumber ( display );
        for (;; ) {
            fd_set in_fds;
            // Create a File Description Set containing x11_fd, and signal pipe
            FD_ZERO ( &in_fds );
            FD_SET ( x11_fd, &in_fds );
            FD_SET ( pfds[0], &in_fds );

            // Wait for X Event or a message on signal pipe
            if ( select ( MAX ( x11_fd, pfds[0] ) + 1, &in_fds, 0, 0, NULL ) >= 0 ) {
                // X11
                if ( FD_ISSET ( x11_fd, &in_fds ) ) {
                    main_loop_x11_event_handler ();
                }
                // Signal Pipe
                if ( FD_ISSET ( pfds[0], &in_fds ) ) {
                    // The signal thread send us a message. Process it.
                    char c;
                    read ( pfds[0], &c, 1 );
                    // Process the signal in the main_loop.
                    if ( main_loop_signal_handler ( c, quiet ) ) {
                        break;
                    }
                }
            }
        }

        release_global_keybindings ();
        // Join the signal process thread. (at this point it should have exited).
        // this also unrefs (de-allocs) the GThread object.
        g_thread_join ( pid_signal_proc );
        // Close pipe
        close ( pfds[0] );
        close ( pfds[1] );
        if ( !quiet ) {
            fprintf ( stdout, "Quit from daemon mode.\n" );
        }
    }

    return EXIT_SUCCESS;
}


SwitcherMode switcher_run ( char **input, Switcher *sw )
{
    char         *prompt       = g_strdup_printf ( "%s:", sw->name );
    unsigned int selected_line = UINT32_MAX;
    int          mretv         = menu ( sw, input, prompt, &selected_line, NULL, NULL );

    g_free ( prompt );
    return sw->result ( mretv, input, selected_line, sw );
}
