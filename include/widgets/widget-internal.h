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

void widget_init ( widget *widget , const char *name, const char *class_name );
void widget_set_state ( widget *widget, const char *state );

int widget_padding_get_left ( const widget *wid );
int widget_padding_get_right ( const widget *wid );
int widget_padding_get_top ( const widget *wid );
int widget_padding_get_bottom ( const widget *wid );
int widget_padding_get_remaining_width ( const widget *wid );
int widget_padding_get_remaining_height ( const widget *wid );
int widget_padding_get_padding_height ( const widget *wid );
int widget_padding_get_padding_width ( const widget *wid );
#endif // WIDGET_INTERNAL_H
