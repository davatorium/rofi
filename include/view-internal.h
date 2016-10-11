#ifndef ROFI_VIEW_INTERNAL_H
#define ROFI_VIEW_INTERNAL_H
#include "widgets/widget.h"
#include "widgets/textbox.h"
#include "widgets/separator.h"
#include "widgets/listview.h"
#include "widgets/box.h"
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

    // Update/Refilter list.
    int              refilter;
    int              rchanged;

    box              *main_box;
    // Entries
    box              *input_bar;
    separator        *input_bar_separator;

    textbox          *prompt;
    textbox          *text;
    textbox          *case_indicator;

    listview         *list_view;
    // Small overlay.
    textbox          *overlay;
    int              *distance;
    unsigned int     *line_map;

    unsigned int     num_lines;

    // Selected element.
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
    unsigned int     border;
    workarea         mon;

    // Sidebar view
    box              *sidebar_bar;
    unsigned int     num_modi;
    textbox          **modi;

    MenuFlags        menu_flags;
    int              mouse_seen;

    int              reload;
    // Handlers.
    void             ( *x11_event_loop )( struct RofiViewState *state, xcb_generic_event_t *ev, xkb_stuff *xkb );
    void             ( *finalize )( struct RofiViewState *state );

    int              width;
    int              height;
    int              x;
    int              y;

    GRegex           **tokens;
};
/** @} */
#endif
