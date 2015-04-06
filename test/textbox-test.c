#include <unistd.h>
#include <stdlib.h>

#include <stdio.h>
#include <assert.h>
#include <glib.h>
#include <history.h>
#include <string.h>
#include <X11/X.h>
#include <X11/Xlib.h>

#include <textbox.h>
#include <rofi.h>


static int test = 0;

#define TASSERT(a) {\
    assert ( a );\
    printf("Test %3i passed (%s)\n", ++test, #a);\
}

Display *display = NULL;
Colormap    map = None;
XVisualInfo vinfo;

static unsigned int color_get ( Display *display, const char *const name )
{
    XColor color;
    // Special format.
    if ( strncmp ( name, "argb:", 5 ) == 0 ) {
        return strtoul ( &name[5], NULL, 16 );
    }
    else {
        return XAllocNamedColor ( display, map, name, &color, &color ) ? color.pixel : None;
    }
}

static void create_visual_and_colormap()
{
    map = None;
    // Try to create TrueColor map
    if(XMatchVisualInfo ( display, DefaultScreen ( display ), 32, TrueColor, &vinfo )) {
        // Visual found, lets try to create map.
        map = XCreateColormap ( display, DefaultRootWindow ( display ), vinfo.visual, AllocNone );
    }
    // Failed to create map.
    if (map == None ) {
        // Two fields we use.
        vinfo.visual = DefaultVisual(display, DefaultScreen(display));
        vinfo.depth = DefaultDepth(display, DefaultScreen(display));
        map = DefaultColormap( display, DefaultScreen (display));
    }
}
int main ( G_GNUC_UNUSED int argc, G_GNUC_UNUSED char **argv )
{

    // Get DISPLAY
    const char *display_str = getenv ( "DISPLAY" );
    if ( !( display = XOpenDisplay ( display_str ) ) ) {
        fprintf ( stderr, "cannot open display!\n" );
        return EXIT_FAILURE;
    }
    create_visual_and_colormap();
    TASSERT( display != NULL );
    XSetWindowAttributes attr;
    attr.colormap         = map;
    attr.border_pixel     = color_get ( display, config.menu_bc );
    attr.background_pixel = color_get ( display, config.menu_bg );
    Window mw = XCreateWindow ( display, DefaultRootWindow ( display ),
                                 0, 0, 200, 100, config.menu_bw, vinfo.depth, InputOutput,
                                 vinfo.visual, CWColormap | CWBorderPixel | CWBackPixel, &attr );
    TASSERT( mw != None );
    // Set alternate row to normal row.
    config.menu_bg_alt = config.menu_bg;
    textbox_setup ( &vinfo, map ); 
    textbox *box = textbox_create(mw , &vinfo, map, TB_EDITABLE|TB_AUTOWIDTH|TB_AUTOHEIGHT, 0,0, -1, -1, NORMAL, "test");
    TASSERT( box != NULL );

    textbox_cursor_end ( box );
    TASSERT ( box->cursor == 4); 
    textbox_cursor ( box, -1 );
    TASSERT ( box->cursor == 0 ); 
    textbox_cursor ( box, 8 );
    TASSERT ( box->cursor == 4 ); 
    textbox_cursor ( box, 2 );
    TASSERT ( box->cursor == 2 ); 
    textbox_insert ( box, 3, "bo");
    TASSERT ( strcmp(box->text, "tesbot") == 0 ); 
    textbox_cursor_end ( box );
    TASSERT ( box->cursor == 6); 

    TASSERT( textbox_get_width( box) > 0 );
    TASSERT( textbox_get_height( box) > 0 );

    TASSERT( textbox_get_width( box) >= textbox_get_font_width( box)  );
    TASSERT( textbox_get_height( box) >= textbox_get_font_height( box)  );

    TASSERT( textbox_get_estimated_char_width ( ) > 0 );

    textbox_cursor_bkspc ( box );
    TASSERT ( strcmp(box->text, "tesbo") == 0 ); 
    TASSERT ( box->cursor == 5); 

    textbox_cursor_dec ( box );
    TASSERT ( box->cursor == 4); 
    textbox_cursor_del ( box );
    TASSERT ( strcmp(box->text, "tesb") == 0 ); 
    textbox_cursor_dec ( box );
    TASSERT ( box->cursor == 3); 
    textbox_cursor_inc ( box );
    TASSERT ( box->cursor == 4); 
    textbox_cursor_inc ( box );
    TASSERT ( box->cursor == 4); 
    // Cursor after delete section.
    textbox_delete ( box, 0, 1 );
    TASSERT ( strcmp(box->text, "esb") == 0 ); 
    TASSERT ( box->cursor == 3); 
    // Cursor before delete.
    textbox_text( box, "aap noot mies");
    TASSERT ( strcmp(box->text, "aap noot mies") == 0 ); 
    textbox_cursor( box, 3 );
    TASSERT ( box->cursor == 3); 
    textbox_delete ( box, 3, 6 );
    TASSERT ( strcmp(box->text, "aapmies") == 0 ); 
    TASSERT ( box->cursor == 3); 

    // Cursor within delete
    textbox_text( box, "aap noot mies");
    TASSERT ( strcmp(box->text, "aap noot mies") == 0 ); 
    textbox_cursor( box, 5 );
    TASSERT ( box->cursor == 5); 
    textbox_delete ( box, 3, 6 );
    TASSERT ( strcmp(box->text, "aapmies") == 0 ); 
    TASSERT ( box->cursor == 3); 
    // Cursor after delete. 
    textbox_text( box, "aap noot mies");
    TASSERT ( strcmp(box->text, "aap noot mies") == 0 ); 
    textbox_cursor( box, 11 );
    TASSERT ( box->cursor == 11); 
    textbox_delete ( box, 3, 6 );
    TASSERT ( strcmp(box->text, "aapmies") == 0 ); 
    TASSERT ( box->cursor == 5); 


    textbox_font ( box, HIGHLIGHT );
    textbox_draw( box);

    textbox_show( box );
    textbox_move ( box, 12, 13);
    TASSERT ( box->x == 12 );
    TASSERT ( box->y == 13 );
    textbox_hide( box );

    textbox_free(box);
    textbox_cleanup( );
    XDestroyWindow ( display, mw);
    XCloseDisplay ( display );
}
