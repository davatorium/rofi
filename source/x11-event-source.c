#include <glib.h>
#include <X11/Xlib.h>
#include "x11-event-source.h"

/**
 * Custom X11 Source implementation.
 */
typedef struct _X11EventSource
{
    // Source
    GSource source;
    // Polling field
    GPollFD fd_x11;
    Display *display;
} X11EventSource;

static gboolean x11_event_source_prepare ( GSource * base, gint * timeout )
{
    X11EventSource *xs = (X11EventSource *) base;
    *timeout = -1;
    return XPending ( xs->display );
}

static gboolean x11_event_source_check ( GSource * base )
{
    X11EventSource *xs = (X11EventSource *) base;
    if ( xs->fd_x11.revents ) {
        return TRUE;
    }
    return FALSE;
}

static gboolean x11_event_source_dispatch ( GSource * base, GSourceFunc callback, gpointer data )
{
    X11EventSource *xs = (X11EventSource *) base;
    if ( callback ) {
        if ( xs->fd_x11.revents ) {
            callback ( data );
        }
    }
    return G_SOURCE_CONTINUE;;
}

static GSourceFuncs x11_event_source_funcs = {
    x11_event_source_prepare,
    x11_event_source_check,
    x11_event_source_dispatch,
    NULL
};

GSource * x11_event_source_new ( Display  *display )
{
    int            x11_fd  = ConnectionNumber ( display );
    X11EventSource *source = (X11EventSource *) g_source_new ( &x11_event_source_funcs, sizeof ( X11EventSource ) );
    source->display       = display;
    source->fd_x11.fd     = x11_fd;
    source->fd_x11.events = G_IO_IN | G_IO_ERR;
    g_source_add_poll ( (GSource *) source, &source->fd_x11 );
    return (GSource *) source;
}
