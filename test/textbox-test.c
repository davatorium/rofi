#include <unistd.h>
#include <stdlib.h>

#include <stdio.h>
#include <assert.h>
#include <glib.h>
#include <history.h>
#include <string.h>
#include <xcb/xcb.h>
#include <textbox.h>
#include <rofi.h>
#include <cairo-xlib.h>
#include "settings.h"

static int   test               = 0;
unsigned int normal_window_mode = 0;

#define TASSERT( a )    {                                 \
        assert ( a );                                     \
        printf ( "Test %3i passed (%s)\n", ++test, # a ); \
}

#include "view.h"
void rofi_view_queue_redraw ()
{
}
Color color_get ( G_GNUC_UNUSED const char *name )
{
    Color retv = { 1.0, 1.0, 1.0, 1.0 };
    return retv;
}

int rofi_view_error_dialog ( const char *msg, G_GNUC_UNUSED int markup )
{
    fputs ( msg, stderr );
    return FALSE;
}

int abe_test_action ( G_GNUC_UNUSED KeyBindingAction action, G_GNUC_UNUSED unsigned int mask, G_GNUC_UNUSED xkb_keysym_t key )
{
    return FALSE;
}
int show_error_message ( const char *msg, int markup )
{
    rofi_view_error_dialog ( msg, markup );
    return 0;
}

int main ( G_GNUC_UNUSED int argc, G_GNUC_UNUSED char **argv )
{
    cairo_surface_t *surf = cairo_image_surface_create ( CAIRO_FORMAT_ARGB32, 100, 100 );
    cairo_t         *draw = cairo_create ( surf );
    PangoContext    *p    = pango_cairo_create_context ( draw );
    textbox_set_pango_context ( p );

    textbox *box = textbox_create ( TB_EDITABLE | TB_AUTOWIDTH | TB_AUTOHEIGHT, 0, 0, -1, -1,
                                    NORMAL, "test" );
    TASSERT ( box != NULL );

    textbox_cursor_end ( box );
    TASSERT ( box->cursor == 4 );
    textbox_cursor ( box, -1 );
    TASSERT ( box->cursor == 0 );
    textbox_cursor ( box, 8 );
    TASSERT ( box->cursor == 4 );
    textbox_cursor ( box, 2 );
    TASSERT ( box->cursor == 2 );
    textbox_insert ( box, 3, "bo", 2 );
    TASSERT ( strcmp ( box->text, "tesbot" ) == 0 );
    textbox_cursor_end ( box );
    TASSERT ( box->cursor == 6 );

    TASSERT ( widget_get_width ( WIDGET ( box ) ) > 0 );
    TASSERT ( textbox_get_height ( box ) > 0 );

    TASSERT ( widget_get_width ( WIDGET ( box ) ) >= textbox_get_font_width ( box )  );
    TASSERT ( textbox_get_height ( box ) >= textbox_get_font_height ( box )  );

    TASSERT ( textbox_get_estimated_char_width ( ) > 0 );

    textbox_cursor_bkspc ( box );
    TASSERT ( strcmp ( box->text, "tesbo" ) == 0 );
    TASSERT ( box->cursor == 5 );

    textbox_cursor_dec ( box );
    TASSERT ( box->cursor == 4 );
    textbox_cursor_del ( box );
    TASSERT ( strcmp ( box->text, "tesb" ) == 0 );
    textbox_cursor_dec ( box );
    TASSERT ( box->cursor == 3 );
    textbox_cursor_inc ( box );
    TASSERT ( box->cursor == 4 );
    textbox_cursor_inc ( box );
    TASSERT ( box->cursor == 4 );
    // Cursor after delete section.
    textbox_delete ( box, 0, 1 );
    TASSERT ( strcmp ( box->text, "esb" ) == 0 );
    TASSERT ( box->cursor == 3 );
    // Cursor before delete.
    textbox_text ( box, "aap noot mies" );
    TASSERT ( strcmp ( box->text, "aap noot mies" ) == 0 );
    textbox_cursor ( box, 3 );
    TASSERT ( box->cursor == 3 );
    textbox_delete ( box, 3, 6 );
    TASSERT ( strcmp ( box->text, "aapmies" ) == 0 );
    TASSERT ( box->cursor == 3 );

    // Cursor within delete
    textbox_text ( box, "aap noot mies" );
    TASSERT ( strcmp ( box->text, "aap noot mies" ) == 0 );
    textbox_cursor ( box, 5 );
    TASSERT ( box->cursor == 5 );
    textbox_delete ( box, 3, 6 );
    TASSERT ( strcmp ( box->text, "aapmies" ) == 0 );
    TASSERT ( box->cursor == 3 );
    // Cursor after delete.
    textbox_text ( box, "aap noot mies" );
    TASSERT ( strcmp ( box->text, "aap noot mies" ) == 0 );
    textbox_cursor ( box, 11 );
    TASSERT ( box->cursor == 11 );
    textbox_delete ( box, 3, 6 );
    TASSERT ( strcmp ( box->text, "aapmies" ) == 0 );
    TASSERT ( box->cursor == 5 );

    textbox_font ( box, HIGHLIGHT );
    //textbox_draw ( box, draw );

    widget_move ( WIDGET ( box ), 12, 13 );
    TASSERT ( box->widget.x == 12 );
    TASSERT ( box->widget.y == 13 );

    widget_free ( WIDGET ( box ) );
    textbox_cleanup ( );
}
