#ifndef THEME_H
#define THEME_H
#include <glib.h>
#include <cairo.h>
typedef enum {
    P_INTEGER,
    P_DOUBLE,
    P_STRING,
    P_BOOLEAN,
    P_COLOR,
    // Used in padding.
    P_PADDING,
} PropertyType;

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

typedef struct
{
    int left;
    int right;
    int top;
    int bottom;
    gboolean percentual;
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

typedef struct _Widget {
    int set;
    char *name;

    unsigned int num_widgets;
    struct _Widget **widgets;

    GHashTable *properties;

    struct _Widget *parent;
} Widget;


extern Widget *rofi_theme;

Widget *rofi_theme_find_or_create_class ( Widget *base, const char *class );


void rofi_theme_print ( Widget *widget );

Property *rofi_theme_property_create ( PropertyType type );
void rofi_theme_property_free ( Property *p );
void rofi_theme_free ( Widget * );
void rofi_theme_parse_file ( const char *file );
void rofi_theme_widget_add_properties ( Widget *widget, GHashTable *table );

/**
 * Public API
 */

int rofi_theme_get_integer   ( const char *wclass,  const char *name, const char *state,  const char *property, int def );
int rofi_theme_get_boolean   ( const char *wclass,  const char *name, const char *state,  const char *property, int def );
char *rofi_theme_get_string  ( const char *wclass,  const char *name, const char *state,  const char *property, char *def );
double rofi_theme_get_double ( const char *wclass,  const char *name, const char *state,  const char *property, double def );
void rofi_theme_get_color ( const char *wclass, const char  *name, const char *state, const char *property, cairo_t *d);
Padding rofi_theme_get_padding ( const char *wclass, const char  *name, const char *state, const char *property, Padding pad );
#endif
