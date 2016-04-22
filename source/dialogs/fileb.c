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
 * \ingroup FILEBMode
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
#include "dialogs/fileb.h"
static void exec_fileb ( const char *cmd )
{
    if ( !cmd || !cmd[0] ) {
        return;
    }
    int  retv   = TRUE;
    char **args = NULL;
    int  argc   = 0;
    char *cmdf  = rofi_expand_path ( cmd );
    helper_parse_setup ( "xdg-open \"{cmd}\"", &args, &argc, "{cmd}", cmdf, NULL );
    g_free ( cmdf );
    GError *error = NULL;
    g_spawn_async ( NULL, args, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error );
    if ( error != NULL ) {
        char *msg = g_strdup_printf ( "Failed to execute: '%s'\nError: '%s'", cmd, error->message );
        rofi_view_error_dialog ( msg, FALSE  );
        g_free ( msg );
        // print error.
        g_error_free ( error );
        retv = FALSE;
    }

    // Free the args list.
    g_strfreev ( args );
}

static char ** get_fileb (  unsigned int *length, char *ipath )
{
    char **retv = NULL;

    if ( getenv ( "HOME" ) == NULL ) {
        return NULL;
    }
    GError *error = NULL;
    printf ( "look in: %s\n", ipath );
    gchar  *fpath = rofi_expand_path ( ipath );
    DIR    *dir   = opendir ( fpath );

    if ( dir != NULL ) {
        struct dirent *dent;
        gsize         dirn_len = 0;
        gchar         *dirn    = g_locale_to_utf8 ( fpath, -1, NULL, &dirn_len, &error );
        if ( error != NULL ) {
            fprintf ( stderr, "Failed to convert directory name to UTF-8: %s\n", error->message );
            g_clear_error ( &error );
            g_free ( fpath );
            return retv;
        }
        g_free ( dirn );

        while ( ( dent = readdir ( dir ) ) != NULL ) {
            if ( dent->d_type != DT_REG && dent->d_type != DT_DIR && dent->d_type != DT_LNK && dent->d_type != DT_UNKNOWN ) {
                continue;
            }
            // Skip dot files.
            if ( dent->d_name[0] == '.' ) {
                continue;
            }

            gsize name_len;
            gchar *fp = g_build_filename ( ipath, dent->d_name, NULL );
            if ( dent->d_type == DT_DIR ) {
                char *a = g_strdup_printf ( "%s/", fp );
                g_free ( fp );
                fp = a;
            }
            gchar *name = g_filename_to_utf8 ( fp, -1, NULL, &name_len, &error );
            g_free ( fp );
            if ( error != NULL ) {
                fprintf ( stderr, "Failed to convert filename to UTF-8: %s\n", error->message );
                g_clear_error ( &error );
                g_free ( name );
                continue;
            }

            retv                  = g_realloc ( retv, ( ( *length ) + 2 ) * sizeof ( char* ) );
            retv[( *length )]     = name;
            retv[( *length ) + 1] = NULL;
            ( *length )++;
        }

        closedir ( dir );
    }

    g_free ( fpath );
    return retv;
}

/**
 * The internal data structure holding the private data of the SSH Mode.
 */
typedef struct
{
    /** List if available fileb file.*/
    char         **file_list;
    /** Length of the #file_list.*/
    unsigned int file_list_length;
    char         *fpath;
} FILEBModePrivateData;

static int _update_result ( Mode *sw, const char *input, unsigned int selected )
{
    FILEBModePrivateData *pd = (FILEBModePrivateData *) mode_get_private_data ( sw );
    printf ( "update result\n" );

    if ( input == NULL || strlen ( input ) == 0 ) {
        printf ( "clear\n" );
        if ( pd->file_list_length > 0 ) {
            g_strfreev ( pd->file_list );
            pd->file_list        = NULL;
            pd->file_list_length = 0;
            return TRUE;
        }
    }
    if ( selected == UINT32_MAX ) {
        g_strfreev ( pd->file_list );
        pd->file_list        = NULL;
        pd->file_list_length = 0;
        char *p = g_strdup ( input );
        g_free ( pd->fpath );
        pd->fpath     = p;
        pd->file_list = get_fileb ( &( pd->file_list_length ), pd->fpath );
        return TRUE;
        return FALSE;
    }
    if ( g_strcmp0 ( input, pd->file_list[selected] ) == 0 ) {
        printf ( "Match\n" );
        g_strfreev ( pd->file_list );
        pd->file_list        = NULL;
        pd->file_list_length = 0;
        char *p = g_strdup ( input );
        g_free ( pd->fpath );
        pd->fpath     = p;
        pd->file_list = get_fileb ( &( pd->file_list_length ), pd->fpath );
        return TRUE;
    }
    else if ( ( strlen ( input ) + 1 ) < strlen ( pd->fpath ) ) {
        printf ( "Match\n" );
        g_strfreev ( pd->file_list );
        pd->file_list        = NULL;
        pd->file_list_length = 0;
        char *p;
        if ( strlen ( input ) == 0 ) {
            p = g_strdup ( "~" );
        }
        else{
            p = g_path_get_dirname ( input );
        }
        g_free ( pd->fpath );
        pd->fpath     = p;
        pd->file_list = get_fileb ( &( pd->file_list_length ), pd->fpath );
        return TRUE;
    }
    return FALSE;
}
/**
 * @param sw Object handle to the SSH Mode object
 *
 * Initializes the SSH Mode private data object and
 * loads the relevant fileb information.
 */
static int fileb_mode_init ( Mode *sw )
{
    if ( mode_get_private_data ( sw ) == NULL ) {
        FILEBModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
        mode_set_private_data ( sw, (void *) pd );
        pd->fpath     = g_strdup ( "/" );
        pd->file_list = get_fileb ( &( pd->file_list_length ), pd->fpath );
    }
    return TRUE;
}

/**
 * @param sw Object handle to the SSH Mode object
 *
 * Get the number of SSH entries.
 *
 * @returns the number of fileb entries.
 */
static unsigned int fileb_mode_get_num_entries ( const Mode *sw )
{
    const FILEBModePrivateData *rmpd = (const FILEBModePrivateData *) mode_get_private_data ( sw );
    return rmpd->file_list_length;
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
static ModeMode fileb_mode_result ( Mode *sw, int mretv, char **input, unsigned int selected_line )
{
    ModeMode             retv  = MODE_EXIT;
    FILEBModePrivateData *rmpd = (FILEBModePrivateData *) mode_get_private_data ( sw );
    if ( mretv & MENU_NEXT ) {
        retv = NEXT_DIALOG;
    }
    else if ( mretv & MENU_PREVIOUS ) {
        retv = PREVIOUS_DIALOG;
    }
    else if ( mretv & MENU_QUICK_SWITCH ) {
        retv = ( mretv & MENU_LOWER_MASK );
    }
    else if ( ( mretv & MENU_OK ) && rmpd->file_list[selected_line] != NULL ) {
        exec_fileb ( rmpd->file_list[selected_line] );
    }
    else if ( ( mretv & MENU_CUSTOM_INPUT ) && *input != NULL && *input[0] != '\0' ) {
        exec_fileb ( *input );
    }
    return retv;
}

/**
 * @param sw Object handle to the SSH Mode object
 *
 * Cleanup the SSH Mode. Free all allocated memory and NULL the private data pointer.
 */
static void fileb_mode_destroy ( Mode *sw )
{
    FILEBModePrivateData *rmpd = (FILEBModePrivateData *) mode_get_private_data ( sw );
    if ( rmpd != NULL ) {
        g_strfreev ( rmpd->file_list );
        g_free ( rmpd->fpath );
        g_free ( rmpd );
        mode_set_private_data ( sw, NULL );
    }
}

static char *_get_display_value ( const Mode *sw, unsigned int selected_line, G_GNUC_UNUSED int *state, int get_entry )
{
    FILEBModePrivateData *rmpd = (FILEBModePrivateData *) mode_get_private_data ( sw );
    return get_entry ? g_path_get_basename ( rmpd->file_list[selected_line] ) : NULL;
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
static int fileb_token_match ( const Mode *sw, char **tokens, int not_ascii, int case_sensitive, unsigned int index )
{
    FILEBModePrivateData *rmpd = (FILEBModePrivateData *) mode_get_private_data ( sw );
    return token_match ( tokens, rmpd->file_list[index], not_ascii, case_sensitive );
}

/**
 * @param sw Object handle to the SSH Mode object
 * @param index The index of the entry to match
 *
 * Check if the selected entry contains non-ascii symbols.
 *
 * @returns TRUE if string contains non-ascii symbols
 */
static int fileb_is_not_ascii ( const Mode *sw, unsigned int index )
{
    FILEBModePrivateData *rmpd = (FILEBModePrivateData *) mode_get_private_data ( sw );
    return !g_str_is_ascii ( rmpd->file_list[index] );
}
static char *fileb_get_completion ( const Mode *sw, unsigned int index )
{
    FILEBModePrivateData *rmpd = (FILEBModePrivateData *) mode_get_private_data ( sw );
    return g_strdup ( rmpd->file_list[index] );
}

#include "mode-private.h"
Mode fileb_mode =
{
    .name               = "fileb",
    .cfg_name_key       = "display-fileb",
    ._init              = fileb_mode_init,
    ._get_num_entries   = fileb_mode_get_num_entries,
    ._result            = fileb_mode_result,
    ._destroy           = fileb_mode_destroy,
    ._token_match       = fileb_token_match,
    ._get_display_value = _get_display_value,
    ._update_result     = _update_result,
    ._get_completion    = fileb_get_completion,
    ._is_not_ascii      = fileb_is_not_ascii,
    .private_data       = NULL,
    .free               = NULL
};
/*@}*/
