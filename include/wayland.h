#ifndef ROFI_WAYLAND_H
#define ROFI_WAYLAND_H

#include <glib.h>
#include <cairo.h>

typedef struct _wayland_buffer_pool wayland_buffer_pool;

gboolean wayland_init(GMainLoop *main_loop, const gchar *display);
void wayland_cleanup(void);

wayland_buffer_pool *wayland_buffer_pool_new(gint width, gint height);
void wayland_buffer_pool_free(wayland_buffer_pool *pool);

cairo_surface_t *wayland_buffer_pool_get_next_buffer(wayland_buffer_pool *pool);
void wayland_surface_commit(cairo_surface_t *surface);

#endif
