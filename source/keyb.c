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

#include <glib.h>
#include "nkutils-bindings.h"
#include "rofi.h"
#include "xrmoptions.h"
#include <string.h>

typedef struct {
  guint id;
  guint scope;
  char *name;
  char *binding;
  char *comment;
} ActionBindingEntry;

/**
 * Data structure holding all the action keybinding.
 */
ActionBindingEntry rofi_bindings[] = {
    {.id = PASTE_PRIMARY,
     .name = "kb-primary-paste",
     .binding = "Control+V,Shift+Insert",
     .comment = "Paste primary selection"},
    {.id = PASTE_SECONDARY,
     .name = "kb-secondary-paste",
     .binding = "Control+v,Insert",
     .comment = "Paste clipboard"},
    {.id = CLEAR_LINE,
     .name = "kb-clear-line",
     .binding = "Control+w",
     .comment = "Clear input line"},
    {.id = MOVE_FRONT,
     .name = "kb-move-front",
     .binding = "Control+a",
     .comment = "Beginning of line"},
    {.id = MOVE_END,
     .name = "kb-move-end",
     .binding = "Control+e",
     .comment = "End of line"},
    {.id = MOVE_WORD_BACK,
     .name = "kb-move-word-back",
     .binding = "Alt+b,Control+Left",
     .comment = "Move back one word"},
    {.id = MOVE_WORD_FORWARD,
     .name = "kb-move-word-forward",
     .binding = "Alt+f,Control+Right",
     .comment = "Move forward one word"},
    {.id = MOVE_CHAR_BACK,
     .name = "kb-move-char-back",
     .binding = "Left,Control+b",
     .comment = "Move back one char"},
    {.id = MOVE_CHAR_FORWARD,
     .name = "kb-move-char-forward",
     .binding = "Right,Control+f",
     .comment = "Move forward one char"},
    {.id = REMOVE_WORD_BACK,
     .name = "kb-remove-word-back",
     .binding = "Control+Alt+h,Control+BackSpace",
     .comment = "Delete previous word"},
    {.id = REMOVE_WORD_FORWARD,
     .name = "kb-remove-word-forward",
     .binding = "Control+Alt+d",
     .comment = "Delete next word"},
    {.id = REMOVE_CHAR_FORWARD,
     .name = "kb-remove-char-forward",
     .binding = "Delete,Control+d",
     .comment = "Delete next char"},
    {.id = REMOVE_CHAR_BACK,
     .name = "kb-remove-char-back",
     .binding = "BackSpace,Shift+BackSpace,Control+h",
     .comment = "Delete previous char"},
    {.id = REMOVE_TO_EOL,
     .name = "kb-remove-to-eol",
     .binding = "Control+k",
     .comment = "Delete till the end of line"},
    {.id = REMOVE_TO_SOL,
     .name = "kb-remove-to-sol",
     .binding = "Control+u",
     .comment = "Delete till the start of line"},
    {.id = ACCEPT_ENTRY,
     .name = "kb-accept-entry",
     .binding = "Control+j,Control+m,Return,KP_Enter",
     .comment = "Accept entry"},
    {.id = ACCEPT_CUSTOM,
     .name = "kb-accept-custom",
     .binding = "Control+Return",
     .comment = "Use entered text as command (in ssh/run modes)"},
    {.id = ACCEPT_CUSTOM_ALT,
     .name = "kb-accept-custom-alt",
     .binding = "Control+Shift+Return",
     .comment = "Use entered text as command (in ssh/run modes)"},
    {.id = ACCEPT_ALT,
     .name = "kb-accept-alt",
     .binding = "Shift+Return",
     .comment = "Use alternate accept command."},
    {.id = DELETE_ENTRY,
     .name = "kb-delete-entry",
     .binding = "Shift+Delete",
     .comment = "Delete entry from history"},
    {.id = MODE_NEXT,
     .name = "kb-mode-next",
     .binding = "Shift+Right,Control+Tab",
     .comment = "Switch to the next mode."},
    {.id = MODE_PREVIOUS,
     .name = "kb-mode-previous",
     .binding = "Shift+Left,Control+ISO_Left_Tab",
     .comment = "Switch to the previous mode."},
    {.id = MODE_COMPLETE,
     .name = "kb-mode-complete",
     .binding = "Control+l",
     .comment = "Start completion for mode."},
    {.id = ROW_LEFT,
     .name = "kb-row-left",
     .binding = "Control+Page_Up",
     .comment = "Go to the previous column"},
    {.id = ROW_RIGHT,
     .name = "kb-row-right",
     .binding = "Control+Page_Down",
     .comment = "Go to the next column"},
    {.id = ROW_UP,
     .name = "kb-row-up",
     .binding = "Up,Control+p",
     .comment = "Select previous entry"},
    {.id = ROW_DOWN,
     .name = "kb-row-down",
     .binding = "Down,Control+n",
     .comment = "Select next entry"},
    {.id = ROW_TAB,
     .name = "kb-row-tab",
     .binding = "",
     .comment =
         "Go to next row, if one left, accept it, if no left next mode."},
    {.id = ELEMENT_NEXT,
     .name = "kb-element-next",
     .binding = "Tab",
     .comment = "Go to next element (in logical order)."},
    {.id = ELEMENT_PREV,
     .name = "kb-element-prev",
     .binding = "ISO_Left_Tab",
     .comment = "Go to next previous element (in logical order)."},
    {.id = PAGE_PREV,
     .name = "kb-page-prev",
     .binding = "Page_Up",
     .comment = "Go to the previous page"},
    {.id = PAGE_NEXT,
     .name = "kb-page-next",
     .binding = "Page_Down",
     .comment = "Go to the next page"},
    {.id = ROW_FIRST,
     .name = "kb-row-first",
     .binding = "Home,KP_Home",
     .comment = "Go to the first entry"},
    {.id = ROW_LAST,
     .name = "kb-row-last",
     .binding = "End,KP_End",
     .comment = "Go to the last entry"},
    {.id = ROW_SELECT,
     .name = "kb-row-select",
     .binding = "Control+space",
     .comment = "Set selected item as input text"},
    {.id = SCREENSHOT,
     .name = "kb-screenshot",
     .binding = "Alt+S",
     .comment = "Take a screenshot of the rofi window"},
    {.id = CHANGE_ELLIPSIZE,
     .name = "kb-ellipsize",
     .binding = "Alt+period",
     .comment = "Toggle between ellipsize modes for displayed data"},
    {.id = TOGGLE_CASE_SENSITIVITY,
     .name = "kb-toggle-case-sensitivity",
     .binding = "grave,dead_grave",
     .comment = "Toggle case sensitivity"},
    {.id = TOGGLE_SORT,
     .name = "kb-toggle-sort",
     .binding = "Alt+grave",
     .comment = "Toggle sort"},
    {.id = CANCEL,
     .name = "kb-cancel",
     .binding = "Escape,Control+g,Control+bracketleft",
     .comment = "Quit rofi"},
    {.id = CUSTOM_1,
     .name = "kb-custom-1",
     .binding = "Alt+1",
     .comment = "Custom keybinding 1"},
    {.id = CUSTOM_2,
     .name = "kb-custom-2",
     .binding = "Alt+2",
     .comment = "Custom keybinding 2"},
    {.id = CUSTOM_3,
     .name = "kb-custom-3",
     .binding = "Alt+3",
     .comment = "Custom keybinding 3"},
    {.id = CUSTOM_4,
     .name = "kb-custom-4",
     .binding = "Alt+4",
     .comment = "Custom keybinding 4"},
    {.id = CUSTOM_5,
     .name = "kb-custom-5",
     .binding = "Alt+5",
     .comment = "Custom Keybinding 5"},
    {.id = CUSTOM_6,
     .name = "kb-custom-6",
     .binding = "Alt+6",
     .comment = "Custom keybinding 6"},
    {.id = CUSTOM_7,
     .name = "kb-custom-7",
     .binding = "Alt+7",
     .comment = "Custom Keybinding 7"},
    {.id = CUSTOM_8,
     .name = "kb-custom-8",
     .binding = "Alt+8",
     .comment = "Custom keybinding 8"},
    {.id = CUSTOM_9,
     .name = "kb-custom-9",
     .binding = "Alt+9",
     .comment = "Custom keybinding 9"},
    {.id = CUSTOM_10,
     .name = "kb-custom-10",
     .binding = "Alt+0",
     .comment = "Custom keybinding 10"},
    {.id = CUSTOM_11,
     .name = "kb-custom-11",
     .binding = "Alt+exclam",
     .comment = "Custom keybinding 11"},
    {.id = CUSTOM_12,
     .name = "kb-custom-12",
     .binding = "Alt+at",
     .comment = "Custom keybinding 12"},
    {.id = CUSTOM_13,
     .name = "kb-custom-13",
     .binding = "Alt+numbersign",
     .comment = "Custom keybinding 13"},
    {.id = CUSTOM_14,
     .name = "kb-custom-14",
     .binding = "Alt+dollar",
     .comment = "Custom keybinding 14"},
    {.id = CUSTOM_15,
     .name = "kb-custom-15",
     .binding = "Alt+percent",
     .comment = "Custom keybinding 15"},
    {.id = CUSTOM_16,
     .name = "kb-custom-16",
     .binding = "Alt+dead_circumflex",
     .comment = "Custom keybinding 16"},
    {.id = CUSTOM_17,
     .name = "kb-custom-17",
     .binding = "Alt+ampersand",
     .comment = "Custom keybinding 17"},
    {.id = CUSTOM_18,
     .name = "kb-custom-18",
     .binding = "Alt+asterisk",
     .comment = "Custom keybinding 18"},
    {.id = CUSTOM_19,
     .name = "kb-custom-19",
     .binding = "Alt+parenleft",
     .comment = "Custom Keybinding 19"},
    {.id = SELECT_ELEMENT_1,
     .name = "kb-select-1",
     .binding = "Super+1",
     .comment = "Select row 1"},
    {.id = SELECT_ELEMENT_2,
     .name = "kb-select-2",
     .binding = "Super+2",
     .comment = "Select row 2"},
    {.id = SELECT_ELEMENT_3,
     .name = "kb-select-3",
     .binding = "Super+3",
     .comment = "Select row 3"},
    {.id = SELECT_ELEMENT_4,
     .name = "kb-select-4",
     .binding = "Super+4",
     .comment = "Select row 4"},
    {.id = SELECT_ELEMENT_5,
     .name = "kb-select-5",
     .binding = "Super+5",
     .comment = "Select row 5"},
    {.id = SELECT_ELEMENT_6,
     .name = "kb-select-6",
     .binding = "Super+6",
     .comment = "Select row 6"},
    {.id = SELECT_ELEMENT_7,
     .name = "kb-select-7",
     .binding = "Super+7",
     .comment = "Select row 7"},
    {.id = SELECT_ELEMENT_8,
     .name = "kb-select-8",
     .binding = "Super+8",
     .comment = "Select row 8"},
    {.id = SELECT_ELEMENT_9,
     .name = "kb-select-9",
     .binding = "Super+9",
     .comment = "Select row 9"},
    {.id = SELECT_ELEMENT_10,
     .name = "kb-select-10",
     .binding = "Super+0",
     .comment = "Select row 10"},

    /* Mouse-aware bindings */

    {.id = SCROLL_LEFT,
     .scope = SCOPE_MOUSE_LISTVIEW,
     .name = "ml-row-left",
     .binding = "ScrollLeft",
     .comment = "Go to the previous column"},
    {.id = SCROLL_RIGHT,
     .scope = SCOPE_MOUSE_LISTVIEW,
     .name = "ml-row-right",
     .binding = "ScrollRight",
     .comment = "Go to the next column"},
    {.id = SCROLL_UP,
     .scope = SCOPE_MOUSE_LISTVIEW,
     .name = "ml-row-up",
     .binding = "ScrollUp",
     .comment = "Select previous entry"},
    {.id = SCROLL_DOWN,
     .scope = SCOPE_MOUSE_LISTVIEW,
     .name = "ml-row-down",
     .binding = "ScrollDown",
     .comment = "Select next entry"},

    {.id = SELECT_HOVERED_ENTRY,
     .scope = SCOPE_MOUSE_LISTVIEW_ELEMENT,
     .name = "me-select-entry",
     .binding = "MousePrimary",
     .comment = "Select hovered row"},
    {.id = ACCEPT_HOVERED_ENTRY,
     .scope = SCOPE_MOUSE_LISTVIEW_ELEMENT,
     .name = "me-accept-entry",
     .binding = "MouseDPrimary",
     .comment = "Accept hovered row"},
    {.id = ACCEPT_HOVERED_CUSTOM,
     .scope = SCOPE_MOUSE_LISTVIEW_ELEMENT,
     .name = "me-accept-custom",
     .binding = "Control+MouseDPrimary",
     .comment = "Accept hovered row with custom action"},
};

/** Default binding of mouse button to action. */
static const gchar *mouse_default_bindings[] = {
    [MOUSE_CLICK_DOWN] = "MousePrimary",
    [MOUSE_CLICK_UP] = "!MousePrimary",
    [MOUSE_DCLICK_DOWN] = "MouseDPrimary",
    [MOUSE_DCLICK_UP] = "!MouseDPrimary",
};

void setup_abe(void) {
  for (gsize i = 0; i < G_N_ELEMENTS(rofi_bindings); ++i) {
    ActionBindingEntry *b = &rofi_bindings[i];
    b->binding = g_strdup(b->binding);
    config_parser_add_option(xrm_String, b->name, (void **)&(b->binding),
                             b->comment);
  }
}

static gboolean binding_check_action(guint64 scope,
                                     G_GNUC_UNUSED gpointer target,
                                     gpointer user_data) {
  return rofi_view_check_action(rofi_view_get_active(), scope,
                                GPOINTER_TO_UINT(user_data))
             ? NK_BINDINGS_BINDING_TRIGGERED
             : NK_BINDINGS_BINDING_NOT_TRIGGERED;
}

static void binding_trigger_action(guint64 scope, G_GNUC_UNUSED gpointer target,
                                   gpointer user_data) {
  rofi_view_trigger_action(rofi_view_get_active(), scope,
                           GPOINTER_TO_UINT(user_data));
}

guint key_binding_get_action_from_name(const char *name) {
  for (gsize i = 0; i < G_N_ELEMENTS(rofi_bindings); ++i) {
    ActionBindingEntry *b = &rofi_bindings[i];
    if (g_strcmp0(b->name, name) == 0) {
      return b->id;
    }
  }
  return UINT32_MAX;
}

gboolean parse_keys_abe(NkBindings *bindings) {
  GError *error = NULL;
  GString *error_msg = g_string_new("");
  for (gsize i = 0; i < G_N_ELEMENTS(rofi_bindings); ++i) {
    ActionBindingEntry *b = &rofi_bindings[i];
    char *keystr = g_strdup(b->binding);
    char *sp = NULL;

    // Iter over bindings.
    const char *const sep = ",";
    for (char *entry = strtok_r(keystr, sep, &sp); entry != NULL;
         entry = strtok_r(NULL, sep, &sp)) {
      if (!nk_bindings_add_binding(bindings, b->scope, entry,
                                   binding_check_action, binding_trigger_action,
                                   GUINT_TO_POINTER(b->id), NULL, &error)) {
        char *str = g_markup_printf_escaped(
            "Failed to set binding <i>%s</i> for: <i>%s (%s)</i>:\n\t<span "
            "size=\"smaller\" style=\"italic\">%s</span>\n",
            b->binding, b->comment, b->name, error->message);
        g_string_append(error_msg, str);
        g_free(str);
        g_clear_error(&error);
      }
    }

    g_free(keystr);
  }
  if (error_msg->len > 0) {
    // rofi_view_error_dialog ( error_msg->str, TRUE );
    rofi_add_error_message(error_msg);
    //        g_string_free ( error_msg, TRUE );
    return FALSE;
  }

  for (gsize i = SCOPE_MIN_FIXED; i <= SCOPE_MAX_FIXED; ++i) {
    for (gsize j = 1; j < G_N_ELEMENTS(mouse_default_bindings); ++j) {
      nk_bindings_add_binding(bindings, i, mouse_default_bindings[j],
                              binding_check_action, binding_trigger_action,
                              GSIZE_TO_POINTER(j), NULL, NULL);
    }
  }

  g_string_free(error_msg, TRUE);
  return TRUE;
}
