#ifndef ROFI_WAYLAND_INTERNAL_H
#define ROFI_WAYLAND_INTERNAL_H

#include <cairo.h>
#include <glib.h>
#include <libgwater-wayland.h>
#include <nkutils-bindings.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "wayland.h"

typedef enum {
  WAYLAND_GLOBAL_COMPOSITOR,
  WAYLAND_GLOBAL_SHM,
  WAYLAND_GLOBAL_LAYER_SHELL,
  WAYLAND_GLOBAL_KEYBOARD_SHORTCUTS_INHIBITOR,
  WAYLAND_GLOBAL_CURSOR_SHAPE,
  _WAYLAND_GLOBAL_SIZE,
} wayland_global_name;

typedef struct {
  uint32_t button;
  char modifiers;
  gint x, y;
  gboolean pressed;
  guint32 time;
} widget_button_event;

typedef struct {
  gint x, y;
  guint32 time;
} widget_motion_event;

typedef struct _wayland_seat wayland_seat;

typedef struct {
  void *offer;
} clipboard_data;

typedef struct {
  GMainLoop *main_loop;
  GWaterWaylandSource *main_loop_source;
  struct wl_display *display;
  struct wl_registry *registry;
  uint32_t global_names[_WAYLAND_GLOBAL_SIZE];
  struct wl_compositor *compositor;

#ifdef HAVE_WAYLAND_CURSOR_SHAPE
  struct wp_cursor_shape_manager_v1 *cursor_shape_manager;
#endif

  struct wl_data_device_manager *data_device_manager;
  struct zwp_primary_selection_device_manager_v1
      *primary_selection_device_manager;

  struct zwlr_layer_shell_v1 *layer_shell;

  struct zwp_keyboard_shortcuts_inhibit_manager_v1 *kb_shortcuts_inhibit_manager;

  struct wl_shm *shm;
  size_t buffer_count;
  struct {
    char *theme_name;
    RofiCursorType type;
    struct wl_cursor_theme *theme;
    struct wl_cursor *cursor;
    struct wl_cursor_image *image;
    struct wl_surface *surface;
    struct wl_callback *frame_cb;
    guint scale;
  } cursor;
  GHashTable *seats;
  GHashTable *seats_by_name;
  wayland_seat *last_seat;
  GHashTable *outputs;
  struct wl_surface *surface;
  struct zwlr_layer_surface_v1 *wlr_surface;
  struct wl_callback *frame_cb;
  size_t scales[3];
  int32_t scale;
  NkBindingsSeat *bindings_seat;

  clipboard_data clipboards[2];

  uint32_t layer_width;
  uint32_t layer_height;

  struct zwp_text_input_manager_v3 *text_input_manager;
} wayland_stuff;

struct _wayland_seat {
  wayland_stuff *context;
  uint32_t global_name;
  struct wl_seat *seat;
  gchar *name;
  struct {
    xkb_keycode_t key;
    GSource *source;
    int32_t rate;
    int32_t delay;
  } repeat;
  uint32_t serial;
  uint32_t pointer_serial;
  struct wl_keyboard *keyboard;
  struct wl_pointer *pointer;

#ifdef HAVE_WAYLAND_CURSOR_SHAPE
  struct wp_cursor_shape_device_v1 *cursor_shape_device;
#endif
  struct wl_data_device *data_device;
  struct zwp_primary_selection_device_v1 *primary_selection_device;

  enum wl_pointer_axis_source axis_source;
  widget_button_event button;
  widget_motion_event motion;
  struct {
    gint vertical;
    gint horizontal;
  } wheel;
  struct {
    double vertical;
    double horizontal;
  } wheel_continuous;

  struct zwp_text_input_v3 *text_input;
};

/* Supported interface versions */
#define WL_COMPOSITOR_INTERFACE_VERSION 3
#define WL_SHM_INTERFACE_VERSION 1
#define WL_SEAT_INTERFACE_MIN_VERSION 5
#define WL_SEAT_INTERFACE_MAX_VERSION 8
#define WL_OUTPUT_INTERFACE_MIN_VERSION 2
#define WL_OUTPUT_INTERFACE_MAX_VERSION 4
#define WL_LAYER_SHELL_INTERFACE_VERSION 1
#define WL_KEYBOARD_SHORTCUTS_INHIBITOR_INTERFACE_VERSION 1

extern wayland_stuff *wayland;

#endif
