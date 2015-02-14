/**
 * rofi
 *
 * MIT/X11 License
 * Copyright (c) 2012 Sean Pringle <sean.pringle@gmail.com>
 * Modified 2013-2015 Qball  Cow <qball@gmpclient.org>
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
#include "x11-helper.h"
#include "xrmoptions.h"
// Switchers.
#include "dialogs/run-dialog.h"
#include "dialogs/ssh-dialog.h"
#include "dialogs/dmenu-dialog.h"
#include "dialogs/script-dialog.h"
#include "dialogs/window-dialog.h"

#define LINE_MARGIN    3

typedef enum _MainLoopEvent
{
    ML_XEVENT,
    ML_TIMEOUT
} MainLoopEvent;

// Pidfile.
char         *pidfile         = NULL;
const char   *cache_dir       = NULL;
Display      *display         = NULL;
char         *display_str     = NULL;

const char   *netatom_names[] = { EWMH_ATOMS ( ATOM_CHAR ) };
Atom         netatoms[NUM_NETATOMS];

unsigned int windows_modmask, rundialog_modmask, sshdialog_modmask;
KeySym       rundialog_keysym, sshdialog_keysym, windows_keysym;


/**
 * Structure defining a switcher.
 * It consists of a name, callback and if enabled
 * a textbox for the sidebar-mode.
 */
typedef struct _Switcher
{
    // Name (max 31 char long)
    char                        name[32];
    // Switcher callback.
    switcher_callback           cb;
    // Callback data.
    void                        *cb_data;
    switcher_callback_free_data cb_data_free;
    // Textbox used in the sidebar-mode.
    textbox                     *tb;
} Switcher;

// Array of switchers.
Switcher     *switchers = NULL;
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
        if ( strcmp ( switchers[i].name, name ) == 0 ) {
            return i;
        }
    }
    return -1;
}

void catch_exit ( __attribute__( ( unused ) ) int sig )
{
    while ( 0 < waitpid ( -1, NULL, WNOHANG ) ) {
        ;
    }
}

Window      main_window = None;
GC          gc          = NULL;
Colormap    map         = None;
XVisualInfo vinfo;

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

static void menu_hide_arrow_text ( int filtered_lines, int selected, int max_elements,
                                   textbox *arrowbox_top, textbox *arrowbox_bottom )
{
    if ( arrowbox_top == NULL || arrowbox_bottom == NULL ) {
        return;
    }
    int page   = ( filtered_lines > 0 ) ? selected / max_elements : 0;
    int npages = ( filtered_lines > 0 ) ? ( ( filtered_lines + max_elements - 1 ) / max_elements ) : 1;
    if ( !( page != 0 && npages > 1 ) ) {
        textbox_hide ( arrowbox_top );
    }
    if ( !( ( npages - 1 ) != page && npages > 1 ) ) {
        textbox_hide ( arrowbox_bottom );
    }
}

static void menu_set_arrow_text ( int filtered_lines, int selected, int max_elements,
                                  textbox *arrowbox_top, textbox *arrowbox_bottom )
{
    if ( arrowbox_top == NULL || arrowbox_bottom == NULL ) {
        return;
    }
    if ( filtered_lines == 0 || max_elements == 0 ) {
        return;
    }
    int page   = ( filtered_lines > 0 ) ? selected / max_elements : 0;
    int npages = ( filtered_lines > 0 ) ? ( ( filtered_lines + max_elements - 1 ) / max_elements ) : 1;
    int entry  = selected % max_elements;
    if ( page != 0 && npages > 1 ) {
        textbox_show ( arrowbox_top );
        textbox_font ( arrowbox_top, ( entry != 0 ) ? NORMAL : HIGHLIGHT );
        textbox_draw ( arrowbox_top );
    }
    if ( ( npages - 1 ) != page && npages > 1 ) {
        textbox_show ( arrowbox_bottom );
        textbox_font ( arrowbox_bottom, ( entry != ( max_elements - 1 ) ) ? NORMAL : HIGHLIGHT );
        textbox_draw ( arrowbox_bottom );
    }
}



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

Window create_window ( Display *display )
{
    XSetWindowAttributes attr;
    attr.colormap         = map;
    attr.border_pixel     = color_get ( display, config.menu_bc );
    attr.background_pixel = color_get ( display, config.menu_bg );

    Window box = XCreateWindow ( display, DefaultRootWindow ( display ),
                                 0, 0, 200, 100, config.menu_bw, vinfo.depth, InputOutput,
                                 vinfo.visual, CWColormap | CWBorderPixel | CWBackPixel, &attr );
    XSelectInput ( display, box, ExposureMask | ButtonPressMask );

    gc = XCreateGC ( display, box, 0, 0 );
    XSetLineAttributes ( display, gc, 2, LineOnOffDash, CapButt, JoinMiter );
    XSetForeground ( display, gc, color_get ( display, config.menu_bc ) );
    // make it an unmanaged window
    window_set_atom_prop ( display, box, netatoms[_NET_WM_STATE], &netatoms[_NET_WM_STATE_ABOVE], 1 );
    XSetWindowAttributes sattr;
    sattr.override_redirect = True;
    XChangeWindowAttributes ( display, box, CWOverrideRedirect, &sattr );

    // Set the WM_NAME
    XStoreName ( display, box, "rofi" );

    x11_set_window_opacity ( display, box, config.window_opacity );
    return box;
}

// State of the menu.

typedef struct MenuState
{
    unsigned int menu_lines;
    unsigned int max_elements;
    unsigned int max_rows;
    unsigned int columns;

    // window width,height
    unsigned int w, h;
    int          x, y;
    unsigned int element_width;

    // Update/Refilter list.
    int          update;
    int          refilter;

    // Entries
    textbox      *text;
    textbox      *prompt_tb;
    textbox      *case_indicator;
    textbox      *arrowbox_top;
    textbox      *arrowbox_bottom;
    textbox      **boxes;
    char         **filtered;
    int          *distance;
    int          *line_map;

    unsigned int num_lines;

    // Selected element.
    unsigned int selected;
    unsigned int filtered_lines;
    // Last offset in paginating.
    unsigned int last_offset;

    KeySym       prev_key;
    Time         last_button_press;

    int          quit;
    int          init;
    // Return state
    int          *selected_line;
    MenuReturn   retv;
    char         **lines;
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
    textbox_free ( state->arrowbox_bottom );
    textbox_free ( state->arrowbox_top );

    for ( unsigned int i = 0; i < state->max_elements; i++ ) {
        textbox_free ( state->boxes[i] );
    }

    g_free ( state->boxes );
    g_free ( state->filtered );
    g_free ( state->line_map );
    g_free ( state->distance );
}

/**
 * @param x [out] the calculated x position.
 * @param y [out] the calculated y position.
 * @param mon     the workarea.
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
        state->element_width = ( state->element_width - ( state->columns - 1 ) * LINE_MARGIN ) / state->columns;
    }
}

/**
 * Nav helper functions, to avoid duplicate code.
 */
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
 * @param state Internal state of the menu.
 * @param key the Key being pressed.
 * @param modstate the modifier state.
 *
 * Keyboard navigation through the elements.
 */
static void menu_keyboard_navigation ( MenuState *state, KeySym key, unsigned int modstate )
{
    if ( key == XK_Escape
         // pressing one of the global key bindings closes the switcher. this allows fast closing of the menu if an item is not selected
         || ( ( windows_modmask == AnyModifier || modstate & windows_modmask ) && key == windows_keysym )
         || ( ( rundialog_modmask == AnyModifier || modstate & rundialog_modmask ) && key == rundialog_keysym )
         || ( ( sshdialog_modmask == AnyModifier || modstate & sshdialog_modmask ) && key == sshdialog_keysym )
         ) {
        state->retv = MENU_CANCEL;
        state->quit = TRUE;
    }
    // Up, Ctrl-p or Shift-Tab
    else if ( key == XK_Up || ( key == XK_Tab && modstate & ShiftMask ) ||
              ( key == XK_p && modstate & ControlMask )  ) {
        menu_nav_up ( state );
    }
    else if ( key == XK_Tab ) {
        if ( state->filtered_lines == 1 ) {
            if ( state->filtered[state->selected] ) {
                state->retv               = MENU_OK;
                *( state->selected_line ) = state->line_map[state->selected];
                state->quit               = 1;
            }
            else{
                fprintf ( stderr, "We should never hit this." );
                abort ();
            }
            return;
        }

        // Double tab!
        if ( state->filtered_lines == 0 && key == state->prev_key ) {
            state->retv               = MENU_NEXT;
            *( state->selected_line ) = 0;
            state->quit               = TRUE;
        }
        else{
            state->selected = state->selected < state->filtered_lines - 1 ? MIN (
                state->filtered_lines - 1, state->selected + 1 ) : 0;
            state->update = TRUE;
        }
    }
    // Down, Ctrl-n
    else if ( key == XK_Down ||
              ( key == XK_n && ( modstate & ControlMask ) ) ) {
        menu_nav_down ( state );
    }
    else if ( key == XK_Page_Up && ( modstate & ControlMask ) ) {
        menu_nav_left ( state );
    }
    else if (  key == XK_Page_Down && ( modstate & ControlMask ) ) {
        menu_nav_right ( state );
    }
    else if ( key == XK_Page_Up ) {
        if ( state->selected < state->max_elements ) {
            state->selected = 0;
        }
        else{
            state->selected -= ( state->max_elements );
        }
        state->update = TRUE;
    }
    else if ( key == XK_Page_Down ) {
        state->selected += ( state->max_elements );
        if ( state->selected >= state->filtered_lines ) {
            state->selected = state->filtered_lines - 1;
        }
        state->update = TRUE;
    }
    else if ( key == XK_Home || key == XK_KP_Home ) {
        state->selected = 0;
        state->update   = TRUE;
    }
    else if ( key == XK_End || key == XK_KP_End ) {
        state->selected = state->filtered_lines - 1;
        state->update   = TRUE;
    }
    else if ( key == XK_space && ( modstate & ControlMask ) == ControlMask ) {
        // If a valid item is selected, return that..
        if ( state->selected < state->filtered_lines && state->filtered[state->selected] != NULL ) {
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
    if ( xbe->window == state->arrowbox_top->window ) {
        // Page up.
        if ( state->selected < state->max_rows ) {
            state->selected = 0;
        }
        else{
            state->selected -= state->max_elements;
        }
        state->update = TRUE;
    }
    else if ( xbe->window == state->arrowbox_bottom->window ) {
        // Page down.
        state->selected += state->max_elements;
        if ( state->selected >= state->filtered_lines ) {
            state->selected = state->filtered_lines - 1;
        }
        state->update = TRUE;
    }
    else {
        for ( unsigned int i = 0; config.sidebar_mode == TRUE && i < num_switchers; i++ ) {
            if ( switchers[i].tb->window == ( xbe->window ) ) {
                *( state->selected_line ) = i;
                state->retv               = MENU_QUICK_SWITCH;
                state->quit               = TRUE;
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
                    state->quit = TRUE;
                }
                state->last_button_press = xbe->time;
                break;
            }
        }
    }
}

static void menu_refilter ( MenuState *state, char **lines, menu_match_cb mmc, void *mmc_data,
                            int sorting, int case_sensitive )
{
    if ( strlen ( state->text->text ) > 0 ) {
        unsigned int j        = 0;
        char         **tokens = tokenize ( state->text->text, case_sensitive );

        // input changed
        for ( unsigned int i = 0; i < state->num_lines; i++ ) {
            int match = mmc ( tokens, lines[i], case_sensitive, i, mmc_data );

            // If each token was matched, add it to list.
            if ( match ) {
                state->line_map[j] = i;
                if ( sorting ) {
                    state->distance[i] = levenshtein ( state->text->text, lines[i] );
                }
                // Try to look-up the selected line and highlight that.
                // This is needed 'hack' to fix the dmenu 'next row' modi.
                // int to unsigned int is valid because we check negativeness of
                // selected_line
                if ( state->init == TRUE && ( state->selected_line ) != NULL &&
                     ( *( state->selected_line ) ) >= 0 &&
                     ( (unsigned int) ( *( state->selected_line ) ) ) == i ) {
                    state->selected = j;
                    state->init     = FALSE;
                }
                j++;
            }
        }
        if ( sorting ) {
            qsort_r ( state->line_map, j, sizeof ( int ), lev_sort, state->distance );
        }
        // Update the filtered list.
        for ( unsigned int i = 0; i < j; i++ ) {
            state->filtered[i] = lines[state->line_map[i]];
        }
        for ( unsigned int i = j; i < state->num_lines; i++ ) {
            state->filtered[i] = NULL;
        }

        // Cleanup + bookkeeping.
        state->filtered_lines = j;
        g_strfreev ( tokens );
    }
    else{
        for ( unsigned int i = 0; i < state->num_lines; i++ ) {
            state->filtered[i] = lines[i];
            state->line_map[i] = i;
        }
        state->filtered_lines = state->num_lines;
    }
    state->selected = MIN ( state->selected, state->filtered_lines - 1 );

    state->refilter = FALSE;
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
    }

    for ( i = 0; i < state->max_elements; i++ ) {
        if ( ( i + offset ) >= state->num_lines || state->filtered[i + offset] == NULL ) {
            textbox_font ( state->boxes[i], NORMAL );
            textbox_text ( state->boxes[i], "" );
        }
        else{
            TextBoxFontType type  = ( ( ( i % state->max_rows ) & 1 ) == 0 ) ? NORMAL : ALT;
            char            *text = state->filtered[i + offset];
            TextBoxFontType tbft  = ( i + offset ) == state->selected ? HIGHLIGHT : type;
            textbox_font ( state->boxes[i], tbft );
            textbox_text ( state->boxes[i], text );
        }

        textbox_draw ( state->boxes[i] );
    }
}

static void menu_update ( MenuState *state )
{
    menu_hide_arrow_text ( state->filtered_lines, state->selected,
                           state->max_elements, state->arrowbox_top,
                           state->arrowbox_bottom );
    textbox_draw ( state->case_indicator );
    textbox_draw ( state->prompt_tb );
    textbox_draw ( state->text );
    menu_draw ( state );
    menu_set_arrow_text ( state->filtered_lines, state->selected,
                          state->max_elements, state->arrowbox_top,
                          state->arrowbox_bottom );
    // Why do we need the specian -1?
    int line_height = textbox_get_height ( state->text );
    XDrawLine ( display, main_window, gc, ( config.padding ),
                line_height + ( config.padding ) + ( LINE_MARGIN  ) / 2,
                state->w - ( ( config.padding ) ) - 1,
                line_height + ( config.padding ) + ( LINE_MARGIN  ) / 2 );

    if ( config.sidebar_mode == TRUE ) {
        int line_height = textbox_get_height ( state->text );
        XDrawLine ( display, main_window, gc,
                    ( config.padding ),
                    state->h - line_height - ( config.padding ) - 1,
                    state->w - ( ( config.padding ) ) - 1,
                    state->h - line_height - ( config.padding ) - 1 );
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
            textbox_cursor ( state->text, state->text->cursor + dl - 1 );
            // Force a redraw and refiltering of the text.
            state->update   = TRUE;
            state->refilter = TRUE;
        }
        g_free ( text );
    }
}

MenuReturn menu ( char **lines, unsigned int num_lines, char **input, char *prompt, Time *time,
                  int *shift, menu_match_cb mmc, void *mmc_data, int *selected_line, int sorting )
{
    MenuState    state = {
        .selected_line     = selected_line,
        .retv              = MENU_CANCEL,
        .prev_key          =             0,
        .last_button_press =             0,
        .last_offset       =             0,
        .num_lines         = num_lines,
        .distance          = NULL,
        .init              = TRUE,
        .quit              = FALSE,
        .filtered_lines    =             0,
        .max_elements      =             0,
        // We want to filter on the first run.
        .refilter = TRUE,
        .update   = FALSE,
        .lines    = lines
    };
    unsigned int i;
    workarea     mon;

    // main window isn't explicitly destroyed in case we switch modes. Reusing it prevents flicker
    XWindowAttributes attr;
    if ( main_window == None || XGetWindowAttributes ( display, main_window, &attr ) == 0 ) {
        main_window = create_window ( display );
    }
    // Get active monitor size.
    monitor_active ( display, &mon );

    // we need this at this point so we can get height.
    state.case_indicator = textbox_create ( main_window, &vinfo, map, TB_AUTOHEIGHT | TB_AUTOWIDTH,
                                            ( config.padding ), ( config.padding ),
                                            0, 0,
                                            NORMAL, "*" );

    // Height of a row.
    int line_height = textbox_get_height ( state.case_indicator );
    if ( config.menu_lines == 0 ) {
        // Autosize it.
        int h = mon.h - config.padding * 2 - LINE_MARGIN - config.menu_bw * 2;
        int r = ( h ) / ( line_height * config.element_height ) - 1 - config.sidebar_mode;
        state.menu_lines = r;
    }
    else {
        state.menu_lines = config.menu_lines;
    }
    menu_calculate_rows_columns ( &state );
    menu_calculate_window_and_element_width ( &state, &mon );

    // Prompt box.
    state.prompt_tb = textbox_create ( main_window, &vinfo, map, TB_AUTOHEIGHT | TB_AUTOWIDTH,
                                       ( config.padding ),
                                       ( config.padding ),
                                       0, 0, NORMAL, prompt );
    // Entry box
    int entrybox_width = state.w
                         - ( 2 * ( config.padding ) )
                         - textbox_get_width ( state.prompt_tb )
                         - textbox_get_width ( state.case_indicator );

    state.text = textbox_create ( main_window, &vinfo, map, TB_AUTOHEIGHT | TB_EDITABLE,
                                  ( config.padding ) + textbox_get_width ( state.prompt_tb ),
                                  ( config.padding ),
                                  entrybox_width, 0,
                                  NORMAL,
                                  ( input != NULL ) ? *input : "" );
    // Move indicator to end.
    textbox_move ( state.case_indicator,
                   config.padding + textbox_get_width ( state.prompt_tb ) + entrybox_width,
                   0 );

    textbox_show ( state.text );
    textbox_show ( state.prompt_tb );

    if ( config.case_sensitive ) {
        textbox_show ( state.case_indicator );
    }

    int element_height = line_height * config.element_height;
    // filtered list display
    state.boxes = g_malloc0_n ( state.max_elements, sizeof ( textbox* ) );

    int y_offset = config.padding + line_height;
    int x_offset = config.padding;

    for ( i = 0; i < state.max_elements; i++ ) {
        unsigned int ex = ( ( i ) / state.max_rows ) * ( state.element_width + LINE_MARGIN );
        unsigned int ey = ( ( i ) % state.max_rows ) * element_height + LINE_MARGIN;

        state.boxes[i] = textbox_create ( main_window, &vinfo, map, 0,
                                          ex + x_offset, ey + y_offset,
                                          state.element_width, element_height, NORMAL, "" );
        textbox_show ( state.boxes[i] );
    }
    // Arrows
    state.arrowbox_top = textbox_create ( main_window, &vinfo, map, TB_AUTOHEIGHT | TB_AUTOWIDTH,
                                          ( config.padding ), ( config.padding ),
                                          0, 0, NORMAL,
                                          "↑" );
    state.arrowbox_bottom = textbox_create ( main_window, &vinfo, map, TB_AUTOHEIGHT | TB_AUTOWIDTH,
                                             ( config.padding ), ( config.padding ),
                                             0, 0, NORMAL,
                                             "↓" );

    textbox_move ( state.arrowbox_top,
                   state.w - config.padding - state.arrowbox_top->w,
                   config.padding + line_height + LINE_MARGIN );
    textbox_move ( state.arrowbox_bottom,
                   state.w - config.padding - state.arrowbox_bottom->w,
                   config.padding + state.max_rows * element_height + LINE_MARGIN );

    // filtered list
    state.filtered = (char * *) g_malloc0_n ( state.num_lines, sizeof ( char* ) );
    state.line_map = g_malloc0_n ( state.num_lines, sizeof ( int ) );
    if ( sorting ) {
        state.distance = (int *) g_malloc0_n ( state.num_lines, sizeof ( int ) );
    }

    // resize window vertically to suit
    // Subtract the margin of the last row.
    state.h = line_height + element_height * state.max_rows + ( config.padding ) * 2 + LINE_MARGIN;

    // Add entry
    if ( config.sidebar_mode == TRUE ) {
        state.h += line_height + LINE_MARGIN;
    }

    // Sidebar mode.
    if ( config.menu_lines == 0 ) {
        state.h = mon.h - config.menu_bw * 2;
    }

    // Move the window to the correct x,y position.
    calculate_window_position ( &state, &mon );

    if ( config.sidebar_mode == TRUE ) {
        int line_height = textbox_get_height ( state.text );
        int width       = ( state.w - ( 2 * ( config.padding ) ) ) / num_switchers;
        for ( unsigned int j = 0; j < num_switchers; j++ ) {
            switchers[j].tb = textbox_create ( main_window, &vinfo, map, TB_CENTER,
                                               config.padding + j * width, state.h - line_height - config.padding,
                                               width, line_height, ( j == curr_switcher ) ? HIGHLIGHT : NORMAL, switchers[j].name );
            textbox_show ( switchers[j].tb );
        }
    }

    // Display it.
    XMoveResizeWindow ( display, main_window, state.x, state.y, state.w, state.h );
    XMapRaised ( display, main_window );

    // if grabbing keyboard failed, fall through
    state.selected = 0;
    // The cast to unsigned in here is valid, we checked if selected_line > 0.
    // So its maximum range is 0-2³¹, well within the num_lines range.
    if ( ( *( state.selected_line ) ) >= 0 && (unsigned int) ( *( state.selected_line ) ) <= state.num_lines ) {
        state.selected = *( state.selected_line );
    }

    state.quit = FALSE;
    menu_refilter ( &state, lines, mmc, mmc_data, sorting, config.case_sensitive );

    int x11_fd       = ConnectionNumber ( display );
    int has_keyboard = take_keyboard ( display, main_window );
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
        if ( !has_keyboard || ( state.refilter && state.num_lines > config.lazy_filter_limit ) ) {
            mle = wait_for_xevent_or_timeout ( display, x11_fd );
            // Whatever happened, try to get keyboard.
            has_keyboard = take_keyboard ( display, main_window );
        }
        // If not in lazy mode, refilter.
        if ( state.num_lines <= config.lazy_filter_limit ) {
            if ( state.refilter ) {
                menu_refilter ( &state, lines, mmc, mmc_data, sorting, config.case_sensitive );
                menu_update ( &state );
            }
        }
        else if ( mle == ML_TIMEOUT ) {
            // When timeout (and in lazy filter mode)
            // We refilter then loop back and wait for Xevent.
            if ( state.refilter ) {
                menu_refilter ( &state, lines, mmc, mmc_data, sorting, config.case_sensitive );
                menu_update ( &state );
            }
        }
        if ( mle == ML_TIMEOUT ) {
            continue;
        }
        // Get next event. (might block)
        XNextEvent ( display, &ev );

        // Handle event.
        if ( ev.type == Expose ) {
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
                if ( time ) {
                    *time = ev.xkey.time;
                }

                KeySym key = XkbKeycodeToKeysym ( display, ev.xkey.keycode, 0, 0 );

                // Handling of paste
                if ( ( ( ( ev.xkey.state & ControlMask ) == ControlMask ) && key == XK_v ) ) {
                    XConvertSelection ( display, ( ev.xkey.state & ShiftMask ) ?
                                        XA_PRIMARY : netatoms[CLIPBOARD],
                                        netatoms[UTF8_STRING], netatoms[UTF8_STRING], main_window, CurrentTime );
                }
                else if ( key == XK_Insert ) {
                    XConvertSelection ( display, ( ev.xkey.state & ShiftMask ) ?
                                        XA_PRIMARY : netatoms[CLIPBOARD],
                                        netatoms[UTF8_STRING], netatoms[UTF8_STRING], main_window, CurrentTime );
                }
                else if ( ( ( ev.xkey.state & ShiftMask ) == ShiftMask ) && key == XK_Left ) {
                    state.retv               = MENU_PREVIOUS;
                    *( state.selected_line ) = 0;
                    state.quit               = TRUE;
                    break;
                }
                // Menu navigation.
                else if ( ( ( ev.xkey.state & ShiftMask ) == ShiftMask ) &&
                          key == XK_Right ) {
                    state.retv               = MENU_NEXT;
                    *( state.selected_line ) = 0;
                    state.quit               = TRUE;
                    break;
                }
                // Toggle case sensitivity.
                else if ( key == XK_grave || key == XK_dead_grave
                          || key == XK_acute ) {
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
                // Switcher short-cut
                else if ( ( ( ev.xkey.state & Mod1Mask ) == Mod1Mask ) &&
                          key >= XK_1 && key <= XK_9 ) {
                    *( state.selected_line ) = ( key - XK_1 );
                    state.retv               = MENU_QUICK_SWITCH;
                    state.quit               = TRUE;
                    break;
                }
                // Special delete entry command.
                else if ( ( ( ev.xkey.state & ShiftMask ) == ShiftMask ) &&
                          key == XK_Delete ) {
                    if ( state.filtered[state.selected] != NULL ) {
                        *( state.selected_line ) = state.line_map[state.selected];
                        state.retv               = MENU_ENTRY_DELETE;
                        state.quit               = TRUE;
                        break;
                    }
                }
                else{
                    int rc = textbox_keypress ( state.text, &ev );
                    // Row is accepted.
                    if ( rc < 0 ) {
                        if ( shift != NULL ) {
                            ( *shift ) = ( ( ev.xkey.state & ShiftMask ) == ShiftMask );
                        }

                        // If a valid item is selected, return that..
                        if ( state.selected < state.filtered_lines && state.filtered[state.selected] != NULL ) {
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
                    else if ( rc > 0 ) {
                        state.refilter = TRUE;
                        state.update   = TRUE;
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

    release_keyboard ( display );
    // Update input string.
    g_free ( *input );
    *input = g_strdup ( state.text->text );

    int retv = state.retv;
    menu_free_state ( &state );

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

void error_dialog ( const char *msg )
{
    MenuState state = {
        .selected_line     = NULL,
        .retv              = MENU_CANCEL,
        .prev_key          =           0,
        .last_button_press =           0,
        .last_offset       =           0,
        .num_lines         =           0,
        .distance          = NULL,
        .init              = FALSE,
        .quit              = FALSE,
        .filtered_lines    =           0,
        .columns           =           0,
        .update            = TRUE,
    };
    workarea  mon;
    // Get active monitor size.
    monitor_active ( display, &mon );
    // main window isn't explicitly destroyed in case we switch modes. Reusing it prevents flicker
    XWindowAttributes attr;
    if ( main_window == None || XGetWindowAttributes ( display, main_window, &attr ) == 0 ) {
        main_window = create_window ( display );
    }


    menu_calculate_window_and_element_width ( &state, &mon );
    state.max_elements = 0;

    state.text = textbox_create ( main_window, &vinfo, map, TB_AUTOHEIGHT,
                                  ( config.padding ),
                                  ( config.padding ),
                                  ( state.w - ( 2 * ( config.padding ) ) ),
                                  1,
                                  NORMAL,
                                  ( msg != NULL ) ? msg : "" );
    textbox_show ( state.text );
    int line_height = textbox_get_height ( state.text );

    // resize window vertically to suit
    state.h = line_height + ( config.padding ) * 2;

    // Move the window to the correct x,y position.
    calculate_window_position ( &state, &mon );
    XMoveResizeWindow ( display, main_window, state.x, state.y, state.w, state.h );

    // Display it.
    XMapRaised ( display, main_window );

    int x11_fd       = ConnectionNumber ( display );
    int has_keyboard = take_keyboard ( display, main_window );
    while ( !state.quit ) {
        // Update if requested.
        if ( state.update ) {
            textbox_draw ( state.text );
            state.update = FALSE;
        }
        // Wait for event.
        XEvent        ev;
        MainLoopEvent mle = ML_XEVENT;
        if ( !has_keyboard ) {
            mle          = wait_for_xevent_or_timeout ( display, x11_fd );
            has_keyboard = take_keyboard ( display, main_window );
        }
        if ( mle == ML_TIMEOUT ) {
            // Loop.
            continue;
        }
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
}


/**
 * Start dmenu mode.
 */
static int run_dmenu ()
{
    // Create pid file
    create_pid_file ( pidfile );

    // Request truecolor visual.
    create_visual_and_colormap ( display );
    textbox_setup ( &vinfo, map,
                    config.menu_bg, config.menu_bg_alt, config.menu_fg,
                    config.menu_hlbg,
                    config.menu_hlfg );
    char *input = NULL;

    // Dmenu modi has a return state.
    int ret_state = dmenu_switcher_dialog ( &input );

    g_free ( input );

    // Cleanup font setup.
    textbox_cleanup ( );

    if ( map != None ) {
        XFreeColormap ( display, map );
        map = None;
    }
    return ret_state;
}

static void run_switcher ( int do_fork, SwitcherMode mode )
{
    // we fork because it's technically possible to have multiple window
    // lists up at once on a zaphod multihead X setup.
    // this also happens to isolate the Xft font stuff in a child process
    // that gets cleaned up every time. that library shows some valgrind
    // strangeness...
    if ( do_fork == TRUE ) {
        if ( fork () ) {
            return;
        }

        display = XOpenDisplay ( display_str );
        XSync ( display, True );
    }
    // Create pid file to avoid multiple instances.
    create_pid_file ( pidfile );
    // Create the colormap and the main visual.
    create_visual_and_colormap ( display );

    // Because of the above fork, we want to do this here.
    // Make sure this is isolated to its own thread.
    textbox_setup ( &vinfo, map,
                    config.menu_bg, config.menu_bg_alt, config.menu_fg,
                    config.menu_hlbg,
                    config.menu_hlfg );
    // Otherwise check if requested mode is enabled.
    if ( switchers[mode].cb != NULL ) {
        char *input = NULL;
        do {
            SwitcherMode retv;

            curr_switcher = mode;
            retv          = switchers[mode].cb ( &input, switchers[mode].cb_data );
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
    }

    // Cleanup font setup.
    textbox_cleanup ( );
    if ( map != None ) {
        XFreeColormap ( display, map );
        map = None;
    }

    if ( do_fork == TRUE ) {
        exit ( EXIT_SUCCESS );
    }
}

/**
 * Function that listens for global key-presses.
 * This is only used when in daemon mode.
 */
static void handle_keypress ( XEvent *ev )
{
    int    index = -1;
    KeySym key   = XkbKeycodeToKeysym ( display, ev->xkey.keycode, 0, 0 );
    if ( ( windows_modmask == AnyModifier || ev->xkey.state & windows_modmask ) &&
         key == windows_keysym ) {
        index = switcher_get ( "window" );
    }

    if ( ( rundialog_modmask == AnyModifier || ev->xkey.state & rundialog_modmask ) &&
         key == rundialog_keysym ) {
        index = switcher_get ( "run" );
    }

    if ( ( sshdialog_modmask == AnyModifier || ev->xkey.state & sshdialog_modmask ) &&
         key == sshdialog_keysym ) {
        index = switcher_get ( "ssh" );
    }
    if ( index >= 0 ) {
        run_switcher ( TRUE, index );
    }
}


/**
 * Help function. This calls man.
 */
static void help ()
{
    int code = execlp ( "man", "man", MANPAGE_PATH, NULL );

    if ( code == -1 ) {
        fprintf ( stderr, "Failed to execute man: %s\n", strerror ( errno ) );
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
            XFreeGC ( display, gc );
            XDestroyWindow ( display, main_window );
            XCloseDisplay ( display );
        }
    }

    // Cleaning up memory allocated by the Xresources file.
    config_xresource_free ();

    for ( unsigned int i = 0; i < num_switchers; i++ ) {
        // only used for script dialog.
        if ( switchers[i].cb_data_free != NULL ) {
            switchers[i].cb_data_free ( switchers[i].cb_data );
        }
    }
    g_free ( switchers );

    // Cleanup pid file.
    if ( pidfile ) {
        unlink ( pidfile );
        g_free ( pidfile );
    }
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
        switchers                             = (Switcher *) g_realloc ( switchers, sizeof ( Switcher ) * ( num_switchers + 1 ) );
        switchers[num_switchers].cb_data      = NULL;
        switchers[num_switchers].cb_data_free = NULL;

        // Window switcher.
        if ( strcasecmp ( token, "window" ) == 0 ) {
            g_strlcpy ( switchers[num_switchers].name, "window", 32 );
            switchers[num_switchers].cb = run_switcher_window;
            num_switchers++;
        }
        // SSh dialog
        else if ( strcasecmp ( token, "ssh" ) == 0 ) {
            g_strlcpy ( switchers[num_switchers].name, "ssh", 32 );
            switchers[num_switchers].cb = ssh_switcher_dialog;
            num_switchers++;
        }
        // Run dialog
        else if ( strcasecmp ( token, "run" ) == 0 ) {
            g_strlcpy ( switchers[num_switchers].name, "run", 32 );
            switchers[num_switchers].cb = run_switcher_dialog;
            num_switchers++;
        }
        else {
            // If not build in, use custom switchers.
            ScriptOptions *sw = script_switcher_parse_setup ( token );
            if ( sw != NULL ) {
                // Resize and add entry.
                g_strlcpy ( switchers[num_switchers].name, sw->name, 32 );
                switchers[num_switchers].cb           = script_switcher_dialog;
                switchers[num_switchers].cb_data      = sw;
                switchers[num_switchers].cb_data_free = script_switcher_free_options;
                num_switchers++;
            }
            else{
                // Report error, don't continue.
                fprintf ( stderr, "Invalid script switcher: %s\n", token );
                token = NULL;
            }
        }
    }
    // Free string that was modified by strtok_r
    g_free ( switcher_str );
}

/**
 * Keep a copy of arc, argv around, so we can use the same parsing method
 */
static int  stored_argc;
static char **stored_argv;

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
    config_parse_cmd_options ( stored_argc, stored_argv );

    // Sanity check
    config_sanity_check ();
}

/**
 * Handle sighub request.
 * Currently we just reload the configuration.
 */
static void hup_action_handler ( int num )
{
    if ( num == SIGHUP ) {
        /**
         * Open new connection to X. It seems the XResources do not get updated on the old
         * connection.
         */
        Display *display = XOpenDisplay ( display_str );
        if ( display ) {
            load_configuration ( display );
            XCloseDisplay ( display );
        }
    }
}

static void show_error_message ( const char *msg )
{
    // Create pid file
    create_pid_file ( pidfile );

    // Request truecolor visual.
    create_visual_and_colormap ( display );
    textbox_setup ( &vinfo, map,
                    config.menu_bg, config.menu_bg_alt, config.menu_fg,
                    config.menu_hlbg,
                    config.menu_hlfg );
    error_dialog ( msg );
    textbox_cleanup ( );
    if ( map != None ) {
        XFreeColormap ( display, map );
        map = None;
    }
}

int main ( int argc, char *argv[] )
{
    stored_argc = argc;
    stored_argv = argv;

    // catch help request
    if ( find_arg ( argc, argv, "-h" ) >= 0 || find_arg ( argc, argv, "-help" ) >= 0 ) {
        help ();
        exit ( EXIT_SUCCESS );
    }
    // Version
    if ( find_arg ( argc, argv, "-v" ) >= 0 || find_arg ( argc, argv, "-version" ) >= 0 ) {
        fprintf ( stdout, "Version: "VERSION "\n" );
        exit ( EXIT_SUCCESS );
    }

    // Get the path to the cache dir.
    cache_dir = g_get_user_cache_dir ();

    // Create pid file path.
    if ( find_arg_str_alloc ( argc, argv, "-pid", &( pidfile ) ) == FALSE ) {
        const char *path = g_get_user_runtime_dir ();
        if ( path ) {
            pidfile = g_build_filename ( path, "rofi.pid", NULL );
        }
    }

    // Register cleanup function.
    atexit ( cleanup );

    // Get DISPLAY, first env, then argument.
    display_str = getenv ( "DISPLAY" );
    find_arg_str ( argc, argv, "-display", &display_str );

    if ( !( display = XOpenDisplay ( display_str ) ) ) {
        fprintf ( stderr, "cannot open display!\n" );
        return EXIT_FAILURE;
    }


    load_configuration ( display );

    // Dump.
    if ( find_arg ( argc, argv, "-dump-xresources" ) >= 0 ) {
        xresource_dump ();
        exit ( EXIT_SUCCESS );
    }

    // setup_switchers
    setup_switchers ();

    // Set up X interaction.
    signal ( SIGCHLD, catch_exit );

    x11_setup ( display );

    char *msg = NULL;
    if ( find_arg_str ( argc, argv, "-e", &( msg ) ) ) {
        show_error_message ( msg );
        exit ( EXIT_SUCCESS );
    }

    // Detect if we are in dmenu mode.
    // This has two possible causes.
    int dmenu_mode = FALSE;
    // 1 the user specifies it on the command-line.
    if ( find_arg ( argc, argv, "-dmenu" ) >= 0 ) {
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

    // Dmenu mode.
    if ( dmenu_mode == TRUE ) {
        // force off sidebar mode:
        config.sidebar_mode = FALSE;
        // Check prompt
        find_arg_str ( argc, argv, "-p", &dmenu_prompt );
        find_arg_int ( argc, argv, "-l", &dmenu_selected_line );
        int retv = run_dmenu ();

        // User canceled the operation.
        if ( retv == FALSE ) {
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    // flags to run immediately and exit
    char *sname = NULL;
    if ( find_arg_str ( argc, argv, "-show", &sname ) == TRUE ) {
        int index = switcher_get ( sname );
        if ( index >= 0 ) {
            run_switcher ( FALSE, index );
        }
        else {
            fprintf ( stderr, "The %s switcher has not been enabled\n", sname );
        }
    }
    // Old modi.
    else if ( find_arg ( argc, argv, "-now" ) >= 0 ) {
        int index = switcher_get ( "window" );
        if ( index >= 0 ) {
            run_switcher ( FALSE, index );
        }
        else {
            fprintf ( stderr, "The window switcher has not been enabled\n" );
        }
    }
    else if ( find_arg ( argc, argv, "-rnow" ) >= 0 ) {
        int index = switcher_get ( "run" );
        if ( index >= 0 ) {
            run_switcher ( FALSE, index );
        }
        else {
            fprintf ( stderr, "The run dialog has not been enabled\n" );
        }
    }
    else if ( find_arg ( argc, argv, "-snow" ) >= 0 ) {
        int index = switcher_get ( "ssh" );
        if ( index >= 0 ) {
            run_switcher ( FALSE, index );
        }
        else {
            fprintf ( stderr, "The ssh dialog has not been enabled\n" );
        }
    }
    else{
        // Daemon mode, Listen to key presses..
        if ( switcher_get ( "window" ) >= 0 ) {
            x11_parse_key ( config.window_key, &windows_modmask, &windows_keysym );
            x11_grab_key ( display, windows_modmask, windows_keysym );
        }

        if ( switcher_get ( "run" ) >= 0 ) {
            x11_parse_key ( config.run_key, &rundialog_modmask, &rundialog_keysym );
            x11_grab_key ( display, rundialog_modmask, rundialog_keysym );
        }

        if ( switcher_get ( "ssh" ) >= 0 ) {
            x11_parse_key ( config.ssh_key, &sshdialog_modmask, &sshdialog_keysym );
            x11_grab_key ( display, sshdialog_modmask, sshdialog_keysym );
        }

        // Setup handler for sighub (reload config)
        const struct sigaction hup_action = {
            .sa_handler = hup_action_handler
        };
        sigaction ( SIGHUP, &hup_action, NULL );


        // Application Main loop.
        // This listens in the background for any events on the Xserver
        // catching global key presses.
        for (;; ) {
            XEvent ev;

            // block and wait for something
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

    return EXIT_SUCCESS;
}
