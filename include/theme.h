#ifndef THEME_H
#define THEME_H
#include <glib.h>
typedef enum {
    P_INTEGER,
    P_FLOAT,
    P_STRING,
    P_BOOLEAN,
    P_COLOR
} PropertyType;

typedef struct {
    char *name;
    PropertyType type;
    union {
        int           i;
        double        f;
        char          *s;
        int           b;
        unsigned int  color;
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
#endif
