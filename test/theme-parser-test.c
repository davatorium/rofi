/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2017 Qball Cow <qball@gmpclient.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <assert.h>
#include <locale.h>
#include <glib.h>
#include <stdio.h>
#include <helper.h>
#include <string.h>
#include <xcb/xcb_ewmh.h>
#include "xcb-internal.h"
#include "rofi.h"
#include "settings.h"
#include "theme.h"
#include "widgets/widget-internal.h"
#include "widgets/textbox.h"


#include <check.h>


int rofi_view_error_dialog ( const char *msg, G_GNUC_UNUSED int markup )
{
    fputs ( msg, stderr );
    return TRUE;
}

void rofi_view_get_current_monitor ( int *width, int *height )
{
    if ( width ) {
        *width = 1920;
    }
    if ( height ) {
        *height = 1080;
    }
}
double textbox_get_estimated_char_height ( void )
{
    return 16.0;
}
xcb_screen_t          *xcb_screen;
xcb_ewmh_connection_t xcb_ewmh;
int                   xcb_screen_nbr;
#include <x11-helper.h>
#ifndef _ck_assert_ptr_null
/* Pointer against NULL comparison macros with improved output
 * compared to ck_assert(). */
/* OP may only be == or !=  */
#define _ck_assert_ptr_null(X, OP) do { \
  const void* _ck_x = (X); \
  ck_assert_msg(_ck_x OP NULL, \
  "Assertion '%s' failed: %s == %#x", \
  #X" "#OP" NULL", \
  #X, _ck_x); \
} while (0)

#define ck_assert_ptr_null(X) _ck_assert_ptr_null(X, ==)
#define ck_assert_ptr_nonnull(X) _ck_assert_ptr_null(X, !=)
#endif

gboolean error = FALSE;
GString *error_msg = NULL;
void rofi_add_error_message ( GString *msg )
{
    ck_assert_ptr_null ( error_msg );
    error_msg = msg;
    error = TRUE;
}

static void theme_parser_setup ( void )
{
    error = 0;

}
static void theme_parser_teardown ( void )
{
    ck_assert_ptr_null ( error_msg );
    ck_assert_int_eq ( error, 0);
    rofi_theme_free ( rofi_theme );
    rofi_theme = NULL;

}

START_TEST (test_core_empty_string )
{
    rofi_theme_parse_string ( "");
    ck_assert_ptr_nonnull ( rofi_theme );
    ck_assert_ptr_null ( rofi_theme->widgets );
    ck_assert_ptr_null ( rofi_theme->properties );
    ck_assert_ptr_null ( rofi_theme->parent );
    ck_assert_str_eq ( rofi_theme->name, "Root" );
}
END_TEST
START_TEST (test_core_empty_global_section )
{
    rofi_theme_parse_string ( " * {}");
    ck_assert_ptr_nonnull ( rofi_theme );
    ck_assert_ptr_null ( rofi_theme->widgets );
    ck_assert_ptr_null ( rofi_theme->properties );
    ck_assert_ptr_null ( rofi_theme->parent );
    ck_assert_str_eq ( rofi_theme->name, "Root" );
}
END_TEST
START_TEST (test_core_empty_section )
{
    rofi_theme_parse_string ( " #test {}");
    ck_assert_ptr_nonnull ( rofi_theme );
    ck_assert_ptr_nonnull ( rofi_theme->widgets );
    ck_assert_ptr_null ( rofi_theme->properties );
    ck_assert_ptr_null ( rofi_theme->parent );
    ck_assert_str_eq ( rofi_theme->name, "Root" );
    ck_assert_str_eq ( rofi_theme->widgets[0]->name, "test" );
    ck_assert_ptr_null ( rofi_theme->widgets[0]->properties );
    ck_assert_ptr_eq ( rofi_theme->widgets[0]->parent, rofi_theme );
}
END_TEST
START_TEST (test_core_error_root )
{
    rofi_theme_parse_string ( "Blaat");
    ck_assert_int_eq ( error, 1 );
    ck_assert_ptr_nonnull ( rofi_theme );
    ck_assert_ptr_null ( rofi_theme->widgets );
    ck_assert_ptr_null ( rofi_theme->properties );
    ck_assert_ptr_null ( rofi_theme->parent );

    ck_assert_str_eq ( error_msg->str, "<big><b>Error while parsing theme:</b></big> <i>Blaat</i>\n"\
	"\tParser error: <span size=\"smaller\" style=\"italic\">syntax error, unexpected error from file parser, expecting end of file or &quot;Element section (&apos;# {name} { ... }&apos;)&quot; or &quot;Default settings section ( &apos;* { ... }&apos;)&quot; or Configuration block</span>\n	Location:     line 1 column 1 to line 1 column 2\n");

    g_string_free ( error_msg, TRUE );
    error_msg = NULL;
    error = 0;
}
END_TEST

START_TEST ( test_core_comments )
{
    rofi_theme_parse_string ( "// Random comments // /*test */");
    rofi_theme_parse_string ( "/* test /* aap */ */");
    rofi_theme_parse_string ( "// Random comments\n// /*test */");
    rofi_theme_parse_string ( "/* test \n*\n* /* aap */ */");
    ck_assert_ptr_nonnull ( rofi_theme );
    ck_assert_ptr_null ( rofi_theme->widgets );
    ck_assert_ptr_null ( rofi_theme->properties );
    ck_assert_ptr_null ( rofi_theme->parent );
    ck_assert_str_eq ( rofi_theme->name, "Root" );

}
END_TEST
START_TEST ( test_core_newline )
{
    rofi_theme_parse_string ( "\r\n\n\r\n\n/*\r\n*/");
    ck_assert_ptr_nonnull ( rofi_theme );
    ck_assert_ptr_null ( rofi_theme->widgets );
    ck_assert_ptr_null ( rofi_theme->properties );
    ck_assert_ptr_null ( rofi_theme->parent );
    ck_assert_str_eq ( rofi_theme->name, "Root" );
}
END_TEST

START_TEST(test_properties_boolean)
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    /** Boolean property */
    rofi_theme_parse_string ( "*{ test: true; test2:/* inline */false; }");
    ck_assert_ptr_nonnull ( rofi_theme );
    ck_assert_ptr_null ( rofi_theme->widgets );
    ck_assert_ptr_nonnull ( rofi_theme->properties );
    ck_assert_ptr_null ( rofi_theme->parent );
    ck_assert_int_eq( rofi_theme_get_boolean ( &wid, "test", FALSE), TRUE );
    ck_assert_int_eq( rofi_theme_get_boolean ( &wid, "test2", TRUE), FALSE );
}
END_TEST

START_TEST(test_properties_boolean_reference)
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { test: true; test2:/* inline */false;} *{ a:@test; b:@test2;}");
    ck_assert_ptr_nonnull ( rofi_theme );
    ck_assert_int_eq ( rofi_theme_get_boolean ( &wid, "test", FALSE),TRUE );
    ck_assert_int_eq ( rofi_theme_get_boolean ( &wid, "b", TRUE),FALSE );
    ck_assert_int_eq ( rofi_theme_get_boolean ( &wid, "a", FALSE),TRUE );
    ck_assert_int_eq ( rofi_theme_get_boolean ( &wid, "test2", TRUE),FALSE );
}
END_TEST

START_TEST ( test_properties_distance_em)
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { test: 10em;}");
    ck_assert_ptr_nonnull ( rofi_theme );
    Distance d = (Distance){ 1, PW_PX, SOLID};
    Padding pi = (Padding){d,d,d,d};
    Padding p = rofi_theme_get_padding ( &wid, "test", pi);
    ck_assert_int_eq (  p.left.distance , 10 );
    ck_assert_int_eq(  p.left.type , PW_EM );
    ck_assert_int_eq(  p.left.style, SOLID);

}
END_TEST
START_TEST ( test_properties_distance_em_linestyle)
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { sol: 1.3em solid; dash: 1.5em dash;}");
    ck_assert_ptr_nonnull ( rofi_theme );
    Distance d = (Distance){ 1, PW_PX, SOLID};
    Padding pi = (Padding){d,d,d,d};
    Padding p = rofi_theme_get_padding ( &wid, "sol", pi);
    ck_assert_double_eq (  p.left.distance , 1.3 );
    ck_assert_int_eq(  p.left.type , PW_EM );
    ck_assert_int_eq(  p.left.style, SOLID);

    p = rofi_theme_get_padding ( &wid, "dash", pi);
    ck_assert_double_eq (  p.left.distance , 1.5 );
    ck_assert_int_eq(  p.left.type , PW_EM );
    ck_assert_int_eq(  p.left.style, DASH);
}
END_TEST
START_TEST ( test_properties_distance_px)
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { test: 10px;}");
    ck_assert_ptr_nonnull ( rofi_theme );
    Distance d = (Distance){ 1, PW_EM, DASH};
    Padding pi = (Padding){d,d,d,d};
    Padding p = rofi_theme_get_padding ( &wid, "test", pi);
    ck_assert_double_eq (  p.left.distance , 10.0 );
    ck_assert_int_eq(  p.left.type , PW_PX );
    ck_assert_int_eq(  p.left.style, SOLID);
}
END_TEST
START_TEST ( test_properties_distance_px_linestyle)
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { sol: 10px solid; dash: 14px dash;}");
    ck_assert_ptr_nonnull ( rofi_theme );
    Distance d = (Distance){ 1, PW_EM, DASH};
    Padding pi = (Padding){d,d,d,d};
    Padding p = rofi_theme_get_padding ( &wid, "sol", pi);
    ck_assert_double_eq (  p.left.distance , 10.0 );
    ck_assert_int_eq(  p.left.type , PW_PX );
    ck_assert_int_eq(  p.left.style, SOLID);
    p = rofi_theme_get_padding ( &wid, "dash", pi);
    ck_assert_double_eq (  p.left.distance , 14.0 );
    ck_assert_int_eq(  p.left.type , PW_PX );
    ck_assert_int_eq(  p.left.style, DASH);
}
END_TEST
START_TEST ( test_properties_distance_percent)
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { test: 10%;}");
    ck_assert_ptr_nonnull ( rofi_theme );
    Distance d = (Distance){ 1, PW_EM, DASH};
    Padding pi = (Padding){d,d,d,d};
    Padding p = rofi_theme_get_padding ( &wid, "test", pi);
    ck_assert_double_eq (  p.left.distance , 10.0 );
    ck_assert_int_eq(  p.left.type , PW_PERCENT);
    ck_assert_int_eq(  p.left.style, SOLID);
}
END_TEST
START_TEST ( test_properties_distance_percent_linestyle)
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { sol: 10% solid; dash: 10% dash;}");
    ck_assert_ptr_nonnull ( rofi_theme );
    Distance d = (Distance){ 1, PW_EM, DASH};
    Padding pi = (Padding){d,d,d,d};
    Padding p = rofi_theme_get_padding ( &wid, "sol", pi);
    ck_assert_double_eq (  p.left.distance , 10.0 );
    ck_assert_int_eq(  p.left.type , PW_PERCENT);
    ck_assert_int_eq(  p.left.style, SOLID);
    p = rofi_theme_get_padding ( &wid, "dash", pi);
    ck_assert_double_eq (  p.left.distance , 10 );
    ck_assert_int_eq(  p.left.type , PW_PERCENT);
    ck_assert_int_eq(  p.left.style, DASH);
}
END_TEST
START_TEST ( test_properties_position)
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { center: center; east: east; west: west; south: south; north:north;}" );
    ck_assert_int_eq ( rofi_theme_get_position ( &wid, "center", WL_SOUTH) , WL_CENTER );
    ck_assert_int_eq ( rofi_theme_get_position ( &wid, "south", WL_EAST) , WL_SOUTH);
    ck_assert_int_eq ( rofi_theme_get_position ( &wid, "east", WL_WEST) , WL_EAST);
    ck_assert_int_eq ( rofi_theme_get_position ( &wid, "west", WL_NORTH) , WL_WEST);
    ck_assert_int_eq ( rofi_theme_get_position ( &wid, "north", WL_CENTER) , WL_NORTH);

    rofi_theme_parse_string ( "* { southwest: southwest; southeast: southeast; northwest: northwest; northeast:northeast;}" );
    ck_assert_int_eq ( rofi_theme_get_position ( &wid, "southwest", WL_EAST) , WL_SOUTH_WEST);
    ck_assert_int_eq ( rofi_theme_get_position ( &wid, "southeast", WL_WEST) , WL_SOUTH_EAST);
    ck_assert_int_eq ( rofi_theme_get_position ( &wid, "northwest", WL_NORTH) , WL_NORTH_WEST);
    ck_assert_int_eq ( rofi_theme_get_position ( &wid, "northeast", WL_CENTER) , WL_NORTH_EAST);
    rofi_theme_parse_string ( "* { southwest: south west; southeast: south east; northwest: north west; northeast:north east;}" );
    ck_assert_int_eq ( rofi_theme_get_position ( &wid, "southwest", WL_EAST) , WL_SOUTH_WEST);
    ck_assert_int_eq ( rofi_theme_get_position ( &wid, "southeast", WL_WEST) , WL_SOUTH_EAST);
    ck_assert_int_eq ( rofi_theme_get_position ( &wid, "northwest", WL_NORTH) , WL_NORTH_WEST);
    ck_assert_int_eq ( rofi_theme_get_position ( &wid, "northeast", WL_CENTER) , WL_NORTH_EAST);
    rofi_theme_parse_string ( "* { westsouth: westsouth; eastsouth: eastsouth; westnorth: westnorth; eastnorth:eastnorth;}" );
    ck_assert_int_eq ( rofi_theme_get_position ( &wid, "westsouth", WL_EAST) , WL_SOUTH_WEST);
    ck_assert_int_eq ( rofi_theme_get_position ( &wid, "eastsouth", WL_WEST) , WL_SOUTH_EAST);
    ck_assert_int_eq ( rofi_theme_get_position ( &wid, "westnorth", WL_NORTH) , WL_NORTH_WEST);
    ck_assert_int_eq ( rofi_theme_get_position ( &wid, "eastnorth", WL_CENTER) , WL_NORTH_EAST);
    rofi_theme_parse_string ( "* { westsouth: west south; eastsouth: east south; westnorth: west north; eastnorth:east north;}" );
    ck_assert_int_eq ( rofi_theme_get_position ( &wid, "westsouth", WL_EAST) , WL_SOUTH_WEST);
    ck_assert_int_eq ( rofi_theme_get_position ( &wid, "eastsouth", WL_WEST) , WL_SOUTH_EAST);
    ck_assert_int_eq ( rofi_theme_get_position ( &wid, "westnorth", WL_NORTH) , WL_NORTH_WEST);
    ck_assert_int_eq ( rofi_theme_get_position ( &wid, "eastnorth", WL_CENTER) , WL_NORTH_EAST);
    rofi_theme_parse_string ( "* { westeast: west east;}" );
    // Should return error.
    // TODO: check error message.
    g_string_free ( error_msg, TRUE);
    error_msg = NULL;
    error = 0;
}
END_TEST

START_TEST ( test_properties_style)
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { none: none; bold: bold; underline: underline; italic: italic;}");
    ThemeHighlight th = { HL_BOLD, {0.0,0.0,0.0,0.0}};
    th = rofi_theme_get_highlight ( &wid, "none",      th);
    ck_assert_int_eq ( th.style , HL_NONE );
    th = rofi_theme_get_highlight ( &wid, "underline", th);
    ck_assert_int_eq ( th.style , HL_UNDERLINE);
    th = rofi_theme_get_highlight ( &wid, "italic",    th);
    ck_assert_int_eq ( th.style , HL_ITALIC);
    th = rofi_theme_get_highlight ( &wid, "bold",      th);
    ck_assert_int_eq ( th.style , HL_BOLD);
}
END_TEST
START_TEST ( test_properties_style2 )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;

    rofi_theme_parse_string ( "* { boldu: bold underline ; boldi: bold italic; underlinei: underline italic; italicu: italic underline;}");
    ThemeHighlight th = { HL_BOLD, {0.0,0.0,0.0,0.0}};
    th = rofi_theme_get_highlight ( &wid, "boldu", th);
    ck_assert_int_eq ( th.style , (HL_UNDERLINE|HL_BOLD));
    th = rofi_theme_get_highlight ( &wid, "boldi", th);
    ck_assert_int_eq ( th.style , (HL_ITALIC|HL_BOLD));
    th = rofi_theme_get_highlight ( &wid, "underlinei", th);
    ck_assert_int_eq ( th.style , (HL_ITALIC|HL_UNDERLINE));
    th = rofi_theme_get_highlight ( &wid, "italicu", th);
    ck_assert_int_eq ( th.style , (HL_ITALIC|HL_UNDERLINE));
}
END_TEST
START_TEST ( test_properties_style_color )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { comb: bold #123; }");
    ThemeHighlight th = { HL_BOLD, {0.0,0.0,0.0,0.0}};
    th = rofi_theme_get_highlight ( &wid, "comb", th);
    ck_assert_int_eq ( th.style , (HL_BOLD|HL_COLOR));
    ck_assert_double_eq ( th.color.red , (1/15.0));
    ck_assert_double_eq ( th.color.green , (2/15.0));
    ck_assert_double_eq ( th.color.blue , (3/15.0));
}
END_TEST

START_TEST ( test_properties_color_h3 )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { red: #F00; green: #0F0; blue: #00F; }");
    ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );
    Property    *p   = rofi_theme_find_property ( twid, P_COLOR, "red", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.red  , 1 );
    ck_assert_double_eq ( p->value.color.green , 0 );
    ck_assert_double_eq ( p->value.color.blue , 0 );
    p   = rofi_theme_find_property ( twid, P_COLOR, "green", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.red  , 0 );
    ck_assert_double_eq ( p->value.color.green , 1 );
    ck_assert_double_eq ( p->value.color.blue , 0 );
    p   = rofi_theme_find_property ( twid, P_COLOR, "blue", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.red  , 0 );
    ck_assert_double_eq ( p->value.color.green , 0 );
    ck_assert_double_eq ( p->value.color.blue , 1 );
}
END_TEST

START_TEST ( test_properties_color_h6 )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { red: #FF0000; green: #00FF00; blue: #0000FF; }");
    ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );
    Property    *p   = rofi_theme_find_property ( twid, P_COLOR, "red", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.red  , 1 );
    ck_assert_double_eq ( p->value.color.green , 0 );
    ck_assert_double_eq ( p->value.color.blue , 0 );
    p   = rofi_theme_find_property ( twid, P_COLOR, "green", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.red  , 0 );
    ck_assert_double_eq ( p->value.color.green , 1 );
    ck_assert_double_eq ( p->value.color.blue , 0 );
    p   = rofi_theme_find_property ( twid, P_COLOR, "blue", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.red  , 0 );
    ck_assert_double_eq ( p->value.color.green , 0 );
    ck_assert_double_eq ( p->value.color.blue , 1 );
}
END_TEST

START_TEST ( test_properties_color_h8 )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { red: #33FF0000; green: #2200FF00; blue: #110000FF; }");
    ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );
    Property    *p   = rofi_theme_find_property ( twid, P_COLOR, "red", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.alpha , 0.2 );
    ck_assert_double_eq ( p->value.color.red  , 1 );
    ck_assert_double_eq ( p->value.color.green , 0 );
    ck_assert_double_eq ( p->value.color.blue , 0 );
    p   = rofi_theme_find_property ( twid, P_COLOR, "green", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.alpha , 1/7.5 );
    ck_assert_double_eq ( p->value.color.red  , 0 );
    ck_assert_double_eq ( p->value.color.green , 1 );
    ck_assert_double_eq ( p->value.color.blue , 0 );
    p   = rofi_theme_find_property ( twid, P_COLOR, "blue", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.alpha , 1/15.0 );
    ck_assert_double_eq ( p->value.color.red  , 0 );
    ck_assert_double_eq ( p->value.color.green , 0 );
    ck_assert_double_eq ( p->value.color.blue , 1 );
}
END_TEST
START_TEST ( test_properties_color_rgb )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { red: rgb(255,0,0); green: rgb(0,255,0); blue: rgb(0,0,255); }");
    ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );
    Property    *p   = rofi_theme_find_property ( twid, P_COLOR, "red", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.red  , 1 );
    ck_assert_double_eq ( p->value.color.green , 0 );
    ck_assert_double_eq ( p->value.color.blue , 0 );
    p   = rofi_theme_find_property ( twid, P_COLOR, "green", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.red  , 0 );
    ck_assert_double_eq ( p->value.color.green , 1 );
    ck_assert_double_eq ( p->value.color.blue , 0 );
    p   = rofi_theme_find_property ( twid, P_COLOR, "blue", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.red  , 0 );
    ck_assert_double_eq ( p->value.color.green , 0 );
    ck_assert_double_eq ( p->value.color.blue , 1 );
}
END_TEST
START_TEST ( test_properties_color_rgba )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { red: rgba(255,0,0,0.3); green: rgba(0,255,0,0.2); blue: rgba(0,0,255,0.7); }");
    ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );
    Property    *p   = rofi_theme_find_property ( twid, P_COLOR, "red", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.alpha , 0.3 );
    ck_assert_double_eq ( p->value.color.red  , 1 );
    ck_assert_double_eq ( p->value.color.green , 0 );
    ck_assert_double_eq ( p->value.color.blue , 0 );
    p   = rofi_theme_find_property ( twid, P_COLOR, "green", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.alpha , 0.2 );
    ck_assert_double_eq ( p->value.color.red  , 0 );
    ck_assert_double_eq ( p->value.color.green , 1 );
    ck_assert_double_eq ( p->value.color.blue , 0 );
    p   = rofi_theme_find_property ( twid, P_COLOR, "blue", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.alpha , 0.7 );
    ck_assert_double_eq ( p->value.color.red  , 0 );
    ck_assert_double_eq ( p->value.color.green , 0 );
    ck_assert_double_eq ( p->value.color.blue , 1 );
}
END_TEST
START_TEST ( test_properties_color_rgba_percent )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { red: rgba(255,0,0,30%); green: rgba(0,255,0,20%); blue: rgba(0,0,255,70.0%); }");
    ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );
    Property *p   = rofi_theme_find_property ( twid, P_COLOR, "red", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.alpha , 0.3 );
    ck_assert_double_eq ( p->value.color.red  , 1 );
    ck_assert_double_eq ( p->value.color.green , 0 );
    ck_assert_double_eq ( p->value.color.blue , 0 );
    p   = rofi_theme_find_property ( twid, P_COLOR, "green", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.alpha , 0.2 );
    ck_assert_double_eq ( p->value.color.red  , 0 );
    ck_assert_double_eq ( p->value.color.green , 1 );
    ck_assert_double_eq ( p->value.color.blue , 0 );
    p   = rofi_theme_find_property ( twid, P_COLOR, "blue", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.alpha , 0.7 );
    ck_assert_double_eq ( p->value.color.red  , 0 );
    ck_assert_double_eq ( p->value.color.green , 0 );
    ck_assert_double_eq ( p->value.color.blue , 1 );
}
END_TEST
START_TEST ( test_properties_color_argb )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { red: argb:33FF0000; green: argb:2200FF00; blue: argb:110000FF; }");
    ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );
    Property    *p   = rofi_theme_find_property ( twid, P_COLOR, "red", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.alpha , 0.2 );
    ck_assert_double_eq ( p->value.color.red  , 1 );
    ck_assert_double_eq ( p->value.color.green , 0 );
    ck_assert_double_eq ( p->value.color.blue , 0 );
    p   = rofi_theme_find_property ( twid, P_COLOR, "green", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.alpha , 1/7.5 );
    ck_assert_double_eq ( p->value.color.red  , 0 );
    ck_assert_double_eq ( p->value.color.green , 1 );
    ck_assert_double_eq ( p->value.color.blue , 0 );
    p   = rofi_theme_find_property ( twid, P_COLOR, "blue", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.alpha , 1/15.0 );
    ck_assert_double_eq ( p->value.color.red  , 0 );
    ck_assert_double_eq ( p->value.color.green , 0 );
    ck_assert_double_eq ( p->value.color.blue , 1 );
}
END_TEST
START_TEST ( test_properties_color_hsl )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { test1: hsl(127,40%,66.66666%); test2: hsl(0, 100%, 50%); }");
    ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );

    Property *p   = rofi_theme_find_property ( twid, P_COLOR, "test1", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.alpha , 1.0 );
    ck_assert_double_eq_tol ( p->value.color.red  , 0x88/255.0 , 0.004);
    ck_assert_double_eq_tol ( p->value.color.green , 0xcd/255.0, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue , 0x90/255.0 , 0.004);
    p   = rofi_theme_find_property ( twid, P_COLOR, "test2", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.alpha , 1.0 );
    ck_assert_double_eq_tol ( p->value.color.red  , 1 , 0.004);
    ck_assert_double_eq_tol ( p->value.color.green , 0, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , 0.004);
}
END_TEST
START_TEST ( test_properties_color_cmyk )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { test1: cmyk ( 41%, 0%, 100%, 0%); test2: cmyk ( 0, 1.0, 1.0, 0);}");
    ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );

    Property *p   = rofi_theme_find_property ( twid, P_COLOR, "test1", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.alpha , 1.0 );
    ck_assert_double_eq_tol ( p->value.color.red  , 0x96/255.0 , 0.004);
    ck_assert_double_eq_tol ( p->value.color.green , 1.0, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue , 0.0 , 0.004);
    p   = rofi_theme_find_property ( twid, P_COLOR, "test2", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq ( p->value.color.alpha , 1.0 );
    ck_assert_double_eq_tol ( p->value.color.red  , 1 , 0.004);
    ck_assert_double_eq_tol ( p->value.color.green , 0, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , 0.004);
}
END_TEST
START_TEST ( test_properties_padding_2 )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { test: 10px 20px;}");
    Distance d = (Distance){ 1, PW_PX, SOLID};
    Padding pi = (Padding){d,d,d,d};
    Padding p = rofi_theme_get_padding ( &wid, "test", pi);
    ck_assert_double_eq (  p.left.distance , 20 );
    ck_assert_int_eq (  p.left.type , PW_PX );
    ck_assert_double_eq (  p.right.distance , 20 );
    ck_assert_int_eq (  p.right.type , PW_PX );
    ck_assert_double_eq (  p.top.distance , 10 );
    ck_assert_int_eq (  p.top.type , PW_PX );
    ck_assert_double_eq (  p.bottom.distance , 10 );
    ck_assert_int_eq (  p.bottom.type , PW_PX );

}
END_TEST
START_TEST ( test_properties_padding_3 )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { test: 10px 30px 20px;}");
    Distance d = (Distance){ 1, PW_PX, SOLID};
    Padding pi = (Padding){d,d,d,d};
    Padding p = rofi_theme_get_padding ( &wid, "test", pi);
    ck_assert_double_eq (  p.left.distance , 30 );
    ck_assert_int_eq (  p.left.type , PW_PX );
    ck_assert_double_eq (  p.right.distance , 30 );
    ck_assert_int_eq (  p.right.type , PW_PX );
    ck_assert_double_eq (  p.top.distance , 10 );
    ck_assert_int_eq (  p.top.type , PW_PX );
    ck_assert_double_eq (  p.bottom.distance , 20 );
    ck_assert_int_eq (  p.bottom.type , PW_PX );

}
END_TEST
START_TEST ( test_properties_padding_4 )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { test: 10px 30px 20px 40px;}");
    Distance d = (Distance){ 1, PW_PX, SOLID};
    Padding pi = (Padding){d,d,d,d};
    Padding p = rofi_theme_get_padding ( &wid, "test", pi);
    ck_assert_double_eq (  p.left.distance , 40 );
    ck_assert_int_eq (  p.left.type , PW_PX );
    ck_assert_double_eq (  p.right.distance , 30 );
    ck_assert_int_eq (  p.right.type , PW_PX );
    ck_assert_double_eq (  p.top.distance , 10 );
    ck_assert_int_eq (  p.top.type , PW_PX );
    ck_assert_double_eq (  p.bottom.distance , 20 );
    ck_assert_int_eq (  p.bottom.type , PW_PX );

}
END_TEST

START_TEST ( test_properties_string )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { font: \"blaat€\"; test: 123.432; }");

    const char *str= rofi_theme_get_string ( &wid, "font", NULL );
    ck_assert_ptr_nonnull ( str );
    ck_assert_int_eq ( g_utf8_collate ( str, "blaat€" ) , 0 );
}
END_TEST
START_TEST ( test_properties_double)
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { test: 123.432; }");
    ck_assert_double_eq ( rofi_theme_get_double ( &wid, "test", 0.0) , 123.432 );
}
END_TEST
START_TEST ( test_properties_integer)
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { yoffset: 4; }");
    ck_assert_int_eq ( rofi_theme_get_integer ( &wid, "yoffset", 0) , 4);
}
END_TEST
START_TEST ( test_configuration )
{
    rofi_theme_parse_string ( "configuration { font: \"blaat€\"; yoffset: 4; }");
    ck_assert_int_eq ( g_utf8_collate ( config.menu_font, "blaat€" ) , 0 );
    ck_assert_int_eq ( config.y_offset , 4 );
}
END_TEST

START_TEST ( test_parse_file_empty )
{
    rofi_theme_parse_file ("/dev/null");
    ck_assert_ptr_nonnull ( rofi_theme );
    ck_assert_ptr_null ( rofi_theme->widgets );
    ck_assert_ptr_null ( rofi_theme->properties );
    ck_assert_ptr_null ( rofi_theme->parent );
    ck_assert_str_eq ( rofi_theme->name, "Root" );
}
END_TEST
START_TEST ( test_parse_file_not_existing)
{
    rofi_theme_parse_file ("/not-existing-file.rasi");
    ck_assert_ptr_null ( rofi_theme );
    ck_assert_int_eq ( error, 1);
    ck_assert_str_eq ( error_msg->str, "Failed to open theme: <i>/not-existing-file.rasi</i>\nError: <b>No such file or directory</b>");

    g_string_free ( error_msg, TRUE);
    error_msg = NULL;
    error = 0;
}
END_TEST
START_TEST ( test_import_empty)
{
    rofi_theme_parse_string("@import \"/dev/null\"");
    ck_assert_ptr_nonnull ( rofi_theme );
    ck_assert_ptr_null ( rofi_theme->widgets );
    ck_assert_ptr_null ( rofi_theme->properties );
    ck_assert_ptr_null ( rofi_theme->parent );
    ck_assert_str_eq ( rofi_theme->name, "Root" );
}
END_TEST

static Suite * theme_parser_suite (void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Theme");

    /* Core test case */
    {
        tc_core = tcase_create("Core");
        tcase_add_checked_fixture(tc_core, theme_parser_setup, theme_parser_teardown);
        tcase_add_test(tc_core, test_core_empty_string);
        tcase_add_test(tc_core, test_core_empty_global_section);
        tcase_add_test(tc_core, test_core_empty_section);
        tcase_add_test(tc_core, test_core_error_root );
        tcase_add_test(tc_core, test_core_comments );
        tcase_add_test(tc_core, test_core_newline );
        suite_add_tcase(s, tc_core);
    }
    {
        TCase *tc_prop_bool = tcase_create("PropertiesBoolean");
        tcase_add_checked_fixture(tc_prop_bool , theme_parser_setup, theme_parser_teardown);
        tcase_add_test ( tc_prop_bool, test_properties_boolean);
        tcase_add_test ( tc_prop_bool, test_properties_boolean_reference);
        suite_add_tcase(s, tc_prop_bool );
    }
    {
        TCase *tc_prop_distance = tcase_create("PropertiesDistance");
        tcase_add_checked_fixture(tc_prop_distance , theme_parser_setup, theme_parser_teardown);
        tcase_add_test ( tc_prop_distance, test_properties_distance_em );
        tcase_add_test ( tc_prop_distance, test_properties_distance_px );
        tcase_add_test ( tc_prop_distance, test_properties_distance_percent );
        tcase_add_test ( tc_prop_distance, test_properties_distance_em_linestyle );
        tcase_add_test ( tc_prop_distance, test_properties_distance_px_linestyle );
        tcase_add_test ( tc_prop_distance, test_properties_distance_percent_linestyle );
        suite_add_tcase(s, tc_prop_distance );
    }
    {
        TCase *tc_prop_position = tcase_create("PropertiesPosition");
        tcase_add_checked_fixture(tc_prop_position, theme_parser_setup, theme_parser_teardown);
        tcase_add_test ( tc_prop_position, test_properties_position);
        suite_add_tcase(s, tc_prop_position );
    }
    {
        TCase *tc_prop_style = tcase_create("PropertiesStyle");
        tcase_add_checked_fixture(tc_prop_style, theme_parser_setup, theme_parser_teardown);
        tcase_add_test ( tc_prop_style, test_properties_style);
        tcase_add_test ( tc_prop_style, test_properties_style2);
        tcase_add_test ( tc_prop_style, test_properties_style_color);
        suite_add_tcase(s, tc_prop_style );
    }
    {
        TCase *tc_prop_color = tcase_create("PropertiesColor");
        tcase_add_checked_fixture(tc_prop_color, theme_parser_setup, theme_parser_teardown);
        tcase_add_test ( tc_prop_color, test_properties_color_h3);
        tcase_add_test ( tc_prop_color, test_properties_color_h6);
        tcase_add_test ( tc_prop_color, test_properties_color_h8);
        tcase_add_test ( tc_prop_color, test_properties_color_rgb);
        tcase_add_test ( tc_prop_color, test_properties_color_rgba);
        tcase_add_test ( tc_prop_color, test_properties_color_rgba_percent);
        tcase_add_test ( tc_prop_color, test_properties_color_argb);
        tcase_add_test ( tc_prop_color, test_properties_color_hsl);
        tcase_add_test ( tc_prop_color, test_properties_color_cmyk);
        suite_add_tcase(s, tc_prop_color );
    }
    {
        TCase *tc_prop_padding = tcase_create("PropertiesPadding");
        tcase_add_checked_fixture(tc_prop_padding, theme_parser_setup, theme_parser_teardown);
        tcase_add_test ( tc_prop_padding, test_properties_padding_2);
        tcase_add_test ( tc_prop_padding, test_properties_padding_3);
        tcase_add_test ( tc_prop_padding, test_properties_padding_4);
        suite_add_tcase(s, tc_prop_padding );
    }
    {
        TCase *tc_prop_string = tcase_create("PropertiesString");
        tcase_add_checked_fixture(tc_prop_string, theme_parser_setup, theme_parser_teardown);
        tcase_add_test ( tc_prop_string, test_properties_string);
        suite_add_tcase(s, tc_prop_string );
    }

    {
        TCase *tc_prop_double = tcase_create("PropertiesDouble");
        tcase_add_checked_fixture(tc_prop_double, theme_parser_setup, theme_parser_teardown);
        tcase_add_test ( tc_prop_double, test_properties_double);
        suite_add_tcase(s, tc_prop_double );
    }
    {
        TCase *tc_prop_integer = tcase_create("PropertiesInteger");
        tcase_add_checked_fixture(tc_prop_integer, theme_parser_setup, theme_parser_teardown);
        tcase_add_test ( tc_prop_integer, test_properties_integer);
        suite_add_tcase(s, tc_prop_integer );
    }
    {
        TCase *tc_prop_configuration = tcase_create("Configuration");
        tcase_add_checked_fixture(tc_prop_configuration, theme_parser_setup, theme_parser_teardown);
        tcase_add_test ( tc_prop_configuration, test_configuration);
        suite_add_tcase(s, tc_prop_configuration );
    }
    {
        TCase *tc_prop_parse_file = tcase_create("ParseFile");
        tcase_add_checked_fixture(tc_prop_parse_file, theme_parser_setup, theme_parser_teardown);
        tcase_add_test ( tc_prop_parse_file, test_parse_file_empty);
        tcase_add_test ( tc_prop_parse_file, test_parse_file_not_existing);
        suite_add_tcase(s, tc_prop_parse_file );
    }
    {
        TCase *tc_prop_import = tcase_create("Import");
        tcase_add_checked_fixture(tc_prop_import, theme_parser_setup, theme_parser_teardown);
        tcase_add_test ( tc_prop_import, test_import_empty);
        suite_add_tcase(s, tc_prop_import );
    }
    return s;
}

int main ( int argc, char ** argv )
{
    cmd_set_arguments ( argc, argv );

    if ( setlocale ( LC_ALL, "" ) == NULL ) {
        fprintf ( stderr, "Failed to set locale.\n" );
        return EXIT_FAILURE;
    }


    Suite *s;
    SRunner *sr;

    s = theme_parser_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
