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

#include <glib.h>
#include <xcb/xcb.h>
#include "keyb.h"
#include "display.h"

#include "settings.h"
#include "timings.h"

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
    if ( state && state->finalize != NULL ) {
        state->finalize ( state );
    }
}

MenuReturn rofi_view_get_return_value ( const RofiViewState *state ) {
    return state->retv;
}

unsigned int rofi_view_get_next_position ( const RofiViewState *state ) {
    return state->selected_line;
}

void rofi_view_handle_text ( RofiViewState *state, char *text ) {
    proxy->handle_text ( state, text );
}

void rofi_view_handle_mouse_motion ( RofiViewState *state, gint x, gint y ) {
    state->mouse.x = x;
    state->mouse.y = y;
    if ( state->mouse.motion_target != NULL ) {
        widget_xy_to_relative ( state->mouse.motion_target, &x, &y );
        widget_motion_notify ( state->mouse.motion_target, x, y );
    }
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
    return state->quit;
}

const char * rofi_view_get_user_input ( const RofiViewState *state ) {
    if ( state->text ) {
        return state->text->text;
    }
    return NULL;
}

void rofi_view_set_selected_line ( RofiViewState *state, unsigned int selected_line ) {
    proxy->set_selected_line ( state, selected_line );
}

unsigned int rofi_view_get_selected_line ( const RofiViewState *state ) {
    return state->selected_line;
}

void rofi_view_restart ( RofiViewState *state ) {
    state->quit = FALSE;
    state->retv = MENU_CANCEL;
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

void rofi_view_calculate_window_position ( RofiViewState * state )
{
    proxy->calculate_window_position ( state );
}

void rofi_view_window_update_size ( RofiViewState * state )
{
    proxy->window_update_size ( state );
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

int rofi_view_calculate_window_height ( RofiViewState *state )
{
    return proxy->calculate_window_height ( state );
}

void rofi_view_reload_message_bar ( RofiViewState *state )
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

void rofi_view_refilter ( struct RofiViewState *state )
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
    int height = rofi_view_calculate_window_height ( state );
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

/**
 * @param data A thread_state object.
 * @param user_data User data to pass to thread_state callback
 *
 * Small wrapper function that is internally used to pass a job to a worker.
 */
void rofi_view_call_thread ( gpointer data, gpointer user_data )
{
    thread_state *t = (thread_state *) data;
    t->callback ( t, user_data );
}

void rofi_view_workers_initialize ( void ) {
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

void rofi_view_workers_finalize ( void ) {
    if ( tpool ) {
        g_thread_pool_free ( tpool, TRUE, TRUE );
        tpool = NULL;
    }
}

void rofi_view_get_current_monitor ( int *width, int *height ) {
    proxy->get_current_monitor ( width, height );
}

void rofi_capture_screenshot ( void ) {
    proxy->capture_screenshot ( );
}

void rofi_view_ellipsize_start ( RofiViewState *state ) {
    listview_set_ellipsize_start ( state->list_view );
}

void rofi_view_set_size ( RofiViewState * state, gint width, gint height ) {
    proxy->set_size ( state, width, height );
}

void rofi_view_get_size ( RofiViewState * state, gint *width, gint *height ) {
    proxy->get_size ( state, width, height );
}
