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
    int           tbft;
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
    // Render font normally
    NORMAL     = 1,
    // Alternating row.
    ALT        = 2,
    // Render font highlighted (inverted colors.)
    HIGHLIGHT  = 4,

    STATE_MASK = ( NORMAL | ALT | HIGHLIGHT ),

    BOLD       = 8,
} TextBoxFontType;

textbox* textbox_create ( Window parent,
                          XVisualInfo *vinfo,
                          Colormap map,
                          TextboxFlags flags,
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
 * Show the textbox (map window)
 */
void textbox_show ( textbox *tb );

/**
 * @param tb  Handle to the textbox
 *
 * Render the textbox.
 */
void textbox_draw ( textbox * tb );

/**
 * @param tb  Handle to the textbox
 * @param ev  XEvent key inputs to textbox
 *
 * Let the textbox handle the input event.
 *
 * @returns if the key was handled (1), unhandled(0) or handled and return was pressed (-1)
 */
int textbox_keypress ( textbox *tb, XEvent *ev );

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
 * @param tb  Handle to the textbox
 * @param x   The new x coordinate to move the window to
 * @param y   The new y coordinate to move the window to
 *
 * Move the window to x,y position.
 */
void textbox_move ( textbox *tb, int x, int y );

/**
 * @param tb  Handle to the textbox
 * @param pos The position to insert the string at
 * @param str The string to insert.
 *
 * Insert the string str at position pos.
 */
void textbox_insert ( textbox *tb, int pos, char *str );

/**
 * @param tb  Handle to the textbox
 *
 * Unmap the textbox window. Effectively hiding it.
 */
void textbox_hide ( textbox *tb );

/**
 * @param visual            Information about the visual to target
 * @param colormap          The colormap to set the colors for.
 * @param bg                The background color.
 * @param bg_alt            The background color for alternating row.
 * @param fg                The foreground color.
 * @param hlbg              The background color for a highlighted entry.
 * @param hlfg              The foreground color for a highlighted entry.
 *
 * Setup the cached fonts. This is required to do
 * before any of the textbox_ functions is called.
 * Clean with textbox_cleanup()
 */
void textbox_setup ( XVisualInfo *visual, Colormap colormap,
                     const char *bg, const char *bg_alt, const char *fg,
                     const char *hlbg, const char *hlfg
                     );

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
#endif //__TEXTBOX_H__
