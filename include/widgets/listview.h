#ifndef ROFI_LISTVIEW_H
#define ROFI_LISTVIEW_H

/**
 * @defgroup listview listview
 * @ingroup widget
 *
 * @{
 */

typedef struct _listview   listview;

typedef enum
{
    LISTVIEW_SCROLL_PER_PAGE,
    LISTVIEW_SCROLL_CONTINIOUS
} ScrollType;

typedef void ( *listview_update_callback )( textbox *tb, unsigned int entry, void *udata, TextBoxFontType type, gboolean full );
typedef void ( *listview_mouse_activated_cb )( listview *, xcb_button_press_event_t *, void * );

listview *listview_create ( listview_update_callback cb, void *udata, unsigned int eh );

void listview_set_num_elements ( listview *lv, unsigned int rows );
void listview_set_selected ( listview *lv, unsigned int selected );
unsigned int listview_get_selected ( listview *lv );
unsigned int listview_get_desired_height ( listview *lv );

/**
 * @param state The listview handle
 *
 * Move the selection one row up.
 * - Wrap around.
 */
void listview_nav_up ( listview *lv );
/**
 * @param lv listview handle.
 *
 * Move the selection one row down.
 * - Wrap around.
 */
void listview_nav_down ( listview *lv );
/**
 * @param state The listview handle
 *
 * Move the selection one column to the right.
 * - No wrap around.
 * - Do not move to top row when at start.
 */
void listview_nav_right ( listview *lv );
/**
 * @param state The listview handle
 *
 * Move the selection one column to the left.
 * - No wrap around.
 */
void listview_nav_left ( listview *lv );
/**
 * @param state The listview handle
 *
 * Move the selection one page down.
 * - No wrap around.
 * - Clip at top/bottom
 */
void listview_nav_page_next ( listview *lv );

/**
 * @param state The current RofiViewState
 *
 * Move the selection one page up.
 * - No wrap around.
 * - Clip at top/bottom
 */
void listview_nav_page_prev ( listview *lv );

/**
 * @param lv Handler to the listview object
 * @param padding The padding
 *
 * Padding on between the widgets.
 */
void listview_set_padding (  listview *lv, unsigned int padding );

/**
 * @param lv Handler to the listview object
 * @param lines The maximum number of lines 
 *
 * Set the maximum number of lines to show.
 */
void listview_set_max_lines ( listview *lv, unsigned int lines );

/**
 * @param lv Handler to the listview object
 * @param columns The maximum number of columns 
 *
 * Set the maximum number of columns to show.
 */
void listview_set_max_columns ( listview *lv, unsigned int columns );

/**
 * @param lv Handler to the listview object
 * @param enabled enable
 *
 * Set fixed num lines mode.
 */
void listview_set_fixed_num_lines ( listview *lv, gboolean enabled );
/**
 * @param lv Handler to the listview object
 * @param enabled enable
 *
 * Hide the scrollbar.
 */
void listview_set_hide_scrollbar ( listview *lv, gboolean enabled );
/**
 * @param lv Handler to the listview object
 * @param width Width in pixels
 *
 * Set the width of the scrollbar
 */
void listview_set_scrollbar_width ( listview *lv, unsigned int width );

/**
 * @param lv Handler to the listview object
 * @param cycle True for cycle mode
 *
 * Set cycle mode. On last entry go to first. 
 */
void listview_set_cycle ( listview *lv, gboolean cycle );
/**
 * @param lv Handler to the listview object
 * @param type ScrollType
 *
 * Set the scroll type ScrollType::LISTVIEW_SCROLL_CONTINIOUS or ScrollType::LISTVIEW_SCROLL_PER_PAGE
 */
void listview_set_scroll_type ( listview *lv, ScrollType type );

/**
 * @param lv Handler to the listview object
 * @param cb The callback
 * @param udata User data
 *
 * Set the mouse activated callback.
 */
void listview_set_mouse_activated_cb ( listview *lv, listview_mouse_activated_cb cb, void *udata );
/**
 * @param lv Handler to the listview object
 * @param enabled
 *
 * Enable,disable multi-select.
 */
void listview_set_multi_select ( listview *lv, gboolean enable );
/* @} */

#endif // ROFI_LISTVIEW_H
