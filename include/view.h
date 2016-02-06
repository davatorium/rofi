
// State of the menu.

typedef struct RofiViewState
{
    Mode         *sw;
    unsigned int menu_lines;
    unsigned int max_elements;
    unsigned int max_rows;
    unsigned int columns;

    // window width,height
    unsigned int w, h;
    int          x, y;
    unsigned int element_width;
    int          top_offset;

    // Update/Refilter list.
    int          update;
    int          refilter;
    int          rchanged;
    int          cur_page;

    // Entries
    textbox      *text;
    textbox      *prompt_tb;
    textbox      *message_tb;
    textbox      *case_indicator;
    textbox      **boxes;
    scrollbar    *scrollbar;
    int          *distance;
    unsigned int *line_map;

    unsigned int num_lines;

    // Selected element.
    unsigned int selected;
    unsigned int filtered_lines;
    // Last offset in paginating.
    unsigned int last_offset;

    KeySym       prev_key;
    Time         last_button_press;

    int          quit;
    int          skip_absorb;
    // Return state
    unsigned int selected_line;
    MenuReturn   retv;
    int          *lines_not_ascii;
    int          line_height;
    unsigned int border;
    workarea     mon;

    ssize_t      num_modi;
    textbox      **modi;
    // Handlers.
    void ( *x11_event_loop )( struct RofiViewState *state, XEvent *ev );
    void ( *finalize )( struct RofiViewState *state );
}RofiViewState;

/**
 * @param state The Menu Handle
 *
 * Check if a finalize function is set, and if sets executes it.
 */
void rofi_view_finalize ( RofiViewState *state );

RofiViewState * rofi_view_state_create ( void );
