#ifndef THEME_H
#define THEME_H
#include <glib.h>
#include <cairo.h>
typedef enum {
    P_INTEGER,
    P_DOUBLE,
    P_STRING,
    P_BOOLEAN,
    P_COLOR
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

typedef struct {
    char *name;
    PropertyType type;
    union {
        int           i;
        double        f;
        char          *s;
        int           b;
        ThemeColor color;
    } value;
} Property;

typedef struct _Widget {
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

/**
 * Public API
 */

int rofi_theme_get_integer ( const char  *name, const char *property, int def );
int rofi_theme_get_boolean ( const char  *name, const char *property, int def );
char *rofi_theme_get_string ( const char  *name, const char *property, char *def );
double rofi_theme_get_double ( const char *name, const char *property, double def );
void rofi_theme_get_color ( const char  *name, const char *property, cairo_t *d);
#endif
