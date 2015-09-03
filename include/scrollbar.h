#ifndef ROFI_SCROLLBAR_H
#define ROFI_SCROLLBAR_H

typedef struct _scrollbar
{
    Window       window, parent;
    short        x, y, w, h;
    GC           gc;
    unsigned int length;
    unsigned int pos;
    unsigned int pos_length;
} scrollbar;

scrollbar *scrollbar_create ( Window parent, XVisualInfo *vinfo, Colormap map,
                              short x, short y, short w, short h );

void scrollbar_hide ( scrollbar *sb );
void scrollbar_show ( scrollbar *sb );
void scrollbar_free ( scrollbar *sb );
void scrollbar_set_pos_length ( scrollbar *sb, unsigned int pos_length );
void scrollbar_set_pos ( scrollbar *sb, unsigned int pos );
void scrollbar_set_length ( scrollbar *sb, unsigned int length );
void scrollbar_draw ( scrollbar *sb );
#endif // ROFI_SCROLLBAR_H
