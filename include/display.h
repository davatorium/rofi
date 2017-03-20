#ifndef ROFI_DISPLAY_H
#define ROFI_DISPLAY_H

#include <glib.h>
#include <cairo.h>
#include <xkbcommon/xkbcommon.h>
#include "widgets/widget.h"

typedef struct _display_buffer_pool display_buffer_pool;

gboolean display_init(GMainLoop *main_loop, const gchar *display);
void display_cleanup(void);

display_buffer_pool *display_buffer_pool_new(gint width, gint height);
void display_buffer_pool_free(display_buffer_pool *pool);

cairo_surface_t *display_buffer_pool_get_next_buffer(display_buffer_pool *pool);
void display_surface_commit(cairo_surface_t *surface);

#endif
