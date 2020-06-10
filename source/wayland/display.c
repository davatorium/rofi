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

#define G_LOG_DOMAIN "Wayland"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include <config.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <locale.h>
#include <linux/input-event-codes.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <cairo.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <xcb/xkb.h>
#include <libgwater-wayland.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>

#include <nkutils-bindings.h>

#include "keyb.h"
#include "view.h"

#include "wayland-internal.h"
#include "display.h"
#include "display-internal.h"

#include "wlr-layer-shell-unstable-v1-protocol.h"

typedef struct _display_buffer_pool wayland_buffer_pool;
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

struct _display_buffer_pool {
    wayland_stuff *context;
    uint8_t *data;
    size_t size;
    int32_t width;
    int32_t height;
    gboolean to_free;
    wayland_buffer *buffers;
};

static wayland_stuff wayland_;
wayland_stuff *wayland = &wayland_;
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
display_buffer_pool_new(gint width, gint height)
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
    //fd = g_open(filename, O_CREAT | O_RDWR | O_CLOEXEC, 0);
    fd = g_open(filename, O_CREAT | O_RDWR, 0);
    g_unlink(filename);
    if ( fd < 0 )
    {
        g_warning("creating a buffer file for %zu B failed: %s", pool_size, g_strerror(errno));
        return NULL;
    }
    if ( fcntl(fd, F_SETFD, FD_CLOEXEC) < 0 )
    {
        g_close(fd, NULL);
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
display_buffer_pool_free(wayland_buffer_pool *self)
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

        rofi_view_set_size(rofi_view_get_active(), -1, -1);
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
        rofi_view_set_size(rofi_view_get_active(), -1, -1);
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
display_buffer_pool_get_next_buffer(wayland_buffer_pool *pool)
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
display_surface_commit(cairo_surface_t *surface)
{
    if ( surface == NULL )
        return;

    wayland_buffer *buffer = cairo_surface_get_user_data(surface, &wayland_cairo_surface_user_data);
    wayland_buffer_pool *pool = buffer->pool;

    cairo_surface_destroy(surface);

    wl_surface_damage(wayland->surface, 0, 0, pool->width, pool->height);
    wl_surface_attach(wayland->surface, buffer->buffer, 0, 0);
    if ( wl_surface_get_version(wayland->surface) >= WL_SURFACE_SET_BUFFER_SCALE_SINCE_VERSION )
        wl_surface_set_buffer_scale(wayland->surface, wayland->scale);
    buffer->released = FALSE;

    wl_surface_commit( wayland->surface );
}

static void
wayland_frame_callback(void *data, struct wl_callback *callback, uint32_t timestamp)
{
    if ( wayland->frame_cb != NULL )
    {
        wl_callback_destroy(wayland->frame_cb);
        rofi_view_frame_callback();
    }
    wayland->frame_cb = wl_surface_frame(wayland->surface);
    wl_callback_add_listener(wayland->frame_cb, &wayland_frame_wl_callback_listener, wayland);
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

    struct xkb_keymap *keymap = xkb_keymap_new_from_string ( nk_bindings_seat_get_context ( wayland->bindings_seat ), str, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS );
    if ( keymap == NULL ) {
        fprintf ( stderr, "Failed to get Keymap for current keyboard device.\n" );
        return;
    }
    struct xkb_state *state = xkb_state_new ( keymap );
    if ( state == NULL ) {
        fprintf ( stderr, "Failed to get state object for current keyboard device.\n" );
        return;
    }

    nk_bindings_seat_update_keymap ( wayland->bindings_seat, keymap, state );
}

static void
wayland_keyboard_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys)
{
    wayland_seat *self = data;

    wayland->last_seat = self;
    self->serial = serial;

    uint32_t *key, *kend;
    for ( key = keys->data, kend = key + keys->size / sizeof(*key) ; key < kend ; ++key ) {
        // TODO: check
        nk_bindings_seat_handle_key( wayland->bindings_seat, NULL, *key + 8, NK_BINDINGS_KEY_STATE_PRESSED );
    }
}

static void
wayland_keyboard_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface)
{
    wayland_seat *self = data;
    // TODO?
}

static gboolean
wayland_key_repeat(void *data)
{
    wayland_seat *self = data;

    if ( self->repeat.key == 0 ) {
        self->repeat.source = NULL;
        return G_SOURCE_REMOVE;
    }

    RofiViewState *state = rofi_view_get_active ( );
    char *text = nk_bindings_seat_handle_key( wayland->bindings_seat, NULL, self->repeat.key, NK_BINDINGS_KEY_STATE_PRESS );

    if ( text != NULL ) {
        rofi_view_handle_text ( state, text );
    }

    rofi_view_maybe_update( state );

    return G_SOURCE_CONTINUE;
}

static gboolean
wayland_key_repeat_delay(void *data)
{
    wayland_seat *self = data;

    if ( self->repeat.key == 0 ) {
        return FALSE;
    }

    RofiViewState *state = rofi_view_get_active ( );
    char *text = nk_bindings_seat_handle_key( wayland->bindings_seat, NULL, self->repeat.key, NK_BINDINGS_KEY_STATE_PRESS );

    if ( text != NULL ) {
        rofi_view_handle_text ( state, text );
    }

    guint source_id = g_timeout_add ( self->repeat.rate, wayland_key_repeat, data );
    self->repeat.source = g_main_context_find_source_by_id ( NULL, source_id);

    rofi_view_maybe_update( state );

    return G_SOURCE_REMOVE;
}

static void
wayland_keyboard_key(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, enum wl_keyboard_key_state kstate)
{
    RofiViewState *state = rofi_view_get_active ( );
    wayland_seat *self = data;

    gchar *text;

    wayland->last_seat = self;
    self->serial = serial;

    xkb_keycode_t keycode = key + 8;
    if ( kstate == WL_KEYBOARD_KEY_STATE_RELEASED ) {
        if ( keycode == self->repeat.key ) {
            self->repeat.key = 0;
            if ( self->repeat.source != NULL ) {
                g_source_destroy ( self->repeat.source );
                self->repeat.source = NULL;
            }
        }
        nk_bindings_seat_handle_key( wayland->bindings_seat, NULL, keycode, NK_BINDINGS_KEY_STATE_RELEASE );
    } else if ( kstate == WL_KEYBOARD_KEY_STATE_PRESSED ) {
        char *text = nk_bindings_seat_handle_key( wayland->bindings_seat, NULL, keycode, NK_BINDINGS_KEY_STATE_PRESS );

        if ( text != NULL ) {
            rofi_view_handle_text ( state, text );
        }
        if ( self->repeat.source != NULL ) {
            g_source_destroy ( self->repeat.source );
            self->repeat.source = NULL;
        }

        self->repeat.key = keycode;
        guint source_id = g_timeout_add ( self->repeat.delay, wayland_key_repeat_delay, data );
        self->repeat.source = g_main_context_find_source_by_id ( NULL, source_id);
    }

    rofi_view_maybe_update( state );
}

static void
wayland_keyboard_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
    RofiViewState *state = rofi_view_get_active ( );
    wayland_seat *self = data;

    nk_bindings_seat_update_mask( wayland->bindings_seat, NULL, mods_depressed, mods_latched, mods_locked, 0, 0, 0 );
    rofi_view_maybe_update( state );
}

static void
wayland_keyboard_repeat_info(void *data, struct wl_keyboard *keyboard, int32_t rate, int32_t delay)
{
    wayland_seat *self = data;
    self->repeat.key = 0;
    self->repeat.rate = rate;
    self->repeat.delay = delay;
    if ( self->repeat.source != NULL ) {
        g_source_destroy ( self->repeat.source );
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
wayland_pointer_send_events(wayland_seat *self)
{
    RofiViewState *state = rofi_view_get_active ();

    if ( self->motion.x > -1 || self->motion.y > -1 )
    {
        rofi_view_handle_mouse_motion(state, self->motion.x, self->motion.y);
        self->motion.x = -1;
        self->motion.y = -1;
    }

    NkBindingsMouseButton button = -1;
    switch ( self->button.button )
    {
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

    if ( self->button.button >= 0 )
    {
        if ( self->button.pressed ) {
            rofi_view_handle_mouse_motion ( state, self->button.x, self->button.y );
            nk_bindings_seat_handle_button ( wayland->bindings_seat, NULL, button, NK_BINDINGS_BUTTON_STATE_PRESS, self->button.time );
        } else {
            nk_bindings_seat_handle_button ( wayland->bindings_seat, NULL, button, NK_BINDINGS_BUTTON_STATE_RELEASE, self->button.time );
        }
        self->button.button = 0;
    }

    if ( self->wheel.vertical != 0 )
    {
        nk_bindings_seat_handle_scroll( wayland->bindings_seat, NULL, NK_BINDINGS_SCROLL_AXIS_VERTICAL, self->wheel.vertical );
        self->wheel.vertical = 0;
    }

    if ( self->wheel.horizontal != 0 )
    {
        nk_bindings_seat_handle_scroll( wayland->bindings_seat, NULL, NK_BINDINGS_SCROLL_AXIS_HORIZONTAL, self->wheel.horizontal );
        self->wheel.horizontal = 0;
    }

    rofi_view_maybe_update(state);
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
    wayland_seat *self = data;

    self->button.x = wl_fixed_to_int(x);
    self->button.y = wl_fixed_to_int(y);
    self->motion.x = wl_fixed_to_int(x);
    self->motion.y = wl_fixed_to_int(y);
    self->motion.time = time;

    if ( wl_pointer_get_version(self->pointer) >= WL_POINTER_FRAME_SINCE_VERSION )
        return;

    wayland_pointer_send_events(self);
}

static void
wayland_pointer_button(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, enum wl_pointer_button_state state)
{
    wayland_seat *self = data;

    wayland->last_seat = self;
    self->serial = serial;

    self->button.time = time;
    self->button.pressed = (state == WL_POINTER_BUTTON_STATE_PRESSED);
    self->button.button = button;

    wayland_pointer_send_events(self);
}

static void
wayland_pointer_axis(void *data, struct wl_pointer *pointer, uint32_t time, enum wl_pointer_axis axis, wl_fixed_t value)
{
    wayland_seat *self = data;

    switch ( axis )
    {
    case WL_POINTER_AXIS_VERTICAL_SCROLL:
        self->wheel.vertical += (gint)(wl_fixed_to_double(value) / 10.);
    break;
    case WL_POINTER_AXIS_HORIZONTAL_SCROLL:
        self->wheel.horizontal += (gint)(wl_fixed_to_double(value) / 10.);
    break;
    }

    wayland_pointer_send_events(self);
}

static void
wayland_pointer_frame(void *data, struct wl_pointer *pointer)
{
    wayland_seat *self = data;
    wayland_pointer_send_events(self);
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

    switch ( axis )
    {
    case WL_POINTER_AXIS_VERTICAL_SCROLL:
        self->wheel.vertical += discrete;
    break;
    case WL_POINTER_AXIS_HORIZONTAL_SCROLL:
        self->wheel.horizontal += discrete;
    break;
    }
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

    if ( wl_keyboard_get_version(self->keyboard) >= WL_KEYBOARD_RELEASE_SINCE_VERSION )
        wl_keyboard_release(self->keyboard);
    else
        wl_keyboard_destroy(self->keyboard);

    self->repeat.key = 0;
    if ( self->repeat.source != NULL ) {
        g_source_destroy ( self->repeat.source );
        self->repeat.source = NULL;
    }

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
    wayland_seat *self = data;

    if ( self->name != NULL )
        g_hash_table_remove(wayland->seats_by_name, self->name);
    self->name = g_strdup(name);
    g_hash_table_insert(wayland->seats_by_name, self->name, self);
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
    if ( g_strcmp0(interface, wl_compositor_interface.name) == 0 )
    {
        wayland->global_names[WAYLAND_GLOBAL_COMPOSITOR] = name;
        wayland->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, MIN(version, WL_COMPOSITOR_INTERFACE_VERSION));
    }
    else if ( g_strcmp0(interface, zwlr_layer_shell_v1_interface.name) == 0 ) {
        wayland->global_names[WAYLAND_GLOBAL_LAYER_SHELL] = name;
        wayland->layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, MIN(version, WL_LAYER_SHELL_INTERFACE_VERSION));
    }
    else if ( g_strcmp0(interface, wl_shm_interface.name) == 0 )
    {
        wayland->global_names[WAYLAND_GLOBAL_SHM] = name;
        wayland->shm = wl_registry_bind(registry, name, &wl_shm_interface, MIN(version, WL_SHM_INTERFACE_VERSION));
    }
    else if ( g_strcmp0(interface, wl_seat_interface.name) == 0 )
    {
        wayland_seat *seat = g_new0(wayland_seat, 1);
        seat->context = wayland;
        seat->global_name = name;
        seat->seat = wl_registry_bind(registry, name, &wl_seat_interface, MIN(version, WL_SEAT_INTERFACE_VERSION));
        g_hash_table_insert(wayland->seats, seat->seat, seat);

        wl_seat_add_listener(seat->seat, &wayland_seat_listener, seat);
    }
    else if ( g_strcmp0(interface, wl_output_interface.name) == 0 )
    {
        wayland_output *output = g_new0(wayland_output, 1);
        output->context = wayland;
        output->global_name = name;
        output->output = wl_registry_bind(registry, name, &wl_output_interface, MIN(version, WL_OUTPUT_INTERFACE_VERSION));
        output->scale = 1;

        g_hash_table_insert(wayland->outputs, output->output, output);

        wl_output_add_listener(output->output, &wayland_output_listener, output);
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
        case WAYLAND_GLOBAL_LAYER_SHELL:
            zwlr_layer_shell_v1_destroy(wayland->layer_shell);
            wayland->layer_shell = NULL;
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

static void wayland_layer_shell_surface_configure(void *data,
        struct zwlr_layer_surface_v1 *surface,
        uint32_t serial,
        uint32_t width,
        uint32_t height)
{
    wayland->layer_width = width;
    wayland->layer_height = height;
    zwlr_layer_surface_v1_ack_configure(surface, serial);
}

static void wayland_layer_shell_surface_closed(void *data,
        struct zwlr_layer_surface_v1 *surface)
{
    zwlr_layer_surface_v1_destroy(surface);
    wl_surface_destroy(wayland->surface);
    wayland->surface = NULL;
}

static const struct zwlr_layer_surface_v1_listener wayland_layer_shell_surface_listener = {
    .configure = wayland_layer_shell_surface_configure,
    .closed = wayland_layer_shell_surface_closed,
};

static gboolean
wayland_error(gpointer user_data)
{
    g_main_loop_quit(wayland->main_loop);
    return G_SOURCE_REMOVE;
}


static gboolean
wayland_display_setup(GMainLoop *main_loop, NkBindings *bindings)
{
    wayland->main_loop = main_loop;

    char *display = ( char *) g_getenv ( "WAYLAND_DISPLAY" );
    wayland->main_loop_source = g_water_wayland_source_new ( NULL, display );
    if ( wayland->main_loop_source == NULL ) {
        g_warning("Could not connect to the Wayland compositor");
        return FALSE;
    }

    g_water_wayland_source_set_error_callback(wayland->main_loop_source, wayland_error, NULL, NULL);

    wayland->buffer_count = 3;
    wayland->scale = 1;

    wayland->outputs = g_hash_table_new(g_direct_hash, g_direct_equal);
    wayland->seats = g_hash_table_new(g_direct_hash, g_direct_equal);
    wayland->seats_by_name = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

    wayland->display = g_water_wayland_source_get_display ( wayland->main_loop_source );
    wayland->registry = wl_display_get_registry ( wayland->display );
    wl_registry_add_listener ( wayland->registry, &wayland_registry_listener, NULL );
    wl_display_roundtrip ( wayland->display );

    if ( wayland->compositor == NULL || wayland->shm == NULL || g_hash_table_size(wayland->outputs) == 0 || g_hash_table_size(wayland->seats) == 0 ) {
        g_error( "Could not connect to wayland compositor" );
        return FALSE;
    }
    if ( wayland->layer_shell == NULL ) {
        g_error( "Rofi on wayland requires support for the layer shell protocol" );
        return FALSE;
    }

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

    wayland->surface = wl_compositor_create_surface(wayland->compositor);

    wayland->bindings_seat = nk_bindings_seat_new ( bindings, XKB_CONTEXT_NO_FLAGS );

    wayland->wlr_surface = zwlr_layer_shell_v1_get_layer_surface(wayland->layer_shell, wayland->surface, NULL, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "rofi");
    return TRUE;
}

static gboolean
wayland_display_late_setup ( void )
{
    // note: ANCHOR_LEFT is needed to get the full window width
    zwlr_layer_surface_v1_set_anchor( wayland->wlr_surface, ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT );
    zwlr_layer_surface_v1_set_keyboard_interactivity( wayland->wlr_surface, 1 );
    zwlr_layer_surface_v1_add_listener( wayland->wlr_surface, &wayland_layer_shell_surface_listener, NULL );

    // By setting 0 here, we'll get some useful sizes in the configure event
    zwlr_layer_surface_v1_set_size(wayland->wlr_surface, 0, 0);
    wl_surface_add_listener( wayland->surface, &wayland_surface_interface, wayland );
    wl_surface_commit( wayland->surface );
    wl_display_roundtrip( wayland->display );
    wayland_frame_callback( wayland, wayland->frame_cb, 0 );

    return TRUE;
}

gboolean
display_get_surface_dimensions ( int *width, int *height )
{
    if ( wayland->layer_width != 0 ) {
        if ( width != NULL )
            *width = wayland->layer_width;
        if ( height != NULL )
            *height = wayland->layer_height;
        return TRUE;
    }
    return FALSE;
}

void
display_set_surface_dimensions ( int width, int height, int loc )
{
    wayland->layer_width = width;
    wayland->layer_height = height;
    zwlr_layer_surface_v1_set_size( wayland->wlr_surface, width, height );

    uint32_t wlr_anchor = 0;
    switch ( loc )
    {
    case WL_NORTH_WEST:
        wlr_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
        break;
    case WL_NORTH:
        wlr_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
        break;
    case WL_NORTH_EAST:
        wlr_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
        break;
    case WL_EAST:
        wlr_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
        break;
    case WL_SOUTH_EAST:
        wlr_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
        break;
    case WL_SOUTH:
        wlr_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
        break;
    case WL_SOUTH_WEST:
        wlr_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
        break;
    case WL_WEST:
        wlr_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
        break;
    case WL_CENTER:
    default:
        break;
    }
    zwlr_layer_surface_v1_set_anchor( wayland->wlr_surface, wlr_anchor );
}

static void
wayland_display_early_cleanup(void)
{
}

static void
wayland_display_cleanup(void)
{
    if ( wayland->main_loop_source == NULL )
        return;

    if ( wayland->surface != NULL )
        wl_surface_destroy(wayland->surface);

    nk_bindings_seat_free( wayland->bindings_seat );
    g_hash_table_unref( wayland->seats_by_name);
    g_hash_table_unref( wayland->seats);
    g_hash_table_unref( wayland->outputs);
    wl_registry_destroy ( wayland->registry );
    wl_display_flush(wayland->display);
    g_water_wayland_source_free ( wayland->main_loop_source );
}


static void
wayland_display_dump_monitor_layout ( void )
{
}

static void
wayland_display_startup_notification ( RofiHelperExecuteContext *context, GSpawnChildSetupFunc *child_setup, gpointer *user_data )
{
}

static int wayland_display_monitor_active ( workarea *mon )
{
    // TODO: do something?
    return FALSE;
}

static const struct _view_proxy* wayland_display_view_proxy ( void )
{
    return wayland_view_proxy;
}

static display_proxy display_ = {
    .setup = wayland_display_setup,
    .late_setup = wayland_display_late_setup,
    .early_cleanup = wayland_display_early_cleanup,
    .cleanup =  wayland_display_cleanup,
    .dump_monitor_layout = wayland_display_dump_monitor_layout,
    .startup_notification = wayland_display_startup_notification,
    .monitor_active = wayland_display_monitor_active,

    .view = wayland_display_view_proxy,
};

display_proxy * const wayland_proxy = &display_;
