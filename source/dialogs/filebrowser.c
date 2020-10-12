/**
 * rofi-file_browser
 *
 * MIT/X11 License
 * Copyright (c) 2017 Qball Cow <qball@gmpclient.org>
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
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <gmodule.h>
#include <gio/gio.h>

#include "mode.h"
#include "helper.h"
#include "mode-private.h"
#include "dialogs/filebrowser.h"

#include <stdint.h>

#include "rofi-icon-fetcher.h"

/**
 * The internal data structure holding the private data of the TEST Mode.
 */
enum FBFileType {
    UP,
    DIRECTORY,
    RFILE,
    NUM_FILE_TYPES,
};
const char *icon_name[NUM_FILE_TYPES] =
{
    "go-up",
    "folder",
    "gtk-file"
};
typedef struct {
    char *name;
    char *path;
    enum FBFileType type;
    uint32_t        icon_fetch_uid;
} FBFile;

typedef struct
{
    GFile           *current_dir;
    FBFile          *array;
    unsigned int    array_length;
} FileBrowserModePrivateData;

static void free_list ( FileBrowserModePrivateData *pd )
{
    for ( unsigned int i = 0; i < pd->array_length; i++ ) {
        FBFile *fb = & ( pd->array[i] );
        g_free ( fb->name );
        g_free ( fb->path );
    }
    g_free (pd->array);
    pd->array  = NULL;
    pd->array_length = 0;
}
#include <sys/types.h>
#include <dirent.h>

static gint compare ( gconstpointer a, gconstpointer b, gpointer data )
{
    FBFile *fa = (FBFile*)a;
    FBFile *fb = (FBFile*)b;
    if ( fa->type != fb->type ){
        return (fa->type - fb->type);
    }

    return g_strcmp0 ( fa->name, fb->name );
}

static void get_file_browser (  Mode *sw )
{
    FileBrowserModePrivateData *pd = (FileBrowserModePrivateData *) mode_get_private_data ( sw );
    /**
     * Get the entries to display.
     * this gets called on plugin initialization.
     */
    char *cdir = g_file_get_path ( pd->current_dir );
    DIR *dir = opendir ( cdir );
    if ( dir ) {
        struct dirent *rd = NULL;
        while ((rd = readdir (dir)) != NULL )
        {
            if ( g_strcmp0 ( rd->d_name, ".." ) == 0 ){
                    pd->array = g_realloc ( pd->array, (pd->array_length+1)*sizeof(FBFile));
                    // Rofi expects utf-8, so lets convert the filename.
                    pd->array[pd->array_length].name = g_strdup ( ".." );
                    pd->array[pd->array_length].path = NULL;
                    pd->array[pd->array_length].type = UP;
                    pd->array[pd->array_length].icon_fetch_uid = 0;
                    pd->array_length++;
                    continue;

            } else if ( rd->d_name[0] == '.' ) {
                continue;
            }

            switch ( rd->d_type )
            {
                case DT_BLK:
                case DT_CHR:
                case DT_FIFO:
                case DT_UNKNOWN:
                case DT_SOCK:
                    break;
                case DT_REG:
                case DT_DIR:
                    pd->array = g_realloc ( pd->array, (pd->array_length+1)*sizeof(FBFile));
                    // Rofi expects utf-8, so lets convert the filename.
                    pd->array[pd->array_length].name = g_filename_to_utf8 ( rd->d_name, -1, NULL, NULL, NULL);
                    pd->array[pd->array_length].path = g_build_filename ( cdir, rd->d_name, NULL );
                    pd->array[pd->array_length].type = (rd->d_type == DT_DIR)? DIRECTORY: RFILE;
                    pd->array[pd->array_length].icon_fetch_uid = 0;
                    pd->array_length++;
            }
        }
        closedir ( dir );
    }
    g_qsort_with_data ( pd->array, pd->array_length, sizeof (FBFile ), compare, NULL );
}


static int file_browser_mode_init ( Mode *sw )
{
    /**
     * Called on startup when enabled (in modi list)
     */
    if ( mode_get_private_data ( sw ) == NULL ) {
        FileBrowserModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
        mode_set_private_data ( sw, (void *) pd );
        pd->current_dir = g_file_new_for_path(g_get_home_dir () );
        // Load content.
        get_file_browser ( sw );
    }
    return TRUE;
}
static unsigned int file_browser_mode_get_num_entries ( const Mode *sw )
{
    const FileBrowserModePrivateData *pd = (const FileBrowserModePrivateData *) mode_get_private_data ( sw );
    return pd->array_length;
}

static ModeMode file_browser_mode_result ( Mode *sw, int mretv, char **input, unsigned int selected_line )
{
    ModeMode           retv  = MODE_EXIT;
    FileBrowserModePrivateData *pd = (FileBrowserModePrivateData *) mode_get_private_data ( sw );
    if ( mretv & MENU_NEXT ) {
        retv = NEXT_DIALOG;
    } else if ( mretv & MENU_PREVIOUS ) {
        retv = PREVIOUS_DIALOG;
    } else if ( mretv & MENU_QUICK_SWITCH ) {
        retv = ( mretv & MENU_LOWER_MASK );
    } else if ( ( mretv & MENU_OK ) ) {
        if ( selected_line < pd->array_length )
        {
            if ( pd->array[selected_line].type == UP ) {
                GFile *new = g_file_get_parent ( pd->current_dir );
               if ( new ){
                   g_object_unref ( pd->current_dir );
                   pd->current_dir = new;
                   free_list (pd);
                   get_file_browser ( sw );
                   return RESET_DIALOG;
               }
            } else if ( pd->array[selected_line].type == DIRECTORY ) {
                GFile *new = g_file_new_for_path ( pd->array[selected_line].path );
                g_object_unref ( pd->current_dir );
                pd->current_dir = new;
                free_list (pd);
                get_file_browser ( sw );
                return RESET_DIALOG;
            } else if ( pd->array[selected_line].type == RFILE ) {
                char *d = g_filename_from_utf8 (pd->array[selected_line].path, -1, NULL, NULL, NULL);
                char *cmd = g_strdup_printf("xdg-open '%s'", d );
                g_free(d);
                char *cdir = g_file_get_path ( pd->current_dir );
                helper_execute_command ( cdir,cmd, FALSE,NULL );
                g_free ( cdir );
                g_free ( cmd );
                return MODE_EXIT;
            }
        }
        retv = RELOAD_DIALOG;
    } else if ( (mretv&MENU_CUSTOM_INPUT) && *input ) {
        char *p = rofi_expand_path ( *input );
        char *dir = g_filename_from_utf8 ( p, -1, NULL, NULL, NULL );
        g_free (p);
        if ( g_file_test ( dir, G_FILE_TEST_EXISTS )  )
        {
            if ( g_file_test ( dir, G_FILE_TEST_IS_DIR ) ){
                g_object_unref ( pd->current_dir );
                pd->current_dir = g_file_new_for_path ( dir );
                g_free ( dir );
                free_list (pd);
                get_file_browser ( sw );
                return RESET_DIALOG;
            }

        }
        g_free ( dir );
        retv = RELOAD_DIALOG;
    } else if ( ( mretv & MENU_ENTRY_DELETE ) == MENU_ENTRY_DELETE ) {
        retv = RELOAD_DIALOG;
    }
    return retv;
}


static void file_browser_mode_destroy ( Mode *sw )
{
    FileBrowserModePrivateData *pd = (FileBrowserModePrivateData *) mode_get_private_data ( sw );
    if ( pd != NULL ) {
        g_object_unref ( pd->current_dir );
        free_list ( pd );
        g_free ( pd );
        mode_set_private_data ( sw, NULL );
    }
}

static char *_get_display_value ( const Mode *sw, unsigned int selected_line, G_GNUC_UNUSED int *state, G_GNUC_UNUSED GList **attr_list, int get_entry )
{
    FileBrowserModePrivateData *pd = (FileBrowserModePrivateData *) mode_get_private_data ( sw );

    // Only return the string if requested, otherwise only set state.
    if ( !get_entry ) return NULL;
    if ( pd->array[selected_line].type == DIRECTORY ){
        return g_strdup ( pd->array[selected_line].name);
    } else if ( pd->array[selected_line].type == UP ){
        return g_strdup( " ..");
    } else {
        return g_strdup ( pd->array[selected_line].name);
    }
    return g_strdup("n/a");
}

/**
 * @param sw The mode object.
 * @param tokens The tokens to match against.
 * @param index  The index in this plugin to match against.
 *
 * Match the entry.
 *
 * @param returns try when a match.
 */
static int file_browser_token_match ( const Mode *sw, rofi_int_matcher **tokens, unsigned int index )
{
    FileBrowserModePrivateData *pd = (FileBrowserModePrivateData *) mode_get_private_data ( sw );

    // Call default matching function.
    return helper_token_match ( tokens, pd->array[index].name);
}

static cairo_surface_t *_get_icon ( const Mode *sw, unsigned int selected_line, int height )
{
    FileBrowserModePrivateData *pd = (FileBrowserModePrivateData *) mode_get_private_data ( sw );
    g_return_val_if_fail ( pd->array != NULL, NULL );
    FBFile *dr = &( pd->array[selected_line] );
    if ( dr->icon_fetch_uid > 0 ) {
        return rofi_icon_fetcher_get ( dr->icon_fetch_uid );
    }
    if ( dr->path && (g_str_has_suffix(dr->path, ".png") || g_str_has_suffix(dr->path, "jpeg") || g_str_has_suffix(dr->path, "jpg") )){
        dr->icon_fetch_uid = rofi_icon_fetcher_query ( dr->path, height );
    } else {
        dr->icon_fetch_uid = rofi_icon_fetcher_query ( icon_name[dr->type], height );
    }
    return rofi_icon_fetcher_get ( dr->icon_fetch_uid );
}

static char * _get_message ( const Mode *sw )
{
    FileBrowserModePrivateData *pd = (FileBrowserModePrivateData *) mode_get_private_data ( sw );
    if ( pd->current_dir ) {
        char *dirname = g_file_get_parse_name ( pd->current_dir );
        char *str = g_markup_printf_escaped("<b>Current directory:</b> %s", dirname);
        g_free ( dirname );
        return str;
    }
    return "n/a";
}


static char *_get_completion ( const Mode *sw, unsigned int index )
{
    FileBrowserModePrivateData *pd = (FileBrowserModePrivateData *) mode_get_private_data ( sw );

    char *d = g_strescape ( pd->array[index].path,NULL );
    return d;
}

Mode *create_new_file_browser (void )
{
    Mode *sw = g_malloc0(sizeof(Mode));

    *sw = file_browser_mode;

    sw->private_data = NULL;
    return sw;
}

#if 0
ModeMode file_browser_mode_completer ( Mode *sw, int mretv, char **input, unsigned int selected_line, char **path )
{
    ModeMode           retv  = MODE_EXIT;
    FileBrowserModePrivateData *pd = (FileBrowserModePrivateData *) mode_get_private_data ( sw );
    if ( mretv & MENU_NEXT ) {
        retv = NEXT_DIALOG;
    } else if ( mretv & MENU_PREVIOUS ) {
        retv = PREVIOUS_DIALOG;
    } else if ( mretv & MENU_QUICK_SWITCH ) {
        retv = ( mretv & MENU_LOWER_MASK );
    } else if ( ( mretv & MENU_OK ) ) {
        if ( selected_line < pd->array_length )
        {
            if ( pd->array[selected_line].type == UP ) {
                GFile *new = g_file_get_parent ( pd->current_dir );
               if ( new ){
                   g_object_unref ( pd->current_dir );
                   pd->current_dir = new;
                   free_list (pd);
                   get_file_browser ( sw );
                   return RESET_DIALOG;
               }
            } else if ( pd->array[selected_line].type == DIRECTORY ) {
                GFile *new = g_file_new_for_path ( pd->array[selected_line].path );
                g_object_unref ( pd->current_dir );
                pd->current_dir = new;
                free_list (pd);
                get_file_browser ( sw );
                return RESET_DIALOG;
            } else if ( pd->array[selected_line].type == RFILE ) {
                *path = g_strescape ( pd->array[selected_line].path,NULL );
                return MODE_EXIT;
            }
        }
        retv = RELOAD_DIALOG;
    } else if ( (mretv&MENU_CUSTOM_INPUT) && *input ) {
        char *p = rofi_expand_path ( *input );
        char *dir = g_filename_from_utf8 ( p, -1, NULL, NULL, NULL );
        g_free (p);
        if ( g_file_test ( dir, G_FILE_TEST_EXISTS )  )
        {
            if ( g_file_test ( dir, G_FILE_TEST_IS_DIR ) ){
                g_object_unref ( pd->current_dir );
                pd->current_dir = g_file_new_for_path ( dir );
                g_free ( dir );
                free_list (pd);
                get_file_browser ( sw );
                return RESET_DIALOG;
            }

        }
        g_free ( dir );
        retv = RELOAD_DIALOG;
    } else if ( ( mretv & MENU_ENTRY_DELETE ) == MENU_ENTRY_DELETE ) {
        retv = RELOAD_DIALOG;
    }
    return retv;
}
#endif

Mode file_browser_mode =
{
    .display_name       = NULL,
    .abi_version        = ABI_VERSION,
    .name               = "file-browser",
    .cfg_name_key       = "display-file_browser",
    ._init              = file_browser_mode_init,
    ._get_num_entries   = file_browser_mode_get_num_entries,
    ._result            = file_browser_mode_result,
    ._destroy           = file_browser_mode_destroy,
    ._token_match       = file_browser_token_match,
    ._get_display_value = _get_display_value,
    ._get_icon          = _get_icon,
    ._get_message       = _get_message,
    ._get_completion    = _get_completion,
    ._preprocess_input  = NULL,
    .private_data       = NULL,
    .free               = NULL,
};
