/**
 * rofi
 *
 * MIT/X11 License
 * Copyright (c)  2013-2016 Qball Cow <qball@gmpclient.org>
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
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <xcb/xcb.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "rofi.h"
#include "x11-helper.h"
#include "i3-support.h"
#include "view.h"

#ifdef HAVE_I3_IPC_H
#include <i3/ipc.h>
// Path to HAVE_I3_IPC_H socket.
char                *i3_socket_path = NULL;
extern xcb_screen_t *xcb_screen;

void i3_support_focus_window ( xcb_window_t id )
{
    i3_ipc_header_t    head;
    int                s;
    ssize_t            t;
    struct sockaddr_un remote;
    size_t             upm = sizeof ( remote.sun_path );
    char               command[upm];

    if ( strlen ( i3_socket_path ) > upm ) {
        fprintf ( stderr, "Socket path is too long. %zu > %zu\n", strlen ( i3_socket_path ), upm );
        return;
    }

    if ( ( s = socket ( AF_UNIX, SOCK_STREAM, 0 ) ) == -1 ) {
        fprintf ( stderr, "Failed to open connection to I3: %s\n", strerror ( errno ) );
        return;
    }

    remote.sun_family = AF_UNIX;
    g_strlcpy ( remote.sun_path, i3_socket_path, upm );

    if ( connect ( s, ( struct sockaddr * ) &remote, sizeof ( struct sockaddr_un ) ) == -1 ) {
        fprintf ( stderr, "Failed to connect to I3 (%s): %s\n", i3_socket_path, strerror ( errno ) );
        close ( s );
        return;
    }

    // Formulate command
    snprintf ( command, upm, "[id=\"%lu\"] focus", id );
    // Prepare header.
    memcpy ( head.magic, I3_IPC_MAGIC, 6 );
    head.size = strlen ( command );
    head.type = I3_IPC_MESSAGE_TYPE_COMMAND;
    // Send header.
    t = send ( s, &head, sizeof ( i3_ipc_header_t ), 0 );
    if ( t == -1 ) {
        char *msg = g_strdup_printf ( "Failed to send message header to i3: %s\n", strerror ( errno ) );
        rofi_view_error_dialog ( msg, FALSE );
        g_free ( msg );
        close ( s );
        return;
    }
    // Send message
    t = send ( s, command, strlen ( command ), 0 );
    if ( t == -1 ) {
        char *msg = g_strdup_printf ( "Failed to send message body to i3: %s\n", strerror ( errno ) );
        rofi_view_error_dialog ( msg, FALSE  );
        g_free ( msg );
        close ( s );
        return;
    }
    // Receive header.
    t = recv ( s, &head, sizeof ( head ), 0 );

    if ( t == sizeof ( head ) ) {
        t = recv ( s, command, head.size, 0 );
        if ( t == head.size ) {
            // Response.
        }
    }

    close ( s );
}

int i3_support_initialize ( xcb_connection_t *xcb_connection )
{
    // If we where initialized, clean this first.
    i3_support_free_internals ();

    // Get atom for I3_SOCKET_PATH
    i3_socket_path = window_get_text_prop ( xcb_connection, xcb_screen->root, netatoms[I3_SOCKET_PATH] );
    // If we find it, go into i3 mode.
    return ( i3_socket_path != NULL ) ? TRUE : FALSE;
}

void i3_support_free_internals ( void )
{
    g_free ( i3_socket_path );
    i3_socket_path = NULL;
}

#else

void i3_support_focus_window ( G_GNUC_UNUSED xcb_window_t id )
{
    fprintf ( stderr, "Trying to control i3, when i3 support is not enabled.\n" );
    abort ();
}
void i3_support_free_internals ( void )
{
}

int i3_support_initialize ( xcb_connection_t *xcb_connection )
{
    return FALSE;
}
#endif // HAVE_I3_IPC_H
