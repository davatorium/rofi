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

#include <cairo.h>
#include <cairo-xlib.h>

#define SN_API_NOT_YET_FROZEN
#include <libsn/sn.h>

#include "rofi.h"
#include "helper.h"
#include "textbox.h"
#include "scrollbar.h"
#include "x11-helper.h"
#include "xrmoptions.h"
#include "dialogs/dialogs.h"

ModeMode switcher_run ( char **input, Mode *sw );

typedef enum _MainLoopEvent
{
    ML_XEVENT,
    ML_TIMEOUT
} MainLoopEvent;
typedef struct _ModeHolder
{
    Mode    *sw;
    textbox *tb;
} ModeHolder;

// Pidfile.
extern Atom       netatoms[NUM_NETATOMS];
char              *pidfile           = NULL;
const char        *cache_dir         = NULL;
SnDisplay         *sndisplay         = NULL;
SnLauncheeContext *sncontext         = NULL;
Display           *display           = NULL;
char              *display_str       = NULL;
Window            main_window        = None;
Colormap          map                = None;
unsigned int      normal_window_mode = FALSE;
// Array of modi.
ModeHolder        *modi    = NULL;
unsigned int      num_modi = 0;
// Current selected switcher.
unsigned int      curr_switcher = 0;
XVisualInfo       vinfo;

cairo_surface_t   *surface = NULL;
cairo_surface_t   *fake_bg = NULL;
cairo_t           *draw    = NULL;
XIM               xim;
XIC               xic;

GThreadPool       *tpool = NULL;

/**
 * @param name Name of the switcher to lookup.
 *
 * Find the index of the switcher with name.
 *
 * @returns index of the switcher in modi, -1 if not found.
 */
static int switcher_get ( const char *name )
{
    for ( unsigned int i = 0; i < num_modi; i++ ) {
        if ( strcmp ( modi[i].sw->name, name ) == 0 ) {
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
    struct timeval tv = { .tv_usec = 200000, .tv_sec = 0 };
    fd_set         in_fds;
    // Create a File Description Set containing x11_fd
    FD_ZERO ( &in_fds );
    FD_SET ( x11_fd, &in_fds );

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

#define MIN3( a, b, c )    ( ( a ) < ( b ) ? ( ( a ) < ( c ) ? ( a ) : ( c ) ) : ( ( b ) < ( c ) ? ( b ) : ( c ) ) )

static int levenshtein ( const char *s1, const char *s2 )
{
    unsigned int x, y, lastdiag, olddiag;
    size_t       s1len = strlen ( s1 );
    size_t       s2len = strlen ( s2 );
    unsigned int column[s1len + 1];
    for ( y = 1; y <= s1len; y++ ) {
        column[y] = y;
    }
    for ( x = 1; x <= s2len; x++ ) {
        column[0] = x;
        for ( y = 1, lastdiag = x - 1; y <= s1len; y++ ) {
            olddiag   = column[y];
            column[y] = MIN3 ( column[y] + 1, column[y - 1] + 1, lastdiag + ( s1[y - 1] == s2[x - 1] ? 0 : 1 ) );
            lastdiag  = olddiag;
        }
    }
    return column[s1len];
}

// State of the menu.

typedef struct MenuState
{
    Mode         *sw;
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
    int          *lines_not_ascii;
    int          line_height;
    unsigned int border;
    workarea     mon;
}MenuState;

static Window create_window ( Display *display )
{
    XSetWindowAttributes attr;
    attr.colormap         = map;
    attr.border_pixel     = 0;
    attr.background_pixel = 0;

    Window box = XCreateWindow ( display, DefaultRootWindow ( display ), 0, 0, 200, 100, 0, vinfo.depth, InputOutput,
                                 vinfo.visual, CWColormap | CWBorderPixel | CWBackPixel, &attr );
    XSelectInput (
        display,
        box,
        KeyReleaseMask | KeyPressMask | ExposureMask | ButtonPressMask | StructureNotifyMask | FocusChangeMask |
        Button1MotionMask );

    surface = cairo_xlib_surface_create ( display, box, vinfo.visual, 200, 100 );
    // Create a drawable.
    draw = cairo_create ( surface );
    cairo_set_operator ( draw, CAIRO_OPERATOR_SOURCE );

    // // make it an unmanaged window
    if ( !normal_window_mode && !config.fullscreen ) {
        window_set_atom_prop ( display, box, netatoms[_NET_WM_STATE], &netatoms[_NET_WM_STATE_ABOVE], 1 );
        XSetWindowAttributes sattr = { .override_redirect = True };
        XChangeWindowAttributes ( display, box, CWOverrideRedirect, &sattr );
    }
    else{
        window_set_atom_prop ( display, box, netatoms[_NET_WM_WINDOW_TYPE], &netatoms[_NET_WM_WINDOW_TYPE_NORMAL], 1 );
    }
    if ( config.fullscreen ) {
        Atom atoms[] = {
            netatoms[_NET_WM_STATE_FULLSCREEN],
            netatoms[_NET_WM_STATE_ABOVE]
        };
        window_set_atom_prop ( display, box, netatoms[_NET_WM_STATE], atoms, sizeof ( atoms ) / sizeof ( Atom ) );
    }

    xim = XOpenIM ( display, NULL, NULL, NULL );
    xic = XCreateIC ( xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow,
                      box, XNFocusWindow, box, NULL );

    // Set the WM_NAME
    XStoreName ( display, box, "rofi" );

    x11_set_window_opacity ( display, box, config.window_opacity );
    return box;
}

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
    g_free ( state->lines_not_ascii );
}

/**
 * @param state   the state of the View.
 * @param mon     the work area.
 *
 * Calculates the window poslition
 */
static void calculate_window_position ( MenuState *state )
{
    // Default location is center.
    state->y = state->mon.y + ( state->mon.h - state->h ) / 2;
    state->x = state->mon.x + ( state->mon.w - state->w ) / 2;
    // Determine window location
    switch ( config.location )
    {
    case WL_NORTH_WEST:
        state->x = state->mon.x;
    case WL_NORTH:
        state->y = state->mon.y;
        break;
    case WL_NORTH_EAST:
        state->y = state->mon.y;
    case WL_EAST:
        state->x = state->mon.x + state->mon.w - state->w;
        break;
    case WL_EAST_SOUTH:
        state->x = state->mon.x + state->mon.w - state->w;
    case WL_SOUTH:
        state->y = state->mon.y + state->mon.h - state->h;
        break;
    case WL_SOUTH_WEST:
        state->y = state->mon.y + state->mon.h - state->h;
    case WL_WEST:
        state->x = state->mon.x;
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
    state->max_rows = MIN ( state->menu_lines, (unsigned int) ( ( state->num_lines + ( state->columns - state->num_lines %
                                                                                       state->columns ) %
                                                                  state->columns ) / ( state->columns ) ) );
    // Always have at least one row.
    state->max_rows = MAX ( 1, state->max_rows );

    if ( config.fixed_num_lines == TRUE ) {
        state->max_elements = state->menu_lines * state->columns;
        state->max_rows     = state->menu_lines;
        // If it would fit in one column, only use one column.
        if ( state->num_lines < state->max_elements ) {
            state->columns =
                ( state->num_lines + ( state->max_rows - state->num_lines % state->max_rows ) % state->max_rows ) / state->max_rows;
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
        state->w += 2 * state->border + 4; // 4 = 2*SIDE_MARGIN
    }
    else{
        // Calculate as float to stop silly, big rounding down errors.
        state->w = config.menu_width < 101 ? ( mon->w / 100.0f ) * ( float ) config.menu_width : config.menu_width;
    }

    if ( state->columns > 0 ) {
        state->element_width = state->w - ( 2 * ( state->border ) );
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
    // If no lines, do nothing.
    if ( state->filtered_lines == 0 ) {
        return;
    }
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
    // If no lines, do nothing.
    if ( state->filtered_lines == 0 ) {
        return;
    }
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
    // If no lines, do nothing.
    if ( state->filtered_lines == 0 ) {
        return;
    }
    state->selected = state->selected < state->filtered_lines - 1 ? MIN ( state->filtered_lines - 1, state->selected + 1 ) : 0;
    state->update   = TRUE;
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
    // If no lines, do nothing.
    if ( state->filtered_lines == 0 ) {
        return;
    }
    state->selected = state->filtered_lines - 1;
    state->update   = TRUE;
}
/**
 * @param key the Key to match
 * @param modstate the modifier state to match
 *
 * Match key and modifier state against modi.
 *
 * @return the index of the switcher that matches the key combination
 * specified by key and modstate. Returns -1 if none was found
 */
extern unsigned int NumlockMask;
static int locate_switcher ( KeySym key, unsigned int modstate )
{
    // ignore annoying modifiers
    unsigned int modstate_filtered = modstate & ~( LockMask | NumlockMask );
    for ( unsigned int i = 0; i < num_modi; i++ ) {
        if ( modi[i].sw->keystr != NULL ) {
            if ( ( modstate_filtered == modi[i].sw->modmask ) &&
                 modi[i].sw->keysym == key ) {
                return i;
            }
        }
    }
    return -1;
}

/**
 * Stores a screenshot of Rofi at that point in time.
 */
static void menu_capture_screenshot ( void )
{
    int index = 0;
    if ( surface == NULL ) {
        // Nothing to store.
        fprintf ( stderr, "There is no rofi surface to store\n" );
        return;
    }
    const char *xdg_pict_dir = g_get_user_special_dir ( G_USER_DIRECTORY_PICTURES );
    if ( xdg_pict_dir == NULL ) {
        fprintf ( stderr, "XDG user picture directory is not set. Cannot store screenshot.\n" );
        return;
    }
    // Get current time.
    GDateTime  *now = g_date_time_new_now_local ();
    // Format filename.
    char       *timestmp = g_date_time_format ( now, "rofi-%Y-%m-%d-%H%M" );
    char       *filename = g_strdup_printf ( "%s.png", timestmp );
    // Build full path
    char       *fpath = NULL;
    const char *outp  = g_getenv ( "ROFI_PNG_OUTPUT" );
    if ( outp == NULL ) {
        fpath = g_build_filename ( xdg_pict_dir, filename, NULL );
        while ( g_file_test ( fpath, G_FILE_TEST_EXISTS ) && index < 99 ) {
            g_free ( fpath );
            g_free ( filename );
            // Try the next index.
            index++;
            // Format filename.
            filename = g_strdup_printf ( "%s-%d.png", timestmp, index );
            // Build full path
            fpath = g_build_filename ( xdg_pict_dir, filename, NULL );
        }
    }
    else {
        fpath = g_strdup ( outp );
    }
    fprintf ( stderr, color_green "Storing screenshot %s\n"color_reset, fpath );
    cairo_status_t status = cairo_surface_write_to_png ( surface, fpath );
    if ( status != CAIRO_STATUS_SUCCESS ) {
        fprintf ( stderr, "Failed to produce screenshot '%s', got error: '%s'\n", filename,
                  cairo_status_to_string ( status ) );
    }
    g_free ( fpath );
    g_free ( filename );
    g_free ( timestmp );
    g_date_time_unref ( now );
}

/**
 * @param state Internal state of the menu.
 * @param key the Key being pressed.
 * @param modstate the modifier state.
 *
 * Keyboard navigation through the elements.
 */
static int menu_keyboard_navigation ( MenuState *state, KeySym key, unsigned int modstate )
{
    // pressing one of the global key bindings closes the switcher. This allows fast closing of the
    // menu if an item is not selected
    if ( locate_switcher ( key, modstate ) != -1 || abe_test_action ( CANCEL, modstate, key ) ) {
        state->retv = MENU_CANCEL;
        state->quit = TRUE;
        return 1;
    }
    // Up, Ctrl-p or Shift-Tab
    else if ( abe_test_action ( ROW_UP, modstate, key ) ) {
        menu_nav_up ( state );
        return 1;
    }
    else if ( abe_test_action ( ROW_TAB, modstate, key ) ) {
        if ( state->filtered_lines == 1 ) {
            state->retv               = MENU_OK;
            *( state->selected_line ) = state->line_map[state->selected];
            state->quit               = 1;
            return 1;
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
        return 1;
    }
    // Down, Ctrl-n
    else if ( abe_test_action ( ROW_DOWN, modstate, key ) ) {
        menu_nav_down ( state );
        return 1;
    }
    else if ( abe_test_action ( ROW_LEFT, modstate, key ) ) {
        menu_nav_left ( state );
        return 1;
    }
    else if ( abe_test_action ( ROW_RIGHT, modstate, key ) ) {
        menu_nav_right ( state );
        return 1;
    }
    else if ( abe_test_action ( PAGE_PREV, modstate, key ) ) {
        menu_nav_page_prev ( state );
        return 1;
    }
    else if ( abe_test_action ( PAGE_NEXT, modstate, key ) ) {
        menu_nav_page_next ( state );
        return 1;
    }
    else if  ( abe_test_action ( ROW_FIRST, modstate, key ) ) {
        menu_nav_first ( state );
        return 1;
    }
    else if ( abe_test_action ( ROW_LAST, modstate, key ) ) {
        menu_nav_last ( state );
        return 1;
    }
    else if ( abe_test_action ( ROW_SELECT, modstate, key ) ) {
        // If a valid item is selected, return that..
        if ( state->selected < state->filtered_lines ) {
            int  st;
            char *str = NULL;
            if ( state->sw->get_completion ) {
                str = state->sw->get_completion ( state->sw, state->line_map[state->selected] );
            }
            else {
                str = state->sw->mgrv ( state->sw, state->line_map[state->selected], &st, TRUE );
            }
            textbox_text ( state->text, str );
            g_free ( str );
            textbox_cursor_end ( state->text );
            state->update   = TRUE;
            state->refilter = TRUE;
        }
        return 1;
    }
    state->prev_key = key;
    return 0;
}

/**
 * @param state Internal state of the menu.
 * @param xbe   The mouse button press event.
 *
 * mouse navigation through the elements.
 *
 */
static int intersect ( const textbox *tb, int x, int y )
{
    if ( x >= ( tb->x ) && x < ( tb->x + tb->w ) ) {
        if ( y >= ( tb->y ) && y < ( tb->y + tb->h ) ) {
            return TRUE;
        }
    }
    return FALSE;
}
static int sb_intersect ( const scrollbar *tb, int x, int y )
{
    if ( x >= ( tb->x ) && x < ( tb->x + tb->w ) ) {
        if ( y >= ( tb->y ) && y < ( tb->y + tb->h ) ) {
            return TRUE;
        }
    }
    return FALSE;
}
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
        if ( state->scrollbar && sb_intersect ( state->scrollbar, xbe->x, xbe->y ) ) {
            state->selected = scrollbar_clicked ( state->scrollbar, xbe->y );
            state->update   = TRUE;
            return;
        }
        for ( unsigned int i = 0; config.sidebar_mode == TRUE && i < num_modi; i++ ) {
            if ( intersect ( modi[i].tb, xbe->x, xbe->y ) ) {
                *( state->selected_line ) = 0;
                state->retv               = MENU_QUICK_SWITCH | ( i & MENU_LOWER_MASK );
                state->quit               = TRUE;
                state->skip_absorb        = TRUE;
                return;
            }
        }
        for ( unsigned int i = 0; i < state->max_elements; i++ ) {
            if ( intersect ( state->boxes[i], xbe->x, xbe->y ) ) {
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

typedef struct _thread_state
{
    MenuState    *state;
    char         **tokens;
    unsigned int start;
    unsigned int stop;
    unsigned int count;
    GCond        *cond;
    GMutex       *mutex;
    unsigned int *acount;
    void ( *callback )( struct _thread_state *t, gpointer data );
}thread_state;

static void filter_elements ( thread_state *t, G_GNUC_UNUSED gpointer user_data )
{
    // input changed
    for ( unsigned int i = t->start; i < t->stop; i++ ) {
        int st;
        int match = t->state->sw->token_match ( t->state->sw, t->tokens,
                                                t->state->lines_not_ascii[i],
                                                config.case_sensitive,
                                                i );
        // If each token was matched, add it to list.
        if ( match ) {
            t->state->line_map[t->start + t->count] = i;
            if ( config.levenshtein_sort ) {
                // This is inefficient, need to fix it.
                char * str = t->state->sw->mgrv ( t->state->sw, i, &st, TRUE );
                t->state->distance[i] = levenshtein ( t->state->text->text, str );
                g_free ( str );
            }
            t->count++;
        }
    }
    g_mutex_lock ( t->mutex );
    ( *( t->acount ) )--;
    g_cond_signal ( t->cond );
    g_mutex_unlock ( t->mutex );
}
static void check_is_ascii ( thread_state *t, G_GNUC_UNUSED gpointer user_data )
{
    for ( unsigned int i = t->start; i < t->stop; i++ ) {
        t->state->lines_not_ascii[i] = t->state->sw->is_not_ascii ( t->state->sw, i );
    }
    g_mutex_lock ( t->mutex );
    ( *( t->acount ) )--;
    g_cond_signal ( t->cond );
    g_mutex_unlock ( t->mutex );
}
/**
 * Small wrapper function to create easy workers.
 */
static void call_thread ( gpointer data, gpointer user_data )
{
    thread_state *t = (thread_state *) data;
    t->callback ( t, user_data );
}

static void menu_refilter ( MenuState *state )
{
    TICK_N ( "Filter start" );
    if ( strlen ( state->text->text ) > 0 ) {
        unsigned int j        = 0;
        char         **tokens = tokenize ( state->text->text, config.case_sensitive );
        /**
         * On long lists it can be beneficial to parallelize.
         * If number of threads is 1, no thread is spawn.
         * If number of threads > 1 and there are enough (> 1000) items, spawn jobs for the thread pool.
         * For large lists with 8 threads I see a factor three speedup of the whole function.
         */
        unsigned int nt = MAX ( 1, state->num_lines / 500 );
        thread_state states[nt];
        GCond        cond;
        GMutex       mutex;
        g_mutex_init ( &mutex );
        g_cond_init ( &cond );
        unsigned int count = nt;
        unsigned int steps = ( state->num_lines + nt ) / nt;
        for ( unsigned int i = 0; i < nt; i++ ) {
            states[i].state    = state;
            states[i].tokens   = tokens;
            states[i].start    = i * steps;
            states[i].stop     = MIN ( state->num_lines, ( i + 1 ) * steps );
            states[i].count    = 0;
            states[i].cond     = &cond;
            states[i].mutex    = &mutex;
            states[i].acount   = &count;
            states[i].callback = filter_elements;
            if ( i > 0 ) {
                g_thread_pool_push ( tpool, &states[i], NULL );
            }
        }
        // Run one in this thread.
        filter_elements ( &states[0], NULL );
        // No need to do this with only one thread.
        if ( nt > 1 ) {
            g_mutex_lock ( &mutex );
            while ( count > 0 ) {
                g_cond_wait ( &cond, &mutex );
            }
            g_mutex_unlock ( &mutex );
        }
        g_cond_clear ( &cond );
        g_mutex_clear ( &mutex );
        for ( unsigned int i = 0; i < nt; i++ ) {
            if ( j != states[i].start ) {
                memcpy ( &( state->line_map[j] ), &( state->line_map[states[i].start] ), sizeof ( unsigned int ) * ( states[i].count ) );
            }
            j += states[i].count;
        }
        if ( config.levenshtein_sort ) {
            g_qsort_with_data ( state->line_map, j, sizeof ( int ), lev_sort, state->distance );
        }

        // Cleanup + bookkeeping.
        state->filtered_lines = j;
        tokenize_free ( tokens );
    }
    else{
        for ( unsigned int i = 0; i < state->num_lines; i++ ) {
            state->line_map[i] = i;
        }
        state->filtered_lines = state->num_lines;
    }
    if ( state->filtered_lines > 0 ) {
        state->selected = MIN ( state->selected, state->filtered_lines - 1 );
    }
    else {
        state->selected = 0;
    }

    if ( config.auto_select == TRUE && state->filtered_lines == 1 && state->num_lines > 1 ) {
        *( state->selected_line ) = state->line_map[state->selected];
        state->retv               = MENU_OK;
        state->quit               = TRUE;
    }

    scrollbar_set_max_value ( state->scrollbar, state->filtered_lines );
    state->refilter = FALSE;
    state->rchanged = TRUE;
    TICK_N ( "Filter done" );
}

static void menu_draw ( MenuState *state, cairo_t *d )
{
    unsigned int i, offset = 0;
    // selected row is always visible.
    // If selected is visible do not scroll.
    if ( ( ( state->selected - ( state->last_offset ) ) < ( state->max_elements ) ) && ( state->selected >= ( state->last_offset ) ) ) {
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
        // Set the position
        scrollbar_set_handle ( state->scrollbar, page * state->max_elements );
    }
    // Re calculate the boxes and sizes, see if we can move this in the menu_calc*rowscolumns
    // Get number of remaining lines to display.
    unsigned int a_lines = MIN ( ( state->filtered_lines - offset ), state->max_elements );

    // Calculate number of columns
    unsigned int columns = ( a_lines + ( state->max_rows - a_lines % state->max_rows ) % state->max_rows ) / state->max_rows;
    columns = MIN ( columns, state->columns );

    // Update the handle length.
    scrollbar_set_handle_length ( state->scrollbar, columns * state->max_rows );
    scrollbar_draw ( state->scrollbar, d );
    // Element width.
    unsigned int element_width = state->w - ( 2 * ( state->border ) );
    if ( state->scrollbar != NULL ) {
        element_width -= state->scrollbar->w;
    }
    if ( columns > 0 ) {
        element_width = ( element_width - ( columns - 1 ) * config.line_margin ) / columns;
    }

    int          element_height = state->line_height * config.element_height;
    int          y_offset       = state->top_offset;
    int          x_offset       = state->border;
    // Calculate number of visible rows.
    unsigned int max_elements = MIN ( a_lines, state->max_rows * columns );

    if ( state->rchanged ) {
        // Move, resize visible boxes and show them.
        for ( i = 0; i < max_elements; i++ ) {
            unsigned int ex = ( ( i ) / state->max_rows ) * ( element_width + config.line_margin );
            unsigned int ey = ( ( i ) % state->max_rows ) * ( element_height + config.line_margin );
            // Move it around.
            textbox_moveresize ( state->boxes[i], ex + x_offset, ey + y_offset, element_width, element_height );
            {
                TextBoxFontType type   = ( ( ( i % state->max_rows ) & 1 ) == 0 ) ? NORMAL : ALT;
                int             fstate = 0;
                char            *text  = state->sw->mgrv ( state->sw, state->line_map[i + offset], &fstate, TRUE );
                TextBoxFontType tbft   = fstate | ( ( i + offset ) == state->selected ? HIGHLIGHT : type );
                textbox_font ( state->boxes[i], tbft );
                textbox_text ( state->boxes[i], text );
                g_free ( text );
            }
            textbox_draw ( state->boxes[i], d );
        }
        state->rchanged = FALSE;
    }
    else{
        // Only do basic redrawing + highlight of row.
        for ( i = 0; i < max_elements; i++ ) {
            TextBoxFontType type   = ( ( ( i % state->max_rows ) & 1 ) == 0 ) ? NORMAL : ALT;
            int             fstate = 0;
            state->sw->mgrv ( state->sw, state->line_map[i + offset], &fstate, FALSE );
            TextBoxFontType tbft = fstate | ( ( i + offset ) == state->selected ? HIGHLIGHT : type );
            textbox_font ( state->boxes[i], tbft );
            textbox_draw ( state->boxes[i], d );
        }
    }
}

static void menu_update ( MenuState *state )
{
    TICK ();
    cairo_surface_t * surf = cairo_image_surface_create ( get_format (), state->w, state->h );
    cairo_t         *d     = cairo_create ( surf );
    cairo_set_operator ( d, CAIRO_OPERATOR_SOURCE );
    if ( config.fake_transparency ) {
        if ( fake_bg != NULL ) {
            cairo_set_source_surface ( d, fake_bg,
                                       -(double) ( state->x - state->mon.x ),
                                       -(double) ( state->y - state->mon.y ) );
            cairo_paint ( d );
            cairo_set_operator ( d, CAIRO_OPERATOR_OVER );
            color_background ( display, d );
            cairo_paint ( d );
        }
    }
    else {
        // Paint the background.
        color_background ( display, d );
        cairo_paint ( d );
    }
    TICK_N ( "Background" );
    color_border ( display, d );

    if ( config.menu_bw > 0 ) {
        cairo_save ( d );
        cairo_set_line_width ( d, config.menu_bw );
        cairo_rectangle ( d,
                          config.menu_bw / 2.0,
                          config.menu_bw / 2.0,
                          state->w - config.menu_bw,
                          state->h - config.menu_bw );
        cairo_stroke ( d );
        cairo_restore ( d );
    }

    // Always paint as overlay over the background.
    cairo_set_operator ( d, CAIRO_OPERATOR_OVER );
    if ( state->max_elements > 0 ) {
        menu_draw ( state, d );
    }
    if ( state->prompt_tb ) {
        textbox_draw ( state->prompt_tb, d );
    }
    if ( state->text ) {
        textbox_draw ( state->text, d );
    }
    if ( state->case_indicator ) {
        textbox_draw ( state->case_indicator, d );
    }
    if ( state->message_tb ) {
        textbox_draw ( state->message_tb, d );
    }
    color_separator ( display, d );

    if ( strcmp ( config.separator_style, "none" ) ) {
        if ( strcmp ( config.separator_style, "dash" ) == 0 ) {
            const double dashes[1] = { 4 };
            cairo_set_dash ( d, dashes, 1, 0.0 );
        }
        cairo_move_to ( d, state->border, state->line_height + ( state->border ) * 1 + config.line_margin + 1 );
        cairo_line_to ( d, state->w - state->border, state->line_height + ( state->border ) * 1 + config.line_margin + 1 );
        cairo_stroke ( d );
        if ( state->message_tb ) {
            cairo_move_to ( d, state->border, state->top_offset - ( config.line_margin ) - 1 );
            cairo_line_to ( d, state->w - state->border, state->top_offset - ( config.line_margin ) - 1 );
            cairo_stroke ( d );
        }

        if ( config.sidebar_mode == TRUE ) {
            cairo_move_to ( d, state->border, state->h - state->line_height - ( state->border ) * 1 - 1 - config.line_margin );
            cairo_line_to ( d, state->w - state->border, state->h - state->line_height - ( state->border ) * 1 - 1 - config.line_margin );
            cairo_stroke ( d );
        }
    }
    if ( config.sidebar_mode == TRUE ) {
        for ( unsigned int j = 0; j < num_modi; j++ ) {
            if ( modi[j].tb != NULL ) {
                textbox_draw ( modi[j].tb, d );
            }
        }
    }
    state->update = FALSE;

    // Draw to actual window.
    cairo_set_source_surface ( draw, surf, 0, 0 );
    cairo_paint ( draw );
    // Cleanup
    cairo_destroy ( d );
    cairo_surface_destroy ( surf );

    // Flush the surface.
    cairo_surface_flush ( surface );
    TICK ();
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
            textbox_insert ( state->text, state->text->cursor, text, dl );
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
    unsigned int sbw = config.line_margin + 8;
    scrollbar_move ( state->scrollbar, state->w - state->border - sbw, state->top_offset );
    if ( config.sidebar_mode == TRUE ) {
        int width = ( state->w - ( 2 * ( state->border ) + ( num_modi - 1 ) * config.line_margin ) ) / num_modi;
        for ( unsigned int j = 0; j < num_modi; j++ ) {
            textbox_moveresize ( modi[j].tb,
                                 state->border + j * ( width + config.line_margin ), state->h - state->line_height - state->border,
                                 width, state->line_height );
            textbox_draw ( modi[j].tb, draw );
        }
    }
    int entrybox_width = state->w - ( 2 * ( state->border ) ) - textbox_get_width ( state->prompt_tb )
                         - textbox_get_width ( state->case_indicator );
    textbox_moveresize ( state->text, state->text->x, state->text->y, entrybox_width, state->line_height );
    textbox_move ( state->case_indicator, state->w - state->border - textbox_get_width ( state->case_indicator ), state->border );
    /**
     * Resize in Height
     */
    {
        unsigned int last_length    = state->max_elements;
        int          element_height = state->line_height * config.element_height + config.line_margin;
        // Calculated new number of boxes.
        int          h = ( state->h - state->top_offset - config.padding );
        if ( config.sidebar_mode == TRUE ) {
            h -= state->line_height + config.line_margin;
        }
        if ( h < 0 ) {
            fprintf ( stderr, "Current padding %u (on each side) does not fit within visible window %u.\n", config.padding, state->h );
            h = ( state->h - state->top_offset - state->h / 3 );
            if ( config.sidebar_mode == TRUE ) {
                h -= state->line_height + config.line_margin;
            }
        }
        state->max_rows     = MAX ( 1, ( h / element_height ) );
        state->max_elements = state->max_rows * config.menu_columns;
        // Free boxes no longer needed.
        for ( unsigned int i = state->max_elements; i < last_length; i++ ) {
            textbox_free ( state->boxes[i] );
        }
        // resize array.
        state->boxes = g_realloc ( state->boxes, state->max_elements * sizeof ( textbox* ) );

        int y_offset = state->top_offset;
        int x_offset = state->border;
        int rstate   = 0;
        if ( config.markup_rows ) {
            rstate = TB_MARKUP;
        }
        // Add newly added boxes.
        for ( unsigned int i = last_length; i < state->max_elements; i++ ) {
            state->boxes[i] = textbox_create ( rstate, x_offset, y_offset,
                                               state->element_width, element_height, NORMAL, "" );
        }
        scrollbar_resize ( state->scrollbar, -1, ( state->max_rows ) * ( element_height ) - config.line_margin );
    }

    state->rchanged = TRUE;
    state->update   = TRUE;
}

static void menu_setup_fake_transparency ( Display *display, MenuState *state )
{
    if ( fake_bg == NULL ) {
        Window          root   = DefaultRootWindow ( display );
        int             screen = DefaultScreen ( display );
        cairo_surface_t *s     = cairo_xlib_surface_create ( display,
                                                             root,
                                                             DefaultVisual ( display, screen ),
                                                             DisplayWidth ( display, screen ),
                                                             DisplayHeight ( display, screen ) );

        fake_bg = cairo_image_surface_create ( get_format (), state->mon.w, state->mon.h );
        cairo_t *dr = cairo_create ( fake_bg );
        cairo_set_source_surface ( dr, s, -state->mon.x, -state->mon.y );
        cairo_paint ( dr );
        cairo_destroy ( dr );
        cairo_surface_destroy ( s );
        TICK_N ( "Fake transparency" );
    }
}

MenuReturn menu ( Mode *sw, char **input, char *prompt, unsigned int *selected_line, unsigned int *next_pos, const char *message )
{
    TICK ();
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
        .top_offset =             0,
        .border     = config.padding + config.menu_bw
    };
    // Request the lines to show.
    state.num_lines       = sw->get_num_entries ( sw );
    state.lines_not_ascii = g_malloc0_n ( state.num_lines, sizeof ( int ) );

    // find out which lines contain non-ascii codepoints, so we can be faster in some cases.
    if ( state.num_lines > 0 ) {
        TICK_N ( "Is ASCII start" );
        unsigned int nt = MAX ( 1, state.num_lines / 5000 );
        thread_state states[nt];
        unsigned int steps = ( state.num_lines + nt ) / nt;
        unsigned int count = nt;
        GCond        cond;
        GMutex       mutex;
        g_mutex_init ( &mutex );
        g_cond_init ( &cond );
        for ( unsigned int i = 0; i < nt; i++ ) {
            states[i].state    = &state;
            states[i].start    = i * steps;
            states[i].stop     = MIN ( ( i + 1 ) * steps, state.num_lines );
            states[i].acount   = &count;
            states[i].mutex    = &mutex;
            states[i].cond     = &cond;
            states[i].callback = check_is_ascii;
            if ( i > 0 ) {
                g_thread_pool_push ( tpool, &( states[i] ), NULL );
            }
        }
        // Run one in this thread.
        check_is_ascii ( &( states[0] ), NULL );
        // No need to do this with only one thread.
        if ( nt > 1 ) {
            g_mutex_lock ( &mutex );
            while ( count > 0 ) {
                g_cond_wait ( &cond, &mutex );
            }
            g_mutex_unlock ( &mutex );
        }
        g_cond_clear ( &cond );
        g_mutex_clear ( &mutex );
        TICK_N ( "Is ASCII stop" );
    }

    if ( next_pos ) {
        *next_pos = *selected_line;
    }
    // Try to grab the keyboard as early as possible.
    // We grab this using the rootwindow (as dmenu does it).
    // this seems to result in the smallest delay for most people.
    int has_keyboard = take_keyboard ( display, DefaultRootWindow ( display ) );

    if ( !has_keyboard ) {
        fprintf ( stderr, "Failed to grab keyboard, even after %d uS.", 500 * 1000 );
        // Break off.
        return MENU_CANCEL;
    }
    TICK_N ( "Grab keyboard" );
    // main window isn't explicitly destroyed in case we switch modes. Reusing it prevents flicker
    XWindowAttributes attr;
    if ( main_window == None || XGetWindowAttributes ( display, main_window, &attr ) == 0 ) {
        main_window = create_window ( display );
        if ( sncontext != NULL ) {
            sn_launchee_context_setup_window ( sncontext, main_window );
        }
    }
    TICK_N ( "Startup notification" );
    // Get active monitor size.
    monitor_active ( display, &( state.mon ) );
    TICK_N ( "Get active monitor" );
    if ( config.fake_transparency ) {
        menu_setup_fake_transparency ( display, &state );
    }

    // we need this at this point so we can get height.
    state.line_height    = textbox_get_estimated_char_height ();
    state.case_indicator = textbox_create ( TB_AUTOWIDTH, ( state.border ), ( state.border ),
                                            0, state.line_height, NORMAL, "*" );
    // Height of a row.
    if ( config.menu_lines == 0 ) {
        // Autosize it.
        int h = state.mon.h - state.border * 2 - config.line_margin;
        int r = ( h ) / ( state.line_height * config.element_height ) - 1 - config.sidebar_mode;
        state.menu_lines = r;
    }
    else {
        state.menu_lines = config.menu_lines;
    }
    menu_calculate_rows_columns ( &state );
    menu_calculate_window_and_element_width ( &state, &( state.mon ) );

    // Prompt box.
    state.prompt_tb = textbox_create ( TB_AUTOWIDTH, ( state.border ), ( state.border ),
                                       0, state.line_height, NORMAL, prompt );
    // Entry box
    int entrybox_width = state.w - ( 2 * ( state.border ) ) - textbox_get_width ( state.prompt_tb )
                         - textbox_get_width ( state.case_indicator );
    state.text = textbox_create ( TB_EDITABLE,
                                  ( state.border ) + textbox_get_width ( state.prompt_tb ), ( state.border ),
                                  entrybox_width, state.line_height, NORMAL, *input );

    state.top_offset = state.border * 1 + state.line_height + 2 + config.line_margin * 2;

    // Move indicator to end.
    textbox_move ( state.case_indicator, state.border + textbox_get_width ( state.prompt_tb ) + entrybox_width, state.border );

    if ( config.case_sensitive ) {
        textbox_text ( state.case_indicator, "*" );
    }
    else{
        textbox_text ( state.case_indicator, " " );
    }
    state.message_tb = NULL;
    if ( message ) {
        state.message_tb = textbox_create ( TB_AUTOHEIGHT | TB_MARKUP | TB_WRAP,
                                            ( state.border ), state.top_offset, state.w - ( 2 * ( state.border ) ),
                                            -1, NORMAL, message );
        state.top_offset += textbox_get_height ( state.message_tb );
        state.top_offset += config.line_margin * 2 + 2;
    }

    int element_height = state.line_height * config.element_height;
    // filtered list display
    state.boxes = g_malloc0_n ( state.max_elements, sizeof ( textbox* ) );

    int y_offset = state.top_offset;
    int x_offset = state.border;

    int rstate = 0;
    if ( config.markup_rows ) {
        rstate = TB_MARKUP;
    }
    for ( unsigned int i = 0; i < state.max_elements; i++ ) {
        state.boxes[i] = textbox_create ( rstate, x_offset, y_offset,
                                          state.element_width, element_height, NORMAL, "" );
    }
    if ( !config.hide_scrollbar ) {
        unsigned int sbw = config.line_margin + config.scrollbar_width;
        state.scrollbar = scrollbar_create ( state.w - state.border - sbw, state.top_offset,
                                             sbw, ( state.max_rows - 1 ) * ( element_height + config.line_margin ) + element_height );
    }

    scrollbar_set_max_value ( state.scrollbar, state.num_lines );
    // filtered list
    state.line_map = g_malloc0_n ( state.num_lines, sizeof ( unsigned int ) );
    state.distance = (int *) g_malloc0_n ( state.num_lines, sizeof ( int ) );

    // resize window vertically to suit
    // Subtract the margin of the last row.
    state.h  = state.top_offset + ( element_height + config.line_margin ) * ( state.max_rows ) - config.line_margin;
    state.h += state.border;
    state.h += 0;
    // Add entry
    if ( config.sidebar_mode == TRUE ) {
        state.h += state.line_height + 2 * config.line_margin + 2;
    }

    // Sidebar mode.
    if ( config.menu_lines == 0 ) {
        state.h = state.mon.h;
    }

    // Move the window to the correct x,y position.
    calculate_window_position ( &state );

    if ( config.sidebar_mode == TRUE ) {
        int width = ( state.w - ( 2 * ( state.border ) + ( num_modi - 1 ) * config.line_margin ) ) / num_modi;
        for ( unsigned int j = 0; j < num_modi; j++ ) {
            modi[j].tb = textbox_create ( TB_CENTER, state.border + j * ( width + config.line_margin ),
                                          state.h - state.line_height - state.border, width, state.line_height,
                                          ( j == curr_switcher ) ? HIGHLIGHT : NORMAL, modi[j].sw->name );
        }
    }

    // Display it.
    XMoveResizeWindow ( display, main_window, state.x, state.y, state.w, state.h );
    cairo_xlib_surface_set_size ( surface, state.w, state.h );
    XMapRaised ( display, main_window );

    // if grabbing keyboard failed, fall through
    state.selected = 0;

    state.quit   = FALSE;
    state.update = TRUE;
    menu_refilter ( &state );

    for ( unsigned int i = 0; ( *( state.selected_line ) ) < UINT32_MAX && !state.selected && i < state.filtered_lines; i++ ) {
        if ( state.line_map[i] == *( state.selected_line ) ) {
            state.selected = i;
            break;
        }
    }

    if ( sncontext != NULL ) {
        sn_launchee_context_complete ( sncontext );
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
                menu_refilter ( &state );
                menu_update ( &state );
            }
        }
        else if ( mle == ML_TIMEOUT ) {
            // When timeout (and in lazy filter mode)
            // We refilter then loop back and wait for Xevent.
            if ( state.refilter ) {
                menu_refilter ( &state );
                menu_update ( &state );
            }
        }
        if ( mle == ML_TIMEOUT ) {
            continue;
        }
        // Get next event. (might block)
        XNextEvent ( display, &ev );
        TICK_N ( "X Event" );
        if ( sndisplay != NULL ) {
            sn_display_process_event ( sndisplay, &ev );
        }
        if ( ev.type == KeymapNotify ) {
            XRefreshKeyboardMapping ( &ev.xmapping );
        }
        else if ( ev.type == ConfigureNotify ) {
            XConfigureEvent xce = ev.xconfigure;
            if ( xce.window == main_window ) {
                if ( state.x != (int ) xce.x || state.y != (int) xce.y ) {
                    state.x      = xce.x;
                    state.y      = xce.y;
                    state.update = TRUE;
                }
                if ( state.w != (unsigned int) xce.width || state.h != (unsigned int ) xce.height ) {
                    state.w = xce.width;
                    state.h = xce.height;
                    cairo_xlib_surface_set_size ( surface, state.w, state.h );
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
        else if ( ev.type == MotionNotify ) {
            while ( XCheckTypedEvent ( display, MotionNotify, &ev ) ) {
                ;
            }
            XMotionEvent xme = ev.xmotion;
            if ( xme.x >= state.scrollbar->x && xme.x < ( state.scrollbar->x + state.scrollbar->w ) ) {
                state.selected = scrollbar_clicked ( state.scrollbar, xme.y );
                state.update   = TRUE;
            }
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
                // This is needed for letting the Input Method handle combined keys.
                // E.g. `e into è
                if ( XFilterEvent ( &ev, main_window ) ) {
                    continue;
                }
                Status stat;
                char   pad[32];
                KeySym key; // = XkbKeycodeToKeysym ( display, ev.xkey.keycode, 0, 0 );
                int    len = Xutf8LookupString ( xic, &( ev.xkey ), pad, sizeof ( pad ), &key, &stat );
                pad[len] = 0;
                if ( stat == XLookupKeySym || stat == XLookupBoth ) {
                    // Handling of paste
                    if ( abe_test_action ( PASTE_PRIMARY, ev.xkey.state, key ) ) {
                        XConvertSelection ( display, XA_PRIMARY, netatoms[UTF8_STRING], netatoms[UTF8_STRING], main_window, CurrentTime );
                    }
                    else if ( abe_test_action ( PASTE_SECONDARY, ev.xkey.state, key ) ) {
                        XConvertSelection ( display, netatoms[CLIPBOARD], netatoms[UTF8_STRING], netatoms[UTF8_STRING], main_window,
                                            CurrentTime );
                    }
                    if ( abe_test_action ( SCREENSHOT, ev.xkey.state, key ) ) {
                        menu_capture_screenshot ( );
                        break;
                    }
                    if ( abe_test_action ( TOGGLE_SORT, ev.xkey.state, key ) ) {
                        config.levenshtein_sort = !config.levenshtein_sort;
                        state.refilter          = TRUE;
                        state.update            = TRUE;
                        break;
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
                            textbox_text ( state.case_indicator, "*" );
                        }
                        else {
                            textbox_text ( state.case_indicator, " " );
                        }
                        break;
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
                    if ( menu_keyboard_navigation ( &state, key, ev.xkey.state ) ) {
                        continue;
                    }
                }
                {
                    // Skip if we detected key before.
                    if ( state.quit ) {
                        continue;
                    }

                    int rc = textbox_keypress ( state.text, &ev, pad, len, key, stat );
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
        if ( ( state.selected + 1 ) < state.num_lines ) {
            *( next_pos ) = state.line_map[state.selected + 1];
        }
    }

    int retv = state.retv;
    menu_free_state ( &state );

    if ( shift ) {
        retv |= MENU_SHIFT;
    }

    // Free the switcher boxes.
    // When state is free'ed we should no longer need these.
    if ( config.sidebar_mode == TRUE ) {
        for ( unsigned int j = 0; j < num_modi; j++ ) {
            textbox_free ( modi[j].tb );
            modi[j].tb = NULL;
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
        .top_offset        =           0,
        .border            = config.padding + config.menu_bw
    };

    // Try to grab the keyboard as early as possible.
    // We grab this using the rootwindow (as dmenu does it).
    // this seems to result in the smallest delay for most people.
    int has_keyboard = take_keyboard ( display, DefaultRootWindow ( display ) );

    if ( !has_keyboard ) {
        fprintf ( stderr, "Failed to grab keyboard, even after %d uS.", 500 * 1000 );
        return;
    }
    // Get active monitor size.
    monitor_active ( display, &( state.mon ) );
    if ( config.fake_transparency ) {
        menu_setup_fake_transparency ( display, &state );
    }
    // main window isn't explicitly destroyed in case we switch modes. Reusing it prevents flicker
    XWindowAttributes attr;
    if ( main_window == None || XGetWindowAttributes ( display, main_window, &attr ) == 0 ) {
        main_window = create_window ( display );
    }

    menu_calculate_window_and_element_width ( &state, &( state.mon ) );
    state.max_elements = 0;

    state.text = textbox_create ( ( TB_AUTOHEIGHT | TB_WRAP ) + ( ( markup ) ? TB_MARKUP : 0 ),
                                  ( state.border ), ( state.border ),
                                  ( state.w - ( 2 * ( state.border ) ) ), 1, NORMAL, ( msg != NULL ) ? msg : "" );
    state.line_height = textbox_get_height ( state.text );

    // resize window vertically to suit
    state.h = state.line_height + ( state.border ) * 2;

    // Move the window to the correct x,y position.
    calculate_window_position ( &state );
    XMoveResizeWindow ( display, main_window, state.x, state.y, state.w, state.h );
    cairo_xlib_surface_set_size ( surface, state.w, state.h );
    // Display it.
    XMapRaised ( display, main_window );

    if ( sncontext != NULL ) {
        sn_launchee_context_complete ( sncontext );
    }
    while ( !state.quit ) {
        // Update if requested.
        if ( state.update ) {
            menu_update ( &state );
        }
        // Wait for event.
        XEvent ev;
        XNextEvent ( display, &ev );
        if ( sndisplay != NULL ) {
            sn_display_process_event ( sndisplay, &ev );
        }
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
        textbox_setup ( display );
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

    if ( fake_bg ) {
        cairo_surface_destroy ( fake_bg );
        fake_bg = NULL;
    }
    if ( draw ) {
        cairo_destroy ( draw );
        draw = NULL;
    }
    if ( surface ) {
        cairo_surface_destroy ( surface );
        surface = NULL;
    }
    if ( main_window != None ) {
        XUnmapWindow ( display, main_window );
        XDestroyWindow ( display, main_window );
        main_window = None;
        XDestroyIC ( xic );
        XCloseIM ( xim );
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

static void run_switcher ( ModeMode mode )
{
    int pfd = setup ();
    if ( pfd < 0 ) {
        return;
    }
    // Otherwise check if requested mode is enabled.
    char *input = g_strdup ( config.filter );
    for ( unsigned int i = 0; i < num_modi; i++ ) {
        modi[i].sw->init ( modi[i].sw );
    }
    do {
        ModeMode retv;

        curr_switcher = mode;
        retv          = switcher_run ( &input, modi[mode].sw );
        // Find next enabled
        if ( retv == NEXT_DIALOG ) {
            mode = ( mode + 1 ) % num_modi;
        }
        else if ( retv == PREVIOUS_DIALOG ) {
            if ( mode == 0 ) {
                mode = num_modi - 1;
            }
            else {
                mode = ( mode - 1 ) % num_modi;
            }
        }
        else if ( retv == RELOAD_DIALOG ) {
            // do nothing.
        }
        else if ( retv < MODE_EXIT ) {
            mode = ( retv ) % num_modi;
        }
        else {
            mode = retv;
        }
    } while ( mode != MODE_EXIT );
    g_free ( input );
    for ( unsigned int i = 0; i < num_modi; i++ ) {
        modi[i].sw->destroy ( modi[i].sw );
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
        fprintf ( stderr,
                  "Warning: Unhandled keypress in global keyhandler, keycode = %u mask = %u\n",
                  ev->xkey.keycode,
                  ev->xkey.state );
    }
}

/**
 * Help function.
 */
static void print_main_application_options ( void )
{
    int is_term = isatty ( fileno ( stdout ) );
    print_help_msg ( "-no-config", "", "Do not load configuration, use default values.", NULL, is_term );
    print_help_msg ( "-quiet", "", "Suppress information messages.", NULL, is_term );
    print_help_msg ( "-v,-version", "", "Print the version number and exit.", NULL, is_term  );
    print_help_msg ( "-dmenu", "", "Start in dmenu mode.", NULL, is_term );
    print_help_msg ( "-display", "[string]", "X server to contact.", "${DISPLAY}", is_term );
    print_help_msg ( "-h,-help", "", "This help message.", NULL, is_term );
    print_help_msg ( "-dump-xresources", "", "Dump the current configuration in Xresources format and exit.", NULL, is_term );
    print_help_msg ( "-dump-xresources-theme", "", "Dump the current color scheme in Xresources format and exit.", NULL, is_term );
    print_help_msg ( "-e", "[string]", "Show a dialog displaying the passed message and exit.", NULL, is_term );
    print_help_msg ( "-markup", "", "Enable pango markup where possible.", NULL, is_term );
    print_help_msg ( "-normal-window", "", "In dmenu mode, behave as a normal window. (experimental)", NULL, is_term );
    print_help_msg ( "-show", "[mode]", "Show the mode 'mode' and exit. The mode has to be enabled.", NULL, is_term );
}
static void help ( G_GNUC_UNUSED int argc, char **argv )
{
    printf ( "%s usage:\n", argv[0] );
    printf ( "\t%s [-options ...]\n\n", argv[0] );
    printf ( "Command line only options:\n" );
    print_main_application_options ();
    printf ( "DMENU command line options:\n" );
    print_dmenu_options ();
    printf ( "Global options:\n" );
    print_options ();
    printf ( "\n" );
    printf ( "For more information see: man rofi\n" );
    printf ( "Version:    "VERSION "\n" );
    printf ( "Bugreports: "PACKAGE_BUGREPORT "\n" );
}

/**
 * Function bound by 'atexit'.
 * Cleanup globally allocated memory.
 */
static void cleanup ()
{
    if ( tpool ) {
        g_thread_pool_free ( tpool, TRUE, FALSE );
        tpool = NULL;
    }
    // Cleanup
    if ( display != NULL ) {
        if ( main_window != None ) {
            // We should never hit this code.
            release_keyboard ( display );
            XDestroyWindow ( display, main_window );
            main_window = None;
            XDestroyIC ( xic );
            XCloseIM ( xim );
        }

        if ( sncontext != NULL ) {
            sn_launchee_context_unref ( sncontext );
            sncontext = NULL;
        }
        if ( sndisplay != NULL ) {
            sn_display_unref ( sndisplay );
            sndisplay = NULL;
        }
        XCloseDisplay ( display );
        display = NULL;
    }

    // Cleaning up memory allocated by the Xresources file.
    config_xresource_free ();
    for ( unsigned int i = 0; i < num_modi; i++ ) {
        // Mode keystr is free'ed when needed by config system.
        if ( modi[i].sw->keycfg != NULL ) {
            g_free ( modi[i].sw->keycfg );
            modi[i].sw->keycfg = NULL;
        }
        // only used for script dialog.
        if ( modi[i].sw->free != NULL ) {
            modi[i].sw->free ( modi[i].sw );
        }
    }
    g_free ( modi );

    // Cleanup the custom keybinding
    cleanup_abe ();

    TIMINGS_STOP ();
}

/**
 * Parse the switcher string, into internal array of type Mode.
 *
 * String is split on separator ','
 * First the three build-in modi are checked: window, run, ssh
 * if that fails, a script-switcher is created.
 */
static void setup_modi ( void )
{
    char *savept = NULL;
    // Make a copy, as strtok will modify it.
    char *switcher_str = g_strdup ( config.modi );
    // Split token on ','. This modifies switcher_str.
    for ( char *token = strtok_r ( switcher_str, ",", &savept ); token != NULL; token = strtok_r ( NULL, ",", &savept ) ) {
        // Resize and add entry.
        modi              = (ModeHolder *) g_realloc ( modi, sizeof ( ModeHolder ) * ( num_modi + 1 ) );
        modi[num_modi].tb = NULL;

        // Window switcher.
        #ifdef WINDOW_MODE
        if ( strcasecmp ( token, "window" ) == 0 ) {
            modi[num_modi].sw = &window_mode;
            num_modi++;
        }
        else if ( strcasecmp ( token, "windowcd" ) == 0 ) {
            modi[num_modi].sw = &window_mode_cd;
            num_modi++;
        }
        else
        #endif // WINDOW_MODE
        // SSh dialog
        if ( strcasecmp ( token, "ssh" ) == 0 ) {
            modi[num_modi].sw = &ssh_mode;
            num_modi++;
        }
        // Run dialog
        else if ( strcasecmp ( token, "run" ) == 0 ) {
            modi[num_modi].sw = &run_mode;
            num_modi++;
        }
        else if ( strcasecmp ( token, "drun" ) == 0 ) {
            modi[num_modi].sw = &drun_mode;
            num_modi++;
        }
        // combi dialog
        else if ( strcasecmp ( token, "combi" ) == 0 ) {
            modi[num_modi].sw = &combi_mode;
            num_modi++;
        }
        else {
            // If not build in, use custom modi.
            Mode *sw = script_switcher_parse_setup ( token );
            if ( sw != NULL ) {
                modi[num_modi].sw = sw;
                num_modi++;
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
    for ( unsigned int i = 0; i < num_modi; i++ ) {
        modi[i].sw->keycfg = g_strdup_printf ( "key-%s", modi[i].sw->name );
        config_parser_add_option ( xrm_String, modi[i].sw->keycfg, (void * *) &( modi[i].sw->keystr ), "Keybinding" );
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
    for ( unsigned int i = 0; i < num_modi; i++ ) {
        if ( modi[i].sw->keystr != NULL ) {
            // No need to parse key, this should be done when grabbing.
            if ( modi[i].sw->keysym != NoSymbol ) {
                x11_ungrab_key ( display, modi[i].sw->modmask, modi[i].sw->keysym );
            }
        }
    }
}
static int grab_global_keybindings ()
{
    int key_bound = FALSE;
    for ( unsigned int i = 0; i < num_modi; i++ ) {
        if ( modi[i].sw->keystr != NULL ) {
            x11_parse_key ( modi[i].sw->keystr, &( modi[i].sw->modmask ), &( modi[i].sw->keysym ) );
            if ( modi[i].sw->keysym != NoSymbol ) {
                x11_grab_key ( display, modi[i].sw->modmask, modi[i].sw->keysym );
                key_bound = TRUE;
            }
        }
    }
    return key_bound;
}
static void print_global_keybindings ()
{
    fprintf ( stdout, "listening to the following keys:\n" );
    for ( unsigned int i = 0; i < num_modi; i++ ) {
        if ( modi[i].sw->keystr != NULL ) {
            fprintf ( stdout, "\t* "color_bold "%s"color_reset " on %s\n", modi[i].sw->name, modi[i].sw->keystr );
        }
        else {
            fprintf ( stdout, "\t* "color_bold "%s"color_reset " on <unspecified>\n", modi[i].sw->name );
        }
    }
}

static void reload_configuration ()
{
    if ( find_arg ( "-no-config" ) < 0 ) {
        TICK ();
        // Reset the color cache
        color_cache_reset ();
        // We need to open a new connection to X11, otherwise we get old
        // configuration
        Display *temp_display = XOpenDisplay ( display_str );
        if ( temp_display ) {
            load_configuration ( temp_display );
            load_configuration_dynamic ( temp_display );

            // Sanity check
            config_sanity_check ( temp_display );
            parse_keys_abe ();
            XCloseDisplay ( temp_display );
        }
        else {
            fprintf ( stderr, "Failed to get a new connection to the X11 server. No point in continuing.\n" );
            abort ();
        }
        TICK_N ( "Load config" );
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
                ssize_t t = write ( pfd, "c", 1 );
                if ( t < 0 ) {
                    fprintf ( stderr, "Failed to send signal to main thread.\n" );
                }
            }
            if ( sig == SIGUSR1 ) {
                ssize_t t = write ( pfd, "i", 1 );
                if ( t < 0 ) {
                    fprintf ( stderr, "Failed to send signal to main thread.\n" );
                }
            }
            if ( sig == SIGINT ) {
                ssize_t t = write ( pfd, "q", 1 );
                if ( t < 0 ) {
                    fprintf ( stderr, "Failed to send signal to main thread.\n" );
                }
                // Close my end and exit.
                g_thread_exit ( NULL );
            }
        }
    }
    return NULL;
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
        if ( sndisplay != NULL ) {
            sn_display_process_event ( sndisplay, &ev );
        }
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

ModeMode switcher_run ( char **input, Mode *sw )
{
    char         *prompt       = g_strdup_printf ( "%s:", sw->name );
    unsigned int selected_line = UINT32_MAX;
    int          mretv         = menu ( sw, input, prompt, &selected_line, NULL, NULL );
    g_free ( prompt );
    return sw->result ( sw, mretv, input, selected_line );
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
    sigprocmask ( SIG_BLOCK, &set, NULL );
    // Create signal handler process.
    // This will use sigwaitinfo to read signals and forward them back to the main thread again.
    return g_thread_new ( "signal_process", rofi_signal_handler_process, (void *) fd );
}

static int error_trap_depth = 0;
static void error_trap_push ( G_GNUC_UNUSED SnDisplay *display, G_GNUC_UNUSED Display   *xdisplay )
{
    ++error_trap_depth;
}

static void error_trap_pop ( G_GNUC_UNUSED SnDisplay *display, Display   *xdisplay )
{
    if ( error_trap_depth == 0 ) {
        fprintf ( stderr, "Error trap underflow!\n" );
        exit ( EXIT_FAILURE );
    }

    XSync ( xdisplay, False ); /* get all errors out of the queue */
    --error_trap_depth;
}

int main ( int argc, char *argv[] )
{
    TIMINGS_START ();

    cmd_set_arguments ( argc, argv );
    // Quiet flag
    int quiet = ( find_arg ( "-quiet" ) >= 0 );
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
    TICK ();
    // Get the path to the cache dir.
    cache_dir = g_get_user_cache_dir ();

    // Create pid file path.
    const char *path = g_get_user_runtime_dir ();
    if ( path ) {
        pidfile = g_build_filename ( path, "rofi.pid", NULL );
    }
    config_parser_add_option ( xrm_String, "pid", (void * *) &pidfile, "Pidfile location" );

    TICK ();
    // Register cleanup function.
    atexit ( cleanup );

    TICK ();
    // Get DISPLAY, first env, then argument.
    display_str = getenv ( "DISPLAY" );
    find_arg_str (  "-display", &display_str );

    if ( setlocale ( LC_ALL, "" ) == NULL ) {
        fprintf ( stderr, "Failed to set locale.\n" );
        return EXIT_FAILURE;
    }

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
    TICK_N ( "Open Display" );
    // startup not.
    sndisplay = sn_display_new ( display, error_trap_push, error_trap_pop );

    if ( sndisplay != NULL ) {
        sncontext = sn_launchee_context_new_from_environment ( sndisplay, DefaultScreen ( display ) );
    }
    TICK_N ( "Startup Notification" );
    // Setup keybinding
    setup_abe ();
    TICK_N ( "Setup abe" );

    if ( find_arg ( "-no-config" ) < 0 ) {
        load_configuration ( display );
    }
    if ( !dmenu_mode ) {
        // setup_modi
        setup_modi ();
    }
    else {
        // Add dmenu options.
        config_parser_add_option ( xrm_Char, "sep", (void * *) &( config.separator ), "Element separator" );
    }
    if ( find_arg ( "-no-config" ) < 0 ) {
        // Reload for dynamic part.
        load_configuration_dynamic ( display );
    }

    x11_setup ( display );

    TICK_N ( "X11 Setup " );
    // Sanity check
    config_sanity_check ( display );
    TICK_N ( "Config sanity check" );
    // Dump.
    // catch help request
    if ( find_arg (  "-h" ) >= 0 || find_arg (  "-help" ) >= 0 || find_arg (  "--help" ) >= 0 ) {
        help ( argc, argv );
        exit ( EXIT_SUCCESS );
    }
    if ( find_arg (  "-dump-xresources" ) >= 0 ) {
        xresource_dump ();
        exit ( EXIT_SUCCESS );
    }
    if ( find_arg (  "-dump-xresources-theme" ) >= 0 ) {
        print_xresources_theme ();
        exit ( EXIT_SUCCESS );
    }
    // Parse the keybindings.
    parse_keys_abe ();
    TICK_N ( "Parse ABE" );
    char *msg = NULL;
    if ( find_arg_str (  "-e", &( msg ) ) ) {
        int markup = FALSE;
        if ( find_arg ( "-markup" ) >= 0 ) {
            markup = TRUE;
        }
        return show_error_message ( msg, markup );
    }

    // Create thread pool
    GError *error = NULL;
    tpool = g_thread_pool_new ( call_thread, NULL, config.threads, FALSE, &error );
    if ( error == NULL ) {
        // Idle threads should stick around for a max of 60 seconds.
        g_thread_pool_set_max_idle_time ( 60000 );
        // We are allowed to have
        g_thread_pool_set_max_threads ( tpool, config.threads, &error );
    }
    // If error occured during setup of pool, tell user and exit.
    if ( error != NULL ) {
        char *msg = g_strdup_printf ( "Failed to setup thread pool: '%s'", error->message );
        show_error_message ( msg, FALSE );
        g_free ( msg );
        g_error_free ( error );
        return EXIT_FAILURE;
    }

    TICK_N ( "Setup Threadpool" );
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
            for ( unsigned int i = 0; i < num_modi; i++ ) {
                fprintf ( stderr, "\t* "color_bold "%s"color_reset ": -key-%s <key>\n", modi[i].sw->name, modi[i].sw->name );
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

        // done starting deamon.

        if ( sncontext != NULL ) {
            sn_launchee_context_complete ( sncontext );
        }
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
                    char    c;
                    ssize_t r = read ( pfds[0], &c, 1 );
                    if ( r < 0 ) {
                        fprintf ( stderr, "Failed to read data from signal thread: %s\n", strerror ( errno ) );
                    }
                    // Process the signal in the main_loop.
                    else if ( main_loop_signal_handler ( c, quiet ) ) {
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
