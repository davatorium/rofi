#ifndef __TEXTBOX_H__
#define __TEXTBOX_H__

typedef struct {
    unsigned long flags;
    Window window, parent;
    short x, y, w, h;
    short cursor;
    XftFont *font;
    XftColor color_fg, color_bg;
    char *text, *prompt;
    XIM xim;
    XIC xic;
    XGlyphInfo extents;
} textbox;


typedef enum {
    TB_AUTOHEIGHT = 1<<0,
    TB_AUTOWIDTH = 1<<1,
    TB_LEFT = 1<<16,
    TB_RIGHT = 1<<17,
    TB_CENTER = 1<<18,
    TB_EDITABLE = 1<<19,
} TextboxFlags;




textbox* textbox_create( Window parent,
        TextboxFlags flags,
        short x, short y, short w, short h,
        char *font, char *fg, char *bg,
        char *text, char *prompt );

void textbox_free( textbox *tb );

void textbox_font( textbox *tb, char *font, char *fg, char *bg );

void textbox_text( textbox *tb, char *text );
void textbox_show( textbox *tb );
void textbox_draw( textbox *tb );

int textbox_keypress( textbox *tb, XEvent *ev );

void textbox_cursor_end( textbox *tb );
#endif //__TEXTBOX_H__
