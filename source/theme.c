/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2023 Qball Cow <qball@gmpclient.org>
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
#define G_LOG_DOMAIN "Theme"

#include "config.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// GFile stuff.
#include "helper.h"
#include "rofi-icon-fetcher.h"
#include "rofi-types.h"
#include "rofi.h"
#include "settings.h"
#include "theme-parser.h"
#include "theme.h"
#include "view.h"
#include "widgets/textbox.h"
#include <gio/gio.h>

/**
 * list of config files we parsed.
 */
GList *parsed_config_files = NULL;

/** cleanup (free) the list of parsed config files. */
void rofi_theme_free_parsed_files(void) {
  g_list_free_full(parsed_config_files, g_free);
  parsed_config_files = NULL;
}

/**
 * @param is_term if print to terminal
 *
 * print the list of parsed config files.
 */
void rofi_theme_print_parsed_files(gboolean is_term) {
  printf("\nParsed files:\n");
  for (GList *iter = g_list_first(parsed_config_files); iter != NULL;
       iter = g_list_next(iter)) {
    printf("\t\u2023 %s%s%s\n", is_term ? color_bold : "",
           (const char *)(iter->data), is_term ? color_reset : "");
  }
  printf("\n");
}

void yyerror(YYLTYPE *yylloc, const char *, const char *);
static gboolean distance_compare(RofiDistance d, RofiDistance e) {
  // TODO UPDATE
  return d.base.type == e.base.type && d.base.distance == e.base.distance &&
         d.style == e.style;
}

ThemeWidget *rofi_theme_find_or_create_name(ThemeWidget *base,
                                            const char *name) {
  for (unsigned int i = 0; i < base->num_widgets; i++) {
    if (g_strcmp0(base->widgets[i]->name, name) == 0) {
      return base->widgets[i];
    }
  }

  base->widgets =
      g_realloc(base->widgets, sizeof(ThemeWidget *) * (base->num_widgets + 1));
  base->widgets[base->num_widgets] = g_slice_new0(ThemeWidget);
  ThemeWidget *retv = base->widgets[base->num_widgets];
  retv->parent = base;
  retv->name = g_strdup(name);
  base->num_widgets++;
  return retv;
}
/**
 *  Properties
 */
Property *rofi_theme_property_create(PropertyType type) {
  Property *retv = g_slice_new0(Property);
  retv->type = type;
  return retv;
}

static RofiDistanceUnit *
rofi_theme_property_copy_distance_unit(RofiDistanceUnit *unit) {
  RofiDistanceUnit *retv = g_slice_new0(RofiDistanceUnit);
  *retv = *unit;
  if (unit->left) {
    retv->left = rofi_theme_property_copy_distance_unit(unit->left);
  }
  if (unit->right) {
    retv->right = rofi_theme_property_copy_distance_unit(unit->right);
  }
  return retv;
}
RofiDistance rofi_theme_property_copy_distance(RofiDistance const distance) {
  RofiDistance retv = distance;
  if (distance.base.left) {
    retv.base.left = rofi_theme_property_copy_distance_unit(distance.base.left);
  }
  if (distance.base.right) {
    retv.base.right =
        rofi_theme_property_copy_distance_unit(distance.base.right);
  }
  return retv;
}

Property *rofi_theme_property_copy(const Property *p,
                                   G_GNUC_UNUSED void *data) {
  Property *retv = rofi_theme_property_create(p->type);
  retv->name = g_strdup(p->name);

  switch (p->type) {
  case P_STRING:
    retv->value.s = g_strdup(p->value.s);
    break;
  case P_LIST:
    retv->value.list = g_list_copy_deep(
        p->value.list, (GCopyFunc)rofi_theme_property_copy, NULL);
    break;
  case P_LINK:
    retv->value.link.name = g_strdup(p->value.link.name);
    retv->value.link.ref = NULL;
    if (p->value.link.def_value) {
      retv->value.link.def_value =
          rofi_theme_property_copy(p->value.link.def_value, NULL);
    }
    break;
  case P_PADDING: {
    retv->value = p->value;
    retv->value.padding.top =
        rofi_theme_property_copy_distance(p->value.padding.top);
    retv->value.padding.left =
        rofi_theme_property_copy_distance(p->value.padding.left);
    retv->value.padding.bottom =
        rofi_theme_property_copy_distance(p->value.padding.bottom);
    retv->value.padding.right =
        rofi_theme_property_copy_distance(p->value.padding.right);
    break;
  }
  case P_IMAGE: {
    retv->value = p->value;
    retv->value.image.url = g_strdup(p->value.image.url);
    retv->value.image.colors = NULL;
    for (GList *l = g_list_first(p->value.image.colors); l;
         l = g_list_next(l)) {
      retv->value.image.colors = g_list_append(
          retv->value.image.colors, g_memdup2(l->data, sizeof(ThemeColor)));
    }
    break;
  }
  default:
    retv->value = p->value;
  }
  return retv;
}

static void rofi_theme_distance_unit_property_free(RofiDistanceUnit *unit) {
  if (unit->left) {
    rofi_theme_distance_unit_property_free(unit->left);
    unit->left = NULL;
  }
  if (unit->right) {
    rofi_theme_distance_unit_property_free(unit->right);
    unit->right = NULL;
  }
  g_slice_free(RofiDistanceUnit, unit);
}
static void rofi_theme_distance_property_free(RofiDistance *distance) {
  if (distance->base.left) {
    rofi_theme_distance_unit_property_free(distance->base.left);
    distance->base.left = NULL;
  }
  if (distance->base.right) {
    rofi_theme_distance_unit_property_free(distance->base.right);
    distance->base.right = NULL;
  }
}

void rofi_theme_property_free(Property *p) {
  if (p == NULL) {
    return;
  }
  g_free(p->name);
  if (p->type == P_STRING) {
    g_free(p->value.s);
  } else if (p->type == P_LIST) {
    g_list_free_full(p->value.list, (GDestroyNotify)rofi_theme_property_free);
    p->value.list = 0;
  } else if (p->type == P_LINK) {
    g_free(p->value.link.name);
    if (p->value.link.def_value) {
      rofi_theme_property_free(p->value.link.def_value);
    }
  } else if (p->type == P_PADDING) {
    rofi_theme_distance_property_free(&(p->value.padding.top));
    rofi_theme_distance_property_free(&(p->value.padding.right));
    rofi_theme_distance_property_free(&(p->value.padding.bottom));
    rofi_theme_distance_property_free(&(p->value.padding.left));
  } else if (p->type == P_IMAGE) {
    if (p->value.image.url) {
      g_free(p->value.image.url);
    }
    if (p->value.image.colors) {
      g_list_free_full(p->value.image.colors, g_free);
    }
  }
  g_slice_free(Property, p);
}

void rofi_theme_reset(void) {
  rofi_theme_free(rofi_theme);
  rofi_theme = g_slice_new0(ThemeWidget);
  rofi_theme->name = g_strdup("Root");
}

void rofi_theme_free(ThemeWidget *wid) {
  if (wid == NULL) {
    return;
  }
  if (wid->properties) {
    g_hash_table_destroy(wid->properties);
    wid->properties = NULL;
  }
  if (wid->media) {
    g_slice_free(ThemeMedia, wid->media);
  }
  for (unsigned int i = 0; i < wid->num_widgets; i++) {
    rofi_theme_free(wid->widgets[i]);
  }
  g_free(wid->widgets);
  g_free(wid->name);
  g_slice_free(ThemeWidget, wid);
}

/**
 * print
 */
inline static void printf_double(double d) {
  char buf[G_ASCII_DTOSTR_BUF_SIZE + 1] = {
      0,
  };
  g_ascii_formatd(buf, G_ASCII_DTOSTR_BUF_SIZE, "%.4f", d);
  fputs(buf, stdout);
}

static void rofi_theme_print_distance_unit(RofiDistanceUnit *unit) {
  if (unit->modtype == ROFI_DISTANCE_MODIFIER_GROUP) {
    fputs("( ", stdout);
  }
  if (unit->left) {
    rofi_theme_print_distance_unit(unit->left);
  }

  if (unit->modtype == ROFI_DISTANCE_MODIFIER_ADD) {
    fputs(" + ", stdout);
  } else if (unit->modtype == ROFI_DISTANCE_MODIFIER_SUBTRACT) {
    fputs(" - ", stdout);
  } else if (unit->modtype == ROFI_DISTANCE_MODIFIER_DIVIDE) {
    fputs(" / ", stdout);
  } else if (unit->modtype == ROFI_DISTANCE_MODIFIER_MULTIPLY) {
    fputs(" * ", stdout);
  } else if (unit->modtype == ROFI_DISTANCE_MODIFIER_MODULO) {
    fputs(" modulo ", stdout);
  } else if (unit->modtype == ROFI_DISTANCE_MODIFIER_MIN) {
    fputs(" min ", stdout);
  } else if (unit->modtype == ROFI_DISTANCE_MODIFIER_MAX) {
    fputs(" max ", stdout);
  } else if (unit->modtype == ROFI_DISTANCE_MODIFIER_ROUND) {
    fputs(" round ", stdout);
  } else if (unit->modtype == ROFI_DISTANCE_MODIFIER_FLOOR) {
    fputs(" floor ", stdout);
  } else if (unit->modtype == ROFI_DISTANCE_MODIFIER_CEIL) {
    fputs(" ceil ", stdout);
  }
  if (unit->right) {
    rofi_theme_print_distance_unit(unit->right);
  }

  if (unit->modtype == ROFI_DISTANCE_MODIFIER_NONE) {
    if (unit->type == ROFI_PU_PX) {
      printf("%upx ", (unsigned int)unit->distance);
    } else if (unit->type == ROFI_PU_MM) {
      printf_double(unit->distance);
      fputs("mm ", stdout);
    } else if (unit->type == ROFI_PU_PERCENT) {
      printf_double(unit->distance);
      fputs("% ", stdout);
    } else if (unit->type == ROFI_PU_CH) {
      printf_double(unit->distance);
      fputs("ch ", stdout);
    } else {
      printf_double(unit->distance);
      fputs("em ", stdout);
    }
  }
  if (unit->modtype == ROFI_DISTANCE_MODIFIER_GROUP) {
    fputs(" )", stdout);
  }
}

static void rofi_theme_print_color(ThemeColor color) {
  uint8_t r, g, b;
  g = 255 * color.green;
  r = 255 * color.red;
  b = 255 * color.blue;
  if (color.alpha < 0.00001) {
    printf("transparent");
    return;
  }
  for (uint32_t x = 0; x < num_CSSColors; x++) {
    if (CSSColors[x].r == r && CSSColors[x].g == g && CSSColors[x].b == b) {
      printf("%s", CSSColors[x].name);
      if (color.alpha < 1) {
        printf("/%.0f%%", color.alpha * 100.0);
      }
      return;
    }
  }
  printf("rgba ( %.0f, %.0f, %.0f, %.0f %% )", (color.red * 255.0),
         (color.green * 255.0), (color.blue * 255.0), (color.alpha * 100.0));
}
static void rofi_theme_print_distance(RofiDistance d) {
  if (d.base.modtype == ROFI_DISTANCE_MODIFIER_GROUP) {
    fputs("calc( ", stdout);
  }
  rofi_theme_print_distance_unit(&(d.base));
  if (d.base.modtype == ROFI_DISTANCE_MODIFIER_GROUP) {
    fputs(")", stdout);
  }
  if (d.style == ROFI_HL_DASH) {
    printf("dash ");
  }
}
/** Textual representation of RofiCursorType */
const char *const RofiCursorTypeStr[3] = {
    "default",
    "pointer",
    "text",
};

static void int_rofi_theme_print_property(Property *p) {
  switch (p->type) {
  case P_LIST:
    printf("[ ");
    for (GList *iter = p->value.list; iter != NULL; iter = g_list_next(iter)) {
      int_rofi_theme_print_property((Property *)iter->data);
      if (iter->next != NULL) {
        printf(",");
      }
    }
    printf(" ]");
    break;
  case P_ORIENTATION:
    printf("%s", (p->value.i == ROFI_ORIENTATION_HORIZONTAL) ? "horizontal"
                                                             : "vertical");
    break;
  case P_CURSOR:
    printf("%s", RofiCursorTypeStr[p->value.i]);
    break;
  case P_HIGHLIGHT:
    if (p->value.highlight.style & ROFI_HL_BOLD) {
      printf("bold ");
    }
    if (p->value.highlight.style & ROFI_HL_UNDERLINE) {
      printf("underline ");
    }
    if (p->value.highlight.style & ROFI_HL_STRIKETHROUGH) {
      printf("strikethrough ");
    }
    if (p->value.highlight.style & ROFI_HL_ITALIC) {
      printf("italic ");
    }
    if (p->value.highlight.style & ROFI_HL_UPPERCASE) {
      printf("uppercase ");
    }
    if (p->value.highlight.style & ROFI_HL_LOWERCASE) {
      printf("lowercase ");
    }
    if (p->value.highlight.style & ROFI_HL_CAPITALIZE) {
      printf("capitalize ");
    }
    if (p->value.highlight.style & ROFI_HL_COLOR) {
      rofi_theme_print_color(p->value.highlight.color);
    }
    break;
  case P_POSITION: {
    switch (p->value.i) {
    case WL_CENTER:
      fputs("center", stdout);
      break;
    case WL_NORTH:
      fputs("north", stdout);
      break;
    case WL_SOUTH:
      fputs("south", stdout);
      break;
    case WL_WEST:
      fputs("west", stdout);
      break;
    case WL_EAST:
      fputs("east", stdout);
      break;
    case WL_NORTH | WL_EAST:
      fputs("northeast", stdout);
      break;
    case WL_SOUTH | WL_EAST:
      fputs("southeast", stdout);
      break;
    case WL_NORTH | WL_WEST:
      fputs("northwest", stdout);
      break;
    case WL_SOUTH | WL_WEST:
      fputs("southwest", stdout);
      break;
    }
    break;
  }
  case P_STRING:
    printf("\"%s\"", p->value.s);
    break;
  case P_INTEGER:
    printf("%d", p->value.i);
    break;
  case P_DOUBLE: {
    char sign = (p->value.f < 0);
    int top = (int)fabs(p->value.f);
    int bottom = (fabs(fmod(p->value.f, 1.0))) * 100;
    printf("%s%d.%02d", sign ? "-" : "", top, bottom);
    break;
  }
  case P_BOOLEAN:
    printf("%s", p->value.b ? "true" : "false");
    break;
  case P_COLOR:
    rofi_theme_print_color(p->value.color);
    break;
  case P_IMAGE: {
    if (p->value.image.type == ROFI_IMAGE_URL) {
      printf("url (\"%s\")", p->value.s);
    } else if (p->value.image.type == ROFI_IMAGE_LINEAR_GRADIENT) {
      printf("linear-gradient ( ");
      guint length = g_list_length(p->value.image.colors);
      guint index = 0;
      for (GList *l = g_list_first(p->value.image.colors); l != NULL;
           l = g_list_next(l)) {
        ThemeColor *color = (ThemeColor *)l->data;
        rofi_theme_print_color(*color);
        index++;
        if (index < length) {
          printf(", ");
        }
      }
      printf(")");
    }

    break;
  }
  case P_PADDING:
    if (distance_compare(p->value.padding.top, p->value.padding.bottom) &&
        distance_compare(p->value.padding.left, p->value.padding.right) &&
        distance_compare(p->value.padding.left, p->value.padding.top)) {
      rofi_theme_print_distance(p->value.padding.left);
    } else if (distance_compare(p->value.padding.top,
                                p->value.padding.bottom) &&
               distance_compare(p->value.padding.left,
                                p->value.padding.right)) {
      rofi_theme_print_distance(p->value.padding.top);
      rofi_theme_print_distance(p->value.padding.left);
    } else if (!distance_compare(p->value.padding.top,
                                 p->value.padding.bottom) &&
               distance_compare(p->value.padding.left,
                                p->value.padding.right)) {
      rofi_theme_print_distance(p->value.padding.top);
      rofi_theme_print_distance(p->value.padding.left);
      rofi_theme_print_distance(p->value.padding.bottom);
    } else {
      rofi_theme_print_distance(p->value.padding.top);
      rofi_theme_print_distance(p->value.padding.right);
      rofi_theme_print_distance(p->value.padding.bottom);
      rofi_theme_print_distance(p->value.padding.left);
    }
    break;
  case P_LINK:
    if (p->value.link.def_value) {
      printf("var( %s, ", p->value.link.name);
      int_rofi_theme_print_property(p->value.link.def_value);
      printf(")");
    } else {
      printf("var(%s)", p->value.link.name);
    }
    break;
  case P_INHERIT:
    printf("inherit");
    break;
  default:
    break;
  }
}

static void rofi_theme_print_property_index(size_t pnl, int cur_depth,
                                            Property *p) {
  int pl = strlen(p->name);
  printf("%*s%s:%*s ", cur_depth, "", p->name, (int)pnl - pl, "");
  int_rofi_theme_print_property(p);
  putchar(';');
  putchar('\n');
}

void rofi_theme_print_index(ThemeWidget *wid, int index) {
  GHashTableIter iter;
  gpointer key, value;

  if (wid->media) {
    printf("%s {\n", wid->name);
    for (unsigned int i = 0; i < wid->num_widgets; i++) {
      rofi_theme_print_index(wid->widgets[i], index + 4);
    }
    printf("}\n");
  } else {
    if (wid->properties) {
      GList *list = NULL;
      ThemeWidget *w = wid;
      while (w) {
        if (g_strcmp0(w->name, "Root") == 0) {
          break;
        }
        if (w->media) {
          break;
        }
        list = g_list_prepend(list, w->name);
        w = w->parent;
      }
      if (g_list_length(list) > 0) {
        printf("%*s", index, "");
        for (GList *citer = g_list_first(list); citer != NULL;
             citer = g_list_next(citer)) {
          char *name = (char *)citer->data;
          fputs(name, stdout);
          if (citer->prev == NULL && citer->next) {
            putchar(' ');
          } else if (citer->next) {
            putchar('.');
          }
        }
        printf(" {\n");
      } else {
        printf("%*s* {\n", index, "");
      }
      size_t property_name_length = 0;
      g_hash_table_iter_init(&iter, wid->properties);
      while (g_hash_table_iter_next(&iter, &key, &value)) {
        Property *pv = (Property *)value;
        property_name_length = MAX(strlen(pv->name), property_name_length);
      }
      g_hash_table_iter_init(&iter, wid->properties);
      while (g_hash_table_iter_next(&iter, &key, &value)) {
        Property *pv = (Property *)value;
        rofi_theme_print_property_index(property_name_length, index + 4, pv);
      }
      printf("%*s}\n", index, "");
      g_list_free(list);
    }
    for (unsigned int i = 0; i < wid->num_widgets; i++) {
      rofi_theme_print_index(wid->widgets[i], index);
    }
  }
}

void rofi_theme_print(ThemeWidget *wid) {
  if (wid != NULL) {
    printf("/**\n * rofi -dump-theme output.\n * Rofi version: %s\n **/\n",
           PACKAGE_VERSION);
    rofi_theme_print_index(wid, 0);
  }
}

/**
 * Destroy the internal of lex parser.
 */
void yylex_destroy(void);

/**
 * Global handle input file to flex parser.
 */
extern FILE *yyin;

/**
 * @param yylloc The file location.
 * @param what   What we are parsing, filename or string.
 * @param s      Error message string.
 *
 * Error handler for the lex parser.
 */
void yyerror(YYLTYPE *yylloc, const char *what, const char *s) {
  char *what_esc = what ? g_markup_escape_text(what, -1) : g_strdup("");
  GString *str = g_string_new("");
  g_string_printf(str,
                  "<big><b>Error while parsing theme:</b></big> <i>%s</i>\n",
                  what_esc);
  g_free(what_esc);
  char *esc = g_markup_escape_text(s, -1);
  g_string_append_printf(
      str,
      "\tParser error: <span size=\"smaller\" style=\"italic\">%s</span>\n",
      esc);
  g_free(esc);
  if (yylloc->filename != NULL) {
    g_string_append_printf(
        str,
        "\tLocation:     line %d column %d to line %d column %d.\n"
        "\tFile          '%s'\n",
        yylloc->first_line, yylloc->first_column, yylloc->last_line,
        yylloc->last_column, yylloc->filename);
  } else {
    g_string_append_printf(
        str, "\tLocation:     line %d column %d to line %d column %d\n",
        yylloc->first_line, yylloc->first_column, yylloc->last_line,
        yylloc->last_column);
  }
  g_log("Parser", G_LOG_LEVEL_DEBUG, "Failed to parse theme:\n%s", str->str);
  rofi_add_error_message(str);
}

static void rofi_theme_copy_property_int(G_GNUC_UNUSED gpointer key,
                                         gpointer value, gpointer user_data) {
  GHashTable *table = (GHashTable *)user_data;
  Property *p = rofi_theme_property_copy((Property *)value, NULL);
  g_hash_table_replace(table, p->name, p);
}
void rofi_theme_widget_add_properties(ThemeWidget *wid, GHashTable *table) {
  if (table == NULL) {
    return;
  }
  if (wid->properties == NULL) {
    wid->properties =
        g_hash_table_new_full(g_str_hash, g_str_equal, NULL,
                              (GDestroyNotify)rofi_theme_property_free);
  }
  g_hash_table_foreach(table, rofi_theme_copy_property_int, wid->properties);
}

/**
 * Public API
 */

static inline ThemeWidget *rofi_theme_find_single(ThemeWidget *wid,
                                                  const char *name) {
  for (unsigned int j = 0; wid && j < wid->num_widgets; j++) {
    if (g_strcmp0(wid->widgets[j]->name, name) == 0) {
      return wid->widgets[j];
    }
  }
  return wid;
}

static ThemeWidget *rofi_theme_find(ThemeWidget *wid, const char *name,
                                    const gboolean exact) {
  if (wid == NULL || name == NULL) {
    return wid;
  }
  char *tname = g_strdup(name);
  char *saveptr = NULL;
  int found = TRUE;
  for (const char *iter = strtok_r(tname, ".", &saveptr); iter != NULL;
       iter = strtok_r(NULL, ".", &saveptr)) {
    found = FALSE;
    ThemeWidget *f = rofi_theme_find_single(wid, iter);
    if (f != wid) {
      wid = f;
      found = TRUE;
    } else if (exact) {
      break;
    }
  }
  g_free(tname);
  if (!exact || found) {
    return wid;
  }
  return NULL;
}

static void rofi_theme_resolve_link_property(Property *p, int cur_depth) {
  // Set name, remove '@' prefix.
  const char *name = p->value.link.name; // + (*(p->value.link.name)== '@'?1:0;
  g_info("Resolving link to %s", p->value.link.name);
  if (cur_depth > 20) {
    g_warning("Found more then 20 redirects for property. Stopping.");
    p->value.link.ref = p;
    return;
  }

  if (rofi_theme->properties &&
      g_hash_table_contains(rofi_theme->properties, name)) {
    Property *pr = g_hash_table_lookup(rofi_theme->properties, name);
    g_info("Resolving link %s found: %s", p->value.link.name, pr->name);
    if (pr->type == P_LINK) {
      if (pr->value.link.ref == NULL) {
        rofi_theme_resolve_link_property(pr, cur_depth + 1);
      }
      if (pr->value.link.ref != pr) {
        p->value.link.ref = pr->value.link.ref;
        return;
      }
    } else {
      p->value.link.ref = pr;
      return;
    }
  }
  // No found and we have default value.
  if (p->value.link.def_value) {
    p->value.link.ref = p->value.link.def_value;
    return;
  }

  // No found, set ref to self.
  p->value.link.ref = p;
}

Property *rofi_theme_find_property(ThemeWidget *wid, PropertyType type,
                                   const char *property, gboolean exact) {
  while (wid) {
    if (wid->properties && g_hash_table_contains(wid->properties, property)) {
      Property *p = g_hash_table_lookup(wid->properties, property);
      if (p->type == P_INHERIT) {
        return p;
      }
      if (p->type == P_LINK) {
        if (p->value.link.ref == NULL) {
          // Resolve link.
          rofi_theme_resolve_link_property(p, 0);
        }
        if (p->value.link.ref != NULL && p->value.link.ref->type == type) {
          return p->value.link.ref;
        }
      }
      if (p->type == type) {
        return p;
      }
      // RofiPadding and integer can be converted.
      if (p->type == P_INTEGER && type == P_PADDING) {
        return p;
      }
      g_debug("Found property: '%s' on '%s', but type %s does not match "
              "expected type %s.",
              property, wid->name, PropertyTypeName[p->type],
              PropertyTypeName[type]);
    }
    if (exact) {
      return NULL;
    }
    // Fall back to defaults.
    wid = wid->parent;
  }
  return NULL;
}
ThemeWidget *rofi_config_find_widget(const char *name, const char *state,
                                     gboolean exact) {
  // First find exact match based on name.
  ThemeWidget *wid = rofi_theme_find_single(rofi_configuration, name);
  wid = rofi_theme_find(wid, state, exact);

  return wid;
}
ThemeWidget *rofi_theme_find_widget(const char *name, const char *state,
                                    gboolean exact) {
  // First find exact match based on name.
  ThemeWidget *wid = rofi_theme_find_single(rofi_theme, name);
  wid = rofi_theme_find(wid, state, exact);

  return wid;
}

static int rofi_theme_get_position_inside(Property *p, const widget *wid,
                                          const char *property, int def) {
  if (p) {
    if (p->type == P_INHERIT) {
      if (wid->parent) {
        ThemeWidget *parent =
            rofi_theme_find_widget(wid->parent->name, wid->state, FALSE);
        Property *pv =
            rofi_theme_find_property(parent, P_POSITION, property, FALSE);
        return rofi_theme_get_position_inside(pv, wid->parent, property, def);
      }
      return def;
    }
    return p->value.i;
  }
  g_debug("Theme entry: #%s %s property %s unset.", wid->name,
          wid->state ? wid->state : "", property);
  return def;
}
int rofi_theme_get_position(const widget *wid, const char *property, int def) {
  ThemeWidget *wid_find = rofi_theme_find_widget(wid->name, wid->state, FALSE);
  Property *p = rofi_theme_find_property(wid_find, P_POSITION, property, FALSE);
  return rofi_theme_get_position_inside(p, wid, property, def);
}
static int rofi_theme_get_integer_inside(Property *p, const widget *wid,
                                         const char *property, int def) {
  if (p) {
    if (p->type == P_INHERIT) {
      if (wid->parent) {
        ThemeWidget *parent =
            rofi_theme_find_widget(wid->parent->name, wid->state, FALSE);
        Property *pv =
            rofi_theme_find_property(parent, P_INTEGER, property, FALSE);
        return rofi_theme_get_integer_inside(pv, wid->parent, property, def);
      }
      return def;
    }
    return p->value.i;
  }
  g_debug("Theme entry: #%s %s property %s unset.", wid->name,
          wid->state ? wid->state : "", property);
  return def;
}
int rofi_theme_get_integer(const widget *wid, const char *property, int def) {
  ThemeWidget *wid_find = rofi_theme_find_widget(wid->name, wid->state, FALSE);
  Property *p = rofi_theme_find_property(wid_find, P_INTEGER, property, FALSE);
  return (int)rofi_theme_get_integer_inside(p, wid, property, (double)def);
}
static RofiDistance rofi_theme_get_distance_inside(Property *p,
                                                   const widget *wid,
                                                   const char *property,
                                                   int def) {
  if (p) {
    if (p->type == P_INHERIT) {
      if (wid->parent) {
        ThemeWidget *parent =
            rofi_theme_find_widget(wid->parent->name, wid->state, FALSE);
        Property *pv =
            rofi_theme_find_property(parent, P_PADDING, property, FALSE);
        return rofi_theme_get_distance_inside(pv, wid->parent, property, def);
      }
      return (RofiDistance){
          .base = {def, ROFI_PU_PX, ROFI_DISTANCE_MODIFIER_NONE, NULL, NULL},
          .style = ROFI_HL_SOLID};
    }
    if (p->type == P_INTEGER) {
      return (RofiDistance){.base = {p->value.i, ROFI_PU_PX,
                                     ROFI_DISTANCE_MODIFIER_NONE, NULL, NULL},
                            .style = ROFI_HL_SOLID};
    }
    return p->value.padding.left;
  }
  g_debug("Theme entry: #%s %s property %s unset.", wid->name,
          wid->state ? wid->state : "", property);
  return (RofiDistance){
      .base = {def, ROFI_PU_PX, ROFI_DISTANCE_MODIFIER_NONE, NULL, NULL},
      .style = ROFI_HL_SOLID};
}
RofiDistance rofi_theme_get_distance(const widget *wid, const char *property,
                                     int def) {
  ThemeWidget *wid_find = rofi_theme_find_widget(wid->name, wid->state, FALSE);
  Property *p = rofi_theme_find_property(wid_find, P_PADDING, property, FALSE);
  return rofi_theme_get_distance_inside(p, wid, property, def);
}

static int rofi_theme_get_boolean_inside(Property *p, const widget *wid,
                                         const char *property, int def) {
  if (p) {
    if (p->type == P_INHERIT) {
      if (wid->parent) {
        ThemeWidget *parent =
            rofi_theme_find_widget(wid->parent->name, wid->state, FALSE);
        Property *pv =
            rofi_theme_find_property(parent, P_BOOLEAN, property, FALSE);
        return rofi_theme_get_boolean_inside(pv, wid->parent, property, def);
      }
      return def;
    }
    return p->value.b;
  }
  g_debug("Theme entry: #%s %s property %s unset.", wid->name,
          wid->state ? wid->state : "", property);
  return def;
}
int rofi_theme_get_boolean(const widget *wid, const char *property, int def) {
  ThemeWidget *wid_find = rofi_theme_find_widget(wid->name, wid->state, FALSE);
  Property *p = rofi_theme_find_property(wid_find, P_BOOLEAN, property, FALSE);
  return rofi_theme_get_boolean_inside(p, wid, property, def);
}

static RofiOrientation rofi_theme_get_orientation_inside(Property *p,
                                                         const widget *wid,
                                                         const char *property,
                                                         RofiOrientation def) {
  if (p) {
    if (p->type == P_INHERIT) {
      if (wid->parent) {
        ThemeWidget *parent =
            rofi_theme_find_widget(wid->parent->name, wid->state, FALSE);
        Property *pv =
            rofi_theme_find_property(parent, P_ORIENTATION, property, FALSE);
        return rofi_theme_get_orientation_inside(pv, wid->parent, property,
                                                 def);
      }
      return def;
    }
    return p->value.b;
  }
  g_debug("Theme entry: #%s %s property %s unset.", wid->name,
          wid->state ? wid->state : "", property);
  return def;
}
RofiOrientation rofi_theme_get_orientation(const widget *wid,
                                           const char *property,
                                           RofiOrientation def) {
  ThemeWidget *wid_find = rofi_theme_find_widget(wid->name, wid->state, FALSE);
  Property *p =
      rofi_theme_find_property(wid_find, P_ORIENTATION, property, FALSE);
  return rofi_theme_get_orientation_inside(p, wid, property, def);
}

static RofiCursorType rofi_theme_get_cursor_type_inside(Property *p,
                                                        const widget *wid,
                                                        const char *property,
                                                        RofiCursorType def) {
  if (p) {
    if (p->type == P_INHERIT) {
      if (wid->parent) {
        ThemeWidget *parent =
            rofi_theme_find_widget(wid->parent->name, wid->state, FALSE);
        Property *pv =
            rofi_theme_find_property(parent, P_CURSOR, property, FALSE);
        return rofi_theme_get_cursor_type_inside(pv, wid->parent, property,
                                                 def);
      }
      return def;
    }
    return p->value.i;
  }
  g_debug("Theme entry: #%s %s property %s unset.", wid->name,
          wid->state ? wid->state : "", property);
  return def;
}
RofiCursorType rofi_theme_get_cursor_type(const widget *wid,
                                          const char *property,
                                          RofiCursorType def) {
  ThemeWidget *wid_find = rofi_theme_find_widget(wid->name, wid->state, FALSE);
  Property *p = rofi_theme_find_property(wid_find, P_CURSOR, property, FALSE);
  return rofi_theme_get_cursor_type_inside(p, wid, property, def);
}
static const char *rofi_theme_get_string_inside(Property *p, const widget *wid,
                                                const char *property,
                                                const char *def) {
  if (p) {
    if (p->type == P_INHERIT) {
      if (wid->parent) {
        ThemeWidget *parent =
            rofi_theme_find_widget(wid->parent->name, wid->state, FALSE);
        Property *pv =
            rofi_theme_find_property(parent, P_STRING, property, FALSE);
        return rofi_theme_get_string_inside(pv, wid->parent, property, def);
      }
      return def;
    }
    return p->value.s;
  }
  g_debug("Theme entry: #%s %s property %s unset.", wid->name,
          wid->state ? wid->state : "", property);
  return def;
}
const char *rofi_theme_get_string(const widget *wid, const char *property,
                                  const char *def) {
  ThemeWidget *wid_find = rofi_theme_find_widget(wid->name, wid->state, FALSE);
  Property *p = rofi_theme_find_property(wid_find, P_STRING, property, FALSE);
  return rofi_theme_get_string_inside(p, wid, property, def);
}

static double rofi_theme_get_double_integer_fb_inside(Property *p,
                                                      const widget *wid,
                                                      const char *property,
                                                      double def) {
  if (p) {
    if (p->type == P_INHERIT) {
      if (wid->parent) {
        ThemeWidget *parent =
            rofi_theme_find_widget(wid->parent->name, wid->state, FALSE);
        Property *pv =
            rofi_theme_find_property(parent, P_INTEGER, property, FALSE);
        return rofi_theme_get_double_integer_fb_inside(pv, wid->parent,
                                                       property, def);
      }
      return def;
    }
    return p->value.i;
  }
  g_debug("Theme entry: #%s %s property %s unset.", wid->name,
          wid->state ? wid->state : "", property);
  return def;
}
static double rofi_theme_get_double_inside(const widget *orig, Property *p,
                                           const widget *wid,
                                           const char *property, double def) {
  if (p) {
    if (p->type == P_INHERIT) {
      if (wid->parent) {
        ThemeWidget *parent =
            rofi_theme_find_widget(wid->parent->name, wid->state, FALSE);
        Property *pv =
            rofi_theme_find_property(parent, P_DOUBLE, property, FALSE);
        return rofi_theme_get_double_inside(orig, pv, wid->parent, property,
                                            def);
      }
      return def;
    }
    return p->value.f;
  }
  ThemeWidget *wid_find = rofi_theme_find_widget(orig->name, wid->state, FALSE);
  // Fallback to integer if double is not found.
  p = rofi_theme_find_property(wid_find, P_INTEGER, property, FALSE);
  return rofi_theme_get_double_integer_fb_inside(p, wid, property, def);
}
double rofi_theme_get_double(const widget *wid, const char *property,
                             double def) {
  ThemeWidget *wid_find = rofi_theme_find_widget(wid->name, wid->state, FALSE);
  Property *p = rofi_theme_find_property(wid_find, P_DOUBLE, property, FALSE);
  return rofi_theme_get_double_inside(wid, p, wid, property, def);
}
static void rofi_theme_get_color_inside(const widget *wid, Property *p,
                                        const char *property, cairo_t *d) {
  if (p) {
    if (p->type == P_INHERIT) {
      if (wid->parent) {
        ThemeWidget *parent =
            rofi_theme_find_widget(wid->parent->name, wid->state, FALSE);
        Property *pv =
            rofi_theme_find_property(parent, P_COLOR, property, FALSE);
        rofi_theme_get_color_inside(wid->parent, pv, property, d);
      }
      return;
    }
    cairo_set_source_rgba(d, p->value.color.red, p->value.color.green,
                          p->value.color.blue, p->value.color.alpha);
  } else {
    g_debug("Theme entry: #%s %s property %s unset.", wid->name,
            wid->state ? wid->state : "", property);
  }
}

void rofi_theme_get_color(const widget *wid, const char *property, cairo_t *d) {
  ThemeWidget *wid_find = rofi_theme_find_widget(wid->name, wid->state, FALSE);
  Property *p = rofi_theme_find_property(wid_find, P_COLOR, property, FALSE);
  rofi_theme_get_color_inside(wid, p, property, d);
}

static gboolean rofi_theme_get_image_inside(Property *p, const widget *wid,
                                            const char *property, cairo_t *d) {
  if (p) {
    if (p->type == P_INHERIT) {
      if (wid->parent) {
        ThemeWidget *parent =
            rofi_theme_find_widget(wid->parent->name, wid->state, FALSE);
        Property *pv =
            rofi_theme_find_property(parent, P_IMAGE, property, FALSE);
        return rofi_theme_get_image_inside(pv, wid->parent, property, d);
      }
      return FALSE;
    }
    if (p->value.image.type == ROFI_IMAGE_URL) {
      int wsize = -1;
      int hsize = -1;
      switch (p->value.image.scaling) {
      case ROFI_SCALE_BOTH:
        wsize = wid->w;
        hsize = wid->h;
        break;
      case ROFI_SCALE_WIDTH:
        wsize = wid->w;
        break;
      case ROFI_SCALE_HEIGHT:
        hsize = wid->h;
        break;
      case ROFI_SCALE_NONE:
      default:
        break;
      }
      if (p->value.image.surface_id == 0 || p->value.image.wsize != wsize ||
          p->value.image.hsize != hsize) {
        p->value.image.surface_id =
            rofi_icon_fetcher_query_advanced(p->value.image.url, wsize, hsize);
        p->value.image.wsize = wsize;
        p->value.image.hsize = hsize;
      }
      cairo_surface_t *img = rofi_icon_fetcher_get(p->value.image.surface_id);

      if (img != NULL) {
        cairo_pattern_t *pat = cairo_pattern_create_for_surface(img);
        cairo_pattern_set_extend(pat, CAIRO_EXTEND_REPEAT);
        cairo_set_source(d, pat);
        cairo_pattern_destroy(pat);
        return TRUE;
      }
    } else if (p->value.image.type == ROFI_IMAGE_LINEAR_GRADIENT) {
      cairo_pattern_t *pat = NULL;
      switch (p->value.image.dir) {
      case ROFI_DIRECTION_RIGHT:
        pat = cairo_pattern_create_linear(0.0, 0.0, wid->w, 0.0);
        break;
      case ROFI_DIRECTION_LEFT:
        pat = cairo_pattern_create_linear(wid->w, 0.0, 0.0, 0.0);
        break;
      case ROFI_DIRECTION_BOTTOM:
        pat = cairo_pattern_create_linear(0.0, 0.0, 0.0, wid->h);
        break;
      case ROFI_DIRECTION_TOP:
        pat = cairo_pattern_create_linear(0.0, wid->h, 0.0, 0.0);
        break;
      case ROFI_DIRECTION_ANGLE: {
        double offsety1 = sin(G_PI * 2 * p->value.image.angle) * (wid->h / 2.0);
        double offsetx1 = cos(G_PI * 2 * p->value.image.angle) * (wid->w / 2.0);
        pat = cairo_pattern_create_linear(
            wid->w / 2.0 - offsetx1, wid->h / 2.0 - offsety1,
            wid->w / 2.0 + offsetx1, wid->h / 2.0 + offsety1);
        break;
      }
      };
      guint length = g_list_length(p->value.image.colors);
      if (length > 1) {
        length--;
        guint color_index = 0;
        for (GList *l = g_list_first(p->value.image.colors); l != NULL;
             l = g_list_next(l)) {
          ThemeColor *c = (ThemeColor *)(l->data);
          cairo_pattern_add_color_stop_rgba(pat, (color_index) / (double)length,
                                            c->red, c->green, c->blue,
                                            c->alpha);
          color_index++;
        }
        cairo_set_source(d, pat);
        cairo_pattern_destroy(pat);
        return TRUE;
      }
      if (length == 1) {
        ThemeColor *c = (ThemeColor *)(p->value.image.colors->data);
        cairo_pattern_add_color_stop_rgba(pat, 0, c->red, c->green, c->blue,
                                          c->alpha);
        cairo_set_source(d, pat);
        cairo_pattern_destroy(pat);
        return TRUE;
      }
    }
  } else {
    g_debug("Theme entry: #%s %s property %s unset.", wid->name,
            wid->state ? wid->state : "", property);
  }
  return FALSE;
}
gboolean rofi_theme_get_image(const widget *wid, const char *property,
                              cairo_t *d) {
  ThemeWidget *wid_find = rofi_theme_find_widget(wid->name, wid->state, FALSE);
  Property *p = rofi_theme_find_property(wid_find, P_IMAGE, property, FALSE);
  return rofi_theme_get_image_inside(p, wid, property, d);
}
static RofiPadding rofi_theme_get_padding_inside(Property *p, const widget *wid,
                                                 const char *property,
                                                 RofiPadding pad) {
  if (p) {
    if (p->type == P_INHERIT) {
      if (wid->parent) {
        ThemeWidget *parent =
            rofi_theme_find_widget(wid->parent->name, wid->state, FALSE);
        Property *pv =
            rofi_theme_find_property(parent, P_PADDING, property, FALSE);
        return rofi_theme_get_padding_inside(pv, wid->parent, property, pad);
      }
      return pad;
    }
    if (p->type == P_PADDING) {
      pad = p->value.padding;
    } else {
      RofiDistance d =
          (RofiDistance){.base = {p->value.i, ROFI_PU_PX,
                                  ROFI_DISTANCE_MODIFIER_NONE, NULL, NULL},
                         .style = ROFI_HL_SOLID};
      return (RofiPadding){d, d, d, d};
    }
  }
  g_debug("Theme entry: #%s %s property %s unset.", wid->name,
          wid->state ? wid->state : "", property);
  return pad;
}
RofiPadding rofi_theme_get_padding(const widget *wid, const char *property,
                                   RofiPadding pad) {
  ThemeWidget *wid_find = rofi_theme_find_widget(wid->name, wid->state, FALSE);
  Property *p = rofi_theme_find_property(wid_find, P_PADDING, property, FALSE);
  return rofi_theme_get_padding_inside(p, wid, property, pad);
}

static GList *rofi_theme_get_list_inside(Property *p, const widget *wid,
                                         const char *property,
                                         PropertyType child_type) {
  if (p) {
    if (p->type == P_INHERIT) {
      if (wid->parent) {
        ThemeWidget *parent =
            rofi_theme_find_widget(wid->parent->name, wid->state, FALSE);
        Property *pv =
            rofi_theme_find_property(parent, P_LIST, property, FALSE);
        return rofi_theme_get_list_inside(pv, wid->parent, property,
                                          child_type);
      }
    } else if (p->type == P_LIST) {
      return p->value.list;
    }
  }
  return NULL;
}
GList *rofi_theme_get_list_distance(const widget *wid, const char *property) {
  ThemeWidget *wid2 = rofi_theme_find_widget(wid->name, wid->state, FALSE);
  Property *p = rofi_theme_find_property(wid2, P_LIST, property, FALSE);
  GList *list = rofi_theme_get_list_inside(p, wid, property, P_PADDING);
  GList *retv = NULL;
  for (GList *iter = g_list_first(list); iter != NULL;
       iter = g_list_next(iter)) {
    Property *prop = (Property *)(iter->data);
    if (prop->type == P_PADDING) {
      RofiDistance *pnew = g_new0(RofiDistance, 1);
      *pnew = prop->value.padding.left;
      retv = g_list_append(retv, pnew);
    } else if (prop->type == P_INTEGER) {
      RofiDistance *pnew = g_new0(RofiDistance, 1);
      RofiDistance d =
          (RofiDistance){.base = {prop->value.i, ROFI_PU_PX,
                                  ROFI_DISTANCE_MODIFIER_NONE, NULL, NULL},
                         .style = ROFI_HL_SOLID};
      *pnew = d;
      retv = g_list_append(retv, pnew);
    } else {
      g_warning("Invalid type detected in list.");
    }
  }
  return retv;
}
GList *rofi_theme_get_list_strings(const widget *wid, const char *property) {
  ThemeWidget *wid2 = rofi_theme_find_widget(wid->name, wid->state, FALSE);
  Property *p = rofi_theme_find_property(wid2, P_LIST, property, FALSE);
  GList *list = rofi_theme_get_list_inside(p, wid, property, P_STRING);
  GList *retv = NULL;
  for (GList *iter = g_list_first(list); iter != NULL;
       iter = g_list_next(iter)) {
    Property *prop = (Property *)(iter->data);
    if (prop->type == P_STRING) {
      retv = g_list_append(retv, g_strdup(prop->value.s));
    } else {
      g_warning("Invalid type detected in list.");
    }
  }
  return retv;
}

static RofiHighlightColorStyle
rofi_theme_get_highlight_inside(Property *p, widget *wid, const char *property,
                                RofiHighlightColorStyle th) {
  if (p) {
    if (p->type == P_INHERIT) {
      if (wid->parent) {
        ThemeWidget *parent =
            rofi_theme_find_widget(wid->parent->name, wid->state, FALSE);
        Property *pv =
            rofi_theme_find_property(parent, P_HIGHLIGHT, property, FALSE);
        return rofi_theme_get_highlight_inside(pv, wid->parent, property, th);
      }
      return th;
    } else if (p->type == P_COLOR) {
      th.style = ROFI_HL_NONE | ROFI_HL_COLOR;
      th.color = p->value.color;
      return th;
    }

    return p->value.highlight;
  } else {
    ThemeWidget *find_wid =
        rofi_theme_find_widget(wid->name, wid->state, FALSE);
    Property *p2 = rofi_theme_find_property(find_wid, P_COLOR, property, FALSE);
    if (p2 != NULL) {
      return rofi_theme_get_highlight_inside(p2, wid, property, th);
    }
    return th;
  }
  g_debug("Theme entry: #%s %s property %s unset.", wid->name,
          wid->state ? wid->state : "", property);
  return th;
}
RofiHighlightColorStyle rofi_theme_get_highlight(widget *wid,
                                                 const char *property,
                                                 RofiHighlightColorStyle th) {
  ThemeWidget *found_wid = rofi_theme_find_widget(wid->name, wid->state, FALSE);
  Property *p =
      rofi_theme_find_property(found_wid, P_HIGHLIGHT, property, FALSE);
  if (p == NULL) {
    p = rofi_theme_find_property(found_wid, P_COLOR, property, FALSE);
  }
  return rofi_theme_get_highlight_inside(p, wid, property, th);
}
static double get_pixels(RofiDistanceUnit *unit, RofiOrientation ori) {
  double val = unit->distance;

  if (unit->type == ROFI_PU_EM) {
    val = unit->distance * textbox_get_estimated_char_height();
  } else if (unit->type == ROFI_PU_CH) {
    val = unit->distance * textbox_get_estimated_ch();
  } else if (unit->type == ROFI_PU_PERCENT) {
    if (ori == ROFI_ORIENTATION_VERTICAL) {
      int height = 0;
      rofi_view_get_current_monitor(NULL, &height);
      val = (unit->distance * height) / (100.0);
    } else {
      int width = 0;
      rofi_view_get_current_monitor(&width, NULL);
      val = (unit->distance * width) / (100.0);
    }
  } else if (unit->type == ROFI_PU_MM) {
    val = unit->distance * config.dpi / 25.4;
  }
  return val;
}

static double distance_unit_get_pixel(RofiDistanceUnit *unit,
                                      RofiOrientation ori) {
  switch (unit->modtype) {
  case ROFI_DISTANCE_MODIFIER_GROUP:
    return distance_unit_get_pixel(unit->left, ori);
    break;
  case ROFI_DISTANCE_MODIFIER_ADD:
    return distance_unit_get_pixel(unit->left, ori) +
           distance_unit_get_pixel(unit->right, ori);
  case ROFI_DISTANCE_MODIFIER_SUBTRACT:
    return distance_unit_get_pixel(unit->left, ori) -
           distance_unit_get_pixel(unit->right, ori);
  case ROFI_DISTANCE_MODIFIER_MULTIPLY:
    return distance_unit_get_pixel(unit->left, ori) *
           distance_unit_get_pixel(unit->right, ori);
  case ROFI_DISTANCE_MODIFIER_DIVIDE: {
    double a = distance_unit_get_pixel(unit->left, ori);
    double b = distance_unit_get_pixel(unit->right, ori);
    if (b != 0) {
      return a / b;
    }
    return a;
  }
  case ROFI_DISTANCE_MODIFIER_MODULO: {
    double a = distance_unit_get_pixel(unit->left, ori);
    double b = distance_unit_get_pixel(unit->right, ori);
    if (b != 0) {
      return fmod(a, b);
    }
    return 0;
  }
  case ROFI_DISTANCE_MODIFIER_MIN: {
    double a = distance_unit_get_pixel(unit->left, ori);
    double b = distance_unit_get_pixel(unit->right, ori);
    return MIN(a, b);
  }
  case ROFI_DISTANCE_MODIFIER_MAX: {
    double a = distance_unit_get_pixel(unit->left, ori);
    double b = distance_unit_get_pixel(unit->right, ori);
    return MAX(a, b);
  }
  case ROFI_DISTANCE_MODIFIER_ROUND: {
    double a = (double)distance_unit_get_pixel(unit->left, ori);
    double b = (double)distance_unit_get_pixel(unit->right, ori);
    return (double)(round(a / b) * b);
  }
  case ROFI_DISTANCE_MODIFIER_CEIL: {
    double a = (double)distance_unit_get_pixel(unit->left, ori);
    double b = (double)distance_unit_get_pixel(unit->right, ori);
    return (double)(ceil(a / b) * b);
  }
  case ROFI_DISTANCE_MODIFIER_FLOOR: {
    double a = (double)distance_unit_get_pixel(unit->left, ori);
    double b = (double)distance_unit_get_pixel(unit->right, ori);
    return (double)(floor(a / b) * b);
  }
  default:
    break;
  }
  return get_pixels(unit, ori);
}

int distance_get_pixel(RofiDistance d, RofiOrientation ori) {
  return distance_unit_get_pixel(&(d.base), ori);
}

void distance_get_linestyle(RofiDistance d, cairo_t *draw) {
  if (d.style == ROFI_HL_DASH) {
    const double dashes[1] = {4};
    cairo_set_dash(draw, dashes, 1, 0.0);
  } else {
    cairo_set_dash(draw, NULL, 0, 0.0);
  }
}

char *rofi_theme_parse_prepare_file(const char *file) {
  char *filename = g_strdup(file);
  // TODO: Why did I write this code? I think it was to get full path.
  GFile *gf = g_file_new_for_path(filename);
  parsed_config_files = g_list_append(parsed_config_files, filename);
  filename = g_file_get_path(gf);
  g_object_unref(gf);

  return filename;
}

void rofi_theme_parse_merge_widgets(ThemeWidget *parent, ThemeWidget *child) {
  g_assert(parent != NULL);
  g_assert(child != NULL);

  if (parent == rofi_theme && g_strcmp0(child->name, "*") == 0) {
    rofi_theme_widget_add_properties(parent, child->properties);
    return;
  }

  ThemeWidget *w = rofi_theme_find_or_create_name(parent, child->name);
  if (child->media) {
    w->media = g_slice_new0(ThemeMedia);
    *(w->media) = *(child->media);
  }
  rofi_theme_widget_add_properties(w, child->properties);
  for (unsigned int i = 0; i < child->num_widgets; i++) {
    rofi_theme_parse_merge_widgets(w, child->widgets[i]);
  }
}

static void rofi_theme_parse_process_conditionals_int(workarea mon,
                                                      ThemeWidget *rwidget) {
  if (rwidget == NULL) {
    return;
  }
  unsigned int i = 0;
  //  for (unsigned int i = 0; i < rwidget->num_widgets; i++) {
  while (i < rwidget->num_widgets) {
    ThemeWidget *child_widget = rwidget->widgets[i];
    if (child_widget->media != NULL) {
      rwidget->num_widgets--;
      for (unsigned x = i; x < rwidget->num_widgets; x++) {
        rwidget->widgets[x] = rwidget->widgets[x + 1];
      }
      rwidget->widgets[rwidget->num_widgets] = NULL;
      switch (child_widget->media->type) {
      case THEME_MEDIA_TYPE_MIN_WIDTH: {
        int w = child_widget->media->value;
        if (mon.w >= w) {
          for (unsigned int x = 0; x < child_widget->num_widgets; x++) {
            rofi_theme_parse_merge_widgets(rwidget, child_widget->widgets[x]);
          }
        }
        break;
      }
      case THEME_MEDIA_TYPE_MAX_WIDTH: {
        int w = child_widget->media->value;
        if (mon.w < w) {
          for (unsigned int x = 0; x < child_widget->num_widgets; x++) {
            rofi_theme_parse_merge_widgets(rwidget, child_widget->widgets[x]);
          }
        }
        break;
      }
      case THEME_MEDIA_TYPE_MIN_HEIGHT: {
        int h = child_widget->media->value;
        if (mon.h >= h) {
          for (unsigned int x = 0; x < child_widget->num_widgets; x++) {
            rofi_theme_parse_merge_widgets(rwidget, child_widget->widgets[x]);
          }
        } else {
        }
        break;
      }
      case THEME_MEDIA_TYPE_MAX_HEIGHT: {
        int h = child_widget->media->value;
        if (mon.h < h) {
          for (unsigned int x = 0; x < child_widget->num_widgets; x++) {
            rofi_theme_parse_merge_widgets(rwidget, child_widget->widgets[x]);
          }
        }
        break;
      }
      case THEME_MEDIA_TYPE_MON_ID: {
        if (mon.monitor_id == child_widget->media->value) {
          for (unsigned int x = 0; x < child_widget->num_widgets; x++) {
            rofi_theme_parse_merge_widgets(rwidget, child_widget->widgets[x]);
          }
        }
        break;
      }
      case THEME_MEDIA_TYPE_MIN_ASPECT_RATIO: {
        double r = child_widget->media->value;
        if ((mon.w / (double)mon.h) >= r) {
          for (unsigned int x = 0; x < child_widget->num_widgets; x++) {
            rofi_theme_parse_merge_widgets(rwidget, child_widget->widgets[x]);
          }
        }
        break;
      }
      case THEME_MEDIA_TYPE_MAX_ASPECT_RATIO: {
        double r = child_widget->media->value;
        if ((mon.w / (double)mon.h) < r) {
          for (unsigned int x = 0; x < child_widget->num_widgets; x++) {
            rofi_theme_parse_merge_widgets(rwidget, child_widget->widgets[x]);
          }
        }
        break;
      }
      case THEME_MEDIA_TYPE_BOOLEAN: {
        if (child_widget->media->boolv) {
          for (unsigned int x = 0; x < child_widget->num_widgets; x++) {
            rofi_theme_parse_merge_widgets(rwidget, child_widget->widgets[x]);
          }
        }
        break;
      }

      default: {
        break;
      }
      }
      rofi_theme_free(child_widget);
      // endif
    } else {
      rofi_theme_parse_process_conditionals_int(mon, child_widget);
      i++;
    }
  }
}

static char *rofi_theme_widget_get_name(ThemeWidget *wid) {
  GString *str = g_string_new(wid->name);
  for (ThemeWidget *i = wid->parent; i->parent != NULL; i = i->parent) {
    g_string_prepend_c(str, ' ');
    g_string_prepend(str, i->name);
  }
  char *retv = str->str;
  g_string_free(str, FALSE);
  return retv;
}

static void rofi_theme_parse_process_links_int(ThemeWidget *wid) {
  if (wid == NULL) {
    return;
  }

  for (unsigned int i = 0; i < wid->num_widgets; i++) {
    ThemeWidget *child_widget = wid->widgets[i];
    rofi_theme_parse_process_links_int(child_widget);
    if (child_widget->properties == NULL) {
      continue;
    }
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, child_widget->properties);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
      Property *pv = (Property *)value;
      if (pv->type == P_LINK) {
        if (pv->value.link.ref == NULL) {
          rofi_theme_resolve_link_property(pv, 0);
          if (pv->value.link.ref == pv) {
            char *n = rofi_theme_widget_get_name(child_widget);
            GString *str = g_string_new(NULL);
            g_string_printf(str,
                            "Validating the theme failed: the variable '%s' in "
                            "`%s { %s: var(%s);}` failed to resolve.",
                            pv->value.link.name, n, pv->name,
                            pv->value.link.name);

            rofi_add_warning_message(str);
            g_free(n);
          }
        }
      }
    }
  }
}

void rofi_theme_parse_process_links(void) {
  rofi_theme_parse_process_links_int(rofi_theme);
}

void rofi_theme_parse_process_conditionals(void) {
  workarea mon;
  monitor_active(&mon);
  rofi_theme_parse_process_conditionals_int(mon, rofi_theme);
}

ThemeMediaType rofi_theme_parse_media_type(const char *type) {
  if (g_strcmp0(type, "monitor-id") == 0) {
    return THEME_MEDIA_TYPE_MON_ID;
  }
  if (g_strcmp0(type, "min-width") == 0) {
    return THEME_MEDIA_TYPE_MIN_WIDTH;
  }
  if (g_strcmp0(type, "min-height") == 0) {
    return THEME_MEDIA_TYPE_MIN_HEIGHT;
  }
  if (g_strcmp0(type, "max-width") == 0) {
    return THEME_MEDIA_TYPE_MAX_WIDTH;
  }
  if (g_strcmp0(type, "max-height") == 0) {
    return THEME_MEDIA_TYPE_MAX_HEIGHT;
  }
  if (g_strcmp0(type, "min-aspect-ratio") == 0) {
    return THEME_MEDIA_TYPE_MIN_ASPECT_RATIO;
  }
  if (g_strcmp0(type, "max-aspect-ratio") == 0) {
    return THEME_MEDIA_TYPE_MAX_ASPECT_RATIO;
  }
  if (g_strcmp0(type, "enabled") == 0) {
    return THEME_MEDIA_TYPE_BOOLEAN;
  }
  return THEME_MEDIA_TYPE_INVALID;
}

static gboolean rofi_theme_has_property_inside(Property *p,
                                               const widget *wid_in,
                                               const char *property) {
  if (p) {
    if (p->type == P_INHERIT) {
      if (wid_in->parent) {
        ThemeWidget *parent =
            rofi_theme_find_widget(wid_in->parent->name, wid_in->state, FALSE);
        Property *pp =
            rofi_theme_find_property(parent, P_STRING, property, FALSE);
        return rofi_theme_has_property_inside(pp, wid_in->parent, property);
      }
      return FALSE;
    }
    return TRUE;
  }
  return FALSE;
}
gboolean rofi_theme_has_property(const widget *wid_in, const char *property) {
  ThemeWidget *wid = rofi_theme_find_widget(wid_in->name, wid_in->state, FALSE);
  Property *p = rofi_theme_find_property(wid, P_STRING, property, FALSE);
  return rofi_theme_has_property_inside(p, wid_in, property);
}
