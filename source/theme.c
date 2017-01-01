#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "theme.h"
#include "lexer/theme-parser.h"
#include "helper.h"
#include "settings.h"
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
    printf("%*s%s: ", depth, "", p->name );
    switch ( p->type )
    {
        case P_STRING:
           printf("\"%s\";", p->value.s);
           break;
        case P_INTEGER:
           printf("%d;", p->value.i);
           break;
        case P_DOUBLE:
           printf("%.2f;", p->value.f);
           break;
        case P_BOOLEAN:
           printf("%s;", p->value.b?"true":"false");
           break;
        case P_COLOR:
           printf("#%02X%02X%02X%02X;",
                   (unsigned char)(p->value.color.alpha*255.0),
                   (unsigned char)(p->value.color.red*255.0),
                   (unsigned char)(p->value.color.green*255.0),
                   (unsigned char)(p->value.color.blue*255.0));
           break;
        case P_PADDING:
           if ( p->value.padding.left.type == PW_PX ) {
               printf("%upx ", (int)p->value.padding.left.distance );
           } else {
               printf("%fem ", p->value.padding.left.distance );
           }
           if ( p->value.padding.right.type == PW_PX ) {
               printf("%upx ", (int)p->value.padding.right.distance );
           } else {
               printf("%fem ", p->value.padding.right.distance );
           }
           if ( p->value.padding.top.type == PW_PX ) {
               printf("%upx ", (int)p->value.padding.top.distance );
           } else {
               printf("%fem ", p->value.padding.top.distance );
           }
           if ( p->value.padding.bottom.type == PW_PX ) {
               printf("%upx ", (int)p->value.padding.bottom.distance );
           } else {
               printf("%fem ", p->value.padding.bottom.distance );
           }
           printf(";\n");
    }
    putchar ( '\n' );
}

static void rofi_theme_print_index ( Widget *widget )
{
    GHashTableIter iter;
    gpointer key, value;
    if ( widget->properties ){
        int index = 0;
        GList *list = NULL;
        Widget *w = widget;
        while ( w){
            if ( g_strcmp0(w->name,"Root") == 0 ) {
                break;
            }
            list = g_list_prepend ( list, w->name );
            w = w->parent;
        }
        if ( g_list_length ( list ) > 0 ) {
            index = 4;
            for ( GList *iter = g_list_first ( list ); iter != NULL; iter = g_list_next ( iter ) ) {
                char *name = (char *)iter->data;
                if ( iter->prev == NULL && name[0] != '@' ){
                    putchar ( '#' );
                }
                fputs(name, stdout);
                if ( name[0] == '@' ) {
                    putchar(' ');
                } else {
                    if ( iter->next ) {
                        putchar('.');
                    }
                }
            }
            printf(" {\n");
        }
        g_hash_table_iter_init (&iter, widget->properties);
        while (g_hash_table_iter_next (&iter, &key, &value))
        {
            Property *p = (Property*)value;
            rofi_theme_print_property_index ( index, p );
        }
        if ( g_list_length ( list ) > 0 ) {
            printf("}\n");
        }
        g_list_free ( list );
    }
    for ( unsigned int i = 0; i < widget->num_widgets;i++){
        rofi_theme_print_index ( widget->widgets[i] );
    }
}
void rofi_theme_print ( Widget *widget )
{
    rofi_theme_print_index ( widget );
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
    // Fall back to px if no metric is given.
    p = rofi_theme_find_property ( widget, P_INTEGER, property );
    if ( p ){
        return (Distance){(double)p->value.i, PW_PX};
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
    }else {
        p = rofi_theme_find_property ( widget, P_INTEGER, property );
        if ( p ){
            Distance d = (Distance){p->value.i, PW_PX};
            pad = (Padding){d,d,d,d};
        }
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

static Property* rofi_theme_convert_get_color ( const char *color, const char *name )
{
    Color c = color_get ( color );
    Property *p = rofi_theme_property_create ( P_COLOR );
    p->name = g_strdup(name);
    p->value.color.alpha = c.alpha;
    p->value.color.red   = c.red;
    p->value.color.green = c.green;
    p->value.color.blue  = c.blue;

    return p;
}
static GHashTable * rofi_theme_convert_create_property_ht ( void )
{
    return g_hash_table_new_full ( g_str_hash, g_str_equal, NULL, (GDestroyNotify)rofi_theme_property_free );
}

void rofi_theme_convert_old_theme ( void )
{
    if ( rofi_theme != NULL ){
        return;
    }
    rofi_theme = (Widget*)g_malloc0 ( sizeof ( Widget ) );
    rofi_theme->name = g_strdup ( "Root" );
    rofi_theme->properties = rofi_theme_convert_create_property_ht ( );
    {
        // Spacing
        Widget *box_widget = rofi_theme_find_or_create_class ( rofi_theme , "@box" );
        box_widget->properties = rofi_theme_convert_create_property_ht ( );
        Property *p = rofi_theme_property_create ( P_INTEGER );
        p->name = g_strdup("spacing");
        p->value.i = config.padding;
        g_hash_table_replace ( box_widget->properties, p->name, p );
    }
    {
        // Spacing
        Widget *listview_widget = rofi_theme_find_or_create_class ( rofi_theme , "@listview" );
        listview_widget->properties = rofi_theme_convert_create_property_ht ( );
        Property *p = rofi_theme_property_create ( P_INTEGER );
        p->name = g_strdup("spacing");
        p->value.i = config.padding;
        g_hash_table_replace ( listview_widget->properties, p->name, p );
    }
    {
        // Border width.
        Widget *window_widget = rofi_theme_find_or_create_class ( rofi_theme , "@window" );
        window_widget->properties = rofi_theme_convert_create_property_ht ( );
        Property *p = rofi_theme_property_create ( P_INTEGER );
        p->name = g_strdup("border-width");
        p->value.i = config.menu_bw;
        g_hash_table_replace ( window_widget->properties, p->name, p );
        // Padding
        p = rofi_theme_property_create ( P_INTEGER );
        p->name = g_strdup("padding");
        p->value.i = config.padding;
        g_hash_table_replace ( window_widget->properties, p->name, p );
    }
    {
        gchar **vals = g_strsplit ( config.color_window, ",", 3 );
        if ( vals != NULL ){
            if ( vals[0] != NULL ) {
                Property *p = rofi_theme_convert_get_color ( vals[0], "background" );
                g_hash_table_replace ( rofi_theme->properties, p->name, p );

                if ( vals[1] != NULL ) {
                    p = rofi_theme_convert_get_color ( vals[1], "foreground" );
                    g_hash_table_replace ( rofi_theme->properties, p->name, p );

                   if ( vals[2] != NULL ) {
                       Widget *widget = rofi_theme_find_or_create_class ( rofi_theme , "@separator" );
                       widget->properties = rofi_theme_convert_create_property_ht ();
                       p = rofi_theme_convert_get_color ( vals[1], "foreground" );
                       g_hash_table_replace ( widget->properties, p->name, p );
                   }
                }

            }
        }
        g_strfreev ( vals );
        {
            Property *p = NULL;
            Widget *widget = rofi_theme_find_or_create_class ( rofi_theme , "@textbox" );

            Widget *wnormal = rofi_theme_find_or_create_class ( widget, "normal" );
            Widget *wselected = rofi_theme_find_or_create_class ( widget, "selected" );
            Widget *walternate = rofi_theme_find_or_create_class ( widget, "alternate" );


            gchar **vals = g_strsplit ( config.color_normal, ",", 5 );
            if ( g_strv_length (vals) == 5 ) {
                Widget *wnn = rofi_theme_find_or_create_class ( wnormal, "normal" );
                wnn->properties = rofi_theme_convert_create_property_ht ();
                p = rofi_theme_convert_get_color ( vals[0], "background" );
                g_hash_table_replace ( wnn->properties, p->name, p );
                p = rofi_theme_convert_get_color ( vals[1], "foreground" );
                g_hash_table_replace ( wnn->properties, p->name, p );

                Widget *wsl = rofi_theme_find_or_create_class ( wselected, "normal" );
                wsl->properties = rofi_theme_convert_create_property_ht ();
                p = rofi_theme_convert_get_color ( vals[3], "background" );
                g_hash_table_replace ( wsl->properties, p->name, p );
                p = rofi_theme_convert_get_color ( vals[4], "foreground" );
                g_hash_table_replace ( wsl->properties, p->name, p );

                Widget *wal = rofi_theme_find_or_create_class ( walternate, "normal" );
                wal->properties = rofi_theme_convert_create_property_ht ();
                p = rofi_theme_convert_get_color ( vals[2], "background" );
                g_hash_table_replace ( wal->properties, p->name, p );
                p = rofi_theme_convert_get_color ( vals[1], "foreground" );
                g_hash_table_replace ( wal->properties, p->name, p );
            }
            g_strfreev ( vals );

            vals = g_strsplit ( config.color_urgent, ",", 5 );
            if ( g_strv_length (vals) == 5 ) {
                Widget *wnn = rofi_theme_find_or_create_class ( wnormal, "urgent" );
                wnn->properties =  rofi_theme_convert_create_property_ht ();
                p = rofi_theme_convert_get_color ( vals[0], "background" );
                g_hash_table_replace ( wnn->properties, p->name, p );
                p = rofi_theme_convert_get_color ( vals[1], "foreground" );
                g_hash_table_replace ( wnn->properties, p->name, p );

                Widget *wsl = rofi_theme_find_or_create_class ( wselected, "urgent" );
                wsl->properties =  rofi_theme_convert_create_property_ht ();
                p = rofi_theme_convert_get_color ( vals[3], "background" );
                g_hash_table_replace ( wsl->properties, p->name, p );
                p = rofi_theme_convert_get_color ( vals[4], "foreground" );
                g_hash_table_replace ( wsl->properties, p->name, p );

                Widget *wal = rofi_theme_find_or_create_class ( walternate, "urgent" );
                wal->properties =  rofi_theme_convert_create_property_ht ();
                p = rofi_theme_convert_get_color ( vals[2], "background" );
                g_hash_table_replace ( wal->properties, p->name, p );
                p = rofi_theme_convert_get_color ( vals[1], "foreground" );
                g_hash_table_replace ( wal->properties, p->name, p );
            }
            g_strfreev ( vals );

            vals = g_strsplit ( config.color_active, ",", 5 );
            if ( g_strv_length (vals) == 5 ) {
                Widget *wnn = rofi_theme_find_or_create_class ( wnormal, "active" );
                wnn->properties =  rofi_theme_convert_create_property_ht ();
                p = rofi_theme_convert_get_color ( vals[0], "background" );
                g_hash_table_replace ( wnn->properties, p->name, p );
                p = rofi_theme_convert_get_color ( vals[1], "foreground" );
                g_hash_table_replace ( wnn->properties, p->name, p );

                Widget *wsl = rofi_theme_find_or_create_class ( wselected, "active" );
                wsl->properties =  rofi_theme_convert_create_property_ht ();
                p = rofi_theme_convert_get_color ( vals[3], "background" );
                g_hash_table_replace ( wsl->properties, p->name, p );
                p = rofi_theme_convert_get_color ( vals[4], "foreground" );
                g_hash_table_replace ( wsl->properties, p->name, p );

                Widget *wal = rofi_theme_find_or_create_class ( walternate, "active" );
                wal->properties =  rofi_theme_convert_create_property_ht ();
                p = rofi_theme_convert_get_color ( vals[2], "background" );
                g_hash_table_replace ( wal->properties, p->name, p );
                p = rofi_theme_convert_get_color ( vals[1], "foreground" );
                g_hash_table_replace ( wal->properties, p->name, p );
            }
            g_strfreev ( vals );
        }
    }
}
