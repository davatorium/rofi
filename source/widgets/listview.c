/**
 *   MIT/X11 License
 *   Modified  (c) 2016 Qball Cow <qball@gmpclient.org>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining
 *   a copy of this software and associated documentation files (the
 *   "Software"), to deal in the Software without restriction, including
 *   without limitation the rights to use, copy, modify, merge, publish,
 *   distribute, sublicense, and/or sell copies of the Software, and to
 *   permit persons to whom the Software is furnished to do so, subject to
 *   the following conditions:
 *
 *   The above copyright notice and this permission notice shall be
 *   included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <config.h>
#include <glib.h>
#include <widgets/widget.h>
#include <widgets/textbox.h>
#include <widgets/listview.h>
#include <widgets/scrollbar.h>

#include "settings.h"
#include "theme.h"

#define DEFAULT_SPACING 2

struct _listview
{
    widget                      widget;
    // RChanged
    // Text needs to be repainted.
    unsigned int                rchanged;
    // Administration

    unsigned int                cur_page;
    unsigned int                last_offset;
    unsigned int                selected;

    unsigned int                element_height;
    unsigned int                element_width;
    unsigned int                max_rows;
    unsigned int                max_elements;

    //
    unsigned int                cur_columns;
    unsigned int                req_elements;
    unsigned int                cur_elements;

    Distance                    spacing;
    unsigned int                menu_lines;
    unsigned int                max_displayed_lines;
    unsigned int                menu_columns;
    unsigned int                fixed_num_lines;
    unsigned int                dynamic;
    unsigned int                eh;
    gboolean                    cycle;
    gboolean                    multi_select;

    ScrollType                  scroll_type;

    textbox                     **boxes;
    scrollbar                   *scrollbar;

    listview_update_callback    callback;
    void                        *udata;

    gboolean                    scrollbar_scroll;

    xcb_timestamp_t             last_click;
    listview_mouse_activated_cb mouse_activated;
    void                        *mouse_activated_data;
};

static int listview_get_desired_height ( widget *wid );

static void listview_free ( widget *wid )
{
    listview *lv = (listview *) wid;
    for ( unsigned int i = 0; i < lv->cur_elements; i++ ) {
        widget_free ( WIDGET ( lv->boxes [i] ) );
    }
    g_free ( lv->boxes );

    widget_free ( WIDGET ( lv->scrollbar ) );
    g_free ( lv );
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

    if ( lv->callback ) {
        lv->callback ( lv->boxes[tb], index, lv->udata, type, full );
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
    scrollbar_set_handle ( lv->scrollbar, lv->selected  );
    lv->last_offset = offset;
    int spacing = distance_get_pixel ( lv->spacing );
    if ( lv->cur_elements > 0 && lv->max_rows > 0 ) {
        // Set new x/y possition.
        unsigned int max = MIN ( lv->cur_elements, lv->req_elements - offset );
        if ( lv->rchanged ) {
            unsigned int width = lv->widget.w - spacing * ( lv->cur_columns - 1 );
            width -= widget_padding_get_padding_width ( wid );
            if ( widget_enabled ( WIDGET ( lv->scrollbar ) ) ) {
                width -= spacing;
                width -= widget_get_width ( WIDGET ( lv->scrollbar ) );
            }
            unsigned int element_width = ( width ) / lv->cur_columns;
            for ( unsigned int i = 0; i < max; i++ ) {
                unsigned int ex = widget_padding_get_left ( wid ) + ( ( i ) / lv->max_rows ) * ( element_width + spacing );
                unsigned int ey = widget_padding_get_top ( wid ) + ( ( i ) % lv->max_rows ) * ( lv->element_height + spacing );
                textbox_moveresize ( lv->boxes[i], ex, ey, element_width, lv->element_height );

                update_element ( lv, i, i + offset, TRUE );
                widget_draw ( WIDGET ( lv->boxes[i] ), draw );
            }
            lv->rchanged = FALSE;
        }
        else {
            for ( unsigned int i = 0; i < max; i++ ) {
                update_element ( lv, i, i + offset, FALSE );
                widget_draw ( WIDGET ( lv->boxes[i] ), draw );
            }
        }
        widget_draw ( WIDGET ( lv->scrollbar ), draw );
    }
}

static void listview_recompute_elements ( listview *lv )
{
    unsigned int newne = 0;
    if ( lv->max_rows == 0 ) {
        return;
    }
    if ( lv->req_elements < lv->max_elements ) {
        newne           = lv->req_elements;
        lv->cur_columns = ( lv->req_elements + ( lv->max_rows - 1 ) ) / lv->max_rows;
    }
    else {
        newne           = lv->max_elements;
        lv->cur_columns = lv->menu_columns;
    }
    for ( unsigned int i = newne; i < lv->cur_elements; i++ ) {
        widget_free ( WIDGET ( lv->boxes[i] ) );
    }
    lv->boxes = g_realloc ( lv->boxes, newne * sizeof ( textbox* ) );
    if ( newne > 0   ) {
        for ( unsigned int i = lv->cur_elements; i < newne; i++ ) {
            TextboxFlags flags = ( lv->multi_select ) ? TB_INDICATOR : 0;
            char *name = g_strjoin (".", lv->widget.name,"element", NULL);
            lv->boxes[i] = textbox_create ( name, flags, NORMAL, "" );
            g_free ( name );
        }
    }
    lv->rchanged = TRUE;
    lv->cur_elements = newne;
}

void listview_set_num_elements ( listview *lv, unsigned int rows )
{
    lv->req_elements = rows;
    listview_set_selected ( lv, lv->selected );
    listview_recompute_elements ( lv );
    widget_queue_redraw ( WIDGET ( lv ) );
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
        lv->selected = MIN ( selected, lv->req_elements - 1 );
        widget_queue_redraw ( WIDGET ( lv ) );
    }
}

static void listview_resize ( widget *wid, short w, short h )
{
    listview *lv = (listview *) wid;
    lv->widget.w     = MAX ( 0, w );
    lv->widget.h     = MAX ( 0, h );
    int height       = lv->widget.h - widget_padding_get_padding_height ( WIDGET (lv) );
    int spacing      = distance_get_pixel ( lv->spacing );
    lv->max_rows     = ( spacing + height ) / ( lv->element_height + spacing );
    lv->max_elements = lv->max_rows * lv->menu_columns;

    widget_move ( WIDGET ( lv->scrollbar ),
            lv->widget.w - widget_padding_get_right ( WIDGET ( lv ) ) - widget_get_width ( WIDGET ( lv->scrollbar ) ),
            widget_padding_get_top ( WIDGET (lv ) ));
    widget_resize (  WIDGET ( lv->scrollbar ), widget_get_width ( WIDGET ( lv->scrollbar ) ), height );

    listview_recompute_elements ( lv );
    widget_queue_redraw ( wid );
}

static gboolean listview_scrollbar_clicked ( widget *sb, xcb_button_press_event_t * xce, void *udata )
{
    listview     *lv = (listview *) udata;

    unsigned int sel = scrollbar_clicked ( (scrollbar *) sb, xce->event_y );
    listview_set_selected ( lv, sel );

    return TRUE;
}

static gboolean listview_clicked ( widget *wid, xcb_button_press_event_t *xce, G_GNUC_UNUSED void *udata )
{
    listview *lv = (listview *) wid;
    lv->scrollbar_scroll = FALSE;
    if ( widget_enabled ( WIDGET ( lv->scrollbar ) ) && widget_intersect ( WIDGET ( lv->scrollbar ), xce->event_x, xce->event_y ) ) {
        // Forward to handler of scrollbar.
        xcb_button_press_event_t xce2 = *xce;
        xce->event_x        -= widget_get_x_pos ( WIDGET ( lv->scrollbar ) );
        xce->event_y        -= widget_get_y_pos ( WIDGET ( lv->scrollbar ) );
        lv->scrollbar_scroll = TRUE;
        return widget_clicked ( WIDGET ( lv->scrollbar ), &xce2 );
    }
    // Handle the boxes.
    unsigned int max = MIN ( lv->cur_elements, lv->req_elements - lv->last_offset );
    for ( unsigned int i = 0; i < max; i++ ) {
        widget *w = WIDGET ( lv->boxes[i] );
        if ( widget_intersect ( w, xce->event_x, xce->event_y ) ) {
            if ( ( lv->last_offset + i ) == lv->selected ) {
                if ( ( xce->time - lv->last_click ) < 200 ) {
                    // Somehow signal we accepted item.
                    lv->mouse_activated ( lv, xce, lv->mouse_activated_data );
                }
            }
            else {
                listview_set_selected ( lv, lv->last_offset + i );
            }
            lv->last_click = xce->time;
            return TRUE;
        }
    }
    return FALSE;
}

static gboolean listview_motion_notify ( widget *wid, xcb_motion_notify_event_t *xme )
{
    listview *lv = (listview *) wid;
    if ( widget_enabled ( WIDGET ( lv->scrollbar ) ) && lv->scrollbar_scroll ) {
        xcb_motion_notify_event_t xle = *xme;
        xle.event_x -= wid->x;
        xle.event_y -= wid->y;
        widget_motion_notify ( WIDGET ( lv->scrollbar ), &xle );
        return TRUE;
    }

    return FALSE;
}
listview *listview_create ( const char *name, listview_update_callback cb, void *udata, unsigned int eh )
{
    listview *lv = g_malloc0 ( sizeof ( listview ) );

    widget_init ( WIDGET ( lv ), name, "@listview" );
    lv->widget.free               = listview_free;
    lv->widget.resize             = listview_resize;
    lv->widget.draw               = listview_draw;
    lv->widget.clicked            = listview_clicked;
    lv->widget.motion_notify      = listview_motion_notify;
    lv->widget.get_desired_height = listview_get_desired_height;
    lv->widget.enabled            = TRUE;
    lv->eh = eh;

    char *n = g_strjoin(".", lv->widget.name,"scrollbar", NULL);
    lv->scrollbar = scrollbar_create ( n, 4);
    g_free(n);
    widget_set_clicked_handler ( WIDGET ( lv->scrollbar ), listview_scrollbar_clicked, lv );
    lv->scrollbar->widget.parent = WIDGET ( lv );
    // Calculate height of an element.
    //
    char *tb_name = g_strjoin (".", lv->widget.name,"element", NULL);
    textbox *tb = textbox_create ( tb_name, 0, NORMAL, "" );
    lv->element_height = textbox_get_estimated_height (tb, lv->eh);
    g_free(tb_name);

    lv->callback = cb;
    lv->udata    = udata;

    // Some settings.
    lv->spacing         = rofi_theme_get_distance (lv->widget.class_name, lv->widget.name, NULL,  "spacing", DEFAULT_SPACING );
    lv->menu_columns    = rofi_theme_get_integer  (lv->widget.class_name, lv->widget.name, NULL, "columns", config.menu_columns );
    lv->fixed_num_lines = rofi_theme_get_boolean  (lv->widget.class_name, lv->widget.name, NULL, "fixed-height", config.fixed_num_lines );
    lv->dynamic         = rofi_theme_get_boolean  (lv->widget.class_name, lv->widget.name, NULL, "dynamic",      TRUE );

    listview_set_show_scrollbar ( lv, rofi_theme_get_boolean ( lv->widget.class_name, lv->widget.name, NULL, "scrollbar", !config.hide_scrollbar ));
    listview_set_scrollbar_width ( lv, rofi_theme_get_integer ( lv->widget.class_name, lv->widget.name, NULL, "scrollbar-width", config.scrollbar_width ));
    lv->cycle = rofi_theme_get_boolean ( lv->widget.class_name, lv->widget.name, NULL,  "cycle", config.cycle );


    return lv;
}

/**
 * Navigation commands.
 */

void listview_nav_up ( listview *lv )
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
    widget_queue_redraw ( WIDGET ( lv ) );
}
void listview_nav_down ( listview *lv )
{
    if ( lv == NULL ) {
        return;
    }
    if ( lv->req_elements == 0 || ( lv->selected == ( lv->req_elements - 1 ) && !lv->cycle ) ) {
        return;
    }
    lv->selected = lv->selected < lv->req_elements - 1 ? MIN ( lv->req_elements - 1, lv->selected + 1 ) : 0;

    widget_queue_redraw ( WIDGET ( lv ) );
}

void listview_nav_left ( listview *lv )
{
    if ( lv == NULL ) {
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

void listview_nav_page_prev ( listview *lv )
{
    if ( lv == NULL ) {
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
void listview_nav_page_next ( listview *lv )
{
    if ( lv == NULL ) {
        return;
    }
    if ( lv->req_elements == 0 ) {
        return;
    }
    lv->selected += ( lv->max_elements );
    if ( lv->selected >= lv->req_elements ) {
        lv->selected = lv->req_elements - 1;
    }
    widget_queue_redraw ( WIDGET ( lv ) );
}

static int listview_get_desired_height ( widget *wid )
{
    listview *lv = (listview *)wid;
    int spacing = distance_get_pixel ( lv->spacing );
    if ( lv == NULL  || lv->widget.enabled == FALSE ) {
        return 0;
    }
    int h = lv->menu_lines;
    if ( !( lv->fixed_num_lines ) ) {
        if ( lv->dynamic ) {
            h = MIN ( lv->menu_lines, lv->req_elements );
        } else {
            h = MIN ( lv->menu_lines, lv->max_displayed_lines );
        }
    }
    if ( h == 0 ) {
        return widget_padding_get_padding_height ( WIDGET (lv) );
    }
    int height = widget_padding_get_padding_height ( WIDGET (lv) );
    height += h*(lv->element_height+spacing)  - spacing;
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
void listview_set_scrollbar_width ( listview *lv, unsigned int width )
{
    if ( lv ) {
        scrollbar_set_width ( lv->scrollbar, width );
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
void listview_set_max_lines ( listview *lv, unsigned int max_lines )
{
    if ( lv ) {
        lv->max_displayed_lines = max_lines;
    }
}
