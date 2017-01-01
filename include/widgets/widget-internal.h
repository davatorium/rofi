#ifndef WIDGET_INTERNAL_H
#define WIDGET_INTERNAL_H

#include "theme.h"
/**
 * Data structure holding the internal state of the Widget
 */
struct _widget
{
    /** X position relative to parent */
    short             x;
    /** Y position relative to parent */
    short             y;
    /** Width of the widget */
    short             w;
    /** Height of the widget */
    short             h;
    /** Padding */
    Padding           pad;
    /** enabled or not */
    gboolean          enabled;
    /** Expand the widget when packed */
    gboolean          expand;
    /** Place widget at end of parent */
    gboolean          end;
    /** Parent widget */
    struct _widget    *parent;
    /** Internal */
    gboolean          need_redraw;
    /** get width of widget implementation function */
    int               ( *get_width )( struct _widget * );
    /** get height of widget implementation function */
    int               ( *get_height )( struct _widget * );
    /** draw widget implementation function */
    void              ( *draw )( struct _widget *widget, cairo_t *draw );
    /** resize widget implementation function */
    void              ( *resize )( struct _widget *, short, short );
    /** update widget implementation function */
    void              ( *update )( struct _widget * );

    /** Handle mouse motion, used for dragging */
    gboolean          ( *motion_notify )( struct _widget *, xcb_motion_notify_event_t * );

    int               (*get_desired_height) ( struct _widget * );

    /** widget clicked callback */
    widget_clicked_cb clicked;
    /** user data for clicked callback */
    void              *clicked_cb_data;

    /** Free widget callback */
    void              ( *free )( struct _widget *widget );

    /** Name of widget (used for theming) */
    char *name;
    char *class_name;
    const char *state;
};

/**
 * @param widget The widget to initialize.
 * @param name The name of the widget.
 * @param class_name The name of the class of the widget.
 *
 * Initializes the widget structure.
 *
 */
void widget_init ( widget *widget , const char *name, const char *class_name );

/**
 * @param widget The widget handle.
 * @param state  The state of the widget.
 *
 * Set the state of the widget.
 */
void widget_set_state ( widget *widget, const char *state );

/**
 * @param wid The widget handle.
 *
 * Get the left padding of the widget.
 *
 * @returns the left padding in pixels.
 */
int widget_padding_get_left ( const widget *wid );

/**
 * @param wid The widget handle.
 *
 * Get the right padding of the widget.
 *
 * @returns the right padding in pixels.
 */
int widget_padding_get_right ( const widget *wid );

/**
 * @param wid The widget handle.
 *
 * Get the top padding of the widget.
 *
 * @returns the top padding in pixels.
 */
int widget_padding_get_top ( const widget *wid );

/**
 * @param wid The widget handle.
 *
 * Get the bottom padding of the widget.
 *
 * @returns the bottom padding in pixels.
 */
int widget_padding_get_bottom ( const widget *wid );

/**
 * @param wid The widget handle.
 *
 * Get width of the content of the widget 
 *
 * @returns the widget width, excluding padding. 
 */
int widget_padding_get_remaining_width ( const widget *wid );
/**
 * @param wid The widget handle.
 *
 * Get height of the content of the widget 
 *
 * @returns the widget height, excluding padding. 
 */
int widget_padding_get_remaining_height ( const widget *wid );
/**
 * @param wid The widget handle.
 *
 * Get the combined top and bottom padding.
 *
 * @returns the top and bottom padding of the widget in pixels.
 */
int widget_padding_get_padding_height ( const widget *wid );
/**
 * @param wid The widget handle.
 *
 * Get the combined left and right padding.
 *
 * @returns the left and right padding of the widget in pixels.
 */
int widget_padding_get_padding_width ( const widget *wid );
#endif // WIDGET_INTERNAL_H
