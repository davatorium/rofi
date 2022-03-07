/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2022 Qball Cow <qball@gmpclient.org>
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

#ifndef ROFI_KEYB_H
#define ROFI_KEYB_H

#include <glib.h>
#include <nkutils-bindings.h>

/**
 * @defgroup KEYB KeyboardBindings
 *
 * @{
 */

/**
 * List of all scopes the mouse can interact on.
 */
typedef enum {
  SCOPE_GLOBAL,
  SCOPE_MOUSE_LISTVIEW,
  SCOPE_MOUSE_LISTVIEW_ELEMENT,

#define SCOPE_MIN_FIXED SCOPE_MOUSE_EDITBOX
  SCOPE_MOUSE_EDITBOX,
  SCOPE_MOUSE_SCROLLBAR,
  SCOPE_MOUSE_MODE_SWITCHER,
#define SCOPE_MAX_FIXED SCOPE_MOUSE_MODE_SWITCHER
} BindingsScope;

/**
 * List of all possible actions that can be triggered by a keybinding.
 */
typedef enum {
  /** Paste from primary clipboard */
  PASTE_PRIMARY = 1,
  /** Paste from secondary clipboard */
  PASTE_SECONDARY,
  /** Clear the entry box. */
  CLEAR_LINE,
  /** Move to front of text */
  MOVE_FRONT,
  /** Move to end of text */
  MOVE_END,
  /** Move on word back */
  MOVE_WORD_BACK,
  /** Move on word forward */
  MOVE_WORD_FORWARD,
  /** Move character back */
  MOVE_CHAR_BACK,
  /** Move character forward */
  MOVE_CHAR_FORWARD,
  /** Remove previous word */
  REMOVE_WORD_BACK,
  /** Remove next work */
  REMOVE_WORD_FORWARD,
  /** Remove next character */
  REMOVE_CHAR_FORWARD,
  /** Remove previous character */
  REMOVE_CHAR_BACK,
  /** Remove till EOL */
  REMOVE_TO_EOL,
  /** Remove till SOL */
  REMOVE_TO_SOL,
  /** Accept the current selected entry */
  ACCEPT_ENTRY,
  ACCEPT_ALT,
  ACCEPT_CUSTOM,
  ACCEPT_CUSTOM_ALT,
  MODE_NEXT,
  MODE_COMPLETE,
  MODE_PREVIOUS,
  TOGGLE_CASE_SENSITIVITY,
  DELETE_ENTRY,
  ROW_LEFT,
  ROW_RIGHT,
  ROW_UP,
  ROW_DOWN,
  ROW_TAB,
  ELEMENT_NEXT,
  ELEMENT_PREV,
  PAGE_PREV,
  PAGE_NEXT,
  ROW_FIRST,
  ROW_LAST,
  ROW_SELECT,
  CANCEL,
  CUSTOM_1,
  CUSTOM_2,
  CUSTOM_3,
  CUSTOM_4,
  CUSTOM_5,
  CUSTOM_6,
  CUSTOM_7,
  CUSTOM_8,
  CUSTOM_9,
  CUSTOM_10,
  CUSTOM_11,
  CUSTOM_12,
  CUSTOM_13,
  CUSTOM_14,
  CUSTOM_15,
  CUSTOM_16,
  CUSTOM_17,
  CUSTOM_18,
  CUSTOM_19,
  SCREENSHOT,
  CHANGE_ELLIPSIZE,
  TOGGLE_SORT,
  SELECT_ELEMENT_1,
  SELECT_ELEMENT_2,
  SELECT_ELEMENT_3,
  SELECT_ELEMENT_4,
  SELECT_ELEMENT_5,
  SELECT_ELEMENT_6,
  SELECT_ELEMENT_7,
  SELECT_ELEMENT_8,
  SELECT_ELEMENT_9,
  SELECT_ELEMENT_10,
} KeyBindingAction;

/**
 * Actions mouse can take on the ListView.
 */
typedef enum {
  SCROLL_LEFT = 1,
  SCROLL_RIGHT,
  SCROLL_DOWN,
  SCROLL_UP,
} MouseBindingListviewAction;

/**
 * Actions mouse can take on the ListView element.
 */
typedef enum {
  SELECT_HOVERED_ENTRY = 1,
  ACCEPT_HOVERED_ENTRY,
  ACCEPT_HOVERED_CUSTOM,
} MouseBindingListviewElementAction;

/**
 * Default mouse actions.
 */
typedef enum {
  MOUSE_CLICK_DOWN = 1,
  MOUSE_CLICK_UP,
  MOUSE_DCLICK_DOWN,
  MOUSE_DCLICK_UP,
} MouseBindingMouseDefaultAction;

/**
 * Parse the keybindings.
 * This should be called after the setting system is initialized.
 */
gboolean parse_keys_abe(NkBindings *bindings);

/**
 * Setup the keybindings
 * This adds all the entries to the settings system.
 */
void setup_abe(void);

/**
 * @param name Don't have the name.
 *
 * @returns id, or UINT32_MAX if not found.
 */
guint key_binding_get_action_from_name(const char *name);
/**@}*/
#endif // ROFI_KEYB_H
