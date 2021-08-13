/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2021 Qball Cow <qball@gmpclient.org>
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
#include <math.h>
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
#include "rofi-icon-fetcher.h"

void yyerror ( YYLTYPE *yylloc, const char *, const char * );
static gboolean distance_compare ( RofiDistance d, RofiDistance e )
{
    // TODO UPDATE
    return d.base.type == e.base.type && d.base.distance == e.base.distance && d.style == e.style;
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

static RofiDistanceUnit  *rofi_theme_property_copy_distance_unit ( RofiDistanceUnit *unit )
{
    RofiDistanceUnit *retv = g_slice_new0 ( RofiDistanceUnit );
    *retv = *unit;
    if ( unit->left ) {
        retv->left = rofi_theme_property_copy_distance_unit ( unit->left );
    }
    if ( unit->right ) {
        retv->right = rofi_theme_property_copy_distance_unit ( unit->right );
    }
    return retv;
}
RofiDistance rofi_theme_property_copy_distance  ( RofiDistance const distance )
{
    RofiDistance retv = distance;
    if ( distance.base.left ) {
        retv.base.left = rofi_theme_property_copy_distance_unit ( distance.base.left );
    }
    if ( distance.base.right ) {
        retv.base.right = rofi_theme_property_copy_distance_unit ( distance.base.right );
    }
    return retv;
}

Property* rofi_theme_property_copy ( const Property *p )
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
        if ( p->value.link.def_value ) {
            retv->value.link.def_value = rofi_theme_property_copy ( p->value.link.def_value );
        }
        break;
    case P_PADDING:
    {
        retv->value                = p->value;
        retv->value.padding.top    = rofi_theme_property_copy_distance ( p->value.padding.top );
        retv->value.padding.left   = rofi_theme_property_copy_distance ( p->value.padding.left );
        retv->value.padding.bottom = rofi_theme_property_copy_distance ( p->value.padding.bottom );
        retv->value.padding.right  = rofi_theme_property_copy_distance ( p->value.padding.right );
        break;
    }
    case P_IMAGE:
    {
        retv->value = p->value;
        retv->value.image.url = g_strdup ( p->value.image.url );
        retv->value.image.colors = NULL;
        for ( GList *l = g_list_first ( p->value.image.colors );
                l ; l = g_list_next(l)) {
            retv->value.image.colors = g_list_append ( retv->value.image.colors,
                    g_memdup ( l->data, sizeof(ThemeColor)));
        }
        break;
    }
    default:
        retv->value = p->value;
    }
    return retv;
}

static void rofi_theme_distance_unit_property_free ( RofiDistanceUnit *unit )
{
    if ( unit->left ) {
        rofi_theme_distance_unit_property_free ( unit->left );
        unit->left = NULL;
    }
    if ( unit->right ) {
        rofi_theme_distance_unit_property_free ( unit->right );
        unit->right = NULL;
    }
    g_slice_free ( RofiDistanceUnit, unit );
}
static void rofi_theme_distance_property_free ( RofiDistance *distance )
{
    if ( distance->base.left ) {
        rofi_theme_distance_unit_property_free ( distance->base.left );
        distance->base.left = NULL;
    }
    if ( distance->base.right ) {
        rofi_theme_distance_unit_property_free ( distance->base.right );
        distance->base.right = NULL;
    }
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
    else  if ( p->type == P_PADDING ) {
        rofi_theme_distance_property_free ( &( p->value.padding.top ) );
        rofi_theme_distance_property_free ( &( p->value.padding.right ) );
        rofi_theme_distance_property_free ( &( p->value.padding.bottom ) );
        rofi_theme_distance_property_free ( &( p->value.padding.left ) );
    }
    else if ( p->type == P_IMAGE )  {
        if ( p->value.image.url ) {
            g_free( p->value.image.url );
        }
        if ( p->value.image.colors ) {
            g_list_free_full ( p->value.image.colors, g_free );
        }
    }
    g_slice_free ( Property, p );
}

void rofi_theme_reset ( void )
{
    rofi_theme_free ( rofi_theme );
    rofi_theme       = g_slice_new0 ( ThemeWidget );
    rofi_theme->name = g_strdup ( "Root" );
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
    if ( widget->media ) {
        g_slice_free ( ThemeMedia, widget->media );
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

static void rofi_theme_print_distance_unit ( RofiDistanceUnit *unit )
{
    if ( unit->modtype == ROFI_DISTANCE_MODIFIER_GROUP ) {
        fputs ( "( ", stdout );
    }
    if ( unit->left ) {
        rofi_theme_print_distance_unit ( unit->left );
    }

    if ( unit->modtype == ROFI_DISTANCE_MODIFIER_ADD ) {
        fputs ( " + ", stdout );
    }
    else if ( unit->modtype == ROFI_DISTANCE_MODIFIER_SUBTRACT ) {
        fputs ( " - ", stdout );
    }
    else if ( unit->modtype == ROFI_DISTANCE_MODIFIER_DIVIDE ) {
        fputs ( " / ", stdout );
    }
    else if ( unit->modtype == ROFI_DISTANCE_MODIFIER_MULTIPLY ) {
        fputs ( " * ", stdout );
    }
    else if ( unit->modtype == ROFI_DISTANCE_MODIFIER_MODULO ) {
        fputs ( " % ", stdout );
    }
    else if ( unit->modtype == ROFI_DISTANCE_MODIFIER_MIN ) {
        fputs ( " min ", stdout );
    }
    else if ( unit->modtype == ROFI_DISTANCE_MODIFIER_MAX ) {
        fputs ( " max ", stdout );
    }
    if ( unit->right ) {
        rofi_theme_print_distance_unit ( unit->right );
    }

    if ( unit->modtype == ROFI_DISTANCE_MODIFIER_NONE ) {
        if ( unit->type == ROFI_PU_PX ) {
            printf ( "%upx ", (unsigned int) unit->distance );
        }
        else if ( unit->type == ROFI_PU_MM ) {
            printf_double ( unit->distance );
            fputs ( "mm ", stdout );
        }
        else if ( unit->type == ROFI_PU_PERCENT ) {
            printf_double ( unit->distance );
            fputs ( "% ", stdout );
        }
        else if ( unit->type == ROFI_PU_CH ) {
            printf_double ( unit->distance );
            fputs ( "ch ", stdout );
        }
        else {
            printf_double ( unit->distance );
            fputs ( "em ", stdout );
        }
    }
    if ( unit->modtype == ROFI_DISTANCE_MODIFIER_GROUP ) {
        fputs ( " )", stdout );
    }
}

static void rofi_theme_print_distance ( RofiDistance d )
{
    if ( d.base.modtype == ROFI_DISTANCE_MODIFIER_GROUP ) {
        fputs ( "calc( ", stdout );
    }
    rofi_theme_print_distance_unit ( &( d.base ) );
    if ( d.base.modtype == ROFI_DISTANCE_MODIFIER_GROUP ) {
        fputs ( ")", stdout );
    }
    if ( d.style == ROFI_HL_DASH ) {
        printf ( "dash " );
    }
}
/** Textual representation of RofiCursorType */
const char *const RofiCursorTypeStr[3] = {
    "default",
    "pointer",
    "text",
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
    case P_CURSOR:
        printf ( "%s", RofiCursorTypeStr[p->value.i] );
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
        {
            switch ( p->value.i)
            {
                case WL_CENTER:
                    fputs("center",stdout);
                    break;
                case WL_NORTH:
                    fputs("north",stdout);
                    break;
                case WL_SOUTH:
                    fputs("south",stdout);
                    break;
                case WL_WEST:
                    fputs("west",stdout);
                    break;
                case WL_EAST:
                    fputs("east",stdout);
                    break;
                case WL_NORTH|WL_EAST:
                    fputs("northeast",stdout);
                    break;
                case WL_SOUTH|WL_EAST:
                    fputs("southeast",stdout);
                    break;
                case WL_NORTH|WL_WEST:
                    fputs("northwest",stdout);
                    break;
                case WL_SOUTH|WL_WEST:
                    fputs("southwest",stdout);
                    break;
            }
            break;
        }
    case P_STRING:
        printf ( "\"%s\"", p->value.s );
        break;
    case P_INTEGER:
        printf ( "%d", p->value.i );
        break;
    case P_DOUBLE:
        {
            char sign = (p->value.f < 0);
            int top = (int)fabs(p->value.f);
            int bottom = (fabs(fmod(p->value.f,1.0)))*100;
            printf ( "%s%d.%02d",sign?"-":"", top,bottom);
            break;
        }
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
    case P_IMAGE:
        {
            if ( p->value.image.type == ROFI_IMAGE_URL ) {
                printf("url (\"%s\")", p->value.s );
            } else if ( p->value.image.type == ROFI_IMAGE_LINEAR_GRADIENT ) {
                printf("linear-gradient ( ");
                guint length = g_list_length ( p->value.image.colors);
                guint index   = 0;
                for ( GList *l = g_list_first ( p->value.image.colors); l != NULL; l = g_list_next(l)) {
                    ThemeColor *color = (ThemeColor*)l->data;
                    printf ( "rgba ( %.0f, %.0f, %.0f, %.0f %% )",
                            ( color->red * 255.0 ),
                            ( color->green * 255.0 ),
                            ( color->blue * 255.0 ),
                            ( color->alpha * 100.0 ) );
                    index++;
                    if ( index < length ) {
                        printf(", ");
                    }
                }
                printf(")");
            }

            break;
        }
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
        if ( p->value.link.def_value ) {
            printf ( "var( %s, ", p->value.link.name );
            int_rofi_theme_print_property ( p->value.link.def_value );
            printf ( ")" );
        }
        else {
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

void rofi_theme_print_index ( ThemeWidget *widget, int index )
{
    GHashTableIter iter;
    gpointer       key, value;

    if ( widget->media ) {
        printf ( "%s {\n", widget->name );
        for ( unsigned int i = 0; i < widget->num_widgets; i++ ) {
            rofi_theme_print_index ( widget->widgets[i], index + 4 );
        }
        printf ( "}\n" );
    }
    else {
        if ( widget->properties ) {
            GList       *list = NULL;
            ThemeWidget *w    = widget;
            while ( w ) {
                if ( g_strcmp0 ( w->name, "Root" ) == 0 ) {
                    break;
                }
                if ( w->media ) {
                    break;
                }
                list = g_list_prepend ( list, w->name );
                w    = w->parent;
            }
            if ( g_list_length ( list ) > 0 ) {
                printf ( "%*s", index, "" );
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
                printf ( "%*s* {\n", index, "" );
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
                rofi_theme_print_property_index ( property_name_length, index + 4, p );
            }
            printf ( "%*s}\n", index, "" );
            g_list_free ( list );
        }
        for ( unsigned int i = 0; i < widget->num_widgets; i++ ) {
            rofi_theme_print_index ( widget->widgets[i], index );
        }
    }
}

void rofi_theme_print ( ThemeWidget *widget )
{
    if ( widget != NULL ) {
        printf ( "/**\n * rofi -dump-theme output.\n * Rofi version: %s\n **/\n", PACKAGE_VERSION );
        rofi_theme_print_index ( widget, 0 );
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
    g_info ( "Resolving link to %s", p->value.link.name );
    if ( depth > 20 ) {
        g_warning ( "Found more then 20 redirects for property. Stopping." );
        p->value.link.ref = p;
        return;
    }

    if ( rofi_theme->properties && g_hash_table_contains ( rofi_theme->properties, name ) ) {
        Property *pr = g_hash_table_lookup ( rofi_theme->properties, name );
        g_info ( "Resolving link %s found: %s", p->value.link.name, pr->name );
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
    if ( p->value.link.def_value ) {
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
ThemeWidget *rofi_config_find_widget ( const char *name, const char *state, gboolean exact )
{
    // First find exact match based on name.
    ThemeWidget *widget = rofi_theme_find_single ( rofi_configuration, name );
    widget = rofi_theme_find ( widget, state, exact );

    return widget;
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
            return (RofiDistance){ .base = { def, ROFI_PU_PX, ROFI_DISTANCE_MODIFIER_NONE, NULL, NULL }, .style = ROFI_HL_SOLID };
        }
        if ( p->type == P_INTEGER ) {
            return (RofiDistance){ .base = { p->value.i, ROFI_PU_PX, ROFI_DISTANCE_MODIFIER_NONE, NULL, NULL }, .style = ROFI_HL_SOLID };
        }
        else {
            return p->value.padding.left;
        }
    }
    g_debug ( "Theme entry: #%s %s property %s unset.", widget->name, widget->state ? widget->state : "", property );
    return (RofiDistance){ .base = { def, ROFI_PU_PX, ROFI_DISTANCE_MODIFIER_NONE, NULL, NULL }, .style = ROFI_HL_SOLID };
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
RofiCursorType rofi_theme_get_cursor_type ( const widget *widget, const char *property, RofiCursorType def )
{
    ThemeWidget *wid = rofi_theme_find_widget ( widget->name, widget->state, FALSE );
    Property    *p   = rofi_theme_find_property ( wid, P_CURSOR, property, FALSE );
    if ( p ) {
        if ( p->type == P_INHERIT ) {
            if ( widget->parent ) {
                return rofi_theme_get_cursor_type ( widget->parent, property, def );
            }
            return def;
        }
        return p->value.i;
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
gboolean rofi_theme_get_image ( const widget *widget, const char *property, cairo_t *d )
{
    ThemeWidget *wid = rofi_theme_find_widget ( widget->name, widget->state, FALSE );
    Property    *p   = rofi_theme_find_property ( wid, P_IMAGE , property, FALSE );
    if ( p ) {
        if ( p->type == P_INHERIT ) {
            if ( widget->parent ) {
                return rofi_theme_get_image ( widget->parent, property, d );
            }
            return FALSE;
        }
        if ( p->value.image.type == ROFI_IMAGE_URL ) {
            int wsize = -1;
            int hsize = -1;
            switch ( p->value.image.scaling ) {
              case ROFI_SCALE_BOTH:
                wsize = widget->w;
                hsize = widget->h;
                break;
              case ROFI_SCALE_WIDTH:
                wsize = widget->w;
                break;
              case ROFI_SCALE_HEIGHT:
                hsize = widget->h;
                break;
              case ROFI_SCALE_NONE:
              default:
                break;
            }
            if ( p->value.image.surface_id == 0  || p->value.image.wsize != wsize || p->value.image.hsize != hsize) {
                p->value.image.surface_id = rofi_icon_fetcher_query_advanced ( p->value.image.url, wsize, hsize );
                p->value.image.wsize = wsize;
                p->value.image.hsize = hsize;
            }
            cairo_surface_t *img = rofi_icon_fetcher_get ( p->value.image.surface_id );

            if ( img != NULL ) {
                cairo_pattern_t * pat = cairo_pattern_create_for_surface ( img );
                cairo_pattern_set_extend ( pat, CAIRO_EXTEND_REPEAT );
                cairo_set_source ( d, pat );
                cairo_pattern_destroy ( pat );
                return TRUE;
            }
        } else if ( p->value.image.type == ROFI_IMAGE_LINEAR_GRADIENT ) {
            cairo_pattern_t * pat = NULL;
            switch ( p->value.image.dir ) {
              case ROFI_DIRECTION_RIGHT:
                pat = cairo_pattern_create_linear (0.0,0.0, widget->w, 0.0);
                break;
              case ROFI_DIRECTION_LEFT:
                pat = cairo_pattern_create_linear (widget->w,0.0, 0.0, 0.0);
                break;
              case ROFI_DIRECTION_BOTTOM:
                pat = cairo_pattern_create_linear (0.0,0.0, 0.0, widget->h);
                break;
              case ROFI_DIRECTION_TOP:
                pat = cairo_pattern_create_linear (0.0,widget->h, 0.0, 0.0);
                break;
              case ROFI_DIRECTION_ANGLE:
              {
                double offsety1 = sin(G_PI*2*p->value.image.angle)*(widget->h/2.0);
                double offsetx1 = cos(G_PI*2*p->value.image.angle)*(widget->w/2.0);
                pat = cairo_pattern_create_linear (
                    widget->w/2.0 - offsetx1, widget->h/2.0 - offsety1,
                    widget->w/2.0 + offsetx1, widget->h/2.0 + offsety1 );
                break;
              }
            };
            guint length = g_list_length ( p->value.image.colors );
            if ( length > 1 ) {
                length--;
                guint color_index = 0;
                for ( GList *l = g_list_first ( p->value.image.colors); l != NULL ; l = g_list_next ( l ) )
                {
                    ThemeColor *c = (ThemeColor*) (l->data);
                    cairo_pattern_add_color_stop_rgba ( pat,(color_index)/(double)length,
                            c->red, c->green,
                            c->blue, c->alpha);
                    color_index++;
                }
                cairo_set_source ( d, pat );
                cairo_pattern_destroy ( pat );
                return TRUE;
            } else if ( length == 1 ) {
                ThemeColor *c = (ThemeColor*) (p->value.image.colors->data);
                cairo_pattern_add_color_stop_rgba ( pat,0,
                        c->red, c->green,
                        c->blue, c->alpha);
                cairo_set_source ( d, pat );
                cairo_pattern_destroy ( pat );
                return TRUE;
            }

        }
    }
    else {
        g_debug ( "Theme entry: #%s %s property %s unset.", widget->name, widget->state ? widget->state : "", property );
    }
    return FALSE;
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
            RofiDistance d = (RofiDistance){ .base = { p->value.i, ROFI_PU_PX, ROFI_DISTANCE_MODIFIER_NONE, NULL, NULL }, .style = ROFI_HL_SOLID };
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

static int get_pixels ( RofiDistanceUnit *unit, RofiOrientation ori )
{
    int val = unit->distance;

    if ( unit->type == ROFI_PU_EM ) {
        val = unit->distance * textbox_get_estimated_char_height ();
    }
    else if ( unit->type == ROFI_PU_CH ) {
        val = unit->distance * textbox_get_estimated_ch ();
    }
    else if ( unit->type == ROFI_PU_PERCENT ) {
        if ( ori == ROFI_ORIENTATION_VERTICAL ) {
            int height = 0;
            rofi_view_get_current_monitor ( NULL, &height );
            val = ( unit->distance * height ) / ( 100.0 );
        }
        else {
            int width = 0;
            rofi_view_get_current_monitor ( &width, NULL );
            val = ( unit->distance * width ) / ( 100.0 );
        }
    }
    else if ( unit->type == ROFI_PU_MM ) {
        val = unit->distance * config.dpi / 25.4;
    }
    return val;
}

static int distance_unit_get_pixel ( RofiDistanceUnit *unit, RofiOrientation ori )
{
    switch ( unit->modtype )
    {
    case ROFI_DISTANCE_MODIFIER_GROUP:
        return distance_unit_get_pixel ( unit->left, ori );
        break;
    case ROFI_DISTANCE_MODIFIER_ADD:
        return distance_unit_get_pixel ( unit->left, ori ) + distance_unit_get_pixel ( unit->right, ori );
    case ROFI_DISTANCE_MODIFIER_SUBTRACT:
        return distance_unit_get_pixel ( unit->left, ori ) - distance_unit_get_pixel ( unit->right, ori );
    case ROFI_DISTANCE_MODIFIER_MULTIPLY:
        return distance_unit_get_pixel ( unit->left, ori ) * distance_unit_get_pixel ( unit->right, ori );
    case ROFI_DISTANCE_MODIFIER_DIVIDE:
    {
        int a = distance_unit_get_pixel ( unit->left, ori );
        int b = distance_unit_get_pixel ( unit->right, ori );
        if ( b != 0 ) {
            return a / b;
        }
        return a;
    }
    case ROFI_DISTANCE_MODIFIER_MODULO:
    {
        int a = distance_unit_get_pixel ( unit->left, ori );
        int b = distance_unit_get_pixel ( unit->right, ori );
        if ( b != 0 ) {
            return a % b;
        }
        return 0;
    }
    case ROFI_DISTANCE_MODIFIER_MIN:
    {
        int a = distance_unit_get_pixel ( unit->left, ori );
        int b = distance_unit_get_pixel ( unit->right, ori );
        return MIN ( a, b );
    }
    case ROFI_DISTANCE_MODIFIER_MAX:
    {
        int a = distance_unit_get_pixel ( unit->left, ori );
        int b = distance_unit_get_pixel ( unit->right, ori );
        return MAX ( a, b );
    }
    default:
        break;
    }
    return get_pixels ( unit, ori );
}

int distance_get_pixel ( RofiDistance d, RofiOrientation ori )
{
    return distance_unit_get_pixel ( &( d.base ), ori );
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
    if ( rofi_theme->num_widgets == 3 ) {
        // HACK: check for default added elements.
        for ( unsigned int i = 0; i < rofi_theme->num_widgets; i++ ) {
            if ( strncmp ( rofi_theme->widgets[i]->name, "element", 7 ) != 0 ) {
                return FALSE;
            }
        }
        return TRUE;
    }

    return FALSE;
}

char * rofi_theme_parse_prepare_file ( const char *file, const char *parent_file )
{
    char *filename = rofi_expand_path ( file );
    // If no absolute path specified, expand it.
    if ( parent_file != NULL && !g_path_is_absolute ( filename ) ) {
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

static void rofi_theme_parse_merge_widgets_no_media ( ThemeWidget *parent, ThemeWidget *child )
{
    g_assert ( parent != NULL );
    g_assert ( child != NULL );

    if ( parent == rofi_theme && g_strcmp0 ( child->name, "*" ) == 0 ) {
        rofi_theme_widget_add_properties ( parent, child->properties );
        return;
    }

    ThemeWidget *w = rofi_theme_find_or_create_name ( parent, child->name );
    rofi_theme_widget_add_properties ( w, child->properties );
    for ( unsigned int i = 0; i < child->num_widgets; i++ ) {
        rofi_theme_parse_merge_widgets_no_media ( w, child->widgets[i] );
    }
}
void rofi_theme_parse_merge_widgets ( ThemeWidget *parent, ThemeWidget *child )
{
    g_assert ( parent != NULL );
    g_assert ( child != NULL );

    if ( parent == rofi_theme && g_strcmp0 ( child->name, "*" ) == 0 ) {
        rofi_theme_widget_add_properties ( parent, child->properties );
        return;
    }

    ThemeWidget *w = rofi_theme_find_or_create_name ( parent, child->name );
    if ( child->media ) {
        w->media      = g_slice_new0 ( ThemeMedia );
        *( w->media ) = *( child->media );
    }
    rofi_theme_widget_add_properties ( w, child->properties );
    for ( unsigned int i = 0; i < child->num_widgets; i++ ) {
        rofi_theme_parse_merge_widgets ( w, child->widgets[i] );
    }
}

static void  rofi_theme_parse_process_conditionals_int ( workarea mon, ThemeWidget *rwidget )
{
    if ( rwidget == NULL ) {
        return;
    }
    for ( unsigned int i = 0; i < rwidget->num_widgets; i++ ) {
        ThemeWidget *widget = rwidget->widgets[i];
        rofi_theme_parse_process_conditionals_int ( mon, widget );
        if ( widget->media != NULL ) {
            switch ( widget->media->type )
            {
            case THEME_MEDIA_TYPE_MIN_WIDTH:
            {
                int w = widget->media->value;
                if ( mon.w >= w ) {
                    for ( unsigned int x = 0; x < widget->num_widgets; x++ ) {
                        rofi_theme_parse_merge_widgets_no_media ( rofi_theme, widget->widgets[x] );
                    }
                }
                break;
            }
            case THEME_MEDIA_TYPE_MAX_WIDTH:
            {
                int w = widget->media->value;
                if ( mon.w < w ) {
                    for ( unsigned int x = 0; x < widget->num_widgets; x++ ) {
                        rofi_theme_parse_merge_widgets_no_media ( rofi_theme, widget->widgets[x] );
                    }
                }
                break;
            }
            case THEME_MEDIA_TYPE_MIN_HEIGHT:
            {
                int h = widget->media->value;
                if ( mon.h >= h ) {
                    for ( unsigned int x = 0; x < widget->num_widgets; x++ ) {
                        rofi_theme_parse_merge_widgets_no_media ( rofi_theme, widget->widgets[x] );
                    }
                }
                break;
            }
            case THEME_MEDIA_TYPE_MAX_HEIGHT:
            {
                int h = widget->media->value;
                if ( mon.h < h ) {
                    for ( unsigned int x = 0; x < widget->num_widgets; x++ ) {
                        rofi_theme_parse_merge_widgets_no_media ( rofi_theme, widget->widgets[x] );
                    }
                }
                break;
            }
            case THEME_MEDIA_TYPE_MON_ID:
            {
                if ( mon.monitor_id == widget->media->value ) {
                    for ( unsigned int x = 0; x < widget->num_widgets; x++ ) {
                        rofi_theme_parse_merge_widgets_no_media ( rofi_theme, widget->widgets[x] );
                    }
                }
                break;
            }
            case THEME_MEDIA_TYPE_MIN_ASPECT_RATIO:
            {
                double r = widget->media->value;
                if ( ( mon.w / (double) mon.h ) >= r ) {
                    for ( unsigned int x = 0; x < widget->num_widgets; x++ ) {
                        rofi_theme_parse_merge_widgets_no_media ( rofi_theme, widget->widgets[x] );
                    }
                }
                break;
            }
            case THEME_MEDIA_TYPE_MAX_ASPECT_RATIO:
            {
                double r = widget->media->value;
                if ( ( mon.w / (double) mon.h ) < r ) {
                    for ( unsigned int x = 0; x < widget->num_widgets; x++ ) {
                        rofi_theme_parse_merge_widgets_no_media ( rofi_theme, widget->widgets[x] );
                    }
                }
                break;
            }
            default:
            {
                break;
            }
            }
        }
    }
}
void  rofi_theme_parse_process_conditionals ( void )
{
    workarea mon;
    monitor_active ( &mon );
    rofi_theme_parse_process_conditionals_int ( mon, rofi_theme );
}

ThemeMediaType rofi_theme_parse_media_type ( const char *type )
{
    if ( g_strcmp0 ( type, "monitor-id" ) == 0 ) {
        return THEME_MEDIA_TYPE_MON_ID;
    }
    else if ( g_strcmp0 ( type, "min-width" ) == 0 ) {
        return THEME_MEDIA_TYPE_MIN_WIDTH;
    }
    else if ( g_strcmp0 ( type, "min-height" ) == 0 ) {
        return THEME_MEDIA_TYPE_MIN_HEIGHT;
    }
    else if ( g_strcmp0 ( type, "max-width" ) == 0 ) {
        return THEME_MEDIA_TYPE_MAX_WIDTH;
    }
    else if ( g_strcmp0 ( type, "max-height" ) == 0 ) {
        return THEME_MEDIA_TYPE_MAX_HEIGHT;
    }
    else if ( g_strcmp0 ( type, "min-aspect-ratio" ) == 0 ) {
        return THEME_MEDIA_TYPE_MIN_ASPECT_RATIO;
    }
    else if ( g_strcmp0 ( type, "max-aspect-ratio" ) == 0 ) {
        return THEME_MEDIA_TYPE_MAX_ASPECT_RATIO;
    }
    return THEME_MEDIA_TYPE_INVALID;
}

gboolean rofi_theme_has_property ( const widget *widget, const char *property )
{
    ThemeWidget *wid = rofi_theme_find_widget ( widget->name, widget->state, FALSE );
    Property    *p   = rofi_theme_find_property ( wid, P_STRING, property, FALSE );
    if ( p ) {
        if ( p->type == P_INHERIT ) {
            if ( widget->parent ) {
                return rofi_theme_has_property ( widget->parent, property );
            }
            return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}
