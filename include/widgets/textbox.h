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

#ifndef ROFI_TEXTBOX_H
#define ROFI_TEXTBOX_H

#include <xkbcommon/xkbcommon.h>
#include <pango/pango.h>
#include <pango/pango-fontmap.h>
#include <pango/pangocairo.h>
#include <cairo.h>
#include "widgets/widget.h"
#include "widgets/widget-internal.h"
#include "keyb.h"

/**
 * @defgroup Textbox Textbox
 * @ingroup widget
 *
 * @{
 */
/**
 * Internal structure of a textbox widget.
 * TODO make this internal to textbox
 */
typedef struct
{
    widget           widget;
    unsigned long    flags;
    short            cursor;
    char             *text;
    PangoLayout      *layout;
    int              tbft;
    int              markup;
    int              changed;

    int              blink;
    guint            blink_timeout;

    double           yalign;
    double           xalign;

    PangoFontMetrics *metrics;
    PangoEllipsizeMode emode;
    //
    const char       *theme_name;
} textbox;

/**
 * Flags for configuring textbox behaviour and looks during creation.
 */
typedef enum
{
    TB_AUTOHEIGHT = 1 << 0,
    TB_AUTOWIDTH  = 1 << 1,
    TB_EDITABLE   = 1 << 19,
    TB_MARKUP     = 1 << 20,
    TB_WRAP       = 1 << 21,
    TB_PASSWORD   = 1 << 22,
    TB_INDICATOR  = 1 << 23,
} TextboxFlags;
/**
 * Flags indicating current state of the textbox.
 */
typedef enum
{
    /** Normal */
    NORMAL     = 0,
    /** Text in box is urgent. */
    URGENT     = 1,
    /** Text in box is active. */
    ACTIVE     = 2,
    /** Text in box is selected. */
    SELECTED   = 4,
    /** Text in box has pango markup. */
    MARKUP     = 8,

    /** Text is on an alternate row */
    ALT        = 16,
    /** Render font highlighted (inverted colors.) */
    HIGHLIGHT  = 32,
    /** Mask for alternate and highlighted */
    FMOD_MASK  = ( ALT | HIGHLIGHT ),
    /** Mask of bits indicating state */
    STATE_MASK = ~( SELECTED | MARKUP | ALT | HIGHLIGHT )
} TextBoxFontType;

/**
 * @param parent The widget's parent.
 * @param type The type of the to be created widget.
 * @param name The name of the to be created widget.
 * @param flags #TextboxFlags indicating the type of textbox.
 * @param tbft #TextBoxFontType current state of textbox.
 * @param text initial text to display.
 * @param xalign Set the Xalign value.
 * @param yalign set the yalign value.
 *
 * Create a new textbox widget.
 *
 * free with #widget_free
 * @returns a new #textbox
 */
textbox* textbox_create ( widget *parent, WidgetType type, const char *name, TextboxFlags flags,
                          TextBoxFontType tbft, const char *text, double xalign, double yalign );
/**
 * @param tb  Handle to the textbox
 * @param tbft The style of font to render.
 *
 * Set the font render style.
 */
void textbox_font ( textbox *tb, TextBoxFontType tbft );

/**
 * @param tb  Handle to the textbox
 * @param text The text to show in the textbox
 *
 * Set the text to show. Cursor is moved to end (if visible)
 */
void textbox_text ( textbox *tb, const char *text );

/**
 * @param tb Handle to the textbox
 * @param action the #KeyBindingAction to execute on textbox
 *
 * Execute an action on the textbox.
 *
 * @return TRUE if action was taken.
 */
int textbox_keybinding ( textbox *tb, KeyBindingAction action );
/**
 * @param tb Handle to the textbox
 * @param pad The text to insert
 * @param pad_len the length of the text
 *
 * The text should be one insert from a keypress..  the first gunichar is validated to be (or not) control
 * return TRUE if inserted
 */
gboolean textbox_append_text ( textbox *tb, const char *pad, const int pad_len );

/**
 * @param tb  Handle to the textbox
 * @param pos New cursor position
 *
 * Set the cursor position (string index)
 */
void textbox_cursor ( textbox *tb, int pos );

/**
 * @param tb   Handle to the textbox
 * @param char_pos  The position to insert the string at
 * @param str  The string to insert.
 * @param slen The length of the string.
 *
 * Insert the string str at position pos.
 */
void textbox_insert ( textbox *tb, const int char_pos, const char *str, const int slen );

/**
 * Setup the cached fonts. This is required to do
 * before any of the textbox_ functions is called.
 * Clean with textbox_cleanup()
 */
void textbox_setup ( void );

/**
 * Cleanup the allocated colors and fonts by textbox_setup().
 */
void textbox_cleanup ( void );

/**
 * @param tb Handle to the textbox
 *
 * Get the height of the textbox
 *
 * @returns the height of the textbox in pixels.
 */
int textbox_get_height ( const textbox *tb );

/**
 * @param tb Handle to the textbox
 *
 * Get the height of the rendered string.
 *
 * @returns the height of the string in pixels.
 */
int textbox_get_font_height ( const textbox *tb );

/**
 * @param tb Handle to the textbox
 *
 * Get the width of the rendered string.
 *
 * @returns the width of the string in pixels.
 */
int textbox_get_font_width ( const textbox *tb );

/**
 * Estimate the width of a character.
 *
 * @returns the width of a character in pixels.
 */
double textbox_get_estimated_char_width ( void );

/**
 * Estimate the width of a 0.
 *
 * @returns the width of a 0 in pixels.
 */
double textbox_get_estimated_ch ( void );
/**
 * Estimate the height of a character.
 *
 * @returns the height of a character in pixels.
 */
double textbox_get_estimated_char_height ( void  );

/**
 * @param tb Handle to the textbox
 * @param pos The start position
 * @param dlen The length
 *
 * Remove dlen bytes from position pos.
 */
void textbox_delete ( textbox *tb, int pos, int dlen );

/**
 * @param tb Handle to the textbox
 * @param x The new horizontal position to place with textbox
 * @param y The new vertical position to place with textbox
 * @param w The new width of the textbox
 * @param h The new height of the textbox
 *
 * Move and resize the textbox.
 * TODO remove for #widget_resize and #widget_move
 */
void textbox_moveresize ( textbox *tb, int x, int y, int w, int h );

/**
 * @param tb Handle to the textbox
 * @param eh The number of rows to display
 *
 * Get the (estimated) with of a character, can be used to calculate window width.
 * This includes padding.
 *
 * @returns the estimated width of a character.
 */
int textbox_get_estimated_height ( const textbox *tb, int eh );
/**
 * @param font The name of the font used.
 * @param p The new default PangoContext
 *
 * Set the default pango context (with font description) for all textboxes.
 */
void textbox_set_pango_context ( const char *font, PangoContext *p );
/**
 * @param tb Handle to the textbox
 * @param list New pango attributes
 *
 * Sets list as active pango attributes.
 */
void textbox_set_pango_attributes ( textbox *tb, PangoAttrList *list );

/**
 * @param tb Handle to the textbox
 *
 * Get the list of currently active pango attributes.
 *
 * @returns the pango attributes
 */
PangoAttrList *textbox_get_pango_attributes ( textbox *tb );

/**
 * @param tb Handle to the textbox
 *
 * @returns the visible text.
 */
const char *textbox_get_visible_text ( const textbox *tb );
/**
 * @param wid The handle to the textbox.
 *
 * TODO: is this deprecated by widget::get_desired_width
 *
 * @returns the desired width of the textbox.
 */
int textbox_get_desired_width ( widget *wid );

/**
 * @param tb  Handle to the textbox
 *
 * Move the cursor to the end of the string.
 */
void textbox_cursor_end ( textbox *tb );

/**
 * @param tb  Handle to the textbox
 * @param mode The PangoEllipsizeMode to use displaying the text in the textbox
 *
 * Set the ellipsizing mode used on the string.
 */
void textbox_set_ellipsize ( textbox *tb, PangoEllipsizeMode mode );
/*@}*/
#endif //ROFI_TEXTBOX_H
