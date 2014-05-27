#ifndef __XRMOPTIONS_H__
#define __XRMOPTIONS_H__

/**
 * @param display Handler of the display to fetch the settings from.
 *
 * Parse the rofi related X resource options of the
 * connected X server.
 */
void parse_xresource_options ( Display *display );

/**
 * Free any allocated memory.
 */
void parse_xresource_free ( void );

/**
 * Dump the settings in a Xresources compatible way to
 * stdout.
 */
void xresource_dump ( void );

#endif
