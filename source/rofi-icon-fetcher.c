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

/** The log domain of this Helper. */
#define G_LOG_DOMAIN "Helpers.IconFetcher"

#include "config.h"
#include <stdlib.h>
#include <xcb/xproto.h>

#include "helper.h"
#include "rofi-icon-fetcher.h"
#include "rofi-types.h"
#include "settings.h"
#include <cairo.h>
#include <pango/pangocairo.h>

#include "keyb.h"
#include "view.h"
#include "xcb.h"

#include "nkutils-enum.h"
#include "nkutils-xdg-theme.h"

#include <stdint.h>

#include "helper.h"
#include <gdk-pixbuf/gdk-pixbuf.h>

typedef struct {
  // Context for icon-themes.
  NkXdgThemeContext *xdg_context;

  // On name.
  GHashTable *icon_cache;
  // On uid.
  GHashTable *icon_cache_uid;

  // list extensions
  GList *supported_extensions;
  uint32_t last_uid;
} IconFetcher;

typedef struct {
  char *name;
  GList *sizes;
} IconFetcherNameEntry;

typedef struct {
  thread_state state;

  GCond *cond;
  GMutex *mutex;
  unsigned int *acount;

  uint32_t uid;
  int wsize;
  int hsize;
  cairo_surface_t *surface;
  gboolean query_done;

  IconFetcherNameEntry *entry;
} IconFetcherEntry;

// Free method.
static void rofi_icon_fetch_entry_free(gpointer data);
/**
 * The icon fetcher internal state.
 */
IconFetcher *rofi_icon_fetcher_data = NULL;

static void rofi_icon_fetch_thread_pool_entry_remove(gpointer data) {
  IconFetcherEntry *entry = (IconFetcherEntry *)data;
  // Mark it in a way it should be re-fetched on next query?
}
static void rofi_icon_fetch_entry_free(gpointer data) {
  IconFetcherNameEntry *entry = (IconFetcherNameEntry *)data;

  // Free name/key.
  g_free(entry->name);

  for (GList *iter = g_list_first(entry->sizes); iter;
       iter = g_list_next(iter)) {
    IconFetcherEntry *sentry = (IconFetcherEntry *)(iter->data);

    cairo_surface_destroy(sentry->surface);
    g_free(sentry);
  }

  g_list_free(entry->sizes);
  g_free(entry);
}

void rofi_icon_fetcher_init(void) {
  g_assert(rofi_icon_fetcher_data == NULL);

  static const gchar *const icon_fallback_themes[] = {"Adwaita", "gnome", NULL};
  const char *themes[2] = {config.icon_theme, NULL};

  rofi_icon_fetcher_data = g_malloc0(sizeof(IconFetcher));

  rofi_icon_fetcher_data->xdg_context =
      nk_xdg_theme_context_new(icon_fallback_themes, NULL);
  nk_xdg_theme_preload_themes_icon(rofi_icon_fetcher_data->xdg_context, themes);

  rofi_icon_fetcher_data->icon_cache_uid =
      g_hash_table_new(g_direct_hash, g_direct_equal);
  rofi_icon_fetcher_data->icon_cache = g_hash_table_new_full(
      g_str_hash, g_str_equal, NULL, rofi_icon_fetch_entry_free);

  GSList *l = gdk_pixbuf_get_formats();
  for (GSList *li = l; li != NULL; li = g_slist_next(li)) {
    gchar **exts =
        gdk_pixbuf_format_get_extensions((GdkPixbufFormat *)li->data);

    for (unsigned int i = 0; exts && exts[i]; i++) {
      rofi_icon_fetcher_data->supported_extensions =
          g_list_append(rofi_icon_fetcher_data->supported_extensions, exts[i]);
      g_info("Add image extension: %s", exts[i]);
      exts[i] = NULL;
    }

    g_free(exts);
  }
  g_slist_free(l);
}

static void free_wrapper(gpointer data, G_GNUC_UNUSED gpointer user_data) {
  g_free(data);
}

void rofi_icon_fetcher_destroy(void) {
  if (rofi_icon_fetcher_data == NULL) {
    return;
  }

  nk_xdg_theme_context_free(rofi_icon_fetcher_data->xdg_context);

  g_hash_table_unref(rofi_icon_fetcher_data->icon_cache_uid);
  g_hash_table_unref(rofi_icon_fetcher_data->icon_cache);

  g_list_foreach(rofi_icon_fetcher_data->supported_extensions, free_wrapper,
                 NULL);
  g_list_free(rofi_icon_fetcher_data->supported_extensions);
  g_free(rofi_icon_fetcher_data);
}

/*
 * _rofi_icon_fetcher_get_icon_surface and alpha_mult
 * are inspired by gdk_cairo_set_source_pixbuf
 * GDK is:
 *     Copyright (C) 2011-2018 Red Hat, Inc.
 */
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
/** Location of red byte */
#define RED_BYTE 2
/** Location of green byte */
#define GREEN_BYTE 1
/** Location of blue byte */
#define BLUE_BYTE 0
/** Location of alpha byte */
#define ALPHA_BYTE 3
#else
/** Location of red byte */
#define RED_BYTE 1
/** Location of green byte */
#define GREEN_BYTE 2
/** Location of blue byte */
#define BLUE_BYTE 3
/** Location of alpha byte */
#define ALPHA_BYTE 0
#endif

static inline guchar alpha_mult(guchar c, guchar a) {
  guint16 t;
  switch (a) {
  case 0xff:
    return c;
  case 0x00:
    return 0x00;
  default:
    t = c * a + 0x7f;
    return ((t >> 8) + t) >> 8;
  }
}

static cairo_surface_t *
rofi_icon_fetcher_get_surface_from_pixbuf(GdkPixbuf *pixbuf) {
  gint width, height;
  const guchar *pixels;
  gint stride;
  gboolean alpha;

  if (pixbuf == NULL) {
    return NULL;
  }

  width = gdk_pixbuf_get_width(pixbuf);
  height = gdk_pixbuf_get_height(pixbuf);
  pixels = gdk_pixbuf_read_pixels(pixbuf);
  stride = gdk_pixbuf_get_rowstride(pixbuf);
  alpha = gdk_pixbuf_get_has_alpha(pixbuf);

  cairo_surface_t *surface = NULL;

  gint cstride;
  guint lo, o;
  guchar a = 0xff;
  const guchar *pixels_end, *line;
  guchar *cpixels;

  pixels_end = pixels + height * stride;
  o = alpha ? 4 : 3;
  lo = o * width;

  surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  cpixels = cairo_image_surface_get_data(surface);
  cstride = cairo_image_surface_get_stride(surface);

  cairo_surface_flush(surface);
  while (pixels < pixels_end) {
    line = pixels;
    const guchar *line_end = line + lo;
    guchar *cline = cpixels;

    while (line < line_end) {
      if (alpha) {
        a = line[3];
      }
      cline[RED_BYTE] = alpha_mult(line[0], a);
      cline[GREEN_BYTE] = alpha_mult(line[1], a);
      cline[BLUE_BYTE] = alpha_mult(line[2], a);
      cline[ALPHA_BYTE] = a;

      line += o;
      cline += 4;
    }

    pixels += stride;
    cpixels += cstride;
  }
  cairo_surface_mark_dirty(surface);
  cairo_surface_flush(surface);

  return surface;
}

gboolean rofi_icon_fetcher_file_is_image(const char *const path) {
  if (path == NULL) {
    return FALSE;
  }
  const char *suf = strrchr(path, '.');
  if (suf == NULL) {
    return FALSE;
  }
  suf++;

  for (GList *iter = rofi_icon_fetcher_data->supported_extensions; iter != NULL;
       iter = g_list_next(iter)) {
    if (g_ascii_strcasecmp(iter->data, suf) == 0) {
      return TRUE;
    }
  }
  return FALSE;
}

static void rofi_icon_fetcher_worker(thread_state *sdata,
                                     G_GNUC_UNUSED gpointer user_data) {
  g_debug("starting up icon fetching thread.");
  // as long as dr->icon is updated atomicly.. (is a pointer write atomic?)
  // this should be fine running in another thread.
  IconFetcherEntry *sentry = (IconFetcherEntry *)sdata;
  const gchar *themes[] = {config.icon_theme, NULL};

  const gchar *icon_path;
  gchar *icon_path_ = NULL;

  if (g_path_is_absolute(sentry->entry->name)) {
    icon_path = sentry->entry->name;
  } else if (g_str_has_prefix(sentry->entry->name, "<span")) {
    cairo_surface_t *surface = cairo_image_surface_create(
        CAIRO_FORMAT_ARGB32, sentry->wsize, sentry->hsize);
    cairo_t *cr = cairo_create(surface);
    PangoLayout *layout = pango_cairo_create_layout(cr);
    pango_layout_set_markup(layout, sentry->entry->name, -1);

    int width, height;
    pango_layout_get_size(layout, &width, &height);
    double ws = sentry->wsize / ((double)width / PANGO_SCALE);
    double wh = sentry->hsize / ((double)height / PANGO_SCALE);
    double scale = MIN(ws, wh);

    cairo_move_to(
        cr, (sentry->wsize - ((double)width / PANGO_SCALE) * scale) / 2.0,
        (sentry->hsize - ((double)height / PANGO_SCALE) * scale) / 2.0);
    cairo_scale(cr, scale, scale);
    pango_cairo_update_layout(cr, layout);
    pango_layout_get_size(layout, &width, &height);
    pango_cairo_show_layout(cr, layout);
    g_object_unref(layout);
    cairo_destroy(cr);
    sentry->surface = surface;
    sentry->query_done = TRUE;
    rofi_view_reload();
    return;

  } else {
    icon_path = icon_path_ = nk_xdg_theme_get_icon(
        rofi_icon_fetcher_data->xdg_context, themes, NULL, sentry->entry->name,
        MIN(sentry->wsize, sentry->hsize), 1, TRUE);
    if (icon_path_ == NULL) {
      g_debug("failed to get icon %s(%dx%d): n/a", sentry->entry->name,
              sentry->wsize, sentry->hsize);

      const char *ext = g_strrstr(sentry->entry->name, ".");
      if (ext) {
        const char *exts2[2] = {ext, NULL};
        icon_path = icon_path_ =
            helper_get_theme_path(sentry->entry->name, exts2, NULL);
      }
      if (icon_path_ == NULL) {
        sentry->query_done = TRUE;
        rofi_view_reload();
        return;
      }
    } else {
      g_debug("found icon %s(%dx%d): %s", sentry->entry->name, sentry->wsize,
              sentry->hsize, icon_path);
    }
  }
  cairo_surface_t *icon_surf = NULL;

  const char *suf = strrchr(icon_path, '.');
  if (suf == NULL) {
    sentry->query_done = TRUE;
    g_free(icon_path_);
    rofi_view_reload();
    return;
  }

  GError *error = NULL;
  GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(
      icon_path, sentry->wsize, sentry->hsize, TRUE, &error);

  /*
   * The GIF codec throws GDK_PIXBUF_ERROR_INCOMPLETE_ANIMATION if it's closed
   * without decoding all the frames. Since gdk_pixbuf_new_from_file_at_scale
   * only decodes the first frame, this specific error needs to be ignored.
   */
  if (error != NULL && g_error_matches(
          error, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_INCOMPLETE_ANIMATION)) {
      g_clear_error(&error);
  }

  if (error != NULL) {
    g_warning("Failed to load image: |%s| %d %d %s (%p)", icon_path,
              sentry->wsize, sentry->hsize, error->message, (void *)pb);
    g_error_free(error);
    if (pb) {
      g_object_unref(pb);
    }
  } else {
    icon_surf = rofi_icon_fetcher_get_surface_from_pixbuf(pb);
    g_object_unref(pb);
  }

  sentry->surface = icon_surf;
  g_free(icon_path_);
  sentry->query_done = TRUE;
  rofi_view_reload();
}

uint32_t rofi_icon_fetcher_query_advanced(const char *name, const int wsize,
                                          const int hsize) {
  g_debug("Query: %s(%dx%d)", name, wsize, hsize);
  IconFetcherNameEntry *entry =
      g_hash_table_lookup(rofi_icon_fetcher_data->icon_cache, name);
  if (entry == NULL) {
    entry = g_new0(IconFetcherNameEntry, 1);
    entry->name = g_strdup(name);
    g_hash_table_insert(rofi_icon_fetcher_data->icon_cache, entry->name, entry);
  }
  IconFetcherEntry *sentry;
  for (GList *iter = g_list_first(entry->sizes); iter;
       iter = g_list_next(iter)) {
    sentry = iter->data;
    if (sentry->wsize == wsize && sentry->hsize == hsize) {
      return sentry->uid;
    }
  }

  // Not found.
  sentry = g_new0(IconFetcherEntry, 1);
  sentry->uid = ++(rofi_icon_fetcher_data->last_uid);
  sentry->wsize = wsize;
  sentry->hsize = hsize;
  sentry->entry = entry;
  sentry->query_done = FALSE;
  sentry->surface = NULL;

  entry->sizes = g_list_prepend(entry->sizes, sentry);
  g_hash_table_insert(rofi_icon_fetcher_data->icon_cache_uid,
                      GINT_TO_POINTER(sentry->uid), sentry);

  // Push into fetching queue.
  sentry->state.callback = rofi_icon_fetcher_worker;
  sentry->state.free = rofi_icon_fetch_thread_pool_entry_remove;
  sentry->state.priority = G_PRIORITY_LOW;
  g_thread_pool_push(tpool, sentry, NULL);

  return sentry->uid;
}
uint32_t rofi_icon_fetcher_query(const char *name, const int size) {
  g_debug("Query: %s(%d)", name, size);
  IconFetcherNameEntry *entry =
      g_hash_table_lookup(rofi_icon_fetcher_data->icon_cache, name);
  if (entry == NULL) {
    entry = g_new0(IconFetcherNameEntry, 1);
    entry->name = g_strdup(name);
    g_hash_table_insert(rofi_icon_fetcher_data->icon_cache, entry->name, entry);
  }
  IconFetcherEntry *sentry;
  for (GList *iter = g_list_first(entry->sizes); iter;
       iter = g_list_next(iter)) {
    sentry = iter->data;
    if (sentry->wsize == size && sentry->hsize == size) {
      return sentry->uid;
    }
  }

  // Not found.
  sentry = g_new0(IconFetcherEntry, 1);
  sentry->uid = ++(rofi_icon_fetcher_data->last_uid);
  sentry->wsize = size;
  sentry->hsize = size;
  sentry->entry = entry;
  sentry->surface = NULL;

  entry->sizes = g_list_prepend(entry->sizes, sentry);
  g_hash_table_insert(rofi_icon_fetcher_data->icon_cache_uid,
                      GINT_TO_POINTER(sentry->uid), sentry);

  // Push into fetching queue.
  sentry->state.callback = rofi_icon_fetcher_worker;
  sentry->state.priority = G_PRIORITY_LOW;
  g_thread_pool_push(tpool, sentry, NULL);

  return sentry->uid;
}

cairo_surface_t *rofi_icon_fetcher_get(const uint32_t uid) {
  IconFetcherEntry *sentry = g_hash_table_lookup(
      rofi_icon_fetcher_data->icon_cache_uid, GINT_TO_POINTER(uid));
  if (sentry) {
    return sentry->surface;
  }
  g_warning("Querying an non-existing uid");
  return NULL;
}

gboolean rofi_icon_fetcher_get_ex(const uint32_t uid,
                                  cairo_surface_t **surface) {
  IconFetcherEntry *sentry = g_hash_table_lookup(
      rofi_icon_fetcher_data->icon_cache_uid, GINT_TO_POINTER(uid));
  *surface = NULL;
  if (sentry) {
    *surface = sentry->surface;
    return sentry->query_done;
  }
  g_warning("Querying an non-existing uid");
  return FALSE;
}
