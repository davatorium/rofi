/**
 *   MIT/X11 License
 *   Modified  (c) 2017 Morgane Glidic, (c) 2020-2025 lbonn, (c) 2025 lvitals
 *
 *   Permission is hereby granted, free of charge, to any person obtaining
 *   a copy of this software and associated documentation files (the
 *   "Software"), to deal in the Software without restriction, including
 *   without limitation the rights to use, copy, modify, merge, publish,
 *   distribute, sublicense, and/or sell copies of the Software, and to
 *   permit persons to whom the Software is furnished to do so, subject to
 *   the following conditions:
 *
 *   The above copyright notice and this permission notice shall be
 *   included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define G_LOG_DOMAIN "Wayland"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include <config.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <math.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include <cairo.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <xkbcommon/xkbcommon-compose.h>
#include <xkbcommon/xkbcommon.h>

#include <libgwater-wayland.h>

#include <nkutils-bindings.h>

#include <rofi.h>

#include "input-codes.h"
#include "keyb.h"
#include "rofi-types.h"
#include "settings.h"
#include "view.h"

#include "display-internal.h"
#include "display.h"
#include "wayland-internal.h"

#ifdef HAVE_WAYLAND_CURSOR_SHAPE
#include "cursor-shape-v1-protocol.h"
#endif
#include "keyboard-shortcuts-inhibit-unstable-v1-protocol.h"
#include "primary-selection-unstable-v1-protocol.h"
#include "wlr-layer-shell-unstable-v1-protocol.h"
#include "text-input-unstable-v3-protocol.h"

#define wayland_output_get_dpi(output, scale, dimension)                       \
  ((output)->current.physical_##dimension > 0 && (scale) > 0                   \
       ? round((double)(output)->current.dimension * 25.4 / (scale) /          \
               (output)->current.physical_##dimension)                         \
       : 0)

typedef struct _display_buffer_pool wayland_buffer_pool;
typedef struct {
  wayland_stuff *context;
  uint32_t global_name;
  struct wl_output *output;
  gchar *name;
  struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    int32_t physical_width;  /* mm */
    int32_t physical_height; /* mm */
    int32_t scale;
    int32_t transform;
  } current, pending;
} wayland_output;

typedef struct {
  wayland_buffer_pool *pool;
  struct wl_buffer *buffer;
  uint8_t *data;
  gboolean released;
} wayland_buffer;

struct _display_buffer_pool {
  wayland_stuff *context;
  uint8_t *data;
  size_t size;
  int32_t width;
  int32_t height;
  gboolean to_free;
  wayland_buffer *buffers;
};

static gboolean wayland_display_late_setup(void);

static wayland_stuff wayland_;
wayland_stuff *wayland = &wayland_;
static const cairo_user_data_key_t wayland_cairo_surface_user_data;

static const struct zwp_text_input_v3_listener text_input_listener;

static void wayland_buffer_cleanup(wayland_buffer_pool *self) {
  if (!self->to_free) {
    return;
  }

  size_t i, count = 0;
  for (i = 0; i < wayland->buffer_count; ++i) {
    if ((self->buffers[i].released) && (self->buffers[i].buffer != NULL)) {
      wl_buffer_destroy(self->buffers[i].buffer);
      self->buffers[i].buffer = NULL;
    }
    if (self->buffers[i].buffer == NULL) {
      ++count;
    }
  }

  if (count < wayland->buffer_count) {
    return;
  }

  munmap(self->data, self->size);
  g_free(self);
}

static void wayland_buffer_release(void *data, struct wl_buffer *buffer) {
  wayland_buffer_pool *self = data;

  size_t i;
  for (i = 0; i < wayland->buffer_count; ++i) {
    if (self->buffers[i].buffer == buffer) {
      self->buffers[i].released = TRUE;
    }
  }

  wayland_buffer_cleanup(self);
}

static const struct wl_buffer_listener wayland_buffer_listener = {
    wayland_buffer_release};

wayland_buffer_pool *display_buffer_pool_new(gint width, gint height) {
  struct wl_shm_pool *wl_pool;
  struct wl_buffer *buffer;
  int fd;
  uint8_t *data;
  width *= wayland->scale;
  height *= wayland->scale;
  int32_t stride;
  size_t size;
  size_t pool_size;

  stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
  if (stride < 0) {
    g_warning("cairo stride width calculation failure");
    return NULL;
  }
  size = (size_t)stride * height;
  pool_size = size * wayland->buffer_count;

  gchar filename[PATH_MAX];
  g_snprintf(filename, PATH_MAX, "%s/rofi-wayland-surface",
             g_get_user_runtime_dir());
  fd = g_open(filename, O_CREAT | O_RDWR, 0);
  g_unlink(filename);
  if (fd < 0) {
    g_warning("creating a buffer file for %zu B failed: %s", pool_size,
              g_strerror(errno));
    return NULL;
  }
  if (fcntl(fd, F_SETFD, FD_CLOEXEC) < 0) {
    g_close(fd, NULL);
    return NULL;
  }
  if (ftruncate(fd, pool_size) < 0) {
    g_close(fd, NULL);
    return NULL;
  }

  data = mmap(NULL, pool_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    g_warning("mmap of size %zu failed: %s", pool_size, g_strerror(errno));
    close(fd);
    return NULL;
  }

  wayland_buffer_pool *pool;
  pool = g_new0(wayland_buffer_pool, 1);

  pool->width = width;
  pool->height = height;

  pool->buffers = g_new0(wayland_buffer, wayland->buffer_count);

  wl_pool = wl_shm_create_pool(wayland->shm, fd, pool_size);
  size_t i;
  for (i = 0; i < wayland->buffer_count; ++i) {
    pool->buffers[i].pool = pool;
    pool->buffers[i].buffer = wl_shm_pool_create_buffer(
        wl_pool, size * i, width, height, stride, WL_SHM_FORMAT_ARGB8888);
    pool->buffers[i].data = data + size * i;
    pool->buffers[i].released = TRUE;
    wl_buffer_add_listener(pool->buffers[i].buffer, &wayland_buffer_listener,
                           pool);
  }
  wl_shm_pool_destroy(wl_pool);
  close(fd);

  return pool;
}

void display_buffer_pool_free(wayland_buffer_pool *self) {
  if (self == NULL) {
    return;
  }
  self->to_free = TRUE;
  wayland_buffer_cleanup(self);
}

static void wayland_surface_protocol_enter(void *data,
                                           struct wl_surface *wl_surface,
                                           struct wl_output *wl_output) {
  wayland_output *output;

  output = g_hash_table_lookup(wayland->outputs, wl_output);
  if (output == NULL) {
    return;
  }

  if (config.dpi == 0 || config.dpi == 1) {
    // DPI auto-detect requested.
    config.dpi = wayland_output_get_dpi(output, output->current.scale, height);
    g_debug("Auto-detected DPI: %d", config.dpi);
  }

  wl_surface_set_buffer_scale(wl_surface, output->current.scale);

  if (wayland->scale != output->current.scale) {
    wayland->scale = output->current.scale;

    // create new buffers with the correct scaled size
    rofi_view_pool_refresh();

    RofiViewState *state = rofi_view_get_active();
    if (state != NULL) {
      rofi_view_set_size(state, -1, -1);
    }
  }
}

static void wayland_surface_protocol_leave(void *data,
                                           struct wl_surface *wl_surface,
                                           struct wl_output *wl_output) {}

static const struct wl_surface_listener wayland_surface_interface = {
    .enter = wayland_surface_protocol_enter,
    .leave = wayland_surface_protocol_leave,
};

static void wayland_frame_callback(void *data, struct wl_callback *callback,
                                   uint32_t time);

static const struct wl_callback_listener wayland_frame_wl_callback_listener = {
    .done = wayland_frame_callback,
};

cairo_surface_t *
display_buffer_pool_get_next_buffer(wayland_buffer_pool *pool) {
  wayland_buffer *buffer = NULL;
  size_t i;
  for (i = 0; (buffer == NULL) && (i < wayland->buffer_count); ++i) {
    buffer = pool->buffers + i;
    if (!buffer->released) {
      buffer = NULL;
    }
  }
  if (buffer == NULL) {
    return NULL;
  }

  cairo_surface_t *surface;

  surface = cairo_image_surface_create_for_data(
      buffer->data, CAIRO_FORMAT_ARGB32, pool->width, pool->height,
      cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, pool->width));
  cairo_surface_set_user_data(surface, &wayland_cairo_surface_user_data, buffer,
                              NULL);
  return surface;
}

void display_surface_commit(cairo_surface_t *surface) {
  if (surface == NULL || wayland->surface == NULL) {
    return;
  }

  wayland_buffer *buffer =
      cairo_surface_get_user_data(surface, &wayland_cairo_surface_user_data);
  wayland_buffer_pool *pool = buffer->pool;

  cairo_surface_destroy(surface);

  wl_surface_damage(wayland->surface, 0, 0, pool->width, pool->height);
  wl_surface_attach(wayland->surface, buffer->buffer, 0, 0);
  // FIXME: hidpi
  wl_surface_set_buffer_scale(wayland->surface, wayland->scale);
  buffer->released = FALSE;

  wl_surface_commit(wayland->surface);
}

static void wayland_frame_callback(void *data, struct wl_callback *callback,
                                   uint32_t timestamp) {
  if (wayland->frame_cb != NULL) {
    wl_callback_destroy(wayland->frame_cb);
    wayland->frame_cb = NULL;
    rofi_view_frame_callback();
  }
  if (wayland->surface != NULL) {
    wayland->frame_cb = wl_surface_frame(wayland->surface);
    wl_callback_add_listener(wayland->frame_cb,
                             &wayland_frame_wl_callback_listener, wayland);
  }
}

static void wayland_keyboard_keymap(void *data, struct wl_keyboard *keyboard,
                                    enum wl_keyboard_keymap_format format,
                                    int32_t fd, uint32_t size) {
  if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
    close(fd);
    return;
  }

  char *str = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (str == MAP_FAILED) {
    close(fd);
    return;
  }

  struct xkb_keymap *keymap = xkb_keymap_new_from_string(
      nk_bindings_seat_get_context(wayland->bindings_seat), str,
      XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
  if (keymap == NULL) {
    fprintf(stderr, "Failed to get Keymap for current keyboard device.\n");
    return;
  }
  struct xkb_state *state = xkb_state_new(keymap);
  if (state == NULL) {
    fprintf(stderr,
            "Failed to get state object for current keyboard device.\n");
    return;
  }

  nk_bindings_seat_update_keymap(wayland->bindings_seat, keymap, state);
}

static void wayland_keyboard_enter(void *data, struct wl_keyboard *keyboard,
                                   uint32_t serial, struct wl_surface *surface,
                                   struct wl_array *keys) {
  wayland_seat *self = data;

  wayland->last_seat = self;
  self->serial = serial;

  uint32_t *key, *kend;
  for (key = keys->data, kend = key + keys->size / sizeof(*key); key < kend;
       ++key) {
    nk_bindings_seat_handle_key(wayland->bindings_seat, NULL, *key + 8,
                                NK_BINDINGS_KEY_STATE_PRESSED);
  }
}

static void wayland_keyboard_leave(void *data, struct wl_keyboard *keyboard,
                                   uint32_t serial,
                                   struct wl_surface *surface) {
  wayland_seat *self = data;
  // TODO?
}

static gboolean wayland_key_repeat(void *data) {
  wayland_seat *self = data;

  if (self->repeat.key == 0) {
    self->repeat.source = NULL;
    return G_SOURCE_REMOVE;
  }

  char *text = nk_bindings_seat_handle_key(wayland->bindings_seat, NULL,
                                           self->repeat.key,
                                           NK_BINDINGS_KEY_STATE_PRESS);

  RofiViewState *state = rofi_view_get_active();
  if (state == NULL) {
    return G_SOURCE_REMOVE;
  }

  if (text != NULL) {
    rofi_view_handle_text(state, text);
  }

  rofi_view_maybe_update(state);

  return G_SOURCE_CONTINUE;
}

static gboolean wayland_key_repeat_delay(void *data) {
  wayland_seat *self = data;

  if (self->repeat.key == 0) {
    return FALSE;
  }

  char *text = nk_bindings_seat_handle_key(wayland->bindings_seat, NULL,
                                           self->repeat.key,
                                           NK_BINDINGS_KEY_STATE_PRESS);

  RofiViewState *state = rofi_view_get_active();
  if (state == NULL) {
    return G_SOURCE_REMOVE;
  }

  if (text != NULL) {
    rofi_view_handle_text(state, text);
  }

  guint repeat_wait_ms = 30;
  if (self->repeat.rate != 0) {
    repeat_wait_ms = 1000 / self->repeat.rate;
  }
  guint source_id = g_timeout_add(repeat_wait_ms, wayland_key_repeat, data);
  self->repeat.source = g_main_context_find_source_by_id(NULL, source_id);

  rofi_view_maybe_update(state);

  return G_SOURCE_REMOVE;
}

static void wayland_keyboard_key(void *data, struct wl_keyboard *keyboard,
                                 uint32_t serial, uint32_t time, uint32_t key,
                                 enum wl_keyboard_key_state kstate) {
  RofiViewState *state = rofi_view_get_active();
  wayland_seat *self = data;

  wayland->last_seat = self;
  self->serial = serial;

  xkb_keycode_t keycode = key + 8;
  if (kstate == WL_KEYBOARD_KEY_STATE_RELEASED) {
    if (keycode == self->repeat.key) {
      self->repeat.key = 0;
      if (self->repeat.source != NULL) {
        g_source_destroy(self->repeat.source);
        self->repeat.source = NULL;
      }
    }
    nk_bindings_seat_handle_key(wayland->bindings_seat, NULL, keycode,
                                NK_BINDINGS_KEY_STATE_RELEASE);
  } else if (kstate == WL_KEYBOARD_KEY_STATE_PRESSED) {
    gchar *text = nk_bindings_seat_handle_key(
        wayland->bindings_seat, NULL, keycode, NK_BINDINGS_KEY_STATE_PRESS);

    if (self->repeat.source != NULL) {
      g_source_destroy(self->repeat.source);
      self->repeat.source = NULL;
    }

    if (state != NULL) {
      if (text != NULL) {
        rofi_view_handle_text(state, text);
      }
      self->repeat.key = keycode;
      guint source_id =
          g_timeout_add(self->repeat.delay, wayland_key_repeat_delay, data);
      self->repeat.source = g_main_context_find_source_by_id(NULL, source_id);
    }
  }

  if (state != NULL) {
    rofi_view_maybe_update(state);
  }
}

static void wayland_keyboard_modifiers(void *data, struct wl_keyboard *keyboard,
                                       uint32_t serial, uint32_t mods_depressed,
                                       uint32_t mods_latched,
                                       uint32_t mods_locked, uint32_t group) {
  wayland_seat *self = data;
  nk_bindings_seat_update_mask(wayland->bindings_seat, NULL, mods_depressed,
                               mods_latched, mods_locked, 0, 0, group);

  RofiViewState *state = rofi_view_get_active();
  if (state != NULL) {
    rofi_view_maybe_update(state);
  }
}

static void wayland_keyboard_repeat_info(void *data,
                                         struct wl_keyboard *keyboard,
                                         int32_t rate, int32_t delay) {
  wayland_seat *self = data;
  self->repeat.key = 0;
  self->repeat.rate = rate;
  self->repeat.delay = delay;
  if (self->repeat.source != NULL) {
    g_source_destroy(self->repeat.source);
    self->repeat.source = NULL;
  }
}

static const struct wl_keyboard_listener wayland_keyboard_listener = {
    .keymap = wayland_keyboard_keymap,
    .enter = wayland_keyboard_enter,
    .leave = wayland_keyboard_leave,
    .key = wayland_keyboard_key,
    .modifiers = wayland_keyboard_modifiers,
    .repeat_info = wayland_keyboard_repeat_info,
};

static gboolean wayland_cursor_reload_theme(guint scale);

static void wayland_cursor_set_image(int i) {
  struct wl_buffer *buffer;
  struct wl_cursor_image *image;
  image = wayland->cursor.cursor->images[i];

  wayland->cursor.image = image;
  buffer = wl_cursor_image_get_buffer(wayland->cursor.image);
  wl_surface_set_buffer_scale(wayland->cursor.surface, wayland->scale);
  wl_surface_attach(wayland->cursor.surface, buffer, 0, 0);
  wl_surface_damage(wayland->cursor.surface, 0, 0, wayland->cursor.image->width,
                    wayland->cursor.image->height);
  wl_surface_commit(wayland->cursor.surface);
}

static void wayland_cursor_frame_callback(void *data,
                                          struct wl_callback *callback,
                                          uint32_t time);

static const struct wl_callback_listener
    wayland_cursor_frame_wl_callback_listener = {
        .done = wayland_cursor_frame_callback,
};

static void wayland_cursor_frame_callback(void *data,
                                          struct wl_callback *callback,
                                          uint32_t time) {
  int i;

  if (wayland->cursor.frame_cb != NULL) {
    wl_callback_destroy(wayland->cursor.frame_cb);
  }
  wayland->cursor.frame_cb = wl_surface_frame(wayland->cursor.surface);
  wl_callback_add_listener(wayland->cursor.frame_cb,
                           &wayland_cursor_frame_wl_callback_listener, wayland);

  i = wl_cursor_frame(wayland->cursor.cursor, time);
  wayland_cursor_set_image(i);
}

static void wayland_pointer_send_events(wayland_seat *self) {
  RofiViewState *state = rofi_view_get_active();

  if (state == NULL) {
    return;
  }

  if (self->motion.x > -1 || self->motion.y > -1) {
    rofi_view_handle_mouse_motion(state, self->motion.x, self->motion.y,
                                  config.hover_select);
    self->motion.x = -1;
    self->motion.y = -1;
  }

  NkBindingsMouseButton button = -1;
  switch (self->button.button) {
  case BTN_LEFT:
    button = NK_BINDINGS_MOUSE_BUTTON_PRIMARY;
    break;
  case BTN_RIGHT:
    button = NK_BINDINGS_MOUSE_BUTTON_SECONDARY;
    break;
  case BTN_MIDDLE:
    button = NK_BINDINGS_MOUSE_BUTTON_MIDDLE;
    break;
  }

  if (self->button.button > 0) {
    if (self->button.pressed) {
      rofi_view_handle_mouse_motion(state, self->button.x, self->button.y,
                                    FALSE);
      nk_bindings_seat_handle_button(wayland->bindings_seat, NULL, button,
                                     NK_BINDINGS_BUTTON_STATE_PRESS,
                                     self->button.time);
    } else {
      nk_bindings_seat_handle_button(wayland->bindings_seat, NULL, button,
                                     NK_BINDINGS_BUTTON_STATE_RELEASE,
                                     self->button.time);
    }
    self->button.button = 0;
  }

  if (self->axis_source == WL_POINTER_AXIS_SOURCE_FINGER ||
      self->axis_source == WL_POINTER_AXIS_SOURCE_CONTINUOUS) {
    self->wheel.vertical += 20 * self->wheel_continuous.vertical;
    self->wheel.horizontal += 20 * self->wheel_continuous.horizontal;
  }

  if (abs(self->wheel.vertical) >= 120) {
    gint v120 = self->wheel.vertical;
    nk_bindings_seat_handle_scroll(wayland->bindings_seat, NULL,
                                   NK_BINDINGS_SCROLL_AXIS_VERTICAL,
                                   v120 / 120);
    if (v120 > 0) {
      self->wheel.vertical = v120 % 120;
    } else {
      self->wheel.vertical = -((-v120) % 120);
    }
  }

  if (abs(self->wheel.horizontal) >= 120) {
    gint v120 = self->wheel.horizontal;
    nk_bindings_seat_handle_scroll(wayland->bindings_seat, NULL,
                                   NK_BINDINGS_SCROLL_AXIS_HORIZONTAL,
                                   v120 / 120);
    if (v120 > 0) {
      self->wheel.horizontal = v120 % 120;
    } else {
      self->wheel.horizontal = -((-v120) % 120);
    }
  }

  self->axis_source = 0;
  self->wheel_continuous.vertical = 0;
  self->wheel_continuous.horizontal = 0;

  rofi_view_maybe_update(state);
}

static struct wl_cursor *
rofi_cursor_type_to_wl_cursor(struct wl_cursor_theme *theme,
                              RofiCursorType type) {
  static const char *const default_names[] = {
      "default", "left_ptr", "top_left_arrow", "left-arrow", NULL};
  static const char *const pointer_names[] = {"pointer", "hand1", NULL};
  static const char *const text_names[] = {"text", "xterm", NULL};

  const char *const *name;
  struct wl_cursor *cursor = NULL;

  switch (type) {
  case ROFI_CURSOR_POINTER:
    name = pointer_names;
    break;
  case ROFI_CURSOR_TEXT:
    name = text_names;
    break;
  default:
    name = default_names;
    break;
  }
  for (; cursor == NULL && *name != NULL; ++name) {
    cursor = wl_cursor_theme_get_cursor(theme, *name);
  }
  return cursor;
}

#ifdef HAVE_WAYLAND_CURSOR_SHAPE
static enum wp_cursor_shape_device_v1_shape
rofi_cursor_type_to_wp_cursor_shape(RofiCursorType type) {
  switch (type) {
  case ROFI_CURSOR_POINTER:
    return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER;
  case ROFI_CURSOR_TEXT:
    return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_TEXT;
  default:
    return WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT;
  }
}
#endif

static void wayland_cursor_update_for_seat(wayland_seat *seat) {
#ifdef HAVE_WAYLAND_CURSOR_SHAPE
  if (seat->cursor_shape_device != NULL) {
    wp_cursor_shape_device_v1_set_shape(
        seat->cursor_shape_device, seat->pointer_serial,
        rofi_cursor_type_to_wp_cursor_shape(wayland->cursor.type));
    return;
  } else if (wayland->cursor.theme == NULL) {
    // cursor-shape-v1 is available, but the seat haven't seen a pointer yet
    return;
  }
#endif

  if (wayland->cursor.surface == NULL) {
    wayland->cursor.surface = wl_compositor_create_surface(wayland->compositor);
  }

  if (wayland->cursor.cursor->image_count < 2) {
    wayland_cursor_set_image(0);
  } else {
    wayland_cursor_frame_callback(wayland, wayland->cursor.frame_cb, 0);
  }

  wl_pointer_set_cursor(
      seat->pointer, seat->pointer_serial, wayland->cursor.surface,
      wayland->cursor.image->hotspot_x / wayland->cursor.scale,
      wayland->cursor.image->hotspot_y / wayland->cursor.scale);
}

static void wayland_pointer_enter(void *data, struct wl_pointer *pointer,
                                  uint32_t serial, struct wl_surface *surface,
                                  wl_fixed_t x, wl_fixed_t y) {
  wayland_seat *self = data;

  self->pointer_serial = serial;

#ifdef HAVE_WAYLAND_CURSOR_SHAPE
  if (wayland->cursor_shape_manager != NULL) {
    if (self->cursor_shape_device == NULL) {
      self->cursor_shape_device = wp_cursor_shape_manager_v1_get_pointer(
          wayland->cursor_shape_manager, pointer);
    }
  } else
#endif
      if (!wayland_cursor_reload_theme(wayland->scale)) {
    return;
  }

  wayland_cursor_update_for_seat(self);
}

void wayland_display_set_cursor_type(RofiCursorType type) {
  wayland_seat *seat;
  GHashTableIter iter;
  struct wl_cursor *cursor;

  if (wayland->cursor.type == type) {
    return;
  }
  wayland->cursor.type = type;

#ifdef HAVE_WAYLAND_CURSOR_SHAPE
  if (wayland->cursor_shape_manager == NULL)
#endif
  {
    if (wayland->cursor.theme == NULL) {
      return;
    }

    cursor = rofi_cursor_type_to_wl_cursor(wayland->cursor.theme, type);
    if (cursor == NULL) {
      g_info("Failed to load cursor type %d", type);
      return;
    }
    wayland->cursor.cursor = cursor;
  }

  g_hash_table_iter_init(&iter, wayland->seats);
  while (g_hash_table_iter_next(&iter, NULL, (gpointer *)&seat)) {
    if (seat->pointer != NULL) {
      wayland_cursor_update_for_seat(seat);
    }
  }
}

static void wayland_pointer_leave(void *data, struct wl_pointer *pointer,
                                  uint32_t serial, struct wl_surface *surface) {
  wayland_seat *self = data;

  if (wayland->cursor.frame_cb != NULL) {
    wl_callback_destroy(wayland->cursor.frame_cb);
    wayland->cursor.frame_cb = NULL;
  }
}

static void wayland_pointer_motion(void *data, struct wl_pointer *pointer,
                                   uint32_t time, wl_fixed_t x, wl_fixed_t y) {
  wayland_seat *self = data;

  self->button.x = wl_fixed_to_int(x);
  self->button.y = wl_fixed_to_int(y);
  self->motion.x = wl_fixed_to_int(x);
  self->motion.y = wl_fixed_to_int(y);
  self->motion.time = time;
}

static void wayland_pointer_button(void *data, struct wl_pointer *pointer,
                                   uint32_t serial, uint32_t time,
                                   uint32_t button,
                                   enum wl_pointer_button_state state) {
  wayland_seat *self = data;

  wayland->last_seat = self;
  self->serial = serial;

  self->button.time = time;
  self->button.pressed = (state == WL_POINTER_BUTTON_STATE_PRESSED);
  self->button.button = button;
}

static void wayland_pointer_axis(void *data, struct wl_pointer *pointer,
                                 uint32_t time, enum wl_pointer_axis axis,
                                 wl_fixed_t value) {
  wayland_seat *self = data;

  switch (axis) {
  case WL_POINTER_AXIS_VERTICAL_SCROLL:
    self->wheel_continuous.vertical += wl_fixed_to_double(value);
    break;
  case WL_POINTER_AXIS_HORIZONTAL_SCROLL:
    self->wheel_continuous.horizontal += wl_fixed_to_double(value);
    break;
  }
}

static void wayland_pointer_frame(void *data, struct wl_pointer *pointer) {
  wayland_seat *self = data;
  wayland_pointer_send_events(self);
}

static void
wayland_pointer_axis_source(void *data, struct wl_pointer *pointer,
                            enum wl_pointer_axis_source axis_source) {

  wayland_seat *self = data;
  self->axis_source = axis_source;
}

static void wayland_pointer_axis_stop(void *data, struct wl_pointer *pointer,
                                      uint32_t time,
                                      enum wl_pointer_axis axis) {}

static void wayland_pointer_axis_discrete(void *data,
                                          struct wl_pointer *pointer,
                                          enum wl_pointer_axis axis,
                                          int32_t discrete) {
  wayland_seat *self = data;

  // values are multiplied by 120 for compatibility with the
  // new high-resolution events
  switch (axis) {
  case WL_POINTER_AXIS_VERTICAL_SCROLL:
    self->wheel.vertical += discrete * 120;
    break;
  case WL_POINTER_AXIS_HORIZONTAL_SCROLL:
    self->wheel.horizontal += discrete * 120;
    break;
  }
}

#ifdef WL_POINTER_AXIS_VALUE120_SINCE_VERSION
static void wayland_pointer_axis120(void *data, struct wl_pointer *wl_pointer,
                                    enum wl_pointer_axis axis,
                                    int32_t value120) {
  wayland_seat *self = data;

  switch (axis) {
  case WL_POINTER_AXIS_VERTICAL_SCROLL:
    self->wheel.vertical += value120;
    break;
  case WL_POINTER_AXIS_HORIZONTAL_SCROLL:
    self->wheel.horizontal += value120;
    break;
  }
}
#endif

static const struct wl_pointer_listener wayland_pointer_listener = {
    .enter = wayland_pointer_enter,
    .leave = wayland_pointer_leave,
    .motion = wayland_pointer_motion,
    .button = wayland_pointer_button,
    .axis = wayland_pointer_axis,
    .frame = wayland_pointer_frame,
    .axis_source = wayland_pointer_axis_source,
    .axis_stop = wayland_pointer_axis_stop,
#ifdef WL_POINTER_AXIS_VALUE120_SINCE_VERSION
    .axis_value120 = wayland_pointer_axis120,
#endif
    .axis_discrete = wayland_pointer_axis_discrete,
};

static void wayland_keyboard_release(wayland_seat *self) {
  if (self->keyboard == NULL) {
    return;
  }

  wl_keyboard_release(self->keyboard);

  self->repeat.key = 0;
  if (self->repeat.source != NULL) {
    g_source_destroy(self->repeat.source);
    self->repeat.source = NULL;
  }

  self->keyboard = NULL;
}

#define CLIPBOARD_READ_INCREMENT 1024

struct clipboard_read_info {
  char *buffer;
  size_t size;
  int fd;
  ClipboardCb callback;
  void *user_data;
};

static gboolean clipboard_read_glib_callback(GIOChannel *channel,
                                             GIOCondition condition,
                                             gpointer opaque) {
  struct clipboard_read_info *info = opaque;
  gsize read;

  GIOStatus status =
      g_io_channel_read_chars(channel, info->buffer + info->size,
                              CLIPBOARD_READ_INCREMENT, &read, NULL);
  switch (status) {
  case G_IO_STATUS_AGAIN:
    return TRUE;

  case G_IO_STATUS_NORMAL: {
    info->size += read;
    info->buffer =
        g_realloc(info->buffer, info->size + CLIPBOARD_READ_INCREMENT);
    if (!info->buffer) {
      g_io_channel_shutdown(channel, FALSE, NULL);
      g_io_channel_unref(channel);
      close(info->fd);
      g_free(info);
      return FALSE;
    }
    return TRUE;
  }

  default:
    info->buffer[info->size] = '\0';
    if (status == G_IO_STATUS_EOF) {
      info->callback(info->buffer, info->user_data);
    } else { // G_IO_STATUS_ERROR
      g_warning("Could not read data from clipboard");
      g_free(info->buffer);
    }
    g_io_channel_shutdown(channel, FALSE, NULL);
    g_io_channel_unref(channel);
    close(info->fd);
    g_free(info);
    return FALSE;
  }
}

static gboolean clipboard_read_data(int fd, ClipboardCb callback,
                                    void *user_data) {
  GIOChannel *channel = g_io_channel_unix_new(fd);

  struct clipboard_read_info *info = g_malloc(sizeof *info);
  if (info == NULL) {
    g_io_channel_unref(channel);
    close(fd);
    return FALSE;
  }

  info->fd = fd;
  info->size = 0;
  info->callback = callback;
  info->user_data = user_data;
  info->buffer = g_malloc(CLIPBOARD_READ_INCREMENT);

  if (info->buffer == NULL) {
    g_io_channel_unref(channel);
    close(info->fd);
    g_free(info);
    return FALSE;
  }

  g_io_add_watch(channel, G_IO_IN | G_IO_HUP | G_IO_ERR,
                 clipboard_read_glib_callback, info);

  return TRUE;
}

static void data_offer_handle_offer(void *data, struct wl_data_offer *offer,
                                    const char *mime_type) {}

static void data_offer_handle_source_actions(
    void *data, struct wl_data_offer *wl_data_offer, uint32_t source_actions) {}

static void data_offer_handle_action(void *data,
                                     struct wl_data_offer *wl_data_offer,
                                     uint32_t dnd_action) {}

static const struct wl_data_offer_listener data_offer_listener = {
    .offer = data_offer_handle_offer,
    .source_actions = data_offer_handle_source_actions,
    .action = data_offer_handle_action,
};

static void data_device_handle_data_offer(void *data,
                                          struct wl_data_device *data_device,
                                          struct wl_data_offer *offer) {
  wl_data_offer_add_listener(offer, &data_offer_listener, NULL);
}

static void data_device_handle_enter(void *data,
                                     struct wl_data_device *wl_data_device,
                                     uint32_t serial,
                                     struct wl_surface *surface, wl_fixed_t x,
                                     wl_fixed_t y, struct wl_data_offer *id) {}

static void data_device_handle_leave(void *data,
                                     struct wl_data_device *wl_data_device) {}

static void data_device_handle_motion(void *data,
                                      struct wl_data_device *wl_data_device,
                                      uint32_t time, wl_fixed_t x,
                                      wl_fixed_t y) {}

static void data_device_handle_drop(void *data,
                                    struct wl_data_device *wl_data_device) {}

static void clipboard_handle_selection(enum clipboard_type cb_type,
                                       void *offer) {
  clipboard_data *clipboard = &wayland->clipboards[cb_type];

  if (clipboard->offer != NULL) {
    if (cb_type == CLIPBOARD_DEFAULT) {
      wl_data_offer_destroy(clipboard->offer);
    } else {
      zwp_primary_selection_offer_v1_destroy(clipboard->offer);
    }
  }
  clipboard->offer = offer;
}

static void data_device_handle_selection(void *data,
                                         struct wl_data_device *data_device,
                                         struct wl_data_offer *offer) {
  clipboard_handle_selection(CLIPBOARD_DEFAULT, offer);
}

static const struct wl_data_device_listener data_device_listener = {
    .data_offer = data_device_handle_data_offer,
    .enter = data_device_handle_enter,
    .leave = data_device_handle_leave,
    .motion = data_device_handle_motion,
    .drop = data_device_handle_drop,
    .selection = data_device_handle_selection,
};

static void
primary_selection_handle_offer(void *data,
                               struct zwp_primary_selection_offer_v1 *offer,
                               const char *mime_type) {}

static const struct zwp_primary_selection_offer_v1_listener
    primary_selection_offer_listener = {
        .offer = primary_selection_handle_offer,
};

static void primary_selection_device_handle_data_offer(
    void *data, struct zwp_primary_selection_device_v1 *data_device,
    struct zwp_primary_selection_offer_v1 *offer) {
  zwp_primary_selection_offer_v1_add_listener(
      offer, &primary_selection_offer_listener, NULL);
}

static void primary_selection_device_handle_selection(
    void *data, struct zwp_primary_selection_device_v1 *data_device,
    struct zwp_primary_selection_offer_v1 *offer) {
  clipboard_handle_selection(CLIPBOARD_PRIMARY, offer);
}

static const struct zwp_primary_selection_device_v1_listener
    primary_selection_device_listener = {
        .data_offer = primary_selection_device_handle_data_offer,
        .selection = primary_selection_device_handle_selection,
};

static void wayland_pointer_release(wayland_seat *self) {
  if (self->pointer == NULL) {
    return;
  }

#ifdef HAVE_WAYLAND_CURSOR_SHAPE
  if (self->cursor_shape_device != NULL) {
    wp_cursor_shape_device_v1_destroy(self->cursor_shape_device);
    self->cursor_shape_device = NULL;
  }
#endif

  wl_pointer_release(self->pointer);

  self->pointer = NULL;
}

static void wayland_seat_release(wayland_seat *self) {
  if (self->text_input) {
    zwp_text_input_v3_destroy(self->text_input);
    self->text_input = NULL;
  }
  wayland_keyboard_release(self);
  wayland_pointer_release(self);

  wl_seat_release(self->seat);

  g_hash_table_remove(wayland->seats, self->seat);

  g_free(self);
}

static void wayland_seat_capabilities(void *data, struct wl_seat *seat,
                                      uint32_t capabilities) {
  wayland_seat *self = data;

  if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) &&
      (self->keyboard == NULL)) {
    self->keyboard = wl_seat_get_keyboard(self->seat);
    wl_keyboard_add_listener(self->keyboard, &wayland_keyboard_listener, self);
    if (wayland->text_input_manager) {
      self->text_input = zwp_text_input_manager_v3_get_text_input(
          wayland->text_input_manager, seat);
      zwp_text_input_v3_add_listener(self->text_input, &text_input_listener,
                                     self);
    }
  } else if ((!(capabilities & WL_SEAT_CAPABILITY_POINTER)) &&
             (self->keyboard != NULL)) {
    wayland_keyboard_release(self);
  }

  if ((capabilities & WL_SEAT_CAPABILITY_POINTER) && (self->pointer == NULL)) {
    self->pointer = wl_seat_get_pointer(self->seat);
    wl_pointer_add_listener(self->pointer, &wayland_pointer_listener, self);
  } else if ((!(capabilities & WL_SEAT_CAPABILITY_POINTER)) &&
             (self->pointer != NULL)) {
    wayland_pointer_release(self);
  }

  if (wayland->data_device_manager != NULL) {
    self->data_device = wl_data_device_manager_get_data_device(
        wayland->data_device_manager, seat);
    wl_data_device_add_listener(self->data_device, &data_device_listener, NULL);
  }

  if (wayland->primary_selection_device_manager != NULL) {
    self->primary_selection_device =
        zwp_primary_selection_device_manager_v1_get_device(
            wayland->primary_selection_device_manager, seat);
    zwp_primary_selection_device_v1_add_listener(
        self->primary_selection_device, &primary_selection_device_listener,
        NULL);
  }
}

static void wayland_seat_name(void *data, struct wl_seat *seat,
                              const char *name) {
  wayland_seat *self = data;

  if (self->name != NULL) {
    g_hash_table_remove(wayland->seats_by_name, self->name);
  }
  self->name = g_strdup(name);
  g_hash_table_insert(wayland->seats_by_name, self->name, self);
}

static const struct wl_seat_listener wayland_seat_listener = {
    .capabilities = wayland_seat_capabilities,
    .name = wayland_seat_name,
};

static void update_cursor_rectangle(struct zwp_text_input_v3 *text_input) {
  textbox *tb = rofi_view_get_active_text();
  if (tb == NULL) {
    return;
  }

  widget *tb_widget = WIDGET(tb);
  int x = widget_get_x_pos(tb_widget) + textbox_get_cursor_x_pos(tb);
  int y = widget_get_y_pos(tb_widget);
  int w = 1;
  int h = widget_get_height(tb_widget);
  zwp_text_input_v3_set_cursor_rectangle(text_input, x, y, w, h);
}

static void text_input_enter(void *data, struct zwp_text_input_v3 *text_input,
                             struct wl_surface *surface) {
  zwp_text_input_v3_enable(text_input);
  zwp_text_input_v3_set_content_type(
      text_input, ZWP_TEXT_INPUT_V3_CONTENT_HINT_NONE,
      ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_NORMAL);
  update_cursor_rectangle(text_input);
  zwp_text_input_v3_commit(text_input);
}
static void text_input_leave(void *data, struct zwp_text_input_v3 *text_input,
                             struct wl_surface *surface) {
  zwp_text_input_v3_disable(text_input);
  zwp_text_input_v3_commit(text_input);
}

static void text_input_preedit_string(void *data,
                                      struct zwp_text_input_v3 *text_input,
                                      const char *text, int32_t cursor_begin,
                                      int32_t cursor_end) {
  update_cursor_rectangle(text_input);
  zwp_text_input_v3_commit(text_input);
}

static void text_input_commit_string(void *data,
                                     struct zwp_text_input_v3 *text_input,
                                     const char *text) {
  if (text == NULL) {
    return;
  }

  RofiViewState *state = rofi_view_get_active();
  if (state) {
    rofi_view_handle_text(state, text);
  }
}

static void
text_input_delete_surrounding_text(void *data,
                                   struct zwp_text_input_v3 *text_input,
                                   uint32_t before_length,
                                   uint32_t after_length) {}

static void text_input_done(void *data, struct zwp_text_input_v3 *text_input,
                            uint32_t serial) {}

static const struct zwp_text_input_v3_listener text_input_listener = {
    .enter = text_input_enter,
    .leave = text_input_leave,
    .preedit_string = text_input_preedit_string,
    .commit_string = text_input_commit_string,
    .delete_surrounding_text = text_input_delete_surrounding_text,
    .done = text_input_done,
};

static void wayland_output_release(wayland_output *self) {
  g_debug("Output release: %s", self->name);

  if (wl_output_get_version(self->output) >= WL_OUTPUT_RELEASE_SINCE_VERSION) {
    wl_output_release(self->output);
  } else {
    wl_output_destroy(self->output);
  }

  g_hash_table_remove(wayland->outputs, self->output);

  g_free(self->name);
  g_free(self);
}

static wayland_output *wayland_output_by_name(const char *name) {
#ifdef WL_OUTPUT_NAME_SINCE_VERSION
  GHashTableIter iter;
  wayland_output *output;

  g_debug("Monitor lookup  by name : %s", name);

  g_hash_table_iter_init(&iter, wayland->outputs);
  while (g_hash_table_iter_next(&iter, NULL, (gpointer *)&output)) {
    if (g_strcmp0(output->name, name) == 0) {
      return output;
    }
  }
#endif
  g_debug("Monitor lookup  by name failed: %s", name);

  return NULL;
}
double wayland_get_dpi_estimation(void) {
  double retv = -1.0;
  if ( wayland == 0 ) {
    return -1.0;
  }
  gsize noutputs = g_hash_table_size(wayland->outputs);
  if ( noutputs == 1) {
    GHashTableIter iter;
    wayland_output *output;
    g_hash_table_iter_init(&iter, wayland->outputs);
    if (g_hash_table_iter_next(&iter, NULL, (gpointer *)&output)) {
      return wayland_output_get_dpi(output, output->current.scale, height);
    }
  } else if (noutputs > 1 && config.monitor != NULL ) {
    wayland_output *output = wayland_output_by_name(config.monitor);
    if (output != NULL) {
      return wayland_output_get_dpi(output, output->current.scale, height);
    }
  }
  return retv;
}

static void wayland_output_done(void *data, struct wl_output *output) {
  wayland_output *self = data;

  self->current = self->pending;

  g_debug("Output %s: %" PRIi32 "x%" PRIi32 " (%" PRIi32 "x%" PRIi32 "mm)"
          " position %" PRIi32 "x%" PRIi32 " scale %" PRIi32
          " transform %" PRIi32,
          self->name ? self->name : "Unknown", self->current.width,
          self->current.height, self->current.physical_width,
          self->current.physical_height, self->current.x, self->current.y,
          self->current.scale, self->current.transform);
}

static void wayland_output_geometry(void *data, struct wl_output *output,
                                    int32_t x, int32_t y, int32_t width,
                                    int32_t height, int32_t subpixel,
                                    const char *make, const char *model,
                                    int32_t transform) {
  wayland_output *self = data;

  self->pending.x = x;
  self->pending.y = y;
  self->pending.physical_width = width;
  self->pending.physical_height = height;
  self->pending.transform = transform;
}

static void wayland_output_mode(void *data, struct wl_output *output,
                                enum wl_output_mode flags, int32_t width,
                                int32_t height, int32_t refresh) {
  if (flags & WL_OUTPUT_MODE_CURRENT) {
    wayland_output *self = data;
    self->pending.width = width;
    self->pending.height = height;
  }
}

static void wayland_output_scale(void *data, struct wl_output *output,
                                 int32_t scale) {
  wayland_output *self = data;

  self->pending.scale = scale;
}

#ifdef WL_OUTPUT_NAME_SINCE_VERSION
static void wayland_output_name(void *data, struct wl_output *output,
                                const char *name) {
  wayland_output *self = data;

  g_free(self->name);
  self->name = g_strdup(name);
}
#endif

#ifdef WL_OUTPUT_DESCRIPTION_SINCE_VERSION
static void wayland_output_description(void *data, struct wl_output *output,
                                       const char *name) {}
#endif

static const struct wl_output_listener wayland_output_listener = {
    .geometry = wayland_output_geometry,
    .mode = wayland_output_mode,
    .scale = wayland_output_scale,
    .done = wayland_output_done,
#ifdef WL_OUTPUT_NAME_SINCE_VERSION
    .name = wayland_output_name,
#endif
#ifdef WL_OUTPUT_DESCRIPTION_SINCE_VERSION
    .description = wayland_output_description,
#endif
};

static void wayland_registry_handle_global(void *data,
                                           struct wl_registry *registry,
                                           uint32_t name, const char *interface,
                                           uint32_t version) {
  g_debug("wayland registry: interface %s", interface);

  if (g_strcmp0(interface, wl_compositor_interface.name) == 0) {
    wayland->global_names[WAYLAND_GLOBAL_COMPOSITOR] = name;
    wayland->compositor =
        wl_registry_bind(registry, name, &wl_compositor_interface,
                         MIN(version, WL_COMPOSITOR_INTERFACE_VERSION));
  } else if (g_strcmp0(interface, zwlr_layer_shell_v1_interface.name) == 0) {
    wayland->global_names[WAYLAND_GLOBAL_LAYER_SHELL] = name;
    wayland->layer_shell =
        wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface,
                         MIN(version, WL_LAYER_SHELL_INTERFACE_VERSION));
  } else if (g_strcmp0(
                 interface,
                 zwp_keyboard_shortcuts_inhibit_manager_v1_interface.name) ==
             0) {
    wayland->global_names[WAYLAND_GLOBAL_KEYBOARD_SHORTCUTS_INHIBITOR] = name;
    wayland->kb_shortcuts_inhibit_manager = wl_registry_bind(
        registry, name, &zwp_keyboard_shortcuts_inhibit_manager_v1_interface,
        MIN(version, WL_KEYBOARD_SHORTCUTS_INHIBITOR_INTERFACE_VERSION));
  } else if (g_strcmp0(interface, wl_shm_interface.name) == 0) {
    wayland->global_names[WAYLAND_GLOBAL_SHM] = name;
    wayland->shm = wl_registry_bind(registry, name, &wl_shm_interface,
                                    MIN(version, WL_SHM_INTERFACE_VERSION));
  } else if (g_strcmp0(interface, wl_seat_interface.name) == 0) {
    if (version < WL_SEAT_INTERFACE_MIN_VERSION) {
      g_error("Minimum version of wayland seat interface is %u, got %u",
              WL_SEAT_INTERFACE_MIN_VERSION, version);
      return;
    }
    version = MIN(version, WL_SEAT_INTERFACE_MAX_VERSION);

    wayland_seat *seat = g_new0(wayland_seat, 1);
    seat->context = wayland;
    seat->global_name = name;
    seat->seat = wl_registry_bind(registry, name, &wl_seat_interface, version);
    g_hash_table_insert(wayland->seats, seat->seat, seat);

    wl_seat_add_listener(seat->seat, &wayland_seat_listener, seat);
  } else if (g_strcmp0(interface, wl_output_interface.name) == 0) {
    if (version < WL_OUTPUT_INTERFACE_MIN_VERSION) {
      g_error("Minimum version of wayland output interface is %u, got %u",
              WL_OUTPUT_INTERFACE_MIN_VERSION, version);
      return;
    }
    version = MIN(version, WL_OUTPUT_INTERFACE_MAX_VERSION);

    wayland_output *output = g_new0(wayland_output, 1);
    output->context = wayland;
    output->global_name = name;
    output->output =
        wl_registry_bind(registry, name, &wl_output_interface, version);
    output->pending.scale = 1;
    output->current = output->pending;

    g_hash_table_insert(wayland->outputs, output->output, output);

    wl_output_add_listener(output->output, &wayland_output_listener, output);
  } else if (strcmp(interface, wl_data_device_manager_interface.name) == 0) {
    wayland->data_device_manager =
        wl_registry_bind(registry, name, &wl_data_device_manager_interface, 3);
  } else if (strcmp(interface,
                    zwp_primary_selection_device_manager_v1_interface.name) ==
             0) {
    wayland->primary_selection_device_manager = wl_registry_bind(
        registry, name, &zwp_primary_selection_device_manager_v1_interface, 1);
  }
#ifdef HAVE_WAYLAND_CURSOR_SHAPE
  else if (strcmp(interface, wp_cursor_shape_manager_v1_interface.name) == 0) {
    wayland->global_names[WAYLAND_GLOBAL_CURSOR_SHAPE] = name;
    wayland->cursor_shape_manager = wl_registry_bind(
        registry, name, &wp_cursor_shape_manager_v1_interface, 1);
  }
#endif
  else if (strcmp(interface, zwp_text_input_manager_v3_interface.name) == 0) {
    wayland->text_input_manager = wl_registry_bind(
        registry, name, &zwp_text_input_manager_v3_interface, 1);
  }
}

static void wayland_registry_handle_global_remove(void *data,
                                                  struct wl_registry *registry,
                                                  uint32_t name) {
  wayland_global_name i;
  for (i = 0; i < _WAYLAND_GLOBAL_SIZE; ++i) {
    if (wayland->global_names[i] != name) {
      continue;
    }
    wayland->global_names[i] = 0;

    switch (i) {
    case WAYLAND_GLOBAL_COMPOSITOR:
      wl_compositor_destroy(wayland->compositor);
      wayland->compositor = NULL;
      break;
    case WAYLAND_GLOBAL_CURSOR_SHAPE:
#ifdef HAVE_WAYLAND_CURSOR_SHAPE
      wp_cursor_shape_manager_v1_destroy(wayland->cursor_shape_manager);
      wayland->cursor_shape_manager = NULL;
#endif
      break;
    case WAYLAND_GLOBAL_LAYER_SHELL:
      zwlr_layer_shell_v1_destroy(wayland->layer_shell);
      wayland->layer_shell = NULL;
      break;
    case WAYLAND_GLOBAL_KEYBOARD_SHORTCUTS_INHIBITOR:
      zwp_keyboard_shortcuts_inhibit_manager_v1_destroy(
          wayland->kb_shortcuts_inhibit_manager);
      wayland->kb_shortcuts_inhibit_manager = NULL;
      break;
    case WAYLAND_GLOBAL_SHM:
      wl_shm_destroy(wayland->shm);
      wayland->shm = NULL;
      break;
    case _WAYLAND_GLOBAL_SIZE:
      g_assert_not_reached();
    }
    return;
  }
  if ((wayland->cursor.theme != NULL) &&
      ((wayland->compositor == NULL) || (wayland->shm == NULL))) {
    if (wayland->cursor.frame_cb != NULL) {
      wl_callback_destroy(wayland->cursor.frame_cb);
      wayland->cursor.frame_cb = NULL;
    }

    wl_surface_destroy(wayland->cursor.surface);
    wl_cursor_theme_destroy(wayland->cursor.theme);
    wayland->cursor.surface = NULL;
    wayland->cursor.image = NULL;
    wayland->cursor.cursor = NULL;
    wayland->cursor.theme = NULL;
    wayland->cursor.scale = 0;
  }

  GHashTableIter iter;

  wayland_seat *seat;
  g_hash_table_iter_init(&iter, wayland->seats);
  while (g_hash_table_iter_next(&iter, NULL, (gpointer *)&seat)) {
    if (seat->global_name != name) {
      continue;
    }

    g_hash_table_iter_remove(&iter);
    wayland_seat_release(seat);
    return;
  }

  wayland_output *output;
  g_hash_table_iter_init(&iter, wayland->outputs);
  while (g_hash_table_iter_next(&iter, NULL, (gpointer *)&output)) {
    if (output->global_name != name) {
      continue;
    }

    g_hash_table_iter_remove(&iter);
    wayland_output_release(output);
    return;
  }
}

static const struct wl_registry_listener wayland_registry_listener = {
    .global = wayland_registry_handle_global,
    .global_remove = wayland_registry_handle_global_remove,
};

static void wayland_layer_shell_surface_configure(
    void *data, struct zwlr_layer_surface_v1 *surface, uint32_t serial,
    uint32_t width, uint32_t height) {
  wayland->layer_width = width;
  wayland->layer_height = height;
  zwlr_layer_surface_v1_ack_configure(surface, serial);
}

static void wayland_surface_destroy(void) {
  if (wayland->wlr_surface != NULL) {
    zwlr_layer_surface_v1_destroy(wayland->wlr_surface);
    wayland->wlr_surface = NULL;
  }
  if (wayland->surface != NULL) {
    wl_surface_destroy(wayland->surface);
    wayland->surface = NULL;
  }
}

static void
wayland_layer_shell_surface_closed(void *data,
                                   struct zwlr_layer_surface_v1 *surface) {
  g_debug("Layer shell surface closed");

  wayland_surface_destroy();

  // In this case, we recreate the layer shell surface the best we can and
  // re-initialize everything:

  // recreate layer shell
  wayland_display_late_setup();

  // create new buffers with the correct scaled size
  rofi_view_pool_refresh();

  RofiViewState *state = rofi_view_get_active();
  if (state != NULL) {
    rofi_view_set_size(state, -1, -1);
  }
}

static const struct zwlr_layer_surface_v1_listener
    wayland_layer_shell_surface_listener = {
        .configure = wayland_layer_shell_surface_configure,
        .closed = wayland_layer_shell_surface_closed,
};

static gboolean wayland_error(gpointer user_data) {
  g_main_loop_quit(wayland->main_loop);
  return G_SOURCE_REMOVE;
}

static gboolean wayland_cursor_reload_theme(guint scale) {
  if (wayland->cursor.theme != NULL) {
    if (wayland->cursor.scale == scale)
      return TRUE;

    wl_cursor_theme_destroy(wayland->cursor.theme);
    wayland->cursor.theme = NULL;
    wayland->cursor.cursor = NULL;
  }

  guint64 cursor_size = 24;
  char *env_cursor_size = (char *)g_getenv("XCURSOR_SIZE");
  if (env_cursor_size && strlen(env_cursor_size) > 0) {
    guint64 size = g_ascii_strtoull(env_cursor_size, NULL, 10);
    if (0 < size && size < G_MAXUINT64) {
      cursor_size = size;
    }
  }
  cursor_size *= scale;

  wayland->cursor.theme = wl_cursor_theme_load(wayland->cursor.theme_name,
                                               cursor_size, wayland->shm);
  if (wayland->cursor.theme != NULL) {
    wayland->cursor.cursor = rofi_cursor_type_to_wl_cursor(
        wayland->cursor.theme, wayland->cursor.type);
    if (wayland->cursor.cursor == NULL) {
      wl_cursor_theme_destroy(wayland->cursor.theme);
      wayland->cursor.theme = NULL;
      return FALSE;
    } else {
      wayland->cursor.scale = scale;
      return TRUE;
    }
  }
  return FALSE;
}

static gboolean wayland_display_setup(GMainLoop *main_loop,
                                      NkBindings *bindings) {
  wayland->main_loop = main_loop;

  char *display = (char *)g_getenv("WAYLAND_DISPLAY");
  wayland->main_loop_source = g_water_wayland_source_new(NULL, display);
  if (wayland->main_loop_source == NULL) {
    g_warning("Could not connect to the Wayland compositor");
    return FALSE;
  }

  g_water_wayland_source_set_error_callback(wayland->main_loop_source,
                                            wayland_error, NULL, NULL);

  wayland->buffer_count = 3;
  wayland->cursor.type = ROFI_CURSOR_DEFAULT;
  wayland->scale = 1;

  wayland->outputs = g_hash_table_new(g_direct_hash, g_direct_equal);
  wayland->seats = g_hash_table_new(g_direct_hash, g_direct_equal);
  wayland->seats_by_name =
      g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

  wayland->display =
      g_water_wayland_source_get_display(wayland->main_loop_source);
  wayland->registry = wl_display_get_registry(wayland->display);
  wl_registry_add_listener(wayland->registry, &wayland_registry_listener, NULL);
  wl_display_roundtrip(wayland->display);

  if (wayland->compositor == NULL || wayland->shm == NULL ||
      g_hash_table_size(wayland->outputs) == 0 ||
      g_hash_table_size(wayland->seats) == 0) {
    g_error("Could not connect to wayland compositor");
    return FALSE;
  }
  if (wayland->layer_shell == NULL) {
    g_error("Rofi on wayland requires support for the layer shell protocol");
    return FALSE;
  }

  wayland->bindings_seat = nk_bindings_seat_new(bindings, XKB_CONTEXT_NO_FLAGS);

  // Wait for output information
  wl_display_roundtrip(wayland->display);

  return TRUE;
}

static gboolean wayland_display_late_setup(void) {
  wayland_output *output = wayland_output_by_name(config.monitor);

  struct wl_output *wlo = NULL;
  if (output != NULL) {
    wlo = output->output;
  }
  wayland->surface = wl_compositor_create_surface(wayland->compositor);

  uint32_t layer = ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY;
  if (strcmp(config.wayland_layer, "overlay") == 0) {
    layer = ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY;
  } else if (strcmp(config.wayland_layer, "top") == 0) {
    layer = ZWLR_LAYER_SHELL_V1_LAYER_TOP;
  } else if (strcmp(config.wayland_layer, "bottom") == 0) {
    layer = ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;
  } else if (strcmp(config.wayland_layer, "background") == 0) {
    layer = ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND;
  } else {
    layer = ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY;
    g_warning("Unknown wayland layer: %s, using default overlay", config.wayland_layer);
  }
  wayland->wlr_surface = zwlr_layer_shell_v1_get_layer_surface(
      wayland->layer_shell, wayland->surface, wlo, layer, "rofi");

  // Set size zero and anchor on all corners to get the usable screen size
  // see https://github.com/swaywm/wlroots/pull/2422
  zwlr_layer_surface_v1_set_anchor(wayland->wlr_surface,
                                   ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                       ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                       ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                       ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
  zwlr_layer_surface_v1_set_size(wayland->wlr_surface, 0, 0);
  zwlr_layer_surface_v1_set_keyboard_interactivity(wayland->wlr_surface, 1);
  zwlr_layer_surface_v1_add_listener(
      wayland->wlr_surface, &wayland_layer_shell_surface_listener, NULL);

  if (config.global_kb && wayland->kb_shortcuts_inhibit_manager) {
    g_debug("inhibit shortcuts from compositor");
    GHashTableIter iter;
    wayland_seat *seat;
    g_hash_table_iter_init(&iter, wayland->seats);
    while (g_hash_table_iter_next(&iter, NULL, (gpointer *)&seat)) {
      // we don't need to keep track of these, they will get inactive when the
      // surface is destroyed
      zwp_keyboard_shortcuts_inhibit_manager_v1_inhibit_shortcuts(
          wayland->kb_shortcuts_inhibit_manager, wayland->surface, seat->seat);
    }
  }

  wl_surface_add_listener(wayland->surface, &wayland_surface_interface,
                          wayland);
  wl_surface_commit(wayland->surface);
  wl_display_roundtrip(wayland->display);
  wayland_frame_callback(wayland, wayland->frame_cb, 0);

  return TRUE;
}

gboolean display_get_surface_dimensions(int *width, int *height) {
  if (wayland->layer_width != 0) {
    if (width != NULL) {
      *width = wayland->layer_width;
    }
    if (height != NULL) {
      *height = wayland->layer_height;
    }
    return TRUE;
  }
  return FALSE;
}

void display_set_surface_dimensions(int width, int height, int x_margin,
                                    int y_margin, int loc) {

  wayland->layer_width = width;
  wayland->layer_height = height;
  zwlr_layer_surface_v1_set_size(wayland->wlr_surface, width, height);

  uint32_t wlr_anchor = 0;
  switch (loc) {
  case WL_NORTH_WEST:
    wlr_anchor =
        ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
    break;
  case WL_NORTH:
    wlr_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
    break;
  case WL_NORTH_EAST:
    wlr_anchor =
        ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
    break;
  case WL_EAST:
    wlr_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
    break;
  case WL_SOUTH_EAST:
    wlr_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
                 ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
    break;
  case WL_SOUTH:
    wlr_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
    break;
  case WL_SOUTH_WEST:
    wlr_anchor =
        ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
    break;
  case WL_WEST:
    wlr_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
    break;
  case WL_CENTER:
  default:
    break;
  }

  if (height == 0) {
    wlr_anchor |=
        ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
  }
  if (width == 0) {
    wlr_anchor |=
        ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
  }

  zwlr_layer_surface_v1_set_anchor(wayland->wlr_surface, wlr_anchor);

  // NOTE: Setting margin for edges we are not anchored to has no effect, so
  // we can safely set contradictory margins (e.g. top vs bottom) - at most
  // one of the margins on a given axis will have effect. This also means that
  // margin has no effect if the window is centered. :(
  zwlr_layer_surface_v1_set_margin(wayland->wlr_surface, y_margin, -x_margin,
                                   -y_margin, x_margin);
}

static void wayland_display_early_cleanup(void) {
  if (wayland->main_loop_source == NULL) {
    return;
  }

  wayland_surface_destroy();
  wl_display_flush(wayland->display);
}

static void wayland_display_cleanup(void) {
  if (wayland->main_loop_source == NULL) {
    return;
  }

  nk_bindings_seat_free(wayland->bindings_seat);
  g_hash_table_unref(wayland->seats_by_name);
  g_hash_table_unref(wayland->seats);
  g_hash_table_unref(wayland->outputs);
  wl_registry_destroy(wayland->registry);
  wl_display_flush(wayland->display);
  g_water_wayland_source_free(wayland->main_loop_source);
}

static void wayland_display_dump_monitor_layout(void) {
  int is_term = isatty(fileno(stdout));
  GHashTableIter iter;
  wayland_output *output;

  g_hash_table_iter_init(&iter, wayland->outputs);
  printf("Monitor layout:\n");
  while (g_hash_table_iter_next(&iter, NULL, (gpointer *)&output)) {
    printf("%s              ID%s: %" PRIu32, is_term ? color_bold : "",
           is_term ? color_reset : "", output->global_name);
    printf("\n");
    printf("%s            name%s: %s\n", is_term ? color_bold : "",
           is_term ? color_reset : "", output->name);
    printf("%s           scale%s: %" PRIi32 "\n", is_term ? color_bold : "",
           is_term ? color_reset : "", output->current.scale);
    printf("%s        position%s: %" PRIi32 ",%" PRIi32 "\n",
           is_term ? color_bold : "", is_term ? color_reset : "",
           output->current.x, output->current.y);
    printf("%s            size%s: %" PRIi32 ",%" PRIi32 "\n",
           is_term ? color_bold : "", is_term ? color_reset : "",
           output->current.width, output->current.height);

    if (output->current.physical_width > 0 &&
        output->current.physical_height > 0) {
      printf("%s            size%s: %" PRIi32 "mm,%" PRIi32
             "mm  dpi: %.0f,%.0f\n",
             is_term ? color_bold : "", is_term ? color_reset : "",
             output->current.physical_width, output->current.physical_height,
             wayland_output_get_dpi(output, output->current.scale, width),
             wayland_output_get_dpi(output, output->current.scale, height));
    }
    printf("\n");
  }
}

static void
wayland_display_startup_notification(RofiHelperExecuteContext *context,
                                     GSpawnChildSetupFunc *child_setup,
                                     gpointer *user_data) {}

static int wayland_display_monitor_active(workarea *mon) {
  // TODO: do something?
  return FALSE;
}

static void wayland_display_set_input_focus(guint w) {}

static void wayland_display_revert_input_focus(void) {}

static const struct _view_proxy *wayland_display_view_proxy(void) {
  return wayland_view_proxy;
}

static guint wayland_display_scale(void) { return wayland->scale; }

static void wayland_get_clipboard_data(int cb_type, ClipboardCb callback,
                                       void *user_data) {
  clipboard_data *clipboard = &wayland->clipboards[cb_type];

  if (clipboard->offer == NULL) {
    return;
  }

  int fds[2];
  if (pipe(fds) < 0) {
    return;
  }

  if (cb_type == CLIPBOARD_DEFAULT) {
    wl_data_offer_receive(clipboard->offer, "text/plain;charset=utf-8", fds[1]);
  } else {
    zwp_primary_selection_offer_v1_receive(clipboard->offer, "text/plain;charset=utf-8",
                                           fds[1]);
  }
  close(fds[1]);

  clipboard_read_data(fds[0], callback, user_data);
}

static void wayland_set_fullscreen_mode(void) {
  if (!wayland->wlr_surface) {
    return;
  }
  zwlr_layer_surface_v1_set_exclusive_zone(wayland->wlr_surface, -1);
  zwlr_layer_surface_v1_set_size(wayland->wlr_surface, 0, 0);
  wl_surface_commit(wayland->surface);
  wl_display_roundtrip(wayland->display);

  rofi_view_pool_refresh();
}

static display_proxy display_ = {
    .setup = wayland_display_setup,
    .late_setup = wayland_display_late_setup,
    .early_cleanup = wayland_display_early_cleanup,
    .cleanup = wayland_display_cleanup,
    .dump_monitor_layout = wayland_display_dump_monitor_layout,
    .startup_notification = wayland_display_startup_notification,
    .monitor_active = wayland_display_monitor_active,
    .set_input_focus = wayland_display_set_input_focus,
    .revert_input_focus = wayland_display_revert_input_focus,
    .scale = wayland_display_scale,
    .get_clipboard_data = wayland_get_clipboard_data,
    .set_fullscreen_mode = wayland_set_fullscreen_mode,

    .view = wayland_display_view_proxy,
};

display_proxy *const wayland_proxy = &display_;
