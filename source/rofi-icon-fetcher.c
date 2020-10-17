/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2020 Qball Cow <qball@gmpclient.org>
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
#define G_LOG_DOMAIN    "Helpers.IconFetcher"

#include "rofi-icon-fetcher.h"
#include "rofi-types.h"
#include "helper.h"
#include "settings.h"

#include "xcb.h"
#include "keyb.h"
#include "view.h"

#include "nkutils-xdg-theme.h"

#include <stdint.h>
#include <jpeglib.h>

#include <setjmp.h>

typedef struct
{
    // Context for icon-themes.
    NkXdgThemeContext *xdg_context;

    // On name.
    GHashTable        *icon_cache;
    // On uid.
    GHashTable        *icon_cache_uid;

    uint32_t          last_uid;
} IconFetcher;

typedef struct
{
    char  *name;
    GList *sizes;
} IconFetcherNameEntry;

typedef struct
{
    thread_state         state;

    GCond                *cond;
    GMutex               *mutex;
    unsigned int         *acount;

    uint32_t             uid;
    int                  size;
    cairo_surface_t      *surface;

    IconFetcherNameEntry *entry;
} IconFetcherEntry;

/**
 * The icon fetcher internal state.
 */
IconFetcher *rofi_icon_fetcher_data = NULL;

static void rofi_icon_fetch_entry_free ( gpointer data )
{
    IconFetcherNameEntry *entry = (IconFetcherNameEntry *) data;

    // Free name/key.
    g_free ( entry->name );

    for ( GList *iter = g_list_first ( entry->sizes ); iter; iter = g_list_next ( iter ) ) {
        IconFetcherEntry *sentry = (IconFetcherEntry *) ( iter->data );

        cairo_surface_destroy ( sentry->surface );
        g_free ( sentry );
    }

    g_list_free ( entry->sizes );
    g_free ( entry );
}

void rofi_icon_fetcher_init ( void )
{
    g_assert ( rofi_icon_fetcher_data == NULL );

    static const gchar * const icon_fallback_themes[] = {
        "Adwaita",
        "gnome",
        NULL
    };
    const char                 *themes[2] = { config.icon_theme, NULL };

    rofi_icon_fetcher_data = g_malloc0 ( sizeof ( IconFetcher ) );

    rofi_icon_fetcher_data->xdg_context = nk_xdg_theme_context_new ( icon_fallback_themes, NULL );
    nk_xdg_theme_preload_themes_icon ( rofi_icon_fetcher_data->xdg_context, themes );

    rofi_icon_fetcher_data->icon_cache_uid = g_hash_table_new ( g_direct_hash, g_direct_equal );
    rofi_icon_fetcher_data->icon_cache     = g_hash_table_new_full ( g_str_hash, g_str_equal, NULL, rofi_icon_fetch_entry_free );
}

void rofi_icon_fetcher_destroy ( void )
{
    if ( rofi_icon_fetcher_data == NULL ) {
        return;
    }

    nk_xdg_theme_context_free ( rofi_icon_fetcher_data->xdg_context );

    g_hash_table_unref ( rofi_icon_fetcher_data->icon_cache_uid );
    g_hash_table_unref ( rofi_icon_fetcher_data->icon_cache );

    g_free ( rofi_icon_fetcher_data );
}

static cairo_surface_t* cairo_image_surface_create_from_jpeg_private ( struct jpeg_decompress_struct* cinfo )
{
    cairo_surface_t* surface = 0;
    unsigned char  * data    = 0;
    unsigned char  * rgb     = 0;

    jpeg_read_header ( cinfo, TRUE );
    jpeg_start_decompress ( cinfo );

    surface = cairo_image_surface_create ( CAIRO_FORMAT_RGB24, cinfo->image_width, cinfo->image_height );
    data    = cairo_image_surface_get_data ( surface );
    rgb     = (unsigned char *) ( malloc ( cinfo->output_width * cinfo->output_components ) );

    while ( cinfo->output_scanline < cinfo->output_height ) {
        unsigned int i;
        int          scanline = cinfo->output_scanline * cairo_image_surface_get_stride ( surface );

        jpeg_read_scanlines ( cinfo, &rgb, 1 );

        for ( i = 0; i < cinfo->output_width; i++ ) {
            int offset = scanline + ( i * 4 );

            data[offset + 3] = 255;
            data[offset + 2] = rgb[( i * 3 )];
            data[offset + 1] = rgb[( i * 3 ) + 1];
            data[offset    ] = rgb[( i * 3 ) + 2];
        }
    }

    free ( rgb );

    jpeg_finish_decompress ( cinfo );
    jpeg_destroy_decompress ( cinfo );

    cairo_surface_mark_dirty ( surface );

    return surface;
}
struct jpegErrorManager {
    /* "public" fields */
    struct jpeg_error_mgr pub;
    /* for return to caller */
    jmp_buf setjmp_buffer;
};
char jpegLastErrorMsg[JMSG_LENGTH_MAX];
static void jpegErrorExit (j_common_ptr cinfo)
{
    /* cinfo->err actually points to a jpegErrorManager struct */
    struct jpegErrorManager* myerr = (struct jpegErrorManager*) cinfo->err;

    /* Create the message */
    ( *(cinfo->err->format_message) ) (cinfo, jpegLastErrorMsg);
    g_warning ( jpegLastErrorMsg );

    /* Jump to the setjmp point */
    longjmp(myerr->setjmp_buffer, 1);
}

static cairo_surface_t* cairo_image_surface_create_from_jpeg ( const char* file )
{
    struct jpeg_decompress_struct cinfo;
    cairo_surface_t               * surface;
    FILE                          * infile;

    if ( ( infile = fopen ( file, "rb" ) ) == NULL ) {
        return NULL;
    }

    struct jpegErrorManager jerr;
    cinfo.err = jpeg_std_error( &jerr.pub);
    jerr.pub.error_exit = jpegErrorExit;
    /* Establish the setjmp return context for my_error_exit to use. */
    if (setjmp(jerr.setjmp_buffer)) {
      /* If we get here, the JPEG code has signaled an error. */
      jpeg_destroy_decompress(&cinfo);
      fclose(infile);
      return NULL;
    }

    jpeg_create_decompress ( &cinfo );
    jpeg_stdio_src ( &cinfo, infile );

    surface = cairo_image_surface_create_from_jpeg_private ( &cinfo );

    fclose ( infile );

    return surface;
}

static void rofi_icon_fetcher_worker ( thread_state *sdata, G_GNUC_UNUSED gpointer user_data )
{
    g_debug ( "starting up icon fetching thread." );
    // as long as dr->icon is updated atomicly.. (is a pointer write atomic?)
    // this should be fine running in another thread.
    IconFetcherEntry *sentry   = (IconFetcherEntry *) sdata;
    const gchar      *themes[] = {
        config.icon_theme,
        NULL
    };

    const gchar      *icon_path;
    gchar            *icon_path_ = NULL;

    if ( g_path_is_absolute ( sentry->entry->name ) ) {
        icon_path = sentry->entry->name;
    }
    else {
        icon_path = icon_path_ = nk_xdg_theme_get_icon ( rofi_icon_fetcher_data->xdg_context, themes, NULL, sentry->entry->name, sentry->size, 1, TRUE );
        if ( icon_path_ == NULL ) {
            g_debug ( "failed to get icon %s(%d): n/a", sentry->entry->name, sentry->size  );
            return;
        }
        else{
            g_debug ( "found icon %s(%d): %s", sentry->entry->name, sentry->size, icon_path  );
        }
    }
    cairo_surface_t *icon_surf = NULL;
    if ( g_str_has_suffix ( icon_path, ".png" ) || g_str_has_suffix ( icon_path, ".PNG" )  ) {
        icon_surf = cairo_image_surface_create_from_png ( icon_path );
    }
    else if (  g_str_has_suffix ( icon_path, ".jpeg" ) || g_str_has_suffix ( icon_path, ".jpg" ) ||
        g_str_has_suffix ( icon_path, ".JPEG" ) || g_str_has_suffix ( icon_path, ".JPG" )
        ) {
        icon_surf = cairo_image_surface_create_from_jpeg ( icon_path );
    }
    else if ( g_str_has_suffix ( icon_path, ".svg" ) ||  g_str_has_suffix ( icon_path, ".SVG" ) ) {
        icon_surf = cairo_image_surface_create_from_svg ( icon_path, sentry->size );
    }
    else {
        g_debug ( "icon type not yet supported: %s", icon_path  );
    }
    if ( icon_surf ) {
        if ( cairo_surface_status ( icon_surf ) == CAIRO_STATUS_SUCCESS ) {
            float sw = sentry->size / (float) cairo_image_surface_get_width ( icon_surf );
            float sh = sentry->size / (float) cairo_image_surface_get_height ( icon_surf );

            float scale = ( sw > sh ) ? sh : sw;
            if ( scale < 0.5 ) {
                cairo_surface_t * surface = cairo_image_surface_create (
                    cairo_image_surface_get_format ( icon_surf ),
                    cairo_image_surface_get_width ( icon_surf ) * scale,
                    cairo_image_surface_get_height ( icon_surf ) * scale );

                cairo_t *d = cairo_create ( surface );
                cairo_scale ( d, scale, scale );
                cairo_set_source_surface ( d, icon_surf, 0.0, 0.0 );
                cairo_pattern_set_filter ( cairo_get_source ( d ), CAIRO_FILTER_FAST );
                cairo_paint ( d );

                cairo_destroy ( d );
                cairo_surface_destroy ( icon_surf );
                icon_surf = surface;
            }
        }
        // check if surface is valid.
        if ( cairo_surface_status ( icon_surf ) != CAIRO_STATUS_SUCCESS ) {
            g_debug ( "icon failed to open: %s(%d): %s", sentry->entry->name, sentry->size, icon_path );
            cairo_surface_destroy ( icon_surf );
            icon_surf = NULL;
        }
        sentry->surface = icon_surf;
    }
    g_free ( icon_path_ );
    rofi_view_reload ();
}

uint32_t rofi_icon_fetcher_query ( const char *name, const int size )
{
    g_debug ( "Query: %s(%d)", name, size );
    IconFetcherNameEntry *entry = g_hash_table_lookup ( rofi_icon_fetcher_data->icon_cache, name );
    if ( entry == NULL ) {
        entry       = g_new0 ( IconFetcherNameEntry, 1 );
        entry->name = g_strdup ( name );
        g_hash_table_insert ( rofi_icon_fetcher_data->icon_cache, entry->name, entry );
    }
    IconFetcherEntry *sentry;
    for ( GList *iter = g_list_first ( entry->sizes ); iter; iter = g_list_next ( iter ) ) {
        sentry = iter->data;
        if ( sentry->size == size ) {
            return sentry->uid;
        }
    }

    // Not found.
    sentry          = g_new0 ( IconFetcherEntry, 1 );
    sentry->uid     = ++( rofi_icon_fetcher_data->last_uid );
    sentry->size    = size;
    sentry->entry   = entry;
    sentry->surface = NULL;

    entry->sizes = g_list_prepend ( entry->sizes, sentry );
    g_hash_table_insert ( rofi_icon_fetcher_data->icon_cache_uid, GINT_TO_POINTER ( sentry->uid ), sentry );

    // Push into fetching queue.
    sentry->state.callback = rofi_icon_fetcher_worker;
    g_thread_pool_push ( tpool, sentry, NULL );

    return sentry->uid;
}

cairo_surface_t * rofi_icon_fetcher_get  ( const uint32_t uid )
{
    IconFetcherEntry *sentry = g_hash_table_lookup ( rofi_icon_fetcher_data->icon_cache_uid, GINT_TO_POINTER ( uid ) );
    if ( sentry ) {
        return sentry->surface;
    }
    return NULL;
}
