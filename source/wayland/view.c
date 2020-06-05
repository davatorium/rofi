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

#include <cairo.h>

/** Indicated we understand the startup notification api is not yet stable.*/
#define SN_API_NOT_YET_FROZEN
#include <libsn/sn.h>
#include "rofi.h"

#include "timings.h"
#include "settings.h"

#include "mode.h"
#include "display.h"
#include "helper.h"
#include "helper-theme.h"
#include "xrmoptions.h"
#include "dialogs/dialogs.h"

#include "view.h"
#include "view-internal.h"

#include "theme.h"

/**
 * @param state The handle to the view
 * @param qr    Indicate if queue_redraw should be called on changes.
 *
 * Update the state of the view. This involves filter state.
 */
static void wayland_rofi_view_update ( RofiViewState *state, gboolean qr );

static int wayland_rofi_view_calculate_window_height ( RofiViewState *state );

static void wayland_rofi_view_maybe_update ( RofiViewState *state );

static RofiViewState * wayland_rofi_view_get_active ( void );

/** Thread pool used for filtering */
extern GThreadPool *tpool;

/** Global pointer to the currently active RofiViewState */
static RofiViewState *current_active_menu = NULL;

/**
 * Structure holding cached state.
 */
static struct
{
    /** Main flags */
    MenuFlags          flags;
    /** List of stacked views */
    GQueue             views;
    /** timeout for reloading */
    guint              idle_timeout;
    /** debug counter for redraws */
    unsigned long long count;
    /** redraw idle time. */
    guint              repaint_source;
    /** Window fullscreen */
    gboolean           fullscreen;
} CacheState = {
    .flags          = MENU_NORMAL,
    .views          = G_QUEUE_INIT,
    .idle_timeout   = 0,
    .count          = 0L,
    .repaint_source = 0,
    .fullscreen     = FALSE,
};

static void wayland_rofi_view_get_current_monitor ( int *width, int *height )
{
    display_get_surface_dimensions( width, height );
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
 * Stores a screenshot of Rofi at that point in time.
 */
static void wayland_rofi_view_capture_screenshot ( void )
{
}

static gboolean wayland_rofi_view_repaint ( G_GNUC_UNUSED void * data  )
{
    if ( current_active_menu  ) {
        // Repaint the view (if needed).
        // After a resize the edit_pixmap surface might not contain anything anymore.
        // If we already re-painted, this does nothing.
        wayland_rofi_view_maybe_update( current_active_menu );
        CacheState.repaint_source = 0;
    }
    return G_SOURCE_REMOVE;
}

static void wayland_rofi_view_update_prompt ( RofiViewState *state )
{
    if ( state->prompt ) {
        const char *str = mode_get_display_name ( state->sw );
        textbox_text ( state->prompt, str );
    }
}

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

static void wayland_rofi_view_calculate_window_position ( RofiViewState *state )
{
}

static int rofi_get_location ( RofiViewState *state )
{
    return rofi_theme_get_position ( WIDGET ( state->main_window ), "location", loc_transtable[config.location] );
}

static void wayland_rofi_view_window_update_size ( RofiViewState * state )
{
    widget_resize ( WIDGET ( state->main_window ), state->width, state->height );
    display_set_surface_dimensions ( state->width, state->height, rofi_get_location(state) );
}

static void wayland_rofi_view_set_size ( RofiViewState * state, gint width, gint height )
{
    if ( width > -1 )
        state->width = width;
    if ( height > -1 )
        state->height = height;
    wayland_rofi_view_window_update_size(state);
}

static void wayland_rofi_view_get_size ( RofiViewState * state, gint *width, gint *height )
{
    *width = state->width;
    *height = state->height;
}

static gboolean wayland_rofi_view_reload_idle ( G_GNUC_UNUSED gpointer data )
{
    RofiViewState *state = wayland_rofi_view_get_active ();

    if ( current_active_menu ) {
        current_active_menu->reload   = TRUE;
        current_active_menu->refilter = TRUE;

        wayland_rofi_view_maybe_update ( state );
    }
    CacheState.idle_timeout = 0;
    return G_SOURCE_REMOVE;
}

static void wayland_rofi_view_reload ( void  )
{
    // @TODO add check if current view is equal to the callee
    if ( CacheState.idle_timeout == 0 ) {
        CacheState.idle_timeout = g_timeout_add ( 1000 / 15, wayland_rofi_view_reload_idle, NULL );
    }
}
static void wayland_rofi_view_queue_redraw ( void  )
{
    if ( current_active_menu && CacheState.repaint_source == 0 ) {
        CacheState.count++;
        g_debug ( "redraw %llu", CacheState.count );

        widget_queue_redraw ( WIDGET ( current_active_menu->main_window ) );

        CacheState.repaint_source = g_idle_add_full (  G_PRIORITY_HIGH_IDLE, wayland_rofi_view_repaint, NULL, NULL );
    }
}

static RofiViewState * wayland_rofi_view_get_active ( void )
{
    return current_active_menu;
}

static void wayland_rofi_view_set_active ( RofiViewState *state )
{
    if ( current_active_menu != NULL && state != NULL ) {
        g_queue_push_head ( &( CacheState.views ), current_active_menu );
        // TODO check.
        current_active_menu = state;
        g_debug ( "stack view." );
        wayland_rofi_view_window_update_size ( current_active_menu );
        wayland_rofi_view_queue_redraw ();
        return;
    }
    else if ( state == NULL && !g_queue_is_empty ( &( CacheState.views ) ) ) {
        g_debug ( "pop view." );
        current_active_menu = g_queue_pop_head ( &( CacheState.views ) );
        wayland_rofi_view_window_update_size ( current_active_menu );
        wayland_rofi_view_queue_redraw ();
        return;
    }
    g_assert ( ( current_active_menu == NULL && state != NULL ) || ( current_active_menu != NULL && state == NULL ) );
    current_active_menu = state;
    wayland_rofi_view_queue_redraw ();
}

static void wayland_rofi_view_set_selected_line ( RofiViewState *state, unsigned int selected_line )
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
}

static void wayland_rofi_view_free ( RofiViewState *state )
{
    if ( state->tokens ) {
        helper_tokenize_free ( state->tokens );
        state->tokens = NULL;
    }
    // Do this here?
    // Wait for final release?
    widget_free ( WIDGET ( state->main_window ) );

    g_free ( state->pool );
    g_free ( state->line_map );
    g_free ( state->distance );
    // Free the switcher boxes.
    // When state is free'ed we should no longer need these.
    g_free ( state->modi );
    state->num_modi = 0;
    g_free ( state );
}

static const char * wayland_rofi_view_get_user_input ( const RofiViewState *state )
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

static void wayland___create_window ( MenuFlags menu_flags )
{
    // FIXME: create surface
    // FIXME: roll next buffer

    TICK_N ( "create cairo surface" );
    // TODO should we update the drawable each time?
    PangoContext *p = pango_context_new (  );
    pango_context_set_font_map(p, pango_cairo_font_map_get_default());
    TICK_N ( "pango cairo font setup" );

    CacheState.flags       = menu_flags;
    // Setup dpi
    // FIXME: use scale, cairo_surface_set_device_scale
    // Setup font.
    // Dummy widget.
    container *win  = container_create ( NULL, "window.box" );
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

    widget_free ( WIDGET ( win ) );
    TICK_N ( "done" );
}

/**
 * @param state Internal state of the menu.
 *
 * Calculate the width of the window and the width of an element.
 */
static void wayland_rofi_view_calculate_window_width ( RofiViewState *state )
{
    if ( config.menu_width < 0 ) {
        double fw = textbox_get_estimated_char_width ( );
        state->width  = -( fw * config.menu_width );
        state->width += widget_padding_get_padding_width ( WIDGET ( state->main_window ) );
    }
    else {
        int width = 1920;
        // Calculate as float to stop silly, big rounding down errors.
        display_get_surface_dimensions( &width, NULL );
        state->width = config.menu_width < 101 ? ( (float)width / 100.0f ) * ( float ) config.menu_width : config.menu_width;
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
static void wayland_rofi_view_nav_row_tab ( RofiViewState *state )
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
inline static void wayland_rofi_view_nav_row_select ( RofiViewState *state )
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
inline static void wayland_rofi_view_nav_first ( RofiViewState * state )
{
//    state->selected = 0;
    listview_set_selected ( state->list_view, 0 );
}

/**
 * @param state The current RofiViewState
 *
 * Move the selection to last row.
 */
inline static void wayland_rofi_view_nav_last ( RofiViewState * state )
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

static void wayland_rofi_view_update ( RofiViewState *state, gboolean qr )
{
    if ( !widget_need_redraw ( WIDGET ( state->main_window ) ) ) {
        return;
    }
    g_debug ( "Redraw view" );
    TICK ();
    if ( state->pool == NULL ) {
        state->pool = display_buffer_pool_new(state->width, state->height);
    }
    cairo_surface_t *surface = display_buffer_pool_get_next_buffer(state->pool);
    cairo_t *d = cairo_create(surface);
    cairo_set_operator ( d, CAIRO_OPERATOR_SOURCE );
    // Paint the background transparent.
    cairo_set_source_rgba ( d, 0, 0, 0, 0.0 );
    cairo_paint ( d );
    TICK_N ( "Background" );

    // Always paint as overlay over the background.
    cairo_set_operator ( d, CAIRO_OPERATOR_OVER );
    widget_draw ( WIDGET ( state->main_window ), d );

    TICK_N ( "widgets" );
    cairo_destroy(d);
    display_surface_commit ( surface );

    if ( qr ) {
        wayland_rofi_view_queue_redraw ();
    }
}

/**
 * @param state The Menu Handle
 *
 * Check if a finalize function is set, and if sets executes it.
 */
void process_result ( RofiViewState *state );

static void wayland_rofi_view_trigger_global_action ( KeyBindingAction action )
{
    RofiViewState *state = wayland_rofi_view_get_active ();
    switch ( action )
    {
    case PASTE_PRIMARY:
        // TODO
        break;
    case PASTE_SECONDARY:
        // TODO
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
        wayland_rofi_view_nav_row_tab ( state );
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
        wayland_rofi_view_nav_first ( state );
        break;
    case ROW_LAST:
        wayland_rofi_view_nav_last ( state );
        break;
    case ROW_SELECT:
        wayland_rofi_view_nav_row_select ( state );
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

static gboolean wayland_rofi_view_trigger_action ( RofiViewState *state, BindingsScope scope, guint action )
{
    switch ( scope )
    {
    case SCOPE_GLOBAL:
        wayland_rofi_view_trigger_global_action ( action );
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

static void wayland_rofi_view_handle_text ( RofiViewState *state, char *text )
{
    if ( textbox_append_text ( state->text, text, strlen ( text ) ) ) {
        state->refilter = TRUE;
    }
}

static void wayland_rofi_view_maybe_update ( RofiViewState *state )
{
    if ( rofi_view_get_completed ( state ) ) {
        // This menu is done.
        rofi_view_finalize ( state );
        // If there a state. (for example error) reload it.
        state = wayland_rofi_view_get_active ();

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
    wayland_rofi_view_update ( state, TRUE );
}

static void wayland_rofi_view_frame_callback ( void )
{
    if ( CacheState.repaint_source == 0 ) {
        CacheState.repaint_source = g_idle_add_full (  G_PRIORITY_HIGH_IDLE, wayland_rofi_view_repaint, NULL, NULL );
    }
}

static int wayland_rofi_view_calculate_window_height ( RofiViewState *state )
{
    if ( CacheState.fullscreen == TRUE ) {
        int height = 1080;
        display_get_surface_dimensions( NULL, &height );
        return height;
    }

    RofiDistance h      = rofi_theme_get_distance ( WIDGET ( state->main_window ), "height", 0 );
    unsigned int height = distance_get_pixel ( h, ROFI_ORIENTATION_VERTICAL );
    // If height is set, return it.
    if ( height > 0 ) {
        return height;
    }
    // Autosize based on widgets.
    widget *main_window = WIDGET ( state->main_window );

    height = widget_get_desired_height ( main_window );
    return height;
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
static void wayland_rofi_view_listview_mouse_activated_cb ( listview *lv, gboolean custom, void *udata )
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

static void wayland_rofi_view_add_widget ( RofiViewState *state, widget *parent_widget, const char *name )
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
        wayland_rofi_view_update_prompt ( state );
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
        listview_set_mouse_activated_cb ( state->list_view, wayland_rofi_view_listview_mouse_activated_cb, state );

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
            wayland_rofi_view_add_widget ( state, wid, (const char *) iter->data );
        }
        g_list_free_full ( list, g_free );
    }
}

static RofiViewState *wayland_rofi_view_create ( Mode *sw,
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
    state->frame_callback = TRUE;

    // Request the lines to show.
    state->num_lines = mode_get_num_entries ( sw );

    state->main_window = box_create ( NULL, "window", ROFI_ORIENTATION_VERTICAL );
    // Get children.
    GList *list = rofi_theme_get_list ( WIDGET ( state->main_window ), "children", "mainbox" );
    for ( const GList *iter = list; iter != NULL; iter = g_list_next ( iter ) ) {
        wayland_rofi_view_add_widget ( state, WIDGET ( state->main_window ), (const char *) iter->data );
    }
    g_list_free_full ( list, g_free );

    if ( state->text && input ) {
        textbox_text ( state->text, input );
        textbox_cursor_end ( state->text );
    }

    // filtered list
    state->line_map = g_malloc0_n ( state->num_lines, sizeof ( unsigned int ) );
    state->distance = (int *) g_malloc0_n ( state->num_lines, sizeof ( int ) );

    wayland_rofi_view_calculate_window_width ( state );
    // Need to resize otherwise calculated desired height is wrong.
    widget_resize ( WIDGET ( state->main_window ), state->width, 100 );
    // Only needed when window is fixed size.
    if ( ( CacheState.flags & MENU_NORMAL_WINDOW ) == MENU_NORMAL_WINDOW ) {
        listview_set_fixed_num_lines ( state->list_view );
    }

    state->height = wayland_rofi_view_calculate_window_height ( state );
    // Move the window to the correct x,y position.
    wayland_rofi_view_calculate_window_position ( state );
    wayland_rofi_view_window_update_size ( state );

    state->quit = FALSE;
    rofi_view_refilter ( state );
    wayland_rofi_view_update ( state, TRUE );
    widget_queue_redraw ( WIDGET ( state->main_window ) );
    return state;
}

static int wayland_rofi_view_error_dialog ( const char *msg, int markup )
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
    wayland_rofi_view_calculate_window_width ( state );
    // Need to resize otherwise calculated desired height is wrong.
    widget_resize ( WIDGET ( state->main_window ), state->width, 100 );
    // resize window vertically to suit
    state->height = widget_get_desired_height ( WIDGET ( state->main_window ) );

    // Calculte window position.
    wayland_rofi_view_calculate_window_position ( state );

    // Move the window to the correct x,y position.
    wayland_rofi_view_window_update_size ( state );

    // Set it as current window.
    wayland_rofi_view_set_active ( state );
    return TRUE;
}

static void wayland_rofi_view_hide ( void )
{
}

static void wayland_rofi_view_cleanup ()
{
    //g_log ( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Cleanup." );
    if ( CacheState.idle_timeout > 0 ) {
        g_source_remove ( CacheState.idle_timeout );
        CacheState.idle_timeout = 0;
    }
    if ( CacheState.repaint_source > 0 ) {
        g_source_remove ( CacheState.repaint_source );
        CacheState.repaint_source = 0;
    }
    g_assert ( g_queue_is_empty ( &( CacheState.views ) ) );
}

static Mode * wayland_rofi_view_get_mode ( RofiViewState *state )
{
    return state->sw;
}

static void wayland_rofi_view_set_overlay ( RofiViewState *state, const char *text )
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
    wayland_rofi_view_queue_redraw ( );
}

static void wayland_rofi_view_clear_input ( RofiViewState *state )
{
    if ( state->text ) {
        textbox_text ( state->text, "" );
        wayland_rofi_view_set_selected_line ( state, 0 );
    }
}

static void wayland_rofi_view_switch_mode ( RofiViewState *state, Mode *mode )
{
    state->sw = mode;
    // Update prompt;
    if ( state->prompt ) {
        wayland_rofi_view_update_prompt ( state );
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
    wayland_rofi_view_update ( state, TRUE );
}

static view_proxy view_ = {
    .create = wayland_rofi_view_create,
    .handle_text = wayland_rofi_view_handle_text,
    .maybe_update = wayland_rofi_view_maybe_update,
    .temp_configure_notify = NULL,
    .temp_click_to_exit = NULL,
    .frame_callback = wayland_rofi_view_frame_callback,
    .get_user_input = wayland_rofi_view_get_user_input,
    .set_selected_line = wayland_rofi_view_set_selected_line,
    .trigger_action = wayland_rofi_view_trigger_action,
    .free = wayland_rofi_view_free,
    .get_active = wayland_rofi_view_get_active,
    .set_active = wayland_rofi_view_set_active,
    .error_dialog = wayland_rofi_view_error_dialog,
    .queue_redraw = wayland_rofi_view_queue_redraw,

    .calculate_window_position = wayland_rofi_view_calculate_window_position,
    .calculate_window_height = wayland_rofi_view_calculate_window_height,
    .window_update_size = wayland_rofi_view_window_update_size,

    .cleanup = wayland_rofi_view_cleanup,
    .get_mode = wayland_rofi_view_get_mode,
    .hide = wayland_rofi_view_hide,
    .reload = wayland_rofi_view_reload,
    .switch_mode = wayland_rofi_view_switch_mode,
    .set_overlay = wayland_rofi_view_set_overlay,
    .clear_input = wayland_rofi_view_clear_input,
    .__create_window = wayland___create_window,
    .get_window = NULL,
    .get_current_monitor = wayland_rofi_view_get_current_monitor,
    .capture_screenshot = wayland_rofi_view_capture_screenshot,

    .set_size = wayland_rofi_view_set_size,
    .get_size = wayland_rofi_view_get_size,
};

const view_proxy *wayland_view_proxy = &view_;
