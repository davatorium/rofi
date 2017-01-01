#ifndef THEME_H
#define THEME_H
#include <glib.h>
#include <cairo.h>

/**
 * Distance unit type.
 */
typedef enum {
    /** PixelWidth in pixels. */
    PW_PX,
    /** PixelWidth in EM. */
    PW_EM,
} PixelWidth;

/**
 * Structure representing a distance.
 */
typedef struct {
    /** Distance */
    double     distance;
    /** Unit type of the distance */
    PixelWidth type;
} Distance;

/**
 * Type of property
 */
typedef enum {
    /** Integer */
    P_INTEGER,
    /** Double */
    P_DOUBLE,
    /** String */
    P_STRING,
    /** Boolean */
    P_BOOLEAN,
    /** Color */
    P_COLOR,
    /** Padding */
    P_PADDING,
} PropertyType;

/**
 * Represent the color in theme.
 */
typedef struct
{
    /** red channel */
    double red;
    /** green channel */
    double green;
    /** blue channel */
    double blue;
    /**  alpha channel */
    double alpha;
} ThemeColor;

/**
 * Padding
 */
typedef struct
{
    Distance top;
    Distance right;
    Distance bottom;
    Distance left;
} Padding;

typedef struct {
    char *name;
    PropertyType type;
    union {
        int           i;
        double        f;
        char          *s;
        int           b;
        ThemeColor color;
        Padding    padding;
    } value;
} Property;
/**
 * ThemeWidget.
 */
typedef struct ThemeWidget {
    int set;
    char *name;

    unsigned int num_widgets;
    struct ThemeWidget **widgets;

    GHashTable *properties;

    struct ThemeWidget *parent;
} ThemeWidget;


/**
 * Global pointer to the current active theme.
 */
extern ThemeWidget *rofi_theme;

/**
 * @param base Handle to the current level in the theme.
 * @param class Name of the new element.
 *
 * Create a new element in the theme structure.
 *
 * @returns handle to the new entry.
 */
ThemeWidget *rofi_theme_find_or_create_class ( ThemeWidget *base, const char *class );

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
 * @param wid
 *
 * Free the widget and alll children.
 */
void rofi_theme_free ( ThemeWidget *wid );

/**
 * @param file filename to parse. 
 *
 * Parse the input theme file.
 */
void rofi_theme_parse_file ( const char *file );

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
 * @param wclass   The widget class
 * @param name     The name class
 * @param state    The widget current state
 * @param property The property to query.
 * @param def      The default value.
 *
 * Obtain the distance of the widget.
 *
 * @returns The distance value of this property for this widget. 
 */
Distance rofi_theme_get_distance ( const char *wclass, const char  *name, const char *state, const char *property, int def );

/**
 * @param wclass   The widget class
 * @param name     The name class
 * @param state    The widget current state
 * @param property The property to query.
 * @param def      The default value.
 *
 * Obtain the integer of the widget.
 *
 * @returns The integer value of this property for this widget. 
 */
int rofi_theme_get_integer   ( const char *wclass,  const char *name, const char *state,  const char *property, int def );

/**
 * @param wclass   The widget class
 * @param name     The name class
 * @param state    The widget current state
 * @param property The property to query.
 * @param def      The default value.
 *
 * Obtain the boolean of the widget.
 *
 * @returns The boolean value of this property for this widget. 
 */
int rofi_theme_get_boolean   ( const char *wclass,  const char *name, const char *state,  const char *property, int def );

/**
 * @param wclass   The widget class
 * @param name     The name class
 * @param state    The widget current state
 * @param property The property to query.
 * @param def      The default value.
 *
 * Obtain the string of the widget.
 *
 * @returns The string value of this property for this widget. 
 */
char *rofi_theme_get_string  ( const char *wclass,  const char *name, const char *state,  const char *property, char *def );

/**
 * @param wclass   The widget class
 * @param name     The name class
 * @param state    The widget current state
 * @param property The property to query.
 * @param def      The default value.
 *
 * Obtain the padding of the widget.
 *
 * @returns The double value of this property for this widget. 
 */
double rofi_theme_get_double ( const char *wclass,  const char *name, const char *state,  const char *property, double def );

/**
 * @param wclass   The widget class
 * @param name     The name class
 * @param state    The widget current state
 * @param property The property to query.
 * @param d        The drawable to apply color. 
 *
 * Obtain the color of the widget and applies this to the drawable d.
 *
 */
void rofi_theme_get_color ( const char *wclass, const char  *name, const char *state, const char *property, cairo_t *d);

/**
 * @param wclass   The widget class
 * @param name     The name class
 * @param state    The widget current state
 * @param property The property to query.
 * @param pad      The default value.
 *
 * Obtain the padding of the widget.
 *
 * @returns The padding of this property for this widget. 
 */
Padding rofi_theme_get_padding ( const char *wclass, const char  *name, const char *state, const char *property, Padding pad );

/**
 * @param d The distance handle.
 *
 * Convert Distance into pixels.
 * @returns the number of pixels this distance represents.
 */
int distance_get_pixel ( Distance d );

#ifdef THEME_CONVERTER
/**
 * Function to convert old theme into new theme format.
 */
void rofi_theme_convert_old_theme ( void );
#endif
#endif
