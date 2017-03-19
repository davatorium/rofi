#ifndef ROFI_VIEW_INTERNAL_H
#define ROFI_VIEW_INTERNAL_H
#include "widgets/container.h"
#include "widgets/widget.h"
#include "widgets/textbox.h"
#include "widgets/listview.h"
#include "widgets/box.h"
#include "keyb.h"
#include "x11-helper.h"
#include "theme.h"

/**
 * @ingroup ViewHandle
 *
 * @{
 */
// State of the menu.

struct RofiViewState
{
    /** #Mode bound to to this view. */
    Mode             *sw;

    /** Flag indicating if view needs to be refiltered. */
    int              refilter;
    /** Widget representing the main container. */
    container        *main_window;
    /** Main #box widget holding different elements. */
    box              *main_box;
    /** #box widget packing the input bar widgets. */
    box              *input_bar;
    /** #textbox showing the prompt in the input bar. */
    textbox          *prompt;
    /** #textbox with the user input in the input bar. */
    textbox          *text;
    /** #textbox showing the state of the case sensitive and sortng. */
    textbox          *case_indicator;

    /** #listview holding the displayed elements. */
    listview         *list_view;
    /** #textbox widget showing the overlay. */
    textbox          *overlay;
    /** #container holding the message box */
    container        *mesg_box;
    /** #textbox containing the message entry */
    textbox          *mesg_tb;

    /** Array with the levenshtein distance for each eleemnt. */
    int              *distance;
    /** Array with the translation between the filtered and unfiltered list. */
    unsigned int     *line_map;
    /** number of (unfiltered) elements to show. */
    unsigned int     num_lines;

    /** number of (filtered) elements to show. */
    unsigned int     filtered_lines;

    /** Previously called key action. */
    KeyBindingAction prev_action;
    /** Time previous key action was executed. */
    xcb_timestamp_t  last_button_press;

    /** Indicate view should terminate */
    int              quit;
    /** Indicate if we should absorb the key release */
    int              skip_absorb;
    /** The selected line (in the unfiltered list) */
    unsigned int     selected_line;
    /** The return state of the view */
    MenuReturn       retv;

    /** #box holding the different modi buttons */
    box              *sidebar_bar;
    /** number of modi to display */
    unsigned int     num_modi;
    /** Array of #textbox that act as buttons for switching modi */
    textbox          **modi;
    /** Settings of the menu */
    MenuFlags        menu_flags;
    /** If mouse was within view previously */
    int              mouse_seen;
    /** Flag indicating if view needs to be reloaded. */
    int              reload;
    /** The funciton to be called when finalizing this view */
    void             ( *finalize )( struct RofiViewState *state );

    /** Width of the view */
    int              width;
    /** Height of the view */
    int              height;
    /** X position of the view */
    int              x;
    /** Y position of the view */
    int              y;

    /** Regexs used for matching */
    GRegex           **tokens;
};
/** @} */
#endif
