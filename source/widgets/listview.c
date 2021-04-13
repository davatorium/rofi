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

#include <config.h>
#include <glib.h>
#include <widgets/widget.h>
#include <widgets/textbox.h>
#include <widgets/scrollbar.h>
#include <widgets/icon.h>
#include <widgets/box.h>
#include <widgets/listview.h>

#include "settings.h"
#include "theme.h"

#include "timings.h"

/** Default spacing between the elements in the listview. */
#define DEFAULT_SPACING    2

/**
 * Orientation of the listview
 */
/** Vertical (classical) list */
#define  LISTVIEW    ROFI_ORIENTATION_VERTICAL
/** Horizontal list. (barview) */
#define BARVIEW      ROFI_ORIENTATION_HORIZONTAL

/**
 * The moving direction of the selection, this (in barview) affects the scrolling.
 */
typedef enum
{
    LEFT_TO_RIGHT = 0,
    RIGHT_TO_LEFT = 1
} MoveDirection;

typedef struct
{
    box     *box;
    textbox *textbox;
    textbox *index;
    icon    *icon;
} _listview_row;

struct _listview
{
    widget          widget;

    RofiOrientation type;

    // RChanged
    // Text needs to be repainted.
    unsigned int                rchanged;
    // Administration

    unsigned int                cur_page;
    unsigned int                last_offset;
    unsigned int                selected;

    unsigned int                element_height;
    unsigned int                max_rows;
    unsigned int                max_elements;

    //
    gboolean                    fixed_columns;
    unsigned int                cur_columns;
    unsigned int                req_elements;
    unsigned int                cur_elements;

    RofiDistance                spacing;
    unsigned int                menu_lines;
    unsigned int                max_displayed_lines;
    unsigned int                menu_columns;
    unsigned int                fixed_num_lines;
    unsigned int                dynamic;
    unsigned int                eh;
    unsigned int                reverse;
    gboolean                    cycle;
    gboolean                    multi_select;

    ScrollType                  scroll_type;

    _listview_row               * boxes;
    scrollbar                   *scrollbar;

    listview_update_callback    callback;
    void                        *udata;

    gboolean                    scrollbar_scroll;

    xcb_timestamp_t             last_click;
    listview_mouse_activated_cb mouse_activated;
    void                        *mouse_activated_data;

    char                        *listview_name;

    PangoEllipsizeMode          emode;
    /** Barview */
    struct
    {
        MoveDirection direction;
        unsigned int  cur_visible;
    } barview;
};
/**
 * Names used for theming the elements in the listview.
 * Each row can have 3 modes,  normal, selected and alternate.
 * Each row can have 3 states, normal, urgent and active.
 */
const char *const listview_theme_prop_names[][3] = {
    /** Normal row */
    { "normal.normal", "selected.normal", "alternate.normal" },
    /** Urgent row */
    { "normal.urgent", "selected.urgent", "alternate.urgent" },
    /** Active row */
    { "normal.active", "selected.active", "alternate.active" },
};

static void listview_set_state ( _listview_row r, TextBoxFontType tbft )
{
    widget          *w = WIDGET ( r.box );
    TextBoxFontType t  = tbft & STATE_MASK;
    if ( w == NULL ) {
        return;
    }
    // ACTIVE has priority over URGENT if both set.
    if ( t == ( URGENT | ACTIVE ) ) {
        t = ACTIVE;
    }
    switch ( ( tbft & FMOD_MASK ) )
    {
    case HIGHLIGHT:
        widget_set_state ( w, listview_theme_prop_names[t][1] );
        break;
    case ALT:
        widget_set_state ( w, listview_theme_prop_names[t][2] );
        break;
    default:
        widget_set_state ( w, listview_theme_prop_names[t][0] );
        break;
    }
}
static void listview_add_widget ( listview *lv, _listview_row *row, widget *wid, const char *label )
{
    TextboxFlags flags = ( lv->multi_select ) ? TB_INDICATOR : 0;
    if ( strcasecmp ( label, "element-icon" ) == 0 ) {
        if ( config.show_icons ) {
            row->icon = icon_create ( WIDGET ( wid ), "element-icon" );
            box_add ( (box *) wid, WIDGET ( row->icon ), FALSE );
        }
    }
    else if ( strcasecmp ( label, "element-text" ) == 0 ) {
        row->textbox = textbox_create ( WIDGET ( wid ), WIDGET_TYPE_TEXTBOX_TEXT, "element-text", TB_AUTOHEIGHT | flags, NORMAL, "DDD", 0, 0 );
        box_add ( (box *) wid, WIDGET ( row->textbox ), TRUE );
    }
    else if ( strcasecmp ( label, "element-index" ) == 0 ) {
        row->index = textbox_create ( WIDGET ( wid ), WIDGET_TYPE_TEXTBOX_TEXT, "element-text", TB_AUTOHEIGHT, NORMAL, " ", 0, 0 );
        box_add ( (box *) wid, WIDGET ( row->index ), FALSE );
    }
    else {
        widget *wid2 = (widget *) box_create ( wid, label, ROFI_ORIENTATION_VERTICAL );
        box_add ( (box *) wid, WIDGET ( wid2 ), TRUE );
        GList  *list = rofi_theme_get_list ( WIDGET ( wid2 ), "children", "" );
        for ( GList *iter = g_list_first ( list ); iter != NULL; iter = g_list_next ( iter ) ) {
            listview_add_widget ( lv, row, wid2, (const char *) iter->data );
        }
    }
}

static void listview_create_row ( listview *lv, _listview_row *row )
{
    row->box = box_create ( WIDGET ( lv ), "element", ROFI_ORIENTATION_HORIZONTAL );
    widget_set_type ( WIDGET ( row->box ), WIDGET_TYPE_LISTVIEW_ELEMENT );
    GList *list = rofi_theme_get_list ( WIDGET ( row->box ), "children", "element-icon,element-text" );

    row->textbox = NULL;
    row->icon    = NULL;
    row->index   = NULL;

    for ( GList *iter = g_list_first ( list ); iter != NULL; iter = g_list_next ( iter ) ) {
        listview_add_widget ( lv, row, WIDGET ( row->box ), (const char *) iter->data );
    }
    g_list_free_full ( list, g_free );
}

static int listview_get_desired_height ( widget *wid );

static void listview_free ( widget *wid )
{
    listview *lv = (listview *) wid;
    for ( unsigned int i = 0; i < lv->cur_elements; i++ ) {
        widget_free ( WIDGET ( lv->boxes [i].box ) );
    }
    g_free ( lv->boxes );

    g_free ( lv->listview_name );
    widget_free ( WIDGET ( lv->scrollbar ) );
    g_free ( lv );
}
static unsigned int scroll_per_page_barview ( listview *lv )
{
    unsigned int offset = lv->last_offset;

    // selected row is always visible.
    // If selected is visible do not scroll.
    if ( lv->selected < lv->last_offset ) {
        offset       = lv->selected;
        lv->rchanged = TRUE;
    }
    else if ( lv->selected >= ( lv->last_offset + lv->barview.cur_visible ) ) {
        offset       = lv->selected;
        lv->rchanged = TRUE;
    }
    return offset;
}
static unsigned int scroll_per_page ( listview * lv )
{
    int offset = 0;

    // selected row is always visible.
    // If selected is visible do not scroll.
    if ( ( ( lv->selected - ( lv->last_offset ) ) < ( lv->max_elements ) ) && ( lv->selected >= ( lv->last_offset ) ) ) {
        offset = lv->last_offset;
    }
    else{
        // Do paginating
        unsigned int page = ( lv->max_elements > 0 ) ? ( lv->selected / lv->max_elements ) : 0;
        offset = page * lv->max_elements;
        if ( page != lv->cur_page ) {
            lv->cur_page = page;
            lv->rchanged = TRUE;
        }
        // Set the position
        //scrollbar_set_handle ( lv->scrollbar, page * lv->max_elements );
    }
    return offset;
}

static unsigned int scroll_continious ( listview *lv )
{
    unsigned int middle = ( lv->max_rows - ( ( lv->max_rows & 1 ) == 0 ) ) / 2;
    unsigned int offset = 0;
    if ( lv->selected > middle ) {
        if ( lv->selected < ( lv->req_elements - ( lv->max_rows - middle ) ) ) {
            offset = lv->selected - middle;
        }
        // Don't go below zero.
        else if ( lv->req_elements > lv->max_rows ) {
            offset = lv->req_elements - lv->max_rows;
        }
    }
    if ( offset != lv->cur_page ) {
        //scrollbar_set_handle ( lv->scrollbar, offset );
        lv->cur_page = offset;
        lv->rchanged = TRUE;
    }
    return offset;
}

static void update_element ( listview *lv, unsigned int tb, unsigned int index, gboolean full )
{
    // Select drawing mode
    TextBoxFontType type = ( index & 1 ) == 0 ? NORMAL : ALT;
    type = ( index ) == lv->selected ? HIGHLIGHT : type;

    if ( lv->boxes[tb].index ) {
        if ( index < 10 ) {
            char str[2] = {
                ( ( index + 1 ) % 10 ) + '0',
                '\0'
            };
            textbox_text ( lv->boxes[tb].index, str );
        }
        else {
            textbox_text ( lv->boxes[tb].index, " " );
        }
    }
    if ( lv->callback ) {
        lv->callback ( lv->boxes[tb].textbox, lv->boxes[tb].icon, index, lv->udata, &type, full );
        listview_set_state ( lv->boxes[tb], type );
    }
}

static void barview_draw ( widget *wid, cairo_t *draw )
{
    unsigned int offset = 0;
    listview     *lv    = (listview *) wid;
    offset          = scroll_per_page_barview ( lv );
    lv->last_offset = offset;
    int spacing_hori = distance_get_pixel ( lv->spacing, ROFI_ORIENTATION_HORIZONTAL );

    int left_offset  = widget_padding_get_left ( wid );
    int right_offset = lv->widget.w - widget_padding_get_right ( wid );
    int top_offset   = widget_padding_get_top ( wid );
    if ( lv->cur_elements > 0 ) {
        // Set new x/y position.
        unsigned int max = MIN ( lv->cur_elements, lv->req_elements - offset );
        if ( lv->rchanged ) {
            int first = TRUE;
            int width = lv->widget.w;
            lv->barview.cur_visible = 0;
            width                  -= widget_padding_get_padding_width ( wid );
            if ( lv->barview.direction == LEFT_TO_RIGHT ) {
                for ( unsigned int i = 0; i < max && width > 0; i++ ) {
                    update_element ( lv, i, i + offset, TRUE );
                    int twidth = widget_get_desired_width ( WIDGET ( lv->boxes[i].box ) );
                    if ( twidth >= width ) {
                        if ( !first ) {
                            break;
                        }
                        twidth = width;
                    }
                    widget_move ( WIDGET ( lv->boxes[i].box ), left_offset, top_offset );
                    widget_resize ( WIDGET ( lv->boxes[i].box ), twidth, lv->element_height );

                    widget_draw ( WIDGET ( lv->boxes[i].box ), draw );
                    width       -= twidth + spacing_hori;
                    left_offset += twidth + spacing_hori;
                    first        = FALSE;
                    lv->barview.cur_visible++;
                }
            }
            else {
                for ( unsigned int i = 0; i < lv->cur_elements && width > 0 && i <= offset; i++ ) {
                    update_element ( lv, i, offset - i, TRUE );
                    int twidth = widget_get_desired_width ( WIDGET ( lv->boxes[i].box ) );
                    if ( twidth >= width ) {
                        if ( !first ) {
                            break;
                        }
                        twidth = width;
                    }
                    right_offset -= twidth;
                    widget_move ( WIDGET ( lv->boxes[i].box ), right_offset, top_offset );
                    widget_resize ( WIDGET ( lv->boxes[i].box ), twidth, lv->element_height );

                    widget_draw ( WIDGET ( lv->boxes[i].box ), draw );
                    width        -= twidth + spacing_hori;
                    right_offset -= spacing_hori;
                    first         = FALSE;
                    lv->barview.cur_visible++;
                }
                offset         -= lv->barview.cur_visible - 1;
                lv->last_offset = offset;
                for  ( unsigned int i = 0; i < ( lv->barview.cur_visible / 2 ); i++ ) {
                    _listview_row temp = lv->boxes[i];
                    int           sw   = lv->barview.cur_visible - i - 1;
                    lv->boxes[i]  = lv->boxes[sw];
                    lv->boxes[sw] = temp;
                }
            }
            lv->rchanged = FALSE;
        }
        else {
            for ( unsigned int i = 0; i < lv->barview.cur_visible; i++ ) {
                update_element ( lv, i, i + offset, TRUE );
                widget_draw ( WIDGET ( lv->boxes[i].box ), draw );
            }
        }
    }
}

static void listview_draw ( widget *wid, cairo_t *draw )
{
    unsigned int offset = 0;
    listview     *lv    = (listview *) wid;
    if ( lv->scroll_type == LISTVIEW_SCROLL_CONTINIOUS ) {
        offset = scroll_continious ( lv );
    }
    else {
        offset = scroll_per_page ( lv );
    }
    // Set these all together to make sure they update consistently.
    scrollbar_set_max_value ( lv->scrollbar, lv->req_elements );
    scrollbar_set_handle_length ( lv->scrollbar, lv->cur_columns * lv->max_rows );
    if ( lv->reverse ) {
        scrollbar_set_handle ( lv->scrollbar, lv->req_elements - lv->selected - 1 );
    }
    else {
        scrollbar_set_handle ( lv->scrollbar, lv->selected  );
    }
    lv->last_offset = offset;
    int spacing_vert = distance_get_pixel ( lv->spacing, ROFI_ORIENTATION_VERTICAL );
    int spacing_hori = distance_get_pixel ( lv->spacing, ROFI_ORIENTATION_HORIZONTAL );

    int left_offset = widget_padding_get_left ( wid );
    int top_offset  = widget_padding_get_top ( wid );
    /*
       if ( lv->scrollbar->widget.index == 0 ) {
        left_offset += spacing_hori + lv->scrollbar->widget.w;
       }
     */
    if ( lv->cur_elements > 0 && lv->max_rows > 0 ) {
        // Set new x/y position.
        unsigned int max = MIN ( lv->cur_elements, lv->req_elements - offset );
        if ( lv->rchanged ) {
            unsigned int width = lv->widget.w;
            width -= widget_padding_get_padding_width ( wid );
            if ( widget_enabled ( WIDGET ( lv->scrollbar ) ) ) {
                width -= spacing_hori;
                width -= widget_get_width ( WIDGET ( lv->scrollbar ) );
            }
            unsigned int element_width = ( width - spacing_hori * ( lv->cur_columns - 1 ) ) / lv->cur_columns;

            int          d = width - ( element_width + spacing_hori ) * ( lv->cur_columns - 1 ) - element_width;
            if ( lv->cur_columns > 1 ) {
                int diff = d / ( lv->cur_columns - 1 );
                if ( diff >= 1 ) {
                    spacing_hori += 1;
                    d            -= lv->cur_columns - 1;
                }
            }
            for ( unsigned int i = 0; i < max; i++ ) {
                unsigned int ex = left_offset + ( ( i ) / lv->max_rows ) * ( element_width + spacing_hori );

                if ( ( i ) / lv->max_rows == ( lv->cur_columns - 1 ) ) {
                    ex += d;
                }
                if ( lv->reverse ) {
                    unsigned int ey = wid->h - ( widget_padding_get_bottom ( wid ) + ( ( i ) % lv->max_rows ) * ( lv->element_height + spacing_vert ) ) - lv->element_height;
                    widget_move ( WIDGET ( lv->boxes[i].box ), ex, ey );
                    widget_resize ( WIDGET ( lv->boxes[i].box ), element_width, lv->element_height );
                }
                else {
                    unsigned int ey = top_offset + ( ( i ) % lv->max_rows ) * ( lv->element_height + spacing_vert );
                    widget_move ( WIDGET ( lv->boxes[i].box ), ex, ey );
                    widget_resize ( WIDGET ( lv->boxes[i].box ), element_width, lv->element_height );
                }

                update_element ( lv, i, i + offset, TRUE );
                widget_draw ( WIDGET ( lv->boxes[i].box ), draw );
            }
            lv->rchanged = FALSE;
        }
        else {
            for ( unsigned int i = 0; i < max; i++ ) {
                update_element ( lv, i, i + offset, TRUE );
                widget_draw ( WIDGET ( lv->boxes[i].box ), draw );
            }
        }
    }
    widget_draw ( WIDGET ( lv->scrollbar ), draw );
}
static WidgetTriggerActionResult listview_element_trigger_action ( widget *wid, MouseBindingListviewElementAction action, gint x, gint y, void *user_data );
static gboolean listview_element_motion_notify ( widget *wid, gint x, gint y );

static void _listview_draw ( widget *wid, cairo_t *draw )
{
    listview *lv = (listview *) wid;
    if ( lv->type == LISTVIEW ) {
        listview_draw ( wid, draw );
    }
    else {
        barview_draw ( wid, draw );
    }
}
/**
 * State names used for theming.
 */
static void listview_recompute_elements ( listview *lv )
{
    unsigned int newne = 0;
    if ( lv->max_rows == 0 ) {
        return;
    }
    if ( !( lv->fixed_columns ) && lv->req_elements < lv->max_elements ) {
        newne           = lv->req_elements;
        lv->cur_columns = ( lv->req_elements + ( lv->max_rows - 1 ) ) / lv->max_rows;
    }
    else {
        newne           = MIN ( lv->req_elements, lv->max_elements );
        lv->cur_columns = lv->menu_columns;
    }
    for ( unsigned int i = newne; i < lv->cur_elements; i++ ) {
        widget_free ( WIDGET ( lv->boxes[i].box ) );
    }
    lv->boxes = g_realloc ( lv->boxes, newne * sizeof ( _listview_row ) );
    if ( newne > 0   ) {
        for ( unsigned int i = lv->cur_elements; i < newne; i++ ) {
            listview_create_row ( lv, &( lv->boxes[i] ) );
            widget *wid = WIDGET ( lv->boxes[i].box );
            widget_set_trigger_action_handler ( wid, listview_element_trigger_action, lv );
            if ( wid != NULL ) {
                wid->motion_notify = listview_element_motion_notify;
            }

            listview_set_state ( lv->boxes[i], NORMAL );
        }
    }
    lv->rchanged     = TRUE;
    lv->cur_elements = newne;
}

void listview_set_num_elements ( listview *lv, unsigned int rows )
{
    if ( lv == NULL ) {
        return;
    }
    TICK_N ( __FUNCTION__ );
    lv->req_elements = rows;
    listview_set_selected ( lv, lv->selected );
    TICK_N ( "Set selected" );
    listview_recompute_elements ( lv );
    TICK_N ( "recompute elements" );
    widget_queue_redraw ( WIDGET ( lv ) );
    TICK_N ( "queue redraw" );
}

unsigned int listview_get_selected ( listview *lv )
{
    if ( lv != NULL ) {
        return lv->selected;
    }
    return 0;
}

void listview_set_selected ( listview *lv, unsigned int selected )
{
    if ( lv && lv->req_elements > 0 ) {
        lv->selected          = MIN ( selected, lv->req_elements - 1 );
        lv->barview.direction = LEFT_TO_RIGHT;
        widget_queue_redraw ( WIDGET ( lv ) );
    }
}

static void listview_resize ( widget *wid, short w, short h )
{
    listview *lv = (listview *) wid;
    lv->widget.w = MAX ( 0, w );
    lv->widget.h = MAX ( 0, h );
    int height       = lv->widget.h - widget_padding_get_padding_height ( WIDGET ( lv ) );
    int spacing_vert = distance_get_pixel ( lv->spacing, ROFI_ORIENTATION_VERTICAL );
    lv->max_rows     = ( spacing_vert + height ) / ( lv->element_height + spacing_vert );
    lv->max_elements = lv->max_rows * lv->menu_columns;

    widget_move ( WIDGET ( lv->scrollbar ),
                  lv->widget.w - widget_padding_get_right ( WIDGET ( lv ) ) - widget_get_width ( WIDGET ( lv->scrollbar ) ),
                  widget_padding_get_top ( WIDGET ( lv ) ) );

    widget_resize (  WIDGET ( lv->scrollbar ), widget_get_width ( WIDGET ( lv->scrollbar ) ), height );

    if ( lv->type == BARVIEW ) {
        lv->max_elements = lv->menu_lines;
    }

    listview_recompute_elements ( lv );
    widget_queue_redraw ( wid );
}

static widget *listview_find_mouse_target ( widget *wid, WidgetType type, gint x, gint y )
{
    widget   *target = NULL;
    gint     rx, ry;
    listview *lv = (listview *) wid;
    if ( widget_enabled ( WIDGET ( lv->scrollbar ) ) && widget_intersect ( WIDGET ( lv->scrollbar ), x, y ) ) {
        rx     = x - widget_get_x_pos ( WIDGET ( lv->scrollbar ) );
        ry     = y - widget_get_y_pos ( WIDGET ( lv->scrollbar ) );
        target = widget_find_mouse_target ( WIDGET ( lv->scrollbar ), type, rx, ry );
    }

    unsigned int max = MIN ( lv->cur_elements, lv->req_elements - lv->last_offset );
    unsigned int i;
    for ( i = 0; i < max && target == NULL; i++ ) {
        widget *w = WIDGET ( lv->boxes[i].box );
        if ( widget_intersect ( w, x, y ) ) {
            rx     = x - widget_get_x_pos ( w );
            ry     = y - widget_get_y_pos ( w );
            target = widget_find_mouse_target ( w, type, rx, ry );
        }
    }

    return target;
}

static WidgetTriggerActionResult listview_trigger_action ( widget *wid, MouseBindingListviewAction action, G_GNUC_UNUSED gint x, G_GNUC_UNUSED gint y, G_GNUC_UNUSED void *user_data )
{
    listview *lv = (listview *) wid;
    switch ( action )
    {
    case SCROLL_LEFT:
        listview_nav_left ( lv );
        break;
    case SCROLL_RIGHT:
        listview_nav_right ( lv );
        break;
    case SCROLL_DOWN:
        listview_nav_down ( lv );
        break;
    case SCROLL_UP:
        listview_nav_up ( lv );
        break;
    }
    return WIDGET_TRIGGER_ACTION_RESULT_HANDLED;
}

static WidgetTriggerActionResult listview_element_trigger_action ( widget *wid, MouseBindingListviewElementAction action, G_GNUC_UNUSED gint x, G_GNUC_UNUSED gint y, void *user_data )
{
    listview     *lv = (listview *) user_data;
    unsigned int max = MIN ( lv->cur_elements, lv->req_elements - lv->last_offset );
    unsigned int i;
    for ( i = 0; i < max && WIDGET ( lv->boxes[i].box ) != wid; i++ ) {
    }
    if ( i == max ) {
        return WIDGET_TRIGGER_ACTION_RESULT_IGNORED;
    }

    gboolean custom = FALSE;
    switch ( action )
    {
    case SELECT_HOVERED_ENTRY:
        listview_set_selected ( lv, lv->last_offset + i );
        break;
    case ACCEPT_HOVERED_CUSTOM:
        custom = TRUE;
    /* FALLTHRU */
    case ACCEPT_HOVERED_ENTRY:
        listview_set_selected ( lv, lv->last_offset + i );
        lv->mouse_activated ( lv, custom, lv->mouse_activated_data );
        break;
    }
    return WIDGET_TRIGGER_ACTION_RESULT_HANDLED;
}

static gboolean listview_element_motion_notify ( widget *wid, G_GNUC_UNUSED gint x, G_GNUC_UNUSED gint y )
{
    listview     *lv = (listview *) wid->parent;
    unsigned int max = MIN ( lv->cur_elements, lv->req_elements - lv->last_offset );
    unsigned int i;
    for ( i = 0; i < max && WIDGET ( lv->boxes[i].box ) != wid; i++ ) {
    }
    if ( i < max && i != listview_get_selected ( lv ) ) {
        listview_set_selected ( lv, lv->last_offset + i );
    }
    return TRUE;
}

listview *listview_create ( widget *parent, const char *name, listview_update_callback cb, void *udata, unsigned int eh, gboolean reverse )
{
    listview *lv = g_malloc0 ( sizeof ( listview ) );
    widget_init ( WIDGET ( lv ), parent, WIDGET_TYPE_LISTVIEW, name );
    lv->listview_name             = g_strdup ( name );
    lv->widget.free               = listview_free;
    lv->widget.resize             = listview_resize;
    lv->widget.draw               = _listview_draw;
    lv->widget.find_mouse_target  = listview_find_mouse_target;
    lv->widget.trigger_action     = listview_trigger_action;
    lv->widget.get_desired_height = listview_get_desired_height;
    lv->eh                        = eh;

    lv->emode     = PANGO_ELLIPSIZE_END;
    lv->scrollbar = scrollbar_create ( WIDGET ( lv ), "scrollbar" );
    // Calculate height of an element.
    //
    _listview_row row;
    listview_create_row ( lv, &row );
    // FIXME: hack to scale hight correctly.
    if ( lv->eh > 1 && row.textbox ) {
        char buff[lv->eh * 2 + 1];
        memset ( buff, '\0', lv->eh * 2 + 1 );
        for ( unsigned int i = 0; i < ( lv->eh - 1 ); i++ ) {
            buff[i * 2] = 'a'; buff[i * 2 + 1] = '\n';
        }
        ;
        textbox_text ( row.textbox, buff );
    }
    lv->element_height = widget_get_desired_height ( WIDGET ( row.box ) );
    widget_free ( WIDGET ( row.box ) );

    lv->callback = cb;
    lv->udata    = udata;

    // Some settings.
    lv->spacing         = rofi_theme_get_distance ( WIDGET ( lv ), "spacing", DEFAULT_SPACING );
    lv->menu_columns    = rofi_theme_get_integer  ( WIDGET ( lv ), "columns", config.menu_columns );
    lv->fixed_num_lines = rofi_theme_get_boolean  ( WIDGET ( lv ), "fixed-height", config.fixed_num_lines );
    lv->dynamic         = rofi_theme_get_boolean  ( WIDGET ( lv ), "dynamic", TRUE );
    lv->reverse         = rofi_theme_get_boolean  ( WIDGET ( lv ), "reverse", reverse );
    lv->cycle           = rofi_theme_get_boolean ( WIDGET ( lv ), "cycle", config.cycle );
    lv->fixed_columns   = rofi_theme_get_boolean ( WIDGET ( lv ), "fixed-columns", FALSE );

    lv->type = rofi_theme_get_orientation ( WIDGET ( lv ), "layout", ROFI_ORIENTATION_VERTICAL );
    if ( lv->type == LISTVIEW ) {
        listview_set_show_scrollbar ( lv, rofi_theme_get_boolean ( WIDGET ( lv ), "scrollbar", FALSE ) );
    }
    else {
        listview_set_show_scrollbar ( lv, FALSE );
    }
    return lv;
}

/**
 * Navigation commands.
 */

static void listview_nav_up_int ( listview *lv )
{
    if ( lv == NULL ) {
        return;
    }
    if ( lv->req_elements == 0 || ( lv->selected == 0 && !lv->cycle ) ) {
        return;
    }
    if ( lv->selected == 0 ) {
        lv->selected = lv->req_elements;
    }
    lv->selected--;
    lv->barview.direction = RIGHT_TO_LEFT;
    widget_queue_redraw ( WIDGET ( lv ) );
}
static void listview_nav_down_int ( listview *lv )
{
    if ( lv == NULL ) {
        return;
    }
    if ( lv->req_elements == 0 || ( lv->selected == ( lv->req_elements - 1 ) && !lv->cycle ) ) {
        return;
    }
    lv->selected          = lv->selected < lv->req_elements - 1 ? MIN ( lv->req_elements - 1, lv->selected + 1 ) : 0;
    lv->barview.direction = LEFT_TO_RIGHT;
    widget_queue_redraw ( WIDGET ( lv ) );
}

void listview_nav_up ( listview *lv )
{
    if ( lv == NULL ) {
        return;
    }
    if ( lv->reverse ) {
        listview_nav_down_int ( lv );
    }
    else {
        listview_nav_up_int ( lv );
    }
}
void listview_nav_down ( listview *lv )
{
    if ( lv == NULL ) {
        return;
    }
    if ( lv->reverse ) {
        listview_nav_up_int ( lv );
    }
    else {
        listview_nav_down_int ( lv );
    }
}

void listview_nav_left ( listview *lv )
{
    if ( lv == NULL ) {
        return;
    }
    if ( lv->type == BARVIEW ) {
        listview_nav_up_int ( lv );
        return;
    }
    if ( lv->selected >= lv->max_rows ) {
        lv->selected -= lv->max_rows;
        widget_queue_redraw ( WIDGET ( lv ) );
    }
}
void listview_nav_right ( listview *lv )
{
    if ( lv == NULL ) {
        return;
    }
    if ( lv->max_rows == 0 ) {
        return;
    }
    if ( lv->type == BARVIEW ) {
        listview_nav_down_int ( lv );
        return;
    }
    if ( ( lv->selected + lv->max_rows ) < lv->req_elements ) {
        lv->selected += lv->max_rows;
        widget_queue_redraw ( WIDGET ( lv ) );
    }
    else if ( lv->selected < ( lv->req_elements - 1 ) ) {
        // We do not want to move to last item, UNLESS the last column is only
        // partially filled, then we still want to move column and select last entry.
        // First check the column we are currently in.
        int col = lv->selected / lv->max_rows;
        // Check total number of columns.
        int ncol = lv->req_elements / lv->max_rows;
        // If there is an extra column, move.
        if ( col != ncol ) {
            lv->selected = lv->req_elements - 1;
            widget_queue_redraw ( WIDGET ( lv ) );
        }
    }
}

static void listview_nav_page_prev_int ( listview *lv )
{
    if ( lv == NULL ) {
        return;
    }
    if ( lv->type == BARVIEW ) {
        if ( lv->last_offset == 0 ) {
            lv->selected = 0;
        }
        else {
            lv->selected = lv->last_offset - 1;
        }
        lv->barview.direction = RIGHT_TO_LEFT;
        widget_queue_redraw ( WIDGET ( lv ) );
        return;
    }

    if ( lv->selected < lv->max_elements ) {
        lv->selected = 0;
    }
    else{
        lv->selected -= ( lv->max_elements );
    }
    widget_queue_redraw ( WIDGET ( lv ) );
}
static void listview_nav_page_next_int ( listview *lv )
{
    if ( lv == NULL ) {
        return;
    }
    if ( lv->req_elements == 0 ) {
        return;
    }
    if ( lv->type == BARVIEW ) {
        unsigned int new = lv->last_offset + lv->barview.cur_visible;
        lv->selected          = MIN ( new, lv->req_elements - 1 );
        lv->barview.direction = LEFT_TO_RIGHT;

        widget_queue_redraw ( WIDGET ( lv ) );
        return;
    }
    lv->selected += ( lv->max_elements );
    if ( lv->selected >= lv->req_elements ) {
        lv->selected = lv->req_elements - 1;
    }
    widget_queue_redraw ( WIDGET ( lv ) );
}

void listview_nav_page_prev ( listview *lv )
{
    if ( lv == NULL ) {
        return;
    }
    if ( lv->reverse ) {
        listview_nav_page_next_int ( lv );
    }
    else {
        listview_nav_page_prev_int ( lv );
    }
}
void listview_nav_page_next ( listview *lv )
{
    if ( lv == NULL ) {
        return;
    }
    if ( lv->reverse ) {
        listview_nav_page_prev_int ( lv );
    }
    else {
        listview_nav_page_next_int ( lv );
    }
}

static int listview_get_desired_height ( widget *wid )
{
    listview *lv = (listview *) wid;
    if ( lv == NULL || lv->widget.enabled == FALSE ) {
        return 0;
    }
    int spacing = distance_get_pixel ( lv->spacing, ROFI_ORIENTATION_VERTICAL );
    int h       = lv->menu_lines;
    if ( !( lv->fixed_num_lines ) ) {
        if ( lv->dynamic ) {
            h = MIN ( lv->menu_lines, lv->req_elements );
        }
        else {
            h = MIN ( lv->menu_lines, lv->max_displayed_lines );
        }
    }
    if ( lv->type == BARVIEW ) {
        h = MIN ( h, 1 );
    }
    if ( h == 0 ) {
        if ( lv->dynamic && !lv->fixed_num_lines ) {
            // Hide widget fully.
            return 0;
        }
        return widget_padding_get_padding_height ( WIDGET ( lv ) );
    }
    int height = widget_padding_get_padding_height ( WIDGET ( lv ) );
    height += h * ( lv->element_height + spacing ) - spacing;
    return height;
}

void listview_set_show_scrollbar ( listview *lv, gboolean enabled )
{
    if ( lv ) {
        if ( enabled ) {
            widget_enable ( WIDGET ( lv->scrollbar ) );
        }
        else {
            widget_disable ( WIDGET ( lv->scrollbar ) );
        }
        listview_recompute_elements ( lv );
    }
}

void listview_set_scroll_type ( listview *lv, ScrollType type )
{
    if ( lv ) {
        lv->scroll_type = type;
    }
}

void listview_set_mouse_activated_cb ( listview *lv, listview_mouse_activated_cb cb, void *udata )
{
    if ( lv ) {
        lv->mouse_activated      = cb;
        lv->mouse_activated_data = udata;
    }
}
void listview_set_multi_select ( listview *lv, gboolean enable )
{
    if ( lv ) {
        lv->multi_select = enable;
    }
}
void listview_set_num_lines ( listview *lv, unsigned int num_lines )
{
    if ( lv ) {
        lv->menu_lines = num_lines;
    }
}

unsigned int listview_get_num_lines ( listview *lv )
{
    if ( lv ) {
        return lv->menu_lines;
    }
    return 0;
}
void listview_set_max_lines ( listview *lv, unsigned int max_lines )
{
    if ( lv ) {
        lv->max_displayed_lines = max_lines;
    }
}

gboolean listview_get_fixed_num_lines ( listview *lv )
{
    if ( lv ) {
        return lv->fixed_num_lines;
    }
    return FALSE;
}
void listview_set_fixed_num_lines ( listview *lv )
{
    if ( lv  ) {
        lv->fixed_num_lines = TRUE;
    }
}

void listview_set_ellipsize_start ( listview *lv )
{
    if ( lv ) {
        lv->emode = PANGO_ELLIPSIZE_START;
        for ( unsigned int i = 0; i < lv->cur_elements; i++ ) {
            textbox_set_ellipsize ( lv->boxes[i].textbox, lv->emode );
        }
    }
}

void listview_toggle_ellipsizing ( listview *lv )
{
    if ( lv ) {
        PangoEllipsizeMode mode = lv->emode;
        if ( mode == PANGO_ELLIPSIZE_START ) {
            mode = PANGO_ELLIPSIZE_MIDDLE;
        }
        else if ( mode == PANGO_ELLIPSIZE_MIDDLE ) {
            mode = PANGO_ELLIPSIZE_END;
        }
        else if ( mode == PANGO_ELLIPSIZE_END ) {
            mode = PANGO_ELLIPSIZE_START;
        }
        lv->emode = mode;
        for ( unsigned int i = 0; i < lv->cur_elements; i++ ) {
            textbox_set_ellipsize ( lv->boxes[i].textbox, mode );
        }
    }
}
