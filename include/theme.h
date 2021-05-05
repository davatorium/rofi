/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2020 Qball Cow <qball@gmpclient.org>
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

#ifndef THEME_H
#define THEME_H
#include <glib.h>
#include <cairo.h>
#include <widgets/widget.h>
#include "rofi-types.h"

/**
 * Describe the media constraint type.
 */
typedef enum
{
    /** Minimum width constraint. */
    THEME_MEDIA_TYPE_MIN_WIDTH,
    /** Maximum width constraint. */
    THEME_MEDIA_TYPE_MAX_WIDTH,
    /** Minimum height constraint. */
    THEME_MEDIA_TYPE_MIN_HEIGHT,
    /** Maximum height constraint. */
    THEME_MEDIA_TYPE_MAX_HEIGHT,
    /** Monitor id constraint. */
    THEME_MEDIA_TYPE_MON_ID,
    /** Minimum aspect ratio constraint. */
    THEME_MEDIA_TYPE_MIN_ASPECT_RATIO,
    /** Maximum aspect ratio constraint. */
    THEME_MEDIA_TYPE_MAX_ASPECT_RATIO,
    /** Invalid entry. */
    THEME_MEDIA_TYPE_INVALID,
} ThemeMediaType;

/**
 * Theme Media description.
 */
typedef struct ThemeMedia
{
    ThemeMediaType type;
    double         value;
} ThemeMedia;

/**
 * ThemeWidget.
 */
typedef struct ThemeWidget
{
    int                set;
    char               *name;

    unsigned int       num_widgets;
    struct ThemeWidget **widgets;

    ThemeMedia         *media;

    GHashTable         *properties;

    struct ThemeWidget *parent;
} ThemeWidget;

/**
 * Global pointer to the current active theme.
 */
extern ThemeWidget *rofi_theme;

/**
 * @param base Handle to the current level in the theme.
 * @param name Name of the new element.
 *
 * Create a new element in the theme structure.
 *
 * @returns handle to the new entry.
 */
ThemeWidget *rofi_theme_find_or_create_name ( ThemeWidget *base, const char *name );

/**
 * @param widget The widget handle.
 *
 * Print out the widget to the commandline.
 */
void rofi_theme_print ( ThemeWidget *widget );

/**
 * @param type The type of the property to create.
 *
 * Create a theme property of type.
 *
 * @returns a new property.
 */
Property *rofi_theme_property_create ( PropertyType type );

/**
 * @param p The property to free.
 *
 * Free the content of the property.
 */
void rofi_theme_property_free ( Property *p );

/**
 * @param p The property to free.
 *
 * @returns a copy of p
 */
Property* rofi_theme_property_copy ( Property *p );
/**
 * @param widget
 *
 * Free the widget and alll children.
 */
void rofi_theme_free ( ThemeWidget *widget );

/**
 * @param file filename to parse.
 *
 * Parse the input theme file.
 *
 * @returns returns TRUE when error.
 */
gboolean rofi_theme_parse_file ( const char *file );

/**
 * @param string to parse.
 *
 * Parse the input string in addition to theme file.
 *
 * @returns returns TRUE when error.
 */
gboolean rofi_theme_parse_string ( const char *string );

/**
 * @param widget The widget handle.
 * @param table HashTable containing properties set.
 *
 * Merge properties with widgets current property.
 */
void rofi_theme_widget_add_properties ( ThemeWidget *widget, GHashTable *table );

/**
 * Public API
 */

/**
 * @param widget   The widget to query
 * @param property The property to query.
 * @param def      The default value.
 *
 * Obtain the distance of the widget.
 *
 * @returns The distance value of this property for this widget.
 */
RofiDistance rofi_theme_get_distance ( const widget *widget, const char *property, int def );

/**
 * @param widget   The widget to query
 * @param property The property to query.
 * @param def      The default value.
 *
 * Obtain the integer of the widget.
 *
 * @returns The integer value of this property for this widget.
 */
int rofi_theme_get_integer   (  const widget *widget, const char *property, int def );

/**
 * @param widget   The widget to query
 * @param property The property to query.
 * @param def      The default value.
 *
 * Obtain the position of the widget.
 *
 * @returns The position value of this property for this widget.
 */
int rofi_theme_get_position ( const widget *widget, const char *property, int def );

/**
 * @param widget   The widget to query
 * @param property The property to query.
 * @param def      The default value.
 *
 * Obtain the boolean of the widget.
 *
 * @returns The boolean value of this property for this widget.
 */
int rofi_theme_get_boolean   (  const widget *widget, const char *property, int def );

/**
 * @param widget   The widget to query
 * @param property The property to query.
 * @param def      The default value.
 *
 * Obtain the orientation indicated by %property of the widget.
 *
 * @returns The orientation of this property for this widget or %def not found.
 */
RofiOrientation rofi_theme_get_orientation ( const widget *widget, const char *property, RofiOrientation def );
/**
 * @param widget   The widget to query
 * @param property The property to query.
 * @param def      The default value.
 *
 * Obtain the string of the widget.
 *
 * @returns The string value of this property for this widget.
 */
const char *rofi_theme_get_string  (  const widget *widget, const char *property, const char *def );

/**
 * @param widget   The widget to query
 * @param property The property to query.
 * @param def      The default value.
 *
 * Obtain the double of the widget.
 *
 * @returns The double value of this property for this widget.
 */
double rofi_theme_get_double (  const widget *widget, const char *property, double def );

/**
 * @param widget   The widget to query
 * @param property The property to query.
 * @param d        The drawable to apply color.
 *
 * Obtain the color of the widget and applies this to the drawable d.
 *
 */
void rofi_theme_get_color ( const widget *widget, const char *property, cairo_t *d );

/**
 * @param widget   The widget to query
 * @param property The property to query.
 *
 * Check if a rofi theme has a property set.
 *
 */
gboolean rofi_theme_has_property ( const widget *widget, const char *property );

/**
 * @param widget   The widget to query
 * @param property The property to query.
 * @param pad      The default value.
 *
 * Obtain the padding of the widget.
 *
 * @returns The padding of this property for this widget.
 */
RofiPadding rofi_theme_get_padding ( const widget *widget, const char *property, RofiPadding pad );

/**
 * @param widget   The widget to query
 * @param property The property to query.
 * @param th The default value.
 *
 * Obtain the highlight .
 *
 * @returns The highlight of this property for this widget.
 */
RofiHighlightColorStyle rofi_theme_get_highlight ( widget *widget, const char *property, RofiHighlightColorStyle th );

/**
 * @param d The distance handle.
 * @param ori The orientation.
 *
 * Convert RofiDistance into pixels.
 * @returns the number of pixels this distance represents.
 */
int distance_get_pixel ( RofiDistance d, RofiOrientation ori );
/**
 * @param d The distance handle.
 * @param draw The cairo drawable.
 *
 * Set linestyle.
 */
void distance_get_linestyle ( RofiDistance d, cairo_t *draw );

/**
 * Low-level functions.
 * These can be used by non-widgets to obtain values.
 */
/**
 * @param name The name of the element to find.
 * @param state The state of the element.
 * @param exact If the match should be exact, or parent can be included.
 *
 * Find the theme element. If not exact, the closest specified element is returned.
 *
 * @returns the ThemeWidget if found, otherwise NULL.
 */
ThemeWidget *rofi_theme_find_widget ( const char *name, const char *state, gboolean exact );

/**
 * @param widget The widget to find the property on.
 * @param type   The %PropertyType to find.
 * @param property The property to find.
 * @param exact  If the property should only be found on this widget, or on parents if not found.
 *
 * Find the property on the widget. If not exact, the parents are searched recursively until match is found.
 *
 * @returns the Property if found, otherwise NULL.
 */
Property *rofi_theme_find_property ( ThemeWidget *widget, PropertyType type, const char *property, gboolean exact );

/**
 * @param widget   The widget to query
 * @param property The property to query.
 * @param defaults The default value.
 *
 * Obtain list of elements (strings) of the widget.
 *
 * @returns a GList holding the names in the list of this property for this widget.
 */
GList *rofi_theme_get_list ( const widget *widget, const char * property, const char *defaults );
/**
 * Checks if a theme is set, or is empty.
 * @returns TRUE when empty.
 */
gboolean rofi_theme_is_empty ( void );

/**
 * Reset the current theme.
 */
void rofi_theme_reset ( void );
#ifdef THEME_CONVERTER
/**
 * Convert old theme colors into default one.
 */
void rofi_theme_convert_old ( void );
#endif

/**
 * @param file File name passed to option.
 *
 * @returns path to theme or copy of filename if not found.
 */
char *helper_get_theme_path ( const char *file );

/**
 * @param file File name to prepare.
 * @param parent_file Filename of parent file.
 *
 * Tries to find full path relative to parent file.
 *
 * @returns full path to file.
 */
char * rofi_theme_parse_prepare_file ( const char *file, const char *parent_file );

/**
 * Process conditionals.
 */
void rofi_theme_parse_process_conditionals ( void );

/**
 * @param parent target theme tree
 * @param child source theme three
 *
 * Merge all the settings from child into parent.
 */
void rofi_theme_parse_merge_widgets ( ThemeWidget *parent, ThemeWidget *child );

/**
 * @param type the media type to parse.
 *
 * Returns the media type described by type.
 */
ThemeMediaType rofi_theme_parse_media_type ( const char *type );

/**
 * @param distance The distance object to copy.
 *
 * @returns a copy of the distance.
 */
RofiDistance rofi_theme_property_copy_distance  ( RofiDistance const distance );

/**
 * @param filename The file to validate.
 *
 * @returns the program exit code.
 */
int rofi_theme_rasi_validate ( const char *filename );

#endif
