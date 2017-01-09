%define api.pure
%glr-parser
%skeleton "glr.c"
%locations
%debug
%error-verbose

%code requires {
#include "theme.h"
}
%{
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>


#include "lexer/theme-parser.h"
ThemeWidget *rofi_theme = NULL;
void yyerror(YYLTYPE *yylloc, const char* s);
int yylex (YYSTYPE *, YYLTYPE *);
%}

%union {
	int           ival;
	double        fval;
    char          *sval;
    int           bval;
    ThemeColor    colorval;
    ThemeWidget        *theme;
    GList         *name_path;
    Property      *property;
    GHashTable    *property_list;
    Distance      distance;
}

%token <ival>     T_END     0  "end of file"
%token <ival>     T_ERROR   1  "error from file parser"
%token <ival>     T_INT
%token <fval>     T_DOUBLE
%token <sval>     T_STRING
%token <sval>     N_STRING
%token <ival>     T_POSITION;
%token <ival>     T_HIGHLIGHT_STYLE
%token <sval>     NAME_ELEMENT "Element name"
%token <bval>     T_BOOLEAN
%token <colorval> T_COLOR
%token <distance> T_PIXEL
%token <sval>     T_LINK
%token <sval>     FIRST_NAME

%token BOPEN        "bracket open"
%token BCLOSE       "bracket close"
%token PSEP         "property separator"
%token PCLOSE       "property close"
%token NSEP         "Name separator"
%token NAME_PREFIX  "Name element prefix ('#')"
%token WHITESPACE   "White space"
%token PDEFAULTS    "Default settings section ( '* { ... }')"

%type <ival> highlight_styles
%type <sval> entry
%type <sval> pvalue
%type <theme> entries
%type <name_path> name_path
%type <property> property
%type <property_list> property_list
%type <property_list> optional_properties
%start entries

%%

entries:
  %empty {
        // There is always a base widget.
        if (rofi_theme == NULL ){
            $$ =  rofi_theme = (ThemeWidget*)g_malloc0 (sizeof(ThemeWidget));
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
|   pvalue PSEP T_COLOR PCLOSE {
        $$ = rofi_theme_property_create ( P_COLOR );
        $$->name = $1;
        $$->value.color = $3;
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
|  pvalue PSEP T_PIXEL PCLOSE {
        $$ = rofi_theme_property_create ( P_PADDING );
        $$->name = $1;
        $$->value.padding = (Padding){ $3, $3, $3, $3 };
}
|  pvalue PSEP T_PIXEL T_PIXEL PCLOSE {
        $$ = rofi_theme_property_create ( P_PADDING );
        $$->name = $1;
        $$->value.padding = (Padding){ $3, $4, $3, $4 };
}
|  pvalue PSEP T_PIXEL T_PIXEL T_PIXEL PCLOSE {
        $$ = rofi_theme_property_create ( P_PADDING );
        $$->name = $1;
        $$->value.padding = (Padding){ $3, $4, $5, $4 };
}
|  pvalue PSEP T_PIXEL T_PIXEL T_PIXEL T_PIXEL PCLOSE {
        $$ = rofi_theme_property_create ( P_PADDING );
        $$->name = $1;
        $$->value.padding = (Padding){ $3, $4, $5, $6 };
}
| pvalue PSEP T_POSITION PCLOSE{
        $$ = rofi_theme_property_create ( P_POSITION );
        $$->name = $1;
        $$->value.i = $3;
}
| pvalue PSEP highlight_styles T_COLOR PCLOSE {
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
;

highlight_styles:
                T_HIGHLIGHT_STYLE { $$ = $1; }
| highlight_styles T_HIGHLIGHT_STYLE {
    $$ = $1 | $2;
}
;
pvalue: N_STRING { $$ = $1; }

name_path:
NAME_ELEMENT { $$ = g_list_append ( NULL, $1 );}
| name_path NSEP NAME_ELEMENT { $$ = g_list_append ( $1, $3);}
| name_path NSEP  { $$ = $1; }
;

%%

