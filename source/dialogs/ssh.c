/**
 * rofi
 *
 * MIT/X11 License
 * Copyright 2013-2015 Qball Cow <qball@gmpclient.org>
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
#include <X11/X.h>

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
#include "history.h"
#include "dialogs/ssh.h"

#define SSH_CACHE_FILE    "rofi-2.sshcache"

// Used in get_ssh() when splitting lines from the user's
// SSH config file into tokens.
#define SSH_TOKEN_DELIM    "= \t\r\n"

static inline int execshssh ( const char *host )
{
    char **args = NULL;
    int  argsv  = 0;
    helper_parse_setup ( config.ssh_command, &args, &argsv, "{host}", host, NULL );

    GError *error = NULL;
    g_spawn_async ( NULL, args, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error );

    if ( error != NULL ) {
        char *msg = g_strdup_printf ( "Failed to execute: 'ssh %s'\nError: '%s'", host, error->message );
        error_dialog ( msg, FALSE  );
        g_free ( msg );
        // print error.
        g_error_free ( error );
    }
    // Free the args list.
    g_strfreev ( args );

    return 0;
}
// execute sub-process
static pid_t exec_ssh ( const char *cmd )
{
    if ( !cmd || !cmd[0] ) {
        return -1;
    }

    execshssh ( cmd );

    /**
     * This happens in non-critical time (After launching app)
     * It is allowed to be a bit slower.
     */
    char *path = g_strdup_printf ( "%s/%s", cache_dir, SSH_CACHE_FILE );
    history_set ( path, cmd );
    g_free ( path );

    return 0;
}
static void delete_ssh ( const char *cmd )
{
    if ( !cmd || !cmd[0] ) {
        return;
    }
    char *path = NULL;
    path = g_strdup_printf ( "%s/%s", cache_dir, SSH_CACHE_FILE );
    history_remove ( path, cmd );
    g_free ( path );
}
static int ssh_sort_func ( const void *a, const void *b, void *data __attribute__( ( unused ) ) )
{
    const char *astr = *( const char * const * ) a;
    const char *bstr = *( const char * const * ) b;
    return g_utf8_collate ( astr, bstr );
}

/**
 * @param retv list of hosts
 * @pwaram length pointer to length of list
 *
 * Read 'known_hosts' file when entries are not hashsed.
 *
 * @returns list of hosts.
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
 * Read /etc/hosts
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

static char ** get_ssh (  unsigned int *length )
{
    char         **retv        = NULL;
    unsigned int num_favorites = 0;
    char         *path;

    if ( getenv ( "HOME" ) == NULL ) {
        return NULL;
    }

    path = g_strdup_printf ( "%s/%s", cache_dir, SSH_CACHE_FILE );
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
    path = g_strdup_printf ( "%s/%s", hd, ".ssh/config" );
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

    // TODO: check this is still fast enough. (takes 1ms on laptop.)
    if ( ( *length ) > num_favorites ) {
        g_qsort_with_data ( &retv[num_favorites], ( *length ) - num_favorites, sizeof ( char* ), ssh_sort_func, NULL );
    }
    g_free ( path );

    return retv;
}

typedef struct _SSHModePrivateData
{
    unsigned int id;
    char         **cmd_list;
    unsigned int cmd_list_length;
} SSHModePrivateData;

static void ssh_mode_init ( Switcher *sw )
{
    if ( sw->private_data == NULL ) {
        SSHModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
        sw->private_data = (void *) pd;
        pd->cmd_list     = get_ssh ( &( pd->cmd_list_length ) );
    }
}

static unsigned int ssh_mode_get_num_entries ( const Switcher *sw )
{
    const SSHModePrivateData *rmpd = (const SSHModePrivateData *) sw->private_data;
    return rmpd->cmd_list_length;
}
static SwitcherMode ssh_mode_result ( Switcher *sw, int mretv, char **input, unsigned int selected_line )
{
    SwitcherMode       retv  = MODE_EXIT;
    SSHModePrivateData *rmpd = (SSHModePrivateData *) sw->private_data;
    if ( mretv & MENU_NEXT ) {
        retv = NEXT_DIALOG;
    }
    else if ( mretv & MENU_PREVIOUS ) {
        retv = PREVIOUS_DIALOG;
    }
    else if ( mretv & MENU_QUICK_SWITCH ) {
        retv = ( mretv & MENU_LOWER_MASK );
    }
    else if ( ( mretv & MENU_OK ) && rmpd->cmd_list[selected_line] != NULL ) {
        exec_ssh ( rmpd->cmd_list[selected_line] );
    }
    else if ( ( mretv & MENU_CUSTOM_INPUT ) && *input != NULL && *input[0] != '\0' ) {
        exec_ssh ( *input );
    }
    else if ( ( mretv & MENU_ENTRY_DELETE ) && rmpd->cmd_list[selected_line] ) {
        delete_ssh ( rmpd->cmd_list[selected_line] );
        g_strfreev ( rmpd->cmd_list );
        rmpd->cmd_list_length = 0;
        rmpd->cmd_list        = NULL;
        // Stay
        retv = RELOAD_DIALOG;
    }
    return retv;
}

static void ssh_mode_destroy ( Switcher *sw )
{
    SSHModePrivateData *rmpd = (SSHModePrivateData *) sw->private_data;
    if ( rmpd != NULL ) {
        g_strfreev ( rmpd->cmd_list );
        g_free ( rmpd );
        sw->private_data = NULL;
    }
}

static char *mgrv ( unsigned int selected_line, const Switcher *sw, G_GNUC_UNUSED int *state, int get_entry )
{
    SSHModePrivateData *rmpd = (SSHModePrivateData *) sw->private_data;
    return get_entry ? g_strdup ( rmpd->cmd_list[selected_line] ) : NULL;
}
static int ssh_token_match ( const Switcher *sw, char **tokens, int not_ascii, int case_sensitive, unsigned int index )
{
    SSHModePrivateData *rmpd = (SSHModePrivateData *) sw->private_data;
    return token_match ( tokens, rmpd->cmd_list[index], not_ascii, case_sensitive );
}

static int ssh_is_not_ascii ( const Switcher *sw, unsigned int index )
{
    SSHModePrivateData *rmpd = (SSHModePrivateData *) sw->private_data;
    return is_not_ascii ( rmpd->cmd_list[index] );
}

Switcher ssh_mode =
{
    .name            = "ssh",
    .keycfg          = NULL,
    .keystr          = NULL,
    .modmask         = AnyModifier,
    .init            = ssh_mode_init,
    .get_num_entries = ssh_mode_get_num_entries,
    .result          = ssh_mode_result,
    .destroy         = ssh_mode_destroy,
    .token_match     = ssh_token_match,
    .mgrv            = mgrv,
    .is_not_ascii    = ssh_is_not_ascii,
    .private_data    = NULL,
    .free            = NULL
};
