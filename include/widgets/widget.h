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
typedef struct _widget widget;

/**
 * Type of the widget. It is used to bubble events to the relevant widget.
 */
typedef enum
{
    /** Default type */
    WIDGET_TYPE_UNKNOWN,
    /** The listview widget */
    WIDGET_TYPE_LISTVIEW         = SCOPE_MOUSE_LISTVIEW,
    /** An element in the listview */
    WIDGET_TYPE_LISTVIEW_ELEMENT = SCOPE_MOUSE_LISTVIEW_ELEMENT,
    /** The input bar edit box */
    WIDGET_TYPE_EDITBOX          = SCOPE_MOUSE_EDITBOX,
    /** The listview scrollbar */
    WIDGET_TYPE_SCROLLBAR        = SCOPE_MOUSE_SCROLLBAR,
    /** A widget allowing user to swithc between modi */
    WIDGET_TYPE_MODE_SWITCHER    = SCOPE_MOUSE_MODE_SWITCHER,
    /** Text-only textbox */
    WIDGET_TYPE_TEXTBOX_TEXT,
} WidgetType;

/**
 * Whether and how the action was handled
 */
typedef enum
{
    /** The action was ignore and should bubble */
    WIDGET_TRIGGER_ACTION_RESULT_IGNORED,
    /** The action was handled directly */
    WIDGET_TRIGGER_ACTION_RESULT_HANDLED,
    /** The action was handled and should start the grab for motion events */
    WIDGET_TRIGGER_ACTION_RESULT_GRAB_MOTION_BEGIN,
    /** The action was handled and should stop the grab for motion events */
    WIDGET_TRIGGER_ACTION_RESULT_GRAB_MOTION_END,
} WidgetTriggerActionResult;

/**
 * @param widget The container widget itself
 * @param type The widget type searched for
 * @param x The X coordination of the mouse event relative to @param widget
 * @param y The Y coordination of the mouse event relative to @param widget
 *
 * This callback must only iterate over the children of a Widget, and return NULL if none of them is relevant.
 *
 * @returns A child widget if found, NULL otherwise
 */
typedef widget * ( *widget_find_mouse_target_cb )( widget *widget, WidgetType type, gint x, gint y );

/**
 * @param widget The target widget
 * @param action The action value (which enum it is depends on the widget type)
 * @param x The X coordination of the mouse event relative to @param widget
 * @param y The Y coordination of the mouse event relative to @param widget
 * @param user_data The data passed to widget_set_trigger_action_handler()
 *
 * This callback should handle the action if relevant, and returns whether it did or not.
 *
 * @returns Whether the action was handled or not, see enum values for details
 */
typedef WidgetTriggerActionResult ( *widget_trigger_action_cb )( widget *widget, guint action, gint x, gint y, void *user_data );

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
 * @param type The widget type.
 *
 * Set the widget type.
 */
void widget_set_type ( widget *widget, WidgetType type );

/**
 * @param widget Handle to widget
 *
 * Check if widget is enabled.
 * @returns TRUE when widget is enabled.
 */
gboolean widget_enabled ( widget *widget );

/**
 * @param widget Handle to widget
 * @param enabled The new state
 *
 * Disable the widget.
 */
void widget_set_enabled ( widget *widget, gboolean enabled );

/**
 * @param widget Handle to widget
 *
 * Disable the widget.
 */
static inline
void widget_disable ( widget *widget )
{
    widget_set_enabled ( widget, FALSE );
}
/**
 * @param widget Handle to widget
 *
 * Enable the widget.
 */
static inline
void widget_enable ( widget *widget )
{
    widget_set_enabled ( widget, TRUE );
}

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
 * @returns the y position of the widget relative to its parent.
 */
int widget_get_y_pos ( widget *widget );

/**
 * @param widget The widget handle
 *
 * @returns the x position of the widget relative to its parent.
 */
int widget_get_x_pos ( widget *widget );

/**
 * @param widget The widget handle
 * @param x A pointer to the absolute X coordinates
 * @param y A pointer to the absolute Y coordinates
 *
 * Will modify param x and param y to make them relative to param widget .
 */
void widget_xy_to_relative ( widget *widget, gint *x, gint *y );

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
 * @param type The type of the wanted widget
 * @param x The x coordinate of the mouse event
 * @param y The y coordinate of the mouse event
 *
 * Get the widget that should handle a mouse event.
 *
 * @returns returns the widget that should handle the mouse event.
 */
widget *widget_find_mouse_target ( widget *wid, WidgetType type, gint x, gint y );

/**
 * @param wid The widget handle
 * @param action The action to trigger
 * @param x A pointer to the x coordinate of the click
 * @param y A pointer to the y coordinate of the click
 *
 * Trigger an action on widget.
 * param x and param y are relative to param wid .
 *
 * @returns Whether the action was handled or not
 */
WidgetTriggerActionResult widget_trigger_action ( widget *wid, guint action, gint x, gint y );

/**
 * @param wid The widget handle
 * @param cb The widget trigger action callback
 * @param cb_data the user data to pass to callback
 *
 * Override the widget trigger action handler on widget.
 */
void widget_set_trigger_action_handler ( widget *wid, widget_trigger_action_cb cb, void *cb_data );

/**
 * @param wid The widget handle
 * @param x The x coordinate of the mouse event
 * @param y The y coordinate of the mouse event
 *
 * Motion notify.
 *
 * @returns TRUE when handled.
 */
gboolean widget_motion_notify ( widget *wid, gint x, gint y );

/**
 * @param wid The widget handle
 *
 * Get the desired height of this widget recursively.
 *
 * @returns the desired height of the widget in pixels.
 */
int widget_get_desired_height ( widget *wid );

/**
 * @param wid The widget handle
 *
 * Get the desired width of this widget recursively.
 *
 * @returns the desired width of the widget in pixels.
 */
int widget_get_desired_width ( widget *wid );
/**
 * @param wid The widget handle
 *
 * Get the absolute x-position on the root widget..
 *
 * @returns the absolute x-position of widget of the widget in pixels.
 */
int widget_get_absolute_xpos ( widget *wid );
/**
 * @param wid The widget handle
 *
 * Get the absolute y-position on the root widget..
 *
 * @returns the absolute y-position of widget of the widget in pixels.
 */
int widget_get_absolute_ypos ( widget *wid );

/**@}*/
#endif // ROFI_WIDGET_H
