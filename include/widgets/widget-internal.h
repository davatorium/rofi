#ifndef WIDGET_INTERNAL_H
#define WIDGET_INTERNAL_H

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
    /** enabled or not */
    gboolean          enabled;
    /** Information about packing. */
    gboolean          expand;
    gboolean          end;

    struct _widget    *parent;
    /** Internal */
    gboolean          need_redraw;
    /** Function prototypes */
    int               ( *get_width )( struct _widget * );
    int               ( *get_height )( struct _widget * );

    void              ( *draw )( struct _widget *widget, cairo_t *draw );
    void              ( *resize )( struct _widget *, short, short );
    void              ( *update )( struct _widget * );

    // Signals.
    widget_clicked_cb clicked;
    void              *clicked_cb_data;

    // Free
    void              ( *free )( struct _widget *widget );
};
#endif // WIDGET_INTERNAL_H
