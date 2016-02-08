/*
 * rofi
 *
 * MIT/X11 License
 * Copyright 2013-2016 Qball Cow <qball@gmpclient.org>
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

/**
 * \ingroup SSHMode
 * @{
 */
#include <config.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <helper.h>

#include "rofi.h"
#include "settings.h"
#include "history.h"
#include "dialogs/ssh.h"

/**
 * Name of the history file where previously choosen hosts are stored.
 */
#define SSH_CACHE_FILE     "rofi-2.sshcache"

/**
 * Used in get_ssh() when splitting lines from the user's
 * SSH config file into tokens.
 */
#define SSH_TOKEN_DELIM    "= \t\r\n"

/**
 * @param host The host to connect too
 *
 * SSH into the selected host.
 *
 * @returns FALSE On failure, TRUE on success
 */
static inline int execshssh ( const char *host )
{
    char **args = NULL;
    int  argsv  = 0;
    helper_parse_setup ( config.ssh_command, &args, &argsv, "{host}", host, NULL );

    GError *error = NULL;
    g_spawn_async ( NULL, args, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error );

    if ( error != NULL ) {
        char *msg = g_strdup_printf ( "Failed to execute: 'ssh %s'\nError: '%s'", host, error->message );
        rofi_view_error_dialog ( msg, FALSE  );
        g_free ( msg );
        // print error.
        g_error_free ( error );

        g_strfreev ( args );
        return FALSE;
    }
    // Free the args list.
    g_strfreev ( args );

    return TRUE;
}

/**
 * @param host The host to connect too
 *
 * SSH into the selected host, if successful update history.
 */
static void exec_ssh ( const char *host )
{
    if ( !host || !host[0] ) {
        return;
    }

    if ( !execshssh ( host ) ) {
        return;
    }

    //  This happens in non-critical time (After launching app)
    //  It is allowed to be a bit slower.
    char *path = g_build_filename ( cache_dir, SSH_CACHE_FILE, NULL );
    history_set ( path, host );
    g_free ( path );
}

/**
 * @param host The host to remove from history
 *
 * Remove host from history.
 */
static void delete_ssh ( const char *host )
{
    if ( !host || !host[0] ) {
        return;
    }
    char *path = g_build_filename ( cache_dir, SSH_CACHE_FILE, NULL );
    history_remove ( path, host );
    g_free ( path );
}

/**
 * @param retv list of hosts
 * @param length pointer to length of list [in][out]
 *
 * Read 'known_hosts' file when entries are not hashsed.
 *
 * @returns updated list of hosts.
 */
static char **read_known_hosts_file ( char ** retv, unsigned int *length )
{
    char *path = g_build_filename ( g_getenv ( "HOME" ), ".ssh", "known_hosts", NULL );
    FILE *fd   = fopen ( path, "r" );
    if ( fd != NULL ) {
        char buffer[1024];
        // Reading one line per time.
        while ( fgets ( buffer, sizeof ( buffer ), fd ) ) {
            char *sep = strstr ( buffer, "," );

            if ( sep != NULL ) {
                *sep = '\0';
                // Is this host name already in the list?
                // We often get duplicates in hosts file, so lets check this.
                int found = 0;
                for ( unsigned int j = 0; j < ( *length ); j++ ) {
                    if ( !g_ascii_strcasecmp ( buffer, retv[j] ) ) {
                        found = 1;
                        break;
                    }
                }

                if ( !found ) {
                    // Add this host name to the list.
                    retv                  = g_realloc ( retv, ( ( *length ) + 2 ) * sizeof ( char* ) );
                    retv[( *length )]     = g_strdup ( buffer );
                    retv[( *length ) + 1] = NULL;
                    ( *length )++;
                }
            }
        }
        if ( fclose ( fd ) != 0 ) {
            fprintf ( stderr, "Failed to close hosts file: '%s'\n", strerror ( errno ) );
        }
    }

    g_free ( path );
    return retv;
}

/**
 * @param retv The list of hosts to update.
 * @param length The length of the list retv [in][out]
 *
 * Read `/etc/hosts` and appends them to the list retv
 *
 * @returns an updated list with the added hosts.
 */
static char **read_hosts_file ( char ** retv, unsigned int *length )
{
    // Read the hosts file.
    FILE *fd = fopen ( "/etc/hosts", "r" );
    if ( fd != NULL ) {
        char buffer[1024];
        // Reading one line per time.
        while ( fgets ( buffer, sizeof ( buffer ), fd ) ) {
            // Evaluate one line.
            unsigned int index  = 0, ti = 0;
            char         *token = buffer;

            // Tokenize it.
            do {
                char c = buffer[index];
                // Break on space, tab, newline and \0.
                if ( c == ' ' || c == '\t' || c == '\n' || c == '\0' || c == '#' ) {
                    buffer[index] = '\0';
                    // Ignore empty tokens
                    if ( token[0] != '\0' ) {
                        ti++;
                        // and first token.
                        if ( ti > 1 ) {
                            // Is this host name already in the list?
                            // We often get duplicates in hosts file, so lets check this.
                            int found = 0;
                            for ( unsigned int j = 0; j < ( *length ); j++ ) {
                                if ( !g_ascii_strcasecmp ( token, retv[j] ) ) {
                                    found = 1;
                                    break;
                                }
                            }

                            if ( !found ) {
                                // Add this host name to the list.
                                retv = g_realloc ( retv,
                                                   ( ( *length ) + 2 ) * sizeof ( char* ) );
                                retv[( *length )]     = g_strdup ( token );
                                retv[( *length ) + 1] = NULL;
                                ( *length )++;
                            }
                        }
                    }
                    // Set start to next element.
                    token = &buffer[index + 1];
                    // Everything after comment ignore.
                    if ( c == '#' ) {
                        break;
                    }
                }
                // Skip to the next entry.
                index++;
            } while ( buffer[index] != '\0' && buffer[index] != '#' );
        }
        if ( fclose ( fd ) != 0 ) {
            fprintf ( stderr, "Failed to close hosts file: '%s'\n", strerror ( errno ) );
        }
    }

    return retv;
}

/**
 * @param length The number of found ssh hosts [out]
 *
 * Gets the list available SSH hosts.
 *
 * @return an array of strings containing all the hosts.
 */
static char ** get_ssh (  unsigned int *length )
{
    char         **retv        = NULL;
    unsigned int num_favorites = 0;
    char         *path;

    if ( getenv ( "HOME" ) == NULL ) {
        return NULL;
    }

    path = g_build_filename ( cache_dir, SSH_CACHE_FILE, NULL );
    retv = history_get_list ( path, length );
    g_free ( path );
    num_favorites = ( *length );

    if ( config.parse_known_hosts == TRUE ) {
        retv = read_known_hosts_file ( retv, length );
    }
    if ( config.parse_hosts == TRUE ) {
        retv = read_hosts_file ( retv, length );
    }

    FILE       *fd = NULL;
    const char *hd = getenv ( "HOME" );
    path = g_build_filename ( hd, ".ssh", "config", NULL );
    fd   = fopen ( path, "r" );

    if ( fd != NULL ) {
        char buffer[1024];
        while ( fgets ( buffer, sizeof ( buffer ), fd ) ) {
            // Each line is either empty, a comment line starting with a '#'
            // character or of the form "keyword [=] arguments", where there may
            // be multiple (possibly quoted) arguments separated by whitespace.
            // The keyword is separated from its arguments by whitespace OR by
            // optional whitespace and a '=' character.
            char *token = strtok ( buffer, SSH_TOKEN_DELIM );

            // Skip empty lines and comment lines. Also skip lines where the
            // keyword is not "Host".
            if ( !token || *token == '#' || g_ascii_strcasecmp ( token, "Host" ) ) {
                continue;
            }

            // Now we know that this is a "Host" line.
            // The "Host" keyword is followed by one more host names separated
            // by whitespace; while host names may be quoted with double quotes
            // to represent host names containing spaces, we don't support this
            // (how many host names contain spaces?).
            while ( ( token = strtok ( NULL, SSH_TOKEN_DELIM ) ) ) {
                // We do not want to show wildcard entries, as you cannot ssh to them.
                if ( *token == '!' || strpbrk ( token, "*?" ) ) {
                    continue;
                }

                // If comment, skip from now on.
                if ( *token == '#' ) {
                    break;
                }

                // Is this host name already in the history file?
                // This is a nice little penalty, but doable? time will tell.
                // given num_favorites is max 25.
                int found = 0;
                for ( unsigned int j = 0; j < num_favorites; j++ ) {
                    if ( !g_ascii_strcasecmp ( token, retv[j] ) ) {
                        found = 1;
                        break;
                    }
                }

                if ( found ) {
                    continue;
                }

                // Add this host name to the list.
                retv                  = g_realloc ( retv, ( ( *length ) + 2 ) * sizeof ( char* ) );
                retv[( *length )]     = g_strdup ( token );
                retv[( *length ) + 1] = NULL;
                ( *length )++;
            }
        }

        if ( fclose ( fd ) != 0 ) {
            fprintf ( stderr, "Failed to close ssh configuration file: '%s'\n", strerror ( errno ) );
        }
    }

    g_free ( path );

    return retv;
}

/**
 * The internal data structure holding the private data of the SSH Mode.
 */
typedef struct _SSHModePrivateData
{
    /** List if available ssh hosts.*/
    char         **hosts_list;
    /** Length of the #hosts_list.*/
    unsigned int hosts_list_length;
} SSHModePrivateData;

/**
 * @param sw Object handle to the SSH Mode object
 *
 * Initializes the SSH Mode private data object and
 * loads the relevant ssh information.
 */
static int ssh_mode_init ( Mode *sw )
{
    if ( mode_get_private_data ( sw ) == NULL ) {
        SSHModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
        mode_set_private_data ( sw, (void *) pd );
        pd->hosts_list = get_ssh ( &( pd->hosts_list_length ) );
    }
    return TRUE;
}

/**
 * @param sw Object handle to the SSH Mode object
 *
 * Get the number of SSH entries.
 *
 * @returns the number of ssh entries.
 */
static unsigned int ssh_mode_get_num_entries ( const Mode *sw )
{
    const SSHModePrivateData *rmpd = (const SSHModePrivateData *) mode_get_private_data ( sw );
    return rmpd->hosts_list_length;
}

/**
 * @param sw Object handle to the SSH Mode object
 * @param mretv The menu return value.
 * @param input Pointer to the user input string.
 * @param selected_line the line selected by the user.
 *
 * Acts on the user interaction.
 *
 * @returns the next #ModeMode.
 */
static ModeMode ssh_mode_result ( Mode *sw, int mretv, char **input, unsigned int selected_line )
{
    ModeMode           retv  = MODE_EXIT;
    SSHModePrivateData *rmpd = (SSHModePrivateData *) mode_get_private_data ( sw );
    if ( mretv & MENU_NEXT ) {
        retv = NEXT_DIALOG;
    }
    else if ( mretv & MENU_PREVIOUS ) {
        retv = PREVIOUS_DIALOG;
    }
    else if ( mretv & MENU_QUICK_SWITCH ) {
        retv = ( mretv & MENU_LOWER_MASK );
    }
    else if ( ( mretv & MENU_OK ) && rmpd->hosts_list[selected_line] != NULL ) {
        exec_ssh ( rmpd->hosts_list[selected_line] );
    }
    else if ( ( mretv & MENU_CUSTOM_INPUT ) && *input != NULL && *input[0] != '\0' ) {
        exec_ssh ( *input );
    }
    else if ( ( mretv & MENU_ENTRY_DELETE ) && rmpd->hosts_list[selected_line] ) {
        delete_ssh ( rmpd->hosts_list[selected_line] );
        g_strfreev ( rmpd->hosts_list );
        rmpd->hosts_list_length = 0;
        rmpd->hosts_list        = NULL;
        // Stay
        retv = RELOAD_DIALOG;
    }
    return retv;
}

/**
 * @param sw Object handle to the SSH Mode object
 *
 * Cleanup the SSH Mode. Free all allocated memory and NULL the private data pointer.
 */
static void ssh_mode_destroy ( Mode *sw )
{
    SSHModePrivateData *rmpd = (SSHModePrivateData *) mode_get_private_data ( sw );
    if ( rmpd != NULL ) {
        g_strfreev ( rmpd->hosts_list );
        g_free ( rmpd );
        mode_set_private_data ( sw, NULL );
    }
}

/**
 * @param sw Object handle to the SSH Mode object
 * @param selected_line The line to view
 * @param state The state of the entry [out]
 * @param get_entry
 *
 * Gets the string as it should be displayed and the display state.
 * If get_entry is FALSE only the state is set.
 *
 * @return the string as it should be displayed and the display state.
 */
static char *_get_display_value ( const Mode *sw, unsigned int selected_line, G_GNUC_UNUSED int *state, int get_entry )
{
    SSHModePrivateData *rmpd = (SSHModePrivateData *) mode_get_private_data ( sw );
    return get_entry ? g_strdup ( rmpd->hosts_list[selected_line] ) : NULL;
}

/**
 * @param sw Object handle to the SSH Mode object
 * @param tokens The set of tokens to match against
 * @param not_ascii If the entry is pure-ascii
 * @param case_sensitive If the entry should be matched case sensitive
 * @param index The index of the entry to match
 *
 * Match entry against the set of tokens.
 *
 * @returns TRUE if matches
 */
static int ssh_token_match ( const Mode *sw, char **tokens, int not_ascii, int case_sensitive, unsigned int index )
{
    SSHModePrivateData *rmpd = (SSHModePrivateData *) mode_get_private_data ( sw );
    return token_match ( tokens, rmpd->hosts_list[index], not_ascii, case_sensitive );
}

/**
 * @param sw Object handle to the SSH Mode object
 * @param index The index of the entry to match
 *
 * Check if the selected entry contains non-ascii symbols.
 *
 * @returns TRUE if string contains non-ascii symbols
 */
static int ssh_is_not_ascii ( const Mode *sw, unsigned int index )
{
    SSHModePrivateData *rmpd = (SSHModePrivateData *) mode_get_private_data ( sw );
    return !g_str_is_ascii ( rmpd->hosts_list[index] );
}

#include "mode-private.h"
Mode ssh_mode =
{
    .name               = "ssh",
    .cfg_name_key       = "display-ssh",
    .keycfg             = NULL,
    .keystr             = NULL,
    .modmask            = AnyModifier,
    ._init              = ssh_mode_init,
    ._get_num_entries   = ssh_mode_get_num_entries,
    ._result            = ssh_mode_result,
    ._destroy           = ssh_mode_destroy,
    ._token_match       = ssh_token_match,
    ._get_display_value = _get_display_value,
    ._get_completion    = NULL,
    ._is_not_ascii      = ssh_is_not_ascii,
    .private_data       = NULL,
    .free               = NULL
};
/*@}*/
