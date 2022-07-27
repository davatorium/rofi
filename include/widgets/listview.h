/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2017 Qball Cow <qball@gmpclient.org>
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

#ifndef ROFI_LISTVIEW_H
#define ROFI_LISTVIEW_H

#include "widgets/textbox.h"

/**
 * @defgroup listview listview
 * @ingroup widget
 *
 * @{
 */

/**
 * Handle to the listview.
 * No internal fields should be accessed directly.
 */
typedef struct _listview listview;

/**
 * The scrolling type used in the list view
 */
typedef enum {
  /** Flip through the pages. */
  LISTVIEW_SCROLL_PER_PAGE,
  /** keep selected item centered */
  LISTVIEW_SCROLL_CONTINIOUS
} ScrollType;

/**
 * @param tb The textbox to set
 * @param ico The icon to set
 * @param entry The position of the textbox
 * @param udata User data
 * @param type The textbox font style to apply to this entry (normal, selected,
 * alternative row)
 * @param full If true Set both text and style.
 *
 * Update callback, this is called to set the value of each (visible) element.
 */
typedef void (*listview_update_callback)(textbox *tb, icon *ico,
                                         unsigned int entry, void *udata,
                                         TextBoxFontType *type, gboolean full);

/**
 * @param lv The listview
 * @param index the selected row
 * @param udata user data
 *
 * Selection changed callback.
 */
typedef void (*listview_selection_changed_callback)(listview *lv,
                                                    unsigned int index,
                                                    void *udata);
/**
 * Callback when a element is activated.
 */
typedef void (*listview_mouse_activated_cb)(listview *, gboolean, void *);

/**
 * @param parent The widget's parent.
 * @param name The name of the to be created widget.
 * @param cb The update callback.
 * @param udata The user data to pass to the callback
 * @param eh The height of one element
 * @param reverse Reverse the listview order.
 *
 * @returns a new listview
 */
listview *listview_create(widget *parent, const char *name,
                          listview_update_callback cb, void *udata,
                          unsigned int eh, gboolean reverse);

/**
 * Set the selection changed callback.
 */
void listview_set_selection_changed_callback(
    listview *lv, listview_selection_changed_callback cb, void *udata);

/**
 * @param lv The listview handle
 * @param rows Number of elements
 *
 * Set the maximum number of elements to display.
 */
void listview_set_num_elements(listview *lv, unsigned int rows);

/**
 * @param lv The listview handle
 * @param selected The row index to select
 *
 * Select the row, if selected > the number of rows, it selects the last one.
 */
void listview_set_selected(listview *lv, unsigned int selected);

/**
 * @param lv The listview handle
 *
 * Returns the selected row.
 *
 * @returns the selected row.
 */
unsigned int listview_get_selected(listview *lv);

/**
 * @param lv The listview handle
 *
 * Move the selection next element.
 * - Wrap around.
 */
void listview_nav_next(listview *lv);
/**
 * @param lv The listview handle
 *
 * Move the selection previous element.
 * - Wrap around.
 */
void listview_nav_prev(listview *lv);

/**
 * @param lv The listview handle
 *
 * Move the selection one row up.
 * - Wrap around.
 */
void listview_nav_up(listview *lv);
/**
 * @param lv listview handle.
 *
 * Move the selection one row down.
 * - Wrap around.
 */
void listview_nav_down(listview *lv);
/**
 * @param lv The listview handle
 *
 * Move the selection one column to the right.
 * - No wrap around.
 * - Do not move to top row when at start.
 */
void listview_nav_right(listview *lv);
/**
 * @param lv The listview handle
 *
 * Move the selection one column to the left.
 * - No wrap around.
 */
void listview_nav_left(listview *lv);
/**
 * @param lv The listview handle
 *
 * Move the selection one page down.
 * - No wrap around.
 * - Clip at top/bottom
 */
void listview_nav_page_next(listview *lv);

/**
 * @param lv The listview handle
 *
 * Move the selection one page up.
 * - No wrap around.
 * - Clip at top/bottom
 */
void listview_nav_page_prev(listview *lv);

/**
 * @param lv Handler to the listview object
 * @param enabled enable
 *
 * Hide the scrollbar.
 */
void listview_set_show_scrollbar(listview *lv, gboolean enabled);
/**
 * @param lv Handler to the listview object
 * @param width Width in pixels
 *
 * Set the width of the scrollbar
 */
void listview_set_scrollbar_width(listview *lv, unsigned int width);

/**
 * @param lv Handler to the listview object
 * @param cycle True for cycle mode
 *
 * Set cycle mode. On last entry go to first.
 */
void listview_set_cycle(listview *lv, gboolean cycle);
/**
 * @param lv Handler to the listview object
 * @param type ScrollType
 *
 * Set the scroll type ScrollType::LISTVIEW_SCROLL_CONTINIOUS or
 * ScrollType::LISTVIEW_SCROLL_PER_PAGE
 */
void listview_set_scroll_type(listview *lv, ScrollType type);

/**
 * @param lv Handler to the listview object
 * @param cb The callback
 * @param udata User data
 *
 * Set the mouse activated callback.
 */
void listview_set_mouse_activated_cb(listview *lv,
                                     listview_mouse_activated_cb cb,
                                     void *udata);
/**
 * @param lv Handler to the listview object.
 * @param num_lines the maximum number of lines to display.
 *
 * Set the maximum number of lines to display.
 */
void listview_set_num_lines(listview *lv, unsigned int num_lines);

/**
 * @param lv Handler to the listview object.
 *
 * Get the fixed-height property.
 *
 * @returns get fixed-height.
 */
gboolean listview_get_fixed_num_lines(listview *lv);

/**
 * @param lv Handler to the listview object.
 *
 * Set fixed num lines mode.
 */
void listview_set_fixed_num_lines(listview *lv);

/**
 * @param lv Handler to the listview object.
 * @param max_lines the maximum number of lines to display.
 *
 * Set the maximum number of lines to display.
 */
void listview_set_max_lines(listview *lv, unsigned int max_lines);

/**
 * @param lv Handler to the listview object.
 *
 * Set ellipsize mode.
 */
void listview_toggle_ellipsizing(listview *lv);

/**
 * @param lv Handler to the listview object.
 *
 * Set ellipsize mode to start.
 */

void listview_set_ellipsize_start(listview *lv);

/**
 * @param lv Handler to the listview object.
 * @param filtered boolean indicating if list is filtered.
 *
 */
void listview_set_filtered(listview *lv, gboolean filtered);
/** @} */

#endif // ROFI_LISTVIEW_H
