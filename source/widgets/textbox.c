/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2012 Sean Pringle <sean.pringle@gmail.com>
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

#include <config.h>
#include <xcb/xcb.h>
#include <ctype.h>
#include <string.h>
#include <glib.h>
#include <math.h>
#include "widgets/textbox.h"
#include "keyb.h"
#include "helper.h"
#include "helper-theme.h"
#include "mode.h"
#include "view.h"

#include "theme.h"

/** The space reserved for the DOT when enabling multi-select. */
#define DOT_OFFSET    15

static void textbox_draw ( widget *, cairo_t * );
static void textbox_free ( widget * );
static int textbox_get_width ( widget * );
static int _textbox_get_height ( widget * );
static void __textbox_update_pango_text ( textbox *tb );

/** Default pango context */
static PangoContext     *p_context = NULL;
/** The pango font metrics */
static PangoFontMetrics *p_metrics = NULL;

/** Cache to hold font descriptions. This it to avoid having to lookup each time. */
typedef struct TBFontConfig
{
    /** Font description */
    PangoFontDescription *pfd;
    /** Font metrics */
    PangoFontMetrics     *metrics;
    /** height */
    double               height;
}TBFontConfig;

/** HashMap of previously parsed font descriptions. */
static GHashTable *tbfc_cache = NULL;

static gboolean textbox_blink ( gpointer data )
{
    textbox *tb = (textbox *) data;
    if ( tb->blink < 2 ) {
        tb->blink = !tb->blink;
        widget_queue_redraw ( WIDGET ( tb ) );
        rofi_view_queue_redraw ( );
    }
    else {
        tb->blink--;
    }
    return TRUE;
}

static void textbox_resize ( widget *wid, short w, short h )
{
    textbox *tb = (textbox *) wid;
    textbox_moveresize ( tb, tb->widget.x, tb->widget.y, w, h );
}
static int textbox_get_desired_height ( widget *wid )
{
    textbox *tb = (textbox *) wid;
    if ( ( tb->flags & TB_AUTOHEIGHT ) == 0 ) {
        return tb->widget.h;
    }
    if ( tb->changed ) {
        __textbox_update_pango_text ( tb );
    }
    int height = textbox_get_height ( tb );
    return height;
}

static WidgetTriggerActionResult textbox_editable_trigger_action ( widget *wid, MouseBindingMouseDefaultAction action, gint x, gint y, G_GNUC_UNUSED void *user_data )
{
    textbox *tb = (textbox *) wid;
    switch ( action )
    {
    case MOUSE_CLICK_DOWN:
    {
        gint i;
        // subtract padding on left.
        x -= widget_padding_get_left ( wid );
        gint max = textbox_get_font_width ( tb );
        // Right of text, move to end.
        if ( x >= max ) {
            textbox_cursor_end ( tb );
        }
        else if ( x > 0 ) {
            // If in range, get index.
            pango_layout_xy_to_index ( tb->layout, x * PANGO_SCALE, y * PANGO_SCALE, &i, NULL );
            textbox_cursor ( tb, i );
        }
        return WIDGET_TRIGGER_ACTION_RESULT_HANDLED;
    }
    case MOUSE_CLICK_UP:
    case MOUSE_DCLICK_DOWN:
    case MOUSE_DCLICK_UP:
        break;
    }
    return WIDGET_TRIGGER_ACTION_RESULT_IGNORED;
}

static void textbox_initialize_font ( textbox *tb )
{
    tb->metrics = p_metrics;
    const char * font = rofi_theme_get_string ( WIDGET ( tb ), "font", NULL );
    if ( font ) {
        TBFontConfig *tbfc = g_hash_table_lookup ( tbfc_cache, font );
        if ( tbfc == NULL ) {
            tbfc      = g_malloc0 ( sizeof ( TBFontConfig ) );
            tbfc->pfd = pango_font_description_from_string ( font );
            if ( helper_validate_font ( tbfc->pfd, font ) ) {
                tbfc->metrics = pango_context_get_metrics ( p_context, tbfc->pfd, NULL );
                tbfc->height  = pango_font_metrics_get_ascent ( tbfc->metrics ) + pango_font_metrics_get_descent ( tbfc->metrics );

                // Cast away consts. (*yuck*) because table_insert does not know it is const.
                g_hash_table_insert ( tbfc_cache, (char *) font, tbfc );
            }
            else {
                pango_font_description_free ( tbfc->pfd );
                g_free ( tbfc );
                tbfc = NULL;
            }
        }
        if ( tbfc ) {
            // Update for used font.
            pango_layout_set_font_description ( tb->layout, tbfc->pfd );
            tb->metrics     = tbfc->metrics;
        }
    }
}

textbox* textbox_create ( widget *parent, WidgetType type, const char *name, TextboxFlags flags, TextBoxFontType tbft, const char *text, double xalign, double yalign )
{
    textbox *tb = g_slice_new0 ( textbox );

    widget_init ( WIDGET ( tb ), parent, type, name );

    tb->widget.draw               = textbox_draw;
    tb->widget.free               = textbox_free;
    tb->widget.resize             = textbox_resize;
    tb->widget.get_width          = textbox_get_width;
    tb->widget.get_height         = _textbox_get_height;
    tb->widget.get_desired_height = textbox_get_desired_height;
    tb->widget.get_desired_width  = textbox_get_desired_width;
    tb->flags                     = flags;
    tb->emode                     = PANGO_ELLIPSIZE_END;

    tb->changed = FALSE;

    tb->layout = pango_layout_new ( p_context );
    textbox_font ( tb, tbft );

    textbox_initialize_font ( tb );

    if ( ( flags & TB_WRAP ) == TB_WRAP ) {
        pango_layout_set_wrap ( tb->layout, PANGO_WRAP_WORD_CHAR );
    }

    const char *txt = rofi_theme_get_string ( WIDGET  ( tb ), "str", text );
    textbox_text ( tb, txt ? txt : "" );
    textbox_cursor_end ( tb );

    // auto height/width modes get handled here
    textbox_moveresize ( tb, tb->widget.x, tb->widget.y, tb->widget.w, tb->widget.h );

    tb->blink_timeout = 0;
    tb->blink         = 1;
    if ( ( flags & TB_EDITABLE ) == TB_EDITABLE ) {
        tb->blink_timeout         = g_timeout_add ( 1200, textbox_blink, tb );
        tb->widget.trigger_action = textbox_editable_trigger_action;
    }

    tb->yalign = rofi_theme_get_double ( WIDGET ( tb ), "vertical-align", yalign );
    tb->yalign = MAX ( 0, MIN ( 1.0, tb->yalign ) );
    tb->xalign = rofi_theme_get_double ( WIDGET ( tb ), "horizontal-align", xalign );
    tb->xalign = MAX ( 0, MIN ( 1.0, tb->xalign ) );

    return tb;
}

/**
 * State names used for theming.
 */
const char *const theme_prop_names[][3] = {
    /** Normal row */
    { "normal.normal", "selected.normal", "alternate.normal" },
    /** Urgent row */
    { "normal.urgent", "selected.urgent", "alternate.urgent" },
    /** Active row */
    { "normal.active", "selected.active", "alternate.active" },
};

void textbox_font ( textbox *tb, TextBoxFontType tbft )
{
    TextBoxFontType t = tbft & STATE_MASK;
    if ( tb == NULL ) {
        return;
    }
    // ACTIVE has priority over URGENT if both set.
    if ( t == ( URGENT | ACTIVE ) ) {
        t = ACTIVE;
    }
    switch ( ( tbft & FMOD_MASK ) )
    {
    case HIGHLIGHT:
        widget_set_state ( WIDGET ( tb ), theme_prop_names[t][1] );
        break;
    case ALT:
        widget_set_state ( WIDGET ( tb ), theme_prop_names[t][2] );
        break;
    default:
        widget_set_state ( WIDGET ( tb ), theme_prop_names[t][0] );
        break;
    }
    if ( tb->tbft != tbft || tb->widget.state == NULL ) {
        widget_queue_redraw ( WIDGET ( tb ) );
    }
    tb->tbft = tbft;
}

/**
 * @param tb The textbox object.
 *
 * Update the pango layout's text. It does this depending on the
 * textbox flags.
 */
static void __textbox_update_pango_text ( textbox *tb )
{
    pango_layout_set_attributes ( tb->layout, NULL );
    if ( ( tb->flags & TB_PASSWORD ) == TB_PASSWORD ) {
        size_t l = g_utf8_strlen ( tb->text, -1 );
        char   string [l + 1];
        memset ( string, '*', l );
        string[l] = '\0';
        pango_layout_set_text ( tb->layout, string, l );
    }
    else if ( tb->flags & TB_MARKUP || tb->tbft & MARKUP ) {
        pango_layout_set_markup ( tb->layout, tb->text, -1 );
    }
    else {
        pango_layout_set_text ( tb->layout, tb->text, -1 );
    }
}
const char *textbox_get_visible_text ( const textbox *tb )
{
    if ( tb == NULL ) {
        return NULL;
    }
    return pango_layout_get_text ( tb->layout );
}
PangoAttrList *textbox_get_pango_attributes ( textbox *tb )
{
    if ( tb == NULL ) {
        return NULL;
    }
    return pango_layout_get_attributes ( tb->layout );
}
void textbox_set_pango_attributes ( textbox *tb, PangoAttrList *list )
{
    if ( tb == NULL ) {
        return;
    }
    pango_layout_set_attributes ( tb->layout, list );
}

// set the default text to display
void textbox_text ( textbox *tb, const char *text )
{
    if ( tb == NULL ) {
        return;
    }
    g_free ( tb->text );
    const gchar *last_pointer = NULL;

    if ( g_utf8_validate ( text, -1, &last_pointer ) ) {
        tb->text = g_strdup ( text );
    }
    else {
        if ( last_pointer != NULL ) {
            // Copy string up to invalid character.
            tb->text = g_strndup ( text, ( last_pointer - text ) );
        }
        else {
            tb->text = g_strdup ( "Invalid UTF-8 string." );
        }
    }
    __textbox_update_pango_text ( tb );
    if ( tb->flags & TB_AUTOWIDTH ) {
        textbox_moveresize ( tb, tb->widget.x, tb->widget.y, tb->widget.w, tb->widget.h );
        if ( WIDGET ( tb )->parent ) {
            widget_update ( WIDGET ( tb )->parent );
        }
    }

    tb->cursor = MAX ( 0, MIN ( ( int ) g_utf8_strlen ( tb->text, -1 ), tb->cursor ) );
    widget_queue_redraw ( WIDGET ( tb ) );
}


// within the parent handled auto width/height modes
void textbox_moveresize ( textbox *tb, int x, int y, int w, int h )
{
    unsigned int offset = ( ( tb->flags & TB_INDICATOR ) ? DOT_OFFSET : 0 );
    if ( tb->flags & TB_AUTOWIDTH ) {
        pango_layout_set_width ( tb->layout, -1 );
        w = textbox_get_font_width ( tb ) + widget_padding_get_padding_width ( WIDGET ( tb ) ) + offset;
    }
    else {
        // set ellipsize
        if ( ( tb->flags & TB_EDITABLE ) == TB_EDITABLE ) {
            pango_layout_set_ellipsize ( tb->layout, PANGO_ELLIPSIZE_MIDDLE );
        }
        else if ( ( tb->flags & TB_WRAP ) != TB_WRAP ) {
            pango_layout_set_ellipsize ( tb->layout, tb->emode );
        } else {
            pango_layout_set_ellipsize ( tb->layout, PANGO_ELLIPSIZE_NONE );
        }
    }

    if ( tb->flags & TB_AUTOHEIGHT ) {
        // Width determines height!
        int tw = MAX ( 1, w );
        pango_layout_set_width ( tb->layout, PANGO_SCALE * ( tw - widget_padding_get_padding_width ( WIDGET ( tb ) ) - offset ) );
        int hd = textbox_get_height ( tb );
        h = MAX ( hd, h );
    }

    if ( x != tb->widget.x || y != tb->widget.y || w != tb->widget.w || h != tb->widget.h ) {
        tb->widget.x = x;
        tb->widget.y = y;
        tb->widget.h = MAX ( 1, h );
        tb->widget.w = MAX ( 1, w );
    }

    // We always want to update this
    pango_layout_set_width ( tb->layout, PANGO_SCALE * ( tb->widget.w - widget_padding_get_padding_width ( WIDGET ( tb ) ) - offset ) );
    widget_queue_redraw ( WIDGET ( tb ) );
}

// will also unmap the window if still displayed
static void textbox_free ( widget *wid )
{
    if ( wid == NULL ) {
        return;
    }
    textbox *tb = (textbox *) wid;
    if ( tb->blink_timeout > 0 ) {
        g_source_remove ( tb->blink_timeout );
        tb->blink_timeout = 0;
    }
    g_free ( tb->text );

    if ( tb->layout != NULL ) {
        g_object_unref ( tb->layout );
    }

    g_slice_free ( textbox, tb );
}

static void textbox_draw ( widget *wid, cairo_t *draw )
{
    if ( wid == NULL ) {
        return;
    }
    textbox      *tb    = (textbox *) wid;
    unsigned int offset = ( ( tb->flags & TB_INDICATOR ) ? DOT_OFFSET : 0 );

    if ( tb->changed ) {
        __textbox_update_pango_text ( tb );
    }

    // Skip the side MARGIN on the X axis.
    int x = widget_padding_get_left ( WIDGET ( tb ) );
    int top = widget_padding_get_top ( WIDGET ( tb ) );
    int y = ( pango_font_metrics_get_ascent ( tb->metrics ) - pango_layout_get_baseline ( tb->layout ) ) / PANGO_SCALE;
    int line_width = 0, line_height = 0;
    // Get actual width.
    pango_layout_get_pixel_size ( tb->layout, &line_width, &line_height );

    if ( tb->yalign > 0.001 ) {
        int bottom = widget_padding_get_bottom ( WIDGET ( tb ) );
        top = ( tb->widget.h - bottom - line_height - top ) * tb->yalign + top;
    }
    y += top;

    x += offset;

    if ( tb->xalign > 0.001 ) {
        int rem = MAX ( 0, tb->widget.w - widget_padding_get_padding_width ( WIDGET ( tb ) ) - line_width );
        x = tb->xalign * rem + widget_padding_get_left ( WIDGET ( tb ) );
    }
    // TODO check if this is still needed after flatning.
    cairo_set_operator ( draw, CAIRO_OPERATOR_OVER );
    cairo_set_source_rgb ( draw, 0.0, 0.0, 0.0 );
    rofi_theme_get_color ( WIDGET ( tb ), "text-color", draw );
    // draw the cursor
    if ( tb->flags & TB_EDITABLE && tb->blink ) {
        // We want to place the cursor based on the text shown.
        const char     *text = pango_layout_get_text ( tb->layout );
        // Clamp the position, should not be needed, but we are paranoid.
        int            cursor_offset = MIN ( tb->cursor, g_utf8_strlen ( text, -1 ) );
        PangoRectangle pos;
        // convert to byte location.
        char           *offset = g_utf8_offset_to_pointer ( text, cursor_offset );
        pango_layout_get_cursor_pos ( tb->layout, offset - text, &pos, NULL );
        int            cursor_x      = pos.x / PANGO_SCALE;
        int            cursor_y      = pos.y / PANGO_SCALE;
        int            cursor_height = pos.height / PANGO_SCALE;
        int            cursor_width  = 2;
        cairo_rectangle ( draw, x + cursor_x, y + cursor_y, cursor_width, cursor_height );
        cairo_fill ( draw );
    }

    // Set ARGB
    // We need to set over, otherwise subpixel hinting wont work.
    cairo_move_to ( draw, x, top );
    pango_cairo_show_layout ( draw, tb->layout );

    if ( ( tb->flags & TB_INDICATOR ) == TB_INDICATOR && ( tb->tbft & ( SELECTED ) ) ) {
        cairo_arc ( draw, DOT_OFFSET / 2.0, tb->widget.h / 2.0, 2.0, 0, 2.0 * M_PI );
        cairo_fill ( draw );
    }
}

// cursor handling for edit mode
void textbox_cursor ( textbox *tb, int pos )
{
    if ( tb == NULL ) {
        return;
    }
    int length = ( tb->text == NULL ) ? 0 : g_utf8_strlen ( tb->text, -1 );
    tb->cursor = MAX ( 0, MIN ( length, pos ) );
    // Stop blink!
    tb->blink = 3;
    widget_queue_redraw ( WIDGET ( tb ) );
}

/**
 * @param tb Handle to the textbox
 *
 * Move cursor one position forward.
 *
 * @returns if cursor was moved.
 */
static int textbox_cursor_inc ( textbox *tb )
{
    int old = tb->cursor;
    textbox_cursor ( tb, tb->cursor + 1 );
    return old != tb->cursor;
}

/**
 * @param tb Handle to the textbox
 *
 * Move cursor one position backward.
 *
 * @returns if cursor was moved.
 */
static int textbox_cursor_dec ( textbox *tb )
{
    int old = tb->cursor;
    textbox_cursor ( tb, tb->cursor - 1 );
    return old != tb->cursor;
}

// Move word right
static void textbox_cursor_inc_word ( textbox *tb )
{
    if ( tb->text == NULL ) {
        return;
    }
    // Find word boundaries, with pango_Break?
    gchar *c = g_utf8_offset_to_pointer ( tb->text, tb->cursor );
    while ( ( c = g_utf8_next_char ( c ) ) ) {
        gunichar          uc = g_utf8_get_char ( c );
        GUnicodeBreakType bt = g_unichar_break_type ( uc );
        if ( ( bt == G_UNICODE_BREAK_ALPHABETIC || bt == G_UNICODE_BREAK_HEBREW_LETTER ||
               bt == G_UNICODE_BREAK_NUMERIC || bt == G_UNICODE_BREAK_QUOTATION ) ) {
            break;
        }
    }
    if ( c == NULL || *c == '\0' ) {
        return;
    }
    while ( ( c = g_utf8_next_char ( c ) ) ) {
        gunichar          uc = g_utf8_get_char ( c );
        GUnicodeBreakType bt = g_unichar_break_type ( uc );
        if ( !( bt == G_UNICODE_BREAK_ALPHABETIC || bt == G_UNICODE_BREAK_HEBREW_LETTER ||
                bt == G_UNICODE_BREAK_NUMERIC || bt == G_UNICODE_BREAK_QUOTATION ) ) {
            break;
        }
    }
    int index = g_utf8_pointer_to_offset ( tb->text, c );
    textbox_cursor ( tb, index );
}
// move word left
static void textbox_cursor_dec_word ( textbox *tb )
{
    // Find word boundaries, with pango_Break?
    gchar *n;
    gchar *c = g_utf8_offset_to_pointer ( tb->text, tb->cursor );
    while ( ( c = g_utf8_prev_char ( c ) ) && c != tb->text ) {
        gunichar          uc = g_utf8_get_char ( c );
        GUnicodeBreakType bt = g_unichar_break_type ( uc );
        if ( ( bt == G_UNICODE_BREAK_ALPHABETIC || bt == G_UNICODE_BREAK_HEBREW_LETTER ||
               bt == G_UNICODE_BREAK_NUMERIC || bt == G_UNICODE_BREAK_QUOTATION ) ) {
            break;
        }
    }
    if ( c != tb->text ) {
        while ( ( n = g_utf8_prev_char ( c ) ) ) {
            gunichar          uc = g_utf8_get_char ( n );
            GUnicodeBreakType bt = g_unichar_break_type ( uc );
            if ( !( bt == G_UNICODE_BREAK_ALPHABETIC || bt == G_UNICODE_BREAK_HEBREW_LETTER ||
                    bt == G_UNICODE_BREAK_NUMERIC || bt == G_UNICODE_BREAK_QUOTATION ) ) {
                break;
            }
            c = n;
            if ( n == tb->text ) {
                break;
            }
        }
    }
    int index = g_utf8_pointer_to_offset ( tb->text, c );
    textbox_cursor ( tb, index );
}

// end of line
void textbox_cursor_end ( textbox *tb )
{
    if ( tb->text == NULL ) {
        tb->cursor = 0;
        widget_queue_redraw ( WIDGET ( tb ) );
        return;
    }
    tb->cursor = ( int ) g_utf8_strlen ( tb->text, -1 );
    widget_queue_redraw ( WIDGET ( tb ) );
    // Stop blink!
    tb->blink = 2;
}

// insert text
void textbox_insert ( textbox *tb, const int char_pos, const char *str, const int slen )
{
    if ( tb == NULL ) {
        return;
    }
    char *c  = g_utf8_offset_to_pointer ( tb->text, char_pos );
    int  pos = c - tb->text;
    int  len = ( int ) strlen ( tb->text );
    pos = MAX ( 0, MIN ( len, pos ) );
    // expand buffer
    tb->text = g_realloc ( tb->text, len + slen + 1 );
    // move everything after cursor upward
    char *at = tb->text + pos;
    memmove ( at + slen, at, len - pos + 1 );
    // insert new str
    memmove ( at, str, slen );

    // Set modified, lay out need te be redrawn
    // Stop blink!
    tb->blink   = 2;
    tb->changed = TRUE;
}

// remove text
void textbox_delete ( textbox *tb, int pos, int dlen )
{
    if ( tb == NULL ) {
        return;
    }
    int len = g_utf8_strlen ( tb->text, -1 );
    if ( len == pos ) {
        return;
    }
    pos = MAX ( 0, MIN ( len, pos ) );
    if ( ( pos + dlen ) > len ) {
        dlen = len - dlen;
    }
    // move everything after pos+dlen down
    char *start = g_utf8_offset_to_pointer ( tb->text, pos );
    char *end   = g_utf8_offset_to_pointer  ( tb->text, pos + dlen );
    // Move remainder + closing \0
    memmove ( start, end, ( tb->text + strlen ( tb->text ) ) - end + 1 );
    if ( tb->cursor >= pos && tb->cursor < ( pos + dlen ) ) {
        tb->cursor = pos;
    }
    else if ( tb->cursor >= ( pos + dlen ) ) {
        tb->cursor -= dlen;
    }
    // Set modified, lay out need te be redrawn
    // Stop blink!
    tb->blink   = 2;
    tb->changed = TRUE;
}

/**
 * @param tb Handle to the textbox
 *
 * Delete character after cursor.
 */
static void textbox_cursor_del ( textbox *tb )
{
    if ( tb == NULL || tb->text == NULL ) {
        return;
    }
    textbox_delete ( tb, tb->cursor, 1 );
}

/**
 * @param tb Handle to the textbox
 *
 * Delete character before cursor.
 */
static void textbox_cursor_bkspc ( textbox *tb )
{
    if ( tb && tb->cursor > 0 ) {
        textbox_cursor_dec ( tb );
        textbox_cursor_del ( tb );
    }
}
static void textbox_cursor_bkspc_word ( textbox *tb )
{
    if ( tb && tb->cursor > 0 ) {
        int cursor = tb->cursor;
        textbox_cursor_dec_word ( tb );
        if ( cursor > tb->cursor ) {
            textbox_delete ( tb, tb->cursor, cursor - tb->cursor );
        }
    }
}
static void textbox_cursor_del_eol ( textbox *tb )
{
    if ( tb && tb->cursor >= 0 ) {
        int length = g_utf8_strlen ( tb->text, -1 ) - tb->cursor;
        if ( length >= 0 ) {
            textbox_delete ( tb, tb->cursor, length );
        }
    }
}
static void textbox_cursor_del_sol ( textbox *tb )
{
    if ( tb && tb->cursor >= 0 ) {
        int length = tb->cursor;
        if ( length >= 0 ) {
            textbox_delete ( tb, 0, length );
        }
    }
}
static void textbox_cursor_del_word ( textbox *tb )
{
    if ( tb && tb->cursor >= 0 ) {
        int cursor = tb->cursor;
        textbox_cursor_inc_word ( tb );
        if ( cursor < tb->cursor ) {
            textbox_delete ( tb, cursor, tb->cursor - cursor );
        }
    }
}

// handle a keypress in edit mode
// 2 = nav
// 0 = unhandled
// 1 = handled
// -1 = handled and return pressed (finished)
int textbox_keybinding ( textbox *tb, KeyBindingAction action )
{
    if ( tb == NULL ) {
        return 0;
    }
    if ( !( tb->flags & TB_EDITABLE ) ) {
        return 0;
    }

    switch ( action )
    {
    // Left or Ctrl-b
    case MOVE_CHAR_BACK:
        return ( textbox_cursor_dec ( tb ) == TRUE ) ? 2 : 0;
    // Right or Ctrl-F
    case MOVE_CHAR_FORWARD:
        return ( textbox_cursor_inc ( tb ) == TRUE ) ? 2 : 0;
    // Ctrl-U: Kill from the beginning to the end of the line.
    case CLEAR_LINE:
        textbox_text ( tb, "" );
        return 1;
    // Ctrl-A
    case MOVE_FRONT:
        textbox_cursor ( tb, 0 );
        return 2;
    // Ctrl-E
    case MOVE_END:
        textbox_cursor_end ( tb );
        return 2;
    // Ctrl-Alt-h
    case REMOVE_WORD_BACK:
        textbox_cursor_bkspc_word ( tb );
        return 1;
    // Ctrl-Alt-d
    case REMOVE_WORD_FORWARD:
        textbox_cursor_del_word ( tb );
        return 1;
    case REMOVE_TO_EOL:
        textbox_cursor_del_eol ( tb );
        return 1;
    case REMOVE_TO_SOL:
        textbox_cursor_del_sol ( tb );
        return 1;
    // Delete or Ctrl-D
    case REMOVE_CHAR_FORWARD:
        textbox_cursor_del ( tb );
        return 1;
    // Alt-B, Ctrl-Left
    case MOVE_WORD_BACK:
        textbox_cursor_dec_word ( tb );
        return 2;
    // Alt-F, Ctrl-Right
    case MOVE_WORD_FORWARD:
        textbox_cursor_inc_word ( tb );
        return 2;
    // BackSpace, Shift-BackSpace, Ctrl-h
    case REMOVE_CHAR_BACK:
        textbox_cursor_bkspc ( tb );
        return 1;
    default:
        g_return_val_if_reached ( 0 );
    }
}

gboolean textbox_append_text ( textbox *tb, const char *pad, const int pad_len )
{
    if ( tb == NULL ) {
        return FALSE;
    }
    if ( !( tb->flags & TB_EDITABLE ) ) {
        return FALSE;
    }

    // Filter When alt/ctrl is pressed do not accept the character.

    gboolean    used_something = FALSE;
    const gchar *w, *n, *e;
    for ( w = pad, n = g_utf8_next_char ( w ), e = w + pad_len; w < e; w = n, n = g_utf8_next_char ( n ) ) {
        if ( g_unichar_iscntrl ( g_utf8_get_char ( w ) ) ) {
            continue;
        }
        textbox_insert ( tb, tb->cursor, w, n - w );
        textbox_cursor ( tb, tb->cursor + 1 );
        used_something = TRUE;
    }
    return used_something;
}

static void tbfc_entry_free ( TBFontConfig *tbfc )
{
    pango_font_metrics_unref ( tbfc->metrics );
    if ( tbfc->pfd ) {
        pango_font_description_free ( tbfc->pfd );
    }
    g_free ( tbfc );
}
void textbox_setup ( void )
{
    tbfc_cache = g_hash_table_new_full ( g_str_hash, g_str_equal, NULL, (GDestroyNotify) tbfc_entry_free );
}

/** Name of the default font (if none is given) */
const char *default_font_name = "default";
void textbox_set_pango_context ( const char *font, PangoContext *p )
{
    g_assert ( p_metrics == NULL );
    p_context = g_object_ref ( p );
    p_metrics = pango_context_get_metrics ( p_context, NULL, NULL );
    TBFontConfig *tbfc = g_malloc0 ( sizeof ( TBFontConfig ) );
    tbfc->metrics = p_metrics;
    tbfc->height  = pango_font_metrics_get_ascent ( tbfc->metrics ) + pango_font_metrics_get_descent ( tbfc->metrics );
    g_hash_table_insert ( tbfc_cache, (gpointer *) ( font ? font : default_font_name ), tbfc );
}

void textbox_cleanup ( void )
{
    g_hash_table_destroy ( tbfc_cache );
    if ( p_context ) {
        g_object_unref ( p_context );
        p_context = NULL;
    }
}

int textbox_get_width ( widget *wid )
{
    textbox *tb = (textbox *) wid;
    if ( tb->flags & TB_AUTOWIDTH ) {
        unsigned int offset = ( tb->flags & TB_INDICATOR ) ? DOT_OFFSET : 0;
        return textbox_get_font_width ( tb ) + widget_padding_get_padding_width ( wid ) + offset;
    }
    return tb->widget.w;
}

int _textbox_get_height ( widget *wid )
{
    textbox *tb = (textbox *) wid;
    if ( tb->flags & TB_AUTOHEIGHT ) {
        return textbox_get_height ( tb );
    }
    return tb->widget.h;
}
int textbox_get_height ( const textbox *tb )
{
    return textbox_get_font_height ( tb ) + widget_padding_get_padding_height ( WIDGET ( tb ) );
}

int textbox_get_font_height ( const textbox *tb )
{
    int height;
    pango_layout_get_pixel_size ( tb->layout, NULL, &height );
    return height;
}

int textbox_get_font_width ( const textbox *tb )
{
    PangoRectangle rect;
    pango_layout_get_pixel_extents ( tb->layout, NULL, &rect );
    return rect.width + rect.x;
}

/** Caching for the estimated character height. (em) */
static double char_height = -1;
double textbox_get_estimated_char_height ( void )
{
    if ( char_height < 0 ) {
        int height = pango_font_metrics_get_ascent ( p_metrics ) + pango_font_metrics_get_descent ( p_metrics );
        char_height = ( height ) / (double) PANGO_SCALE;
    }
    return char_height;
}

/** Caching for the expected character width. */
static double char_width = -1;
double textbox_get_estimated_char_width ( void )
{
    if ( char_width < 0 ) {
        int width = pango_font_metrics_get_approximate_char_width ( p_metrics );
        char_width = ( width ) / (double) PANGO_SCALE;
    }
    return char_width;
}

/** Cache storing the estimated width of a digit (ch). */
static double ch_width = -1;
double textbox_get_estimated_ch ( void )
{
    if ( ch_width < 0 ) {
        int width = pango_font_metrics_get_approximate_digit_width ( p_metrics );
        ch_width = ( width ) / (double) PANGO_SCALE;
    }
    return ch_width;
}

int textbox_get_estimated_height ( const textbox *tb, int eh )
{
    int height = pango_font_metrics_get_ascent ( tb->metrics ) + pango_font_metrics_get_descent ( tb->metrics );
    return ( eh * height ) / PANGO_SCALE + widget_padding_get_padding_height ( WIDGET ( tb ) );
}
int textbox_get_desired_width ( widget *wid )
{
    if ( wid == NULL ) {
        return 0;
    }
    textbox      *tb    = (textbox *) wid;
    unsigned int offset = ( ( tb->flags & TB_INDICATOR ) ? DOT_OFFSET : 0 );
    if ( wid->expand && tb->flags & TB_AUTOWIDTH ) {
        return textbox_get_font_width ( tb ) + widget_padding_get_padding_width ( wid ) + offset;
    }
    RofiDistance w  = rofi_theme_get_distance ( WIDGET ( tb ), "width", 0 );
    int          wi = distance_get_pixel ( w, ROFI_ORIENTATION_HORIZONTAL );
    if ( wi > 0 ) {
        return wi;
    }
    int padding = widget_padding_get_left ( WIDGET ( tb ) );
    padding += widget_padding_get_right ( WIDGET ( tb ) );
    int old_width = pango_layout_get_width ( tb->layout );
    pango_layout_set_width ( tb->layout, -1 );
    int width = textbox_get_font_width ( tb );
    // Restore.
    pango_layout_set_width ( tb->layout, old_width );
    return width + padding + offset;
}


void textbox_set_ellipsize ( textbox *tb, PangoEllipsizeMode mode )
{
    if ( tb )
    {
        tb->emode = mode;
        if ( ( tb->flags & TB_WRAP ) != TB_WRAP ) {
            // Store the mode.
            pango_layout_set_ellipsize ( tb->layout, tb->emode );
            widget_queue_redraw ( WIDGET ( tb ) );
        }
    }
}
