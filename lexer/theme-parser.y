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
#include "css-colors.h"

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
    colour.alpha = 1.0;

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
static ThemeColor hwb_to_rgb ( double h, double w, double b)
{
    ThemeColor retv = hsl_to_rgb ( h, 1.0, 0.5);
    retv.red   *= ( 1. - w - b );
    retv.red   += w;
    retv.green *= ( 1. - w - b );
    retv.green += w;
    retv.blue  *= ( 1. - w - b );
    retv.blue += w;
    return retv;
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
    GList         *list;
    Property      *property;
    GHashTable    *property_list;
    RofiDistance      distance;
}

%token <ival>     T_END              0  "end of file"
%token <ival>     T_ERROR            1  "error from file parser"
%token <ival>     T_ERROR_PROPERTY   2  "invalid property value"
%token <ival>     T_ERROR_SECTION    3  "invalid property name"
%token <ival>     T_ERROR_NAMESTRING 4  "invalid element name"
%token <ival>     T_ERROR_DEFAULTS   5  "invalid defaults name"
%token <ival>     T_ERROR_INCLUDE    6  "invalid import value"
%token <ival>     T_ERROR_ARGB_SPEC  7  "invalid argb color. Requires 8 (not 7) elements: argb:AARRGGBB."
%token <ival>     T_INT                 "Integer number"
%token <fval>     T_DOUBLE              "Floating-point number"
%token <sval>     T_STRING              "UTF-8 encoded string"
%token <sval>     T_PROP_NAME           "property name"
%token <colorval> T_COLOR_NAME          "Color value by name"
%token <sval>     T_NAME_ELEMENT        "Element name"
%token <bval>     T_BOOLEAN             "Boolean value (true or false)"
%token <colorval> T_COLOR               "Hexidecimal color value"
%token <sval>     T_LINK                "Reference"
%token <sval>     T_ELEMENT             "Name of element"
%token T_POS_CENTER                     "Center"
%token T_POS_EAST                       "East"
%token T_POS_WEST                       "West"
%token T_POS_NORTH                      "North"
%token T_POS_SOUTH                      "South"

%token T_NONE                           "None"
%token T_BOLD                           "Bold"
%token T_ITALIC                         "Italic"
%token T_UNDERLINE                      "Underline"
%token T_STRIKETHROUGH                  "Strikethrough"
%token T_SMALLCAPS                      "Small CAPS"
%token T_DASH                           "Dash"
%token T_SOLID                          "Solid"

%token T_UNIT_PX                        "pixels"
%token T_UNIT_EM                        "em"
%token T_UNIT_CH                        "ch"
%token T_UNIT_PERCENT                   "%"

%token T_ANGLE_DEG                      "Degrees"
%token T_ANGLE_GRAD                     "Gradians"
%token T_ANGLE_RAD                      "Radians"
%token T_ANGLE_TURN                     "Turns"

%token ORIENTATION_HORI                 "Horizontal"
%token ORIENTATION_VERT                 "Vertical"

%token T_COL_RGBA                       "rgb[a] colorscheme"
%token T_COL_HSL                        "hsl colorscheme"
%token T_COL_HWB                        "hwb colorscheme"
%token T_COL_CMYK                       "cmyk colorscheme"

%token T_PARENT_LEFT                    "Parent left ('(')"
%token T_PARENT_RIGHT                   "Parent right (')')"
%token T_COMMA                          "comma separator (',')"
%token T_OPTIONAL_COMMA                          "Optional comma separator (',')"
%token T_FORWARD_SLASH                  "forward slash ('/')"
%token T_PERCENT                        "Percent sign ('%')"
%token T_LIST_OPEN                      "List open ('[')"
%token T_LIST_CLOSE                     "List close (']')"

%token T_BOPEN                          "bracket open ('{')"
%token T_BCLOSE                         "bracket close ('}')"
%token T_PSEP                           "property separator (':')"
%token T_PCLOSE                         "property close (';')"
%token T_NSEP                           "Name separator (' ' or '.')"
%token T_SSEP                           "Selector separator (',')"
%token T_NAME_PREFIX                    "Element section ('# {name} { ... }')"
%token T_WHITESPACE                     "White space"
%token T_PDEFAULTS                      "Default settings section ( '* { ... }')"
%token T_CONFIGURATION                  "Configuration block"

%token T_COLOR_TRANSPARENT              "Transparent"

%token T_INHERIT                        "Inherit"

%type <sval>           t_entry
%type <theme>          t_entry_list
%type <list>           t_entry_name_path
%type <list>           t_entry_name_path_selectors
%type <property>       t_property
%type <property_list>  t_property_list
%type <property_list>  t_property_list_optional
%type <colorval>       t_property_color
%type <fval>           t_property_color_value
%type <fval>           t_property_color_opt_alpha_c
%type <fval>           t_property_color_opt_alpha_ws
%type <fval>           t_property_color_value_unit
%type <fval>           t_property_color_value_angle
%type <sval>           t_property_name
%type <distance>       t_property_distance
%type <distance>       t_property_distance_zero
%type <ival>           t_property_unit
%type <wloc>           t_property_position
%type <wloc>           t_property_position_ew
%type <wloc>           t_property_position_sn
%type <ival>           t_property_highlight_styles
%type <ival>           t_property_highlight_style
%type <ival>           t_property_line_style
%type <list>           t_property_element_list
%type <list>           t_property_element_list_optional
%type <ival>           t_property_orientation
%type <ival>           t_name_prefix_optional
%start t_entry_list

%%

t_entry_list:
  %empty {
        // There is always a base widget.
        if (rofi_theme == NULL ){
            rofi_theme_reset();
            $$ =  rofi_theme;
        }
  }
|  t_entry_list
   t_entry {
    }
;

/**
 * Small dummy object to make the prefix optional.
 */
t_name_prefix_optional
: T_NAME_PREFIX {}
| %empty {}
;

t_entry:
t_name_prefix_optional t_entry_name_path_selectors T_BOPEN t_property_list_optional T_BCLOSE
{
    for ( GList *liter = g_list_first ( $2); liter; liter = g_list_next ( liter ) ) {
        ThemeWidget *widget = rofi_theme;
        for ( GList *iter = g_list_first ( (GList*)liter->data ); iter ; iter = g_list_next ( iter ) ) {
            widget = rofi_theme_find_or_create_name ( widget, iter->data );
        }
        g_list_foreach ( (GList*)liter->data, (GFunc)g_free , NULL );
        g_list_free ( (GList*)liter->data );
        widget->set = TRUE;
        rofi_theme_widget_add_properties ( widget, $4);
    }
    if ( $4 ) {
        g_hash_table_destroy ( $4 );
    }
    g_list_free ( $2 );
}
|
    T_PDEFAULTS T_BOPEN t_property_list_optional T_BCLOSE {
    rofi_theme_widget_add_properties ( rofi_theme, $3);
    if ( $3 ) {
        g_hash_table_destroy ( $3 );
    }
}
| T_CONFIGURATION T_BOPEN t_config_property_list_optional T_BCLOSE {
    // Dummy at this point.
}
;

t_config_property_list_optional
: %empty {}
| t_config_property_list
;

t_config_property_list
: t_config_property {
}
| t_config_property_list  t_config_property  {
};

t_config_property
: t_property {
    char *error = NULL;
    if ( config_parse_set_property ( $1, &error ) ) {
        // TODO Generate error.
        yyerror ( &(@$), @$.filename, error );
        g_free(error);
    }
}

/**
 * properties
 */
t_property_list_optional
          : %empty { $$ = NULL; }
          | t_property_list { $$ = $1; }
;

t_property_list:
  t_property {
    $$ = g_hash_table_new_full ( g_str_hash, g_str_equal, NULL, (GDestroyNotify)rofi_theme_property_free );
    g_hash_table_replace ( $$, $1->name, $1 );
  }
| t_property_list t_property {
    // Old will be free'ed, and key/value will be replaced.
    g_hash_table_replace ( $$, $2->name, $2 );
  }
;

t_property
:   t_property_name T_PSEP T_INHERIT T_PCLOSE {
        $$ = rofi_theme_property_create ( P_INHERIT );
        $$->name = $1;
    }
|   t_property_name T_PSEP T_INT T_PCLOSE  {
        $$ = rofi_theme_property_create ( P_INTEGER );
        $$->name = $1;
        $$->value.i = $3;
    }
|   t_property_name T_PSEP T_DOUBLE T_PCLOSE {
        $$ = rofi_theme_property_create ( P_DOUBLE );
        $$->name = $1;
        $$->value.f = $3;
    }
|   t_property_name T_PSEP T_STRING T_PCLOSE {
        $$ = rofi_theme_property_create ( P_STRING );
        $$->name = $1;
        $$->value.s = $3;
    }
|   t_property_name T_PSEP T_LINK T_PCLOSE {
        $$ = rofi_theme_property_create ( P_LINK );
        $$->name = $1;
        $$->value.link.name = $3;
    }
|   t_property_name T_PSEP T_BOOLEAN T_PCLOSE {
        $$ = rofi_theme_property_create ( P_BOOLEAN );
        $$->name = $1;
        $$->value.b = $3;
    }
|  t_property_name T_PSEP t_property_distance T_PCLOSE {
        $$ = rofi_theme_property_create ( P_PADDING );
        $$->name = $1;
        $$->value.padding = (RofiPadding){ $3, $3, $3, $3 };
}
|  t_property_name T_PSEP t_property_distance_zero t_property_distance_zero T_PCLOSE {
        $$ = rofi_theme_property_create ( P_PADDING );
        $$->name = $1;
        $$->value.padding = (RofiPadding){ $3, $4, $3, $4 };
}
|  t_property_name T_PSEP t_property_distance_zero t_property_distance_zero t_property_distance_zero T_PCLOSE {
        $$ = rofi_theme_property_create ( P_PADDING );
        $$->name = $1;
        $$->value.padding = (RofiPadding){ $3, $4, $5, $4 };
}
|  t_property_name T_PSEP t_property_distance_zero t_property_distance_zero t_property_distance_zero t_property_distance_zero T_PCLOSE {
        $$ = rofi_theme_property_create ( P_PADDING );
        $$->name = $1;
        $$->value.padding = (RofiPadding){ $3, $4, $5, $6 };
}
| t_property_name T_PSEP t_property_position T_PCLOSE{
        $$ = rofi_theme_property_create ( P_POSITION );
        $$->name = $1;
        $$->value.i = $3;
}
| t_property_name T_PSEP t_property_highlight_styles t_property_color T_PCLOSE {
        $$ = rofi_theme_property_create ( P_HIGHLIGHT );
        $$->name = $1;
        $$->value.highlight.style = $3|ROFI_HL_COLOR;
        $$->value.highlight.color = $4;
}
| t_property_name T_PSEP t_property_highlight_styles T_PCLOSE {
        $$ = rofi_theme_property_create ( P_HIGHLIGHT );
        $$->name = $1;
        $$->value.highlight.style = $3;
}
| t_property_name T_PSEP t_property_color T_PCLOSE {
        $$ = rofi_theme_property_create ( P_COLOR );
        $$->name = $1;
        $$->value.color = $3;
}
| t_property_name T_PSEP T_LIST_OPEN t_property_element_list_optional T_LIST_CLOSE T_PCLOSE {
        $$ = rofi_theme_property_create ( P_LIST );
        $$->name = $1;
        $$->value.list = $4;
}
| t_property_name T_PSEP t_property_orientation T_PCLOSE {
        $$ = rofi_theme_property_create ( P_ORIENTATION );
        $$->name = $1;
        $$->value.i = $3;
}
;

/** List of elements */
t_property_element_list_optional
: %empty { $$ = NULL; }
| t_property_element_list { $$ = $1; }
;

t_property_element_list
: T_ELEMENT { $$ = g_list_append ( NULL, $1); }
| t_property_element_list T_COMMA T_ELEMENT {
    $$ = g_list_append ( $1, $3 );
}
;

/**
 * Position can be either center,
 * East or West, North Or South
 * Or combi of East or West and North or South
 */
t_property_position
: T_POS_CENTER { $$ =WL_CENTER;}
| t_property_position_ew
| t_property_position_sn
| t_property_position_ew t_property_position_sn { $$ = $1|$2;}
| t_property_position_sn t_property_position_ew { $$ = $1|$2;}
;
t_property_position_ew
: T_POS_EAST   { $$ = WL_EAST;}
| T_POS_WEST   { $$ = WL_WEST;}
;
t_property_position_sn
: T_POS_NORTH  { $$ = WL_NORTH;}
| T_POS_SOUTH  { $$ = WL_SOUTH;}
;

/**
 * Highlight style, allow mulitple styles to be combined.
 * Empty not allowed
 */
t_property_highlight_styles
: t_property_highlight_style { $$ = $1;}
| t_property_highlight_styles t_property_highlight_style { $$ = $1|$2;}
;
/** Single style. */
t_property_highlight_style
: T_NONE          { $$ = ROFI_HL_NONE; }
| T_BOLD          { $$ = ROFI_HL_BOLD; }
| T_UNDERLINE     { $$ = ROFI_HL_UNDERLINE; }
| T_STRIKETHROUGH { $$ = ROFI_HL_STRIKETHROUGH; }
| T_ITALIC        { $$ = ROFI_HL_ITALIC; }
| T_SMALLCAPS     { $$ = ROFI_HL_SMALL_CAPS; }
;


t_property_distance_zero
: T_INT t_property_line_style {
    $$.distance = (double) $1;
    $$.type     = ROFI_PU_PX;
    $$.style    = $2;
}
| t_property_distance { $$ = $1;}
;
/** Distance. */
t_property_distance
/** Integer unit and line style */
: T_INT t_property_unit t_property_line_style {
    $$.distance = (double)$1;
    $$.type     = $2;
    $$.style    = $3;
}
/** Double unit and line style */
| T_DOUBLE t_property_unit t_property_line_style {
    $$.distance = (double)$1;
    $$.type     = $2;
    $$.style    = $3;
};

/** distance unit. px, em, % */
t_property_unit
: T_UNIT_PX      { $$ = ROFI_PU_PX; }
| T_UNIT_EM      { $$ = ROFI_PU_EM; }
| T_UNIT_CH      { $$ = ROFI_PU_CH; }
| T_PERCENT        { $$ = ROFI_PU_PERCENT; }
;
/******
 * Line style
 * If not set, solid.
 */
t_property_line_style
: %empty   { $$ = ROFI_HL_SOLID; }
| T_SOLID  { $$ = ROFI_HL_SOLID; }
| T_DASH   { $$ = ROFI_HL_DASH;  }
;

/**
 * Color formats
 */
t_property_color
 /** rgba ( 0-255 , 0-255, 0-255, 0-1.0 ) */
: T_COL_RGBA T_PARENT_LEFT  T_INT T_COMMA T_INT T_COMMA T_INT t_property_color_opt_alpha_c T_PARENT_RIGHT {
    if ( ! check_in_range($3,0,255, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($5,0,255, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($7,0,255, &(@$)) ) { YYABORT; }
    $$.alpha = $8;
    $$.red   = $3/255.0;
    $$.green = $5/255.0;
    $$.blue  = $7/255.0;
}
 /** rgba ( 0-255   0-255  0-255  / 0-1.0 ) */
| T_COL_RGBA T_PARENT_LEFT  T_INT  T_INT  T_INT  t_property_color_opt_alpha_ws T_PARENT_RIGHT {
    if ( ! check_in_range($3,0,255, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($4,0,255, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($5,0,255, &(@$)) ) { YYABORT; }
    $$.alpha = $6;
    $$.red   = $3/255.0;
    $$.green = $4/255.0;
    $$.blue  = $5/255.0;
}
 /** rgba ( 0-100% , 0-100%, 0-100%, 0-1.0 ) */
| T_COL_RGBA T_PARENT_LEFT  t_property_color_value T_PERCENT T_COMMA t_property_color_value T_PERCENT T_COMMA t_property_color_value T_PERCENT t_property_color_opt_alpha_c T_PARENT_RIGHT {
    if ( ! check_in_range($3,0,100, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($6,0,100, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($9,0,100, &(@$)) ) { YYABORT; }
    $$.alpha = $11; $$.red   = $3/100.0; $$.green = $6/100.0; $$.blue  = $9/100.0;
}
 /** rgba ( 0-100%   0-100%  0-100%  / 0-1.0 ) */
| T_COL_RGBA T_PARENT_LEFT  t_property_color_value T_PERCENT  t_property_color_value T_PERCENT  t_property_color_value T_PERCENT  t_property_color_opt_alpha_ws T_PARENT_RIGHT {
    if ( ! check_in_range($3,0,100, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($5,0,100, &(@$)) ) { YYABORT; }
    if ( ! check_in_range($7,0,100, &(@$)) ) { YYABORT; }
    $$.alpha = $9; $$.red   = $3/100.0; $$.green = $5/100.0; $$.blue  = $7/100.0;
}
 /** hwb with comma */
| T_COL_HWB T_PARENT_LEFT t_property_color_value_angle T_COMMA t_property_color_value_unit T_COMMA t_property_color_value_unit t_property_color_opt_alpha_c T_PARENT_RIGHT {
    double h = $3, w = $5, b = $7;
    $$ = hwb_to_rgb ( h, w, b );
    $$.alpha = $8;
}
 /** hwb whitespace */
| T_COL_HWB T_PARENT_LEFT t_property_color_value_angle  t_property_color_value_unit  t_property_color_value_unit t_property_color_opt_alpha_ws T_PARENT_RIGHT {
    double h = $3, w = $4, b = $5;
    $$ = hwb_to_rgb ( h, w, b );
    $$.alpha = $6;
}
  /** cmyk  with comma */
| T_COL_CMYK T_PARENT_LEFT t_property_color_value_unit T_COMMA t_property_color_value_unit T_COMMA t_property_color_value_unit T_COMMA t_property_color_value_unit t_property_color_opt_alpha_c T_PARENT_RIGHT {
    $$.alpha = $10;
    double  c= $3, m= $5, y= $7, k= $9;
    $$.red   = (1.0-c)*(1.0-k);
    $$.green = (1.0-m)*(1.0-k);
    $$.blue  = (1.0-y)*(1.0-k);
}
 /** cmyk whitespace edition. */
| T_COL_CMYK T_PARENT_LEFT t_property_color_value_unit  t_property_color_value_unit  t_property_color_value_unit t_property_color_value_unit t_property_color_opt_alpha_ws T_PARENT_RIGHT {
    $$.alpha = $7;
    double  c= $3, m= $4, y= $5, k= $6;
    $$.red   = (1.0-c)*(1.0-k);
    $$.green = (1.0-m)*(1.0-k);
    $$.blue  = (1.0-y)*(1.0-k);
}
 /** hsl ( 0-360 0-100  % 0 - 100  % / alpha) */
| T_COL_HSL T_PARENT_LEFT t_property_color_value_angle t_property_color_value_unit t_property_color_value_unit t_property_color_opt_alpha_ws T_PARENT_RIGHT {
    double h = $3, s = $4, l = $5;
    $$ = hsl_to_rgb ( h, s, l );
    $$.alpha = $6;
}
 /** hsl ( 0-360 , 0-100  %, 0 - 100  %) */
| T_COL_HSL T_PARENT_LEFT t_property_color_value_angle T_COMMA t_property_color_value_unit T_COMMA t_property_color_value_unit t_property_color_opt_alpha_c T_PARENT_RIGHT {
    double h = $3, s = $5, l = $7;
    $$ = hsl_to_rgb ( h, s, l );
    $$.alpha = $8;
}
/** Hex colors parsed by lexer. */
| T_COLOR {
    $$ = $1;
}
| T_COLOR_TRANSPARENT {
    $$.alpha = 0.0;
    $$.red = $$.green = $$.blue = 0.0;
}
| T_COLOR_NAME t_property_color_opt_alpha_ws {
    $$ = $1;
    $$.alpha  = $2;
}
;
t_property_color_opt_alpha_c
: %empty { $$ = 1.0; }
| T_COMMA t_property_color_value_unit { $$ = $2;}
;
t_property_color_opt_alpha_ws
: %empty { $$ = 1.0; }
| T_FORWARD_SLASH t_property_color_value_unit { $$ = $2;}
;
 t_property_color_value_angle
: t_property_color_value              { $$ = $1/360.0;    if ( ! check_in_range ( $1, 0, 360, &(@$))){YYABORT;}}
| t_property_color_value T_ANGLE_DEG  { $$ = $1/360.0;    if ( ! check_in_range ( $1, 0, 360, &(@$))){YYABORT;}}
| t_property_color_value T_ANGLE_RAD  { $$ = $1/(2*G_PI); if ( ! check_in_range ( $1, 0.0, (2*G_PI), &(@$))){YYABORT;}}
| t_property_color_value T_ANGLE_GRAD { $$ = $1/400.0;    if ( ! check_in_range ( $1, 0, 400, &(@$))){YYABORT;}}
| t_property_color_value T_ANGLE_TURN { $$ = $1;          if ( ! check_in_range ( $1, 0.0, 1.0, &(@$))){YYABORT;}}
;

t_property_color_value_unit
: t_property_color_value T_PERCENT { $$ = $1/100.0; if ( !check_in_range ( $1, 0, 100, &(@$))){YYABORT;}}
| t_property_color_value           { $$ = $1;       if ( !check_in_range ( $1, 0.0, 1.0, &(@$))){YYABORT;}}
;
/** Color value to be double or integer. */
t_property_color_value
: T_DOUBLE { $$ = $1; }
| T_INT    { $$ = $1; }
;

t_property_orientation
: ORIENTATION_HORI {  $$ = ROFI_ORIENTATION_HORIZONTAL; }
| ORIENTATION_VERT {  $$ = ROFI_ORIENTATION_VERTICAL;   }
;

/** Property name */
t_property_name
: T_PROP_NAME { $$ = $1; }
;

t_entry_name_path_selectors:
t_entry_name_path { $$ = g_list_append  ( NULL, $1 ); }
| t_entry_name_path_selectors T_SSEP t_entry_name_path {
    $$ = g_list_append ( $1, $3);
}
| t_entry_name_path_selectors T_SSEP {
   $$ = $1;
}

;
t_entry_name_path:
T_NAME_ELEMENT { $$ = g_list_append ( NULL, $1 );}
| t_entry_name_path T_NSEP T_NAME_ELEMENT { $$ = g_list_append ( $1, $3);}
| t_entry_name_path T_NSEP  { $$ = $1; }
;

%%

