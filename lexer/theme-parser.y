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


#include "theme.h"
#include "lexer/theme-parser.h"
Widget *rofi_theme = NULL;
void yyerror(YYLTYPE *yylloc, const char* s);
int yylex (YYSTYPE *, YYLTYPE *);
%}

%union {
	int           ival;
	double        fval;
    char          *sval;
    int           bval;
    ThemeColor    colorval;
    Widget        *theme;
    GList         *name_path;
    Property      *property;
    GHashTable    *property_list;
}

%token <ival>     T_INT
%token <fval>     T_DOUBLE
%token <sval>     T_STRING
%token <sval>     N_STRING
%token <sval>     NAME_ELEMENT
%token <bval>     T_BOOLEAN
%token <colorval> T_COLOR
%token <sval>     CLASS_NAME
%token <sval>     FIRST_NAME

%token BOPEN  "bracket open";
%token BCLOSE "bracket close";
%token PSEP   "property separator";
%token PCLOSE "property close";
%token NSEP   "Name separator";
%token CLASS_PREFIX  "Class prefix";
%token NAME_PREFIX  "Name prefix";
%token WHITESPACE    "White space";

%type <sval> entry
%type <sval> pvalue
%type <sval> class_name
%type <theme> entries
%type <theme> start
%type <name_path> name_path
%type <name_path> state_path
%type <property> property
%type <property_list> property_list
%type <property_list> optional_properties
%start start

%%

start:
     optional_properties
     entries {
        $$ = $2;
        if ( $1 != NULL ) {
            $$->properties = $1;
        }
     }
;
entries:
  %empty {
        // There is always a base widget.
        $$ =  rofi_theme = (Widget*)g_malloc0 (sizeof(Widget));
        rofi_theme->name = g_strdup ( "Window" );
  }
| entries entry { $$ = $1; }
;

entry:
CLASS_PREFIX class_name state_path BOPEN optional_properties BCLOSE
{
        gchar *classn = g_strconcat ( "@", $2, NULL);
        Widget *widget = rofi_theme_find_or_create_class ( rofi_theme , classn );
        g_free(classn);
        widget->set = TRUE;
        for ( GList *iter = g_list_first ( $3 ); iter ; iter = g_list_next ( iter ) ) {
            widget = rofi_theme_find_or_create_class ( widget, iter->data );
        }
        g_list_foreach ( $3, (GFunc)g_free , NULL );
        g_list_free ( $3 );
        widget->set = TRUE;
        if ( widget->properties != NULL ) {
            fprintf(stderr, "Properties already set on this widget.\n");
            exit ( EXIT_FAILURE );
        }
        widget->properties = $5;
}
| NAME_PREFIX name_path state_path BOPEN optional_properties BCLOSE
{
        Widget *widget = rofi_theme;
        for ( GList *iter = g_list_first ( $2 ); iter ; iter = g_list_next ( iter ) ) {
            widget = rofi_theme_find_or_create_class ( widget, iter->data );
        }
        g_list_foreach ( $2, (GFunc)g_free , NULL );
        g_list_free ( $2 );
        widget->set = TRUE;
        for ( GList *iter = g_list_first ( $3 ); iter ; iter = g_list_next ( iter ) ) {
            widget = rofi_theme_find_or_create_class ( widget, iter->data );
        }
        g_list_foreach ( $3, (GFunc)g_free , NULL );
        g_list_free ( $3 );
        widget->set = TRUE;
        if ( widget->properties != NULL ) {
            fprintf(stderr, "Properties already set on this widget.\n");
            exit ( EXIT_FAILURE );
        }
        widget->properties = $5;
};

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
|   pvalue PSEP T_BOOLEAN PCLOSE {
        $$ = rofi_theme_property_create ( P_BOOLEAN );
        $$->name = $1;
        $$->value.b = $3;
    }
;

pvalue: N_STRING { $$ = $1; }
class_name: NAME_ELEMENT {$$ = $1;}

name_path:
NAME_ELEMENT { $$ = g_list_append ( NULL, $1 );}
| name_path NSEP NAME_ELEMENT { $$ = g_list_append ( $1, $3);}
;

state_path:
  %empty                   { $$ = NULL; }
| N_STRING { $$ = g_list_append ( NULL, $1 );}
| state_path NSEP N_STRING  { $$ = g_list_append ( $1, $3);}
;


%%

