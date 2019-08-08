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
#include "display.h"
#include "xcb.h"
#include "theme.h"
#include "css-colors.h"
#include "widgets/widget-internal.h"
#include "widgets/textbox.h"


#include <check.h>

#define REAL_COMPARE_DELTA 0.001


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

double textbox_get_estimated_ch ( void )
{
    return 8.0;
}

int monitor_active ( G_GNUC_UNUSED workarea *mon )
{
    return 0;
}

void display_startup_notification ( G_GNUC_UNUSED RofiHelperExecuteContext *context, G_GNUC_UNUSED GSpawnChildSetupFunc *child_setup, G_GNUC_UNUSED gpointer *user_data )
{
}

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
    //ck_assert_ptr_null ( rofi_theme->widgets );
    ck_assert_ptr_null ( rofi_theme->properties );
    ck_assert_ptr_null ( rofi_theme->parent );
    ck_assert_str_eq ( rofi_theme->name, "Root" );
}
END_TEST
START_TEST (test_core_empty_global_section )
{
    rofi_theme_parse_string ( " * {}");
    ck_assert_ptr_nonnull ( rofi_theme );
    //ck_assert_ptr_null ( rofi_theme->widgets );
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
    ck_assert_str_eq ( rofi_theme->widgets[3]->name, "test" );
    ck_assert_ptr_null ( rofi_theme->widgets[3]->properties );
    ck_assert_ptr_eq ( rofi_theme->widgets[3]->parent, rofi_theme );
}
END_TEST
START_TEST (test_core_error_root )
{
    rofi_theme_parse_string ( "Blaat");
    ck_assert_int_eq ( error, 1 );
    ck_assert_ptr_nonnull ( rofi_theme );
    //ck_assert_ptr_null ( rofi_theme->widgets );
    ck_assert_ptr_null ( rofi_theme->properties );
    ck_assert_ptr_null ( rofi_theme->parent );
    const char *error_str =  "<big><b>Error while parsing theme:</b></big> <i>Blaat</i>\n"\
"	Parser error: <span size=\"smaller\" style=\"italic\">syntax error, unexpected end of file, expecting &quot;bracket open (&apos;{&apos;)&quot; or &quot;Selector separator (&apos;,&apos;)&quot;</span>\n"\
"	Location:     line 1 column 6 to line 1 column 6\n";
    ck_assert_str_eq ( error_msg->str, error_str );


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
    //ck_assert_ptr_null ( rofi_theme->widgets );
    ck_assert_ptr_null ( rofi_theme->properties );
    ck_assert_ptr_null ( rofi_theme->parent );
    ck_assert_str_eq ( rofi_theme->name, "Root" );

    // Test comment on last lines
    rofi_theme_parse_string ( "// c++ style" );
    rofi_theme_parse_string ( "/* c style" );
}
END_TEST
START_TEST ( test_core_newline )
{
    rofi_theme_parse_string ( "\r\n\n\r\n\n/*\r\n*/");
    ck_assert_ptr_nonnull ( rofi_theme );
    //ck_assert_ptr_null ( rofi_theme->widgets );
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
    //ck_assert_ptr_null ( rofi_theme->widgets );
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
    RofiDistance d = (RofiDistance){ 1, ROFI_PU_PX, ROFI_HL_SOLID};
    RofiPadding pi = (RofiPadding){d,d,d,d};
    RofiPadding p = rofi_theme_get_padding ( &wid, "test", pi);
    ck_assert_int_eq (  p.left.distance , 10 );
    ck_assert_int_eq(  p.left.type , ROFI_PU_EM );
    ck_assert_int_eq(  p.left.style, ROFI_HL_SOLID);

}
END_TEST
START_TEST ( test_properties_distance_em_linestyle)
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { sol: 1.3em solid; dash: 1.5em dash;}");
    ck_assert_ptr_nonnull ( rofi_theme );
    RofiDistance d = (RofiDistance){ 1, ROFI_PU_PX, ROFI_HL_SOLID};
    RofiPadding pi = (RofiPadding){d,d,d,d};
    RofiPadding p = rofi_theme_get_padding ( &wid, "sol", pi);
    ck_assert_double_eq_tol (  p.left.distance , 1.3 , REAL_COMPARE_DELTA );
    ck_assert_int_eq(  p.left.type , ROFI_PU_EM );
    ck_assert_int_eq(  p.left.style, ROFI_HL_SOLID);

    p = rofi_theme_get_padding ( &wid, "dash", pi);
    ck_assert_double_eq_tol (  p.left.distance , 1.5 , REAL_COMPARE_DELTA );
    ck_assert_int_eq(  p.left.type , ROFI_PU_EM );
    ck_assert_int_eq(  p.left.style, ROFI_HL_DASH);
}
END_TEST
START_TEST ( test_properties_distance_px)
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { test: 10px;}");
    ck_assert_ptr_nonnull ( rofi_theme );
    RofiDistance d = (RofiDistance){ 1, ROFI_PU_EM, ROFI_HL_DASH};
    RofiPadding pi = (RofiPadding){d,d,d,d};
    RofiPadding p = rofi_theme_get_padding ( &wid, "test", pi);
    ck_assert_double_eq_tol (  p.left.distance , 10.0 , REAL_COMPARE_DELTA );
    ck_assert_int_eq(  p.left.type , ROFI_PU_PX );
    ck_assert_int_eq(  p.left.style, ROFI_HL_SOLID);
}
END_TEST
START_TEST ( test_properties_distance_px_linestyle)
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { sol: 10px solid; dash: 14px dash;}");
    ck_assert_ptr_nonnull ( rofi_theme );
    RofiDistance d = (RofiDistance){ 1, ROFI_PU_EM, ROFI_HL_DASH};
    RofiPadding pi = (RofiPadding){d,d,d,d};
    RofiPadding p = rofi_theme_get_padding ( &wid, "sol", pi);
    ck_assert_double_eq_tol (  p.left.distance , 10.0 , REAL_COMPARE_DELTA );
    ck_assert_int_eq(  p.left.type , ROFI_PU_PX );
    ck_assert_int_eq(  p.left.style, ROFI_HL_SOLID);
    p = rofi_theme_get_padding ( &wid, "dash", pi);
    ck_assert_double_eq_tol (  p.left.distance , 14.0 , REAL_COMPARE_DELTA );
    ck_assert_int_eq(  p.left.type , ROFI_PU_PX );
    ck_assert_int_eq(  p.left.style, ROFI_HL_DASH);
}
END_TEST
START_TEST ( test_properties_distance_percent)
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { test: 10%;}");
    ck_assert_ptr_nonnull ( rofi_theme );
    RofiDistance d = (RofiDistance){ 1, ROFI_PU_EM, ROFI_HL_DASH};
    RofiPadding pi = (RofiPadding){d,d,d,d};
    RofiPadding p = rofi_theme_get_padding ( &wid, "test", pi);
    ck_assert_double_eq_tol (  p.left.distance , 10.0 , REAL_COMPARE_DELTA );
    ck_assert_int_eq(  p.left.type , ROFI_PU_PERCENT);
    ck_assert_int_eq(  p.left.style, ROFI_HL_SOLID);
}
END_TEST
START_TEST ( test_properties_distance_percent_linestyle)
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { sol: 10% solid; dash: 10% dash;}");
    ck_assert_ptr_nonnull ( rofi_theme );
    RofiDistance d = (RofiDistance){ 1, ROFI_PU_EM, ROFI_HL_DASH};
    RofiPadding pi = (RofiPadding){d,d,d,d};
    RofiPadding p = rofi_theme_get_padding ( &wid, "sol", pi);
    ck_assert_double_eq_tol (  p.left.distance , 10.0 , REAL_COMPARE_DELTA );
    ck_assert_int_eq(  p.left.type , ROFI_PU_PERCENT);
    ck_assert_int_eq(  p.left.style, ROFI_HL_SOLID);
    p = rofi_theme_get_padding ( &wid, "dash", pi);
    ck_assert_double_eq_tol (  p.left.distance , 10 , REAL_COMPARE_DELTA );
    ck_assert_int_eq(  p.left.type , ROFI_PU_PERCENT);
    ck_assert_int_eq(  p.left.style, ROFI_HL_DASH);
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
    rofi_theme_parse_string ( "* { none: none; bold: bold; underline: underline; italic: italic; st: italic strikethrough;}");
    RofiHighlightColorStyle th = { ROFI_HL_BOLD, {0.0,0.0,0.0,0.0}};
    th = rofi_theme_get_highlight ( &wid, "none",      th);
    ck_assert_int_eq ( th.style , ROFI_HL_NONE );
    th = rofi_theme_get_highlight ( &wid, "underline", th);
    ck_assert_int_eq ( th.style , ROFI_HL_UNDERLINE);
    th = rofi_theme_get_highlight ( &wid, "italic",    th);
    ck_assert_int_eq ( th.style , ROFI_HL_ITALIC);
    th = rofi_theme_get_highlight ( &wid, "bold",      th);
    ck_assert_int_eq ( th.style , ROFI_HL_BOLD);
    th = rofi_theme_get_highlight ( &wid, "st",      th);
    ck_assert_int_eq ( th.style , ROFI_HL_ITALIC|ROFI_HL_STRIKETHROUGH);
}
END_TEST
START_TEST ( test_properties_style2 )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;

    rofi_theme_parse_string ( "* { boldu: bold underline ; boldi: bold italic; underlinei: underline italic; italicu: italic underline;}");
    RofiHighlightColorStyle th = { ROFI_HL_BOLD, {0.0,0.0,0.0,0.0}};
    th = rofi_theme_get_highlight ( &wid, "boldu", th);
    ck_assert_int_eq ( th.style , (ROFI_HL_UNDERLINE|ROFI_HL_BOLD));
    th = rofi_theme_get_highlight ( &wid, "boldi", th);
    ck_assert_int_eq ( th.style , (ROFI_HL_ITALIC|ROFI_HL_BOLD));
    th = rofi_theme_get_highlight ( &wid, "underlinei", th);
    ck_assert_int_eq ( th.style , (ROFI_HL_ITALIC|ROFI_HL_UNDERLINE));
    th = rofi_theme_get_highlight ( &wid, "italicu", th);
    ck_assert_int_eq ( th.style , (ROFI_HL_ITALIC|ROFI_HL_UNDERLINE));
}
END_TEST
START_TEST ( test_properties_style_color )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { comb: bold #123; }");
    RofiHighlightColorStyle th = { ROFI_HL_BOLD, {0.0,0.0,0.0,0.0}};
    th = rofi_theme_get_highlight ( &wid, "comb", th);
    ck_assert_int_eq ( th.style , (ROFI_HL_BOLD|ROFI_HL_COLOR));
    ck_assert_double_eq_tol ( th.color.red , (1/15.0), REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( th.color.green , (2/15.0), REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( th.color.blue , (3/15.0), REAL_COMPARE_DELTA );
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
    ck_assert_double_eq_tol ( p->value.color.red  , 1 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , REAL_COMPARE_DELTA );
    p   = rofi_theme_find_property ( twid, P_COLOR, "green", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.red  , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 1 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , REAL_COMPARE_DELTA );
    p   = rofi_theme_find_property ( twid, P_COLOR, "blue", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.red  , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 1 , REAL_COMPARE_DELTA );
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
    ck_assert_double_eq_tol ( p->value.color.red  , 1 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , REAL_COMPARE_DELTA );
    p   = rofi_theme_find_property ( twid, P_COLOR, "green", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.red  , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 1 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , REAL_COMPARE_DELTA );
    p   = rofi_theme_find_property ( twid, P_COLOR, "blue", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.red  , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 1 , REAL_COMPARE_DELTA );
}
END_TEST

START_TEST ( test_properties_color_h4 )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { red: #F003; green: #0F02; blue: #00F1; }");
    ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );
    Property    *p   = rofi_theme_find_property ( twid, P_COLOR, "red", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 0.2 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 1 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , REAL_COMPARE_DELTA );
    p   = rofi_theme_find_property ( twid, P_COLOR, "green", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 1/7.5 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 1 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , REAL_COMPARE_DELTA );
    p   = rofi_theme_find_property ( twid, P_COLOR, "blue", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 1/15.0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 1 , REAL_COMPARE_DELTA );
}
END_TEST
START_TEST ( test_properties_color_h8 )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { red: #FF000033; green: #00FF0022; blue: #0000FF11; }");
    ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );
    Property    *p   = rofi_theme_find_property ( twid, P_COLOR, "red", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 0.2 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 1 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , REAL_COMPARE_DELTA );
    p   = rofi_theme_find_property ( twid, P_COLOR, "green", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 1/7.5 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 1 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , REAL_COMPARE_DELTA );
    p   = rofi_theme_find_property ( twid, P_COLOR, "blue", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 1/15.0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 1 , REAL_COMPARE_DELTA );
}
END_TEST
START_TEST ( test_properties_color_rgb )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { red: rgb(100%,0%,0%); green: rgb(0%,100%,0%); blue: rgb(0%,0%,100%); }");
    ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );
    Property    *p   = rofi_theme_find_property ( twid, P_COLOR, "red", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.red  , 1 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , REAL_COMPARE_DELTA );
    p   = rofi_theme_find_property ( twid, P_COLOR, "green", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.red  , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 1 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , REAL_COMPARE_DELTA );
    p   = rofi_theme_find_property ( twid, P_COLOR, "blue", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.red  , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 1 , REAL_COMPARE_DELTA );
}
END_TEST
START_TEST ( test_properties_color_rgba_p )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { red: rgba(100%,0%,0%,0.3); green: rgba(0%,100%,0%,0.2); blue: rgba(0%,0%,100%,0.7); }");
    ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );
    Property    *p   = rofi_theme_find_property ( twid, P_COLOR, "red", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 0.3 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 1 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , REAL_COMPARE_DELTA );
    p   = rofi_theme_find_property ( twid, P_COLOR, "green", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 0.2 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 1 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , REAL_COMPARE_DELTA );
    p   = rofi_theme_find_property ( twid, P_COLOR, "blue", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 0.7 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 1 , REAL_COMPARE_DELTA );
}
END_TEST
START_TEST ( test_properties_color_rgba_percent_p )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { red: rgba(100%,0%,0%,30%); green: rgba(0%,100%,0%,20%); blue: rgba(0% 0% 100%/70.0%); }");
    ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );
    Property *p   = rofi_theme_find_property ( twid, P_COLOR, "red", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 0.3 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 1 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , REAL_COMPARE_DELTA );
    p   = rofi_theme_find_property ( twid, P_COLOR, "green", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 0.2 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 1 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , REAL_COMPARE_DELTA );
    p   = rofi_theme_find_property ( twid, P_COLOR, "blue", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 0.7 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 1 , REAL_COMPARE_DELTA );
}
END_TEST
START_TEST ( test_properties_color_rgb_p )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { red: rgb(255,0,0); green: rgb(0,255,0); blue: rgb(0,0,255); }");
    ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );
    Property    *p   = rofi_theme_find_property ( twid, P_COLOR, "red", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.red  , 1 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , REAL_COMPARE_DELTA );
    p   = rofi_theme_find_property ( twid, P_COLOR, "green", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.red  , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 1 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , REAL_COMPARE_DELTA );
    p   = rofi_theme_find_property ( twid, P_COLOR, "blue", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.red  , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 1 , REAL_COMPARE_DELTA );
}
END_TEST
START_TEST ( test_properties_color_rgba )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { red: rgba(255,0,0,0.3); green: rgba(0,255,0,0.2); blue: rgba(0 0 255 /0.7); }");
    ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );
    Property    *p   = rofi_theme_find_property ( twid, P_COLOR, "red", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 0.3 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 1 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , REAL_COMPARE_DELTA );
    p   = rofi_theme_find_property ( twid, P_COLOR, "green", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 0.2 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 1 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , REAL_COMPARE_DELTA );
    p   = rofi_theme_find_property ( twid, P_COLOR, "blue", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 0.7 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 1 , REAL_COMPARE_DELTA );
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
    ck_assert_double_eq_tol ( p->value.color.alpha , 0.3 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 1 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , REAL_COMPARE_DELTA );
    p   = rofi_theme_find_property ( twid, P_COLOR, "green", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 0.2 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 1 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , REAL_COMPARE_DELTA );
    p   = rofi_theme_find_property ( twid, P_COLOR, "blue", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 0.7 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 1 , REAL_COMPARE_DELTA );
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
    ck_assert_double_eq_tol ( p->value.color.alpha , 0.2 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 1 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , REAL_COMPARE_DELTA );
    p   = rofi_theme_find_property ( twid, P_COLOR, "green", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 1/7.5 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 1 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , REAL_COMPARE_DELTA );
    p   = rofi_theme_find_property ( twid, P_COLOR, "blue", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 1/15.0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.green , 0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.blue , 1 , REAL_COMPARE_DELTA );
}
END_TEST
START_TEST ( test_properties_color_hsl )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { test1: hsl(127,40%,66.66666%); test2: hsl(0, 100%, 50%); testa: hsl(127,40%, 66.66666%, 30%);}");
    ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );

    Property *p   = rofi_theme_find_property ( twid, P_COLOR, "test1", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 1.0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 0x88/255.0 , 0.004);
    ck_assert_double_eq_tol ( p->value.color.green, 0xcd/255.0, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue , 0x90/255.0 , 0.004);
    p   = rofi_theme_find_property ( twid, P_COLOR, "test2", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 1.0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 1 , 0.004);
    ck_assert_double_eq_tol ( p->value.color.green , 0, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , 0.004);
    p   = rofi_theme_find_property ( twid, P_COLOR, "testa", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 0.3 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 0x88/255.0 ,0.004);
    ck_assert_double_eq_tol ( p->value.color.green ,0xcd/255.0, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue , 0x90/255.0 ,0.004);
}
END_TEST
START_TEST ( test_properties_color_hsla )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { test1: hsla(127,40%,66.66666%, 40%); test2: hsla(0, 100%, 50%,55%); }");
    ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );

    Property *p   = rofi_theme_find_property ( twid, P_COLOR, "test1", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 0.4 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 0x88/255.0 , 0.004);
    ck_assert_double_eq_tol ( p->value.color.green , 0xcd/255.0, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue , 0x90/255.0 , 0.004);
    p   = rofi_theme_find_property ( twid, P_COLOR, "test2", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 0.55 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 1 , 0.004);
    ck_assert_double_eq_tol ( p->value.color.green , 0, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , 0.004);
}
END_TEST
START_TEST ( test_properties_color_hsl_ws )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { test1: hsl(127 40% 66.66666%); test2: hsl(0  100%  50%); testa: hsl(127 40%  66.66666%  / 30%);}");
    ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );

    Property *p   = rofi_theme_find_property ( twid, P_COLOR, "test1", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 1.0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 0x88/255.0 , 0.004);
    ck_assert_double_eq_tol ( p->value.color.green, 0xcd/255.0, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue , 0x90/255.0 , 0.004);
    p   = rofi_theme_find_property ( twid, P_COLOR, "test2", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 1.0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 1 , 0.004);
    ck_assert_double_eq_tol ( p->value.color.green , 0, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , 0.004);
    p   = rofi_theme_find_property ( twid, P_COLOR, "testa", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 0.3 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 0x88/255.0 ,0.004);
    ck_assert_double_eq_tol ( p->value.color.green ,0xcd/255.0, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue , 0x90/255.0 ,0.004);
}
END_TEST
START_TEST ( test_properties_color_hsla_ws )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { test1: hsla(127 40% 66.66666% / 0.3); test2: hsla(0  100%  50%/ 55%); }");
    ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );

    Property *p   = rofi_theme_find_property ( twid, P_COLOR, "test1", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 0.3 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 0x88/255.0 , 0.004);
    ck_assert_double_eq_tol ( p->value.color.green , 0xcd/255.0, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue , 0x90/255.0 , 0.004);
    p   = rofi_theme_find_property ( twid, P_COLOR, "test2", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 0.55 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 1 , 0.004);
    ck_assert_double_eq_tol ( p->value.color.green , 0, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , 0.004);
}
END_TEST
START_TEST ( test_properties_color_hwb )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { test1: hwb(190,65%,0%); test2: hwb(265, 31%, 29%); testa: hwb(265, 31%, 29%, 40%); }");
    ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );

    Property *p   = rofi_theme_find_property ( twid, P_COLOR, "test2", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 1.0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  ,  0x7a/255.0 , 0.004);
    ck_assert_double_eq_tol ( p->value.color.green , 0x4f/255.0, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue ,  0xb5/255.0 , 0.004);
    p   = rofi_theme_find_property ( twid, P_COLOR, "test1", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 1.0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 166/255.0, 0.004);
    ck_assert_double_eq_tol ( p->value.color.green ,240/255.0, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue , 255/255.0 , 0.004);
    p   = rofi_theme_find_property ( twid, P_COLOR, "testa", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 0.4 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  ,  0x7a/255.0 , 0.004);
    ck_assert_double_eq_tol ( p->value.color.green , 0x4f/255.0, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue ,  0xb5/255.0 , 0.004);
}
END_TEST
START_TEST ( test_properties_color_hwb_ws )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { test1: hwb(190 deg 65 %0%); test2: hwb(295 grad 31% 29%);testa: hwb(0.736 turn 31% 29% / 40%); rada: hwb(0.2 rad 30% 30%/40%); }");
    ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );

    Property *p   = rofi_theme_find_property ( twid, P_COLOR, "test2", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 1.0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  ,  0x7a/255.0 , 0.004);
    ck_assert_double_eq_tol ( p->value.color.green , 0x4f/255.0, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue ,  0xb5/255.0 , 0.004);
    p   = rofi_theme_find_property ( twid, P_COLOR, "test1", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 1.0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 166/255.0, 0.004);
    ck_assert_double_eq_tol ( p->value.color.green ,240/255.0, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue , 255/255.0 , 0.004);
    p   = rofi_theme_find_property ( twid, P_COLOR, "testa", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 0.4 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  ,  0x7a/255.0 , 0.004);
    ck_assert_double_eq_tol ( p->value.color.green , 0x4f/255.0, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue ,  0xb5/255.0 , 0.004);
    p   = rofi_theme_find_property ( twid, P_COLOR, "rada", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 0.4 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  ,  0.7 , 0.004);
    ck_assert_double_eq_tol ( p->value.color.green , 0.376, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue ,  0.3 , 0.004);
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
    ck_assert_double_eq_tol ( p->value.color.alpha , 1.0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 0x96/255.0 , 0.004);
    ck_assert_double_eq_tol ( p->value.color.green , 1.0, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue , 0.0 , 0.004);
    p   = rofi_theme_find_property ( twid, P_COLOR, "test2", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 1.0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 1 , 0.004);
    ck_assert_double_eq_tol ( p->value.color.green , 0, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , 0.004);
}
END_TEST
START_TEST ( test_properties_color_cmyk_ws )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { test1: cmyk ( 41% 0% 100% 0%); test2: cmyk ( 0 1.0 1.0 0);}");
    ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );

    Property *p   = rofi_theme_find_property ( twid, P_COLOR, "test1", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 1.0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 0x96/255.0 , 0.004);
    ck_assert_double_eq_tol ( p->value.color.green , 1.0, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue , 0.0 , 0.004);
    p   = rofi_theme_find_property ( twid, P_COLOR, "test2", FALSE );
    ck_assert_ptr_nonnull ( p );
    ck_assert_double_eq_tol ( p->value.color.alpha , 1.0 , REAL_COMPARE_DELTA );
    ck_assert_double_eq_tol ( p->value.color.red  , 1 , 0.004);
    ck_assert_double_eq_tol ( p->value.color.green , 0, 0.004 );
    ck_assert_double_eq_tol ( p->value.color.blue , 0 , 0.004);
}
END_TEST
START_TEST ( test_properties_color_names )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    for ( unsigned int iter = 0; iter < num_CSSColors; iter++ ) {
        char * str = g_strdup_printf("* { color: %s;}", CSSColors[iter].name);
        rofi_theme_parse_string(str);
        ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );
        Property *p   = rofi_theme_find_property ( twid, P_COLOR, "text-color", FALSE );
        ck_assert_ptr_nonnull ( p );
        ck_assert_double_eq_tol ( p->value.color.alpha , 1.0 , REAL_COMPARE_DELTA );
        ck_assert_double_eq_tol ( p->value.color.red  , CSSColors[iter].r/255.0, 0.004);
        ck_assert_double_eq_tol ( p->value.color.green, CSSColors[iter].g/255.0, 0.004 );
        ck_assert_double_eq_tol ( p->value.color.blue , CSSColors[iter].b/255.0, 0.004);

        g_free ( str );
    }
    {
        rofi_theme_parse_string("* {color: transparent;}");
        ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );
        Property *p   = rofi_theme_find_property ( twid, P_COLOR, "text-color", FALSE );
        ck_assert_ptr_nonnull ( p );
        ck_assert_double_eq_tol ( p->value.color.alpha , 0.0 , REAL_COMPARE_DELTA );
        ck_assert_double_eq_tol ( p->value.color.red  , 0.0, 0.004);
        ck_assert_double_eq_tol ( p->value.color.green, 0.0, 0.004 );
        ck_assert_double_eq_tol ( p->value.color.blue , 0.0, 0.004);
    }
}
END_TEST
START_TEST ( test_properties_color_names_alpha )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    for ( unsigned int iter = 0; iter < num_CSSColors; iter++ ) {
        char * str = g_strdup_printf("* { color: %s / %d %%;}", CSSColors[iter].name, iter%101);
        rofi_theme_parse_string(str);
        ThemeWidget *twid = rofi_theme_find_widget ( wid.name, wid.state, FALSE );
        Property *p   = rofi_theme_find_property ( twid, P_COLOR, "text-color", FALSE );
        ck_assert_ptr_nonnull ( p );
        ck_assert_double_eq_tol ( p->value.color.alpha , (iter%101)/100.0, REAL_COMPARE_DELTA);
        ck_assert_double_eq_tol ( p->value.color.red  , CSSColors[iter].r/255.0, 0.004);
        ck_assert_double_eq_tol ( p->value.color.green, CSSColors[iter].g/255.0, 0.004 );
        ck_assert_double_eq_tol ( p->value.color.blue , CSSColors[iter].b/255.0, 0.004);

        g_free ( str );
    }
}
END_TEST
START_TEST ( test_properties_padding_2 )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { test: 10px 20px;}");
    RofiDistance d = (RofiDistance){ 1, ROFI_PU_PX, ROFI_HL_SOLID};
    RofiPadding pi = (RofiPadding){d,d,d,d};
    RofiPadding p = rofi_theme_get_padding ( &wid, "test", pi);
    ck_assert_double_eq_tol (  p.left.distance , 20, REAL_COMPARE_DELTA );
    ck_assert_int_eq (  p.left.type , ROFI_PU_PX );
    ck_assert_double_eq_tol (  p.right.distance , 20, REAL_COMPARE_DELTA  );
    ck_assert_int_eq (  p.right.type , ROFI_PU_PX );
    ck_assert_double_eq_tol (  p.top.distance , 10, REAL_COMPARE_DELTA);
    ck_assert_int_eq (  p.top.type , ROFI_PU_PX );
    ck_assert_double_eq_tol (  p.bottom.distance , 10, REAL_COMPARE_DELTA );
    ck_assert_int_eq (  p.bottom.type , ROFI_PU_PX );

}
END_TEST
START_TEST ( test_properties_padding_3 )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { test: 10px 30px 20px;}");
    RofiDistance d = (RofiDistance){ 1, ROFI_PU_PX, ROFI_HL_SOLID};
    RofiPadding pi = (RofiPadding){d,d,d,d};
    RofiPadding p = rofi_theme_get_padding ( &wid, "test", pi);
    ck_assert_double_eq_tol (  p.left.distance , 30, REAL_COMPARE_DELTA);
    ck_assert_int_eq (  p.left.type , ROFI_PU_PX );
    ck_assert_double_eq_tol (  p.right.distance , 30, REAL_COMPARE_DELTA );
    ck_assert_int_eq (  p.right.type , ROFI_PU_PX );
    ck_assert_double_eq_tol (  p.top.distance , 10, REAL_COMPARE_DELTA );
    ck_assert_int_eq (  p.top.type , ROFI_PU_PX );
    ck_assert_double_eq_tol (  p.bottom.distance , 20, REAL_COMPARE_DELTA );
    ck_assert_int_eq (  p.bottom.type , ROFI_PU_PX );

}
END_TEST
START_TEST ( test_properties_padding_4 )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { test: 10px 30px 20px 40px;}");
    RofiDistance d = (RofiDistance){ 1, ROFI_PU_PX, ROFI_HL_SOLID};
    RofiPadding pi = (RofiPadding){d,d,d,d};
    RofiPadding p = rofi_theme_get_padding ( &wid, "test", pi);
    ck_assert_double_eq_tol (  p.left.distance , 40 , REAL_COMPARE_DELTA );
    ck_assert_int_eq (  p.left.type , ROFI_PU_PX );
    ck_assert_double_eq_tol (  p.right.distance , 30 , REAL_COMPARE_DELTA );
    ck_assert_int_eq (  p.right.type , ROFI_PU_PX );
    ck_assert_double_eq_tol (  p.top.distance , 10 , REAL_COMPARE_DELTA );
    ck_assert_int_eq (  p.top.type , ROFI_PU_PX );
    ck_assert_double_eq_tol (  p.bottom.distance , 20 , REAL_COMPARE_DELTA );
    ck_assert_int_eq (  p.bottom.type , ROFI_PU_PX );

}
END_TEST

START_TEST ( test_properties_string_escape )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { font: \"aap\" noot\" mies \";\ntest: \"'123.432'\"; }");

    const char *str= rofi_theme_get_string ( &wid, "font", NULL );
    ck_assert_ptr_nonnull ( str );
    ck_assert_int_eq ( g_utf8_collate ( str, "aap\" noot\" mies " ) , 0 );
    const char *str2= rofi_theme_get_string ( &wid, "test", NULL );
    ck_assert_ptr_nonnull ( str2 );
    ck_assert_int_eq ( g_utf8_collate ( str2, "'123.432'" ) , 0 );
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
    ck_assert_double_eq_tol ( rofi_theme_get_double ( &wid, "test", 0.0) , 123.432 , REAL_COMPARE_DELTA );
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


START_TEST  ( test_properties_orientation )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { vert: vertical; hori: horizontal; }");
    ck_assert_int_eq ( rofi_theme_get_orientation( &wid, "vert", ROFI_ORIENTATION_HORIZONTAL) , ROFI_ORIENTATION_VERTICAL);
    ck_assert_int_eq ( rofi_theme_get_orientation( &wid, "hori", ROFI_ORIENTATION_VERTICAL) , ROFI_ORIENTATION_HORIZONTAL);
    // default propagation
    ck_assert_int_eq ( rofi_theme_get_orientation( &wid, "notfo", ROFI_ORIENTATION_HORIZONTAL) , ROFI_ORIENTATION_HORIZONTAL);
    ck_assert_int_eq ( rofi_theme_get_orientation( &wid, "notfo", ROFI_ORIENTATION_VERTICAL) , ROFI_ORIENTATION_VERTICAL);

}
END_TEST
START_TEST  ( test_properties_orientation_case )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "* { vert: Vertical; hori: HoriZonTal;}");
    ck_assert_int_eq ( rofi_theme_get_orientation( &wid, "vert", ROFI_ORIENTATION_HORIZONTAL) , ROFI_ORIENTATION_VERTICAL);
    ck_assert_int_eq ( rofi_theme_get_orientation( &wid, "hori", ROFI_ORIENTATION_VERTICAL) , ROFI_ORIENTATION_HORIZONTAL);

}
END_TEST
START_TEST  ( test_properties_list )
{
    widget wid;
    wid.name = "blaat";
    wid.state = NULL;
    rofi_theme_parse_string ( "#blaat { liste: [];  list1: [ one ]; list2: [ one, two ];}");
    GList *list = rofi_theme_get_list ( &wid, "liste", NULL );
    ck_assert_ptr_null ( list );
    list = rofi_theme_get_list ( &wid, "list1", NULL );
    ck_assert_ptr_nonnull ( list );
    ck_assert_str_eq ( (char *)list->data, "one" );
    g_list_free_full ( list, (GDestroyNotify)g_free);
    list = rofi_theme_get_list ( &wid, "list2", NULL );
    ck_assert_ptr_nonnull ( list );
    ck_assert_int_eq ( g_list_length ( list ), 2 );
    ck_assert_str_eq ( (char *)list->data, "one" );
    ck_assert_str_eq ( (char *)list->next->data, "two" );
    g_list_free_full ( list, (GDestroyNotify)g_free);

    list = rofi_theme_get_list ( &wid, "blaat", "aap,noot,mies");
    ck_assert_ptr_nonnull ( list );
    ck_assert_int_eq ( g_list_length ( list ), 3 );
    ck_assert_str_eq ( (char *)list->data, "aap" );
    ck_assert_str_eq ( (char *)list->next->data, "noot" );
    ck_assert_str_eq ( (char *)list->next->next->data, "mies" );
    g_list_free_full ( list, (GDestroyNotify)g_free);
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
    //ck_assert_ptr_null ( rofi_theme->widgets );
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
    //ck_assert_ptr_null ( rofi_theme->widgets );
    ck_assert_ptr_null ( rofi_theme->properties );
    ck_assert_ptr_null ( rofi_theme->parent );
    ck_assert_str_eq ( rofi_theme->name, "Root" );
}
END_TEST

START_TEST ( test_core_properties_error )
{
    rofi_theme_parse_string ( " * { test: cmky(a,e,3); }");
    const char *errstr = "<big><b>Error while parsing theme:</b></big> <i> * { test: cmky(a,e,3); }</i>\n"\
    "	Parser error: <span size=\"smaller\" style=\"italic\">syntax error, unexpected invalid property value</span>\n"\
    "	Location:     line 1 column 11 to line 1 column 23\n";
    ck_assert_int_eq ( error, 1);
    ck_assert_str_eq ( error_msg->str, errstr );
    g_string_free ( error_msg, TRUE);
    error_msg = NULL;
    error = 0;

    const char *errstr2 = "<big><b>Error while parsing theme:</b></big> <i></i>\n"\
                           "	Parser error: <span size=\"smaller\" style=\"italic\">Value out of range: \n"\
                           "		Value: X = 500.00;\n"\
                           "		Range: 0.00 &lt;= X &lt;= 360.00.</span>\n"\
                           "	Location:     line 0 column 15 to line 0 column 18\n";
    rofi_theme_parse_string ( " * { test: hsl(500, 100% 10% ); }");
    ck_assert_int_eq ( error, 1);
    ck_assert_str_eq ( error_msg->str, errstr2 );
    g_string_free ( error_msg, TRUE);
    error_msg = NULL;
    error = 0;

}
END_TEST

START_TEST ( test_import_error )
{

    rofi_theme_parse_string("@import \"/non-existing-file.rasi\"");

    const char *errstr =
        "Failed to open theme: <i>/non-existing-file.rasi</i>\n"\
        "Error: <b>No such file or directory</b>";
    ck_assert_int_eq ( error, 1);
    ck_assert_str_eq ( error_msg->str, errstr );
    g_string_free ( error_msg, TRUE);
    error_msg = NULL;
    error = 0;
}
END_TEST

START_TEST ( test_prepare_path )
{
    char *current_dir = g_get_current_dir ();
    ck_assert_ptr_nonnull ( current_dir );
    char *f = rofi_theme_parse_prepare_file ( "../", NULL );
    ck_assert_ptr_nonnull ( f );
    ck_assert_int_eq ( *f, '/');
    ck_assert_str_ne ( f, current_dir);
    ck_assert ( g_str_has_prefix( current_dir, f ) == TRUE );
    g_free(f);

    f = rofi_theme_parse_prepare_file ( "../", "/tmp/" );
    ck_assert_ptr_nonnull ( f );
    ck_assert_str_eq ( f, "/");
    g_free ( f );

    f = rofi_theme_parse_prepare_file ( "/tmp/test.rasi" , "/random/");
    ck_assert_ptr_nonnull ( f );
    ck_assert_str_eq ( f, "/tmp/test.rasi" );
    g_free ( f  );

    g_free ( current_dir );
}
END_TEST


START_TEST(test_properties_types_names)
{
    ck_assert_str_eq ( PropertyTypeName[P_INTEGER],     "Integer");
    ck_assert_str_eq ( PropertyTypeName[P_DOUBLE],      "Double");
    ck_assert_str_eq ( PropertyTypeName[P_STRING],      "String");
    ck_assert_str_eq ( PropertyTypeName[P_BOOLEAN],     "Boolean");
    ck_assert_str_eq ( PropertyTypeName[P_COLOR],       "Color");
    ck_assert_str_eq ( PropertyTypeName[P_PADDING],     "Padding");
    ck_assert_str_eq ( PropertyTypeName[P_LINK],        "Reference");
    ck_assert_str_eq ( PropertyTypeName[P_POSITION],    "Position");
    ck_assert_str_eq ( PropertyTypeName[P_HIGHLIGHT],   "Highlight");
    ck_assert_str_eq ( PropertyTypeName[P_LIST],        "List");
    ck_assert_str_eq ( PropertyTypeName[P_ORIENTATION], "Orientation");

}
END_TEST

static Suite * theme_parser_suite (void)
{
    Suite *s;

    s = suite_create("Theme");

    /* Core test case */
    {
        TCase *tc_core = tcase_create("Core");
        tcase_add_checked_fixture(tc_core, theme_parser_setup, theme_parser_teardown);
        tcase_add_test(tc_core, test_properties_types_names);
        tcase_add_test(tc_core, test_core_empty_string);
        tcase_add_test(tc_core, test_core_empty_global_section);
        tcase_add_test(tc_core, test_core_empty_section);
        tcase_add_test(tc_core, test_core_error_root );
        tcase_add_test(tc_core, test_core_comments );
        tcase_add_test(tc_core, test_core_newline );
        tcase_add_test(tc_core, test_core_properties_error );
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
        tcase_add_test ( tc_prop_color, test_properties_color_h4);
        tcase_add_test ( tc_prop_color, test_properties_color_h6);
        tcase_add_test ( tc_prop_color, test_properties_color_h8);
        tcase_add_test ( tc_prop_color, test_properties_color_rgb);
        tcase_add_test ( tc_prop_color, test_properties_color_rgba);
        tcase_add_test ( tc_prop_color, test_properties_color_rgba_percent);
        tcase_add_test ( tc_prop_color, test_properties_color_rgb_p);
        tcase_add_test ( tc_prop_color, test_properties_color_rgba_p);
        tcase_add_test ( tc_prop_color, test_properties_color_rgba_percent_p);
        tcase_add_test ( tc_prop_color, test_properties_color_argb);
        tcase_add_test ( tc_prop_color, test_properties_color_hsl);
        tcase_add_test ( tc_prop_color, test_properties_color_hsla);
        tcase_add_test ( tc_prop_color, test_properties_color_hsl_ws);
        tcase_add_test ( tc_prop_color, test_properties_color_hsla_ws);
        tcase_add_test ( tc_prop_color, test_properties_color_hwb);
        tcase_add_test ( tc_prop_color, test_properties_color_hwb_ws);
        tcase_add_test ( tc_prop_color, test_properties_color_cmyk);
        tcase_add_test ( tc_prop_color, test_properties_color_cmyk_ws);
        tcase_add_test ( tc_prop_color, test_properties_color_names);
        tcase_add_test ( tc_prop_color, test_properties_color_names_alpha);
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
        tcase_add_test ( tc_prop_string, test_properties_string_escape);
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
        TCase *tc_prop_orientation = tcase_create("Propertiesorientation");
        tcase_add_checked_fixture(tc_prop_orientation, theme_parser_setup, theme_parser_teardown);
        tcase_add_test ( tc_prop_orientation, test_properties_orientation);
        tcase_add_test ( tc_prop_orientation, test_properties_orientation_case );
        suite_add_tcase(s, tc_prop_orientation );
    }
    {
        TCase *tc_prop_list = tcase_create("Propertieslist");
        tcase_add_checked_fixture(tc_prop_list, theme_parser_setup, theme_parser_teardown);
        tcase_add_test ( tc_prop_list, test_properties_list);
        suite_add_tcase(s, tc_prop_list );
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
        tcase_add_test ( tc_prop_import, test_import_error);
        suite_add_tcase(s, tc_prop_import );
    }
    {
        TCase *tc_prepare_path = tcase_create("prepare_path");
        tcase_add_test ( tc_prepare_path, test_prepare_path);
        suite_add_tcase(s, tc_prepare_path );
    }
    return s;
}

int main ( int argc, char ** argv )
{
    cmd_set_arguments ( argc, argv );

    if ( setlocale ( LC_ALL, "C" ) == NULL ) {
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
