#ifndef ROFI_LISTVIEW_H
#define ROFI_LISTVIEW_H

/**
 * @defgroup listview listview
 * @ingroup widgets
 *
 * @{
 */

typedef struct _listview   listview;

typedef void ( *listview_update_callback )( textbox *tb, unsigned int entry, void *udata, TextBoxFontType type );

listview *listview_create ( listview_update_callback cb, void *udata );

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
/* @} */

#endif // ROFI_LISTVIEW_H
