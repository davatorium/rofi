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

/**
 * \ingroup SSHMode
 * @{
 */

/**
 * Log domain for the ssh modi.
 */
#define G_LOG_DOMAIN    "Dialogs.Ssh"

#include <config.h>
#include <glib.h>
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
#include <glob.h>

#include "rofi.h"
#include "settings.h"
#include "history.h"
#include "dialogs/ssh.h"

/**
 * Name of the history file where previously chosen hosts are stored.
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
static int execshssh ( const char *host )
{
    char **args = NULL;
    int  argsv  = 0;

    helper_parse_setup ( config.ssh_command, &args, &argsv, "{host}", host, (char *) 0 );

    gsize l     = strlen ( "Connecting to '' via rofi" ) + strlen ( host ) + 1;
    gchar *desc = g_newa ( gchar, l );

    g_snprintf ( desc, l, "Connecting to '%s' via rofi", host );

    RofiHelperExecuteContext context = {
        .name        = "ssh",
        .description = desc,
        .command     = "ssh",
    };
    return helper_execute ( NULL, args, "ssh ", host, &context );
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
    char *path = g_build_filename ( g_get_home_dir (), ".ssh", "known_hosts", NULL );
    FILE *fd   = fopen ( path, "r" );
    if ( fd != NULL ) {
        char   *buffer       = NULL;
        size_t buffer_length = 0;
        // Reading one line per time.
        while ( getline ( &buffer, &buffer_length, fd ) > 0 ) {
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
        if ( buffer != NULL ) {
            free ( buffer );
        }
        if ( fclose ( fd ) != 0 ) {
            g_warning ( "Failed to close hosts file: '%s'", g_strerror ( errno ) );
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
        char   *buffer       = NULL;
        size_t buffer_length = 0;
        // Reading one line per time.
        while ( getline ( &buffer, &buffer_length, fd ) > 0 ) {
            // Evaluate one line.
            unsigned int index = 0, ti = 0;
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
        if ( buffer != NULL ) {
            free ( buffer );
        }
        if ( fclose ( fd ) != 0 ) {
            g_warning ( "Failed to close hosts file: '%s'", g_strerror ( errno ) );
        }
    }

    return retv;
}

static void parse_ssh_config_file ( const char *filename, char ***retv, unsigned int *length, unsigned int num_favorites )
{
    FILE *fd = fopen ( filename, "r" );

    g_debug ( "Parsing ssh config file: %s", filename );
    if ( fd != NULL ) {
        char   *buffer         = NULL;
        size_t buffer_length   = 0;
        char   *strtok_pointer = NULL;
        while ( getline ( &buffer, &buffer_length, fd ) > 0 ) {
            // Each line is either empty, a comment line starting with a '#'
            // character or of the form "keyword [=] arguments", where there may
            // be multiple (possibly quoted) arguments separated by whitespace.
            // The keyword is separated from its arguments by whitespace OR by
            // optional whitespace and a '=' character.
            char *token = strtok_r ( buffer, SSH_TOKEN_DELIM, &strtok_pointer );

            // Skip empty lines and comment lines. Also skip lines where the
            // keyword is not "Host".
            if ( !token || *token == '#' ) {
                continue;
            }

            if ( g_strcmp0 ( token, "Include" ) == 0 ) {
                token = strtok_r ( NULL, SSH_TOKEN_DELIM, &strtok_pointer );
                g_debug ( "Found Include: %s", token );
                gchar *path      = rofi_expand_path ( token );
                gchar *full_path = NULL;
                if ( !g_path_is_absolute ( path ) ) {
                    char *dirname = g_path_get_dirname ( filename );
                    full_path = g_build_filename ( dirname, path, NULL );
                    g_free ( dirname );
                }
                else {
                    full_path = g_strdup ( path );
                }
                glob_t globbuf = { .gl_pathc = 0, .gl_pathv = NULL, .gl_offs = 0 };

                if ( glob ( full_path, 0, NULL, &globbuf ) == 0 ) {
                    for ( size_t iter = 0; iter < globbuf.gl_pathc; iter++ ) {
                        parse_ssh_config_file ( globbuf.gl_pathv[iter], retv, length, num_favorites );
                    }
                }
                globfree ( &globbuf );

                g_free ( full_path );
                g_free ( path );
            }
            else if ( g_strcmp0 ( token, "Host" ) == 0 ) {
                // Now we know that this is a "Host" line.
                // The "Host" keyword is followed by one more host names separated
                // by whitespace; while host names may be quoted with double quotes
                // to represent host names containing spaces, we don't support this
                // (how many host names contain spaces?).
                while ( ( token = strtok_r ( NULL, SSH_TOKEN_DELIM, &strtok_pointer ) ) ) {
                    // We do not want to show wildcard entries, as you cannot ssh to them.
                    const char *const sep = "*?";
                    if ( *token == '!' || strpbrk ( token, sep ) ) {
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
                        if ( !g_ascii_strcasecmp ( token, ( *retv )[j] ) ) {
                            found = 1;
                            break;
                        }
                    }

                    if ( found ) {
                        continue;
                    }

                    // Add this host name to the list.
                    ( *retv )                  = g_realloc ( ( *retv ), ( ( *length ) + 2 ) * sizeof ( char* ) );
                    ( *retv )[( *length )]     = g_strdup ( token );
                    ( *retv )[( *length ) + 1] = NULL;
                    ( *length )++;
                }
            }
        }
        if ( buffer != NULL ) {
            free ( buffer );
        }

        if ( fclose ( fd ) != 0 ) {
            g_warning ( "Failed to close ssh configuration file: '%s'", g_strerror ( errno ) );
        }
    }
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

    if ( g_get_home_dir () == NULL ) {
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

    const char *hd = g_get_home_dir ();
    path = g_build_filename ( hd, ".ssh", "config", NULL );

    parse_ssh_config_file ( path, &retv, length, num_favorites );
    g_free ( path );

    return retv;
}

/**
 * The internal data structure holding the private data of the SSH Mode.
 */
typedef struct
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
        // Stay
        retv = RELOAD_DIALOG;
        ssh_mode_destroy ( sw );
        ssh_mode_init ( sw );
    }
    return retv;
}

/**
 * @param sw Object handle to the SSH Mode object
 * @param selected_line The line to view
 * @param state The state of the entry [out]
 * @param attr_list List of extra rendering attributes to set [out]
 * @param get_entry
 *
 * Gets the string as it should be displayed and the display state.
 * If get_entry is FALSE only the state is set.
 *
 * @return the string as it should be displayed and the display state.
 */
static char *_get_display_value ( const Mode *sw, unsigned int selected_line, G_GNUC_UNUSED int *state, G_GNUC_UNUSED GList **attr_list, int get_entry )
{
    SSHModePrivateData *rmpd = (SSHModePrivateData *) mode_get_private_data ( sw );
    return get_entry ? g_strdup ( rmpd->hosts_list[selected_line] ) : NULL;
}

/**
 * @param sw Object handle to the SSH Mode object
 * @param tokens The set of tokens to match against
 * @param index The index of the entry to match
 *
 * Match entry against the set of tokens.
 *
 * @returns TRUE if matches
 */
static int ssh_token_match ( const Mode *sw, rofi_int_matcher **tokens, unsigned int index )
{
    SSHModePrivateData *rmpd = (SSHModePrivateData *) mode_get_private_data ( sw );
    return helper_token_match ( tokens, rmpd->hosts_list[index] );
}
#include "mode-private.h"
Mode ssh_mode =
{
    .name               = "ssh",
    .cfg_name_key       = "display-ssh",
    ._init              = ssh_mode_init,
    ._get_num_entries   = ssh_mode_get_num_entries,
    ._result            = ssh_mode_result,
    ._destroy           = ssh_mode_destroy,
    ._token_match       = ssh_token_match,
    ._get_display_value = _get_display_value,
    ._get_completion    = NULL,
    ._preprocess_input  = NULL,
    .private_data       = NULL,
    .free               = NULL
};
/*@}*/
