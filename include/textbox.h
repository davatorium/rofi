#ifndef __TEXTBOX_H__
#define __TEXTBOX_H__

typedef struct
{
    unsigned long flags;
    Window        window, parent;
    short         x, y, w, h;
    short         cursor;
    XftFont       *font;
    XftColor      color_fg, color_bg;
    char          *text;
    XIM           xim;
    XIC           xic;
    XGlyphInfo    extents;
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
    ACTIVE_HIGHLIGHT,
    ACTIVE
} TextBoxFontType;

textbox* textbox_create ( Window parent,
                          TextboxFlags flags,
                          short x, short y, short w, short h,
                          TextBoxFontType tbft,
                          char *text );

void textbox_free ( textbox *tb );

void textbox_font ( textbox *tb, TextBoxFontType tbft );

void textbox_text ( textbox *tb, char *text );
void textbox_show ( textbox *tb );
void textbox_draw ( textbox *tb );

int textbox_keypress ( textbox *tb, XEvent *ev );

void textbox_cursor_end ( textbox *tb );
void textbox_move ( textbox *tb, int x, int y );
void textbox_hide ( textbox *tb );


void textbox_setup (
    const char *font_str, const char *font_active_str,
    const char *bg, const char *fg,
    const char *hlbg, const char *hlfg
    );
void textbox_cleanup ();
#endif //__TEXTBOX_H__
