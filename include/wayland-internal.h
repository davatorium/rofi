#ifndef ROFI_WAYLAND_INTERNAL_H
#define ROFI_WAYLAND_INTERNAL_H

#include <glib.h>
#include <cairo.h>
#include <xkbcommon/xkbcommon.h>
#include <xcb/xkb.h>
#include <wayland-client.h>
#include <libgwater-wayland.h>
#include <nkutils-bindings.h>

typedef enum {
    WAYLAND_GLOBAL_COMPOSITOR,
    WAYLAND_GLOBAL_SHM,
    WAYLAND_GLOBAL_LAYER_SHELL,
    _WAYLAND_GLOBAL_SIZE,
} wayland_global_name;

#if 0
typedef enum {
    WIDGET_MODMASK_SHIFT   = (1 << WIDGET_MOD_SHIFT),
    WIDGET_MODMASK_CONTROL = (1 << WIDGET_MOD_CONTROL),
    WIDGET_MODMASK_ALT     = (1 << WIDGET_MOD_ALT),
    WIDGET_MODMASK_META    = (1 << WIDGET_MOD_META),
    WIDGET_MODMASK_SUPER   = (1 << WIDGET_MOD_SUPER),
    WIDGET_MODMASK_HYPER   = (1 << WIDGET_MOD_HYPER),
    WIDGET_MODMASK_ALL     = (WIDGET_MODMASK_SHIFT | WIDGET_MODMASK_CONTROL | WIDGET_MODMASK_ALT | WIDGET_MODMASK_META | WIDGET_MODMASK_SUPER | WIDGET_MODMASK_HYPER)
} widget_modifier_mask;
#endif

typedef struct {
    NkBindingsMouseButton button;
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
    GMainLoop       *main_loop;
    GWaterWaylandSource *main_loop_source;
    struct wl_display *display;
    struct wl_registry *registry;
    uint32_t global_names[_WAYLAND_GLOBAL_SIZE];
    struct wl_compositor *compositor;
    struct zww_launcher_menu_v1 *launcher_menu;
    struct zww_window_switcher_v1 *window_switcher;

    struct zwlr_layer_shell_v1 *layer_shell;

    struct wl_shm *shm;
    size_t buffer_count;
    struct {
        char *theme_name;
        char **name;
        struct wl_cursor_theme *theme;
        struct wl_cursor *cursor;
        struct wl_cursor_image *image;
        struct wl_surface *surface;
        struct wl_callback *frame_cb;
    } cursor;
    GHashTable *seats;
    GHashTable *seats_by_name;
    wayland_seat *last_seat;
    GHashTable *outputs;
    struct wl_surface *surface;
    struct zwlr_layer_surface_v1* wlr_surface;
    struct wl_callback *frame_cb;
    size_t scales[3];
    int32_t scale;
    NkBindingsSeat *bindings_seat;

    uint32_t layer_width;
    uint32_t layer_height;
} wayland_stuff;

struct _wayland_seat {
    wayland_stuff *context;
    uint32_t global_name;
    struct wl_seat *seat;
    gchar *name;
    uint32_t serial;
    struct wl_keyboard *keyboard;
    // xkb_stuff xkb;
    struct wl_pointer *pointer;
    widget_button_event button;
    widget_motion_event motion;
    struct {
        gint vertical;
        gint horizontal;
    } wheel;
};

/* Supported interface versions */
#define WL_COMPOSITOR_INTERFACE_VERSION 3
#define WL_SHM_INTERFACE_VERSION 1
#define WL_SEAT_INTERFACE_VERSION 5
#define WL_OUTPUT_INTERFACE_VERSION 2
#define WL_LAYER_SHELL_INTERFACE_VERSION 1

extern wayland_stuff *wayland;

#endif
