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
XftColor     color_fg;
XftColor     color_bg;
XftColor     color_hlfg;
XftColor     color_hlbg;

PangoContext *p_context = NULL;


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

    tb->layout = pango_layout_new ( p_context );

    PangoFontDescription *pfd = pango_font_description_from_string ( config.menu_font );
    pango_layout_set_font_description ( tb->layout, pfd );
    pango_font_description_free ( pfd );

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
    PangoFontDescription *pfd = pango_font_description_from_string ( config.menu_font );
    switch ( tbft )
    {
    case HIGHLIGHT:
        tb->color_bg = color_hlbg;
        tb->color_fg = color_hlfg;
        break;
    case ACTIVE:
        pango_font_description_set_style ( pfd, PANGO_STYLE_ITALIC );
        tb->color_bg = color_bg;
        tb->color_fg = color_fg;
        break;
    case ACTIVE_HIGHLIGHT:
        pango_font_description_set_style ( pfd, PANGO_STYLE_ITALIC );
        tb->color_bg = color_hlbg;
        tb->color_fg = color_hlfg;
        break;
    case NORMAL:
    default:
        tb->color_bg = color_bg;
        tb->color_fg = color_fg;
        break;
    }
    pango_layout_set_font_description ( tb->layout, pfd );
    pango_font_description_free ( pfd );
}

// outer code may need line height, width, etc
void textbox_extents ( textbox *tb )
{
}

// set the default text to display
void textbox_text ( textbox *tb, char *text )
{
    if ( tb->text ) {
        free ( tb->text );
    }

    tb->text = strdup ( text );
    pango_layout_set_text ( tb->layout, tb->text, strlen ( tb->text ) );

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
        h = textbox_get_height ( tb );
    }

    if ( tb->flags & TB_AUTOWIDTH ) {
        if ( w > 1 ) {
            w = MIN ( w, textbox_get_width ( tb ) );
        }
        else{
            w = textbox_get_width ( tb );
        }
    }
    else {
        // set ellipsize
        if( (tb->flags & TB_EDITABLE) == TB_EDITABLE ) {
            pango_layout_set_ellipsize ( tb->layout, PANGO_ELLIPSIZE_MIDDLE );
        }else{
            pango_layout_set_ellipsize ( tb->layout, PANGO_ELLIPSIZE_END );
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
    if ( tb->layout == NULL ) {
        g_object_unref ( tb->layout );
    }

    XDestroyWindow ( display, tb->window );
    free ( tb );
}

void textbox_draw ( textbox *tb )
{
    GC      context = XCreateGC ( display, tb->window, 0, 0 );
    Pixmap  canvas  = XCreatePixmap ( display, tb->window, tb->w, tb->h, DefaultDepth ( display, DefaultScreen ( display ) ) );
    XftDraw *draw   = XftDrawCreate ( display, canvas, DefaultVisual ( display, DefaultScreen ( display ) ), DefaultColormap ( display, DefaultScreen ( display ) ) );

    // clear canvas
    XftDrawRect ( draw, &tb->color_bg, 0, 0, tb->w, tb->h );

    char *text       = tb->text ? tb->text : "";
    int  text_len    = strlen ( text );
    int  length      = text_len;
    int  line_height = textbox_get_font_height ( tb );
    int  line_width  = 0;

    int  cursor_x     = 0;
    int  cursor_width = MAX ( 2, line_height / 10 );

    pango_layout_set_text ( tb->layout, tb->text, strlen ( tb->text ) );

    if ( tb->flags & TB_EDITABLE ) {
        PangoRectangle pos;
        int            cursor_offset = 0;
        cursor_offset = MIN ( tb->cursor, text_len );
        pango_layout_get_cursor_pos ( tb->layout, cursor_offset, &pos, NULL );
        // Add a small 4px offset between cursor and last glyph.
        cursor_x = pos.x / PANGO_SCALE;
    }

    pango_layout_set_width ( tb->layout, PANGO_SCALE * ( tb->w - 2 * SIDE_MARGIN ) );

    int x = PANGO_SCALE*SIDE_MARGIN, y = 0;

    if ( tb->flags & TB_RIGHT ) {
        x = (tb->w - line_width)*PANGO_SCALE;
    }
    else if ( tb->flags & TB_CENTER ) {
        x = (PANGO_SCALE*( tb->w - line_width )) / 2;
    }
    y = (PANGO_SCALE*(textbox_get_width(tb) - textbox_get_font_width(tb)))/2;
    // Render the layout.
    pango_xft_render_layout ( draw, &( tb->color_fg ), tb->layout,
            x , y );

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
    else if ( key == XK_Return || key == XK_KP_Enter ) {
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
    const char *bg, const char *fg,
    const char *hlbg, const char *hlfg
    )
{
    Visual   *visual  = DefaultVisual ( display, DefaultScreen ( display )  );
    Colormap colormap = DefaultColormap ( display, DefaultScreen ( display ) );

    XftColorAllocName ( display, visual, colormap, fg, &color_fg );
    XftColorAllocName ( display, visual, colormap, bg, &color_bg );
    XftColorAllocName ( display, visual, colormap, hlfg, &color_hlfg );
    XftColorAllocName ( display, visual, colormap, hlbg, &color_hlbg );

    PangoFontMap *font_map = pango_xft_get_font_map ( display, DefaultScreen ( display ) );
    p_context = pango_font_map_create_context ( font_map );
}


void textbox_cleanup ()
{
    if ( p_context ) {
        Visual   *visual  = DefaultVisual ( display, DefaultScreen ( display )  );
        Colormap colormap = DefaultColormap ( display, DefaultScreen ( display ) );


        XftColorFree ( display, visual, colormap, &color_fg );
        XftColorFree ( display, visual, colormap, &color_bg );
        XftColorFree ( display, visual, colormap, &color_hlfg );
        XftColorFree ( display, visual, colormap, &color_hlbg );
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
