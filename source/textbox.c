/**
 *   MIT/X11 License
 *   Copyright (c) 2012 Sean Pringle <sean.pringle@gmail.com>
 *   Modified  (c) 2013-2016 Qball Cow <qball@gmpclient.org>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining
 *   a copy of this software and associated documentation files (the
 *   "Software"), to deal in the Software without restriction, including
 *   without limitation the rights to use, copy, modify, merge, publish,
 *   distribute, sublicense, and/or sell copies of the Software, and to
 *   permit persons to whom the Software is furnished to do so, subject to
 *   the following conditions:
 *
 *   The above copyright notice and this permission notice shall be
 *   included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <config.h>
#include <xcb/xcb.h>
#include <ctype.h>
#include <string.h>
#include <glib.h>
#include "settings.h"
#include "textbox.h"
#include "keyb.h"
#include "x11-helper.h"
#include "mode.h"
#include "view.h"

#define SIDE_MARGIN    1

/**
 * Font + font color cache.
 * Avoid re-loading font on every change on every textbox.
 */
typedef struct
{
    Color fg;
    Color bg;
    Color bgalt;
    Color hlfg;
    Color hlbg;
} RowColor;

#define num_states    3
RowColor     colors[num_states];

PangoContext *p_context = NULL;
static gboolean textbox_blink ( gpointer data )
{
    textbox *tb = (textbox *) data;
    if ( tb->blink < 2 ) {
        tb->blink  = !tb->blink;
        tb->update = TRUE;
        rofi_view_queue_redraw ( );
    }
    else {
        tb->blink--;
    }
    return TRUE;
}

textbox* textbox_create ( TextboxFlags flags, short x, short y, short w, short h,
                          TextBoxFontType tbft, const char *text )
{
    textbox *tb = g_slice_new0 ( textbox );

    tb->flags = flags;

    tb->widget.x = x;
    tb->widget.y = y;
    tb->widget.w = MAX ( 1, w );
    tb->widget.h = MAX ( 1, h );

    tb->changed = FALSE;

    tb->main_surface = cairo_image_surface_create ( CAIRO_FORMAT_ARGB32, tb->widget.w, tb->widget.h );
    tb->main_draw    = cairo_create ( tb->main_surface );
    tb->layout       = pango_layout_new ( p_context );
    textbox_font ( tb, tbft );

    if ( ( flags & TB_WRAP ) == TB_WRAP ) {
        pango_layout_set_wrap ( tb->layout, PANGO_WRAP_WORD_CHAR );
    }
    textbox_text ( tb, text ? text : "" );
    textbox_cursor_end ( tb );

    // auto height/width modes get handled here
    textbox_moveresize ( tb, tb->widget.x, tb->widget.y, tb->widget.w, tb->widget.h );

    tb->blink_timeout = 0;
    tb->blink         = 1;
    if ( ( flags & TB_EDITABLE ) == TB_EDITABLE ) {
        tb->blink_timeout = g_timeout_add ( 1200, textbox_blink, tb );
    }

    return tb;
}

void textbox_font ( textbox *tb, TextBoxFontType tbft )
{
    TextBoxFontType t = tbft & STATE_MASK;
    // ACTIVE has priority over URGENT if both set.
    if ( t == ( URGENT | ACTIVE ) ) {
        t = ACTIVE;
    }
    RowColor *color = &( colors[t] );
    switch ( ( tbft & FMOD_MASK ) )
    {
    case HIGHLIGHT:
        tb->color_bg = color->hlbg;
        tb->color_fg = color->hlfg;
        break;
    case ALT:
        tb->color_bg = color->bgalt;
        tb->color_fg = color->fg;
        break;
    default:
        tb->color_bg = color->bg;
        tb->color_fg = color->fg;
        break;
    }
    if ( ( tbft & SELECTED ) == SELECTED ) {
        tb->color_bg = color->hlbg;
        tb->color_fg = color->hlfg;
    }
    if ( tb->tbft != tbft ) {
        tb->update = TRUE;
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
    if ( ( tb->flags & TB_PASSWORD ) == TB_PASSWORD ) {
        size_t l = g_utf8_strlen ( tb->text, -1 );
        char   string [l + 1];
        memset ( string, '*', l );
        string[l] = '\0';
        pango_layout_set_attributes ( tb->layout, NULL );
        pango_layout_set_text ( tb->layout, string, l );
    }
    else if ( tb->flags & TB_MARKUP || tb->tbft & MARKUP ) {
        pango_layout_set_markup ( tb->layout, tb->text, -1 );
    }
    else {
        pango_layout_set_attributes ( tb->layout, NULL );
        pango_layout_set_text ( tb->layout, tb->text, -1 );
    }
}

// set the default text to display
void textbox_text ( textbox *tb, const char *text )
{
    tb->update = TRUE;
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
    }

    tb->cursor = MAX ( 0, MIN ( ( int ) strlen ( text ), tb->cursor ) );
}

// within the parent handled auto width/height modes
void textbox_moveresize ( textbox *tb, int x, int y, int w, int h )
{
    if ( tb->flags & TB_AUTOWIDTH ) {
        pango_layout_set_width ( tb->layout, -1 );
        w = textbox_get_width ( tb );
    }
    else {
        // set ellipsize
        if ( ( tb->flags & TB_EDITABLE ) == TB_EDITABLE ) {
            pango_layout_set_ellipsize ( tb->layout, PANGO_ELLIPSIZE_MIDDLE );
        }
        else if ( ( tb->flags & TB_WRAP ) != TB_WRAP ) {
            pango_layout_set_ellipsize ( tb->layout, PANGO_ELLIPSIZE_END );
        }
    }

    if ( tb->flags & TB_AUTOHEIGHT ) {
        // Width determines height!
        int tw = MAX ( 1, w );
        pango_layout_set_width ( tb->layout, PANGO_SCALE * ( tw - 2 * SIDE_MARGIN ) );
        h = textbox_get_height ( tb );
    }

    if ( x != tb->widget.x || y != tb->widget.y || w != tb->widget.w || h != tb->widget.h ) {
        tb->widget.x = x;
        tb->widget.y = y;
        tb->widget.h = MAX ( 1, h );
        tb->widget.w = MAX ( 1, w );
    }

    // We always want to update this
    pango_layout_set_width ( tb->layout, PANGO_SCALE * ( tb->widget.w - 2 * SIDE_MARGIN ) );
    tb->update = TRUE;
}

// will also unmap the window if still displayed
void textbox_free ( textbox *tb )
{
    if ( tb == NULL ) {
        return;
    }
    if ( tb->blink_timeout > 0 ) {
        g_source_remove ( tb->blink_timeout );
        tb->blink_timeout = 0;
    }

    g_free ( tb->text );

    if ( tb->layout != NULL ) {
        g_object_unref ( tb->layout );
    }
    if ( tb->main_draw ) {
        cairo_destroy ( tb->main_draw );
        tb->main_draw = NULL;
    }
    if ( tb->main_surface ) {
        cairo_surface_destroy ( tb->main_surface );
        tb->main_surface = NULL;
    }

    g_slice_free ( textbox, tb );
}

static void texbox_update ( textbox *tb )
{
    if ( tb->update ) {
        if ( tb->main_surface ) {
            cairo_destroy ( tb->main_draw );
            cairo_surface_destroy ( tb->main_surface );
            tb->main_draw    = NULL;
            tb->main_surface = NULL;
        }
        tb->main_surface = cairo_image_surface_create ( CAIRO_FORMAT_ARGB32, tb->widget.w, tb->widget.h );
        tb->main_draw    = cairo_create ( tb->main_surface );
        cairo_set_operator ( tb->main_draw, CAIRO_OPERATOR_SOURCE );

        pango_cairo_update_layout ( tb->main_draw, tb->layout );
        char *text       = tb->text ? tb->text : "";
        int  text_len    = strlen ( text );
        int  font_height = textbox_get_font_height ( tb );

        int  cursor_x     = 0;
        int  cursor_width = MAX ( 2, font_height / 10 );

        if ( tb->changed ) {
            __textbox_update_pango_text ( tb );
        }

        if ( tb->flags & TB_EDITABLE ) {
            PangoRectangle pos;
            int            cursor_offset = 0;
            cursor_offset = MIN ( tb->cursor, text_len );
            pango_layout_get_cursor_pos ( tb->layout, cursor_offset, &pos, NULL );
            // Add a small 4px offset between cursor and last glyph.
            cursor_x = pos.x / PANGO_SCALE;
        }

        // Skip the side MARGIN on the X axis.
        int x = SIDE_MARGIN;
        int y = 0;

        if ( tb->flags & TB_RIGHT ) {
            int line_width = 0;
            // Get actual width.
            pango_layout_get_pixel_size ( tb->layout, &line_width, NULL );
            x = ( tb->widget.w - line_width - SIDE_MARGIN );
        }
        else if ( tb->flags & TB_CENTER ) {
            int tw = textbox_get_font_width ( tb );
            x = (  ( tb->widget.w - tw - 2 * SIDE_MARGIN ) ) / 2;
        }
        short fh = textbox_get_font_height ( tb );
        if ( fh > tb->widget.h ) {
            y = 0;
        }
        else {
            y = (   ( tb->widget.h - fh ) ) / 2;
        }

        // Set ARGB
        Color  col   = tb->color_bg;
        double scale = 1.0;
        if ( ( tb->tbft & SELECTED ) == SELECTED && ( tb->tbft & FMOD_MASK ) != HIGHLIGHT ) {
            scale = 0.4;
        }
        cairo_set_source_rgba ( tb->main_draw, col.red, col.green, col.blue, col.alpha * scale );
        cairo_paint ( tb->main_draw );

        col = tb->color_fg;
        cairo_set_source_rgba ( tb->main_draw, col.red, col.green, col.blue, col.alpha * scale );
        // draw the cursor
        if ( tb->flags & TB_EDITABLE && tb->blink ) {
            cairo_rectangle ( tb->main_draw, x + cursor_x, y, cursor_width, font_height );
            cairo_fill ( tb->main_draw );
        }

        // Set ARGB
        // We need to set over, otherwise subpixel hinting wont work.
        cairo_set_operator ( tb->main_draw, CAIRO_OPERATOR_OVER );
        cairo_move_to ( tb->main_draw, x, y );
        pango_cairo_show_layout ( tb->main_draw, tb->layout );

        tb->update = FALSE;
    }
}
void textbox_draw ( textbox *tb, cairo_t *draw )
{
    texbox_update ( tb );

    /* Write buffer */

    cairo_set_source_surface ( draw, tb->main_surface, tb->widget.x, tb->widget.y );
    cairo_rectangle ( draw, tb->widget.x, tb->widget.y, tb->widget.w, tb->widget.h );
    cairo_fill ( draw );
}

// cursor handling for edit mode
void textbox_cursor ( textbox *tb, int pos )
{
    int length = ( tb->text == NULL ) ? 0 : strlen ( tb->text );
    tb->cursor = MAX ( 0, MIN ( length, pos ) );
    tb->update = TRUE;
}

// move right
void textbox_cursor_inc ( textbox *tb )
{
    int index = g_utf8_next_char ( &( tb->text[tb->cursor] ) ) - tb->text;
    textbox_cursor ( tb, index );
}

// move left
void textbox_cursor_dec ( textbox *tb )
{
    int index = g_utf8_prev_char ( &( tb->text[tb->cursor] ) ) - tb->text;
    textbox_cursor ( tb, index );
}

// Move word right
static void textbox_cursor_inc_word ( textbox *tb )
{
    if ( tb->text == NULL ) {
        return;
    }
    // Find word boundaries, with pango_Break?
    gchar *c = &( tb->text[tb->cursor] );
    while ( ( c = g_utf8_next_char ( c ) ) ) {
        gunichar          uc = g_utf8_get_char ( c );
        GUnicodeBreakType bt = g_unichar_break_type ( uc );
        if ( ( bt == G_UNICODE_BREAK_ALPHABETIC || bt == G_UNICODE_BREAK_HEBREW_LETTER ||
               bt == G_UNICODE_BREAK_NUMERIC || bt == G_UNICODE_BREAK_QUOTATION ) ) {
            break;
        }
    }
    if ( c == NULL ) {
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
    int index = c - tb->text;
    textbox_cursor ( tb, index );
}
// move word left
static void textbox_cursor_dec_word ( textbox *tb )
{
    // Find word boundaries, with pango_Break?
    gchar *n;
    gchar *c = &( tb->text[tb->cursor] );
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
    int index = c - tb->text;
    textbox_cursor ( tb, index );
}

// end of line
void textbox_cursor_end ( textbox *tb )
{
    if ( tb->text == NULL ) {
        tb->cursor = 0;
        tb->update = TRUE;
        return;
    }
    tb->cursor = ( int ) strlen ( tb->text );
    tb->update = TRUE;
}

// insert text
void textbox_insert ( textbox *tb, int pos, char *str, int slen )
{
    int len = ( int ) strlen ( tb->text );
    pos = MAX ( 0, MIN ( len, pos ) );
    // expand buffer
    tb->text = g_realloc ( tb->text, len + slen + 1 );
    // move everything after cursor upward
    char *at = tb->text + pos;
    memmove ( at + slen, at, len - pos + 1 );
    // insert new str
    memmove ( at, str, slen );

    // Set modified, lay out need te be redrawn
    tb->changed = TRUE;
    tb->update  = TRUE;
}

// remove text
void textbox_delete ( textbox *tb, int pos, int dlen )
{
    int len = strlen ( tb->text );
    pos = MAX ( 0, MIN ( len, pos ) );
    // move everything after pos+dlen down
    char *at = tb->text + pos;
    // Move remainder + closing \0
    memmove ( at, at + dlen, len - pos - dlen + 1 );
    if ( tb->cursor >= pos && tb->cursor < ( pos + dlen ) ) {
        tb->cursor = pos;
    }
    else if ( tb->cursor >= ( pos + dlen ) ) {
        tb->cursor -= dlen;
    }
    // Set modified, lay out need te be redrawn
    tb->changed = TRUE;
    tb->update  = TRUE;
}

// delete on character
void textbox_cursor_del ( textbox *tb )
{
    if ( tb->text == NULL ) {
        return;
    }
    int index = g_utf8_next_char ( &( tb->text[tb->cursor] ) ) - tb->text;
    textbox_delete ( tb, tb->cursor, index - tb->cursor );
}

// back up and delete one character
void textbox_cursor_bkspc ( textbox *tb )
{
    if ( tb->cursor > 0 ) {
        textbox_cursor_dec ( tb );
        textbox_cursor_del ( tb );
    }
}
static void textbox_cursor_bkspc_word ( textbox *tb )
{
    if ( tb->cursor > 0 ) {
        int cursor = tb->cursor;
        textbox_cursor_dec_word ( tb );
        if ( cursor > tb->cursor ) {
            textbox_delete ( tb, tb->cursor, cursor - tb->cursor );
        }
    }
}
static void textbox_cursor_del_word ( textbox *tb )
{
    if ( tb->cursor >= 0 ) {
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
    if ( !( tb->flags & TB_EDITABLE ) ) {
        g_return_val_if_reached(0);
    }

    switch ( action )
    {
    // Left or Ctrl-b
    case MOVE_CHAR_BACK:
        textbox_cursor_dec ( tb );
        return 2;
    // Right or Ctrl-F
    case MOVE_CHAR_FORWARD:
        textbox_cursor_inc ( tb );
        return 2;
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
    // Delete or Ctrl-D
    case REMOVE_CHAR_FORWARD:
        textbox_cursor_del ( tb );
        return 1;
    // Alt-B
    case MOVE_WORD_BACK:
        textbox_cursor_dec_word ( tb );
        return 2;
    // Alt-F
    case MOVE_WORD_FORWARD:
        textbox_cursor_inc_word ( tb );
        return 2;
    // BackSpace, Ctrl-h
    case REMOVE_CHAR_BACK:
        textbox_cursor_bkspc ( tb );
        return 1;
    case ACCEPT_CUSTOM:
        return -2;
    case ACCEPT_ENTRY:
        return -1;
    default:
        g_return_val_if_reached(0);
    }

    g_return_val_if_reached(0);
}

gboolean textbox_append ( textbox *tb, char *pad, int pad_len )
{
    if ( !( tb->flags & TB_EDITABLE ) ) {
        return FALSE;
    }
    int old_blink = tb->blink;
    tb->blink = 2;

    // Filter When alt/ctrl is pressed do not accept the character.
    if (  !g_ascii_iscntrl ( *pad ) ) {
        textbox_insert ( tb, tb->cursor, pad, pad_len );
        textbox_cursor ( tb, tb->cursor + pad_len );
        return TRUE;
    }

    tb->blink = old_blink;
    return FALSE;
}

/***
 * Font setup.
 */
static void textbox_parse_string (  const char *str, RowColor *color )
{
    if ( str == NULL ) {
        return;
    }
    char              *cstr = g_strdup ( str );
    char              *endp = NULL;
    char              *token;
    int               index = 0;
    const char *const sep   = ",";
    for ( token = strtok_r ( cstr, sep, &endp ); token != NULL; token = strtok_r ( NULL, sep, &endp ) ) {
        switch ( index )
        {
        case 0:
            color->bg = color_get ( g_strstrip ( token ) );
            break;
        case 1:
            color->fg = color_get ( g_strstrip ( token ) );
            break;
        case 2:
            color->bgalt = color_get ( g_strstrip ( token ) );
            break;
        case 3:
            color->hlbg = color_get ( g_strstrip ( token ) );
            break;
        case 4:
            color->hlfg = color_get ( g_strstrip ( token ) );
            break;
        }
        index++;
    }
    g_free ( cstr );
}
void textbox_setup ( void )
{
    textbox_parse_string ( config.color_normal, &( colors[NORMAL] ) );
    textbox_parse_string ( config.color_urgent, &( colors[URGENT] ) );
    textbox_parse_string ( config.color_active, &( colors[ACTIVE] ) );
}

void textbox_set_pango_context ( PangoContext *p )
{
    textbox_cleanup ();
    p_context = g_object_ref ( p );
}

void textbox_cleanup ( void )
{
    if ( p_context ) {
        g_object_unref ( p_context );
        p_context = NULL;
    }
}

int textbox_get_width ( textbox *tb )
{
    return textbox_get_font_width ( tb ) + 2 * SIDE_MARGIN;
}

int textbox_get_height ( textbox *tb )
{
    return textbox_get_font_height ( tb ) + 2 * SIDE_MARGIN;
}

int textbox_get_font_height ( textbox *tb )
{
    int height;
    pango_layout_get_pixel_size ( tb->layout, NULL, &height );
    return height;
}

int textbox_get_font_width ( textbox *tb )
{
    int width;
    pango_layout_get_pixel_size ( tb->layout, &width, NULL );
    return width;
}

double textbox_get_estimated_char_width ( void )
{
    // Get width
    PangoFontMetrics *metric = pango_context_get_metrics ( p_context, NULL, NULL );
    int              width   = pango_font_metrics_get_approximate_char_width ( metric );
    pango_font_metrics_unref ( metric );
    return ( width ) / (double) PANGO_SCALE;
}

int textbox_get_estimated_char_height ( void )
{
    // Get width
    PangoFontMetrics *metric = pango_context_get_metrics ( p_context, NULL, NULL );
    int              height  = pango_font_metrics_get_ascent ( metric ) + pango_font_metrics_get_descent ( metric );
    pango_font_metrics_unref ( metric );
    return ( height ) / PANGO_SCALE + 2 * SIDE_MARGIN;
}
