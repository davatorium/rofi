#ifndef ROFI_ICON_FETCHER_H
#define ROFI_ICON_FETCHER_H

#include <cairo.h>
#include <glib.h>
#include <stdint.h>

/**
 * @defgroup ICONFETCHER IconFetcher
 * @ingroup HELPERS
 *
 * Small helper of to fetch icons. This makes use of the 'view' threadpool.
 * @{
 */

/**
 * Initialize the icon fetcher.
 */
void rofi_icon_fetcher_init(void);

/**
 * Destroy and free the memory used by the icon fetcher.
 */
void rofi_icon_fetcher_destroy(void);

/**
 * @param name The name of the icon to fetch.
 * @param size The size of the icon to fetch.
 *
 * Query the icon-theme for icon with name and size.
 * The returned icon will be the best match for the requested size, it should
 * still be resized to the actual size.
 *
 * name can also be a full path, if prefixed with file://.
 *
 * @returns the uid identifying the request.
 */
uint32_t rofi_icon_fetcher_query(const char *name, const int size);

/**
 * @param name The name of the icon to fetch.
 * @param wsize The width of the icon to fetch.
 * @param hsize The height of the icon to fetch.
 *
 * Query the icon-theme for icon with name and size.
 * The returned icon will be the best match for the requested size, it should
 * still be resized to the actual size. For icons it will take the min of wsize
 * and hsize.
 *
 * name can also be a full path, if prefixed with file://.
 *
 * @returns the uid identifying the request.
 */
uint32_t rofi_icon_fetcher_query_advanced(const char *name, const int wsize,
                                          const int hsize);

/**
 * @param uid The unique id representing the matching request.
 *
 * If the surface is used, the user should reference the surface.
 *
 * @returns the surface with the icon, NULL when not found.
 */
cairo_surface_t *rofi_icon_fetcher_get(const uint32_t uid);

/**
 * @param path the image path to check.
 *
 * Checks if a file is a supported image. (by looking at extension).
 *
 * @returns true if image, false otherwise.
 */
gboolean rofi_icon_fetcher_file_is_image(const char *const path);
/** @} */
#endif // ROFI_ICON_FETCHER_H
