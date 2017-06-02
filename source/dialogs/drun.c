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

#define G_LOG_DOMAIN    "Dialogs.DRun"

#include <config.h>
#ifdef ENABLE_DRUN
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <strings.h>
#include <string.h>
#include <errno.h>

#include "rofi.h"
#include "settings.h"
#include "helper.h"
#include "timings.h"
#include "widgets/textbox.h"
#include "history.h"
#include "dialogs/drun.h"
#include "nkutils-xdg-theme.h"
#include "xcb.h"

#define DRUN_CACHE_FILE    "rofi2.druncache"


/**
 * Store extra information about the entry.
 * Currently the executable and if it should run in terminal.
 */
typedef struct
{
    /* Root */
    char            *root;
    /* Path to desktop file */
    char            *path;
    /* Icon stuff */
    int             icon_size;
    char            *icon_name;
    cairo_surface_t *icon;
    /* Executable */
    char            *exec;
    /* Name of the Entry */
    char            *name;
    /* Generic Name */
    char            *generic_name;
    char            **categories;

    GKeyFile        *key_file;
} DRunModeEntry;

typedef struct
{
    NkXdgThemeContext *xdg_context;
    DRunModeEntry     *entry_list;
    unsigned int      cmd_list_length;
    unsigned int      cmd_list_length_actual;
    unsigned int      history_length;
    // List of disabled entries.
    GHashTable        *disabled_entries;
    unsigned int      disabled_entries_length;
    GThread           *thread;
    GAsyncQueue      *icon_fetch_queue;

    unsigned int      expected_line_height;
    DRunModeEntry    quit_entry;
} DRunModePrivateData;

struct RegexEvalArg
{
    DRunModeEntry *e;
    gboolean      success;
};
static gboolean drun_helper_eval_cb ( const GMatchInfo *info, GString *res, gpointer data )
{
    // TODO quoting is not right? Find description not very clear, need to check.
    struct RegexEvalArg *e = (struct RegexEvalArg *) data;

    gchar               *match;
    // Get the match
    match = g_match_info_fetch ( info, 0 );
    if ( match != NULL ) {
        switch ( match[1] )
        {
        // Unsupported
        case 'f':
        case 'F':
        case 'u':
        case 'U':
        case 'i':
        // Deprecated
        case 'd':
        case 'D':
        case 'n':
        case 'N':
        case 'v':
        case 'm':
            break;
        case 'k':
            if ( e->e->path ) {
                char *esc = g_shell_quote ( e->e->path );
                g_string_append ( res, esc );
                g_free ( esc );
            }
            break;
        case 'c':
            if ( e->e->name ) {
                char *esc = g_shell_quote ( e->e->name );
                g_string_append ( res, esc );
                g_free ( esc );
            }
            break;
        // Invalid, this entry should not be processed -> throw error.
        default:
            e->success = FALSE;
            g_free ( match );
            return TRUE;
        }
        g_free ( match );
    }
    // Continue replacement.
    return FALSE;
}
static void exec_cmd_entry ( DRunModeEntry *e )
{
    GError *error = NULL;
    GRegex *reg   = g_regex_new ( "%[a-zA-Z]", 0, 0, &error );
    if ( error != NULL ) {
        g_warning ( "Internal error, failed to create regex: %s.", error->message );
        g_error_free ( error );
        return;
    }
    struct RegexEvalArg earg = { .e = e, .success = TRUE };
    char                *str = g_regex_replace_eval ( reg, e->exec, -1, 0, 0, drun_helper_eval_cb, &earg, &error );
    if ( error != NULL ) {
        g_warning ( "Internal error, failed replace field codes: %s.", error->message );
        g_error_free ( error );
        return;
    }
    g_regex_unref ( reg );
    if ( earg.success == FALSE ) {
        g_warning ( "Invalid field code in Exec line: %s.", e->exec );;
        return;
    }
    if ( str == NULL ) {
        g_warning ( "Nothing to execute after processing: %s.", e->exec );;
        return;
    }
    gchar *fp        = rofi_expand_path ( g_strstrip ( str ) );
    gchar *exec_path = g_key_file_get_string ( e->key_file, "Desktop Entry", "Path", NULL );
    if ( exec_path != NULL && strlen ( exec_path ) == 0 ) {
        // If it is empty, ignore this property. (#529)
        g_free ( exec_path );
        exec_path = NULL;
    }

    // Returns false if not found, if key not found, we don't want run in terminal.
    gboolean terminal = g_key_file_get_boolean ( e->key_file, "Desktop Entry", "Terminal", NULL );
    if ( helper_execute_command ( exec_path, fp, terminal ) ) {
        char *path = g_build_filename ( cache_dir, DRUN_CACHE_FILE, NULL );
        char *key  = g_strdup_printf ( "%s:::%s", e->root, e->path );
        history_set ( path, key );
        g_free ( key );
        g_free ( path );
    }
    g_free ( exec_path );
    g_free ( str );
    g_free ( fp );
}
/**
 * This function absorbs/freeś path, so this is no longer available afterwards.
 */
static gboolean read_desktop_file ( DRunModePrivateData *pd, const char *root, const char *path )
{
    // Create ID on stack.
    // We know strlen (path ) > strlen(root)+1
    const ssize_t id_len = strlen ( path ) - strlen ( root );
    char          id[id_len];
    g_strlcpy ( id, &( path[strlen ( root ) + 1] ), id_len );
    for ( int index = 0; index < id_len; index++ ) {
        if ( id[index] == '/' ) {
            id[index] = '-';
        }
    }

    // Check if item is on disabled list.
    if ( g_hash_table_contains ( pd->disabled_entries, id ) ) {
        g_debug ( "Skipping: %s, was previously seen.", id );
        return TRUE;
    }
    GKeyFile *kf    = g_key_file_new ();
    GError   *error = NULL;
    g_key_file_load_from_file ( kf, path, 0, &error );
    // If error, skip to next entry
    if ( error != NULL ) {
        g_debug ( "Failed to parse desktop file: %s because: %s", path, error->message );
        g_error_free ( error );
        g_key_file_free ( kf );
        return FALSE;
    }
    // Skip non Application entries.
    gchar *key = g_key_file_get_string ( kf, "Desktop Entry", "Type", NULL );
    if ( key == NULL ) {
        // No type? ignore.
        g_debug ( "Skipping desktop file: %s because: No type indicated", path );
        g_key_file_free ( kf );
        return FALSE;
    }
    if ( g_strcmp0 ( key, "Application" ) ) {
        g_debug ( "Skipping desktop file: %s because: Not of type application (%s)", path, key );
        g_free ( key );
        g_key_file_free ( kf );
        return FALSE;
    }
    g_free ( key );

    // Name key is required.
    if ( !g_key_file_has_key ( kf, "Desktop Entry", "Name", NULL ) ) {
        g_debug ( "Invalid DesktopFile: '%s', no 'Name' key present.", path );
        g_key_file_free ( kf );
        return FALSE;
    }

    // Skip hidden entries.
    if ( g_key_file_get_boolean ( kf, "Desktop Entry", "Hidden", NULL ) ) {
        g_debug ( "Adding desktop file: %s to disabled list because: Hdden", path );
        g_key_file_free ( kf );
        g_hash_table_add ( pd->disabled_entries, g_strdup ( id ) );
        return FALSE;
    }
    // Skip entries that have NoDisplay set.
    if ( g_key_file_get_boolean ( kf, "Desktop Entry", "NoDisplay", NULL ) ) {
        g_debug ( "Adding desktop file: %s to disabled list because: NoDisplay", path );
        g_key_file_free ( kf );
        g_hash_table_add ( pd->disabled_entries, g_strdup ( id ) );
        return FALSE;
    }
    // We need Exec, don't support DBusActivatable
    if ( !g_key_file_has_key ( kf, "Desktop Entry", "Exec", NULL ) ) {
        g_debug ( "Unsupported DesktopFile: '%s', no 'Exec' key present.", path );
        g_key_file_free ( kf );
        return FALSE;
    }
    size_t nl = ( ( pd->cmd_list_length ) + 1 );
    if ( nl >= pd->cmd_list_length_actual ) {
        pd->cmd_list_length_actual += 256;
        pd->entry_list              = g_realloc ( pd->entry_list, pd->cmd_list_length_actual * sizeof ( *( pd->entry_list ) ) );
    }
    pd->entry_list[pd->cmd_list_length].root = g_strdup ( root );
    pd->entry_list[pd->cmd_list_length].path = g_strdup ( path );
    gchar *n = g_key_file_get_locale_string ( kf, "Desktop Entry", "Name", NULL, NULL );
    pd->entry_list[pd->cmd_list_length].name = n;
    gchar *gn = g_key_file_get_locale_string ( kf, "Desktop Entry", "GenericName", NULL, NULL );
    pd->entry_list[pd->cmd_list_length].generic_name = gn;
    pd->entry_list[pd->cmd_list_length].categories = g_key_file_get_locale_string_list ( kf, "Desktop Entry", "Categories", NULL, NULL, NULL );
    pd->entry_list[pd->cmd_list_length].exec = g_key_file_get_string ( kf, "Desktop Entry", "Exec", NULL );

    if ( config.show_icons ) {
        pd->entry_list[pd->cmd_list_length].icon_name = g_key_file_get_locale_string ( kf, "Desktop Entry", "Icon", NULL, NULL );
    }
    else{
        pd->entry_list[pd->cmd_list_length].icon_name = NULL;
    }
    pd->entry_list[pd->cmd_list_length].icon = NULL;

    // Keep keyfile around.
    pd->entry_list[pd->cmd_list_length].key_file = kf;
    // We don't want to parse items with this id anymore.
    g_hash_table_add ( pd->disabled_entries, g_strdup ( id ) );
    ( pd->cmd_list_length )++;
    return TRUE;
}

/**
 * Internal spider used to get list of executables.
 */
static void walk_dir ( DRunModePrivateData *pd, const char *root, const char *dirname )
{
    DIR *dir;

    g_debug ( "Checking directory %s for desktop files.", root );
    dir = opendir ( dirname );
    if ( dir == NULL ) {
        return;
    }

    struct dirent *file;
    gchar         *filename = NULL;
    struct stat   st;
    while ( ( file = readdir ( dir ) ) != NULL ) {
        if ( file->d_name[0] == '.' ) {
            continue;
        }
        switch ( file->d_type )
        {
        case DT_LNK:
        case DT_REG:
        case DT_DIR:
        case DT_UNKNOWN:
            filename = g_build_filename ( dirname, file->d_name, NULL );
            break;
        default:
            continue;
        }

        // On a link, or if FS does not support providing this information
        // Fallback to stat method.
        if ( file->d_type == DT_LNK || file->d_type == DT_UNKNOWN ) {
            file->d_type = DT_UNKNOWN;
            if ( stat ( filename, &st ) == 0 ) {
                if ( S_ISDIR ( st.st_mode ) ) {
                    file->d_type = DT_DIR;
                }
                else if ( S_ISREG ( st.st_mode ) ) {
                    file->d_type = DT_REG;
                }
            }
        }

        switch ( file->d_type )
        {
        case DT_REG:
            // Skip files not ending on .desktop.
            if ( g_str_has_suffix ( file->d_name, ".desktop" ) ) {
                read_desktop_file ( pd, root, filename );
            }
            break;
        case DT_DIR:
            walk_dir ( pd, root, filename );
            break;
        default:
            break;
        }
        g_free ( filename );
    }
    closedir ( dir );
}
/**
 * @param entry The command entry to remove from history
 *
 * Remove command from history.
 */
static void delete_entry_history ( const DRunModeEntry *entry )
{
    char *path = g_build_filename ( cache_dir, DRUN_CACHE_FILE, NULL );
    char *key  = g_strdup_printf ( "%s:::%s", entry->root, entry->path );
    history_remove ( path, key );
    g_free ( key );
    g_free ( path );
}

static void get_apps_history ( DRunModePrivateData *pd )
{
    unsigned int length = 0;
    gchar        *path  = g_build_filename ( cache_dir, DRUN_CACHE_FILE, NULL );
    gchar        **retv = history_get_list ( path, &length );
    for ( unsigned int index = 0; index < length; index++ ) {
        char **st = g_strsplit ( retv[index], ":::", 2 );
        if ( st && st[0] && st[1] ) {
            if ( !read_desktop_file ( pd, st[0], st[1] ) ) {
                history_remove ( path, retv[index] );
            }
        }
        g_strfreev ( st );
    }
    g_strfreev ( retv );
    g_free ( path );
    pd->history_length = pd->cmd_list_length;
}

static void get_apps ( DRunModePrivateData *pd )
{
    TICK_N ( "Get Desktop apps (start)" );
    get_apps_history ( pd );

    gchar *dir;
    // First read the user directory.
    dir = g_build_filename ( g_get_user_data_dir (), "applications", NULL );
    walk_dir ( pd, dir, dir );
    g_free ( dir );
    TICK_N ( "Get Desktop apps (user dir)" );
    // Then read thee system data dirs.
    const gchar * const * sys = g_get_system_data_dirs ();
    for (; *sys != NULL; ++sys ) {
        dir = g_build_filename ( *sys, "applications", NULL );
        walk_dir ( pd, dir, dir );
        g_free ( dir );
    }
    TICK_N ( "Get Desktop apps (system dirs)" );
}

static gpointer drun_icon_fetch ( gpointer data )
{
    // as long as dr->icon is updated atomicly.. (is a pointer write atomic?)
    // this should be fine running in another thread.
    DRunModePrivateData *pd = (DRunModePrivateData *) data;
    DRunModeEntry *dr;
    while ( ( dr = g_async_queue_pop ( pd->icon_fetch_queue )) != &(pd->quit_entry)  )
    {
        if ( dr->icon_name == NULL ) {
            continue;
        }
        gchar *icon_path = nk_xdg_theme_get_icon ( pd->xdg_context, NULL, "Applications", dr->icon_name, dr->icon_size, 1, TRUE );
        if ( icon_path == NULL ) {
            g_free ( dr->icon_name );
            dr->icon_name = NULL;
            continue;
        }
        else{
            g_log ( G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Found Icon %s(%d): %s", dr->icon_name, dr->icon_size, icon_path );
        }

        if ( g_str_has_suffix ( icon_path, ".png" ) ) {
            dr->icon = cairo_image_surface_create_from_png ( icon_path );
        }
        else if ( g_str_has_suffix ( icon_path, ".svg" ) ) {
            dr->icon = cairo_image_surface_create_from_svg ( icon_path, dr->icon_size);
        }
        else {
            g_log ( G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Icon type not yet supported: %s", icon_path );
            char *r = dr->icon_name;
            dr->icon_name = NULL;
            g_free ( r );
        }
        g_free ( icon_path );
        rofi_view_reload ();
    }
    return NULL;
}

static int drun_mode_init ( Mode *sw )
{
    if ( mode_get_private_data ( sw ) == NULL ) {
        DRunModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
        pd->disabled_entries = g_hash_table_new_full ( g_str_hash, g_str_equal, g_free, NULL );
        mode_set_private_data ( sw, (void *) pd );
        pd->xdg_context = nk_xdg_theme_context_new ();
        get_apps ( pd );
        pd->icon_fetch_queue = g_async_queue_new ( );
    }
    return TRUE;
}
static void drun_entry_clear ( DRunModeEntry *e )
{
    g_free ( e->root );
    g_free ( e->path );
    if ( e->icon != NULL ) {
        cairo_surface_destroy ( e->icon );
    }
    g_free ( e->icon_name );
    g_free ( e->exec );
    g_free ( e->name );
    g_free ( e->generic_name );
    g_strfreev ( e->categories );
    g_key_file_free ( e->key_file );
}

static ModeMode drun_mode_result ( Mode *sw, int mretv, char **input, unsigned int selected_line )
{
    DRunModePrivateData *rmpd = (DRunModePrivateData *) mode_get_private_data ( sw );
    ModeMode            retv  = MODE_EXIT;

    gboolean            run_in_term = ( ( mretv & MENU_CUSTOM_ACTION ) == MENU_CUSTOM_ACTION );

    if ( mretv & MENU_NEXT ) {
        retv = NEXT_DIALOG;
    }
    else if ( mretv & MENU_PREVIOUS ) {
        retv = PREVIOUS_DIALOG;
    }
    else if ( mretv & MENU_QUICK_SWITCH ) {
        retv = ( mretv & MENU_LOWER_MASK );
    }
    else if ( ( mretv & MENU_OK )  ) {
        exec_cmd_entry ( &( rmpd->entry_list[selected_line] ) );
    }
    else if ( ( mretv & MENU_CUSTOM_INPUT ) && *input != NULL && *input[0] != '\0' ) {
        helper_execute_command ( NULL, *input, run_in_term );
    }
    else if ( ( mretv & MENU_ENTRY_DELETE ) && selected_line < rmpd->cmd_list_length ) {
        if ( selected_line < rmpd->history_length ) {
            if ( rmpd->thread ) {
                g_async_queue_lock ( rmpd->icon_fetch_queue );
                DRunModeEntry *dr;
                while ( (dr = g_async_queue_try_pop_unlocked ( rmpd->icon_fetch_queue )) != NULL ){
                    // Reset for possible re-fetch.
                    dr->icon_size = 0;
                }
                g_async_queue_unlock ( rmpd->icon_fetch_queue );
            }
            delete_entry_history ( &( rmpd->entry_list[selected_line] ) );
            drun_entry_clear ( &( rmpd->entry_list[selected_line] ) );
            memmove ( &( rmpd->entry_list[selected_line] ), &rmpd->entry_list[selected_line + 1],
                      sizeof ( DRunModeEntry ) * ( rmpd->cmd_list_length - selected_line - 1 ) );
            rmpd->cmd_list_length--;
        }
        retv = RELOAD_DIALOG;
    }
    return retv;
}
static void drun_mode_destroy ( Mode *sw )
{
    DRunModePrivateData *rmpd = (DRunModePrivateData *) mode_get_private_data ( sw );
    if ( rmpd != NULL ) {
        if ( rmpd->thread ) {
            g_async_queue_lock ( rmpd->icon_fetch_queue );
            while ( g_async_queue_try_pop_unlocked ( rmpd->icon_fetch_queue ) );
            // Make the thread quit.
            g_async_queue_push_front_unlocked ( rmpd->icon_fetch_queue, (gpointer) &(rmpd->quit_entry));
            g_async_queue_unlock ( rmpd->icon_fetch_queue );
            g_thread_join ( rmpd->thread );
            rmpd->thread = NULL;
        }
        if ( rmpd->icon_fetch_queue ) {
            g_async_queue_unref ( rmpd->icon_fetch_queue );
            rmpd->icon_fetch_queue = NULL;
        }
        for ( size_t i = 0; i < rmpd->cmd_list_length; i++ ) {
            drun_entry_clear ( &( rmpd->entry_list[i] ) );
        }
        g_hash_table_destroy ( rmpd->disabled_entries );
        g_free ( rmpd->entry_list );
        nk_xdg_theme_context_free ( rmpd->xdg_context );
        g_free ( rmpd );
        mode_set_private_data ( sw, NULL );
    }
}

static char *_get_display_value ( const Mode *sw, unsigned int selected_line, int *state, G_GNUC_UNUSED GList **list, int get_entry )
{
    DRunModePrivateData *pd = (DRunModePrivateData *) mode_get_private_data ( sw );
    *state |= MARKUP;
    if ( !get_entry ) {
        return NULL;
    }
    if ( pd->entry_list == NULL ) {
        // Should never get here.
        return g_strdup ( "Failed" );
    }
    /* Free temp storage. */
    DRunModeEntry *dr = &( pd->entry_list[selected_line] );
    if ( dr->generic_name == NULL ) {
        return g_markup_printf_escaped ( "%s", dr->name );
    }
    else {
        return g_markup_printf_escaped ( "%s <span weight='light' size='small'><i>(%s)</i></span>", dr->name,
                                         dr->generic_name );
    }
}

static cairo_surface_t *_get_icon ( const Mode *sw, unsigned int selected_line, int height )
{
    DRunModePrivateData *pd = (DRunModePrivateData *) mode_get_private_data ( sw );
    g_return_val_if_fail ( pd->entry_list != NULL, NULL );
    DRunModeEntry       *dr = &( pd->entry_list[selected_line] );
    if ( pd->thread == NULL ) {
        pd->thread = g_thread_new ( "icon-fetch-drun", drun_icon_fetch, pd );
    }
    if ( dr->icon_size ==  0 && dr->icon_name  ) {
        dr->icon_size = height;
        g_async_queue_push ( pd->icon_fetch_queue, dr );
    }
    return dr->icon;
}

static char *drun_get_completion ( const Mode *sw, unsigned int index )
{
    DRunModePrivateData *pd = (DRunModePrivateData *) mode_get_private_data ( sw );
    /* Free temp storage. */
    DRunModeEntry       *dr = &( pd->entry_list[index] );
    if ( dr->generic_name == NULL ) {
        return g_strdup ( dr->name );
    }
    else {
        return g_strdup_printf ( "%s", dr->name );
    }
}

static int drun_token_match ( const Mode *data, GRegex **tokens, unsigned int index )
{
    DRunModePrivateData *rmpd = (DRunModePrivateData *) mode_get_private_data ( data );
    int                 match = 1;
    if ( tokens ) {
        for ( int j = 0; match && tokens != NULL && tokens[j] != NULL; j++ ) {
            int    test        = 0;
            GRegex *ftokens[2] = { tokens[j], NULL };
            // Match name
            if ( rmpd->entry_list[index].name &&
                 helper_token_match ( ftokens, rmpd->entry_list[index].name ) ) {
                test = 1;
            }
            // Match generic name
            if ( !test && rmpd->entry_list[index].generic_name &&
                 helper_token_match ( ftokens, rmpd->entry_list[index].generic_name ) ) {
                test = 1;
            }
            // Match executable name.
            if ( !test && helper_token_match ( ftokens, rmpd->entry_list[index].exec ) ) {
                test = 1;
            }
            // Match against category.
            if ( !test ) {
                gchar **list = rmpd->entry_list[index].categories;
                for ( int iter = 0; !test && list && list[iter]; iter++ ) {
                    test = helper_token_match ( ftokens, list[iter] );
                }
            }
            if ( test == 0 ) {
                match = 0;
            }
        }
    }
    return match;
}

static unsigned int drun_mode_get_num_entries ( const Mode *sw )
{
    const DRunModePrivateData *pd = (const DRunModePrivateData *) mode_get_private_data ( sw );
    return pd->cmd_list_length;
}
#include "mode-private.h"
Mode drun_mode =
{
    .name               = "drun",
    .cfg_name_key       = "display-drun",
    ._init              = drun_mode_init,
    ._get_num_entries   = drun_mode_get_num_entries,
    ._result            = drun_mode_result,
    ._destroy           = drun_mode_destroy,
    ._token_match       = drun_token_match,
    ._get_completion    = drun_get_completion,
    ._get_display_value = _get_display_value,
    ._get_icon          = _get_icon,
    ._preprocess_input  = NULL,
    .private_data       = NULL,
    .free               = NULL
};

#endif // ENABLE_DRUN
