#ifndef ROFI_VIEW_INTERNAL_H
#define ROFI_VIEW_INTERNAL_H
#include "widget.h"
#include "textbox.h"
#include "scrollbar.h"
#include "keyb.h"
#include "x11-helper.h"

/**
 * @ingroup ViewHandle
 *
 * @{
 */
// State of the menu.

struct RofiViewState
{
    Mode             *sw;
    unsigned int     menu_lines;
    unsigned int     max_elements;
    unsigned int     max_rows;
    unsigned int     columns;

    unsigned int     element_width;
    int              top_offset;

    // Update/Refilter list.
    int              update;
    int              refilter;
    int              rchanged;
    unsigned int     cur_page;

    // Entries
    textbox          *text;
    textbox          *prompt_tb;
    textbox          *message_tb;
    textbox          *case_indicator;
    textbox          **boxes;
    scrollbar        *scrollbar;
    // Small overlay.
    textbox          *overlay;
    int              *distance;
    unsigned int     *line_map;

    unsigned int     num_lines;

    // Selected element.
    unsigned int     selected;
    unsigned int     filtered_lines;
    // Last offset in paginating.
    unsigned int     last_offset;

    KeyBindingAction prev_action;
    xcb_timestamp_t  last_button_press;

    int              quit;
    int              skip_absorb;
    // Return state
    unsigned int     selected_line;
    MenuReturn       retv;
    int              line_height;
    unsigned int     border;
    workarea         mon;

    // Sidebar view
    unsigned int     num_modi;
    textbox          **modi;

    MenuFlags        menu_flags;
    // Handlers.
    void             ( *x11_event_loop )( struct RofiViewState *state, xcb_generic_event_t *ev, xkb_stuff *xkb );
    void             ( *finalize )( struct RofiViewState *state );
};
/** @} */
#endif
