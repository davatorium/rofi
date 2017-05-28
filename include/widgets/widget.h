/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2013-2017 Qball Cow <qball@gmpclient.org>
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

#ifndef ROFI_WIDGET_H
#define ROFI_WIDGET_H
#include <glib.h>
#include <cairo.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include "keyb.h"
/**
 * @defgroup widget widget
 *
 * Generic abstract widget class. Widgets should 'inherit' from this class (first structure in there structure should be
 * widget).
 * The generic widget implements generic functions like get_width, get_height, draw, resize, update, free and
 * clicked.
 * It also holds information about how the widget should be packed.
 *
 * @{
 */

/**
 * Abstract structure holding internal state of a widget.
 * Structure is elaborated in widget-internal.h
 */
typedef struct _widget   widget;
typedef enum
{
    WIDGET_TYPE_UNKNOWN,
    WIDGET_TYPE_LISTVIEW         = SCOPE_MOUSE_LISTVIEW,
    WIDGET_TYPE_LISTVIEW_ELEMENT = SCOPE_MOUSE_LISTVIEW_ELEMENT,
    WIDGET_TYPE_EDITBOX          = SCOPE_MOUSE_EDITBOX,
    WIDGET_TYPE_SCROLLBAR        = SCOPE_MOUSE_SCROLLBAR,
    WIDGET_TYPE_SIDEBAR_MODI     = SCOPE_MOUSE_SIDEBAR_MODI,
} WidgetType;

typedef widget * ( *widget_find_mouse_target_cb )( widget *, WidgetType type, gint *x, gint *y );
typedef gboolean ( *widget_trigger_action_cb )( widget *, guint action, gint x, gint y, void * );

/** Macro to get widget from an implementation (e.g. textbox/scrollbar) */
#define WIDGET( a )    ( (widget *) ( a ) )

/**
 * @param widget The widget to check
 * @param x The X position relative to parent window
 * @param y the Y position relative to parent window
 *
 * Check if x,y falls within the widget.
 *
 * @return TRUE if x,y falls within the widget
 */
int widget_intersect ( const widget *widget, int x, int y );

/**
 * @param widget The widget to move
 * @param x The new X position relative to parent window
 * @param y The new Y position relative to parent window
 *
 * Moves the widget.
 */
void widget_move ( widget *widget, short x, short y );

/**
 * @param widget Handle to widget
 *
 * Get the type of the widget.
 * @returns The type of the widget.
 */
WidgetType widget_type ( widget *widget );

/**
 * @param widget Handle to widget
 *
 * Check if widget is enabled.
 * @returns TRUE when widget is enabled.
 */
gboolean widget_enabled ( widget *widget );
/**
 * @param widget Handle to widget
 *
 * Disable the widget.
 */
void widget_disable ( widget *widget );
/**
 * @param widget Handle to widget
 *
 * Enable the widget.
 */
void widget_enable ( widget *widget );

/**
 * @param widget widget  Handle to the widget
 * @param d The cairo object used to draw itself.
 *
 * Render the textbox.
 */
void widget_draw ( widget *widget, cairo_t *d );

/**
 * @param wid Handle to the widget
 *
 * Free the widget and all allocated memory.
 */
void widget_free ( widget *wid );

/**
 * @param widget The widget toresize
 * @param w The new width
 * @param h The new height
 *
 * Resizes the widget.
 */
void widget_resize ( widget *widget, short w, short h );

/**
 * @param widget The widget handle
 *
 * @returns the height of the widget.
 */
int widget_get_height ( widget *widget );

/**
 * @param widget The widget handle
 *
 * @returns the width of the widget.
 */
int widget_get_width ( widget *widget );

/**
 * @param widget The widget handle
 *
 * @returns the y postion of the widget relative to its parent.
 */
int widget_get_y_pos ( widget *widget );

/**
 * @param widget The widget handle
 *
 * @returns the x postion of the widget relative to its parent.
 */
int widget_get_x_pos ( widget *widget );

/**
 * @param widget The widget handle
 *
 * Update the widget, and its parent recursively.
 * This should be called when size of widget changes.
 */
void widget_update ( widget *widget );
/**
 * @param wid The widget handle
 *
 * Indicate that the widget needs to be redrawn.
 * This is done by setting the redraw flag on the toplevel widget.
 */
void widget_queue_redraw ( widget *wid );
/**
 * @param wid The widget handle
 *
 * Check the flag indicating the widget needs to be redrawn.
 */
gboolean widget_need_redraw ( widget *wid );

/**
 * @param wid The widget handle
 * @param x A pointer to the x coordinate of the mouse event
 * @param y A pointer to the y coordinate of the mouse event
 *
 * Get the widget that should handle a mouse event.
 * @x and @y are adjusted to be relative to the widget.
 *
 * @returns returns the widget that should handle the mouse event.
 */
widget *widget_find_mouse_target ( widget *wid, WidgetType type, gint *x, gint *y );

/**
 * @param wid The widget handle
 * @param action The action to trigger
 * @param x A pointer to the x coordinate of the click
 * @param y A pointer to the y coordinate of the click
 *
 * Trigger an action on widget.
 * @x and @y are relative to the widget.
 */
gboolean widget_trigger_action ( widget *wid, guint action, gint x, gint y );

/**
 * @param wid The widget handle
 * @param cb The widget trigger action callback
 * @param udata the user data to pass to callback
 *
 * Override the widget trigger action handler on widget.
 */
void widget_set_trigger_action_handler ( widget *wid, widget_trigger_action_cb cb, void *udata );

/**
 * @param wid The widget handle
 * @param xme The motion notify object.
 *
 * Motion notify.
 * TODO make this like clicked with callback.
 * returns TRUE when handled.
 */
gboolean widget_motion_notify ( widget *wid, xcb_motion_notify_event_t *xme );

/**
 * @param wid The widget handle
 *
 * Get the desired height of this widget recursively.
 *
 * @returns the desired height of the widget in pixels.
 */
int widget_get_desired_height ( widget *wid );

/*@}*/
#endif // ROFI_WIDGET_H
