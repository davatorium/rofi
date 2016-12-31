#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "theme.h"
#include "lexer/theme-parser.h"
#include "helper.h"
#include "widgets/textbox.h"

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
        case P_PADDING:
           printf("%f%s %f%s %f%s %f%s",
                    p->value.padding.left.distance,
                    p->value.padding.left.type == PW_PX? "px":"em",
                    p->value.padding.right.distance,
                    p->value.padding.right.type == PW_PX? "px":"em",
                    p->value.padding.top.distance,
                    p->value.padding.top.type == PW_PX? "px":"em",
                    p->value.padding.bottom.distance,
                    p->value.padding.bottom.type == PW_PX? "px":"em"
                 );
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

int yyparse();
void yylex_destroy( void );
extern FILE* yyin;
extern Widget *rofi_theme;

void yyerror(YYLTYPE *yylloc, const char* s) {
	fprintf(stderr, "Parse error: %s\n", s);
    fprintf(stderr, "From line %d column %d to line %d column %d\n", yylloc->first_line, yylloc->first_column, yylloc->last_line, yylloc->last_column);
	exit(EXIT_FAILURE);
}

static gboolean rofi_theme_steal_property_int ( gpointer key, gpointer value, gpointer user_data)
{
    GHashTable *table = (GHashTable*)user_data;
    g_hash_table_replace ( table, key, value);
    return TRUE;
}
void rofi_theme_widget_add_properties ( Widget *widget, GHashTable *table )
{
    if ( table == NULL ) {
        return;
    }
    if ( widget->properties == NULL ){
        widget->properties = table;
        return;
    }
    g_hash_table_foreach_steal ( table, rofi_theme_steal_property_int, widget->properties );
    g_hash_table_destroy ( table );
}


/**
 * Public API
 */

void rofi_theme_parse_file ( const char *file )
{
    char *filename = rofi_expand_path ( file );
    yyin = fopen ( filename, "rb");
    if ( yyin == NULL ){
        fprintf(stderr, "Failed to open file: %s: '%s'\n", filename, strerror ( errno ) );
        g_free(filename);
        return;
    }
    while ( yyparse() );
    yylex_destroy();
    g_free(filename);
}
static Widget *rofi_theme_find_single ( Widget *widget, const char *name)
{
    for ( unsigned int j = 0; j < widget->num_widgets;j++){
        if ( g_strcmp0(widget->widgets[j]->name, name ) == 0 ){
            return widget->widgets[j];
        }
    }
    return widget;
}

static Widget *rofi_theme_find ( Widget *widget , const char *name, const gboolean exact )
{
    if ( widget  == NULL  || name == NULL ) {
        return widget;
    }
    char **names = g_strsplit ( name, "." , 0 );
    int found = TRUE;
    for ( unsigned int i = 0; found && names && names[i]; i++ ){
        found = FALSE;
        Widget *f = rofi_theme_find_single ( widget, names[i]);
        if ( f != widget ){
            widget = f;
            found = TRUE;
        }
    }
    g_strfreev(names);
    if ( !exact || found ){
        return widget;
    } else {
        return NULL;
    }
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
static Widget *rofi_theme_find_widget ( const char *wclass, const char *name, const char *state )
{
    // First find exact match based on name.
    Widget *widget = rofi_theme_find ( rofi_theme, name, TRUE );
    widget = rofi_theme_find ( widget, state, TRUE );

    if ( widget == NULL ){
        // Fall back to class
        widget = rofi_theme_find ( rofi_theme, wclass, TRUE);
        widget = rofi_theme_find ( widget, state, TRUE );
    }
    if ( widget == NULL ){
        // Fuzzy finder
        widget = rofi_theme_find ( rofi_theme, name, FALSE );
        if ( widget == rofi_theme ){
            widget = rofi_theme_find ( widget, wclass, FALSE );
        }
        widget = rofi_theme_find ( widget, state, FALSE );
    }
    return widget;
}

int rofi_theme_get_integer ( const char *wclass, const char  *name, const char *state, const char *property, int def )
{
    Widget *widget = rofi_theme_find_widget ( wclass, name, state );
    Property *p = rofi_theme_find_property ( widget, P_INTEGER, property );
    if ( p ){
        return p->value.i;
    }
    return def;
}
Distance rofi_theme_get_distance ( const char *wclass, const char  *name, const char *state, const char *property, int def )
{
    Widget *widget = rofi_theme_find_widget ( wclass, name, state );
    Property *p = rofi_theme_find_property ( widget, P_PADDING, property );
    if ( p ){
        return p->value.padding.left;
    }
    return (Distance){def, PW_PX};
}

int rofi_theme_get_boolean ( const char *wclass, const char  *name, const char *state, const char *property, int def )
{
    Widget *widget = rofi_theme_find_widget ( wclass, name, state );
    Property *p = rofi_theme_find_property ( widget, P_BOOLEAN, property );
    if ( p ){
        return p->value.b;
    }
    return def;
}

char *rofi_theme_get_string ( const char *wclass, const char  *name, const char *state, const char *property, char *def )
{
    Widget *widget = rofi_theme_find_widget ( wclass, name, state );
    Property *p = rofi_theme_find_property ( widget, P_STRING, property );
    if ( p ){
        return p->value.s;
    }
    return def;
}
double rofi_theme_get_double ( const char *wclass, const char  *name, const char *state, const char *property, double def )
{
    Widget *widget = rofi_theme_find_widget ( wclass, name, state );
    Property *p = rofi_theme_find_property ( widget, P_DOUBLE, property );
    if ( p ){
        return p->value.b;
    }
    return def;
}
void rofi_theme_get_color ( const char *wclass, const char  *name, const char *state, const char *property, cairo_t *d)
{
    Widget *widget = rofi_theme_find_widget ( wclass, name, state );
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
Padding rofi_theme_get_padding ( const char *wclass, const char  *name, const char *state, const char *property, Padding pad )
{
    Widget *widget = rofi_theme_find_widget ( wclass, name, state );
    Property *p = rofi_theme_find_property ( widget, P_PADDING, property );
    if ( p ){
        pad = p->value.padding;
    }
    return pad;
}

int distance_get_pixel ( Distance d )
{
    if ( d.type == PW_EM ){
        return d.distance*textbox_get_estimated_char_height();
    }
    return d.distance;
}
