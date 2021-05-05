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

/** The Rofi View log domain */
#define G_LOG_DOMAIN    "View"

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
#include <xkbcommon/xkbcommon-x11.h>
#include <xcb/xkb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>

#include <cairo.h>
#include <cairo-xcb.h>

/** Indicated we understand the startup notification api is not yet stable.*/
#define SN_API_NOT_YET_FROZEN
#include <libsn/sn.h>
#include "rofi.h"

#include "timings.h"
#include "settings.h"

#include "mode.h"
#include "display.h"
#include "xcb-internal.h"
#include "helper.h"
#include "helper-theme.h"
#include "xrmoptions.h"
#include "dialogs/dialogs.h"

#include "view.h"
#include "view-internal.h"

#include "theme.h"

#include "xcb.h"

/**
 * @param state The handle to the view
 * @param qr    Indicate if queue_redraw should be called on changes.
 *
 * Update the state of the view. This involves filter state.
 */
void rofi_view_update ( RofiViewState *state, gboolean qr );

static int rofi_view_calculate_height ( RofiViewState *state );

/** Thread pool used for filtering */
GThreadPool *tpool = NULL;

/** Global pointer to the currently active RofiViewState */
RofiViewState *current_active_menu = NULL;

/**
 * Structure holding cached state.
 */
struct
{
    /** main x11 windows */
    xcb_window_t       main_window;
    /** surface containing the fake background. */
    cairo_surface_t    *fake_bg;
    /** Draw context  for main window */
    xcb_gcontext_t     gc;
    /** Main X11 side pixmap to draw on. */
    xcb_pixmap_t       edit_pixmap;
    /** Cairo Surface for edit_pixmap */
    cairo_surface_t    *edit_surf;
    /** Drawable context for edit_surf */
    cairo_t            *edit_draw;
    /** Indicate that fake background should be drawn relative to the window */
    int                fake_bgrel;
    /** Main flags */
    MenuFlags          flags;
    /** List of stacked views */
    GQueue             views;
    /** Current work area */
    workarea           mon;
    /** timeout for reloading */
    guint              idle_timeout;
    /** debug counter for redraws */
    unsigned long long count;
    /** redraw idle time. */
    guint              repaint_source;
    /** Window fullscreen */
    gboolean           fullscreen;
} CacheState = {
    .main_window    = XCB_WINDOW_NONE,
    .fake_bg        = NULL,
    .edit_surf      = NULL,
    .edit_draw      = NULL,
    .fake_bgrel     = FALSE,
    .flags          = MENU_NORMAL,
    .views          = G_QUEUE_INIT,
    .idle_timeout   = 0,
    .count          = 0L,
    .repaint_source = 0,
    .fullscreen     = FALSE,
};

void rofi_view_get_current_monitor ( int *width, int *height )
{
    if ( width ) {
        *width = CacheState.mon.w;
    }
    if ( height ) {
        *height = CacheState.mon.h;
    }
}
static char * get_matching_state ( void )
{
    if ( config.case_sensitive ) {
        if ( config.sort ) {
            return "±";
        }
        else {
            return "-";
        }
    }
    else{
        if ( config.sort ) {
            return "+";
        }
    }
    return " ";
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

/**
 * Stores a screenshot of Rofi at that point in time.
 */
void rofi_capture_screenshot ( void )
{
    const char *outp = g_getenv ( "ROFI_PNG_OUTPUT" );
    if ( CacheState.edit_surf == NULL ) {
        // Nothing to store.
        g_warning ( "There is no rofi surface to store" );
        return;
    }
    const char *xdg_pict_dir = g_get_user_special_dir ( G_USER_DIRECTORY_PICTURES );
    if ( outp == NULL && xdg_pict_dir == NULL ) {
        g_warning ( "XDG user picture directory or ROFI_PNG_OUTPUT is not set. Cannot store screenshot." );
        return;
    }
    // Get current time.
    GDateTime *now = g_date_time_new_now_local ();
    // Format filename.
    char      *timestmp = g_date_time_format ( now, "rofi-%Y-%m-%d-%H%M" );
    char      *filename = g_strdup_printf ( "%s-%05d.png", timestmp, 0 );
    // Build full path
    char      *fpath = NULL;
    if ( outp == NULL ) {
        int index = 0;
        fpath = g_build_filename ( xdg_pict_dir, filename, NULL );
        while ( g_file_test ( fpath, G_FILE_TEST_EXISTS ) && index < 99 ) {
            g_free ( fpath );
            g_free ( filename );
            // Try the next index.
            index++;
            // Format filename.
            filename = g_strdup_printf ( "%s-%05d.png", timestmp, index );
            // Build full path
            fpath = g_build_filename ( xdg_pict_dir, filename, NULL );
        }
    }
    else {
        fpath = g_strdup ( outp );
    }
    fprintf ( stderr, color_green "Storing screenshot %s\n"color_reset, fpath );
    cairo_status_t status = cairo_surface_write_to_png ( CacheState.edit_surf, fpath );
    if ( status != CAIRO_STATUS_SUCCESS ) {
        g_warning ( "Failed to produce screenshot '%s', got error: '%s'", fpath,
                    cairo_status_to_string ( status ) );
    }
    g_free ( fpath );
    g_free ( filename );
    g_free ( timestmp );
    g_date_time_unref ( now );
}

/**
 * Code used for benchmarking drawing the gui, this will keep updating the UI as fast as possible.
 */
gboolean do_bench = TRUE;

/**
 * Internal structure that hold benchmarking information.
 */
static struct
{
    /** timer used for timestamping. */
    GTimer   *time;
    /** number of draws done. */
    uint64_t draws;
    /** previous timestamp */
    double   last_ts;
    /** minimum draw time. */
    double   min;
} BenchMark = {
    .time    = NULL,
    .draws   = 0,
    .last_ts = 0.0,
    .min     = G_MAXDOUBLE
};

static gboolean bench_update ( void )
{
    if ( !config.benchmark_ui ) {
        return FALSE;
    }
    BenchMark.draws++;
    if ( BenchMark.time == NULL ) {
        BenchMark.time = g_timer_new ();
    }

    if ( ( BenchMark.draws & 1023 ) == 0 ) {
        double ts  = g_timer_elapsed ( BenchMark.time, NULL );
        double fps = 1024 / ( ts - BenchMark.last_ts );

        if ( fps < BenchMark.min ) {
            BenchMark.min = fps;
        }
        printf ( "current: %.2f fps, avg: %.2f fps, min: %.2f fps, %lu draws\r\n", fps, BenchMark.draws / ts, BenchMark.min, BenchMark.draws );

        BenchMark.last_ts = ts;
    }
    return TRUE;
}

static gboolean rofi_view_repaint ( G_GNUC_UNUSED void * data  )
{
    if ( current_active_menu  ) {
        // Repaint the view (if needed).
        // After a resize the edit_pixmap surface might not contain anything anymore.
        // If we already re-painted, this does nothing.
        rofi_view_update ( current_active_menu, FALSE );
        g_debug ( "expose event" );
        TICK_N ( "Expose" );
        xcb_copy_area ( xcb->connection, CacheState.edit_pixmap, CacheState.main_window, CacheState.gc,
                        0, 0, 0, 0, current_active_menu->width, current_active_menu->height );
        xcb_flush ( xcb->connection );
        TICK_N ( "flush" );
        CacheState.repaint_source = 0;
    }
    return ( bench_update () == TRUE ) ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

static void rofi_view_update_prompt ( RofiViewState *state )
{
    if ( state->prompt ) {
        const char *str = mode_get_display_name ( state->sw );
        textbox_text ( state->prompt, str );
    }
}

/**
 * Calculates the window position
 */
/** Convert the old location to the new location type.
 * 123
 * 804
 * 765
 *
 * nw n ne
 * w  c e
 * sw s se
 */
static const int loc_transtable[9] = {
    WL_CENTER,
    WL_NORTH | WL_WEST,
    WL_NORTH,
    WL_NORTH | WL_EAST,
    WL_EAST,
    WL_SOUTH | WL_EAST,
    WL_SOUTH,
    WL_SOUTH | WL_WEST,
    WL_WEST
};
static void rofi_view_calculate_window_position ( RofiViewState *state )
{
    int location = rofi_theme_get_position ( WIDGET ( state->main_window ), "location", loc_transtable[config.location] );
    int anchor   = location;
    if ( !listview_get_fixed_num_lines ( state->list_view ) ) {
        anchor = location;
        if ( location == WL_CENTER ) {
            anchor = WL_NORTH;
        }
        else if ( location == WL_EAST ) {
            anchor = WL_NORTH_EAST;
        }
        else if ( location == WL_WEST ) {
            anchor = WL_NORTH_WEST;
        }
    }
    anchor = rofi_theme_get_position ( WIDGET ( state->main_window ), "anchor", anchor );

    if ( CacheState.fullscreen ) {
        state->x = CacheState.mon.x;
        state->y = CacheState.mon.y;
        return;
    }
    state->y = CacheState.mon.y + ( CacheState.mon.h ) / 2;
    state->x = CacheState.mon.x + ( CacheState.mon.w ) / 2;
    // Determine window location
    switch ( location )
    {
    case WL_NORTH_WEST:
        state->x = CacheState.mon.x;
    /* FALLTHRU */
    case WL_NORTH:
        state->y = CacheState.mon.y;
        break;
    case WL_NORTH_EAST:
        state->y = CacheState.mon.y;
    /* FALLTHRU */
    case WL_EAST:
        state->x = CacheState.mon.x + CacheState.mon.w;
        break;
    case WL_SOUTH_EAST:
        state->x = CacheState.mon.x + CacheState.mon.w;
    /* FALLTHRU */
    case WL_SOUTH:
        state->y = CacheState.mon.y + CacheState.mon.h;
        break;
    case WL_SOUTH_WEST:
        state->y = CacheState.mon.y + CacheState.mon.h;
    /* FALLTHRU */
    case WL_WEST:
        state->x = CacheState.mon.x;
        break;
    case WL_CENTER:
        ;
    /* FALLTHRU */
    default:
        break;
    }
    switch ( anchor )
    {
    case WL_SOUTH_WEST:
        state->y -= state->height;
        break;
    case WL_SOUTH:
        state->x -= state->width / 2;
        state->y -= state->height;
        break;
    case WL_SOUTH_EAST:
        state->x -= state->width;
        state->y -= state->height;
        break;
    case WL_NORTH_EAST:
        state->x -= state->width;
        break;
    case WL_NORTH_WEST:
        break;
    case WL_NORTH:
        state->x -= state->width / 2;
        break;
    case WL_EAST:
        state->x -= state->width;
        state->y -= state->height / 2;
        break;
    case WL_WEST:
        state->y -= state->height / 2;
        break;
    case WL_CENTER:
        state->y -= state->height / 2;
        state->x -= state->width / 2;
        break;
    default:
        break;
    }
    // Apply offset.
    RofiDistance x = rofi_theme_get_distance ( WIDGET ( state->main_window ), "x-offset", config.x_offset );
    RofiDistance y = rofi_theme_get_distance ( WIDGET ( state->main_window ), "y-offset", config.y_offset );
    state->x += distance_get_pixel ( x, ROFI_ORIENTATION_HORIZONTAL );
    state->y += distance_get_pixel ( y, ROFI_ORIENTATION_VERTICAL );
}

static void rofi_view_window_update_size ( RofiViewState * state )
{
    if ( state == NULL ) {
        return;
    }
    uint16_t mask   = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
    uint32_t vals[] = { state->x, state->y, state->width, state->height };

    // Display it.
    xcb_configure_window ( xcb->connection, CacheState.main_window, mask, vals );
    cairo_destroy ( CacheState.edit_draw );
    cairo_surface_destroy ( CacheState.edit_surf );

    xcb_free_pixmap ( xcb->connection, CacheState.edit_pixmap );
    CacheState.edit_pixmap = xcb_generate_id ( xcb->connection );
    xcb_create_pixmap ( xcb->connection, depth->depth,
                        CacheState.edit_pixmap, CacheState.main_window, state->width, state->height );

    CacheState.edit_surf = cairo_xcb_surface_create ( xcb->connection, CacheState.edit_pixmap, visual, state->width, state->height );
    CacheState.edit_draw = cairo_create ( CacheState.edit_surf );

    g_debug ( "Re-size window based internal request: %dx%d.", state->width, state->height );
    // Should wrap main window in a widget.
    widget_resize ( WIDGET ( state->main_window ), state->width, state->height );
}

static void rofi_view_reload_message_bar ( RofiViewState *state )
{
    if ( state->mesg_box == NULL ) {
        return;
    }
    char *msg = mode_get_message ( state->sw );
    if ( msg ) {
        textbox_text ( state->mesg_tb, msg );
        widget_enable ( WIDGET ( state->mesg_box ) );
        g_free ( msg );
    }
    else {
        widget_disable ( WIDGET ( state->mesg_box ) );
    }
}

static gboolean rofi_view_reload_idle ( G_GNUC_UNUSED gpointer data )
{
    if ( current_active_menu ) {
        current_active_menu->reload   = TRUE;
        current_active_menu->refilter = TRUE;
        rofi_view_queue_redraw ();
    }
    CacheState.idle_timeout = 0;
    return G_SOURCE_REMOVE;
}

void rofi_view_reload ( void  )
{
    // @TODO add check if current view is equal to the callee
    if ( CacheState.idle_timeout == 0 ) {
        CacheState.idle_timeout = g_timeout_add ( 1000 / 10, rofi_view_reload_idle, NULL );
    }
}
void rofi_view_queue_redraw ( void  )
{
    if ( current_active_menu && CacheState.repaint_source == 0 ) {
        CacheState.count++;
        g_debug ( "redraw %llu", CacheState.count );
        CacheState.repaint_source = g_idle_add_full (  G_PRIORITY_HIGH_IDLE, rofi_view_repaint, NULL, NULL );
    }
}

void rofi_view_restart ( RofiViewState *state )
{
    state->quit = FALSE;
    state->retv = MENU_CANCEL;
}

RofiViewState * rofi_view_get_active ( void )
{
    return current_active_menu;
}

void rofi_view_remove_active ( RofiViewState *state )
{
    if ( state == current_active_menu ) {
        rofi_view_set_active ( NULL );
    }
    else if ( state ) {
        g_queue_remove ( &( CacheState.views ), state );
    }
}
void rofi_view_set_active ( RofiViewState *state )
{
    if ( current_active_menu != NULL && state != NULL ) {
        g_queue_push_head ( &( CacheState.views ), current_active_menu );
        // TODO check.
        current_active_menu = state;
        g_debug ( "stack view." );
        rofi_view_window_update_size ( current_active_menu );
        rofi_view_queue_redraw ();
        return;
    }
    else if ( state == NULL && !g_queue_is_empty ( &( CacheState.views ) ) ) {
        g_debug ( "pop view." );
        current_active_menu = g_queue_pop_head ( &( CacheState.views ) );
        rofi_view_window_update_size ( current_active_menu );
        rofi_view_queue_redraw ();
        return;
    }
    g_assert ( ( current_active_menu == NULL && state != NULL ) || ( current_active_menu != NULL && state == NULL ) );
    current_active_menu = state;
    rofi_view_queue_redraw ();
}

void rofi_view_set_selected_line ( RofiViewState *state, unsigned int selected_line )
{
    state->selected_line = selected_line;
    // Find the line.
    unsigned int selected = 0;
    for ( unsigned int i = 0; ( ( state->selected_line ) ) < UINT32_MAX && !selected && i < state->filtered_lines; i++ ) {
        if ( state->line_map[i] == ( state->selected_line ) ) {
            selected = i;
            break;
        }
    }
    listview_set_selected ( state->list_view, selected );
    xcb_clear_area ( xcb->connection, CacheState.main_window, 1, 0, 0, 1, 1 );
    xcb_flush ( xcb->connection );
}

void rofi_view_free ( RofiViewState *state )
{
    if ( state->tokens ) {
        helper_tokenize_free ( state->tokens );
        state->tokens = NULL;
    }
    // Do this here?
    // Wait for final release?
    widget_free ( WIDGET ( state->main_window ) );

    g_free ( state->line_map );
    g_free ( state->distance );
    // Free the switcher boxes.
    // When state is free'ed we should no longer need these.
    g_free ( state->modi );
    state->num_modi = 0;
    g_free ( state );
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
    unsigned int selected = listview_get_selected ( state->list_view );
    if ( ( selected + 1 ) < state->num_lines ) {
        ( next_pos ) = state->line_map[selected + 1];
    }
    return next_pos;
}

unsigned int rofi_view_get_completed ( const RofiViewState *state )
{
    return state->quit;
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
static RofiViewState * __rofi_view_state_create ( void )
{
    return g_malloc0 ( sizeof ( RofiViewState ) );
}

/**
 * Thread state for workers started for the view.
 */
typedef struct _thread_state_view
{
    /** Generic thread state. */
    thread_state  st;

    /** Condition. */
    GCond         *cond;
    /** Lock for condition. */
    GMutex        *mutex;
    /** Count that is protected by lock. */
    unsigned int  *acount;

    /** Current state. */
    RofiViewState *state;
    /** Start row for this worker. */
    unsigned int  start;
    /** Stop row for this worker. */
    unsigned int  stop;
    /** Rows processed. */
    unsigned int  count;

    /** Pattern input to filter. */
    const char    *pattern;
    /** Length of pattern. */
    glong         plen;
} thread_state_view;
/**
 * @param data A thread_state object.
 * @param user_data User data to pass to thread_state callback
 *
 * Small wrapper function that is internally used to pass a job to a worker.
 */
static void rofi_view_call_thread ( gpointer data, gpointer user_data )
{
    thread_state *t = (thread_state *) data;
    t->callback ( t, user_data );
}

static void filter_elements ( thread_state *ts, G_GNUC_UNUSED gpointer user_data )
{
    thread_state_view *t = (thread_state_view *) ts;
    for ( unsigned int i = t->start; i < t->stop; i++ ) {
        int match = mode_token_match ( t->state->sw, t->state->tokens, i );
        // If each token was matched, add it to list.
        if ( match ) {
            t->state->line_map[t->start + t->count] = i;
            if ( config.sort ) {
                // This is inefficient, need to fix it.
                char  * str = mode_get_completion ( t->state->sw, i );
                glong slen  = g_utf8_strlen ( str, -1 );
                switch ( config.sorting_method_enum )
                {
                case SORT_FZF:
                    t->state->distance[i] = rofi_scorer_fuzzy_evaluate ( t->pattern, t->plen, str, slen );
                    break;
                case SORT_NORMAL:
                default:
                    t->state->distance[i] = levenshtein ( t->pattern, t->plen, str, slen );
                    break;
                }
                g_free ( str );
            }
            t->count++;
        }
    }
    if ( t->acount != NULL  ) {
        g_mutex_lock ( t->mutex );
        ( *( t->acount ) )--;
        g_cond_signal ( t->cond );
        g_mutex_unlock ( t->mutex );
    }
}

static void rofi_view_setup_fake_transparency ( widget *win, const char* const fake_background )
{
    if ( CacheState.fake_bg == NULL ) {
        cairo_surface_t *s = NULL;
        /**
         * Select Background to use for fake transparency.
         * Current options: 'real', 'screenshot','background'
         */
        TICK_N ( "Fake start" );
        if ( g_strcmp0 ( fake_background, "real" ) == 0 ) {
            return;
        }
        else if ( g_strcmp0 ( fake_background, "screenshot" ) == 0 ) {
            s = x11_helper_get_screenshot_surface ();
        }
        else if ( g_strcmp0 ( fake_background, "background" ) == 0 ) {
            s = x11_helper_get_bg_surface ();
        }
        else {
            char *fpath = rofi_expand_path ( fake_background );
            g_debug ( "Opening %s to use as background.", fpath );
            s                     = cairo_image_surface_create_from_png ( fpath );
            CacheState.fake_bgrel = TRUE;
            g_free ( fpath );
        }
        TICK_N ( "Get surface." );
        if ( s != NULL ) {
            if ( cairo_surface_status ( s ) != CAIRO_STATUS_SUCCESS ) {
                g_debug ( "Failed to open surface fake background: %s",
                          cairo_status_to_string ( cairo_surface_status ( s ) ) );
                cairo_surface_destroy ( s );
                s = NULL;
            }
            else {
                CacheState.fake_bg = cairo_image_surface_create ( CAIRO_FORMAT_ARGB32, CacheState.mon.w, CacheState.mon.h );
                
                int blur = rofi_theme_get_integer ( WIDGET ( win ), "blur", 0 );
                cairo_t *dr = cairo_create ( CacheState.fake_bg );
                if ( CacheState.fake_bgrel ) {
                    cairo_set_source_surface ( dr, s, 0, 0 );
                }
                else {
                    cairo_set_source_surface ( dr, s, -CacheState.mon.x, -CacheState.mon.y );
                }
                cairo_paint ( dr );
                cairo_destroy ( dr );
                cairo_surface_destroy ( s );
                if ( blur > 0 ){
                    cairo_image_surface_blur( CacheState.fake_bg, (double)blur, 0 );
                    TICK_N("BLUR");
                }
            }
        }
        TICK_N ( "Fake transparency" );
    }
}
void __create_window ( MenuFlags menu_flags )
{
    uint32_t selmask         = XCB_CW_BACK_PIXMAP | XCB_CW_BORDER_PIXEL | XCB_CW_BIT_GRAVITY | XCB_CW_BACKING_STORE | XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
    uint32_t xcb_event_masks = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
                               XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_KEYMAP_STATE |
                               XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_BUTTON_1_MOTION;
    if ( config.hover_select == TRUE ) {
        xcb_event_masks |= XCB_EVENT_MASK_POINTER_MOTION;
    }
    uint32_t          selval[] = {
        XCB_BACK_PIXMAP_NONE,         0,
        XCB_GRAVITY_STATIC,
        XCB_BACKING_STORE_NOT_USEFUL,
        xcb_event_masks,
        map
    };

    xcb_window_t      box_window = xcb_generate_id ( xcb->connection );
    xcb_void_cookie_t cc         = xcb_create_window_checked ( xcb->connection, depth->depth, box_window, xcb_stuff_get_root_window ( ),
                                                               0, 0, 200, 100, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                                                               visual->visual_id, selmask, selval );
    xcb_generic_error_t *error;
    error = xcb_request_check ( xcb->connection, cc );
    if ( error ) {
        g_error ( "xcb_create_window() failed error=0x%x\n", error->error_code );
        exit ( EXIT_FAILURE );
    }
    TICK_N ( "xcb create window" );
    CacheState.gc = xcb_generate_id ( xcb->connection );
    xcb_create_gc ( xcb->connection, CacheState.gc, box_window, 0, 0 );

    TICK_N ( "xcb create gc" );
    // Create a drawable.
    CacheState.edit_pixmap = xcb_generate_id ( xcb->connection );
    xcb_create_pixmap ( xcb->connection, depth->depth,
                        CacheState.edit_pixmap, CacheState.main_window, 200, 100 );

    CacheState.edit_surf = cairo_xcb_surface_create ( xcb->connection, CacheState.edit_pixmap, visual, 200, 100 );
    CacheState.edit_draw = cairo_create ( CacheState.edit_surf );

    TICK_N ( "create cairo surface" );
    // Set up pango context.
    cairo_font_options_t *fo = cairo_font_options_create ();
    // Take font description from xlib surface
    cairo_surface_get_font_options ( CacheState.edit_surf, fo );
    // TODO should we update the drawable each time?
    PangoContext *p = pango_cairo_create_context ( CacheState.edit_draw );
    // Set the font options from the xlib surface
    pango_cairo_context_set_font_options ( p, fo );
    TICK_N ( "pango cairo font setup" );

    CacheState.main_window = box_window;
    CacheState.flags       = menu_flags;
    monitor_active ( &( CacheState.mon ) );
    // Setup dpi
    if ( config.dpi > 1 ) {
        PangoFontMap *font_map = pango_cairo_font_map_get_default ();
        pango_cairo_font_map_set_resolution ( (PangoCairoFontMap *) font_map, (double) config.dpi );
    }
    else if  ( config.dpi == 0 || config.dpi == 1 ) {
        // Auto-detect mode.
        double dpi = 96;
        if ( CacheState.mon.mh > 0 && config.dpi == 1 ) {
            dpi = ( CacheState.mon.h * 25.4 ) / (double) ( CacheState.mon.mh );
        }
        else {
            dpi = ( xcb->screen->height_in_pixels * 25.4 ) / (double) ( xcb->screen->height_in_millimeters );
        }

        g_debug ( "Auto-detected DPI: %.2lf", dpi );
        PangoFontMap *font_map = pango_cairo_font_map_get_default ();
        pango_cairo_font_map_set_resolution ( (PangoCairoFontMap *) font_map, dpi );
        config.dpi = dpi;
    }
    else {
        // default pango is 96.
        PangoFontMap *font_map = pango_cairo_font_map_get_default ();
        config.dpi = pango_cairo_font_map_get_resolution ( (PangoCairoFontMap *) font_map );
    }
    // Setup font.
    // Dummy widget.
    box        *win  = box_create ( NULL, "window", ROFI_ORIENTATION_HORIZONTAL );
    const char *font = rofi_theme_get_string ( WIDGET ( win ), "font", config.menu_font );
    if ( font ) {
        PangoFontDescription *pfd = pango_font_description_from_string ( font );
        if ( helper_validate_font ( pfd, font ) ) {
            pango_context_set_font_description ( p, pfd );
        }
        pango_font_description_free ( pfd );
    }
    PangoLanguage *l = pango_language_get_default ();
    pango_context_set_language ( p, l );
    TICK_N ( "configure font" );

    // Tell textbox to use this context.
    textbox_set_pango_context ( font, p );
    // cleanup
    g_object_unref ( p );
    cairo_font_options_destroy ( fo );

    TICK_N ( "textbox setup" );
    // // make it an unmanaged window
    if ( ( ( menu_flags & MENU_NORMAL_WINDOW ) == 0 ) ) {
        window_set_atom_prop ( box_window, xcb->ewmh._NET_WM_STATE, &( xcb->ewmh._NET_WM_STATE_ABOVE ), 1 );
        uint32_t values[] = { 1 };
        xcb_change_window_attributes ( xcb->connection, box_window, XCB_CW_OVERRIDE_REDIRECT, values );
    }
    else{
        window_set_atom_prop ( box_window, xcb->ewmh._NET_WM_WINDOW_TYPE, &( xcb->ewmh._NET_WM_WINDOW_TYPE_NORMAL ), 1 );
        x11_disable_decoration ( box_window );
    }

    TICK_N ( "setup window attributes" );
    CacheState.fullscreen = rofi_theme_get_boolean ( WIDGET ( win ), "fullscreen", FALSE );
    if ( CacheState.fullscreen ) {
        xcb_atom_t atoms[] = {
            xcb->ewmh._NET_WM_STATE_FULLSCREEN,
            xcb->ewmh._NET_WM_STATE_ABOVE
        };
        window_set_atom_prop (  box_window, xcb->ewmh._NET_WM_STATE, atoms, sizeof ( atoms ) / sizeof ( xcb_atom_t ) );
    }

    xcb_atom_t protocols[] = {
        netatoms[WM_TAKE_FOCUS]
    };
    xcb_icccm_set_wm_protocols ( xcb->connection, box_window, xcb->ewmh.WM_PROTOCOLS, G_N_ELEMENTS ( protocols ), protocols );

    TICK_N ( "setup window fullscreen" );
    // Set the WM_NAME
    rofi_view_set_window_title ( "rofi" );
    const char wm_class_name[] = "rofi\0Rofi";
    xcb_icccm_set_wm_class ( xcb->connection, box_window, sizeof ( wm_class_name ), wm_class_name );

    TICK_N ( "setup window name and class" );
    const char *transparency = rofi_theme_get_string ( WIDGET ( win ), "transparency", NULL );
    if ( transparency ) {
        rofi_view_setup_fake_transparency ( WIDGET (win), transparency  );
    }
    else if ( config.fake_transparency && config.fake_background ) {
        rofi_view_setup_fake_transparency ( WIDGET ( win ), config.fake_background );
    }
    if ( xcb->sncontext != NULL ) {
        sn_launchee_context_setup_window ( xcb->sncontext, CacheState.main_window );
    }
    TICK_N ( "setup startup notification" );
    widget_free ( WIDGET ( win ) );
    TICK_N ( "done" );

    // Set the PID.
    pid_t pid = getpid ();
    xcb_ewmh_set_wm_pid ( &( xcb->ewmh ), CacheState.main_window, pid );

    // Get hostname
    const char *hostname = g_get_host_name ();
    char       *ahost    = g_hostname_to_ascii ( hostname );
    if ( ahost != NULL ) {
        xcb_icccm_set_wm_client_machine ( xcb->connection,
                                          CacheState.main_window,
                                          XCB_ATOM_STRING, 8,
                                          strlen ( ahost ), ahost );
        g_free ( ahost );
    }
}

/**
 * @param state Internal state of the menu.
 *
 * Calculate the width of the window and the width of an element.
 */
static void rofi_view_calculate_window_width ( RofiViewState *state )
{
    if ( CacheState.fullscreen ) {
        state->width = CacheState.mon.w;
        return;
    }
    if ( config.menu_width < 0 ) {
        double fw = textbox_get_estimated_char_width ( );
        state->width  = -( fw * config.menu_width );
        state->width += widget_padding_get_padding_width ( WIDGET ( state->main_window ) );
    }
    else{
        // Calculate as float to stop silly, big rounding down errors.
        state->width = config.menu_width < 101 ? ( CacheState.mon.w / 100.0f ) * ( float ) config.menu_width : config.menu_width;
    }
    // Use theme configured width, if set.
    RofiDistance width = rofi_theme_get_distance ( WIDGET ( state->main_window ), "width", state->width );
    state->width = distance_get_pixel ( width, ROFI_ORIENTATION_HORIZONTAL );
}

/**
 * Nav helper functions, to avoid duplicate code.
 */

/**
 * @param state The current RofiViewState
 *
 * Tab handling.
 */
static void rofi_view_nav_row_tab ( RofiViewState *state )
{
    if ( state->filtered_lines == 1 ) {
        state->retv              = MENU_OK;
        ( state->selected_line ) = state->line_map[listview_get_selected ( state->list_view )];
        state->quit              = 1;
        return;
    }

    // Double tab!
    if ( state->filtered_lines == 0 && ROW_TAB == state->prev_action ) {
        state->retv              = MENU_NEXT;
        ( state->selected_line ) = 0;
        state->quit              = TRUE;
    }
    else {
        listview_nav_down ( state->list_view );
    }
    state->prev_action = ROW_TAB;
}
/**
 * @param state The current RofiViewState
 *
 * complete current row.
 */
inline static void rofi_view_nav_row_select ( RofiViewState *state )
{
    if ( state->list_view == NULL ) {
        return;
    }
    unsigned int selected = listview_get_selected ( state->list_view );
    // If a valid item is selected, return that..
    if ( selected < state->filtered_lines ) {
        char *str = mode_get_completion ( state->sw, state->line_map[selected] );
        textbox_text ( state->text, str );
        g_free ( str );
        textbox_keybinding ( state->text, MOVE_END );
        state->refilter = TRUE;
    }
}

/**
 * @param state The current RofiViewState
 *
 * Move the selection to first row.
 */
inline static void rofi_view_nav_first ( RofiViewState * state )
{
//    state->selected = 0;
    listview_set_selected ( state->list_view, 0 );
}

/**
 * @param state The current RofiViewState
 *
 * Move the selection to last row.
 */
inline static void rofi_view_nav_last ( RofiViewState * state )
{
    // If no lines, do nothing.
    if ( state->filtered_lines == 0 ) {
        return;
    }
    //state->selected = state->filtered_lines - 1;
    listview_set_selected ( state->list_view, -1 );
}

static void update_callback ( textbox *t, icon *ico, unsigned int index, void *udata, TextBoxFontType *type, gboolean full )
{
    RofiViewState *state = (RofiViewState *) udata;
    if ( full ) {
        GList *add_list = NULL;
        int   fstate    = 0;
        char  *text     = mode_get_display_value ( state->sw, state->line_map[index], &fstate, &add_list, TRUE );
        ( *type ) |= fstate;
        // TODO needed for markup.
        textbox_font ( t, *type );
        // Move into list view.
        textbox_text ( t, text );
        PangoAttrList *list = textbox_get_pango_attributes ( t );
        if ( list != NULL ) {
            pango_attr_list_ref ( list );
        }
        else{
            list = pango_attr_list_new ();
        }
        if ( ico ) {
            int             icon_height = widget_get_desired_height ( WIDGET ( ico ) );
            cairo_surface_t *icon       = mode_get_icon ( state->sw, state->line_map[index], icon_height );
            icon_set_surface ( ico, icon );
        }

        if ( state->tokens ) {
            RofiHighlightColorStyle th = { ROFI_HL_BOLD | ROFI_HL_UNDERLINE, { 0.0, 0.0, 0.0, 0.0 } };
            th = rofi_theme_get_highlight ( WIDGET ( t ), "highlight", th );
            helper_token_match_get_pango_attr ( th, state->tokens, textbox_get_visible_text ( t ), list );
        }
        for ( GList *iter = g_list_first ( add_list ); iter != NULL; iter = g_list_next ( iter ) ) {
            pango_attr_list_insert ( list, (PangoAttribute *) ( iter->data ) );
        }
        textbox_set_pango_attributes ( t, list );
        pango_attr_list_unref ( list );
        g_list_free ( add_list );
        g_free ( text );
    }
    else {
        int fstate = 0;
        mode_get_display_value ( state->sw, state->line_map[index], &fstate, NULL, FALSE );
        ( *type ) |= fstate;
        // TODO needed for markup.
        textbox_font ( t, *type );
    }
}

void rofi_view_update ( RofiViewState *state, gboolean qr )
{
    if ( !widget_need_redraw ( WIDGET ( state->main_window ) ) ) {
        return;
    }
    g_debug ( "Redraw view" );
    TICK ();
    cairo_t *d = CacheState.edit_draw;
    cairo_set_operator ( d, CAIRO_OPERATOR_SOURCE );
    if ( CacheState.fake_bg != NULL ) {
        if ( CacheState.fake_bgrel ) {
            cairo_set_source_surface ( d, CacheState.fake_bg, 0.0, 0.0 );
        }
        else {
            cairo_set_source_surface ( d, CacheState.fake_bg,
                                       -(double) ( state->x - CacheState.mon.x ),
                                       -(double) ( state->y - CacheState.mon.y ) );
        }
        cairo_paint ( d );
        cairo_set_operator ( d, CAIRO_OPERATOR_OVER );
    }
    else {
        // Paint the background transparent.
        cairo_set_source_rgba ( d, 0, 0, 0, 0.0 );
        cairo_paint ( d );
    }
    TICK_N ( "Background" );

    // Always paint as overlay over the background.
    cairo_set_operator ( d, CAIRO_OPERATOR_OVER );
    widget_draw ( WIDGET ( state->main_window ), d );

    TICK_N ( "widgets" );
    cairo_surface_flush ( CacheState.edit_surf );
    if ( qr ) {
        rofi_view_queue_redraw ();
    }
}

static void _rofi_view_reload_row ( RofiViewState *state )
{
    g_free ( state->line_map );
    g_free ( state->distance );
    state->num_lines = mode_get_num_entries ( state->sw );
    state->line_map  = g_malloc0_n ( state->num_lines, sizeof ( unsigned int ) );
    state->distance  = g_malloc0_n ( state->num_lines, sizeof ( int ) );
    listview_set_max_lines ( state->list_view, state->num_lines );
    rofi_view_reload_message_bar ( state );
}

static void rofi_view_refilter ( RofiViewState *state )
{
    TICK_N ( "Filter start" );
    if ( state->reload ) {
        _rofi_view_reload_row ( state );
        state->reload = FALSE;
    }
    TICK_N ( "Filter reload rows" );
    if ( state->tokens ) {
        helper_tokenize_free ( state->tokens );
        state->tokens = NULL;
    }
    TICK_N ( "Filter tokenize" );
    if ( state->text && strlen ( state->text->text ) > 0 ) {
        unsigned int j        = 0;
        gchar        *pattern = mode_preprocess_input ( state->sw, state->text->text );
        glong        plen     = pattern ? g_utf8_strlen ( pattern, -1 ) : 0;
        state->tokens = helper_tokenize ( pattern, config.case_sensitive );
        /**
         * On long lists it can be beneficial to parallelize.
         * If number of threads is 1, no thread is spawn.
         * If number of threads > 1 and there are enough (> 1000) items, spawn jobs for the thread pool.
         * For large lists with 8 threads I see a factor three speedup of the whole function.
         */
        unsigned int      nt = MAX ( 1, state->num_lines / 500 );
        thread_state_view states[nt];
        GCond             cond;
        GMutex            mutex;
        g_mutex_init ( &mutex );
        g_cond_init ( &cond );
        unsigned int count = nt;
        unsigned int steps = ( state->num_lines + nt ) / nt;
        for ( unsigned int i = 0; i < nt; i++ ) {
            states[i].state       = state;
            states[i].start       = i * steps;
            states[i].stop        = MIN ( state->num_lines, ( i + 1 ) * steps );
            states[i].count       = 0;
            states[i].cond        = &cond;
            states[i].mutex       = &mutex;
            states[i].acount      = &count;
            states[i].plen        = plen;
            states[i].pattern     = pattern;
            states[i].st.callback = filter_elements;
            if ( i > 0 ) {
                g_thread_pool_push ( tpool, &states[i], NULL );
            }
        }
        // Run one in this thread.
        rofi_view_call_thread ( &states[0], NULL );
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
                memmove ( &( state->line_map[j] ), &( state->line_map[states[i].start] ), sizeof ( unsigned int ) * ( states[i].count ) );
            }
            j += states[i].count;
        }
        if ( config.sort ) {
            g_qsort_with_data ( state->line_map, j, sizeof ( int ), lev_sort, state->distance );
        }

        // Cleanup + bookkeeping.
        state->filtered_lines = j;
        g_free ( pattern );
    }
    else{
        for ( unsigned int i = 0; i < state->num_lines; i++ ) {
            state->line_map[i] = i;
        }
        state->filtered_lines = state->num_lines;
    }
    TICK_N ( "Filter matching done" );
    listview_set_num_elements ( state->list_view, state->filtered_lines );

    if ( state->tb_filtered_rows ) {
        char *r = g_strdup_printf ( "%u", state->filtered_lines );
        textbox_text ( state->tb_filtered_rows, r );
        g_free ( r );
    }
    if ( state->tb_total_rows ) {
        char *r = g_strdup_printf ( "%u", state->num_lines );
        textbox_text ( state->tb_total_rows, r );
        g_free ( r );
    }
    TICK_N ( "Update filter lines" );

    if ( config.auto_select == TRUE && state->filtered_lines == 1 && state->num_lines > 1 ) {
        ( state->selected_line ) = state->line_map[listview_get_selected ( state->list_view  )];
        state->retv              = MENU_OK;
        state->quit              = TRUE;
    }

    // Size the window.
    int height = rofi_view_calculate_height ( state );
    if ( height != state->height ) {
        state->height = height;
        rofi_view_calculate_window_position ( state );
        rofi_view_window_update_size ( state );
        g_debug ( "Resize based on re-filter" );
    }
    TICK_N ( "Filter resize window based on window " );
    state->refilter = FALSE;
    TICK_N ( "Filter done" );
}
/**
 * @param state The Menu Handle
 *
 * Check if a finalize function is set, and if sets executes it.
 */
void process_result ( RofiViewState *state );
void rofi_view_finalize ( RofiViewState *state )
{
    if ( state && state->finalize != NULL ) {
        state->finalize ( state );
    }
}

static void rofi_view_trigger_global_action ( KeyBindingAction action )
{
    RofiViewState *state = rofi_view_get_active ();
    switch ( action )
    {
    // Handling of paste
    case PASTE_PRIMARY:
        xcb_convert_selection ( xcb->connection, CacheState.main_window, XCB_ATOM_PRIMARY,
                                xcb->ewmh.UTF8_STRING, xcb->ewmh.UTF8_STRING, XCB_CURRENT_TIME );
        xcb_flush ( xcb->connection );
        break;
    case PASTE_SECONDARY:
        xcb_convert_selection ( xcb->connection, CacheState.main_window, netatoms[CLIPBOARD],
                                xcb->ewmh.UTF8_STRING, xcb->ewmh.UTF8_STRING, XCB_CURRENT_TIME );
        xcb_flush ( xcb->connection );
        break;
    case SCREENSHOT:
        rofi_capture_screenshot ( );
        break;
    case CHANGE_ELLIPSIZE:
        if ( state->list_view ) {
            listview_toggle_ellipsizing ( state->list_view );
        }
        break;
    case TOGGLE_SORT:
        if ( state->case_indicator != NULL ) {
            config.sort     = !config.sort;
            state->refilter = TRUE;
            textbox_text ( state->case_indicator, get_matching_state () );
        }
        break;
    case MODE_PREVIOUS:
        state->retv              = MENU_PREVIOUS;
        ( state->selected_line ) = 0;
        state->quit              = TRUE;
        break;
    // Menu navigation.
    case MODE_NEXT:
        state->retv              = MENU_NEXT;
        ( state->selected_line ) = 0;
        state->quit              = TRUE;
        break;
    // Toggle case sensitivity.
    case TOGGLE_CASE_SENSITIVITY:
        if ( state->case_indicator != NULL ) {
            config.case_sensitive    = !config.case_sensitive;
            ( state->selected_line ) = 0;
            state->refilter          = TRUE;
            textbox_text ( state->case_indicator, get_matching_state () );
        }
        break;
    // Special delete entry command.
    case DELETE_ENTRY:
    {
        unsigned int selected = listview_get_selected ( state->list_view );
        if ( selected < state->filtered_lines ) {
            ( state->selected_line ) = state->line_map[selected];
            state->retv              = MENU_ENTRY_DELETE;
            state->quit              = TRUE;
        }
        break;
    }
    case SELECT_ELEMENT_1:
    case SELECT_ELEMENT_2:
    case SELECT_ELEMENT_3:
    case SELECT_ELEMENT_4:
    case SELECT_ELEMENT_5:
    case SELECT_ELEMENT_6:
    case SELECT_ELEMENT_7:
    case SELECT_ELEMENT_8:
    case SELECT_ELEMENT_9:
    case SELECT_ELEMENT_10:
    {
        unsigned int index = action - SELECT_ELEMENT_1;
        if ( index < state->filtered_lines ) {
            state->selected_line = state->line_map[index];
            state->retv          = MENU_OK;
            state->quit          = TRUE;
        }
        break;
    }
    case CUSTOM_1:
    case CUSTOM_2:
    case CUSTOM_3:
    case CUSTOM_4:
    case CUSTOM_5:
    case CUSTOM_6:
    case CUSTOM_7:
    case CUSTOM_8:
    case CUSTOM_9:
    case CUSTOM_10:
    case CUSTOM_11:
    case CUSTOM_12:
    case CUSTOM_13:
    case CUSTOM_14:
    case CUSTOM_15:
    case CUSTOM_16:
    case CUSTOM_17:
    case CUSTOM_18:
    case CUSTOM_19:
    {
        state->selected_line = UINT32_MAX;
        unsigned int selected = listview_get_selected ( state->list_view );
        if ( selected < state->filtered_lines ) {
            ( state->selected_line ) = state->line_map[selected];
        }
        state->retv = MENU_CUSTOM_COMMAND | ( ( action - CUSTOM_1 ) & MENU_LOWER_MASK );
        state->quit = TRUE;
        break;
    }
    // If you add a binding here, make sure to add it to rofi_view_keyboard_navigation too
    case CANCEL:
        state->retv = MENU_CANCEL;
        state->quit = TRUE;
        break;
    case ROW_UP:
        listview_nav_up ( state->list_view );
        break;
    case ROW_TAB:
        rofi_view_nav_row_tab ( state );
        break;
    case ROW_DOWN:
        listview_nav_down ( state->list_view );
        break;
    case ROW_LEFT:
        listview_nav_left ( state->list_view );
        break;
    case ROW_RIGHT:
        listview_nav_right ( state->list_view );
        break;
    case PAGE_PREV:
        listview_nav_page_prev ( state->list_view );
        break;
    case PAGE_NEXT:
        listview_nav_page_next ( state->list_view );
        break;
    case ROW_FIRST:
        rofi_view_nav_first ( state );
        break;
    case ROW_LAST:
        rofi_view_nav_last ( state );
        break;
    case ROW_SELECT:
        rofi_view_nav_row_select ( state );
        break;
    // If you add a binding here, make sure to add it to textbox_keybinding too
    case MOVE_CHAR_BACK:
    {
        if ( textbox_keybinding ( state->text, action ) == 0 ) {
            listview_nav_left ( state->list_view );
        }
        break;
    }
    case MOVE_CHAR_FORWARD:
    {
        if ( textbox_keybinding ( state->text, action ) == 0 ) {
            listview_nav_right ( state->list_view );
        }
        break;
    }
    case CLEAR_LINE:
    case MOVE_FRONT:
    case MOVE_END:
    case REMOVE_TO_EOL:
    case REMOVE_TO_SOL:
    case REMOVE_WORD_BACK:
    case REMOVE_WORD_FORWARD:
    case REMOVE_CHAR_FORWARD:
    case MOVE_WORD_BACK:
    case MOVE_WORD_FORWARD:
    case REMOVE_CHAR_BACK:
    {
        int rc = textbox_keybinding ( state->text, action );
        if ( rc == 1 ) {
            // Entry changed.
            state->refilter = TRUE;
        }
        else if ( rc == 2 ) {
            // Movement.
        }
        break;
    }
    case ACCEPT_ALT:
    {
        unsigned int selected = listview_get_selected ( state->list_view );
        state->selected_line = UINT32_MAX;
        if ( selected < state->filtered_lines ) {
            ( state->selected_line ) = state->line_map[selected];
            state->retv              = MENU_OK;
        }
        else {
            // Nothing entered and nothing selected.
            state->retv = MENU_CUSTOM_INPUT;
        }
        state->retv |= MENU_CUSTOM_ACTION;
        state->quit  = TRUE;
        break;
    }
    case ACCEPT_CUSTOM:
    {
        state->selected_line = UINT32_MAX;
        state->retv          = MENU_CUSTOM_INPUT;
        state->quit          = TRUE;
        break;
    }
    case ACCEPT_ENTRY:
    {
        // If a valid item is selected, return that..
        unsigned int selected = listview_get_selected ( state->list_view );
        state->selected_line = UINT32_MAX;
        if ( selected < state->filtered_lines ) {
            ( state->selected_line ) = state->line_map[selected];
            state->retv              = MENU_OK;
        }
        else {
            // Nothing entered and nothing selected.
            state->retv = MENU_CUSTOM_INPUT;
        }

        state->quit = TRUE;
        break;
    }
    }
}

gboolean rofi_view_trigger_action ( RofiViewState *state, BindingsScope scope, guint action )
{
    switch ( scope )
    {
    case SCOPE_GLOBAL:
        rofi_view_trigger_global_action ( action );
        return TRUE;
    case SCOPE_MOUSE_LISTVIEW:
    case SCOPE_MOUSE_LISTVIEW_ELEMENT:
    case SCOPE_MOUSE_EDITBOX:
    case SCOPE_MOUSE_SCROLLBAR:
    case SCOPE_MOUSE_MODE_SWITCHER:
    {
        gint   x = state->mouse.x, y = state->mouse.y;
        widget *target = widget_find_mouse_target ( WIDGET ( state->main_window ), scope, x, y );
        if ( target == NULL ) {
            return FALSE;
        }
        widget_xy_to_relative ( target, &x, &y );
        switch ( widget_trigger_action ( target, action, x, y ) )
        {
        case WIDGET_TRIGGER_ACTION_RESULT_IGNORED:
            return FALSE;
        case WIDGET_TRIGGER_ACTION_RESULT_GRAB_MOTION_END:
            target = NULL;
        /* FALLTHRU */
        case WIDGET_TRIGGER_ACTION_RESULT_GRAB_MOTION_BEGIN:
            state->mouse.motion_target = target;
        /* FALLTHRU */
        case WIDGET_TRIGGER_ACTION_RESULT_HANDLED:
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

void rofi_view_handle_text ( RofiViewState *state, char *text )
{
    if ( textbox_append_text ( state->text, text, strlen ( text ) ) ) {
        state->refilter = TRUE;
    }
}

void rofi_view_handle_mouse_motion ( RofiViewState *state, gint x, gint y, gboolean find_mouse_target )
{
    state->mouse.x = x;
    state->mouse.y = y;
    if ( find_mouse_target ) {
        widget *target = widget_find_mouse_target ( WIDGET ( state->main_window ), SCOPE_MOUSE_LISTVIEW_ELEMENT, x, y );
        if ( target != NULL ) {
            state->mouse.motion_target = target;
        }
    }
    if ( state->mouse.motion_target != NULL ) {
        widget_xy_to_relative ( state->mouse.motion_target, &x, &y );
        widget_motion_notify ( state->mouse.motion_target, x, y );
        if ( find_mouse_target ) {
            state->mouse.motion_target = NULL;
        }
    }
}

void rofi_view_maybe_update ( RofiViewState *state )
{
    if ( rofi_view_get_completed ( state ) ) {
        // This menu is done.
        rofi_view_finalize ( state );
        // If there a state. (for example error) reload it.
        state = rofi_view_get_active ();

        // cleanup, if no more state to display.
        if ( state == NULL ) {
            // Quit main-loop.
            rofi_quit_main_loop ();
            return;
        }
    }

    // Update if requested.
    if ( state->refilter ) {
        rofi_view_refilter ( state );
    }
    rofi_view_update ( state, TRUE );
}

/**
 * Handle window configure event.
 * Handles resizes.
 */
void rofi_view_temp_configure_notify ( RofiViewState *state, xcb_configure_notify_event_t *xce )
{
    if ( xce->window == CacheState.main_window ) {
        if ( state->x != xce->x || state->y != xce->y ) {
            state->x = xce->x;
            state->y = xce->y;
            widget_queue_redraw ( WIDGET ( state->main_window ) );
        }
        if ( state->width != xce->width || state->height != xce->height ) {
            state->width  = xce->width;
            state->height = xce->height;

            cairo_destroy ( CacheState.edit_draw );
            cairo_surface_destroy ( CacheState.edit_surf );

            xcb_free_pixmap ( xcb->connection, CacheState.edit_pixmap );
            CacheState.edit_pixmap = xcb_generate_id ( xcb->connection );
            xcb_create_pixmap ( xcb->connection, depth->depth, CacheState.edit_pixmap, CacheState.main_window,
                                state->width, state->height );

            CacheState.edit_surf = cairo_xcb_surface_create ( xcb->connection, CacheState.edit_pixmap, visual, state->width, state->height );
            CacheState.edit_draw = cairo_create ( CacheState.edit_surf );
            g_debug ( "Re-size window based external request: %d %d", state->width, state->height );
            widget_resize ( WIDGET ( state->main_window ), state->width, state->height );
        }
    }
}

/**
 * Quit rofi on click (outside of view )
 */
void rofi_view_temp_click_to_exit ( RofiViewState *state, xcb_window_t target )
{
    if ( ( CacheState.flags & MENU_NORMAL_WINDOW ) == 0 ) {
        if ( target != CacheState.main_window ) {
            state->quit = TRUE;
            state->retv = MENU_CANCEL;
        }
    }
}

void rofi_view_frame_callback ( void )
{
    if ( CacheState.repaint_source == 0 ) {
        CacheState.repaint_source = g_idle_add_full (  G_PRIORITY_HIGH_IDLE, rofi_view_repaint, NULL, NULL );
    }
}

static int rofi_view_calculate_height ( RofiViewState *state )
{
    if ( CacheState.fullscreen == TRUE ) {
        return CacheState.mon.h;
    }

    RofiDistance h      = rofi_theme_get_distance ( WIDGET ( state->main_window ), "height", 0 );
    unsigned int height = distance_get_pixel ( h, ROFI_ORIENTATION_VERTICAL );
    // If height is set, return it.
    if ( height > 0 ) {
        return height;
    }
    // Autosize based on widgets.
    widget *main_window = WIDGET ( state->main_window );
    return widget_get_desired_height ( main_window );
}

static WidgetTriggerActionResult textbox_button_trigger_action ( widget *wid, MouseBindingMouseDefaultAction action, G_GNUC_UNUSED gint x, G_GNUC_UNUSED gint y, G_GNUC_UNUSED void *user_data )
{
    RofiViewState *state = ( RofiViewState *) user_data;
    switch ( action )
    {
    case MOUSE_CLICK_DOWN:
    {
        const char * type = rofi_theme_get_string ( wid, "action", "ok" );
        ( state->selected_line ) = state->line_map[listview_get_selected ( state->list_view )];
        if ( strcmp ( type, "ok" ) == 0 ) {
            state->retv = MENU_OK;
        }
        else if ( strcmp ( type, "ok|alternate" ) == 0 ) {
            state->retv = MENU_CUSTOM_ACTION | MENU_OK;
        }
        else if ( strcmp ( type, "custom" ) ) {
            state->retv = MENU_CUSTOM_INPUT;
        }
        else if ( strcmp ( type, "custom|alternate" ) == 0 ) {
            state->retv = MENU_CUSTOM_ACTION | MENU_CUSTOM_INPUT;
        }
        else {
            g_warning ( "Invalid action specified." );
            return WIDGET_TRIGGER_ACTION_RESULT_IGNORED;
        }
        state->quit        = TRUE;
        state->skip_absorb = TRUE;
        return WIDGET_TRIGGER_ACTION_RESULT_HANDLED;
    }
    case MOUSE_CLICK_UP:
    case MOUSE_DCLICK_DOWN:
    case MOUSE_DCLICK_UP:
        break;
    }
    return WIDGET_TRIGGER_ACTION_RESULT_IGNORED;
}
static WidgetTriggerActionResult textbox_sidebar_modi_trigger_action ( widget *wid, MouseBindingMouseDefaultAction action, G_GNUC_UNUSED gint x, G_GNUC_UNUSED gint y, G_GNUC_UNUSED void *user_data )
{
    RofiViewState *state = ( RofiViewState *) user_data;
    unsigned int  i;
    for ( i = 0; i < state->num_modi; i++ ) {
        if ( WIDGET ( state->modi[i] ) == wid ) {
            break;
        }
    }
    if ( i == state->num_modi ) {
        return WIDGET_TRIGGER_ACTION_RESULT_IGNORED;
    }

    switch ( action )
    {
    case MOUSE_CLICK_DOWN:
        state->retv        = MENU_QUICK_SWITCH | ( i & MENU_LOWER_MASK );
        state->quit        = TRUE;
        state->skip_absorb = TRUE;
        return WIDGET_TRIGGER_ACTION_RESULT_HANDLED;
    case MOUSE_CLICK_UP:
    case MOUSE_DCLICK_DOWN:
    case MOUSE_DCLICK_UP:
        break;
    }
    return WIDGET_TRIGGER_ACTION_RESULT_IGNORED;
}

// @TODO don't like this construction.
static void rofi_view_listview_mouse_activated_cb ( listview *lv, gboolean custom, void *udata )
{
    RofiViewState *state = (RofiViewState *) udata;
    state->retv = MENU_OK;
    if ( custom ) {
        state->retv |= MENU_CUSTOM_ACTION;
    }
    ( state->selected_line ) = state->line_map[listview_get_selected ( lv )];
    // Quit
    state->quit        = TRUE;
    state->skip_absorb = TRUE;
}

static void rofi_view_add_widget ( RofiViewState *state, widget *parent_widget, const char *name )
{
    char   *defaults = NULL;
    widget *wid      = NULL;

    /**
     * MAINBOX
     */
    if ( strcmp ( name, "mainbox" ) == 0 ) {
        wid = (widget *) box_create ( parent_widget, name, ROFI_ORIENTATION_VERTICAL );
        box_add ( (box *) parent_widget, WIDGET ( wid ), TRUE );
        if ( config.sidebar_mode ) {
            defaults = "inputbar,message,listview,mode-switcher";
        }
        else {
            defaults = "inputbar,message,listview";
        }
    }
    /**
     * INPUTBAR
     */
    else if ( strcmp ( name, "inputbar" ) == 0 ) {
        wid      = (widget *) box_create ( parent_widget, name, ROFI_ORIENTATION_HORIZONTAL );
        defaults = "prompt,entry,overlay,case-indicator";
        box_add ( (box *) parent_widget, WIDGET ( wid ), FALSE );
    }
    /**
     * PROMPT
     */
    else if ( strcmp ( name, "prompt" ) == 0 ) {
        if ( state->prompt != NULL ) {
            g_error ( "Prompt widget can only be added once to the layout." );
            return;
        }
        // Prompt box.
        state->prompt = textbox_create ( parent_widget, WIDGET_TYPE_TEXTBOX_TEXT, name, TB_AUTOWIDTH | TB_AUTOHEIGHT, NORMAL, "", 0, 0 );
        rofi_view_update_prompt ( state );
        box_add ( (box *) parent_widget, WIDGET ( state->prompt ), FALSE );
        defaults = NULL;
    }
    else if ( strcmp ( name, "num-rows" ) == 0 ) {
        state->tb_total_rows = textbox_create ( parent_widget, WIDGET_TYPE_TEXTBOX_TEXT, name, TB_AUTOWIDTH | TB_AUTOHEIGHT, NORMAL, "", 0, 0 );
        box_add ( (box *) parent_widget, WIDGET ( state->tb_total_rows ), FALSE );
        defaults = NULL;
    }
    else if ( strcmp ( name, "num-filtered-rows" ) == 0 ) {
        state->tb_filtered_rows = textbox_create ( parent_widget, WIDGET_TYPE_TEXTBOX_TEXT, name, TB_AUTOWIDTH | TB_AUTOHEIGHT, NORMAL, "", 0, 0 );
        box_add ( (box *) parent_widget, WIDGET ( state->tb_filtered_rows ), FALSE );
        defaults = NULL;
    }
    /**
     * CASE INDICATOR
     */
    else if ( strcmp ( name, "case-indicator" ) == 0 ) {
        if ( state->case_indicator != NULL ) {
            g_error ( "Case indicator widget can only be added once to the layout." );
            return;
        }
        state->case_indicator = textbox_create ( parent_widget, WIDGET_TYPE_TEXTBOX_TEXT, name, TB_AUTOWIDTH | TB_AUTOHEIGHT, NORMAL, "*", 0, 0 );
        // Add small separator between case indicator and text box.
        box_add ( (box *) parent_widget, WIDGET ( state->case_indicator ), FALSE );
        textbox_text ( state->case_indicator, get_matching_state () );
    }
    /**
     * ENTRY BOX
     */
    else if ( strcmp ( name, "entry" ) == 0 ) {
        if ( state->text != NULL ) {
            g_error ( "Entry textbox widget can only be added once to the layout." );
            return;
        }
        // Entry box
        TextboxFlags tfl = TB_EDITABLE;
        tfl        |= ( ( state->menu_flags & MENU_PASSWORD ) == MENU_PASSWORD ) ? TB_PASSWORD : 0;
        state->text = textbox_create ( parent_widget, WIDGET_TYPE_EDITBOX, name, tfl | TB_AUTOHEIGHT, NORMAL, NULL, 0, 0 );
        box_add ( (box *) parent_widget, WIDGET ( state->text ), TRUE );
    }
    /**
     * MESSAGE
     */
    else if ( strcmp ( name, "message" ) == 0 ) {
        if ( state->mesg_box != NULL ) {
            g_error ( "Message widget can only be added once to the layout." );
            return;
        }
        state->mesg_box = container_create ( parent_widget, name );
        state->mesg_tb  = textbox_create ( WIDGET ( state->mesg_box ), WIDGET_TYPE_TEXTBOX_TEXT, "textbox", TB_AUTOHEIGHT | TB_MARKUP | TB_WRAP, NORMAL, NULL, 0, 0 );
        container_add ( state->mesg_box, WIDGET ( state->mesg_tb ) );
        rofi_view_reload_message_bar ( state );
        box_add ( (box *) parent_widget, WIDGET ( state->mesg_box ), FALSE );
    }
    /**
     * LISTVIEW
     */
    else if ( strcmp ( name, "listview" ) == 0 ) {
        if ( state->list_view != NULL ) {
            g_error ( "Listview widget can only be added once to the layout." );
            return;
        }
        state->list_view = listview_create ( parent_widget, name, update_callback, state, config.element_height, 0 );
        box_add ( (box *) parent_widget, WIDGET ( state->list_view ), TRUE );
        // Set configuration
        listview_set_multi_select ( state->list_view, ( state->menu_flags & MENU_INDICATOR ) == MENU_INDICATOR );
        listview_set_scroll_type ( state->list_view, config.scroll_method );
        listview_set_mouse_activated_cb ( state->list_view, rofi_view_listview_mouse_activated_cb, state );

        int lines = rofi_theme_get_integer ( WIDGET ( state->list_view ), "lines", config.menu_lines );
        listview_set_num_lines ( state->list_view, lines );
        listview_set_max_lines ( state->list_view, state->num_lines );
    }
    /**
     * MODE SWITCHER
     */
    else if ( strcmp ( name, "mode-switcher" ) == 0 || strcmp ( name, "sidebar" ) == 0 ) {
        if ( state->sidebar_bar != NULL ) {
            g_error ( "Mode-switcher can only be added once to the layout." );
            return;
        }
        state->sidebar_bar = box_create ( parent_widget, name, ROFI_ORIENTATION_HORIZONTAL );
        box_add ( (box *) parent_widget, WIDGET ( state->sidebar_bar ), FALSE );
        state->num_modi = rofi_get_num_enabled_modi ();
        state->modi     = g_malloc0 ( state->num_modi * sizeof ( textbox * ) );
        for ( unsigned int j = 0; j < state->num_modi; j++ ) {
            const Mode * mode = rofi_get_mode ( j );
            state->modi[j] = textbox_create ( WIDGET ( state->sidebar_bar ), WIDGET_TYPE_MODE_SWITCHER, "button", TB_AUTOHEIGHT, ( mode == state->sw ) ? HIGHLIGHT : NORMAL,
                                              mode_get_display_name ( mode  ), 0.5, 0.5 );
            box_add ( state->sidebar_bar, WIDGET ( state->modi[j] ), TRUE );
            widget_set_trigger_action_handler ( WIDGET ( state->modi[j] ), textbox_sidebar_modi_trigger_action, state );
        }
    }
    else if ( g_ascii_strcasecmp ( name, "overlay" ) == 0 ) {
        state->overlay = textbox_create ( WIDGET ( parent_widget ), WIDGET_TYPE_TEXTBOX_TEXT, "overlay", TB_AUTOWIDTH | TB_AUTOHEIGHT, URGENT, "blaat", 0.5, 0 );
        box_add ( (box *) parent_widget, WIDGET ( state->overlay ), FALSE );
        widget_disable ( WIDGET ( state->overlay ) );
    }
    else if (  g_ascii_strncasecmp ( name, "textbox", 7 ) == 0 ) {
        textbox *t = textbox_create ( parent_widget, WIDGET_TYPE_TEXTBOX_TEXT, name, TB_AUTOHEIGHT | TB_WRAP, NORMAL, "", 0, 0 );
        box_add ( (box *) parent_widget, WIDGET ( t ), TRUE );
    }
    else if (  g_ascii_strncasecmp ( name, "button", 6 ) == 0 ) {
        textbox *t = textbox_create ( parent_widget, WIDGET_TYPE_EDITBOX, name, TB_AUTOHEIGHT | TB_WRAP, NORMAL, "", 0, 0 );
        box_add ( (box *) parent_widget, WIDGET ( t ), TRUE );
        widget_set_trigger_action_handler ( WIDGET ( t ), textbox_button_trigger_action, state );
    }
    else if (  g_ascii_strncasecmp ( name, "icon", 4 ) == 0 ) {
        icon *t = icon_create ( parent_widget, name );
        box_add ( (box *) parent_widget, WIDGET ( t ), TRUE );
    }
    else {
        wid = (widget *) box_create ( parent_widget, name, ROFI_ORIENTATION_VERTICAL );
        box_add ( (box *) parent_widget, WIDGET ( wid ), TRUE );
        //g_error("The widget %s does not exists. Invalid layout.", name);
    }
    if ( wid ) {
        GList *list = rofi_theme_get_list ( wid, "children", defaults );
        for ( const GList *iter = list; iter != NULL; iter = g_list_next ( iter ) ) {
            rofi_view_add_widget ( state, wid, (const char *) iter->data );
        }
        g_list_free_full ( list, g_free );
    }
}

RofiViewState *rofi_view_create ( Mode *sw,
                                  const char *input,
                                  MenuFlags menu_flags,
                                  void ( *finalize )( RofiViewState * ) )
{
    TICK ();
    RofiViewState *state = __rofi_view_state_create ();
    state->menu_flags    = menu_flags;
    state->sw            = sw;
    state->selected_line = UINT32_MAX;
    state->retv          = MENU_CANCEL;
    state->distance      = NULL;
    state->quit          = FALSE;
    state->skip_absorb   = FALSE;
    //We want to filter on the first run.
    state->refilter   = TRUE;
    state->finalize   = finalize;
    state->mouse_seen = FALSE;

    // Request the lines to show.
    state->num_lines = mode_get_num_entries ( sw );

    if ( state->sw ) {
        char * title = g_strdup_printf ( "rofi - %s", mode_get_display_name ( state->sw ) );
        rofi_view_set_window_title ( title );
        g_free ( title );
    }
    else {
        rofi_view_set_window_title ( "rofi" );
    }
    TICK_N ( "Startup notification" );

    // Get active monitor size.
    TICK_N ( "Get active monitor" );

    state->main_window = box_create ( NULL, "window", ROFI_ORIENTATION_VERTICAL );
    // Get children.
    GList *list = rofi_theme_get_list ( WIDGET ( state->main_window ), "children", "mainbox" );
    for ( const GList *iter = list; iter != NULL; iter = g_list_next ( iter ) ) {
        rofi_view_add_widget ( state, WIDGET ( state->main_window ), (const char *) iter->data );
    }
    g_list_free_full ( list, g_free );

    if ( state->text && input ) {
        textbox_text ( state->text, input );
        textbox_cursor_end ( state->text );
    }

    // filtered list
    state->line_map = g_malloc0_n ( state->num_lines, sizeof ( unsigned int ) );
    state->distance = (int *) g_malloc0_n ( state->num_lines, sizeof ( int ) );

    rofi_view_calculate_window_width ( state );
    // Need to resize otherwise calculated desired height is wrong.
    widget_resize ( WIDGET ( state->main_window ), state->width, 100 );
    // Only needed when window is fixed size.
    if ( ( CacheState.flags & MENU_NORMAL_WINDOW ) == MENU_NORMAL_WINDOW ) {
        listview_set_fixed_num_lines ( state->list_view );
    }

    state->height = rofi_view_calculate_height ( state );
    // Move the window to the correct x,y position.
    rofi_view_calculate_window_position ( state );
    rofi_view_window_update_size ( state );

    state->quit = FALSE;
    rofi_view_refilter ( state );
    rofi_view_update ( state, TRUE );
    xcb_map_window ( xcb->connection, CacheState.main_window );
    widget_queue_redraw ( WIDGET ( state->main_window ) );
    xcb_flush ( xcb->connection );

    /* When Override Redirect, the WM will not let us know we can take focus, so just steal it */
    if ( ( ( menu_flags & MENU_NORMAL_WINDOW ) == 0 ) ) {
        rofi_xcb_set_input_focus(CacheState.main_window);
    }

    if ( xcb->sncontext != NULL ) {
        sn_launchee_context_complete ( xcb->sncontext );
    }
    return state;
}

int rofi_view_error_dialog ( const char *msg, int markup )
{
    RofiViewState *state = __rofi_view_state_create ();
    state->retv       = MENU_CANCEL;
    state->menu_flags = MENU_ERROR_DIALOG;
    state->finalize   = process_result;

    state->main_window = box_create ( NULL, "window", ROFI_ORIENTATION_VERTICAL );
    box *box = box_create ( WIDGET ( state->main_window ), "error-message", ROFI_ORIENTATION_VERTICAL );
    box_add ( state->main_window, WIDGET ( box ), TRUE );
    state->text = textbox_create ( WIDGET ( box ), WIDGET_TYPE_TEXTBOX_TEXT, "textbox", ( TB_AUTOHEIGHT | TB_WRAP ) + ( ( markup ) ? TB_MARKUP : 0 ),
                                   NORMAL, ( msg != NULL ) ? msg : "", 0, 0 );
    box_add ( box, WIDGET ( state->text ), TRUE );

    // Make sure we enable fixed num lines when in normal window mode.
    if ( ( CacheState.flags & MENU_NORMAL_WINDOW ) == MENU_NORMAL_WINDOW ) {
        listview_set_fixed_num_lines ( state->list_view );
    }
    rofi_view_calculate_window_width ( state );
    // Need to resize otherwise calculated desired height is wrong.
    widget_resize ( WIDGET ( state->main_window ), state->width, 100 );
    // resize window vertically to suit
    state->height = widget_get_desired_height ( WIDGET ( state->main_window ) );

    // Calculte window position.
    rofi_view_calculate_window_position ( state );

    // Move the window to the correct x,y position.
    rofi_view_window_update_size ( state );

    // Display it.
    xcb_map_window ( xcb->connection, CacheState.main_window );
    widget_queue_redraw ( WIDGET ( state->main_window ) );

    if ( xcb->sncontext != NULL ) {
        sn_launchee_context_complete ( xcb->sncontext );
    }

    // Set it as current window.
    rofi_view_set_active ( state );
    return TRUE;
}

void rofi_view_hide ( void )
{
    if ( CacheState.main_window != XCB_WINDOW_NONE ) {
        rofi_xcb_revert_input_focus();
        xcb_unmap_window ( xcb->connection, CacheState.main_window );
        display_early_cleanup ();
    }
}

void rofi_view_cleanup ()
{
    g_debug ( "Cleanup." );
    if ( CacheState.idle_timeout > 0 ) {
        g_source_remove ( CacheState.idle_timeout );
        CacheState.idle_timeout = 0;
    }
    if ( CacheState.repaint_source > 0 ) {
        g_source_remove ( CacheState.repaint_source );
        CacheState.repaint_source = 0;
    }
    if ( CacheState.fake_bg ) {
        cairo_surface_destroy ( CacheState.fake_bg );
        CacheState.fake_bg = NULL;
    }
    if ( CacheState.edit_draw ) {
        cairo_destroy ( CacheState.edit_draw );
        CacheState.edit_draw = NULL;
    }
    if ( CacheState.edit_surf ) {
        cairo_surface_destroy ( CacheState.edit_surf );
        CacheState.edit_surf = NULL;
    }
    if ( CacheState.main_window != XCB_WINDOW_NONE ) {
        g_debug ( "Unmapping and free'ing window" );
        xcb_unmap_window ( xcb->connection, CacheState.main_window );
        rofi_xcb_revert_input_focus();
        xcb_free_gc ( xcb->connection, CacheState.gc );
        xcb_free_pixmap ( xcb->connection, CacheState.edit_pixmap );
        xcb_destroy_window ( xcb->connection, CacheState.main_window );
        CacheState.main_window = XCB_WINDOW_NONE;
    }
    if ( map != XCB_COLORMAP_NONE ) {
        xcb_free_colormap ( xcb->connection, map );
        map = XCB_COLORMAP_NONE;
    }
    xcb_flush ( xcb->connection );
    g_assert ( g_queue_is_empty ( &( CacheState.views ) ) );
}
void rofi_view_workers_initialize ( void )
{
    TICK_N ( "Setup Threadpool, start" );
    if ( config.threads == 0 ) {
        config.threads = 1;
        long procs = sysconf ( _SC_NPROCESSORS_CONF );
        if ( procs > 0 ) {
            config.threads = MIN ( procs, 128l );
        }
    }
    // Create thread pool
    GError *error = NULL;
    tpool = g_thread_pool_new ( rofi_view_call_thread, NULL, config.threads, FALSE, &error );
    if ( error == NULL ) {
        // Idle threads should stick around for a max of 60 seconds.
        g_thread_pool_set_max_idle_time ( 60000 );
        // We are allowed to have
        g_thread_pool_set_max_threads ( tpool, config.threads, &error );
    }
    // If error occurred during setup of pool, tell user and exit.
    if ( error != NULL ) {
        g_warning ( "Failed to setup thread pool: '%s'", error->message );
        g_error_free ( error );
        exit ( EXIT_FAILURE );
    }
    TICK_N ( "Setup Threadpool, done" );
}
void rofi_view_workers_finalize ( void )
{
    if ( tpool ) {
        g_thread_pool_free ( tpool, TRUE, TRUE );
        tpool = NULL;
    }
}
Mode * rofi_view_get_mode ( RofiViewState *state )
{
    return state->sw;
}

void rofi_view_set_overlay ( RofiViewState *state, const char *text )
{
    if ( state->overlay == NULL || state->list_view == NULL ) {
        return;
    }
    if ( text == NULL ) {
        widget_disable ( WIDGET ( state->overlay ) );
        return;
    }
    widget_enable ( WIDGET ( state->overlay ) );
    textbox_text ( state->overlay, text );
    // We want to queue a repaint.
    rofi_view_queue_redraw ( );
}

void rofi_view_clear_input ( RofiViewState *state )
{
    if ( state->text ) {
        textbox_text ( state->text, "" );
        rofi_view_set_selected_line ( state, 0 );
    }
}

void rofi_view_ellipsize_start ( RofiViewState *state )
{
    listview_set_ellipsize_start ( state->list_view );
}

void rofi_view_switch_mode ( RofiViewState *state, Mode *mode )
{
    state->sw = mode;
    // Update prompt;
    if ( state->prompt ) {
        rofi_view_update_prompt ( state );
    }
    if ( state->sw ) {
        char * title = g_strdup_printf ( "rofi - %s", mode_get_display_name ( state->sw ) );
        rofi_view_set_window_title ( title );
        g_free ( title );
    }
    else {
        rofi_view_set_window_title ( "rofi" );
    }
    if ( state->sidebar_bar ) {
        for ( unsigned int j = 0; j < state->num_modi; j++ ) {
            const Mode * mode = rofi_get_mode ( j );
            textbox_font ( state->modi[j], ( mode == state->sw ) ? HIGHLIGHT : NORMAL );
        }
    }
    rofi_view_restart ( state );
    state->reload   = TRUE;
    state->refilter = TRUE;
    rofi_view_refilter ( state );
    rofi_view_update ( state, TRUE );
}

xcb_window_t rofi_view_get_window ( void )
{
    return CacheState.main_window;
}

void rofi_view_set_window_title ( const char * title )
{
    ssize_t len = strlen ( title );
    xcb_change_property ( xcb->connection, XCB_PROP_MODE_REPLACE, CacheState.main_window, xcb->ewmh._NET_WM_NAME, xcb->ewmh.UTF8_STRING, 8, len, title );
    xcb_change_property ( xcb->connection, XCB_PROP_MODE_REPLACE, CacheState.main_window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, len, title );
}
