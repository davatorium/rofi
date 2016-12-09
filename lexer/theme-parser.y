%locations
%debug
%error-verbose

%code requires {
#include "theme.h"
}
%{
#include <stdio.h>
#include <stdlib.h>

void yyerror(const char* s);
int yylex (void );

#include "theme.h"
Widget *rofi_theme = NULL;
%}

%union {
	int           ival;
	double        fval;
    char          *sval;
    int           bval;
    unsigned int  colorval;
    Widget        *theme;
    GList         *name_path;
    Property      *property;
    GHashTable    *property_list;
}

%token <ival>     T_INT
%token <fval>     T_FLOAT
%token <sval>     T_STRING
%token <sval>     N_STRING
%token <bval>     T_BOOLEAN
%token <colorval> T_COLOR

%token CLASS  "class";
%token BOPEN  "bracket open";
%token BCLOSE "bracket close";
%token PSEP   "property separator";
%token PCLOSE "property close";
%token NSEP   "Name separator";

%type <sval> class
%type <sval> entry 
%type <sval> pvalue
%type <theme> entries 
%type <theme> start 
%type <name_path> name_path
%type <property> property
%type <property_list> property_list
%type <property_list> properties
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
     class
     name_path
     properties 
{
        Widget *widget = rofi_theme_find_or_create_class ( rofi_theme , $1 );
        g_free($1);
        for ( GList *iter = g_list_first ( $2 ); iter ; iter = g_list_next ( iter ) ) {
            widget = rofi_theme_find_or_create_class ( widget, iter->data ); 
        }
        g_list_foreach ( $2, (GFunc)g_free , NULL );
        g_list_free ( $2 );
        if ( widget->properties != NULL ) {
            fprintf(stderr, "Properties already set on this widget.\n");
            exit ( EXIT_FAILURE );
        }
        widget->properties = $3;

};

/**
 * properties
 */
optional_properties
          : %empty { $$ = NULL; }
          | property_list { $$ = $1; }
; 
properties: BOPEN property_list BCLOSE  { $$ = $2;}
          | BOPEN BCLOSE                { $$ = NULL; }
          | %empty { $$ = NULL; }
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
|   pvalue PSEP T_FLOAT PCLOSE { 
        $$ = rofi_theme_property_create ( P_FLOAT );
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

class:
     CLASS N_STRING { $$ = $2; }
;


name_path:
  %empty                   { $$ = NULL; }
| N_STRING                 { $$ = g_list_append ( NULL, $1 );}
| name_path NSEP N_STRING  { $$ = g_list_append ( $1, $3);} 
;


%%

