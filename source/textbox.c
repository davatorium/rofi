/*

   MIT/X11 License
   Copyright (c) 2012 Sean Pringle <sean.pringle@gmail.com>
   Modified  (c) 2013-2014 Qball Cow <qball@gmpclient.org>

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#define _GNU_SOURCE
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xft/Xft.h>
#include <ctype.h>

#include "rofi.h"
#include "textbox.h"
#define SIDE_MARGIN    2

extern Display *display;

/**
 * Font + font color cache.
 * Avoid re-loading font on every change on every textbox.
 */
XftFont  *font        = NULL;
XftFont  *font_active = NULL;
XftColor color_fg;
XftColor color_bg;
XftColor color_hlfg;
XftColor color_hlbg;

void textbox_moveresize ( textbox *tb, int x, int y, int w, int h );

// Xft text box, optionally editable
textbox* textbox_create ( Window parent,
                          TextboxFlags flags,
                          short x, short y, short w, short h,
                          TextBoxFontType tbft,
                          char *text )
{
    textbox *tb = calloc ( 1, sizeof ( textbox ) );

    tb->flags  = flags;
    tb->parent = parent;

    tb->x = x;
    tb->y = y;
    tb->w = MAX ( 1, w );
    tb->h = MAX ( 1, h );

    unsigned int cp = ( tbft == NORMAL ) ? color_bg.pixel : color_hlbg.pixel;

    tb->window = XCreateSimpleWindow ( display, tb->parent, tb->x, tb->y, tb->w, tb->h, 0, None, cp );

    // need to preload the font to calc line height
    textbox_font ( tb, tbft );

    textbox_text ( tb, text ? text : "" );
    textbox_cursor_end ( tb );

    // auto height/width modes get handled here
    textbox_moveresize ( tb, tb->x, tb->y, tb->w, tb->h );

    // edit mode controls
    if ( tb->flags & TB_EDITABLE ) {
        tb->xim = XOpenIM ( display, NULL, NULL, NULL );
        tb->xic = XCreateIC ( tb->xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, tb->window, XNFocusWindow, tb->window, NULL );
    }

    return tb;
}

// set an Xft font by name
void textbox_font ( textbox *tb, TextBoxFontType tbft )
{
    switch ( tbft )
    {
    case HIGHLIGHT:
        tb->font     = font;
        tb->color_bg = color_hlbg;
        tb->color_fg = color_hlfg;
        break;
    case ACTIVE:
        tb->font     = font_active;
        tb->color_bg = color_bg;
        tb->color_fg = color_fg;
        break;
    case ACTIVE_HIGHLIGHT:
        tb->font     = font_active;
        tb->color_bg = color_hlbg;
        tb->color_fg = color_hlfg;
        break;
    case NORMAL:
    default:
        tb->font     = font;
        tb->color_bg = color_bg;
        tb->color_fg = color_fg;
        break;
    }
}

// outer code may need line height, width, etc
void textbox_extents ( textbox *tb )
{
    XftTextExtentsUtf8 ( display, tb->font, ( unsigned char * ) tb->text, strlen ( tb->text ), &tb->extents );
}

// set the default text to display
void textbox_text ( textbox *tb, char *text )
{
    if ( tb->text ) {
        free ( tb->text );
    }

    tb->text   = strdup ( text );
    tb->cursor = MAX ( 0, MIN ( ( int ) strlen ( text ), tb->cursor ) );
    textbox_extents ( tb );
}

void textbox_move ( textbox *tb, int x, int y )
{
    if ( x != tb->x || y != tb->y ) {
        tb->x = x;
        tb->y = y;
        XMoveResizeWindow ( display, tb->window, tb->x, tb->y, tb->w, tb->h );
    }
}
// within the parent. handled auto width/height modes
void textbox_moveresize ( textbox *tb, int x, int y, int w, int h )
{
    if ( tb->flags & TB_AUTOHEIGHT ) {
        h = tb->font->ascent + tb->font->descent;
    }

    if ( tb->flags & TB_AUTOWIDTH ) {
        if ( w > 1 ) {
            w = MIN ( w, tb->extents.width + 2 * SIDE_MARGIN );
        }
        else{
            w = tb->extents.width + 2 * SIDE_MARGIN;
        }
    }

    if ( x != tb->x || y != tb->y || w != tb->w || h != tb->h ) {
        tb->x = x;
        tb->y = y;
        tb->w = MAX ( 1, w );
        tb->h = MAX ( 1, h );
        XMoveResizeWindow ( display, tb->window, tb->x, tb->y, tb->w, tb->h );
    }
}

void textbox_show ( textbox *tb )
{
    XMapWindow ( display, tb->window );
}
void textbox_hide ( textbox *tb )
{
    XUnmapWindow ( display, tb->window );
}


// will also unmap the window if still displayed
void textbox_free ( textbox *tb )
{
    if ( tb == NULL ) {
        return;
    }
    if ( tb->flags & TB_EDITABLE ) {
        XDestroyIC ( tb->xic );
        XCloseIM ( tb->xim );
    }

    if ( tb->text ) {
        free ( tb->text );
    }

    XDestroyWindow ( display, tb->window );
    free ( tb );
}

void textbox_draw ( textbox *tb )
{
    XGlyphInfo extents;

    GC         context = XCreateGC ( display, tb->window, 0, 0 );
    Pixmap     canvas  = XCreatePixmap ( display, tb->window, tb->w, tb->h, DefaultDepth ( display, DefaultScreen ( display ) ) );
    XftDraw    *draw   = XftDrawCreate ( display, canvas, DefaultVisual ( display, DefaultScreen ( display ) ), DefaultColormap ( display, DefaultScreen ( display ) ) );

    // clear canvas
    XftDrawRect ( draw, &tb->color_bg, 0, 0, tb->w, tb->h );

    char *text       = tb->text ? tb->text : "";
    int  text_len    = strlen ( text );
    int  length      = text_len;
    int  line_height = tb->font->ascent + tb->font->descent;
    int  line_width  = 0;

    int  cursor_x     = 0;
    int  cursor_width = MAX ( 2, line_height / 10 );

    if ( tb->flags & TB_EDITABLE ) {
        int cursor_offset = 0;
        cursor_offset = MIN ( tb->cursor, text_len );

        // Trailing spaces still go wrong....
        // The replace by _ is needed, one way or the other.
        // Make a copy, and replace all trailing spaces.
        char *test = strdup ( text );
        for ( int iter = strlen ( text ) - 1; iter >= 0 && test[iter] == ' '; iter-- ) {
            test[iter] = '_';
        }
        // calc cursor position
        XftTextExtentsUtf8 ( display, tb->font, ( unsigned char * ) test, cursor_offset, &extents );
        // Add a small 4px offset between cursor and last glyph.
        cursor_x = extents.width + ( ( extents.width > 0 ) ? 2 : 0 );
        free ( test );
    }

    // calc full input text width
    // Calculate the right size, so no characters are cut off.
    // TODO: Check performance of this.
    do {
        XftTextExtentsUtf8 ( display, tb->font, ( unsigned char * ) text, length, &extents );
        line_width = extents.width;
        if ( line_width <= ( tb->w - 2 * SIDE_MARGIN ) ) {
            break;
        }

        for ( length -= 1; length > 0 && ( text[length] & 0xc0 ) == 0x80; length -= 1 ) {
            ;
        }
    } while ( line_width > 0 );

    int x = SIDE_MARGIN, y = tb->font->ascent;

    if ( tb->flags & TB_RIGHT ) {
        x = tb->w - line_width;
    }

    if ( tb->flags & TB_CENTER ) {
        x = ( tb->w - line_width ) / 2;
    }

    // draw the text.
    XftDrawStringUtf8 ( draw, &tb->color_fg, tb->font, x, y, ( unsigned char * ) text, length );

    // draw the cursor
    if ( tb->flags & TB_EDITABLE ) {
        XftDrawRect ( draw, &tb->color_fg, cursor_x + SIDE_MARGIN, 2, cursor_width, line_height - 4 );
    }

    XftDrawRect ( draw, &tb->color_bg, tb->w, 0, 0, tb->h );
    // flip canvas to window
    XCopyArea ( display, canvas, tb->window, context, 0, 0, tb->w, tb->h, 0, 0 );

    XFreeGC ( display, context );
    XftDrawDestroy ( draw );
    XFreePixmap ( display, canvas );
}


static size_t nextrune ( textbox *tb, int inc )
{
    ssize_t n;

    /* return location of next utf8 rune in the given direction (+1 or -1) */
    for ( n = tb->cursor + inc; n + inc >= 0 && ( tb->text[n] & 0xc0 ) == 0x80; n += inc ) {
        ;
    }
    return n;
}
// cursor handling for edit mode
void textbox_cursor ( textbox *tb, int pos )
{
    tb->cursor = MAX ( 0, MIN ( ( int ) strlen ( tb->text ), pos ) );
}

// move right
void textbox_cursor_inc ( textbox *tb )
{
    textbox_cursor ( tb, nextrune ( tb, 1 ) );
}

// move left
void textbox_cursor_dec ( textbox *tb )
{
    textbox_cursor ( tb, nextrune ( tb, -1 ) );
}


// end of line
void textbox_cursor_end ( textbox *tb )
{
    tb->cursor = ( int ) strlen ( tb->text );
}

// insert text
void textbox_insert ( textbox *tb, int pos, char *str )
{
    int len = ( int ) strlen ( tb->text ), slen = ( int ) strlen ( str );
    pos = MAX ( 0, MIN ( len, pos ) );
    // expand buffer
    tb->text = realloc ( tb->text, len + slen + 1 );
    // move everything after cursor upward
    char *at = tb->text + pos;
    memmove ( at + slen, at, len - pos + 1 );
    // insert new str
    memmove ( at, str, slen );
    textbox_extents ( tb );
}

// remove text
void textbox_delete ( textbox *tb, int pos, int dlen )
{
    int len = strlen ( tb->text );
    pos = MAX ( 0, MIN ( len, pos ) );
    // move everything after pos+dlen down
    char *at = tb->text + pos;
    memmove ( at, at + dlen, len - pos );
    textbox_extents ( tb );
}

// delete on character
void textbox_cursor_del ( textbox *tb )
{
    int del_r = nextrune ( tb, 1 );
    textbox_delete ( tb, tb->cursor, del_r - tb->cursor );
}

// back up and delete one character
void textbox_cursor_bkspc ( textbox *tb )
{
    if ( tb->cursor > 0 ) {
        textbox_cursor_dec ( tb );
        textbox_cursor_del ( tb );
    }
}

// handle a keypress in edit mode
// 0 = unhandled
// 1 = handled
// -1 = handled and return pressed (finished)
int textbox_keypress ( textbox *tb, XEvent *ev )
{
    KeySym key;
    Status stat;
    char   pad[32];
    int    len;

    if ( !( tb->flags & TB_EDITABLE ) ) {
        return 0;
    }

    len      = Xutf8LookupString ( tb->xic, &ev->xkey, pad, sizeof ( pad ), &key, &stat );
    pad[len] = 0;

    if ( key == XK_Left ) {
        textbox_cursor_dec ( tb );
        return 1;
    }
    else if ( key == XK_Right ) {
        textbox_cursor_inc ( tb );
        return 1;
    } /*else if ( key == XK_Home ) {

         textbox_cursor_home( tb );
         return 1;
         } else if ( key == XK_End ) {
         textbox_cursor_end( tb );
         return 1;
         } */
    else if ( key == XK_Delete ) {
        textbox_cursor_del ( tb );
        return 1;
    }
    else if ( key == XK_BackSpace ) {
        textbox_cursor_bkspc ( tb );
        return 1;
    }
    else if ( key == XK_Return ) {
        return -1;
    }
    else if ( !iscntrl ( *pad ) ) {
        textbox_insert ( tb, tb->cursor, pad );
        textbox_cursor_inc ( tb );
        return 1;
    }

    return 0;
}



/***
 * Font setup.
 */

void textbox_setup (
    const char *font_str, const char *font_active_str,
    const char *bg, const char *fg,
    const char *hlbg, const char *hlfg
    )
{
    Visual   *visual  = DefaultVisual ( display, DefaultScreen ( display )  );
    Colormap colormap = DefaultColormap ( display, DefaultScreen ( display ) );
    font        = XftFontOpenName ( display, DefaultScreen ( display ), font_str );
    font_active = XftFontOpenName ( display, DefaultScreen ( display ), font_active_str );

    XftColorAllocName ( display, visual, colormap, fg, &color_fg );
    XftColorAllocName ( display, visual, colormap, bg, &color_bg );
    XftColorAllocName ( display, visual, colormap, hlfg, &color_hlfg );
    XftColorAllocName ( display, visual, colormap, hlbg, &color_hlbg );
}


void textbox_cleanup ()
{
    if ( font != NULL ) {
        Visual   *visual  = DefaultVisual ( display, DefaultScreen ( display )  );
        Colormap colormap = DefaultColormap ( display, DefaultScreen ( display ) );

        XftFontClose ( display, font );
        font = NULL;

        XftFontClose ( display, font_active );
        font_active = NULL;

        XftColorFree ( display, visual, colormap, &color_fg );
        XftColorFree ( display, visual, colormap, &color_bg );
        XftColorFree ( display, visual, colormap, &color_hlfg );
        XftColorFree ( display, visual, colormap, &color_hlbg );
    }
}
