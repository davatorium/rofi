#ifndef ROFI_WAYLAND_H
#define ROFI_WAYLAND_H

#include <glib.h>
#include <cairo.h>
#include <xkbcommon/xkbcommon.h>

enum
{
    /** Shift key */
    X11MOD_SHIFT,
    /** Control Key */
    X11MOD_CONTROL,
    /** Alt key */
    X11MOD_ALT,
    /** Meta key */
    X11MOD_META,
    /** Super (window) key */
    X11MOD_SUPER,
    /** Hyper key */
    X11MOD_HYPER,
    /** Any modifier */
    X11MOD_ANY,
    /** Number of modifier keys */
    NUM_X11MOD
};

typedef struct _wayland_seat wayland_seat;
typedef struct _wayland_buffer_pool wayland_buffer_pool;

gboolean wayland_init(GMainLoop *main_loop, const gchar *display);
void wayland_cleanup(void);

wayland_buffer_pool *wayland_buffer_pool_new(gint width, gint height);
void wayland_buffer_pool_free(wayland_buffer_pool *pool);

cairo_surface_t *wayland_buffer_pool_get_next_buffer(wayland_buffer_pool *pool);
void wayland_surface_commit(cairo_surface_t *surface);

gboolean xkb_check_mod_match(wayland_seat *seat, unsigned int mask, xkb_keysym_t key);

#endif
