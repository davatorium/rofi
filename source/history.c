/**
 * rofi
 *
 * MIT/X11 License
 * Copyright 2013-2017 Qball Cow <qball@gmpclient.org>
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "rofi.h"
#include "history.h"
#include "settings.h"

#define HISTORY_MAX_ENTRIES    25

/**
 * History element
 */
typedef struct __element
{
    /** Index in history */
    long int index;
    /** Entry */
    char     *name;
}_element;

static int __element_sort_func ( const void *ea, const void *eb, void *data __attribute__( ( unused ) ) )
{
    _element *a = *(_element * *) ea;
    _element *b = *(_element * *) eb;
    return b->index - a->index;
}

static void __history_write_element_list ( FILE *fd, _element **list, unsigned int length )
{
    if ( list == NULL || length == 0 ) {
        return;
    }
    // Sort the list before writing out.
    g_qsort_with_data ( list, length, sizeof ( _element* ), __element_sort_func, NULL );

    // Get minimum index.
    int min_value = list[length - 1]->index;

    // Set the max length of the list.
    length = ( length > HISTORY_MAX_ENTRIES ) ? HISTORY_MAX_ENTRIES : length;

    // Write out entries.
    for ( unsigned int iter = 0; iter < length; iter++ ) {
        fprintf ( fd, "%ld %s\n", list[iter]->index - min_value, list[iter]->name );
    }
}

static _element ** __history_get_element_list ( FILE *fd, unsigned int *length )
{
    _element **retv = NULL;

    if  ( length == NULL ) {
        return NULL;
    }
    *length = 0;

    if ( fd == NULL ) {
        return NULL;
    }
    char    *buffer       = NULL;
    size_t  buffer_length = 0;
    ssize_t l             = 0;
    while ( ( l = getline ( &buffer, &buffer_length, fd ) ) > 0 ) {
        char * start = NULL;
        // Skip empty lines.
        if ( l <= 1 ) {
            continue;
        }

        long int index = strtol ( buffer, &start, 10 );
        if ( start == buffer || *start == '\0' ) {
            continue;
        }
        start++;
        if ( ( l - ( start - buffer ) ) < 2 ) {
            continue;
        }
        // Resize and check.
        retv = g_realloc ( retv, ( *length + 2 ) * sizeof ( _element* ) );

        retv[( *length )] = g_malloc ( sizeof ( _element ) );

        // remove trailing \n
        buffer[l - 1] = '\0';
        // Parse the number of times.
        retv[( *length )]->index = index;
        retv[( *length )]->name  = g_strndup ( start, l - 1 - ( start - buffer ) );
        // Force trailing '\0'
        retv[( *length ) + 1] = NULL;

        ( *length )++;
    }
    if ( buffer != NULL  ) {
        free ( buffer );
        buffer = NULL;
    }
    return retv;
}

void history_set ( const char *filename, const char *entry )
{
    if ( config.disable_history ) {
        return;
    }
    int          found  = 0;
    unsigned int curr   = 0;
    unsigned int length = 0;
    _element     **list = NULL;
    // Open file for reading and writing.
    FILE         *fd = g_fopen ( filename, "r" );
    if ( fd != NULL ) {
        // Get list.
        list = __history_get_element_list ( fd, &length );
        // Close file, if fails let user know on stderr.
        if ( fclose ( fd ) != 0 ) {
            fprintf ( stderr, "Failed to close history file: %s\n", strerror ( errno ) );
        }
    }
    // Look if the entry exists.
    for ( unsigned int iter = 0; !found && iter < length; iter++ ) {
        if ( strcmp ( list[iter]->name, entry ) == 0 ) {
            curr  = iter;
            found = 1;
        }
    }

    if ( found ) {
        // If exists, increment list index number
        list[curr]->index++;
    }
    else{
        // If not exists, add it.
        // Increase list by one
        list         = g_realloc ( list, ( length + 2 ) * sizeof ( _element* ) );
        list[length] = g_malloc ( sizeof ( _element ) );
        // Copy name
        if ( list[length] != NULL ) {
            list[length]->name = g_strdup ( entry );
            // set # hits
            list[length]->index = 1;

            length++;
            list[length] = NULL;
        }
    }

    fd = fopen ( filename, "w" );
    if ( fd == NULL ) {
        fprintf ( stderr, "Failed to open file: %s\n", strerror ( errno ) );
    }
    else {
        // Write list.
        __history_write_element_list ( fd, list, length );
        // Close file, if fails let user know on stderr.
        if ( fclose ( fd ) != 0 ) {
            fprintf ( stderr, "Failed to close history file: %s\n", strerror ( errno ) );
        }
    }
    // Free the list.
    for ( unsigned int iter = 0; iter < length; iter++ ) {
        g_free ( list[iter]->name );
        g_free ( list[iter] );
    }
    g_free ( list );
}

void history_remove ( const char *filename, const char *entry )
{
    if ( config.disable_history ) {
        return;
    }
    _element     ** list = NULL;
    int          found   = 0;
    unsigned int curr    = 0;
    unsigned int length  = 0;
    // Open file for reading and writing.
    FILE         *fd = g_fopen ( filename, "r" );
    if ( fd == NULL ) {
        fprintf ( stderr, "Failed to open file: %s\n", strerror ( errno ) );
        return;
    }
    // Get list.
    list = __history_get_element_list ( fd, &length );

    // Close file, if fails let user know on stderr.
    if ( fclose ( fd ) != 0 ) {
        fprintf ( stderr, "Failed to close history file: %s\n", strerror ( errno ) );
    }
    // Find entry.
    for ( unsigned int iter = 0; !found && iter < length; iter++ ) {
        if ( strcmp ( list[iter]->name, entry ) == 0 ) {
            curr  = iter;
            found = 1;
        }
    }

    // If found, remove it and write out new file.
    if ( found ) {
        // Remove the entry.
        g_free ( list[curr]->name );
        g_free ( list[curr] );
        // Swap last to here (if list is size 1, we just swap empty sets).
        list[curr] = list[length - 1];
        // Empty last.
        list[length - 1] = NULL;
        length--;

        fd = g_fopen ( filename, "w" );
        // Clear list.
        if ( fd != NULL ) {
            // Write list.
            __history_write_element_list ( fd, list, length );
            // Close file, if fails let user know on stderr.
            if ( fclose ( fd ) != 0 ) {
                fprintf ( stderr, "Failed to close history file: %s\n", strerror ( errno ) );
            }
        }
        else{
            fprintf ( stderr, "Failed to open file: %s\n", strerror ( errno ) );
        }
    }

    // Free the list.
    for ( unsigned int iter = 0; iter < length; iter++ ) {
        g_free ( list[iter]->name );
        g_free ( list[iter] );
    }
    if ( list != NULL ) {
        g_free ( list );
    }
}

char ** history_get_list ( const char *filename, unsigned int *length )
{
    *length = 0;

    if ( config.disable_history ) {
        return NULL;
    }
    _element **list = NULL;
    char     **retv = NULL;
    // Open file.
    FILE     *fd = g_fopen ( filename, "r" );
    if ( fd == NULL ) {
        // File that does not exists is not an error, so ignore it.
        // Everything else? panic.
        if ( errno != ENOENT ) {
            fprintf ( stderr, "Failed to open file: %s\n", strerror ( errno ) );
        }
        return NULL;
    }
    // Get list.
    list = __history_get_element_list ( fd, length );

    // Copy list in right format.
    // Lists are always short, so performance should not be an issue.
    if ( ( *length ) > 0 ) {
        retv = g_malloc ( ( ( *length ) + 1 ) * sizeof ( char * ) );
        for ( unsigned int iter = 0; iter < ( *length ); iter++ ) {
            retv[iter] = ( list[iter]->name );
            g_free ( list[iter] );
        }
        retv[( *length )] = NULL;
        g_free ( list );
    }

    // Close file, if fails let user know on stderr.
    if ( fclose ( fd ) != 0 ) {
        fprintf ( stderr, "Failed to close history file: %s\n", strerror ( errno ) );
    }
    return retv;
}
