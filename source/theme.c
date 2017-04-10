#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "theme.h"
#include "lexer/theme-parser.h"
#include "helper.h"
#include "settings.h"
#include "widgets/textbox.h"
#include "view.h"
#include "rofi.h"

/** Logging domain for theme */
#define LOG_DOMAIN    "Theme"

/**
 * Name of the property type
 */
const char *PropertyTypeName[] = {
    /** Integer */
    "Integer",
    /** Double */
    "Double",
    /** String */
    "String",
    /** Boolean */
    "Boolean",
    /** Color */
    "Color",
    /** Padding */
    "Padding",
    /** Link to global setting */
    "Reference",
    /** Position */
    "Position",
    /** Highlight */
    "Highlight",
};
void yyerror ( YYLTYPE *ylloc, const char *, const char * );
static gboolean distance_compare ( Distance d, Distance e )
{
    return d.type == e.type && d.distance == e.distance && d.style == e.style;
}

ThemeWidget *rofi_theme_find_or_create_name ( ThemeWidget *base, const char *name )
{
    for ( unsigned int i = 0; i < base->num_widgets; i++ ) {
        if ( g_strcmp0 ( base->widgets[i]->name, name ) == 0 ) {
            return base->widgets[i];
        }
    }

    base->widgets                    = g_realloc ( base->widgets, sizeof ( ThemeWidget* ) * ( base->num_widgets + 1 ) );
    base->widgets[base->num_widgets] = g_slice_new0 ( ThemeWidget );
    ThemeWidget *retv = base->widgets[base->num_widgets];
    retv->parent = base;
    retv->name   = g_strdup ( name );
    base->num_widgets++;
    return retv;
}
/**
 *  Properties
 */
Property *rofi_theme_property_create ( PropertyType type )
{
    Property *retv = g_slice_new0 ( Property );
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
    else if ( p->type == P_LINK ) {
        g_free ( p->value.link.name );
    }
    g_slice_free ( Property, p );
}

void rofi_theme_free ( ThemeWidget *widget )
{
    if ( widget == NULL ) {
        return;
    }
    if ( widget->properties ) {
        g_hash_table_destroy ( widget->properties );
    }
    for ( unsigned int i = 0; i < widget->num_widgets; i++ ) {
        rofi_theme_free ( widget->widgets[i] );
    }
    g_free ( widget->widgets );
    g_free ( widget->name );
    g_slice_free ( ThemeWidget, widget );
}

/**
 * print
 */
static void rofi_theme_print_distance ( Distance d )
{
    if ( d.type == PW_PX ) {
        printf ( "%upx ", (unsigned int) d.distance );
    }
    else if ( d.type == PW_PERCENT ) {
        printf ( "%f%% ", d.distance );
    }
    else {
        printf ( "%fem ", d.distance );
    }
    if ( d.style == DASH ) {
        printf ( "dash " );
    }
}
/** Textual representation of Window Location */
const char * const WindowLocationStr[9] = {
    "center",
    "northwest",
    "north",
    "northeast",
    "east",
    "southeast",
    "south",
    "southwest",
    "west"
};

static void rofi_theme_print_property_index ( size_t pnl, int depth, Property *p )
{
    int pl = strlen ( p->name );
    printf ( "%*s%s:%*s ", depth, "", p->name, (int) pnl - pl, "" );
    switch ( p->type )
    {
    case P_HIGHLIGHT:
        if ( p->value.highlight.style & HL_BOLD ) {
            printf ( "bold " );
        }
        if ( p->value.highlight.style & HL_UNDERLINE ) {
            printf ( "underline " );
        }
        if ( p->value.highlight.style & HL_ITALIC ) {
            printf ( "italic " );
        }
        if ( p->value.highlight.style & HL_COLOR ) {
            printf ( "#%02X%02X%02X%02X",
                     (unsigned char) ( p->value.highlight.color.alpha * 255.0 ),
                     (unsigned char) ( p->value.highlight.color.red * 255.0 ),
                     (unsigned char) ( p->value.highlight.color.green * 255.0 ),
                     (unsigned char) ( p->value.highlight.color.blue * 255.0 ) );
        }
        printf ( ";" );
        break;
    case P_POSITION:
        printf ( "%s;", WindowLocationStr[p->value.i] );
        break;
    case P_STRING:
        printf ( "\"%s\";", p->value.s );
        break;
    case P_INTEGER:
        printf ( "%d;", p->value.i );
        break;
    case P_DOUBLE:
        printf ( "%.2f;", p->value.f );
        break;
    case P_BOOLEAN:
        printf ( "%s;", p->value.b ? "true" : "false" );
        break;
    case P_COLOR:
        printf ( "#%02X%02X%02X%02X;",
                 (unsigned char) ( p->value.color.alpha * 255.0 ),
                 (unsigned char) ( p->value.color.red * 255.0 ),
                 (unsigned char) ( p->value.color.green * 255.0 ),
                 (unsigned char) ( p->value.color.blue * 255.0 ) );
        break;
    case P_PADDING:
        if ( distance_compare ( p->value.padding.top, p->value.padding.bottom ) &&
             distance_compare ( p->value.padding.left, p->value.padding.right ) &&
             distance_compare ( p->value.padding.left, p->value.padding.top ) ) {
            rofi_theme_print_distance ( p->value.padding.left );
        }
        else if ( distance_compare ( p->value.padding.top, p->value.padding.bottom ) &&
                  distance_compare ( p->value.padding.left, p->value.padding.right ) ) {
            rofi_theme_print_distance ( p->value.padding.top );
            rofi_theme_print_distance ( p->value.padding.left );
        }
        else if ( !distance_compare ( p->value.padding.top, p->value.padding.bottom ) &&
                  distance_compare ( p->value.padding.left, p->value.padding.right ) ) {
            rofi_theme_print_distance ( p->value.padding.top );
            rofi_theme_print_distance ( p->value.padding.left );
            rofi_theme_print_distance ( p->value.padding.bottom );
        }
        else {
            rofi_theme_print_distance ( p->value.padding.top );
            rofi_theme_print_distance ( p->value.padding.right );
            rofi_theme_print_distance ( p->value.padding.bottom );
            rofi_theme_print_distance ( p->value.padding.left );
        }
        printf ( ";" );
        break;
    case P_LINK:
        printf ( "%s;", p->value.link.name );
        break;
    }
    putchar ( '\n' );
}

static void rofi_theme_print_index ( ThemeWidget *widget )
{
    GHashTableIter iter;
    gpointer       key, value;
    if ( widget->properties ) {
        int         index = 0;
        GList       *list = NULL;
        ThemeWidget *w    = widget;
        while ( w ) {
            if ( g_strcmp0 ( w->name, "Root" ) == 0 ) {
                break;
            }
            list = g_list_prepend ( list, w->name );
            w    = w->parent;
        }
        if ( g_list_length ( list ) > 0 ) {
            index = 4;
            for ( GList *iter = g_list_first ( list ); iter != NULL; iter = g_list_next ( iter ) ) {
                char *name = (char *) iter->data;
                if ( iter->prev == NULL ) {
                    putchar ( '#' );
                }
                fputs ( name, stdout );
                if ( iter->next ) {
                    putchar ( '.' );
                }
            }
            printf ( " {\n" );
        }
        else {
            index = 4;
            printf ( "* {\n" );
        }
        size_t property_name_length = 0;
        g_hash_table_iter_init ( &iter, widget->properties );
        while ( g_hash_table_iter_next ( &iter, &key, &value ) ) {
            Property *p = (Property *) value;
            property_name_length = MAX ( strlen ( p->name ), property_name_length );
        }
        g_hash_table_iter_init ( &iter, widget->properties );
        while ( g_hash_table_iter_next ( &iter, &key, &value ) ) {
            Property *p = (Property *) value;
            rofi_theme_print_property_index ( property_name_length, index, p );
        }
        printf ( "}\n" );
        g_list_free ( list );
    }
    for ( unsigned int i = 0; i < widget->num_widgets; i++ ) {
        rofi_theme_print_index ( widget->widgets[i] );
    }
}
void rofi_theme_print ( ThemeWidget *widget )
{
    if ( widget != NULL ) {
        rofi_theme_print_index ( widget );
    }
}

/**
 * Main lex parser.
 */
int yyparse ();

/**
 * Destroy the internal of lex parser.
 */
void yylex_destroy ( void );

/**
 * Global handle input file to flex parser.
 */
extern FILE* yyin;

/**
 * @param yylloc The file location.
 * @param what   What we are parsing, filename or string.
 * @param s      Error message string.
 *
 * Error handler for the lex parser.
 */
void yyerror ( YYLTYPE *yylloc, const char *what, const char* s )
{
    char    *what_esc = g_markup_escape_text ( what, -1 );
    GString *str      = g_string_new ( "" );
    g_string_printf ( str, "<big><b>Error while parsing theme:</b></big> <i>%s</i>\n", what_esc );
    g_free ( what_esc );
    char *esc = g_markup_escape_text ( s, -1 );
    g_string_append_printf ( str, "\tParser error: <span size=\"smaller\" style=\"italic\">%s</span>\n", esc );
    g_free ( esc );
    if ( yylloc->filename != NULL ) {
        g_string_append_printf ( str, "\tLocation:     line %d column %d to line %d column %d.\n" \
                                 "\tFile          '%s'\n", yylloc->first_line, yylloc->first_column, yylloc->last_line, yylloc->last_column, yylloc->filename );
    }
    else {
        g_string_append_printf ( str, "\tLocation:     line %d column %d to line %d column %d\n", yylloc->first_line, yylloc->first_column, yylloc->last_line, yylloc->last_column );
    }
    g_log ( "Parser", G_LOG_LEVEL_DEBUG, "Failed to parse theme:\n%s", str->str );
    rofi_add_error_message ( str );
}

static gboolean rofi_theme_steal_property_int ( gpointer key, gpointer value, gpointer user_data )
{
    GHashTable *table = (GHashTable *) user_data;
    g_hash_table_replace ( table, key, value );
    return TRUE;
}
void rofi_theme_widget_add_properties ( ThemeWidget *widget, GHashTable *table )
{
    if ( table == NULL ) {
        return;
    }
    if ( widget->properties == NULL ) {
        widget->properties = table;
        return;
    }
    g_hash_table_foreach_steal ( table, rofi_theme_steal_property_int, widget->properties );
    g_hash_table_destroy ( table );
}

/**
 * Public API
 */

static inline ThemeWidget *rofi_theme_find_single ( ThemeWidget *widget, const char *name )
{
    for ( unsigned int j = 0; j < widget->num_widgets; j++ ) {
        if ( g_strcmp0 ( widget->widgets[j]->name, name ) == 0 ) {
            return widget->widgets[j];
        }
    }
    return widget;
}

static ThemeWidget *rofi_theme_find ( ThemeWidget *widget, const char *name, const gboolean exact )
{
    if ( widget == NULL || name == NULL ) {
        return widget;
    }
    char *tname   = g_strdup ( name );
    char *saveptr = NULL;
    int  found    = TRUE;
    for ( const char *iter = strtok_r ( tname, ".", &saveptr ); iter != NULL; iter = strtok_r ( NULL, ".", &saveptr ) ) {
        found = FALSE;
        ThemeWidget *f = rofi_theme_find_single ( widget, iter );
        if ( f != widget ) {
            widget = f;
            found  = TRUE;
        }
    }
    g_free ( tname );
    if ( !exact || found ) {
        return widget;
    }
    else {
        return NULL;
    }
}

static void rofi_theme_resolve_link_property ( Property *p, int depth )
{
    // Set name, remove '@' prefix.
    const char *name = p->value.link.name + 1;
    if ( depth > 20 ) {
        g_log ( LOG_DOMAIN, G_LOG_LEVEL_WARNING, "Found more then 20 redirects for property. Stopping." );
        p->value.link.ref = p;
        return;
    }

    if ( rofi_theme->properties && g_hash_table_contains ( rofi_theme->properties, name ) ) {
        Property *pr = g_hash_table_lookup ( rofi_theme->properties, name );
        if ( pr->type == P_LINK ) {
            if ( pr->value.link.ref == NULL ) {
                rofi_theme_resolve_link_property ( pr, depth + 1 );
            }
            if ( pr->value.link.ref != pr ) {
                p->value.link.ref = pr->value.link.ref;
                return;
            }
        }
        else {
            p->value.link.ref = pr;
            return;
        }
    }

    // No found, set ref to self.
    p->value.link.ref = p;
}

Property *rofi_theme_find_property ( ThemeWidget *widget, PropertyType type, const char *property, gboolean exact )
{
    while ( widget ) {
        if ( widget->properties && g_hash_table_contains ( widget->properties, property ) ) {
            Property *p = g_hash_table_lookup ( widget->properties, property );
            if ( p->type == P_LINK ) {
                if ( p->value.link.ref == NULL ) {
                    // Resolve link.
                    rofi_theme_resolve_link_property ( p, 0 );
                }
                if ( p->value.link.ref != NULL && p->value.link.ref->type == type ) {
                    return p->value.link.ref;
                }
            }
            if ( p->type == type ) {
                return p;
            }
            // Padding and integer can be converted.
            if ( p->type == P_INTEGER && type == P_PADDING ) {
                return p;
            }
            g_log ( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Found property: '%s' on '%s', but type %s does not match expected type %s.",
                    property, widget->name,
                    PropertyTypeName[p->type],
                    PropertyTypeName[type]
                    );
        }
        if ( exact ) {
            return NULL;
        }
        widget = widget->parent;
    }
    return NULL;
}
ThemeWidget *rofi_theme_find_widget ( const char *name, const char *state, gboolean exact )
{
    // First find exact match based on name.
    ThemeWidget *widget = rofi_theme_find ( rofi_theme, name, exact );
    widget = rofi_theme_find ( widget, state, exact );

    return widget;
}

int rofi_theme_get_position ( const widget *widget, const char *property, int def )
{
    ThemeWidget *wid = rofi_theme_find_widget ( widget->name, widget->state, FALSE );
    Property    *p   = rofi_theme_find_property ( wid, P_POSITION, property, FALSE );
    if ( p ) {
        return p->value.i;
    }
    g_log ( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Theme entry: #%s %s property %s unset.", widget->name, widget->state ? widget->state : "", property );
    return def;
}

int rofi_theme_get_integer ( const widget *widget, const char *property, int def )
{
    ThemeWidget *wid = rofi_theme_find_widget ( widget->name, widget->state, FALSE );
    Property    *p   = rofi_theme_find_property ( wid, P_INTEGER, property, FALSE );
    if ( p ) {
        return p->value.i;
    }
    g_log ( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Theme entry: #%s %s property %s unset.", widget->name, widget->state ? widget->state : "", property );
    return def;
}
int rofi_theme_get_integer_exact ( const widget *widget, const char *property, int def )
{
    ThemeWidget *wid = rofi_theme_find_widget ( widget->name, widget->state, TRUE );
    Property    *p   = rofi_theme_find_property ( wid, P_INTEGER, property, TRUE );
    if ( p ) {
        return p->value.i;
    }
    g_log ( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Theme entry: #%s %s property %s unset.", widget->name, widget->state ? widget->state : "", property );
    return def;
}

Distance rofi_theme_get_distance ( const widget *widget, const char *property, int def )
{
    ThemeWidget *wid = rofi_theme_find_widget ( widget->name, widget->state, FALSE );
    Property    *p   = rofi_theme_find_property ( wid, P_PADDING, property, FALSE );
    if ( p ) {
        if ( p->type == P_INTEGER ) {
            return (Distance){ p->value.i, PW_PX, SOLID };
        }
        else {
            return p->value.padding.left;
        }
    }
    g_log ( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Theme entry: #%s %s property %s unset.", widget->name, widget->state ? widget->state : "", property );
    return (Distance){ def, PW_PX, SOLID };
}

int rofi_theme_get_boolean ( const widget *widget, const char *property, int def )
{
    ThemeWidget *wid = rofi_theme_find_widget ( widget->name, widget->state, FALSE );
    Property    *p   = rofi_theme_find_property ( wid, P_BOOLEAN, property, FALSE );
    if ( p ) {
        return p->value.b;
    }
    g_log ( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Theme entry: #%s %s property %s unset.", widget->name, widget->state ? widget->state : "", property );
    return def;
}

char *rofi_theme_get_string ( const widget *widget, const char *property, char *def )
{
    ThemeWidget *wid = rofi_theme_find_widget ( widget->name, widget->state, FALSE );
    Property    *p   = rofi_theme_find_property ( wid, P_STRING, property, FALSE );
    if ( p ) {
        return p->value.s;
    }
    g_log ( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Theme entry: #%s %s property %s unset.", widget->name, widget->state ? widget->state : "", property );
    return def;
}
double rofi_theme_get_double ( const widget *widget, const char *property, double def )
{
    ThemeWidget *wid = rofi_theme_find_widget ( widget->name, widget->state, FALSE );
    Property    *p   = rofi_theme_find_property ( wid, P_DOUBLE, property, FALSE );
    if ( p ) {
        return p->value.b;
    }
    g_log ( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Theme entry: #%s %s property %s unset.", widget->name, widget->state ? widget->state : "", property );
    return def;
}
void rofi_theme_get_color ( const widget *widget, const char *property, cairo_t *d )
{
    ThemeWidget *wid = rofi_theme_find_widget ( widget->name, widget->state, FALSE );
    Property    *p   = rofi_theme_find_property ( wid, P_COLOR, property, FALSE );
    if ( p ) {
        cairo_set_source_rgba ( d,
                                p->value.color.red,
                                p->value.color.green,
                                p->value.color.blue,
                                p->value.color.alpha
                                );
    }
    else {
        g_log ( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Theme entry: #%s %s property %s unset.", widget->name, widget->state ? widget->state : "", property );
    }
}
Padding rofi_theme_get_padding ( const widget *widget, const char *property, Padding pad )
{
    ThemeWidget *wid = rofi_theme_find_widget ( widget->name, widget->state, FALSE );
    Property    *p   = rofi_theme_find_property ( wid, P_PADDING, property, FALSE );
    if ( p ) {
        if ( p->type == P_PADDING ) {
            pad = p->value.padding;
        }
        else {
            Distance d = (Distance){ p->value.i, PW_PX, SOLID };
            return (Padding){ d, d, d, d };
        }
    }
    g_log ( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Theme entry: #%s %s property %s unset.", widget->name, widget->state ? widget->state : "", property );
    return pad;
}
ThemeHighlight rofi_theme_get_highlight ( widget *widget, const char *property, ThemeHighlight th )
{
    ThemeWidget *wid = rofi_theme_find_widget ( widget->name, widget->state, FALSE );
    Property    *p   = rofi_theme_find_property ( wid, P_HIGHLIGHT, property, FALSE );
    if ( p ) {
        return p->value.highlight;
    }
    g_log ( LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Theme entry: #%s %s property %s unset.", widget->name, widget->state ? widget->state : "", property );
    return th;
}

int distance_get_pixel ( Distance d, Orientation ori )
{
    if ( d.type == PW_EM ) {
        return d.distance * textbox_get_estimated_char_height ();
    }
    else if ( d.type == PW_PERCENT ) {
        if ( ori == ORIENTATION_VERTICAL ) {
            int height = 0;
            rofi_view_get_current_monitor ( NULL, &height );
            return ( d.distance * height ) / ( 100.0 );
        }
        else {
            int width = 0;
            rofi_view_get_current_monitor ( &width, NULL );
            return ( d.distance * width ) / ( 100.0 );
        }
    }
    return d.distance;
}

void distance_get_linestyle ( Distance d, cairo_t *draw )
{
    if ( d.style == DASH ) {
        const double dashes[1] = { 4 };
        cairo_set_dash ( draw, dashes, 1, 0.0 );
    }
    else {
        cairo_set_dash ( draw, NULL, 0, 0.0 );
    }
}

gboolean rofi_theme_is_empty ( void )
{
    if ( rofi_theme == NULL ) {
        return TRUE;
    }
    if ( rofi_theme->properties == NULL && rofi_theme->num_widgets == 0 ) {
        return TRUE;
    }

    return FALSE;
}

#ifdef THEME_CONVERTER

void rofi_theme_convert_old ( void )
{
    if ( config.color_window ) {
        char             **retv  = g_strsplit ( config.color_window, ",", -1 );
        const char const *conf[] = {
            "* { background: %s; }",
            "* { bordercolor: %s; }",
            "* { separatorcolor: %s; }"
        };
        for ( int i = 0; retv && retv[i] && i < 3; i++ ) {
            char *str = g_strdup_printf ( conf[i], retv[i] );
            rofi_theme_parse_string ( str );
            g_free ( str );
        }
        g_strfreev ( retv );
    }
    if ( config.color_normal ) {
        char             **retv  = g_strsplit ( config.color_normal, ",", -1 );
        const char const *conf[] = {
            "* { normal-background: %s; }",
            "* { foreground: %s; normal-foreground: @foreground; alternate-normal-foreground: @foreground; }",
            "* { alternate-normal-background: %s; }",
            "* { selected-normal-background: %s; }",
            "* { selected-normal-foreground: %s; }"
        };
        for ( int i = 0; retv && retv[i]; i++ ) {
            char *str = g_strdup_printf ( conf[i], retv[i] );
            rofi_theme_parse_string ( str );
            g_free ( str );
        }
        g_strfreev ( retv );
    }
    if ( config.color_urgent ) {
        char             **retv  = g_strsplit ( config.color_urgent, ",", -1 );
        const char const *conf[] = {
            "* { urgent-background: %s; }",
            "* { urgent-foreground: %s; alternate-urgent-foreground: @urgent-foreground;}",
            "* { alternate-urgent-background: %s; }",
            "* { selected-urgent-background: %s; }",
            "* { selected-urgent-foreground: %s; }"
        };
        for ( int i = 0; retv && retv[i]; i++ ) {
            char *str = g_strdup_printf ( conf[i], retv[i] );
            rofi_theme_parse_string ( str );
            g_free ( str );
        }
        g_strfreev ( retv );
    }
    if ( config.color_active ) {
        char             **retv  = g_strsplit ( config.color_active, ",", -1 );
        const char const *conf[] = {
            "* { active-background: %s; }",
            "* { active-foreground: %s; alternate-active-foreground: @active-foreground;}",
            "* { alternate-active-background: %s; }",
            "* { selected-active-background: %s; }",
            "* { selected-active-foreground: %s; }"
        };
        for ( int i = 0; retv && retv[i]; i++ ) {
            char *str = g_strdup_printf ( conf[i], retv[i] );
            rofi_theme_parse_string ( str );
            g_free ( str );
        }
        g_strfreev ( retv );
    }
}
#endif // THEME_CONVERTER
