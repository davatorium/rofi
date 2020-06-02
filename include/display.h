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

#ifndef ROFI_DISPLAY_H
#define ROFI_DISPLAY_H

#include <glib.h>
#include "helper.h"
#include "nkutils-bindings.h"

/**
 * Structure describing a workarea/monitor.
 */
typedef struct _workarea
{
    /** numeric monitor id. */
    int              monitor_id;
    /** if monitor is set as primary monitor. */
    int              primary;
    /** Horizontal location (in pixels) of the monitor. */
    int              x;
    /** Vertical location  (in pixels) of the monitor. */
    int              y;
    /** Width of the monitor. */
    int              w;
    /** Height of the monitor */
    int              h;
    int              mw, mh;
    /** Output name of the monitor, e.g. eDP1 or VGA-1 */
    char             *name;
    /** Pointer to next monitor */
    struct _workarea *next;
} workarea;

struct _view_proxy;

typedef struct _display_proxy {
    gboolean (*setup) ( GMainLoop *main_loop, NkBindings *bindings );
    gboolean (*late_setup) ( void );
    void (*early_cleanup) ( void );
    void (*cleanup) ( void );
    void (*dump_monitor_layout) ( void );
    void (*startup_notification) ( RofiHelperExecuteContext *context, GSpawnChildSetupFunc *child_setup, gpointer *user_data );
    int (*monitor_active) ( workarea *mon );

    const struct _view_proxy* (*view) ( void );
} display_proxy;

/* Implementations */
extern display_proxy * const xcb_proxy;
#ifdef ENABLE_WAYLAND
extern display_proxy * const wayland_proxy;
#endif

void display_init( const display_proxy *disp_in );

/**
 * @param mon workarea to be filled in.
 *
 * Fills in #mon with the information about the monitor rofi should show on.
 *
 * @returns TRUE if monitor is found, FALSE if no monitor could be detected.
 */
int monitor_active ( workarea *mon );

/**
 * @param main_loop The GMainLoop
 * @param bindings The bindings object
 *
 * Setup the display backend
 *
 * @returns Whether the setup succeeded or not
 */
gboolean display_setup ( GMainLoop *main_loop, NkBindings *bindings );

/**
 * Do some late setup of the display backend
 *
 * @returns Whether the setup succeeded or not
 */
gboolean display_late_setup ( void );

/**
 * Do some early cleanup, like unmapping the surface
 */
void display_early_cleanup ( void );

/**
 * Cleanup any remaining display related stuff
 */
void display_cleanup ( void );

/**
 * Dumps the display layout for -help output
 */
void display_dump_monitor_layout ( void );

/**
 * @param context The startup notification context for the application to launch
 * @param child_setup A pointer to return the child setup function
 * @param user_data A pointer to return the child setup function user_data
 *
 * Provides the needed child setup function
 */
void display_startup_notification ( RofiHelperExecuteContext *context, GSpawnChildSetupFunc *child_setup, gpointer *user_data );


#endif
