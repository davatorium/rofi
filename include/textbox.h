#ifndef ROFI_TEXTBOX_H
#define ROFI_TEXTBOX_H

#include <xkbcommon/xkbcommon.h>
#include <pango/pango.h>
#include <pango/pango-fontmap.h>
#include <pango/pangocairo.h>
#include <cairo.h>
#include "widget.h"
#include "x11-helper.h"

/**
 * @defgroup Textbox Textbox
 * @ingroup Widgets
 *
 * @{
 */

typedef struct
{
    Widget          widget;
    unsigned long   flags;
    short           cursor;
    Color           color_fg, color_bg;
    char            *text;
    PangoLayout     *layout;
    int             tbft;
    int             markup;
    int             changed;

    cairo_surface_t *main_surface;
    cairo_t         *main_draw;

    int             update;
    int             blink;
    guint           blink_timeout;
} textbox;

typedef enum
{
    TB_AUTOHEIGHT    = 1 << 0,
        TB_AUTOWIDTH = 1 << 1,
        TB_LEFT      = 1 << 16,
        TB_RIGHT     = 1 << 17,
        TB_CENTER    = 1 << 18,
        TB_EDITABLE  = 1 << 19,
        TB_MARKUP    = 1 << 20,
        TB_WRAP      = 1 << 21,
        TB_PASSWORD  = 1 << 22,
} TextboxFlags;

typedef enum
{
    // Render font normally
    NORMAL     = 0,
    URGENT     = 1,
    ACTIVE     = 2,
    SELECTED   = 4,
    MARKUP     = 8,

    // Alternating row.
    ALT        = 16,
    // Render font highlighted (inverted colors.)
    HIGHLIGHT  = 32,

    FMOD_MASK  = ( ALT | HIGHLIGHT ),
    STATE_MASK = ~( SELECTED | MARKUP | ALT | HIGHLIGHT )
} TextBoxFontType;

textbox* textbox_create ( TextboxFlags flags,
                          short x, short y, short w, short h,
                          TextBoxFontType tbft,
                          const char *text );
/**
 * @param tb  Handle to the textbox
 *
 * Free the textbox and all allocated memory.
 */
void textbox_free ( textbox *tb );

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
 * @param tb  Handle to the textbox
 *
 * Render the textbox.
 */
void textbox_draw ( textbox *tb, cairo_t *draw );

/**
 * @param tb  Handle to the textbox
 * @param ev  XEvent key inputs to textbox
 *
 * Let the textbox handle the input event.
 *
 * @returns if the key was handled (1), unhandled(0) or handled and return was pressed (-1)
 */
int textbox_keybinding ( textbox *tb, unsigned int modstate, xkb_keysym_t key );
gboolean textbox_append ( textbox *tb, char *pad, int pad_len );

/**
 * @param tb  Handle to the textbox
 *
 * Move the cursor to the end of the string.
 */
void textbox_cursor_end ( textbox *tb );

/**
 * @param tb  Handle to the textbox
 * @param pos New cursor position
 *
 * Set the cursor position (string index)
 */
void textbox_cursor ( textbox *tb, int pos );

/**
 * @param tb   Handle to the textbox
 * @param pos  The position to insert the string at
 * @param str  The string to insert.
 * @param slen The length of the string.
 *
 * Insert the string str at position pos.
 */
void textbox_insert ( textbox *tb, int pos, char *str, int slen );

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
int textbox_get_height ( textbox *tb );

/**
 * @param tb Handle to the textbox
 *
 * Get the width of the textbox
 *
 * @returns the width of the textbox in pixels.
 */
int textbox_get_width ( textbox *tb );

/**
 * @param tb Handle to the textbox
 *
 * Get the height of the rendered string.
 *
 * @returns the height of the string in pixels.
 */
int textbox_get_font_height ( textbox *tb );

/**
 * @param tb Handle to the textbox
 *
 * Get the width of the rendered string.
 *
 * @returns the width of the string in pixels.
 */
int textbox_get_font_width ( textbox *tb );

/**
 * Estimate the width of a character.
 *
 * @returns the width of a character in pixels.
 */
double textbox_get_estimated_char_width ( void );

/**
 * @param tb Handle to the textbox
 *
 * Delete character before cursor.
 */
void textbox_cursor_bkspc ( textbox *tb );

/**
 * @param tb Handle to the textbox
 *
 * Delete character after cursor.
 */
void textbox_cursor_del ( textbox *tb );

/**
 * @param tb Handle to the textbox
 *
 * Move cursor one position backward.
 */
void textbox_cursor_dec ( textbox *tb );

/**
 * @param tb Handle to the textbox
 *
 * Move cursor one position forward.
 */
void textbox_cursor_inc ( textbox *tb );

/**
 * @param tb Handle to the textbox
 * @param pos The start position
 * @param dlen The length
 *
 * Remove dlen bytes from position pos.
 */
void textbox_delete ( textbox *tb, int pos, int dlen );

void textbox_moveresize ( textbox *tb, int x, int y, int w, int h );
int textbox_get_estimated_char_height ( void );
void textbox_set_pango_context ( PangoContext *p );

/*@}*/
#endif //ROFI_TEXTBOX_H
