#ifndef INCLUDE_ROFI_TYPES_H
#define INCLUDE_ROFI_TYPES_H

/**
 * Type of property
 */
typedef enum
{
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
    /** RofiPadding */
    P_PADDING,
    /** Link to global setting */
    P_LINK,
    /** Position */
    P_POSITION,
    /** Highlight */
    P_HIGHLIGHT,
    /** List */
    P_LIST,
    /** Orientation */
    P_ORIENTATION,
    /** Number of types. */
    P_NUM_TYPES,
} PropertyType;

/**
 * This array maps PropertyType to a user-readable name.
 * It is important this is kept in sync.
 */
extern const char * const PropertyTypeName[P_NUM_TYPES];

#endif // INCLUDE_ROFI_TYPES_H
