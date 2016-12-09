#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "theme.h"

void yyerror ( const char *);
Widget *rofi_theme_find_or_create_class ( Widget *base, const char *class )
{
    for ( unsigned int i = 0; i < base->num_widgets;i++){
        if ( g_strcmp0(base->widgets[i]->name, class) == 0 ){
            return base->widgets[i];
        } 
    }

    base->widgets = g_realloc ( base->widgets, sizeof(Widget*)*(base->num_widgets+1));
    base->widgets[base->num_widgets] = g_malloc0(sizeof(Widget));
    Widget *retv = base->widgets[base->num_widgets];
    retv->parent = base;
    retv->name = g_strdup(class);
    base->num_widgets++;
    return retv;
}
/**
 *  Properties
 */
Property *rofi_theme_property_create ( PropertyType type )
{
    Property *retv = g_malloc0 ( sizeof(Property) );
    retv->type = type;
    return retv;
}
void rofi_theme_property_free ( Property *p )
{
    if ( p == NULL ) {
        return;
    }
    g_free ( p->name );
    if ( p->type == P_STRING ) {
        g_free ( p->value.s );
    }
    g_free(p);
}

void rofi_theme_free ( Widget *widget )
{
    if ( widget == NULL ){
        return;
    }
    if ( widget->properties ) {
        g_hash_table_destroy ( widget->properties );
    }
    for ( unsigned int i = 0; i < widget->num_widgets; i++ ){
        rofi_theme_free ( widget->widgets[i] ); 
    }
    g_free ( widget->widgets );
    g_free ( widget->name );
    g_free ( widget );
}

/**
 * print
 */
static void rofi_theme_print_property_index ( int depth, Property *p )
{
    printf("%*s     %s: ", depth, "", p->name );
    switch ( p->type )
    {
        case P_STRING:
           printf("\"%s\"", p->value.s); 
           break;
        case P_INTEGER:
           printf("%d", p->value.i); 
           break;
        case P_FLOAT:
           printf("%.2f", p->value.f);
           break;
        case P_BOOLEAN:
           printf("%s", p->value.b?"true":"false");
           break;
        case P_COLOR:
           printf("#%08X", p->value.color);
           break;
    }
    putchar ( '\n' );
}

static void rofi_theme_print_index ( int depth, Widget *widget )
{
    printf ( "%*sName: %s \n", depth, "", widget->name );

    GHashTableIter iter;
    gpointer key, value;
    if ( widget->properties ){
        g_hash_table_iter_init (&iter, widget->properties);
        while (g_hash_table_iter_next (&iter, &key, &value))
        {
            Property *p = (Property*)value;
            rofi_theme_print_property_index ( depth, p );
        }
    }
    for ( unsigned int i = 0; i < widget->num_widgets;i++){
        rofi_theme_print_index ( depth+2, widget->widgets[i] );
    }
}
void rofi_theme_print ( Widget *widget )
{
    rofi_theme_print_index ( 0, widget);
}

extern int yyparse();
extern FILE* yyin;
extern Widget *rofi_theme;

void rofi_theme_parse_file ( const char *file )
{
    yyin = fopen ( file, "rb");
    if ( yyin == NULL ){
        fprintf(stderr, "Failed to open file: '%s'\n", strerror ( errno ) );
        return;
    }
    while ( yyparse() );
}

void yyerror(const char* s) {
	fprintf(stderr, "Parse error: %s\n", s);
	exit(EXIT_FAILURE);
}
