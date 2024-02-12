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

// thumbnailers key file's group and file extension
#define THUMBNAILER_ENTRY_GROUP "Thumbnailer Entry"
#define THUMBNAILER_EXTENSION   ".thumbnailer"

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
  
  // thumbnailers per mime-types hashmap
  GHashTable *thumbnailers;
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
      gchar *command = g_key_file_get_string(
        key_file, THUMBNAILER_ENTRY_GROUP, "Exec", NULL);
      gchar **mime_types = g_key_file_get_string_list(
        key_file, THUMBNAILER_ENTRY_GROUP, "MimeType", NULL, NULL);

      if (mime_types && command) {
        guint i;
        for (i = 0; mime_types[i] != NULL; i++) {
          if (!g_hash_table_lookup(
              rofi_icon_fetcher_data->thumbnailers, mime_types[i])) {
            g_info("Loading thumbnailer %s for mimetype %s", filename, mime_types[i]);
            g_hash_table_insert(
              rofi_icon_fetcher_data->thumbnailers,
              g_strdup(mime_types[i]),
              g_strdup(command)
            );
          }
        }
      }

      if (mime_types) g_strfreev(mime_types);
      if (command) g_free(command);
    }

    g_key_file_free(key_file);
    g_free(filename);
  }

  g_dir_close(dir);
  g_free(thumb_path);
}

static gchar** setup_thumbnailer_command(const gchar *command,
                                         const gchar *filename,
                                         const gchar *encoded_uri,
                                         const gchar *output_path,
                                         int size) {
  gchar **command_parts = g_strsplit(command, " ", 0);
  guint command_parts_count = g_strv_length(command_parts);
  
  gchar **command_args = NULL; 
  
  if (command_parts) {
    command_args = malloc(sizeof(gchar*) * (command_parts_count + 3 + 1));
    
    // set process niceness value to 19 (low priority)
    guint current_index = 0;
    
    command_args[current_index++] = strdup("nice");
    command_args[current_index++] = strdup("-n");
    command_args[current_index++] = strdup("19");
    
    // add executable and arguments of the thumbnailer to the list
    guint i;
    for (i = 0; command_parts[i] != NULL; i++) {
      if (strcmp(command_parts[i], "%i") == 0) {
        command_args[current_index++] = strdup(filename);
      } else if (strcmp(command_parts[i], "%u") == 0) {
        command_args[current_index++] = strdup(encoded_uri);
      } else if (strcmp(command_parts[i], "%o") == 0) {
        command_args[current_index++] = strdup(output_path);
      } else if (strcmp(command_parts[i], "%s") == 0) {
        char size_str[33];
        snprintf(size_str, 33, "%d", size);
        command_args[current_index++] = strdup(size_str);
      } else {
        command_args[current_index++] = strdup(command_parts[i]);
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

  gboolean spawned = g_spawn_sync("/usr/bin", command_args,
    NULL, G_SPAWN_DEFAULT, NULL, NULL, NULL, NULL, &wait_status, &error);

  if (spawned) {
    return g_spawn_check_exit_status(wait_status, NULL);
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

  gchar *command = g_hash_table_lookup(
    rofi_icon_fetcher_data->thumbnailers, mime_type);

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
  
  // load available thumbnailers from system dirs and user dir
  rofi_icon_fetcher_data->thumbnailers = g_hash_table_new_full(
    g_str_hash, g_str_equal, (GDestroyNotify)g_free, (GDestroyNotify)g_free);

  const gchar * const *system_data_dirs = g_get_system_data_dirs();
  const gchar *user_data_dir = g_get_user_data_dir();

  rofi_icon_fetcher_load_thumbnailers(user_data_dir);

  guint i;
  for (i = 0; system_data_dirs[i] != NULL; i++) {
      rofi_icon_fetcher_load_thumbnailers(system_data_dirs[i]);
  }
}

static void free_wrapper(gpointer data, G_GNUC_UNUSED gpointer user_data) {
  g_free(data);
}

void rofi_icon_fetcher_destroy(void) {
  if (rofi_icon_fetcher_data == NULL) {
    return;
  }
  
  g_hash_table_unref(rofi_icon_fetcher_data->thumbnailers);

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

// build thumbnail's path using md5 hash of an entry name
static gchar* rofi_icon_fetcher_get_thumbnail(gchar* name, 
                                              int requested_size, 
                                              int *thumb_size) {
  // calc entry_name md5 hash
  GChecksum* checksum = g_checksum_new(G_CHECKSUM_MD5);
  g_checksum_update(checksum, (guchar*)name, -1);
  const gchar *md5_hex = g_checksum_get_string(checksum);

  // determine thumbnail folder based on the request size
  const gchar* cache_dir = g_get_user_cache_dir();
  gchar* thumb_path;

  if (requested_size <= 128) {
    *thumb_size = 128;
    thumb_path = g_strconcat(cache_dir, "/thumbnails/normal/",
        md5_hex, ".png", NULL);
  } else if (requested_size <= 256) {
    *thumb_size = 256;
    thumb_path = g_strconcat(cache_dir, "/thumbnails/large/",
        md5_hex, ".png", NULL);
  } else if (requested_size <= 512) {
    *thumb_size = 512;
    thumb_path = g_strconcat(cache_dir, "/thumbnails/x-large/",
        md5_hex, ".png", NULL);
  } else {
    *thumb_size = 1024;
    thumb_path = g_strconcat(cache_dir, "/thumbnails/xx-large/",
        md5_hex, ".png", NULL);
  }
  
  g_checksum_free(checksum);

  return thumb_path;
}

// retrieves icon key from a .desktop file
static gchar* rofi_icon_fetcher_get_desktop_icon(const gchar* file_path) {
  GKeyFile *kf = g_key_file_new();
  GError *key_error = NULL;
  gchar *icon_key = NULL;
  
  gboolean res = g_key_file_load_from_file(kf, file_path, 0, &key_error);
  
  if (res) {
    icon_key = g_key_file_get_string(kf, "Desktop Entry", "Icon", NULL);
  } else {
    g_debug("Failed to parse desktop file %s because: %s.",
        file_path, key_error->message);
    
    g_error_free(key_error);
  }
  
  g_key_file_free(kf);
  
  return icon_key;
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

  if (g_str_has_prefix(sentry->entry->name, "thumbnail://")) {
    // remove uri thumbnail prefix from entry name
    gchar *entry_name = &sentry->entry->name[12];
    
    if (strcmp(entry_name, "") == 0) {
      sentry->query_done = TRUE;
      rofi_view_reload();
      return;
    }
    
    // use custom user command to generate the thumbnail
    if (config.preview_cmd != NULL) {
      int requested_size = MAX(sentry->wsize, sentry->hsize);
      int thumb_size;

      icon_path = icon_path_ = rofi_icon_fetcher_get_thumbnail(
          entry_name, requested_size, &thumb_size);
      
      if (!g_file_test(icon_path, G_FILE_TEST_EXISTS)) {
        char **command_args = NULL;
        int argsv = 0;
        char size_str[33];
        snprintf(size_str, 33, "%d", thumb_size);
        
        helper_parse_setup(
          config.preview_cmd, &command_args, &argsv,
          "{input}", entry_name,
          "{output}", icon_path_, "{size}", size_str, NULL);
        
        if (command_args) {
          exec_thumbnailer_command(command_args);
          g_strfreev(command_args);
        }
      }
    } else if (g_path_is_absolute(entry_name)) {
      // if the entry name is an absolute path try to fetch its thumbnail
      if (g_str_has_suffix(entry_name, ".desktop")) {
        // if the entry is a .desktop file try to read its icon key
        gchar *icon_key = rofi_icon_fetcher_get_desktop_icon(entry_name);
        
        if (icon_key == NULL || strlen(icon_key) == 0) {
          // no icon in .desktop file, fallback on mimetype icon (text/plain)
          icon_path = icon_path_ = nk_xdg_theme_get_icon(
            rofi_icon_fetcher_data->xdg_context, themes, NULL, "text-plain",
            MIN(sentry->wsize, sentry->hsize), 1, TRUE);
          
          g_free(icon_key);
        } else if (g_path_is_absolute(icon_key)) {
          // icon in .desktop file is an absolute path to an image
          icon_path = icon_path_ = icon_key;
        } else {
          // icon in .desktop file is a standard icon name
          icon_path = icon_path_ = nk_xdg_theme_get_icon(
            rofi_icon_fetcher_data->xdg_context, themes, NULL, icon_key,
            MIN(sentry->wsize, sentry->hsize), 1, TRUE);
          
          g_free(icon_key);
        }
      } else {
        // build encoded uri string from absolute file path
        gchar *encoded_uri = g_filename_to_uri(entry_name, NULL, NULL);
        int requested_size = MAX(sentry->wsize, sentry->hsize);
        int thumb_size;

        // look for file thumbnail in appropriate folder based on requested size
        icon_path = icon_path_ = rofi_icon_fetcher_get_thumbnail(
          encoded_uri, requested_size, &thumb_size);

        if (!g_file_test(icon_path, G_FILE_TEST_EXISTS)) {
          // try to generate thumbnail
          char *content_type = g_content_type_guess(entry_name, NULL, 0, NULL);
          char *mime_type = g_content_type_get_mime_type(content_type);
          
          if (mime_type) {
            gboolean created = rofi_icon_fetcher_create_thumbnail(
              mime_type, entry_name, encoded_uri, icon_path_, thumb_size);

            if (!created) {
              // replace forward slashes with minus sign to get the icon's name
              int index = 0;

              while(mime_type[index]) {
                 if(mime_type[index] == '/')
                    mime_type[index] = '-';
                 index++;
              }
              
              g_free(icon_path_);

              // try to fetch the mime-type icon
              icon_path = icon_path_ = nk_xdg_theme_get_icon(
                rofi_icon_fetcher_data->xdg_context, themes, NULL, mime_type,
                MIN(sentry->wsize, sentry->hsize), 1, TRUE);
            }
            
            g_free(mime_type);
            g_free(content_type);
          }
        }
        
        g_free(encoded_uri);
      }
    }

    // no suitable icon or thumbnail was found
    if (icon_path_ == NULL || !g_file_test(icon_path, G_FILE_TEST_EXISTS)) {
      sentry->query_done = TRUE;
      rofi_view_reload();
      return;
    }
  } else if (g_path_is_absolute(sentry->entry->name)) {
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
  if (error != NULL) {
    g_warning("Failed to load image: |%s| %d %d %s (%p)", icon_path,
              sentry->wsize, sentry->hsize, error->message, pb);
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
