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

#include <glib.h>
#include <xcb/xcb.h>

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

static const view_proxy *proxy;

void view_init( const view_proxy *view_in )
{
    proxy = view_in;
}

/** Thread pool used for filtering */
GThreadPool *tpool = NULL;

/** Global pointer to the currently active RofiViewState */
RofiViewState *current_active_menu = NULL;

struct _rofi_view_cache_state CacheState = {
    .main_window    = XCB_WINDOW_NONE,
    .flags          = MENU_NORMAL,
    .views          = G_QUEUE_INIT,
};

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

static void rofi_view_update_prompt ( RofiViewState *state )
{
    if ( state->prompt ) {
        const char *str = mode_get_display_name ( state->sw );
        textbox_text ( state->prompt, str );
    }
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

void rofi_view_restart ( RofiViewState *state ) {
    state->quit = FALSE;
    state->retv = MENU_CANCEL;
}

RofiViewState * rofi_view_get_active ( void )
{
    return current_active_menu;
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

    if ( config.backend == DISPLAY_XCB ) {
        xcb_clear_area ( xcb->connection, CacheState.main_window, 1, 0, 0, 1, 1 );
        xcb_flush ( xcb->connection );
    }
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

        if ( state->tokens && config.show_match ) {
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

void rofi_view_refilter ( RofiViewState *state )
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
        if ( config.backend == DISPLAY_XCB ) {
            xcb_convert_selection ( xcb->connection, CacheState.main_window, XCB_ATOM_PRIMARY,
                                    xcb->ewmh.UTF8_STRING, xcb->ewmh.UTF8_STRING, XCB_CURRENT_TIME );
            xcb_flush ( xcb->connection );
        }
        break;
    case PASTE_SECONDARY:
        if ( config.backend == DISPLAY_XCB ) {
            xcb_convert_selection ( xcb->connection, CacheState.main_window, netatoms[CLIPBOARD],
                                    xcb->ewmh.UTF8_STRING, xcb->ewmh.UTF8_STRING, XCB_CURRENT_TIME );
            xcb_flush ( xcb->connection );
        }
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
        state->retv = MENU_QUICK_SWITCH | ( ( action - CUSTOM_1 ) & MENU_LOWER_MASK );
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

void rofi_view_handle_mouse_motion ( RofiViewState *state, gint x, gint y )
{
    state->mouse.x = x;
    state->mouse.y = y;
    if ( state->mouse.motion_target != NULL ) {
        widget_xy_to_relative ( state->mouse.motion_target, &x, &y );
        widget_motion_notify ( state->mouse.motion_target, x, y );
    }
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

    state->height = rofi_view_calculate_window_height ( state );
    // Move the window to the correct x,y position.
    rofi_view_calculate_window_position ( state );
    rofi_view_window_update_size ( state );

    state->quit = FALSE;
    rofi_view_refilter ( state );
    rofi_view_update ( state, TRUE );
    if ( xcb->connection ) {
        xcb_map_window ( xcb->connection, CacheState.main_window );
    }
    widget_queue_redraw ( WIDGET ( state->main_window ) );
    if ( xcb->connection ) {
        xcb_flush ( xcb->connection );
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
    if ( config.backend == DISPLAY_XCB ) {
        xcb_map_window ( xcb->connection, CacheState.main_window );
    }
    widget_queue_redraw ( WIDGET ( state->main_window ) );

    if ( xcb->sncontext != NULL ) {
        sn_launchee_context_complete ( xcb->sncontext );
    }

    // Set it as current window.
    rofi_view_set_active ( state );
    return TRUE;
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

/** ------ */

void rofi_view_update ( RofiViewState *state, gboolean qr ) {
    proxy->update ( state, qr );
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

void rofi_view_queue_redraw ( void ) {
    proxy->queue_redraw ( );
}

void rofi_view_set_window_title ( const char * title ) {
    proxy->set_window_title ( title );
}

void rofi_view_calculate_window_position ( RofiViewState * state )
{
    proxy->calculate_window_position ( state );
}

void rofi_view_calculate_window_width ( struct RofiViewState *state ) {
    proxy->calculate_window_width ( state );
}

int rofi_view_calculate_window_height ( RofiViewState *state )
{
    return proxy->calculate_window_height ( state );
}

void rofi_view_window_update_size ( RofiViewState * state )
{
    proxy->window_update_size ( state );
}

void rofi_view_cleanup ( void ) {
    proxy->cleanup ( );
}

void rofi_view_hide ( void ) {
    proxy->hide ( );
}

void rofi_view_reload ( void  ) {
    proxy->reload ( );
}

void __create_window ( MenuFlags menu_flags ) {
    proxy->__create_window ( menu_flags );
}

xcb_window_t rofi_view_get_window ( void ) {
    return proxy->get_window ( );
}

void rofi_view_get_current_monitor ( int *width, int *height ) {
    proxy->get_current_monitor ( width, height );
}

void rofi_capture_screenshot ( void ) {
    proxy->capture_screenshot ( );
}

void rofi_view_set_size ( RofiViewState * state, gint width, gint height ) {
    proxy->set_size ( state, width, height );
}

void rofi_view_get_size ( RofiViewState * state, gint *width, gint *height ) {
    proxy->get_size ( state, width, height );
}
