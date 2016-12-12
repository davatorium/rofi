#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "theme.h"
#include "lexer/theme-parser.h"
void yyerror ( YYLTYPE *ylloc, const char *);

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
        case P_DOUBLE:
           printf("%.2f", p->value.f);
           break;
        case P_BOOLEAN:
           printf("%s", p->value.b?"true":"false");
           break;
        case P_COLOR:
           printf("#%02X%02X%02X%02X",
                   (unsigned char)(p->value.color.alpha*255.0),
                   (unsigned char)(p->value.color.red*255.0),
                   (unsigned char)(p->value.color.green*255.0),
                   (unsigned char)(p->value.color.blue*255.0));
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

void yyerror(YYLTYPE *yylloc, const char* s) {
	fprintf(stderr, "Parse error: %s\n", s);
    fprintf(stderr, "From line %d column %d to line %d column %d\n", yylloc->first_line, yylloc->first_column, yylloc->last_line, yylloc->last_column);
	exit(EXIT_FAILURE);
}
/**
 * Public API
 */

void rofi_theme_parse_file ( const char *file )
{
    yyin = fopen ( file, "rb");
    if ( yyin == NULL ){
        fprintf(stderr, "Failed to open file: '%s'\n", strerror ( errno ) );
        return;
    }
    while ( yyparse() );
}

static Widget *rofi_theme_find ( Widget *widget , const char *name )
{
    if ( name == NULL ) {
        return widget;
    }
    char **names = g_strsplit ( name, "." , 0 );
    int found = TRUE;
    for ( unsigned int i = 0; found && names && names[i]; i++ ){
        found = FALSE;
        for ( unsigned int j = 0; j < widget ->num_widgets;j++){
            if ( g_strcmp0(widget->widgets[j]->name, names[i]) == 0 ){
                widget = widget->widgets[j];
                found = TRUE;
                break;
            }
        }
    } 
    g_strfreev(names);
    return widget;
}

static Property *rofi_theme_find_property ( Widget *widget, PropertyType type, const char *property )
{
    while ( widget ) {
        if ( widget->properties && g_hash_table_contains ( widget->properties, property) ) {
            Property *p = g_hash_table_lookup ( widget->properties, property);
            if ( p->type == type ){
                return p;
            }
        } 
        widget = widget->parent;
    }
    return NULL;
}

int rofi_theme_get_integer ( const char  *name, const char *property, int def )
{
    if ( rofi_theme == NULL ) {
        return def;
    }
    Widget *widget = rofi_theme_find ( rofi_theme, name );
    Property *p = rofi_theme_find_property ( widget, P_INTEGER, property );
    if ( p ){
        return p->value.i;
    }
    return def;
}

int rofi_theme_get_boolean ( const char  *name, const char *property, int def )
{
    if ( rofi_theme == NULL ) {
        return def;
    }
    Widget *widget = rofi_theme_find ( rofi_theme, name );
    Property *p = rofi_theme_find_property ( widget, P_BOOLEAN, property );
    if ( p ){
        return p->value.b;
    }
    return def;
}

char *rofi_theme_get_string ( const char  *name, const char *property, char *def )
{
    if ( rofi_theme == NULL ) {
        return def;
    }
    Widget *widget = rofi_theme_find ( rofi_theme, name );
    Property *p = rofi_theme_find_property ( widget, P_STRING, property );
    if ( p ){
        return p->value.s;
    }
    return def;
}
double rofi_theme_get_double ( const char  *name, const char *property, double def )
{
    if ( rofi_theme == NULL ) {
        return def;
    }
    Widget *widget = rofi_theme_find ( rofi_theme, name );
    Property *p = rofi_theme_find_property ( widget, P_DOUBLE, property );
    if ( p ){
        return p->value.b;
    }
    return def;
}
void rofi_theme_get_color ( const char  *name, const char *property, cairo_t *d) 
{
    if ( rofi_theme == NULL ) {
        return ;
    }
    Widget *widget = rofi_theme_find ( rofi_theme, name );
    Property *p = rofi_theme_find_property ( widget, P_COLOR, property );
    if ( p ){
        cairo_set_source_rgba ( d,
                p->value.color.red,
                p->value.color.green,
                p->value.color.blue,
                p->value.color.alpha
                );

    }
}
