#ifndef __TEXTBOX_H__
#define __TEXTBOX_H__
#include <X11/Xft/Xft.h>

#include <pango/pango.h>
#include <pango/pangoxft.h>
#include <pango/pango-fontmap.h>

typedef struct
{
    unsigned long flags;
    Window        window, parent;
    short         x, y, w, h;
    short         cursor;
    XftColor      color_fg, color_bg;
    char          *text;
    XIM           xim;
    XIC           xic;
    PangoLayout   *layout;
} textbox;


typedef enum
{
    TB_AUTOHEIGHT = 1 << 0,
    TB_AUTOWIDTH  = 1 << 1,
    TB_LEFT       = 1 << 16,
    TB_RIGHT      = 1 << 17,
    TB_CENTER     = 1 << 18,
    TB_EDITABLE   = 1 << 19,
} TextboxFlags;

typedef enum
{
    NORMAL,
    HIGHLIGHT,
} TextBoxFontType;

textbox* textbox_create ( Window parent,
                          TextboxFlags flags,
                          short x, short y, short w, short h,
                          TextBoxFontType tbft,
                          char *text );
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

void textbox_text ( textbox *tb, char *text );
void textbox_show ( textbox *tb );
void textbox_draw ( textbox *tb );

int textbox_keypress ( textbox *tb, XEvent *ev );

void textbox_cursor_end ( textbox *tb );
void textbox_cursor ( textbox *tb, int pos );
void textbox_move ( textbox *tb, int x, int y );

void textbox_insert ( textbox *tb, int pos, char *str );

/**
 * @param tb  Handle to the textbox
 *
 * Unmap the textbox window. Effectively hiding it.
 */
void textbox_hide ( textbox *tb );

/**
 * @param font_str          The font to use.
 * @param font_active_str   The font to use for active entries.
 * @param bg                The background color.
 * @param fg                The foreground color.
 * @param hlbg              The background color for a highlighted entry.
 * @param hlfg              The foreground color for a highlighted entry.
 *
 * Setup the cached fonts. This is required to do
 * before any of the textbox_ functions is called.
 * Clean with textbox_cleanup()
 */
void textbox_setup (
    const char *bg, const char *fg,
    const char *hlbg, const char *hlfg
    );

/**
 * Cleanup the allocated colors and fonts by textbox_setup().
 */
void textbox_cleanup ();

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
double textbox_get_estimated_char_width ( );


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

void textbox_delete ( textbox *tb, int pos, int dlen );

#endif //__TEXTBOX_H__
