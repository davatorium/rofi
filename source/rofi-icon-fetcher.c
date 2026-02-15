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
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>

#include "helper.h"
#include "rofi-icon-fetcher.h"
#include "rofi-types.h"
#include "settings.h"
#include <cairo.h>
#include <pango/pangocairo.h>

#include "display.h"
#include "keyb.h"
#include "view.h"

#include "nkutils-enum.h"
#include "nkutils-xdg-theme.h"

#include <stdint.h>

#include "helper.h"
#include <gio/gio.h>

/* ThorVG for fast image rendering (SVG and PNG) */
#include <thorvg_capi.h>

/** Desktop entry specifying the thumbnailer. */
#define THUMBNAILER_ENTRY_GROUP "Thumbnailer Entry"
/** Extension used for the thumbnailer. */
#define THUMBNAILER_EXTENSION ".thumbnailer"

/** Fast icon path cache - maps "name" or "name:size" to path */
typedef struct {
  GHashTable *cache;        /* name -> path */
  GHashTable *cache_sized;  /* "name:size" -> path */
  gboolean initialized;
} IconPathCache;

typedef struct {
  // Context for icon-themes.
  NkXdgThemeContext *xdg_context;

  // On name.
  GHashTable *icon_cache;
  // On uid.
  GHashTable *icon_cache_uid;

  uint32_t last_uid;

  // thumbnailers per mime-types hashmap
  GHashTable *thumbnailers;

  // Fast icon path cache
  IconPathCache path_cache;
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
  guint scale;
  cairo_surface_t *surface;
  gboolean query_done;
  gboolean query_started;

  IconFetcherNameEntry *entry;
} IconFetcherEntry;

// Free method.
static void rofi_icon_fetch_entry_free(gpointer data);
/**
 * The icon fetcher internal state.
 */
IconFetcher *rofi_icon_fetcher_data = NULL;

static void rofi_icon_fetcher_load_thumbnailers(const gchar *path) {
  gchar *thumb_path = g_build_filename(path, "thumbnailers", NULL);

  GDir *dir = g_dir_open(thumb_path, 0, NULL);

  if (!dir) {
    g_free(thumb_path);
    return;
  }

  const gchar *dirent;

  while ((dirent = g_dir_read_name(dir))) {
    if (!g_str_has_suffix(dirent, THUMBNAILER_EXTENSION))
      continue;

    gchar *filename = g_build_filename(thumb_path, dirent, NULL);
    GKeyFile *key_file = g_key_file_new();
    GError *error = NULL;

    if (!g_key_file_load_from_file(key_file, filename, 0, &error)) {
      g_warning("Error loading thumbnailer %s: %s", filename, error->message);
      g_error_free(error);
    } else {
      gchar *command = g_key_file_get_string(key_file, THUMBNAILER_ENTRY_GROUP,
                                             "Exec", NULL);
      gchar **mime_types = g_key_file_get_string_list(
          key_file, THUMBNAILER_ENTRY_GROUP, "MimeType", NULL, NULL);

      if (mime_types && command) {
        guint i;
        for (i = 0; mime_types[i] != NULL; i++) {
          if (!g_hash_table_lookup(rofi_icon_fetcher_data->thumbnailers,
                                   mime_types[i])) {
            g_info("Loading thumbnailer %s for mimetype %s", filename,
                   mime_types[i]);
            g_hash_table_insert(rofi_icon_fetcher_data->thumbnailers,
                                g_strdup(mime_types[i]), g_strdup(command));
          }
        }
      }

      if (mime_types)
        g_strfreev(mime_types);
      if (command)
        g_free(command);
    }

    g_key_file_free(key_file);
    g_free(filename);
  }

  g_dir_close(dir);
  g_free(thumb_path);
}

static gchar **setup_thumbnailer_command(const gchar *command,
                                         const gchar *filename,
                                         const gchar *encoded_uri,
                                         const gchar *output_path, int size) {
  gchar **command_parts = g_strsplit(command, " ", 0);
  guint command_parts_count = g_strv_length(command_parts);

  gchar **command_args = NULL;

  if (command_parts) {
    command_args = g_malloc0(sizeof(gchar *) * (command_parts_count + 3 + 1));

    // set process niceness value to 19 (low priority)
    guint current_index = 0;

    command_args[current_index++] = g_strdup("nice");
    command_args[current_index++] = g_strdup("-n");
    command_args[current_index++] = g_strdup("19");

    // add executable and arguments of the thumbnailer to the list
    guint i;
    for (i = 0; command_parts[i] != NULL; i++) {
      if (strcmp(command_parts[i], "%i") == 0) {
        command_args[current_index++] = g_strdup(filename);
      } else if (strcmp(command_parts[i], "%u") == 0) {
        command_args[current_index++] = g_strdup(encoded_uri);
      } else if (strcmp(command_parts[i], "%o") == 0) {
        command_args[current_index++] = g_strdup(output_path);
      } else if (strcmp(command_parts[i], "%s") == 0) {
        command_args[current_index++] = g_strdup_printf("%d", size);
      } else {
        command_args[current_index++] = g_strdup(command_parts[i]);
      }
    }

    command_args[current_index++] = NULL;

    g_strfreev(command_parts);
  }

  return command_args;
}

static gboolean exec_thumbnailer_command(gchar **command_args) {
  // launch and wait thumbnailers process
  gint wait_status;
  GError *error = NULL;

  gboolean spawned = g_spawn_sync(NULL, command_args, NULL,
                                  G_SPAWN_DEFAULT | G_SPAWN_SEARCH_PATH, NULL,
                                  NULL, NULL, NULL, &wait_status, &error);

  if (spawned) {
    return g_spawn_check_wait_status(wait_status, NULL);
  } else {
    g_warning("Error calling thumbnailer: %s", error->message);
    g_error_free(error);

    return FALSE;
  }
}

static gboolean rofi_icon_fetcher_create_thumbnail(const gchar *mime_type,
                                                   const gchar *filename,
                                                   const gchar *encoded_uri,
                                                   const gchar *output_path,
                                                   int size) {
  gboolean thumbnail_created = FALSE;

  gchar *command =
      g_hash_table_lookup(rofi_icon_fetcher_data->thumbnailers, mime_type);

  if (!command) {
    return thumbnail_created;
  }

  // split command string to isolate arguments and expand them in a list
  gchar **command_args = setup_thumbnailer_command(
      command, filename, encoded_uri, output_path, size);

  if (command_args) {
    thumbnail_created = exec_thumbnailer_command(command_args);
    g_strfreev(command_args);
  }

  return thumbnail_created;
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

/**
 * Fast Icon Path Cache
 *
 * Scans icon directories at startup and builds a hash table for O(1) lookups.
 */

/* Icon directory to scan (in priority order) */
static const char *icon_search_dirs[] = {
  "/usr/share/icons",
  "/usr/share/pixmaps",
  NULL
};

/* Scan a single directory and add icons to cache */
static void scan_icon_dir(IconPathCache *cache, const char *base_path, const char *subdir, int size_hint) {
  char path[PATH_MAX];
  snprintf(path, sizeof(path), "%s/%s", base_path, subdir);

  DIR *dir = opendir(path);
  if (!dir) return;

  struct dirent *ent;
  char filepath[PATH_MAX];

  while ((ent = readdir(dir)) != NULL) {
    if (ent->d_name[0] == '.') continue;

    /* Get extension */
    const char *ext = strrchr(ent->d_name, '.');
    if (!ext) continue;

    /* Check for supported image extensions */
    gboolean is_image = (g_ascii_strcasecmp(ext, ".png") == 0 ||
                         g_ascii_strcasecmp(ext, ".svg") == 0 ||
                         g_ascii_strcasecmp(ext, ".svgz") == 0 ||
                         g_ascii_strcasecmp(ext, ".xpm") == 0);
    if (!is_image) continue;

    /* Get icon name (without extension) */
    char *icon_name = g_strndup(ent->d_name, ext - ent->d_name);
    snprintf(filepath, sizeof(filepath), "%s/%s", path, ent->d_name);

    /* Add to unsized cache if not already present (first match wins = priority) */
    if (!g_hash_table_contains(cache->cache, icon_name)) {
      g_hash_table_insert(cache->cache, g_strdup(icon_name), g_strdup(filepath));
    }

    /* Add sized entry if we know the size */
    if (size_hint > 0) {
      char *sized_key = g_strdup_printf("%s:%d", icon_name, size_hint);
      if (!g_hash_table_contains(cache->cache_sized, sized_key)) {
        g_hash_table_insert(cache->cache_sized, sized_key, g_strdup(filepath));
      } else {
        g_free(sized_key);
      }
    }

    g_free(icon_name);
  }
  closedir(dir);
}

/* Recursively scan icon theme directories */
static void scan_icon_theme_dir(IconPathCache *cache, const char *theme_path, const char *theme_name) {
  DIR *dir = opendir(theme_path);
  if (!dir) return;

  struct dirent *ent;
  char subdir[PATH_MAX];

  while ((ent = readdir(dir)) != NULL) {
    if (ent->d_name[0] == '.') continue;
    if (ent->d_type != DT_DIR) continue;

    /* Parse directory name for size (e.g., "48x48", "scalable", "128x128@2") */
    int size = 0;
    const char *p = ent->d_name;
    while (*p && !g_ascii_isdigit(*p)) p++;
    if (*p) size = atoi(p);

    snprintf(subdir, sizeof(subdir), "%s/%s", theme_path, ent->d_name);

    /* Scan this directory */
    scan_icon_dir(cache, subdir, "", size);

    /* Also scan subdirectories (e.g., apps, mimetypes, categories) */
    DIR *subdir_dir = opendir(subdir);
    if (subdir_dir) {
      struct dirent *subent;
      char subsubdir[PATH_MAX];
      while ((subent = readdir(subdir_dir)) != NULL) {
        if (subent->d_name[0] == '.') continue;
        if (subent->d_type != DT_DIR) continue;
        snprintf(subsubdir, sizeof(subsubdir), "%s/%s", subdir, subent->d_name);
        scan_icon_dir(cache, subsubdir, "", size);
      }
      closedir(subdir_dir);
    }
  }
  closedir(dir);
}

/* Build the icon path cache */
static void build_icon_path_cache(IconPathCache *cache) {
  if (cache->initialized) return;

  cache->cache = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
  cache->cache_sized = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

  /* Get configured icon theme */
  const char *icon_theme = config.icon_theme;
  char theme_path[PATH_MAX];

  /* Scan icon directories */
  for (int i = 0; icon_search_dirs[i]; i++) {
    const char *base = icon_search_dirs[i];

    /* Scan configured theme first (highest priority) */
    if (icon_theme && icon_theme[0]) {
      snprintf(theme_path, sizeof(theme_path), "%s/%s", base, icon_theme);
      scan_icon_theme_dir(cache, theme_path, icon_theme);
    }

    /* Scan fallback themes */
    snprintf(theme_path, sizeof(theme_path), "%s/Adwaita", base);
    scan_icon_theme_dir(cache, theme_path, "Adwaita");

    snprintf(theme_path, sizeof(theme_path), "%s/hicolor", base);
    scan_icon_theme_dir(cache, theme_path, "hicolor");

    /* Scan pixmaps directly */
    if (g_ascii_strcasecmp(base, "/usr/share/pixmaps") == 0) {
      scan_icon_dir(cache, base, "", 48);
    }
  }

  cache->initialized = TRUE;
  g_info("Icon path cache built: %d icons", g_hash_table_size(cache->cache));
}

/* Fast icon path lookup from cache */
static const char *lookup_icon_path_cache(IconPathCache *cache, const char *name, int size) {
  if (!cache->initialized || !name) return NULL;

  /* Try sized lookup first */
  if (size > 0) {
    char *sized_key = g_strdup_printf("%s:%d", name, size);
    const char *path = g_hash_table_lookup(cache->cache_sized, sized_key);
    g_free(sized_key);
    if (path) return path;
  }

  /* Fall back to unsized lookup */
  return g_hash_table_lookup(cache->cache, name);
}

void rofi_icon_fetcher_init(void) {
  g_assert(rofi_icon_fetcher_data == NULL);

  /* Initialize ThorVG engine for fast image rendering */
  tvg_engine_init(0);

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

  // load available thumbnailers from system dirs and user dir
  rofi_icon_fetcher_data->thumbnailers = g_hash_table_new_full(
      g_str_hash, g_str_equal, (GDestroyNotify)g_free, (GDestroyNotify)g_free);

  const gchar *const *system_data_dirs = g_get_system_data_dirs();
  const gchar *user_data_dir = g_get_user_data_dir();

  rofi_icon_fetcher_load_thumbnailers(user_data_dir);

  guint i;
  for (i = 0; system_data_dirs[i] != NULL; i++) {
    rofi_icon_fetcher_load_thumbnailers(system_data_dirs[i]);
  }

  /* Build fast icon path cache */
  g_info("Building icon path cache...");
  double start = g_get_monotonic_time() / 1000.0;
  build_icon_path_cache(&rofi_icon_fetcher_data->path_cache);
  double elapsed = g_get_monotonic_time() / 1000.0 - start;
  g_info("Icon path cache built in %.1f ms", elapsed);
}

void rofi_icon_fetcher_destroy(void) {
  if (rofi_icon_fetcher_data == NULL) {
    return;
  }

  g_hash_table_unref(rofi_icon_fetcher_data->thumbnailers);

  nk_xdg_theme_context_free(rofi_icon_fetcher_data->xdg_context);

  g_hash_table_unref(rofi_icon_fetcher_data->icon_cache_uid);
  g_hash_table_unref(rofi_icon_fetcher_data->icon_cache);

  /* Free icon path cache */
  if (rofi_icon_fetcher_data->path_cache.cache) {
    g_hash_table_unref(rofi_icon_fetcher_data->path_cache.cache);
  }
  if (rofi_icon_fetcher_data->path_cache.cache_sized) {
    g_hash_table_unref(rofi_icon_fetcher_data->path_cache.cache_sized);
  }

  g_free(rofi_icon_fetcher_data);

  /* Terminate ThorVG engine */
  tvg_engine_term();
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

  /* Check for supported image extensions (thorvg supports these) */
  return (g_ascii_strcasecmp(suf, "png") == 0 ||
          g_ascii_strcasecmp(suf, "svg") == 0 ||
          g_ascii_strcasecmp(suf, "svgz") == 0 ||
          g_ascii_strcasecmp(suf, "jpg") == 0 ||
          g_ascii_strcasecmp(suf, "jpeg") == 0 ||
          g_ascii_strcasecmp(suf, "webp") == 0 ||
          g_ascii_strcasecmp(suf, "tvg") == 0);
}

// build thumbnail's path using md5 hash of an entry name
static gchar *rofi_icon_fetcher_get_thumbnail(gchar *name, int requested_size,
                                              int *thumb_size) {
  // calc entry_name md5 hash
  GChecksum *checksum = g_checksum_new(G_CHECKSUM_MD5);
  g_checksum_update(checksum, (guchar *)name, -1);
  const gchar *md5_hex = g_checksum_get_string(checksum);

  // determine thumbnail folder based on the request size
  const gchar *cache_dir = g_get_user_cache_dir();
  gchar *thumb_dir;
  gchar *thumb_path;

  if (requested_size <= 128) {
    *thumb_size = 128;
    thumb_dir = g_strconcat(cache_dir, "/thumbnails/normal/", NULL);
    thumb_path =
        g_strconcat(cache_dir, "/thumbnails/normal/", md5_hex, ".png", NULL);
  } else if (requested_size <= 256) {
    *thumb_size = 256;
    thumb_dir = g_strconcat(cache_dir, "/thumbnails/large/", NULL);
    thumb_path =
        g_strconcat(cache_dir, "/thumbnails/large/", md5_hex, ".png", NULL);
  } else if (requested_size <= 512) {
    *thumb_size = 512;
    thumb_dir = g_strconcat(cache_dir, "/thumbnails/x-large/", NULL);
    thumb_path =
        g_strconcat(cache_dir, "/thumbnails/x-large/", md5_hex, ".png", NULL);
  } else {
    *thumb_size = 1024;
    thumb_dir = g_strconcat(cache_dir, "/thumbnails/xx-large/", NULL);
    thumb_path =
        g_strconcat(cache_dir, "/thumbnails/xx-large/", md5_hex, ".png", NULL);
  }

  // create thumbnail directory if it does not exist
  g_mkdir_with_parents(thumb_dir, 0700);

  g_free(thumb_dir);
  g_checksum_free(checksum);

  return thumb_path;
}

// retrieves icon key from a .desktop file
static gchar *rofi_icon_fetcher_get_desktop_icon(const gchar *file_path) {
  GKeyFile *kf = g_key_file_new();
  GError *key_error = NULL;
  gchar *icon_key = NULL;

  gboolean res = g_key_file_load_from_file(kf, file_path, 0, &key_error);

  if (res) {
    icon_key = g_key_file_get_string(kf, "Desktop Entry", "Icon", NULL);
  } else {
    g_debug("Failed to parse desktop file %s because: %s.", file_path,
            key_error->message);

    g_error_free(key_error);
  }

  g_key_file_free(kf);

  return icon_key;
}

/**
 * Load an image file using ThorVG and convert to a cairo surface.
 * This is significantly faster than using gdk-pixbuf for both SVG and PNG.
 */
static cairo_surface_t *rofi_icon_fetcher_load_image_thorvg(const char *path,
                                                             int width, int height) {
  if (width <= 0 || height <= 0) {
    return NULL;
  }

  /* Create a picture and load the SVG file */
  Tvg_Paint picture = tvg_picture_new();
  if (!picture) {
    g_warning("ThorVG: Failed to create picture");
    return NULL;
  }

  Tvg_Result result = tvg_picture_load(picture, path);
  if (result != TVG_RESULT_SUCCESS) {
    g_debug("ThorVG: Failed to load SVG %s: %d", path, result);
    tvg_paint_unref(picture, TRUE);
    return NULL;
  }

  /* Get original size and calculate scale */
  float orig_w, orig_h;
  tvg_picture_get_size(picture, &orig_w, &orig_h);
  if (orig_w <= 0 || orig_h <= 0) {
    orig_w = width;
    orig_h = height;
  }

  /* Calculate scaled size maintaining aspect ratio */
  float scale = (float)width / orig_w;
  if (height / orig_h < scale) {
    scale = (float)height / orig_h;
  }
  float scaled_w = orig_w * scale;
  float scaled_h = orig_h * scale;

  /* Set the picture size */
  tvg_picture_set_size(picture, scaled_w, scaled_h);

  /* Allocate buffer for ARGB32 output */
  uint32_t *buffer = g_malloc0(width * height * sizeof(uint32_t));
  if (!buffer) {
    tvg_paint_unref(picture, TRUE);
    return NULL;
  }

  /* Create a sw canvas */
  Tvg_Canvas canvas = tvg_swcanvas_create(TVG_ENGINE_OPTION_DEFAULT);
  if (!canvas) {
    g_warning("ThorVG: Failed to create canvas");
    g_free(buffer);
    tvg_paint_unref(picture, TRUE);
    return NULL;
  }

  result = tvg_swcanvas_set_target(canvas, buffer, width, width, height,
                                    TVG_COLORSPACE_ARGB8888);
  if (result != TVG_RESULT_SUCCESS) {
    g_warning("ThorVG: Failed to set canvas target: %d", result);
    tvg_canvas_destroy(canvas);
    g_free(buffer);
    tvg_paint_unref(picture, TRUE);
    return NULL;
  }

  /* Push picture to canvas and render */
  result = tvg_canvas_add(canvas, picture);
  if (result != TVG_RESULT_SUCCESS) {
    g_warning("ThorVG: Failed to push picture: %d", result);
    tvg_canvas_destroy(canvas);
    g_free(buffer);
    return NULL;
  }

  result = tvg_canvas_draw(canvas, TRUE);
  if (result != TVG_RESULT_SUCCESS) {
    g_warning("ThorVG: Failed to draw: %d", result);
    tvg_canvas_destroy(canvas);
    g_free(buffer);
    return NULL;
  }

  result = tvg_canvas_sync(canvas);
  if (result != TVG_RESULT_SUCCESS) {
    g_warning("ThorVG: Failed to sync: %d", result);
    tvg_canvas_destroy(canvas);
    g_free(buffer);
    return NULL;
  }

  /* Create cairo surface from the buffer */
  cairo_surface_t *surface = cairo_image_surface_create_for_data(
      (unsigned char *)buffer, CAIRO_FORMAT_ARGB32, width, height,
      width * 4);

  /* Make cairo own the buffer so it frees it when surface is destroyed */
  cairo_surface_set_user_data(surface, NULL, buffer,
                               (cairo_destroy_func_t)g_free);

  tvg_canvas_destroy(canvas);

  return surface;
}

/**
 * Synchronously load an icon - called directly from query functions.
 * This is fast enough (~50us) to not need async threading.
 */
static cairo_surface_t *rofi_icon_fetcher_load_icon_sync(const char *name, int wsize, int hsize, guint scale) {
  const gchar *themes[] = {config.icon_theme, NULL};
  const gchar *icon_path;
  gchar *icon_path_ = NULL;
  cairo_surface_t *icon_surf = NULL;

  /* Handle absolute paths directly */
  if (g_path_is_absolute(name)) {
    icon_path = name;
  } else if (g_str_has_prefix(name, "<span")) {
    /* Pango markup - render text */
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, wsize, hsize);
    cairo_t *cr = cairo_create(surface);
    PangoLayout *layout = pango_cairo_create_layout(cr);
    pango_layout_set_markup(layout, name, -1);

    int width, height;
    pango_layout_get_size(layout, &width, &height);
    double ws = wsize / ((double)width / PANGO_SCALE);
    double wh = hsize / ((double)height / PANGO_SCALE);
    double s = MIN(ws, wh);

    cairo_move_to(cr, (wsize - ((double)width / PANGO_SCALE) * s) / 2.0,
                     (hsize - ((double)height / PANGO_SCALE) * s) / 2.0);
    cairo_scale(cr, s, s);
    pango_cairo_update_layout(cr, layout);
    pango_layout_get_size(layout, &width, &height);
    pango_cairo_show_layout(cr, layout);
    g_object_unref(layout);
    cairo_destroy(cr);
    return surface;
  } else {
    /* Regular icon name - use fast cache lookup */
    int size = MIN(wsize, hsize);
    icon_path = lookup_icon_path_cache(&rofi_icon_fetcher_data->path_cache, name, size);

    if (icon_path != NULL) {
      icon_path_ = g_strdup(icon_path);
    } else {
      /* Cache miss - fall back to theme lookup */
      icon_path = icon_path_ = nk_xdg_theme_get_icon(
          rofi_icon_fetcher_data->xdg_context, themes, NULL, name, size, scale, TRUE);

      if (icon_path_ == NULL) {
        /* Try as filename with extension */
        const char *ext = g_strrstr(name, ".");
        if (ext) {
          const char *exts2[2] = {ext, NULL};
          icon_path = icon_path_ = helper_get_theme_path(name, exts2, NULL);
        }
        if (icon_path_ == NULL) {
          return NULL;
        }
      }
    }
  }

  /* Load the image with ThorVG */
  int width = wsize, height = hsize;
  if (width > 0) width *= scale;
  if (height > 0) height *= scale;

  icon_surf = rofi_icon_fetcher_load_image_thorvg(icon_path, width, height);

  g_free(icon_path_);
  return icon_surf;
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
  const guint scale = display_scale();
  for (GList *iter = g_list_first(entry->sizes); iter;
       iter = g_list_next(iter)) {
    sentry = iter->data;
    if (sentry->wsize == wsize && sentry->hsize == hsize &&
        sentry->scale == scale) {
      /* Already have this size cached - return immediately */
      return sentry->uid;
    }
  }

  // Not found - create new entry and load synchronously
  sentry = g_new0(IconFetcherEntry, 1);
  sentry->uid = ++(rofi_icon_fetcher_data->last_uid);
  sentry->wsize = wsize;
  sentry->hsize = hsize;
  sentry->scale = scale;
  sentry->entry = entry;
  sentry->query_done = FALSE;
  sentry->query_started = TRUE;
  sentry->surface = NULL;

  entry->sizes = g_list_prepend(entry->sizes, sentry);
  g_hash_table_insert(rofi_icon_fetcher_data->icon_cache_uid,
                      GINT_TO_POINTER(sentry->uid), sentry);

  /* Load synchronously - fast enough with thorvg (~50us) */
  sentry->surface = rofi_icon_fetcher_load_icon_sync(name, wsize, hsize, scale);
  sentry->query_done = TRUE;

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
  const guint scale = display_scale();
  for (GList *iter = g_list_first(entry->sizes); iter;
       iter = g_list_next(iter)) {
    sentry = iter->data;
    if (sentry->wsize == size && sentry->hsize == size &&
        sentry->scale == scale) {
      /* Already have this size cached - return immediately */
      return sentry->uid;
    }
  }

  // Not found - create new entry and load synchronously
  sentry = g_new0(IconFetcherEntry, 1);
  sentry->uid = ++(rofi_icon_fetcher_data->last_uid);
  sentry->wsize = size;
  sentry->hsize = size;
  sentry->scale = scale;
  sentry->entry = entry;
  sentry->query_done = FALSE;
  sentry->query_started = TRUE;
  sentry->surface = NULL;

  entry->sizes = g_list_prepend(entry->sizes, sentry);
  g_hash_table_insert(rofi_icon_fetcher_data->icon_cache_uid,
                      GINT_TO_POINTER(sentry->uid), sentry);

  /* Load synchronously - fast enough with thorvg (~50us) */
  sentry->surface = rofi_icon_fetcher_load_icon_sync(name, size, size, scale);
  sentry->query_done = TRUE;

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
