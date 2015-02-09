/**
 * rofi
 *
 * MIT/X11 License
 * Copyright (c)  2013-2014 Qball  Cow <qball@gmpclient.org>
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
#include <X11/X.h>
#include <X11/Xlib.h>
#include <sys/socket.h>
#include <linux/un.h>

#include "rofi.h"
#include "x11-helper.h"
#include "rofi-i3.h"

#ifdef HAVE_I3_IPC_H
#include <i3/ipc.h>
// Path to HAVE_I3_IPC_H socket.
char *i3_socket_path = NULL;

void i3_support_focus_window ( Window id )
{
    i3_ipc_header_t    head;
    char               command[128];
    int                s, len;
    ssize_t            t;
    struct sockaddr_un remote;

    if ( strlen ( i3_socket_path ) > UNIX_PATH_MAX ) {
        fprintf ( stderr, "Socket path is to long. %zd > %d\n", strlen ( i3_socket_path ), UNIX_PATH_MAX );
        return;
    }

    if ( ( s = socket ( AF_UNIX, SOCK_STREAM, 0 ) ) == -1 ) {
        fprintf ( stderr, "Failed to open connection to I3: %s\n", strerror ( errno ) );
        return;
    }

    remote.sun_family = AF_UNIX;
    strcpy ( remote.sun_path, i3_socket_path );
    len = strlen ( remote.sun_path ) + sizeof ( remote.sun_family );

    if ( connect ( s, ( struct sockaddr * ) &remote, len ) == -1 ) {
        fprintf ( stderr, "Failed to connect to I3 (%s): %s\n", i3_socket_path, strerror ( errno ) );
        close ( s );
        return;
    }


    // Formulate command
    snprintf ( command, 128, "[id=\"%lu\"] focus", id );
    // Prepare header.
    memcpy ( head.magic, I3_IPC_MAGIC, 6 );
    head.size = strlen ( command );
    head.type = I3_IPC_MESSAGE_TYPE_COMMAND;
    // Send header.
    t = send ( s, &head, sizeof ( i3_ipc_header_t ), 0 );
    if ( t == -1 ) {
        char *msg = g_strdup_printf ( "Failed to send message header to i3: %s\n", strerror ( errno ) );
        error_dialog ( msg );
        g_free ( msg );
        close ( s );
        return;
    }
    // Send message
    t = send ( s, command, strlen ( command ), 0 );
    if ( t == -1 ) {
        char *msg = g_strdup_printf ( "Failed to send message body to i3: %s\n", strerror ( errno ) );
        error_dialog ( msg );
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

int i3_support_initialize ( Display *display )
{
    Atom i3_sp_atom = XInternAtom ( display, "I3_SOCKET_PATH", False );

    if ( i3_sp_atom != None ) {
        // Get the default screen.
        Screen *screen = DefaultScreenOfDisplay ( display );
        // Find the root window (each X has one.).
        Window root = RootWindow ( display, XScreenNumberOfScreen ( screen ) );
        // Get the i3 path property.
        i3_socket_path = window_get_text_prop ( display, root, i3_sp_atom );
    }
    // If we find it, go into i3 mode.
    return ( i3_socket_path != NULL ) ? TRUE : FALSE;
}

void i3_support_free_internals ( void )
{
    if ( i3_socket_path != NULL ) {
        g_free ( i3_socket_path );
    }
}

#else


void i3_support_focus_window ( Window id )
{
    fprintf ( stderr, "Trying to control i3, when i3 support is not enabled.\n" );
    abort ();
}
void i3_support_free_internals ( void )
{
}

int i3_support_initialize ( Display *display )
{
    return FALSE;
}
#endif // HAVE_I3_IPC_H
