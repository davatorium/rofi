/**
 *   MIT/X11 License
 *   Modified  (c) 2017 Quentin “Sardem FF7” Glidic
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

#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <locale.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <cairo.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <libgwater-wayland.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>

#include "view.h"

#include "unstable/launcher-menu/launcher-menu-unstable-v1-client-protocol.h"
#include "wayland.h"

/* Supported interface versions */
#define WL_COMPOSITOR_INTERFACE_VERSION 3
#define WW_LAUNCHER_MENU_INTERFACE_VERSION 1
#define WL_SHM_INTERFACE_VERSION 1
#define WL_SEAT_INTERFACE_VERSION 5
#define WL_OUTPUT_INTERFACE_VERSION 2

typedef enum {
    WAYLAND_GLOBAL_COMPOSITOR,
    WAYLAND_GLOBAL_LAUNCHER_MENU,
    WAYLAND_GLOBAL_SHM,
    _WAYLAND_GLOBAL_SIZE,
} wayland_global_name;


typedef struct {
    GMainLoop       *main_loop;
    GWaterWaylandSource *main_loop_source;
    struct wl_display *display;
    struct wl_registry *registry;
    uint32_t global_names[_WAYLAND_GLOBAL_SIZE];
    struct wl_compositor *compositor;
    struct zww_launcher_menu_v1 *launcher_menu;
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
    GHashTable *outputs;
    struct wl_surface *surface;
    struct wl_callback *frame_cb;
    size_t scales[3];
    int32_t scale;
} wayland_stuff;

struct _wayland_seat {
    wayland_stuff *context;
    uint32_t global_name;
    struct wl_seat *seat;
    struct wl_keyboard *keyboard;
    struct {
        /** Keyboard context */
        struct xkb_context *context;
        /** Current keymap */
        struct xkb_keymap  *keymap;
        /** Keyboard state */
        struct xkb_state   *state;
        /** Modifiers indexes */
        xkb_mod_index_t mods[X11MOD_ANY][8];
        /** Compose information */
        struct
        {
            /** Compose table */
            struct xkb_compose_table *table;
            /** Compose state */
            struct xkb_compose_state *state;
        } compose;
    } xkb;
    struct wl_pointer *pointer;
};

typedef struct {
    wayland_stuff *context;
    uint32_t global_name;
    struct wl_output *output;
    int32_t scale;
} wayland_output;

typedef struct {
    wayland_buffer_pool *pool;
    struct wl_buffer *buffer;
    uint8_t *data;
    gboolean released;
} wayland_buffer;

struct _wayland_buffer_pool {
    wayland_stuff *context;
    uint8_t *data;
    size_t size;
    int32_t width;
    int32_t height;
    gboolean to_free;
    wayland_buffer *buffers;
};

static wayland_stuff wayland_;
static wayland_stuff *wayland = &wayland_;
static const cairo_user_data_key_t wayland_cairo_surface_user_data;

static void
wayland_buffer_cleanup(wayland_buffer_pool *self)
{
    if ( ! self->to_free )
        return;

    size_t i, count = 0;
    for ( i = 0 ; i < wayland->buffer_count ; ++i )
    {
        if ( ( self->buffers[i].released ) && ( self->buffers[i].buffer != NULL ) )
        {
            wl_buffer_destroy(self->buffers[i].buffer);
            self->buffers[i].buffer = NULL;
        }
        if ( self->buffers[i].buffer == NULL )
            ++count;
    }

    if ( count < wayland->buffer_count )
        return;

    munmap(self->data, self->size);
    g_free(self);
}

static void
wayland_buffer_release(void *data, struct wl_buffer *buffer)
{
    wayland_buffer_pool *self = data;

    size_t i;
    for ( i = 0 ; i < wayland->buffer_count ; ++i )
    {
        if ( self->buffers[i].buffer == buffer )
            self->buffers[i].released = TRUE;
    }

    wayland_buffer_cleanup(self);
}

static const struct wl_buffer_listener wayland_buffer_listener = {
    wayland_buffer_release
};

wayland_buffer_pool *
wayland_buffer_pool_new(gint width, gint height)
{
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
    size = stride * height;
    pool_size = size * wayland->buffer_count;

    gchar filename[PATH_MAX];
    g_snprintf(filename, PATH_MAX, "%s/rofi-wayland-surface", g_get_user_runtime_dir());
    fd = g_open(filename, O_CREAT | O_RDWR | O_CLOEXEC, 0);
    g_unlink(filename);
    if ( fd < 0 )
    {
        g_warning("creating a buffer file for %zu B failed: %s", pool_size, g_strerror(errno));
        return NULL;
    }
    if ( ftruncate(fd, pool_size) < 0 )
    {
        g_close(fd, NULL);
        return NULL;
    }

    data = mmap(NULL, pool_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if ( data == MAP_FAILED )
    {
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
    for ( i = 0 ; i < wayland->buffer_count ; ++i )
    {
        pool->buffers[i].pool = pool;
        pool->buffers[i].buffer = wl_shm_pool_create_buffer(wl_pool, size * i, width, height, stride, WL_SHM_FORMAT_ARGB8888);
        pool->buffers[i].data = data + size * i;
        pool->buffers[i].released = TRUE;
        wl_buffer_add_listener(pool->buffers[i].buffer, &wayland_buffer_listener, pool);
    }
    wl_shm_pool_destroy(wl_pool);
    close(fd);

    return pool;
}

void
wayland_buffer_pool_free(wayland_buffer_pool *self)
{
    self->to_free = TRUE;
    wayland_buffer_cleanup(self);
}

static void
wayland_surface_protocol_enter(void *data, struct wl_surface *wl_surface, struct wl_output *wl_output)
{
    wayland_output *output;

    output = g_hash_table_lookup(wayland->outputs, wl_output);
    if ( output == NULL )
        return;
    if ( ( output->scale < 1 ) || ( output->scale > 3 ) )
        return;

    ++wayland->scales[output->scale - 1];
    if ( wayland->scale < output->scale )
    {
        wayland->scale = output->scale;

        // FIXME: resize
    }
}

static void
wayland_surface_protocol_leave(void *data, struct wl_surface *wl_surface, struct wl_output *wl_output)
{
    wayland_output *output;

    output = g_hash_table_lookup(wayland->outputs, wl_output);
    if ( output == NULL )
        return;
    if ( ( output->scale < 1 ) || ( output->scale > 3 ) )
        return;

    if ( ( --wayland->scales[output->scale - 1] < 1 ) && ( wayland->scale == output->scale ) )
    {
        int32_t i;
        for ( i = 0 ; i < 3 ; ++i )
        {
            if ( wayland->scales[i] > 0 )
                wayland->scale = i + 1;
        }
        // FIXME: resize
    }

}

static const struct wl_surface_listener wayland_surface_interface = {
    .enter = wayland_surface_protocol_enter,
    .leave = wayland_surface_protocol_leave,
};

static void wayland_frame_callback(void *data, struct wl_callback *callback, uint32_t time);

static const struct wl_callback_listener wayland_frame_wl_callback_listener = {
    .done = wayland_frame_callback,
};

cairo_surface_t *
wayland_buffer_pool_get_next_buffer(wayland_buffer_pool *pool)
{
    wayland_buffer *buffer = NULL;
    size_t i;
    for ( i = 0 ; ( buffer == NULL ) && ( i < wayland->buffer_count ) ; ++i )
    {
        buffer = pool->buffers + i;
        if ( ! buffer->released )
            buffer = NULL;
    }
    if ( buffer == NULL )
        return NULL;

    cairo_surface_t *surface;

    surface = cairo_image_surface_create_for_data(buffer->data, CAIRO_FORMAT_ARGB32, pool->width, pool->height, cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, pool->width));
    cairo_surface_set_user_data(surface, &wayland_cairo_surface_user_data, buffer, NULL);
    return surface;
}

void
wayland_surface_commit(cairo_surface_t *surface)
{
    wayland_buffer *buffer = cairo_surface_get_user_data(surface, &wayland_cairo_surface_user_data);
    wayland_buffer_pool *pool = buffer->pool;

    cairo_surface_destroy(surface);

    wl_surface_damage(wayland->surface, 0, 0, pool->width, pool->height);
    wl_surface_attach(wayland->surface, buffer->buffer, 0, 0);
    if ( wl_surface_get_version(wayland->surface) >= WL_SURFACE_SET_BUFFER_SCALE_SINCE_VERSION )
        wl_surface_set_buffer_scale(wayland->surface, wayland->scale);
    buffer->released = FALSE;

    wl_surface_commit(wayland->surface);
}

static void
wayland_frame_callback(void *data, struct wl_callback *callback, uint32_t timestamp)
{
    if ( wayland->frame_cb != NULL )
        wl_callback_destroy(wayland->frame_cb);
    wayland->frame_cb = wl_surface_frame(wayland->surface);
    wl_callback_add_listener(wayland->frame_cb, &wayland_frame_wl_callback_listener, wayland);

    rofi_view_frame_callback();
}

static void
wayland_dismiss(void *data, struct zww_launcher_menu_v1 *launcher_menu)
{
    g_main_loop_quit ( wayland->main_loop );
}

static const struct zww_launcher_menu_v1_listener wayland_listener = {
    .dismiss = wayland_dismiss,
};

static void xkb_find_mod_mask ( wayland_seat *self, gsize mod, ... )
{
    va_list         names;
    const char      *name;
    xkb_mod_index_t i, *m = self->xkb.mods[mod];
    va_start ( names, mod );
    while ( ( name = va_arg ( names, const char * ) ) != NULL ) {
        i = xkb_keymap_mod_get_index ( self->xkb.keymap, name );
        if ( i != XKB_MOD_INVALID ) {
            *m++ = i;
        }
    }
    *m = G_MAXUINT32;
    va_end ( names );
}

static void xkb_figure_out_masks ( wayland_seat *self )
{
    xkb_find_mod_mask ( self, X11MOD_SHIFT, XKB_MOD_NAME_SHIFT, NULL );
    xkb_find_mod_mask ( self, X11MOD_CONTROL, XKB_MOD_NAME_CTRL, NULL );
    xkb_find_mod_mask ( self, X11MOD_ALT, XKB_MOD_NAME_ALT, "Alt", "LAlt", "RAlt", "AltGr", "Mod5", "LevelThree", NULL );
    xkb_find_mod_mask ( self, X11MOD_META, "Meta", NULL );
    xkb_find_mod_mask ( self, X11MOD_SUPER, XKB_MOD_NAME_LOGO, "Super", NULL );
    xkb_find_mod_mask ( self, X11MOD_HYPER, "Hyper", NULL );
}

gboolean
xkb_check_mod_match(wayland_seat *self, unsigned int mask, xkb_keysym_t key)
{
    guint i;
    xkb_mod_index_t *m;
    for ( i = 0; i < X11MOD_ANY; ++i ) {
        if ( ( mask & ( 1 << i ) ) == 0 )
            continue;

        for ( m = self->xkb.mods[i] ; *m != G_MAXUINT32 ; ++m ) {
            if ( ! xkb_state_mod_index_is_active(self->xkb.state, *m, XKB_STATE_MODS_EFFECTIVE) || xkb_state_mod_index_is_consumed(self->xkb.state, key, *m) )
                return FALSE;
        }
    }
    return TRUE;
}

static void
wayland_keyboard_keymap(void *data, struct wl_keyboard *keyboard, enum wl_keyboard_keymap_format format, int32_t fd, uint32_t size)
{
    wayland_seat *self = data;

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd);
        return;
    }

    char *str = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (str == MAP_FAILED) {
        close(fd);
        return;
    }

    self->xkb.keymap = xkb_keymap_new_from_string ( self->xkb.context, str, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS );
    if ( self->xkb.keymap == NULL ) {
        fprintf ( stderr, "Failed to get Keymap for current keyboard device.\n" );
        return;
    }
    self->xkb.state = xkb_state_new ( self->xkb.keymap );
    if ( self->xkb.state == NULL ) {
        fprintf ( stderr, "Failed to get state object for current keyboard device.\n" );
        return;
    }

    self->xkb.compose.table = xkb_compose_table_new_from_locale ( self->xkb.context, setlocale ( LC_CTYPE, NULL ), 0 );
    if ( self->xkb.compose.table != NULL ) {
        self->xkb.compose.state = xkb_compose_state_new ( self->xkb.compose.table, 0 );
    }
    xkb_figure_out_masks(self);
}

static void
wayland_keyboard_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys)
{
    wayland_seat *self = data;
}

static void
wayland_keyboard_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface)
{
    wayland_seat *self = data;
}

static void
wayland_keyboard_key(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, enum wl_keyboard_key_state state)
{
    wayland_seat *self = data;
    xkb_keysym_t keysym;
    char         pad[32];
    int          len = 0;

    key += 8;

    if ( state == WL_KEYBOARD_KEY_STATE_RELEASED ) {
        // FIXME: check mods
        // abe_trigger_release ();
        return;
    }

    keysym = xkb_state_key_get_one_sym ( self->xkb.state, key );

    if ( self->xkb.compose.state != NULL ) {
        if ( ( keysym != XKB_KEY_NoSymbol ) && ( xkb_compose_state_feed ( self->xkb.compose.state, keysym ) == XKB_COMPOSE_FEED_ACCEPTED ) ) {
            switch ( xkb_compose_state_get_status ( self->xkb.compose.state ) )
            {
            case XKB_COMPOSE_CANCELLED:
            /* Eat the keysym that cancelled the compose sequence.
             * This is default behaviour with Xlib */
            case XKB_COMPOSE_COMPOSING:
                keysym = XKB_KEY_NoSymbol;
                break;
            case XKB_COMPOSE_COMPOSED:
                keysym = xkb_compose_state_get_one_sym ( self->xkb.compose.state );
                len = xkb_compose_state_get_utf8 ( self->xkb.compose.state, pad, sizeof ( pad ) );
                break;
            case XKB_COMPOSE_NOTHING:
                break;
            }
            if ( ( keysym == XKB_KEY_NoSymbol ) && ( len == 0 ) ) {
                return;
            }
        }
    }

    if ( len == 0 ) {
        len = xkb_state_key_get_utf8 ( self->xkb.state, key, pad, sizeof ( pad ) );
    }

    rofi_view_handle_keypress ( self, keysym, pad, len );
    rofi_view_maybe_update();
}

static void
wayland_keyboard_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
    wayland_seat *self = data;

    xkb_state_update_mask ( self->xkb.state, mods_depressed, mods_latched, mods_locked, 0, 0, group );
}

static void
wayland_keyboard_repeat_info(void *data, struct wl_keyboard *keyboard, int32_t rate, int32_t delay)
{
    wayland_seat *self = data;
}

static const struct wl_keyboard_listener wayland_keyboard_listener = {
    .keymap = wayland_keyboard_keymap,
    .enter = wayland_keyboard_enter,
    .leave = wayland_keyboard_leave,
    .key = wayland_keyboard_key,
    .modifiers = wayland_keyboard_modifiers,
    .repeat_info = wayland_keyboard_repeat_info,
};


static void
wayland_cursor_set_image(int i)
{
    struct wl_buffer *buffer;
    struct wl_cursor_image *image;
    image = wayland->cursor.cursor->images[i];

    wayland->cursor.image = image;
    buffer = wl_cursor_image_get_buffer(wayland->cursor.image);
    wl_surface_attach(wayland->cursor.surface, buffer, 0, 0);
    wl_surface_damage(wayland->cursor.surface, 0, 0, wayland->cursor.image->width, wayland->cursor.image->height);
    wl_surface_commit(wayland->cursor.surface);
}

static void wayland_cursor_frame_callback(void *data, struct wl_callback *callback, uint32_t time);

static const struct wl_callback_listener wayland_cursor_frame_wl_callback_listener = {
    .done = wayland_cursor_frame_callback,
};

static void
wayland_cursor_frame_callback(void *data, struct wl_callback *callback, uint32_t time)
{
    int i;

    if ( wayland->cursor.frame_cb != NULL )
        wl_callback_destroy(wayland->cursor.frame_cb);
    wayland->cursor.frame_cb = wl_surface_frame(wayland->cursor.surface);
    wl_callback_add_listener(wayland->cursor.frame_cb, &wayland_cursor_frame_wl_callback_listener, wayland);

    i = wl_cursor_frame(wayland->cursor.cursor, time);
    wayland_cursor_set_image(i);
}

static void
wayland_pointer_trigger_wheel(wayland_seat *self)
{
    //rofi_view_mouse_navigation();
}

static void
wayland_pointer_enter(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y)
{
    wayland_seat *self = data;

    if ( wayland->cursor.surface == NULL )
        return;

    if ( wayland->cursor.cursor->image_count < 2 )
        wayland_cursor_set_image(0);
    else
        wayland_cursor_frame_callback(wayland, wayland->cursor.frame_cb, 0);

    wl_pointer_set_cursor(self->pointer, serial, wayland->cursor.surface, wayland->cursor.image->hotspot_x, wayland->cursor.image->hotspot_y);
}

static void
wayland_pointer_leave(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface)
{
    wayland_seat *self = data;

    if ( wayland->cursor.frame_cb != NULL )
        wl_callback_destroy(wayland->cursor.frame_cb);
}

static void
wayland_pointer_motion(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
}

static void
wayland_pointer_button(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, enum wl_pointer_button_state state)
{
    wayland_seat *self = data;

    if ( wl_pointer_get_version(self->pointer) >= WL_POINTER_FRAME_SINCE_VERSION )
        return;

    rofi_view_maybe_update();
}

static void
wayland_pointer_axis(void *data, struct wl_pointer *pointer, uint32_t time, enum wl_pointer_axis axis, wl_fixed_t value)
{
    wayland_seat *self = data;

    if ( wl_pointer_get_version(self->pointer) >= WL_POINTER_FRAME_SINCE_VERSION )
        return;

    wayland_pointer_trigger_wheel(self);
}

static void
wayland_pointer_frame(void *data, struct wl_pointer *pointer)
{
    wayland_seat *self = data;
    wayland_pointer_trigger_wheel(self);
    rofi_view_maybe_update();
}

static void
wayland_pointer_axis_source(void *data, struct wl_pointer *pointer, enum wl_pointer_axis_source axis_source)
{
}

static void
wayland_pointer_axis_stop(void *data, struct wl_pointer *pointer, uint32_t time, enum wl_pointer_axis axis)
{
}

static void
wayland_pointer_axis_discrete(void *data, struct wl_pointer *pointer, enum wl_pointer_axis axis, int32_t discrete)
{
    wayland_seat *self = data;
}

static const struct wl_pointer_listener wayland_pointer_listener = {
    .enter = wayland_pointer_enter,
    .leave = wayland_pointer_leave,
    .motion = wayland_pointer_motion,
    .button = wayland_pointer_button,
    .axis = wayland_pointer_axis,
    .frame = wayland_pointer_frame,
    .axis_source = wayland_pointer_axis_source,
    .axis_stop = wayland_pointer_axis_stop,
    .axis_discrete = wayland_pointer_axis_discrete,
};

static void
wayland_keyboard_release(wayland_seat *self)
{
    if ( self->keyboard == NULL )
        return;

    if ( self->xkb.compose.state != NULL ) {
        xkb_compose_state_unref ( self->xkb.compose.state );
        self->xkb.compose.state = NULL;
    }
    if ( self->xkb.compose.table != NULL ) {
        xkb_compose_table_unref ( self->xkb.compose.table );
        self->xkb.compose.table = NULL;
    }
    if ( self->xkb.state != NULL ) {
        xkb_state_unref ( self->xkb.state );
        self->xkb.state = NULL;
    }
    if ( self->xkb.keymap != NULL ) {
        xkb_keymap_unref ( self->xkb.keymap );
        self->xkb.keymap = NULL;
    }
    if ( self->xkb.context != NULL ) {
        xkb_context_unref ( self->xkb.context );
        self->xkb.context = NULL;
    }

    if ( wl_keyboard_get_version(self->keyboard) >= WL_KEYBOARD_RELEASE_SINCE_VERSION )
        wl_keyboard_release(self->keyboard);
    else
        wl_keyboard_destroy(self->keyboard);

    self->keyboard = NULL;
}


static void
wayland_pointer_release(wayland_seat *self)
{
    if ( self->pointer == NULL )
        return;

    if ( wl_pointer_get_version(self->pointer) >= WL_POINTER_RELEASE_SINCE_VERSION )
        wl_pointer_release(self->pointer);
    else
        wl_pointer_destroy(self->pointer);

    self->pointer = NULL;
}

static void
wayland_seat_release(wayland_seat *self)
{
    wayland_keyboard_release(self);
    wayland_pointer_release(self);

    if ( wl_seat_get_version(self->seat) >= WL_SEAT_RELEASE_SINCE_VERSION )
        wl_seat_release(self->seat);
    else
        wl_seat_destroy(self->seat);

    g_hash_table_remove(wayland->seats, self->seat);

    g_free(self);
}

static void
wayland_seat_capabilities(void *data, struct wl_seat *seat, uint32_t capabilities)
{
    wayland_seat *self = data;

    if ( ( capabilities & WL_SEAT_CAPABILITY_KEYBOARD ) && ( self->keyboard == NULL ) )
    {
        self->keyboard = wl_seat_get_keyboard(self->seat);
        wl_keyboard_add_listener(self->keyboard, &wayland_keyboard_listener, self);
        self->xkb.context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    }
    else if ( ( ! ( capabilities & WL_SEAT_CAPABILITY_POINTER ) ) && ( self->keyboard != NULL ) )
        wayland_keyboard_release(self);

    if ( ( capabilities & WL_SEAT_CAPABILITY_POINTER ) && ( self->pointer == NULL ) )
    {
        self->pointer = wl_seat_get_pointer(self->seat);
        wl_pointer_add_listener(self->pointer, &wayland_pointer_listener, self);
    }
    else if ( ( ! ( capabilities & WL_SEAT_CAPABILITY_POINTER ) ) && ( self->pointer != NULL ) )
        wayland_pointer_release(self);
}

static void
wayland_seat_name(void *data, struct wl_seat *seat, const char *name)
{
}

static const struct wl_seat_listener wayland_seat_listener = {
    .capabilities = wayland_seat_capabilities,
    .name = wayland_seat_name,
};

static void
wayland_output_release(wayland_output *self)
{
    if ( wl_output_get_version(self->output) >= WL_OUTPUT_RELEASE_SINCE_VERSION )
        wl_output_release(self->output);
    else
        wl_output_destroy(self->output);

    g_hash_table_remove(wayland->outputs, self->output);

    g_free(self);
}

static void
wayland_output_done(void *data, struct wl_output *output)
{
}

static void
wayland_output_geometry(void *data, struct wl_output *output, int32_t x, int32_t y, int32_t width, int32_t height, int32_t subpixel, const char *make, const char *model, int32_t transform)
{
}

static void
wayland_output_mode(void *data, struct wl_output *output, enum wl_output_mode flags, int32_t width, int32_t height, int32_t refresh)
{
}

static void
wayland_output_scale(void *data, struct wl_output *output, int32_t scale)
{
    wayland_output *self = data;

    self->scale = scale;
}

static const struct wl_output_listener wayland_output_listener = {
    .geometry = wayland_output_geometry,
    .mode = wayland_output_mode,
    .scale = wayland_output_scale,
    .done = wayland_output_done,
};

static const char * const wayland_cursor_names[] = {
    "left_ptr",
    "default",
    "top_left_arrow",
    "left-arrow",
    NULL
};

static void
wayland_registry_handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
    if ( g_strcmp0(interface, "wl_compositor") == 0 )
    {
        wayland->global_names[WAYLAND_GLOBAL_COMPOSITOR] = name;
        wayland->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, MIN(version, WL_COMPOSITOR_INTERFACE_VERSION));
    }
    else if ( g_strcmp0(interface, "zww_launcher_menu_v1") == 0 )
    {
        wayland->global_names[WAYLAND_GLOBAL_LAUNCHER_MENU] = name;
        wayland->launcher_menu = wl_registry_bind(registry, name, &zww_launcher_menu_v1_interface, WW_LAUNCHER_MENU_INTERFACE_VERSION);
        zww_launcher_menu_v1_add_listener(wayland->launcher_menu, &wayland_listener, wayland);
    }
    else if ( g_strcmp0(interface, "wl_shm") == 0 )
    {
        wayland->global_names[WAYLAND_GLOBAL_SHM] = name;
        wayland->shm = wl_registry_bind(registry, name, &wl_shm_interface, MIN(version, WL_SHM_INTERFACE_VERSION));
    }
    else if ( g_strcmp0(interface, "wl_seat") == 0 )
    {
        wayland_seat *seat = g_new0(wayland_seat, 1);
        seat->context = wayland;
        seat->global_name = name;
        seat->seat = wl_registry_bind(registry, name, &wl_seat_interface, MIN(version, WL_SEAT_INTERFACE_VERSION));

        g_hash_table_insert(wayland->seats, seat->seat, seat);

        wl_seat_add_listener(seat->seat, &wayland_seat_listener, seat);
    }
    else if ( g_strcmp0(interface, "wl_output") == 0 )
    {
        wayland_output *output = g_new0(wayland_output, 1);
        output->context = wayland;
        output->global_name = name;
        output->output = wl_registry_bind(registry, name, &wl_output_interface, MIN(version, WL_OUTPUT_INTERFACE_VERSION));
        output->scale = 1;

        g_hash_table_insert(wayland->outputs, output->output, output);

        wl_output_add_listener(output->output, &wayland_output_listener, output);
    }

    if ( ( wayland->cursor.theme == NULL ) && ( wayland->compositor != NULL ) && ( wayland->shm != NULL ) )
    {
        wayland->cursor.theme = wl_cursor_theme_load(wayland->cursor.theme_name, 32, wayland->shm);
        if ( wayland->cursor.theme != NULL )
        {
            const char * const *cname = (const char * const *) wayland->cursor.name;
            for ( cname = ( cname != NULL ) ? cname : wayland_cursor_names ; ( wayland->cursor.cursor == NULL ) && ( *cname != NULL ) ; ++cname )
                wayland->cursor.cursor = wl_cursor_theme_get_cursor(wayland->cursor.theme, *cname);
            if ( wayland->cursor.cursor == NULL )
            {
                wl_cursor_theme_destroy(wayland->cursor.theme);
                wayland->cursor.theme = NULL;
            }
            else
                wayland->cursor.surface = wl_compositor_create_surface(wayland->compositor);
        }
    }
}

static void
wayland_registry_handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
    wayland_global_name i;
    for ( i = 0 ; i < _WAYLAND_GLOBAL_SIZE ; ++i )
    {
        if ( wayland->global_names[i] != name )
            continue;
        wayland->global_names[i] = 0;

        switch ( i )
        {
        case WAYLAND_GLOBAL_COMPOSITOR:
            wl_compositor_destroy(wayland->compositor);
            wayland->compositor = NULL;
        break;
        case WAYLAND_GLOBAL_LAUNCHER_MENU:
            zww_launcher_menu_v1_destroy(wayland->launcher_menu);
            wayland->launcher_menu = NULL;
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
    if ( ( wayland->cursor.theme != NULL ) && ( ( wayland->compositor == NULL ) || ( wayland->shm == NULL ) ) )
    {
        if ( wayland->cursor.frame_cb != NULL )
            wl_callback_destroy(wayland->cursor.frame_cb);
        wayland->cursor.frame_cb = NULL;

        wl_surface_destroy(wayland->cursor.surface);
        wl_cursor_theme_destroy(wayland->cursor.theme);
        wayland->cursor.surface = NULL;
        wayland->cursor.image = NULL;
        wayland->cursor.cursor = NULL;
        wayland->cursor.theme = NULL;
    }

    GHashTableIter iter;

    wayland_seat *seat;
    g_hash_table_iter_init(&iter, wayland->seats);
    while ( g_hash_table_iter_next(&iter, NULL, (gpointer *) &seat) )
    {
        if ( seat->global_name != name )
            continue;

        g_hash_table_iter_remove(&iter);
        wayland_seat_release(seat);
        return;
    }

    wayland_output *output;
    g_hash_table_iter_init(&iter, wayland->outputs);
    while ( g_hash_table_iter_next(&iter, NULL, (gpointer *) &output) )
    {
        if ( output->global_name != name )
            continue;

        g_hash_table_iter_remove(&iter);
        wayland_output_release(output);
        return;
    }
}

static const struct wl_registry_listener wayland_registry_listener = {
    .global = wayland_registry_handle_global,
    .global_remove = wayland_registry_handle_global_remove,
};

gboolean
wayland_init(GMainLoop *main_loop, const gchar *display)
{
    wayland->main_loop = main_loop;

    wayland->main_loop_source = g_water_wayland_source_new ( NULL, display );
    if ( wayland->main_loop_source == NULL )
        return FALSE;

    wayland->buffer_count = 3;
    wayland->scale = 1;

    wayland->outputs = g_hash_table_new(g_direct_hash, g_direct_equal);
    wayland->seats = g_hash_table_new(g_direct_hash, g_direct_equal);

    wayland->display = g_water_wayland_source_get_display ( wayland->main_loop_source );
    wayland->registry = wl_display_get_registry ( wayland->display );
    wl_registry_add_listener ( wayland->registry, &wayland_registry_listener, NULL );
    wl_display_roundtrip ( wayland->display );

    if ( wayland->launcher_menu == NULL )
        return FALSE;

    wayland->surface = wl_compositor_create_surface(wayland->compositor);
    zww_launcher_menu_v1_show(wayland->launcher_menu, wayland->surface);
    wl_surface_add_listener(wayland->surface, &wayland_surface_interface, wayland);
    wayland_frame_callback(wayland, wayland->frame_cb, 0);

    return TRUE;
}

void
wayland_cleanup(void)
{
    wl_surface_destroy(wayland->surface);
    g_hash_table_unref( wayland->seats);
    g_hash_table_unref( wayland->outputs);
    wl_registry_destroy ( wayland->registry );
    g_water_wayland_source_free ( wayland->main_loop_source );
}
