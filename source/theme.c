/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2017 Qball Cow <qball@gmpclient.org>
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

/** Log domain used by the theme engine.*/
#define G_LOG_DOMAIN    "Theme"

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
// GFile stuff.
#include <gio/gio.h>
#include "theme.h"
#include "theme-parser.h"
#include "helper.h"
#include "settings.h"
#include "widgets/textbox.h"
#include "view.h"
#include "rofi.h"
#include "rofi-types.h"

void yyerror ( YYLTYPE *yylloc, const char *, const char * );
static gboolean distance_compare ( RofiDistance d, RofiDistance e )
{
    return d.type == e.type && d.distance == e.distance && d.style == e.style;
}

static gpointer rofi_g_list_strdup ( gconstpointer data, G_GNUC_UNUSED gpointer user_data )
{
    return g_strdup ( data );
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
Property* rofi_theme_property_copy ( Property *p )
{
    Property *retv = rofi_theme_property_create ( p->type );
    retv->name = g_strdup ( p->name );

    switch ( p->type )
    {
    case P_STRING:
        retv->value.s = g_strdup ( p->value.s );
        break;
    case P_LIST:
        retv->value.list = g_list_copy_deep ( p->value.list, rofi_g_list_strdup, NULL );
        break;
    case P_LINK:
        retv->value.link.name = g_strdup ( p->value.link.name );
        retv->value.link.ref  = NULL;
        if ( p->value.link.def_value ){
            retv->value.link.def_value = rofi_theme_property_copy(p->value.link.def_value);
        }
        break;
    default:
        retv->value = p->value;
    }
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
        if ( p->value.link.def_value ) {
            rofi_theme_property_free ( p->value.link.def_value );
        }
    }
    g_slice_free ( Property, p );
}

/**
 * This function is a hack to insert backward support for older theme with the updated listvie structure.
 */
static void rofi_theme_insert_listview_backwards_fix ( void )
{
    GHashTable *table= g_hash_table_new_full ( g_str_hash, g_str_equal, NULL, (GDestroyNotify) rofi_theme_property_free );
    ThemeWidget *t = rofi_theme_find_or_create_name ( rofi_theme, "element" );
    ThemeWidget *tt = rofi_theme_find_or_create_name ( rofi_theme, "element-text" );
    ThemeWidget *ti = rofi_theme_find_or_create_name ( rofi_theme, "element-icon" );

    // Inherit text color
    Property *ptc = rofi_theme_property_create ( P_INHERIT );
    ptc->name = g_strdup("text-color");
    g_hash_table_replace ( table, ptc->name, ptc );
    // Transparent background
    Property *ptb = rofi_theme_property_create ( P_COLOR );
    ptb->name = g_strdup("background-color");
    ptb->value.color.red   = 0.0;
    ptb->value.color.green = 0.0;
    ptb->value.color.blue  = 0.0;
    ptb->value.color.alpha = 0.0;
    g_hash_table_replace ( table, ptb->name, ptb );


    rofi_theme_widget_add_properties ( tt, table);


    RofiDistance dsize = (RofiDistance){1.2, ROFI_PU_CH, ROFI_HL_SOLID };
    Property *pts = rofi_theme_property_create ( P_PADDING );
    pts->value.padding.top = pts->value.padding.right = pts->value.padding.bottom = pts->value.padding.left = dsize;
    pts->name = g_strdup ( "size" );
    g_hash_table_replace ( table, pts->name, pts );


    rofi_theme_widget_add_properties ( ti, table);

    /** Add spacing between icon and text. */
    g_hash_table_destroy ( table );
    table= g_hash_table_new_full ( g_str_hash, g_str_equal, NULL, (GDestroyNotify) rofi_theme_property_free );
    Property *psp = rofi_theme_property_create ( P_PADDING );
    psp->name           = g_strdup( "spacing" );
    RofiDistance d = (RofiDistance){5, ROFI_PU_PX, ROFI_HL_SOLID };
    psp->value.padding  = (RofiPadding){d,d,d,d};
    g_hash_table_replace ( table, psp->name, psp );
    rofi_theme_widget_add_properties ( t, table);
    g_hash_table_destroy ( table );


}

void rofi_theme_reset ( void )
{
    rofi_theme_free ( rofi_theme );
    rofi_theme       = g_slice_new0 ( ThemeWidget );
    rofi_theme->name = g_strdup ( "Root" );
    // Hack to fix backwards compatibility.
    rofi_theme_insert_listview_backwards_fix ( );
}

void rofi_theme_free ( ThemeWidget *widget )
{
    if ( widget == NULL ) {
        return;
    }
    if ( widget->properties ) {
        g_hash_table_destroy ( widget->properties );
        widget->properties = NULL;
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
inline static void printf_double ( double d )
{
    char buf[G_ASCII_DTOSTR_BUF_SIZE];
    g_ascii_formatd ( buf, G_ASCII_DTOSTR_BUF_SIZE, "%.4lf", d );
    fputs ( buf, stdout );
}
static void rofi_theme_print_distance ( RofiDistance d )
{
    if ( d.type == ROFI_PU_PX ) {
        printf ( "%upx ", (unsigned int) d.distance );
    }
    else if ( d.type == ROFI_PU_PERCENT ) {
        printf_double ( d.distance );
        fputs ( "%% ", stdout );
    }
    else if ( d.type == ROFI_PU_CH ) {
        printf_double ( d.distance );
        fputs ( "ch ", stdout );
    }
    else {
        printf_double ( d.distance );
        fputs ( "em ", stdout );
    }
    if ( d.style == ROFI_HL_DASH ) {
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

static void int_rofi_theme_print_property ( Property *p )
{
    switch ( p->type )
    {
        case P_LIST:
            printf ( "[ " );
            for ( GList *iter = p->value.list; iter != NULL; iter = g_list_next ( iter ) ) {
                printf ( "%s", (char *) ( iter->data ) );
                if ( iter->next != NULL ) {
                    printf ( "," );
                }
            }
            printf ( " ]" );
            break;
        case P_ORIENTATION:
            printf ( "%s", ( p->value.i == ROFI_ORIENTATION_HORIZONTAL ) ? "horizontal" : "vertical" );
            break;
        case P_HIGHLIGHT:
            if ( p->value.highlight.style & ROFI_HL_BOLD ) {
                printf ( "bold " );
            }
            if ( p->value.highlight.style & ROFI_HL_UNDERLINE ) {
                printf ( "underline " );
            }
            if ( p->value.highlight.style & ROFI_HL_STRIKETHROUGH ) {
                printf ( "strikethrough " );
            }
            if ( p->value.highlight.style & ROFI_HL_ITALIC ) {
                printf ( "italic " );
            }
            if ( p->value.highlight.style & ROFI_HL_COLOR ) {
                printf ( "rgba ( %.0f, %.0f, %.0f, %.0f %% )",
                        ( p->value.highlight.color.red * 255.0 ),
                        ( p->value.highlight.color.green * 255.0 ),
                        ( p->value.highlight.color.blue * 255.0 ),
                        ( p->value.highlight.color.alpha * 100.0 ) );
            }
            break;
        case P_POSITION:
            printf ( "%s", WindowLocationStr[p->value.i] );
            break;
        case P_STRING:
            printf ( "\"%s\"", p->value.s );
            break;
        case P_INTEGER:
            printf ( "%d", p->value.i );
            break;
        case P_DOUBLE:
            printf ( "%.2f", p->value.f );
            break;
        case P_BOOLEAN:
            printf ( "%s", p->value.b ? "true" : "false" );
            break;
        case P_COLOR:
            printf ( "rgba ( %.0f, %.0f, %.0f, %.0f %% )",
                    ( p->value.color.red * 255.0 ),
                    ( p->value.color.green * 255.0 ),
                    ( p->value.color.blue * 255.0 ),
                    ( p->value.color.alpha * 100.0 ) );
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
            break;
        case P_LINK:
            if ( p->value.link.def_value) {
                printf( "var( %s, ", p->value.link.name );
                int_rofi_theme_print_property ( p->value.link.def_value );
                printf (")");
            }else {
                printf ( "var(%s)", p->value.link.name );
            }
            break;
        case P_INHERIT:
            printf ( "inherit" );
            break;
        default:
            break;
    }

}

static void rofi_theme_print_property_index ( size_t pnl, int depth, Property *p )
{
    int pl = strlen ( p->name );
    printf ( "%*s%s:%*s ", depth, "", p->name, (int) pnl - pl, "" );
    int_rofi_theme_print_property ( p );
    putchar ( ';' );
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
                fputs ( name, stdout );
                if ( iter->prev == NULL && iter->next ) {
                    putchar ( ' ' );
                }
                else if ( iter->next ) {
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
        printf ( "/**\n * rofi -dump-theme output.\n * Rofi version: %s\n **/\n", PACKAGE_VERSION );
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
    char    *what_esc = what ? g_markup_escape_text ( what, -1 ) : g_strdup ( "" );
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

static void rofi_theme_copy_property_int ( G_GNUC_UNUSED gpointer key, gpointer value, gpointer user_data )
{
    GHashTable *table = (GHashTable *) user_data;
    Property   *p     = rofi_theme_property_copy ( (Property *) value );
    g_hash_table_replace ( table, p->name, p );
}
void rofi_theme_widget_add_properties ( ThemeWidget *widget, GHashTable *table )
{
    if ( table == NULL ) {
        return;
    }
    if ( widget->properties == NULL ) {
        widget->properties = g_hash_table_new_full ( g_str_hash, g_str_equal, NULL, (GDestroyNotify) rofi_theme_property_free );
    }
    g_hash_table_foreach ( table, rofi_theme_copy_property_int, widget->properties );
}

/**
 * Public API
 */

static inline ThemeWidget *rofi_theme_find_single ( ThemeWidget *widget, const char *name )
{
    for ( unsigned int j = 0; widget && j < widget->num_widgets; j++ ) {
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
        else if ( exact ) {
            break;
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
    const char *name = p->value.link.name;// + (*(p->value.link.name)== '@'?1:0;
    g_info ( "Resolving link to %s", p->value.link.name);
    if ( depth > 20 ) {
        g_warning ( "Found more then 20 redirects for property. Stopping." );
        p->value.link.ref = p;
        return;
    }

    if ( rofi_theme->properties && g_hash_table_contains ( rofi_theme->properties, name ) ) {
        Property *pr = g_hash_table_lookup ( rofi_theme->properties, name );
        g_info ("Resolving link %s found: %s", p->value.link.name, pr->name);
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
    // No found and we have default value.
    if ( p->value.link.def_value ){
        p->value.link.ref = p->value.link.def_value;
        return;
    }

    // No found, set ref to self.
    p->value.link.ref = p;
}

Property *rofi_theme_find_property ( ThemeWidget *widget, PropertyType type, const char *property, gboolean exact )
{
    while ( widget ) {
        if ( widget->properties && g_hash_table_contains ( widget->properties, property ) ) {
            Property *p = g_hash_table_lookup ( widget->properties, property );
            if ( p->type == P_INHERIT ) {
                return p;
            }
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
            // RofiPadding and integer can be converted.
            if ( p->type == P_INTEGER && type == P_PADDING ) {
                return p;
            }
            g_debug ( "Found property: '%s' on '%s', but type %s does not match expected type %s.",
                      property, widget->name,
                      PropertyTypeName[p->type],
                      PropertyTypeName[type]
                      );
        }
        if ( exact ) {
            return NULL;
        }
        // Fall back to defaults.
        widget = widget->parent;
    }
    return NULL;
}
ThemeWidget *rofi_theme_find_widget ( const char *name, const char *state, gboolean exact )
{
    // First find exact match based on name.
    ThemeWidget *widget = rofi_theme_find_single ( rofi_theme, name );
    widget = rofi_theme_find ( widget, state, exact );

    return widget;
}

int rofi_theme_get_position ( const widget *widget, const char *property, int def )
{
    ThemeWidget *wid = rofi_theme_find_widget ( widget->name, widget->state, FALSE );
    Property    *p   = rofi_theme_find_property ( wid, P_POSITION, property, FALSE );
    if ( p ) {
        if ( p->type == P_INHERIT ) {
            if ( widget->parent ) {
                return rofi_theme_get_position ( widget->parent, property, def );
            }
            return def;
        }
        return p->value.i;
    }
    g_debug ( "Theme entry: #%s %s property %s unset.", widget->name, widget->state ? widget->state : "", property );
    return def;
}

int rofi_theme_get_integer ( const widget *widget, const char *property, int def )
{
    ThemeWidget *wid = rofi_theme_find_widget ( widget->name, widget->state, FALSE );
    Property    *p   = rofi_theme_find_property ( wid, P_INTEGER, property, FALSE );
    if ( p ) {
        if ( p->type == P_INHERIT ) {
            if ( widget->parent ) {
                return rofi_theme_get_integer ( widget->parent, property, def );
            }
            return def;
        }
        return p->value.i;
    }
    g_debug ( "Theme entry: #%s %s property %s unset.", widget->name, widget->state ? widget->state : "", property );
    return def;
}
RofiDistance rofi_theme_get_distance ( const widget *widget, const char *property, int def )
{
    ThemeWidget *wid = rofi_theme_find_widget ( widget->name, widget->state, FALSE );
    Property    *p   = rofi_theme_find_property ( wid, P_PADDING, property, FALSE );
    if ( p ) {
        if ( p->type == P_INHERIT ) {
            if ( widget->parent ) {
                return rofi_theme_get_distance ( widget->parent, property, def );
            }
            return (RofiDistance){ def, ROFI_PU_PX, ROFI_HL_SOLID };
        }
        if ( p->type == P_INTEGER ) {
            return (RofiDistance){ p->value.i, ROFI_PU_PX, ROFI_HL_SOLID };
        }
        else {
            return p->value.padding.left;
        }
    }
    g_debug ( "Theme entry: #%s %s property %s unset.", widget->name, widget->state ? widget->state : "", property );
    return (RofiDistance){ def, ROFI_PU_PX, ROFI_HL_SOLID };
}

int rofi_theme_get_boolean ( const widget *widget, const char *property, int def )
{
    ThemeWidget *wid = rofi_theme_find_widget ( widget->name, widget->state, FALSE );
    Property    *p   = rofi_theme_find_property ( wid, P_BOOLEAN, property, FALSE );
    if ( p ) {
        if ( p->type == P_INHERIT ) {
            if ( widget->parent ) {
                return rofi_theme_get_boolean ( widget->parent, property, def );
            }
            return def;
        }
        return p->value.b;
    }
    g_debug ( "Theme entry: #%s %s property %s unset.", widget->name, widget->state ? widget->state : "", property );
    return def;
}
RofiOrientation rofi_theme_get_orientation ( const widget *widget, const char *property, RofiOrientation def )
{
    ThemeWidget *wid = rofi_theme_find_widget ( widget->name, widget->state, FALSE );
    Property    *p   = rofi_theme_find_property ( wid, P_ORIENTATION, property, FALSE );
    if ( p ) {
        if ( p->type == P_INHERIT ) {
            if ( widget->parent ) {
                return rofi_theme_get_orientation ( widget->parent, property, def );
            }
            return def;
        }
        return p->value.b;
    }
    g_debug ( "Theme entry: #%s %s property %s unset.", widget->name, widget->state ? widget->state : "", property );
    return def;
}

const char *rofi_theme_get_string ( const widget *widget, const char *property, const char *def )
{
    ThemeWidget *wid = rofi_theme_find_widget ( widget->name, widget->state, FALSE );
    Property    *p   = rofi_theme_find_property ( wid, P_STRING, property, FALSE );
    if ( p ) {
        if ( p->type == P_INHERIT ) {
            if ( widget->parent ) {
                return rofi_theme_get_string ( widget->parent, property, def );
            }
            return def;
        }
        return p->value.s;
    }
    g_debug ( "Theme entry: #%s %s property %s unset.", widget->name, widget->state ? widget->state : "", property );
    return def;
}
double rofi_theme_get_double ( const widget *widget, const char *property, double def )
{
    ThemeWidget *wid = rofi_theme_find_widget ( widget->name, widget->state, FALSE );
    Property    *p   = rofi_theme_find_property ( wid, P_DOUBLE, property, FALSE );
    if ( p ) {
        if ( p->type == P_INHERIT ) {
            if ( widget->parent ) {
                return rofi_theme_get_double ( widget->parent, property, def );
            }
            return def;
        }
        return p->value.f;
    }
    // Fallback to integer if double is not found.
    p = rofi_theme_find_property ( wid, P_INTEGER, property, FALSE );
    if ( p ) {
        if ( p->type == P_INHERIT ) {
            if ( widget->parent ) {
                return rofi_theme_get_double ( widget->parent, property, def );
            }
            return def;
        }
        return (double) p->value.i;
    }
    g_debug ( "Theme entry: #%s %s property %s unset.", widget->name, widget->state ? widget->state : "", property );
    return def;
}
void rofi_theme_get_color ( const widget *widget, const char *property, cairo_t *d )
{
    ThemeWidget *wid = rofi_theme_find_widget ( widget->name, widget->state, FALSE );
    Property    *p   = rofi_theme_find_property ( wid, P_COLOR, property, FALSE );
    if ( p ) {
        if ( p->type == P_INHERIT ) {
            if ( widget->parent ) {
                rofi_theme_get_color ( widget->parent, property, d );
            }
            return;
        }
        cairo_set_source_rgba ( d,
                                p->value.color.red,
                                p->value.color.green,
                                p->value.color.blue,
                                p->value.color.alpha
                                );
    }
    else {
        g_debug ( "Theme entry: #%s %s property %s unset.", widget->name, widget->state ? widget->state : "", property );
    }
}
RofiPadding rofi_theme_get_padding ( const widget *widget, const char *property, RofiPadding pad )
{
    ThemeWidget *wid = rofi_theme_find_widget ( widget->name, widget->state, FALSE );
    Property    *p   = rofi_theme_find_property ( wid, P_PADDING, property, FALSE );
    if ( p ) {
        if ( p->type == P_INHERIT ) {
            if ( widget->parent ) {
                return rofi_theme_get_padding ( widget->parent, property, pad );
            }
            return pad;
        }
        if ( p->type == P_PADDING ) {
            pad = p->value.padding;
        }
        else {
            RofiDistance d = (RofiDistance){ p->value.i, ROFI_PU_PX, ROFI_HL_SOLID };
            return (RofiPadding){ d, d, d, d };
        }
    }
    g_debug ( "Theme entry: #%s %s property %s unset.", widget->name, widget->state ? widget->state : "", property );
    return pad;
}

GList *rofi_theme_get_list ( const widget *widget, const char * property, const char *defaults )
{
    ThemeWidget *wid2 = rofi_theme_find_widget ( widget->name, widget->state, TRUE );
    Property    *p    = rofi_theme_find_property ( wid2, P_LIST, property, TRUE );
    if ( p ) {
        if ( p->type == P_INHERIT ) {
            if ( widget->parent ) {
                return rofi_theme_get_list ( widget->parent, property, defaults );
            }
        }
        else if ( p->type == P_LIST ) {
            return g_list_copy_deep ( p->value.list, rofi_g_list_strdup, NULL );
        }
    }
    char **r = defaults ? g_strsplit ( defaults, ",", 0 ) : NULL;
    if ( r ) {
        GList *l = NULL;
        for ( int i = 0; r[i] != NULL; i++ ) {
            l = g_list_append ( l, r[i] );
        }
        g_free ( r );
        return l;
    }
    return NULL;
}

RofiHighlightColorStyle rofi_theme_get_highlight ( widget *widget, const char *property, RofiHighlightColorStyle th )
{
    ThemeWidget *wid = rofi_theme_find_widget ( widget->name, widget->state, FALSE );
    Property    *p   = rofi_theme_find_property ( wid, P_HIGHLIGHT, property, FALSE );
    if ( p ) {
        if ( p->type == P_INHERIT ) {
            if ( widget->parent ) {
                return rofi_theme_get_highlight ( widget->parent, property, th );
            }
            return th;
        }
        return p->value.highlight;
    }
    g_debug ( "Theme entry: #%s %s property %s unset.", widget->name, widget->state ? widget->state : "", property );
    return th;
}

int distance_get_pixel ( RofiDistance d, RofiOrientation ori )
{
    if ( d.type == ROFI_PU_EM ) {
        return d.distance * textbox_get_estimated_char_height ();
    }
    else if ( d.type == ROFI_PU_CH ) {
        return d.distance * textbox_get_estimated_ch ();
    }
    else if ( d.type == ROFI_PU_PERCENT ) {
        if ( ori == ROFI_ORIENTATION_VERTICAL ) {
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

void distance_get_linestyle ( RofiDistance d, cairo_t *draw )
{
    if ( d.style == ROFI_HL_DASH ) {
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

static char * rofi_theme_convert_color ( char *col )
{
    char *r = g_strstrip ( col );
    if ( *r == '#' && strlen ( r ) == 9 ) {
        char a1 = r[1];
        char a2 = r[2];
        r[1] = r[3];
        r[2] = r[4];
        r[3] = r[5];
        r[4] = r[6];
        r[5] = r[7];
        r[6] = r[8];
        r[7] = a1;
        r[8] = a2;
    }

    return r;
}
void rofi_theme_convert_old ( void )
{
    {
        char *str = g_strdup_printf ( "#window { border: %d; padding: %d;}", config.menu_bw, config.padding );
        rofi_theme_parse_string ( str );
        g_free ( str );
    }
    if ( config.color_window ) {
        char               **retv = g_strsplit ( config.color_window, ",", -1 );
        const char * const conf[] = {
            "* { background: %s; }",
            "* { border-color: %s; }",
            "* { separatorcolor: %s; }"
        };
        for ( int i = 0; retv && i < 3 && retv[i]; i++ ) {
            char *str = g_strdup_printf ( conf[i], rofi_theme_convert_color ( retv[i] ) );
            rofi_theme_parse_string ( str );
            g_free ( str );
        }
        g_strfreev ( retv );
    }
    if ( config.color_normal ) {
        char               **retv = g_strsplit ( config.color_normal, ",", -1 );
        const char * const conf[] = {
            "* { normal-background: %s; }",
            "* { foreground: %s; normal-foreground: @foreground; alternate-normal-foreground: @foreground; }",
            "* { alternate-normal-background: %s; }",
            "* { selected-normal-background: %s; }",
            "* { selected-normal-foreground: %s; }"
        };
        for ( int i = 0; retv && retv[i] && i < 5; i++ ) {
            char *str = g_strdup_printf ( conf[i], rofi_theme_convert_color ( retv[i] ) );
            rofi_theme_parse_string ( str );
            g_free ( str );
        }
        g_strfreev ( retv );
    }
    if ( config.color_urgent ) {
        char               **retv = g_strsplit ( config.color_urgent, ",", -1 );
        const char * const conf[] = {
            "* { urgent-background: %s; }",
            "* { urgent-foreground: %s; alternate-urgent-foreground: @urgent-foreground;}",
            "* { alternate-urgent-background: %s; }",
            "* { selected-urgent-background: %s; }",
            "* { selected-urgent-foreground: %s; }"
        };
        for ( int i = 0; retv && retv[i] && i < 5; i++ ) {
            char *str = g_strdup_printf ( conf[i], rofi_theme_convert_color ( retv[i] ) );
            rofi_theme_parse_string ( str );
            g_free ( str );
        }
        g_strfreev ( retv );
    }
    if ( config.color_active ) {
        char               **retv = g_strsplit ( config.color_active, ",", -1 );
        const char * const conf[] = {
            "* { active-background: %s; }",
            "* { active-foreground: %s; alternate-active-foreground: @active-foreground;}",
            "* { alternate-active-background: %s; }",
            "* { selected-active-background: %s; }",
            "* { selected-active-foreground: %s; }"
        };
        for ( int i = 0; retv && retv[i] && i < 5; i++ ) {
            char *str = g_strdup_printf ( conf[i], rofi_theme_convert_color ( retv[i] ) );
            rofi_theme_parse_string ( str );
            g_free ( str );
        }
        g_strfreev ( retv );
    }

    if ( config.separator_style != NULL  ) {
        if ( g_strcmp0 ( config.separator_style, "none" ) == 0 ) {
            const char *const str = "#listview { border: 0px; }";
            rofi_theme_parse_string ( str );
            const char *const str2 = "#mode-switcher { border: 0px; }";
            rofi_theme_parse_string ( str2 );
            const char *const str3 = "#message { border: 0px; }";
            rofi_theme_parse_string ( str3 );
        }
        else if  ( g_strcmp0 ( config.separator_style, "solid" ) == 0 ) {
            const char *const str = "#listview { border: 2px solid 0px 0px 0px; }";
            rofi_theme_parse_string ( str );
            const char *const str2 = "#mode-switcher { border: 2px solid 0px 0px 0px; }";
            rofi_theme_parse_string ( str2 );
            const char *const str3 = "#message { border: 2px solid 0px 0px 0px; }";
            rofi_theme_parse_string ( str3 );
        } /* dash is default */
    }
    /* Line Margin */
    {
        char *str = g_strdup_printf ( "#listview { spacing: %dpx;}", config.line_margin );
        rofi_theme_parse_string ( str );
        g_free ( str );
    }
    /* Line Padding */
    {
        char *str = g_strdup_printf ( "#element, inputbar, message { padding: %dpx;}", config.line_padding );
        rofi_theme_parse_string ( str );
        g_free ( str );
    }
    if ( config.hide_scrollbar ) {
        const char *str = "#listview { scrollbar: false; }";
        rofi_theme_parse_string ( str );
    }
    else {
        const char *str = "#listview { scrollbar: true; }";
        rofi_theme_parse_string ( str );
        char       *str2 = g_strdup_printf ( "#scrollbar { handle-width: %dpx; }", config.scrollbar_width );
        rofi_theme_parse_string ( str2 );
        g_free ( str2 );
    }
    if ( config.fake_transparency ) {
        char *str = g_strdup_printf ( "#window { transparency: \"%s\"; }", config.fake_background );
        rofi_theme_parse_string ( str );
        g_free ( str );
    }
}
#endif // THEME_CONVERTER

char * rofi_theme_parse_prepare_file ( const char *file, const char *parent_file )
{
    char *filename = rofi_expand_path ( file );
    // If no absolute path specified, expand it.
    if ( parent_file != NULL && !g_path_is_absolute ( filename )   ) {
        char *basedir = g_path_get_dirname ( parent_file );
        char *path    = g_build_filename ( basedir, filename, NULL );
        g_free ( filename );
        filename = path;
        g_free ( basedir );
    }
    GFile *gf = g_file_new_for_path ( filename );
    g_free ( filename );
    filename = g_file_get_path ( gf );
    g_object_unref ( gf );

    return filename;
}
