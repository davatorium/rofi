/*
 * rofi
 *
 * MIT/X11 License
 * Copyright 2013-2017 Qball Cow <qball@gmpclient.org>
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

%define api.pure
%define parse.error verbose
%locations
%glr-parser
%skeleton "glr.c"
%debug
%parse-param {const char *what}
%code requires {
#include "theme.h"
#include "xrmoptions.h"

typedef struct YYLTYPE {
  int first_line;
  int first_column;
  int last_line;
  int last_column;
  char *filename;
} YYLTYPE;
# define YYLTYPE_IS_DECLARED 1 /* alert the parser that we have our own definition */

# define YYLLOC_DEFAULT(Current, Rhs, N)                               \
    do                                                                 \
      if (N)                                                           \
        {                                                              \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;       \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;     \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;        \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;      \
          (Current).filename     = YYRHSLOC (Rhs, 1).filename;         \
        }                                                              \
      else                                                             \
        { /* empty RHS */                                              \
          (Current).first_line   = (Current).last_line   =             \
            YYRHSLOC (Rhs, 0).last_line;                               \
          (Current).first_column = (Current).last_column =             \
            YYRHSLOC (Rhs, 0).last_column;                             \
          (Current).filename  = NULL;                        /* new */ \
        }                                                              \
    while (0)
}
%{
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#include "theme-parser.h"
ThemeWidget *rofi_theme = NULL;
void yyerror(YYLTYPE *yylloc, const char *what, const char* s);
int yylex (YYSTYPE *, YYLTYPE *);


static int check_in_range ( double index, double low, double high, YYLTYPE *loc )
{
    if ( index > high || index < low ){
        gchar *str = g_strdup_printf("Value out of range: \n\t\tValue: X = %.2lf;\n\t\tRange: %.2lf <= X <= %.2lf.", index, low, high );
        yyerror ( loc, loc->filename, str);
        g_free(str);
        return FALSE;
    }

    return TRUE;
}

static double hue2rgb(double p, double q, double t){
    t += (t<0)?1.0:0.0;
    t -= (t>1)?1.0:0.0;
    if( t < (1/6.0) ) {
         return p + (q - p) * 6 * t;
    }
    if( t < (1/2.0) ) {
         return q;
    }
    if( t < (2/3.0) ) {
        return p + (q - p) * (2/3.0 - t) * 6;
    }
    return p;
}
static ThemeColor hsl_to_rgb ( double h, double s, double l )
{
    ThemeColor colour;

    if(s <  0.001 && s > -0.001){
        colour.red = colour.green = colour.blue = l; // achromatic
    }else{

        double q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        double p = 2 * l - q;
        colour.red   = hue2rgb(p, q, h + 1/3.0);
        colour.green = hue2rgb(p, q, h);
        colour.blue  = hue2rgb(p, q, h - 1/3.0);
    }

    return colour;
}
%}

%union {
	int           ival;
	double        fval;
    char          *sval;
    int           bval;
    WindowLocation wloc;
    ThemeColor    colorval;
    ThemeWidget   *theme;
    GList         *name_path;
    Property      *property;
    GHashTable    *property_list;
    Distance      distance;
}

%token <ival>     T_END              0  "end of file"
%token <ival>     T_ERROR            1  "error from file parser"
%token <ival>     T_ERROR_PROPERTY   2  "invalid property value"
%token <ival>     T_ERROR_SECTION    3  "invalid property name"
%token <ival>     T_ERROR_NAMESTRING 4  "invalid element name"
%token <ival>     T_ERROR_DEFAULTS   5  "invalid defaults name"
%token <ival>     T_ERROR_INCLUDE    6  "invalid import value"
%token <ival>     T_INT
%token <fval>     T_DOUBLE
%token <sval>     T_STRING
%token <sval>     N_STRING     "property name"
%token <sval>     NAME_ELEMENT "Element name"
%token <bval>     T_BOOLEAN
%token <colorval> T_COLOR
%token <sval>     T_LINK
%token <sval>     FIRST_NAME
%token T_POS_CENTER  "Center"
%token T_POS_EAST    "East"
%token T_POS_WEST    "West"
%token T_POS_NORTH   "North"
%token T_POS_SOUTH   "South"

%token T_NONE          "None"
%token T_BOLD          "Bold"
%token T_ITALIC        "Italic"
%token T_UNDERLINE     "Underline"
%token T_DASH          "Dash"
%token T_SOLID         "Solid"

%token T_UNIT_PX       "pixels"
%token T_UNIT_EM       "em"
%token T_UNIT_PERCENT  "%"

%token T_COL_ARGB
%token T_COL_RGBA
%token T_COL_RGB
%token T_COL_HSL
%token T_COL_HWB
%token T_COL_CMYK

%token PARENT_LEFT "Parent left '('"
%token PARENT_RIGHT "Parent right ')'"
%token COMMA "comma separator"
%token PERCENT "Percent sign"

%token BOPEN        "bracket open ('{')"
%token BCLOSE       "bracket close ('}')"
%token PSEP         "property separator (':')"
%token PCLOSE       "property close (';')"
%token NSEP         "Name separator (' ' or '.')"
%token NAME_PREFIX  "Element section ('# {name} { ... }')"
%token WHITESPACE   "White space"
%token PDEFAULTS    "Default settings section ( '* { ... }')"
%token CONFIGURATION "Configuration block"

%type <ival> highlight_styles
%type <ival> highlight_style
%type <ival> t_line_style
%type <ival> t_unit
%type <fval> color_val
%type <wloc> t_position
%type <wloc> t_position_ew
%type <wloc> t_position_sn
%type <sval> entry
%type <sval> pvalue
%type <theme> entries
%type <name_path> name_path
%type <property> property
%type <property_list> property_list
%type <property_list> optional_properties
%type <distance> t_distance
%type <colorval> t_color
%start entries

%%

entries:
  %empty {
        // There is always a base widget.
        if (rofi_theme == NULL ){
            $$ =  rofi_theme = g_slice_new0 ( ThemeWidget  );
            rofi_theme->name = g_strdup ( "Root" );
        }
  }
|  entries
   entry {
    }
;

entry:
NAME_PREFIX name_path BOPEN optional_properties BCLOSE
{
        ThemeWidget *widget = rofi_theme;
        for ( GList *iter = g_list_first ( $2 ); iter ; iter = g_list_next ( iter ) ) {
            widget = rofi_theme_find_or_create_name ( widget, iter->data );
        }
        g_list_foreach ( $2, (GFunc)g_free , NULL );
        g_list_free ( $2 );
        widget->set = TRUE;
        rofi_theme_widget_add_properties ( widget, $4);
}
|
    PDEFAULTS BOPEN optional_properties BCLOSE {
    rofi_theme_widget_add_properties ( rofi_theme, $3);
}
| CONFIGURATION BOPEN optional_properties BCLOSE {
    GHashTableIter iter;
    g_hash_table_iter_init ( &iter, $3 );
    gpointer key,value;
    while ( g_hash_table_iter_next ( &iter, &key, &value ) ) {
            Property *p = (Property *) value;
            config_parse_set_property ( p );
    }
    g_hash_table_destroy ( $3 );
}
;

/**
 * properties
 */
optional_properties
          : %empty { $$ = NULL; }
          | property_list { $$ = $1; }
;

property_list:
  property {
    $$ = g_hash_table_new_full ( g_str_hash, g_str_equal, NULL, (GDestroyNotify)rofi_theme_property_free );
    g_hash_table_replace ( $$, $1->name, $1 );
  }
| property_list property {
    // Old will be free'ed, and key/value will be replaced.
    g_hash_table_replace ( $$, $2->name, $2 );
  }
;

property
:   pvalue PSEP T_INT PCLOSE  {
        $$ = rofi_theme_property_create ( P_INTEGER );
        $$->name = $1;
        $$->value.i = $3;
    }
|   pvalue PSEP T_DOUBLE PCLOSE {
        $$ = rofi_theme_property_create ( P_DOUBLE );
        $$->name = $1;
        $$->value.f = $3;
    }
|   pvalue PSEP T_STRING PCLOSE {
        $$ = rofi_theme_property_create ( P_STRING );
        $$->name = $1;
        $$->value.s = $3;
    }
|   pvalue PSEP T_LINK PCLOSE {
        $$ = rofi_theme_property_create ( P_LINK );
        $$->name = $1;
        $$->value.link.name = $3;
    }
|   pvalue PSEP T_BOOLEAN PCLOSE {
        $$ = rofi_theme_property_create ( P_BOOLEAN );
        $$->name = $1;
        $$->value.b = $3;
    }
|  pvalue PSEP t_distance PCLOSE {
        $$ = rofi_theme_property_create ( P_PADDING );
        $$->name = $1;
        $$->value.padding = (Padding){ $3, $3, $3, $3 };
}
|  pvalue PSEP t_distance t_distance PCLOSE {
        $$ = rofi_theme_property_create ( P_PADDING );
        $$->name = $1;
        $$->value.padding = (Padding){ $3, $4, $3, $4 };
}
|  pvalue PSEP t_distance t_distance t_distance PCLOSE {
        $$ = rofi_theme_property_create ( P_PADDING );
        $$->name = $1;
        $$->value.padding = (Padding){ $3, $4, $5, $4 };
}
|  pvalue PSEP t_distance t_distance t_distance t_distance PCLOSE {
        $$ = rofi_theme_property_create ( P_PADDING );
        $$->name = $1;
        $$->value.padding = (Padding){ $3, $4, $5, $6 };
}
| pvalue PSEP t_position PCLOSE{
        $$ = rofi_theme_property_create ( P_POSITION );
        $$->name = $1;
        $$->value.i = $3;
}
| pvalue PSEP highlight_styles t_color PCLOSE {
        $$ = rofi_theme_property_create ( P_HIGHLIGHT );
        $$->name = $1;
        $$->value.highlight.style = $3|HL_COLOR;
        $$->value.highlight.color = $4;
}
| pvalue PSEP highlight_styles PCLOSE {
        $$ = rofi_theme_property_create ( P_HIGHLIGHT );
        $$->name = $1;
        $$->value.highlight.style = $3;
}
| pvalue PSEP t_color PCLOSE {
        $$ = rofi_theme_property_create ( P_COLOR );
        $$->name = $1;
        $$->value.color = $3;
}
;

/**
 * Position can be either center,
 * East or West, North Or South
 * Or combi of East or West and North or South
 */
t_position
: T_POS_CENTER { $$ =WL_CENTER;}
| t_position_ew
| t_position_sn
| t_position_ew t_position_sn { $$ = $1|$2;}
| t_position_sn t_position_ew { $$ = $1|$2;}
;
t_position_ew
: T_POS_EAST   { $$ = WL_EAST;}
| T_POS_WEST   { $$ = WL_WEST;}
;
t_position_sn
: T_POS_NORTH  { $$ = WL_NORTH;}
| T_POS_SOUTH  { $$ = WL_SOUTH;}
;

/**
 * Highlight style, allow mulitple styles to be combined.
 * Empty not allowed
 */
highlight_styles
: highlight_style { $$ = $1;}
| highlight_styles highlight_style { $$ = $1|$2;}
;
/** Single style. */
highlight_style
: T_NONE      { $$ = HL_NONE; }
| T_BOLD      { $$ = HL_BOLD; }
| T_UNDERLINE { $$ = HL_UNDERLINE; }
| T_ITALIC    { $$ = HL_ITALIC; }
;


t_distance
: T_INT t_unit t_line_style {
    $$.distance = (double)$1;
    $$.type     = $2;
    $$.style    = $3;
}
| T_DOUBLE t_unit t_line_style {
    $$.distance = (double)$1;
    $$.type     = $2;
    $$.style    = $3;
};

t_unit
: T_UNIT_PX      { $$ = PW_PX; }
| T_UNIT_EM      { $$ = PW_EM; }
| PERCENT        { $$ = PW_PERCENT; }
;
/******
 * Line style
 * If not set, solid.
 */
t_line_style
: %empty   { $$ = SOLID; }
| T_SOLID  { $$ = SOLID; }
| T_DASH   { $$ = DASH;  }
;

/**
 * Color formats
 */
t_color
: T_COL_RGBA PARENT_LEFT  T_INT COMMA T_INT COMMA T_INT COMMA T_DOUBLE PARENT_RIGHT {
    if ( ! check_in_range($3,0,255, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($5,0,255, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($7,0,255, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($9,0,1.00, &(@$)) ) { YYABORT; }
    $$.alpha = $9;
    $$.red   = $3/255.0;
    $$.green = $5/255.0;
    $$.blue  = $7/255.0;
}
| T_COL_RGBA PARENT_LEFT  T_INT COMMA T_INT COMMA T_INT COMMA color_val PERCENT PARENT_RIGHT {
    if ( ! check_in_range($3,0,255, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($5,0,255, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($7,0,255, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($9,0,100, &(@$)) ) { YYABORT; }
    $$.alpha = $9/100.0;
    $$.red   = $3/255.0;
    $$.green = $5/255.0;
    $$.blue  = $7/255.0;
}
| T_COL_RGB PARENT_LEFT  T_INT COMMA T_INT COMMA T_INT PARENT_RIGHT {
    if ( ! check_in_range($3,0,255, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($5,0,255, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($7,0,255, &(@$)) ) { YYABORT; }
    $$.alpha = 1.0;
    $$.red   = $3/255.0;
    $$.green = $5/255.0;
    $$.blue  = $7/255.0;
}
| T_COL_HWB PARENT_LEFT T_INT COMMA color_val PERCENT COMMA color_val PERCENT PARENT_RIGHT {
    $$.alpha = 1.0;
    if ( ! check_in_range($3,0,360, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($5,0,100, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($8,0,100, &(@$)) ) { YYABORT; }
    double h = $3/360.0;
    double w = $5/100.0;
    double b = $8/100.0;
    $$ = hsl_to_rgb ( h, 1.0, 0.5);
    $$.red   *= ( 1. - w - b );
    $$.red   += w;
    $$.green *= ( 1. - w - b );
    $$.green += w;
    $$.blue  *= ( 1. - w - b );
    $$.blue += w;
}
| T_COL_CMYK PARENT_LEFT color_val PERCENT COMMA color_val PERCENT COMMA color_val PERCENT COMMA color_val PERCENT PARENT_RIGHT {
    $$.alpha = 1.0;
    if ( ! check_in_range($3, 0,100, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($6, 0,100, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($9, 0,100, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($12,0,100, &(@$)) ) { YYABORT; }
    double  c= $3/100.0;
    double  m= $6/100.0;
    double  y= $9/100.0;
    double  k= $12/100.0;
    $$.red   = (1.0-c)*(1.0-k);
    $$.green = (1.0-m)*(1.0-k);
    $$.blue  = (1.0-y)*(1.0-k);
}
| T_COL_CMYK PARENT_LEFT color_val COMMA color_val COMMA color_val COMMA color_val PARENT_RIGHT {
    $$.alpha = 1.0;
    if ( ! check_in_range($3, 0,1.00, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($5, 0,1.00, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($7, 0,1.00, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($9, 0,1.00, &(@$)) ) { YYABORT; }
    double  c= $3;
    double  m= $5;
    double  y= $7;
    double  k= $9;
    $$.red   = (1.0-c)*(1.0-k);
    $$.green = (1.0-m)*(1.0-k);
    $$.blue  = (1.0-y)*(1.0-k);
}
| T_COL_HSL PARENT_LEFT T_INT COMMA color_val PERCENT COMMA color_val PERCENT PARENT_RIGHT {
    if ( ! check_in_range($3, 0,360, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($5, 0,100, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($8, 0,100, &(@$)) ) { YYABORT; }
    gdouble h = $3;
    gdouble s = $5;
    gdouble l = $8;
    $$ = hsl_to_rgb ( h/360.0, s/100.0, l/100.0 );
    $$.alpha = 1.0;
}
| T_COLOR {
    $$ = $1;
}
;

color_val
: T_DOUBLE { $$ = $1; }
| T_INT    { $$ = $1; }
;


pvalue: N_STRING { $$ = $1; }

name_path:
NAME_ELEMENT { $$ = g_list_append ( NULL, $1 );}
| name_path NSEP NAME_ELEMENT { $$ = g_list_append ( $1, $3);}
| name_path NSEP  { $$ = $1; }
;

%%

