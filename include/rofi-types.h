#ifndef INCLUDE_ROFI_TYPES_H
#define INCLUDE_ROFI_TYPES_H
#include <glib.h>
#include <stdint.h>
G_BEGIN_DECLS

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
  /** Character */
  P_CHAR,
  /** Boolean */
  P_BOOLEAN,
  /** Color */
  P_COLOR,
  /** Image */
  P_IMAGE,
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
  /** Cursor */
  P_CURSOR,
  /** Inherit */
  P_INHERIT,
  /** Number of types. */
  P_NUM_TYPES,
} PropertyType;

/**
 * This array maps PropertyType to a user-readable name.
 * It is important this is kept in sync.
 */
extern const char *const PropertyTypeName[P_NUM_TYPES];

/** Style of text highlight */
typedef enum {
  /** no highlight */
  ROFI_HL_NONE = 0,
  /** bold */
  ROFI_HL_BOLD = 1,
  /** underline */
  ROFI_HL_UNDERLINE = 2,
  /** strikethrough */
  ROFI_HL_STRIKETHROUGH = 16,
  /** italic */
  ROFI_HL_ITALIC = 4,
  /** color */
  ROFI_HL_COLOR = 8,
  /** uppercase */
  ROFI_HL_UPPERCASE = 32,
  /** lowercase */
  ROFI_HL_LOWERCASE = 64,
  /** capitalize */
  ROFI_HL_CAPITALIZE = 128
} RofiHighlightStyle;

/** Style of line */
typedef enum {
  /** Solid line */
  ROFI_HL_SOLID,
  /** Dashed line */
  ROFI_HL_DASH
} RofiLineStyle;

/**
 * Distance unit type.
 */
typedef enum {
  /** PixelWidth in pixels. */
  ROFI_PU_PX,
  /** PixelWidth in millimeters. */
  ROFI_PU_MM,
  /** PixelWidth in EM. */
  ROFI_PU_EM,
  /** PixelWidget in percentage */
  ROFI_PU_PERCENT,
  /** PixelWidth in CH. */
  ROFI_PU_CH,
} RofiPixelUnit;

/**
 * Structure representing a distance.
 */
typedef enum {
  ROFI_DISTANCE_MODIFIER_NONE,
  ROFI_DISTANCE_MODIFIER_ADD,
  ROFI_DISTANCE_MODIFIER_SUBTRACT,
  ROFI_DISTANCE_MODIFIER_DIVIDE,
  ROFI_DISTANCE_MODIFIER_MULTIPLY,
  ROFI_DISTANCE_MODIFIER_MODULO,
  ROFI_DISTANCE_MODIFIER_GROUP,
  ROFI_DISTANCE_MODIFIER_MIN,
  ROFI_DISTANCE_MODIFIER_MAX,
  ROFI_DISTANCE_MODIFIER_ROUND,
  ROFI_DISTANCE_MODIFIER_FLOOR,
  ROFI_DISTANCE_MODIFIER_CEIL,
} RofiDistanceModifier;

typedef struct RofiDistanceUnit {
  /** Distance */
  double distance;
  /** Unit type of the distance */
  RofiPixelUnit type;

  /** Type */
  RofiDistanceModifier modtype;

  /** Modifier */
  struct RofiDistanceUnit *left;

  /** Modifier */
  struct RofiDistanceUnit *right;
} RofiDistanceUnit;

typedef struct {
  /** Base */
  RofiDistanceUnit base;
  /** Style of the line (optional)*/
  RofiLineStyle style;
} RofiDistance;

/**
 * Type of orientation.
 */
typedef enum {
  ROFI_ORIENTATION_VERTICAL,
  ROFI_ORIENTATION_HORIZONTAL
} RofiOrientation;

/**
 * Cursor type.
 */
typedef enum {
  ROFI_CURSOR_DEFAULT,
  ROFI_CURSOR_POINTER,
  ROFI_CURSOR_TEXT
} RofiCursorType;

/**
 * Represent the color in theme.
 */
typedef struct {
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
 * Theme Image
 */
typedef enum { ROFI_IMAGE_URL, ROFI_IMAGE_LINEAR_GRADIENT } RofiImageType;

typedef enum {
  ROFI_DIRECTION_LEFT,
  ROFI_DIRECTION_RIGHT,
  ROFI_DIRECTION_TOP,
  ROFI_DIRECTION_BOTTOM,
  ROFI_DIRECTION_ANGLE,
} RofiDirection;

typedef enum {
  ROFI_SCALE_NONE,
  ROFI_SCALE_BOTH,
  ROFI_SCALE_HEIGHT,
  ROFI_SCALE_WIDTH,
} RofiScaleType;

typedef struct {
  RofiImageType type;
  char *url;
  RofiScaleType scaling;
  int wsize;
  int hsize;

  RofiDirection dir;
  double angle;
  /** colors */
  GList *colors;

  /** cached image */
  uint32_t surface_id;

} RofiImage;

/**
 * RofiPadding
 */
typedef struct {
  RofiDistance top;
  RofiDistance right;
  RofiDistance bottom;
  RofiDistance left;
} RofiPadding;

/**
 * Theme highlight.
 */
typedef struct {
  /** style to display */
  RofiHighlightStyle style;
  /** Color */
  ThemeColor color;
} RofiHighlightColorStyle;

/**
 * Enumeration indicating location or gravity of window.
 *
 * \verbatim WL_NORTH_WEST      WL_NORTH      WL_NORTH_EAST \endverbatim
 * \verbatim WL_EAST            WL_CENTER     WL_EAST \endverbatim
 * \verbatim WL_SOUTH_WEST      WL_SOUTH      WL_SOUTH_EAST\endverbatim
 *
 * @ingroup CONFIGURATION
 */
typedef enum {
  /** Center */
  WL_CENTER = 0,
  /** Top middle */
  WL_NORTH = 1,
  /** Middle right */
  WL_EAST = 2,
  /** Bottom middle */
  WL_SOUTH = 4,
  /** Middle left */
  WL_WEST = 8,
  /** Left top corner. */
  WL_NORTH_WEST = WL_NORTH | WL_WEST,
  /** Top right */
  WL_NORTH_EAST = WL_NORTH | WL_EAST,
  /** Bottom right */
  WL_SOUTH_EAST = WL_SOUTH | WL_EAST,
  /** Bottom left */
  WL_SOUTH_WEST = WL_SOUTH | WL_WEST,
} WindowLocation;

typedef union _PropertyValue {
  /** integer */
  int i;
  /** Double */
  double f;
  /** String */
  char *s;
  /** Character */
  char c;
  /** boolean */
  gboolean b;
  /** Color */
  ThemeColor color;
  /** RofiPadding */
  RofiPadding padding;
  /** Reference */
  struct {
    /** Name */
    char *name;
    /** Cached looked up ref */
    struct Property *ref;
    /** Property default */
    struct Property *def_value;
  } link;
  /** Highlight Style */
  RofiHighlightColorStyle highlight;
  /** Image */
  RofiImage image;
  /** List */
  GList *list;
} PropertyValue;

/**
 * Property structure.
 */
typedef struct Property {
  /** Name of property */
  char *name;
  /** Type of property. */
  PropertyType type;
  /** Value */
  PropertyValue value;
} Property;

/**
 * Structure to hold a range.
 */
typedef struct rofi_range_pair {
  int start;
  int stop;
} rofi_range_pair;

/**
 * Internal structure for matching.
 */
typedef struct rofi_int_matcher_t {
  GRegex *regex;
  gboolean invert;
} rofi_int_matcher;

/**
 * Structure with data to process by each worker thread.
 * TODO: Make this more generic wrapper.
 */
typedef struct _thread_state {
  void (*callback)(struct _thread_state *t, gpointer data);
} thread_state;

extern GThreadPool *tpool;

G_END_DECLS
#endif // INCLUDE_ROFI_TYPES_H
