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

/** The log domain of this dialog. */
#define G_LOG_DOMAIN    "Dialogs.DRun"

#include <config.h>
#ifdef ENABLE_DRUN
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

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
#include "xcb.h"

#include "rofi-icon-fetcher.h"

#define DRUN_CACHE_FILE            "rofi3.druncache"
#define DRUN_DESKTOP_CACHE_FILE    "rofi-drun-desktop.cache"

char *DRUN_GROUP_NAME = "Desktop Entry";

typedef struct _DRunModePrivateData DRunModePrivateData;

typedef enum
{
    DRUN_DESKTOP_ENTRY_TYPE_UNDETERMINED = 0,
    DRUN_DESKTOP_ENTRY_TYPE_APPLICATION,
    DRUN_DESKTOP_ENTRY_TYPE_LINK,
    DRUN_DESKTOP_ENTRY_TYPE_DIRECTORY,
} DRunDesktopEntryType;

/**
 * Store extra information about the entry.
 * Currently the executable and if it should run in terminal.
 */
typedef struct
{
    DRunModePrivateData *pd;
    /* category */
    char                *action;
    /* Root */
    char                *root;
    /* Path to desktop file */
    char                *path;
    /* Application id (.desktop filename) */
    char                *app_id;
    /* Desktop id */
    char                *desktop_id;
    /* Icon stuff */
    char                *icon_name;
    /* Icon size is used to indicate what size is requested by the gui.
     * secondary it indicates if the request for a lookup has been issued (0 not issued )
     */
    int                  icon_size;
    /* Surface holding the icon. */
    cairo_surface_t      *icon;
    /* Executable - for Application entries only */
    char                 *exec;
    /* Name of the Entry */
    char                 *name;
    /* Generic Name */
    char                 *generic_name;
    /* Categories */
    char                 **categories;
    /* Keywords */
    char                 **keywords;
    /* Comments */
    char                 *comment;

    GKeyFile             *key_file;

    gint                 sort_index;

    uint32_t             icon_fetch_uid;

    DRunDesktopEntryType type;
} DRunModeEntry;

typedef struct
{
    const char *entry_field_name;
    gboolean   enabled;
} DRunEntryField;

typedef enum
{
    DRUN_MATCH_FIELD_NAME,
    DRUN_MATCH_FIELD_GENERIC,
    DRUN_MATCH_FIELD_EXEC,
    DRUN_MATCH_FIELD_CATEGORIES,
    DRUN_MATCH_FIELD_KEYWORDS,
    DRUN_MATCH_FIELD_COMMENT,
    DRUN_MATCH_NUM_FIELDS,
} DRunMatchingFields;

static DRunEntryField matching_entry_fields[DRUN_MATCH_NUM_FIELDS] = {
    { .entry_field_name = "name",       .enabled = TRUE,  },
    { .entry_field_name = "generic",    .enabled = TRUE,  },
    { .entry_field_name = "exec",       .enabled = TRUE,  },
    { .entry_field_name = "categories", .enabled = TRUE,  },
    { .entry_field_name = "keywords",   .enabled = TRUE,  },
    { .entry_field_name = "comment",    .enabled = FALSE, }
};

struct _DRunModePrivateData
{
    DRunModeEntry *entry_list;
    unsigned int  cmd_list_length;
    unsigned int  cmd_list_length_actual;
    // List of disabled entries.
    GHashTable    *disabled_entries;
    unsigned int  disabled_entries_length;
    unsigned int  expected_line_height;

    char          **show_categories;

    // Theme
    const gchar   *icon_theme;
    // DE
    gchar         **current_desktop_list;
};

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
        case '%':
            g_string_append ( res, "%" );
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
static void launch_link_entry ( DRunModeEntry *e )
{
    if ( e->key_file == NULL ) {
        GKeyFile *kf    = g_key_file_new ();
        GError   *error = NULL;
        gboolean res    = g_key_file_load_from_file ( kf, e->path, 0, &error );
        if ( res ) {
            e->key_file = kf;
        }
        else {
            g_warning ( "[%s] [%s] Failed to parse desktop file because: %s.", e->app_id, e->path, error->message );
            g_error_free ( error );
            g_key_file_free ( kf );
            return;
        }
    }

    gchar *url = g_key_file_get_string ( e->key_file, e->action, "URL", NULL );
    if ( url == NULL || strlen ( url ) == 0 ) {
        g_warning ( "[%s] [%s] No URL found.", e->app_id, e->path );
        g_free ( url );
        return;
    }

    gsize command_len = strlen ( config.drun_url_launcher ) + strlen ( url ) + 2; // space + terminator = 2
    gchar *command    = g_newa ( gchar, command_len );
    g_snprintf ( command, command_len, "%s %s", config.drun_url_launcher, url );
    g_free ( url );

    g_debug ( "Link launch command: |%s|", command );
    if ( helper_execute_command ( NULL, command, FALSE, NULL ) ) {
        char *path = g_build_filename ( cache_dir, DRUN_CACHE_FILE, NULL );
        // Store it based on the unique identifiers (desktop_id).
        history_set ( path, e->desktop_id );
        g_free ( path );
    }
}
static void exec_cmd_entry ( DRunModeEntry *e )
{
    GError *error = NULL;
    GRegex *reg   = g_regex_new ( "%[a-zA-Z%]", 0, 0, &error );
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
    g_debug ( "Parsed command: |%s| into |%s|.", e->exec, str );

    if ( e->key_file == NULL ) {
        GKeyFile *kf    = g_key_file_new ();
        GError   *error = NULL;
        gboolean res    = g_key_file_load_from_file ( kf, e->path, 0, &error );
        if ( res ) {
            e->key_file = kf;
        }
        else {
            g_warning ( "[%s] [%s] Failed to parse desktop file because: %s.", e->app_id, e->path, error->message );
            g_error_free ( error );
            g_key_file_free ( kf );

            return;
        }
    }

    const gchar *fp        = g_strstrip ( str );
    gchar       *exec_path = g_key_file_get_string ( e->key_file, e->action, "Path", NULL );
    if ( exec_path != NULL && strlen ( exec_path ) == 0 ) {
        // If it is empty, ignore this property. (#529)
        g_free ( exec_path );
        exec_path = NULL;
    }

    RofiHelperExecuteContext context = {
        .name   = e->name,
        .icon   = e->icon_name,
        .app_id = e->app_id,
    };
    gboolean                 sn       = g_key_file_get_boolean ( e->key_file, e->action, "StartupNotify", NULL );
    gchar                    *wmclass = NULL;
    if ( sn && g_key_file_has_key ( e->key_file, e->action, "StartupWMClass", NULL ) ) {
        context.wmclass = wmclass = g_key_file_get_string ( e->key_file, e->action, "StartupWMClass", NULL );
    }

    // Returns false if not found, if key not found, we don't want run in terminal.
    gboolean terminal = g_key_file_get_boolean ( e->key_file, e->action, "Terminal", NULL );
    if ( helper_execute_command ( exec_path, fp, terminal, sn ? &context : NULL ) ) {
        char *path = g_build_filename ( cache_dir, DRUN_CACHE_FILE, NULL );
        // Store it based on the unique identifiers (desktop_id).
        history_set ( path, e->desktop_id );
        g_free ( path );
    }
    g_free ( wmclass );
    g_free ( exec_path );
    g_free ( str );
}

static gboolean rofi_strv_contains ( const char * const *categories, const char *const *field )
{
    for ( int i = 0; categories && categories[i]; i++ ) {
        for ( int j = 0; field[j]; j++ ) {
            if ( g_str_equal ( categories[i], field[j] ) ) {
                return TRUE;
            }
        }
    }
    return FALSE;
}
/**
 * This function absorbs/freeś path, so this is no longer available afterwards.
 */
static void read_desktop_file ( DRunModePrivateData *pd, const char *root, const char *path, const gchar *basename, const char *action )
{
    DRunDesktopEntryType desktop_entry_type = DRUN_DESKTOP_ENTRY_TYPE_UNDETERMINED;
    int                  parse_action       = ( config.drun_show_actions && action != DRUN_GROUP_NAME );
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
    if ( g_hash_table_contains ( pd->disabled_entries, id ) && !parse_action ) {
        g_debug ( "[%s] [%s] Skipping, was previously seen.", id, path );
        return;
    }
    GKeyFile *kf    = g_key_file_new ();
    GError   *error = NULL;
    gboolean res    = g_key_file_load_from_file ( kf, path, 0, &error );
    // If error, skip to next entry
    if ( !res ) {
        g_debug ( "[%s] [%s] Failed to parse desktop file because: %s.", id, path, error->message );
        g_error_free ( error );
        g_key_file_free ( kf );
        return;
    }

    if ( g_key_file_has_group ( kf, action ) == FALSE ) {
        // No type? ignore.
        g_debug ( "[%s] [%s] Invalid desktop file: No %s group", id, path, action );
        g_key_file_free ( kf );
        return;
    }
    // Skip non Application entries.
    gchar *key = g_key_file_get_string ( kf, DRUN_GROUP_NAME, "Type", NULL );
    if ( key == NULL ) {
        // No type? ignore.
        g_debug ( "[%s] [%s] Invalid desktop file: No type indicated", id, path );
        g_key_file_free ( kf );
        return;
    }
    if ( !g_strcmp0 ( key, "Application" ) ) {
        desktop_entry_type = DRUN_DESKTOP_ENTRY_TYPE_APPLICATION;
    }
    else if ( !g_strcmp0 ( key, "Link" ) ) {
        desktop_entry_type = DRUN_DESKTOP_ENTRY_TYPE_LINK;
    }
    else {
        g_debug ( "[%s] [%s] Skipping desktop file: Not of type Application or Link (%s)", id, path, key );
        g_free ( key );
        g_key_file_free ( kf );
        return;
    }
    g_free ( key );

    // Name key is required.
    if ( !g_key_file_has_key ( kf, DRUN_GROUP_NAME, "Name", NULL ) ) {
        g_debug ( "[%s] [%s] Invalid desktop file: no 'Name' key present.", id, path );
        g_key_file_free ( kf );
        return;
    }

    // Skip hidden entries.
    if ( g_key_file_get_boolean ( kf, DRUN_GROUP_NAME, "Hidden", NULL ) ) {
        g_debug ( "[%s] [%s] Adding desktop file to disabled list: 'Hidden' key is true", id, path );
        g_key_file_free ( kf );
        g_hash_table_add ( pd->disabled_entries, g_strdup ( id ) );
        return;
    }
    if ( pd->current_desktop_list ) {
        gboolean show = TRUE;
        // If the DE is set, check the keys.
        if ( g_key_file_has_key ( kf, DRUN_GROUP_NAME, "OnlyShowIn", NULL ) ) {
            gsize llength = 0;
            show = FALSE;
            gchar **list = g_key_file_get_string_list ( kf, DRUN_GROUP_NAME, "OnlyShowIn", &llength, NULL );
            if ( list ) {
                for ( gsize lcd = 0; !show && pd->current_desktop_list[lcd]; lcd++ ) {
                    for ( gsize lle = 0; !show && lle < llength; lle++ ) {
                        show = ( g_strcmp0  ( pd->current_desktop_list[lcd], list[lle] ) == 0 );
                    }
                }
                g_strfreev ( list );
            }
        }
        if ( show && g_key_file_has_key ( kf, DRUN_GROUP_NAME, "NotShowIn", NULL ) ) {
            gsize llength = 0;
            gchar **list  = g_key_file_get_string_list ( kf, DRUN_GROUP_NAME, "NotShowIn", &llength, NULL );
            if ( list ) {
                for ( gsize lcd = 0; show && pd->current_desktop_list[lcd]; lcd++ ) {
                    for ( gsize lle = 0; show && lle < llength; lle++ ) {
                        show = !( g_strcmp0  ( pd->current_desktop_list[lcd], list[lle] ) == 0 );
                    }
                }
                g_strfreev ( list );
            }
        }

        if ( !show ) {
            g_debug ( "[%s] [%s] Adding desktop file to disabled list: 'OnlyShowIn'/'NotShowIn' keys don't match current desktop", id, path );
            g_key_file_free ( kf );
            g_hash_table_add ( pd->disabled_entries, g_strdup ( id ) );
            return;
        }
    }
    // Skip entries that have NoDisplay set.
    if ( g_key_file_get_boolean ( kf, DRUN_GROUP_NAME, "NoDisplay", NULL ) ) {
        g_debug ( "[%s] [%s] Adding desktop file to disabled list: 'NoDisplay' key is true", id, path );
        g_key_file_free ( kf );
        g_hash_table_add ( pd->disabled_entries, g_strdup ( id ) );
        return;
    }

    // We need Exec, don't support DBusActivatable
    if ( desktop_entry_type == DRUN_DESKTOP_ENTRY_TYPE_APPLICATION
         && !g_key_file_has_key ( kf, DRUN_GROUP_NAME, "Exec", NULL ) ) {
        g_debug ( "[%s] [%s] Unsupported desktop file: no 'Exec' key present for type Application.", id, path );
        g_key_file_free ( kf );
        return;
    }
    if ( desktop_entry_type == DRUN_DESKTOP_ENTRY_TYPE_LINK
         && !g_key_file_has_key ( kf, DRUN_GROUP_NAME, "URL", NULL ) ) {
        g_debug ( "[%s] [%s] Unsupported desktop file: no 'URL' key present for type Link.", id, path );
        g_key_file_free ( kf );
        return;
    }

    if ( g_key_file_has_key ( kf, DRUN_GROUP_NAME, "TryExec", NULL ) ) {
        char *te = g_key_file_get_string ( kf, DRUN_GROUP_NAME, "TryExec", NULL );
        if ( !g_path_is_absolute ( te ) ) {
            char *fp = g_find_program_in_path ( te );
            if ( fp == NULL ) {
                g_free ( te );
                g_key_file_free ( kf );
                return;
            }
            g_free ( fp );
        }
        else {
            if ( g_file_test ( te, G_FILE_TEST_IS_EXECUTABLE ) == FALSE ) {
                g_free ( te );
                g_key_file_free ( kf );
                return;
            }
        }
        g_free ( te );
    }

    char **categories = NULL;
    if ( pd->show_categories ) {
        categories = g_key_file_get_locale_string_list ( kf, DRUN_GROUP_NAME, "Categories", NULL, NULL, NULL );
        if (  !rofi_strv_contains ( (const char * const *) categories, (const char * const *) pd->show_categories ) ) {
            g_strfreev ( categories );
            g_key_file_free ( kf );
            return;
        }
    }

    size_t nl = ( ( pd->cmd_list_length ) + 1 );
    if ( nl >= pd->cmd_list_length_actual ) {
        pd->cmd_list_length_actual += 256;
        pd->entry_list              = g_realloc ( pd->entry_list, pd->cmd_list_length_actual * sizeof ( *( pd->entry_list ) ) );
    }
    // Make sure order is preserved, this will break when cmd_list_length is bigger then INT_MAX.
    // This is not likely to happen.
    if ( G_UNLIKELY ( pd->cmd_list_length > INT_MAX ) ) {
        // Default to smallest value.
        pd->entry_list[pd->cmd_list_length].sort_index = INT_MIN;
    }
    else {
        pd->entry_list[pd->cmd_list_length].sort_index = -nl;
    }
    pd->entry_list[pd->cmd_list_length].icon_size      = 0;
    pd->entry_list[pd->cmd_list_length].icon_fetch_uid = 0;
    pd->entry_list[pd->cmd_list_length].root           = g_strdup ( root );
    pd->entry_list[pd->cmd_list_length].path           = g_strdup ( path );
    pd->entry_list[pd->cmd_list_length].desktop_id     = g_strdup ( id );
    pd->entry_list[pd->cmd_list_length].app_id         = g_strndup ( basename, strlen ( basename ) - strlen ( ".desktop" ) );
    gchar *n = g_key_file_get_locale_string ( kf, DRUN_GROUP_NAME, "Name", NULL, NULL );

    if ( action != DRUN_GROUP_NAME ) {
        gchar *na = g_key_file_get_locale_string ( kf, action, "Name", NULL, NULL );
        gchar *l  = g_strdup_printf ( "%s - %s", n, na );
        g_free ( n );
        n = l;
    }
    pd->entry_list[pd->cmd_list_length].name   = n;
    pd->entry_list[pd->cmd_list_length].action = DRUN_GROUP_NAME;
    gchar *gn = g_key_file_get_locale_string ( kf, DRUN_GROUP_NAME, "GenericName", NULL, NULL );
    pd->entry_list[pd->cmd_list_length].generic_name = gn;

    if ( matching_entry_fields[DRUN_MATCH_FIELD_KEYWORDS].enabled ) {
        pd->entry_list[pd->cmd_list_length].keywords = g_key_file_get_locale_string_list ( kf, DRUN_GROUP_NAME, "Keywords", NULL, NULL, NULL );
    }
    else {
        pd->entry_list[pd->cmd_list_length].keywords = NULL;
    }

    if ( matching_entry_fields[DRUN_MATCH_FIELD_CATEGORIES].enabled ) {
        if ( categories ) {
            pd->entry_list[pd->cmd_list_length].categories = categories;
            categories                                     = NULL;
        }
        else {
            pd->entry_list[pd->cmd_list_length].categories = g_key_file_get_locale_string_list ( kf, DRUN_GROUP_NAME, "Categories", NULL, NULL, NULL );
        }
    }
    else {
        pd->entry_list[pd->cmd_list_length].categories = NULL;
    }
    g_strfreev ( categories );

    pd->entry_list[pd->cmd_list_length].type = desktop_entry_type;
    if ( desktop_entry_type == DRUN_DESKTOP_ENTRY_TYPE_APPLICATION ) {
        pd->entry_list[pd->cmd_list_length].exec = g_key_file_get_string ( kf, action, "Exec", NULL );
    }
    else {
        pd->entry_list[pd->cmd_list_length].exec = NULL;
    }

    if ( matching_entry_fields[DRUN_MATCH_FIELD_COMMENT].enabled ) {
        pd->entry_list[pd->cmd_list_length].comment = g_key_file_get_locale_string ( kf,
                                                                                     DRUN_GROUP_NAME, "Comment", NULL, NULL );
    }
    else {
        pd->entry_list[pd->cmd_list_length].comment = NULL;
    }
    if ( config.show_icons ) {
        pd->entry_list[pd->cmd_list_length].icon_name = g_key_file_get_locale_string ( kf, DRUN_GROUP_NAME, "Icon", NULL, NULL );
    }
    else{
        pd->entry_list[pd->cmd_list_length].icon_name = NULL;
    }
    pd->entry_list[pd->cmd_list_length].icon = NULL;

    // Keep keyfile around.
    pd->entry_list[pd->cmd_list_length].key_file = kf;
    // We don't want to parse items with this id anymore.
    g_hash_table_add ( pd->disabled_entries, g_strdup ( id ) );
    g_debug ( "[%s] Using file %s.", id, path );
    ( pd->cmd_list_length )++;

    if ( !parse_action ) {
        gsize actions_length = 0;
        char  **actions      = g_key_file_get_string_list ( kf, DRUN_GROUP_NAME, "Actions", &actions_length, NULL );
        for ( gsize iter = 0; iter < actions_length; iter++ ) {
            char *new_action = g_strdup_printf ( "Desktop Action %s", actions[iter] );
            read_desktop_file ( pd, root, path, basename, new_action );
            g_free ( new_action );
        }
        g_strfreev ( actions );
    }
    return;
}

/**
 * Internal spider used to get list of executables.
 */
static void walk_dir ( DRunModePrivateData *pd, const char *root, const char *dirname )
{
    DIR *dir;

    g_debug ( "Checking directory %s for desktop files.", dirname );
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
                read_desktop_file ( pd, root, filename, file->d_name, DRUN_GROUP_NAME );
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
    history_remove ( path, entry->desktop_id );
    g_free ( path );
}

static void get_apps_history ( DRunModePrivateData *pd )
{
    TICK_N ( "Start drun history" );
    unsigned int length = 0;
    gchar        *path  = g_build_filename ( cache_dir, DRUN_CACHE_FILE, NULL );
    gchar        **retv = history_get_list ( path, &length );
    for ( unsigned int index = 0; index < length; index++ ) {
        for ( size_t i = 0; i < pd->cmd_list_length; i++ ) {
            if ( g_strcmp0 ( pd->entry_list[i].desktop_id, retv[index] ) == 0 ) {
                unsigned int sort_index = length - index;
                if ( G_LIKELY ( sort_index < INT_MAX ) ) {
                    pd->entry_list[i].sort_index = sort_index;
                }
                else {
                    // This won't sort right anymore, but never gonna hit it anyway.
                    pd->entry_list[i].sort_index = INT_MAX;
                }
            }
        }
    }
    g_strfreev ( retv );
    g_free ( path );
    TICK_N ( "Stop drun history" );
}

static gint drun_int_sort_list ( gconstpointer a, gconstpointer b, G_GNUC_UNUSED gpointer user_data )
{
    DRunModeEntry *da = (DRunModeEntry *) a;
    DRunModeEntry *db = (DRunModeEntry *) b;

    if ( da->sort_index < 0 && db->sort_index < 0 ) {
        return g_utf8_collate ( da->name, db->name );
    }
    else {
        return db->sort_index - da->sort_index;
    }
}

/*******************************************
* Cache voodoo                            *
*******************************************/

#define CACHE_VERSION    2
static void drun_write_str ( FILE *fd, const char *str )
{
    size_t l = ( str == NULL ? 0 : strlen ( str ) );
    fwrite ( &l, sizeof ( l ), 1, fd );
    // Only write string if it is not NULL or empty.
    if ( l > 0 ) {
        // Also writeout terminating '\0'
        fwrite ( str, 1, l + 1, fd );
    }
}
static void drun_write_integer ( FILE *fd, int32_t val )
{
    fwrite ( &val, sizeof ( val ), 1, fd );
}
static void drun_read_integer ( FILE *fd, int32_t *type )
{
    if ( fread ( type, sizeof ( int32_t ), 1, fd ) != 1 ) {
        g_warning ( "Failed to read entry, cache corrupt?" );
        return;
    }
}
static void drun_read_string ( FILE *fd, char **str )
{
    size_t l = 0;

    if ( fread ( &l, sizeof ( l ), 1, fd ) != 1 ) {
        g_warning ( "Failed to read entry, cache corrupt?" );
        return;
    }
    ( *str ) = NULL;
    if ( l > 0 ) {
        // Include \0
        l++;
        ( *str ) = g_malloc ( l );
        if ( fread ( ( *str ), 1, l, fd ) != l ) {
            g_warning ( "Failed to read entry, cache corrupt?" );
        }
    }
}
static void drun_write_strv ( FILE *fd, char **str )
{
    guint vl = ( str == NULL ? 0 : g_strv_length ( str ) );
    fwrite ( &vl, sizeof ( vl ), 1, fd );
    for ( guint index = 0; index < vl; index++ ) {
        drun_write_str ( fd, str[index] );
    }
}
static void drun_read_stringv ( FILE *fd, char ***str )
{
    guint vl = 0;
    ( *str ) = NULL;
    if ( fread ( &vl, sizeof ( vl ), 1, fd ) != 1 ) {
        g_warning ( "Failed to read entry, cache corrupt?" );
        return;
    }
    if ( vl > 0 ) {
        // Include terminating NULL entry.
        ( *str ) = g_malloc0 ( ( vl + 1 ) * sizeof ( **str ) );
        for ( guint index = 0; index < vl; index++ ) {
            drun_read_string ( fd, &( ( *str )[index] ) );
        }
    }
}

static void write_cache ( DRunModePrivateData *pd, const char *cache_file )
{
    if ( cache_file == NULL || config.drun_use_desktop_cache == FALSE ) {
        return;
    }
    TICK_N ( "DRUN Write CACHE: start" );

    FILE *fd = fopen ( cache_file, "w" );
    if ( fd == NULL ) {
        g_warning ( "Failed to write to cache file" );
        return;
    }
    uint8_t version = CACHE_VERSION;
    fwrite ( &version, sizeof ( version ), 1, fd );

    fwrite ( &( pd->cmd_list_length ), sizeof ( pd->cmd_list_length ), 1, fd );
    for ( unsigned int index = 0; index < pd->cmd_list_length; index++ ) {
        DRunModeEntry *entry = &( pd->entry_list[index] );

        drun_write_str ( fd, entry->action );
        drun_write_str ( fd, entry->root );
        drun_write_str ( fd, entry->path );
        drun_write_str ( fd, entry->app_id );
        drun_write_str ( fd, entry->desktop_id );
        drun_write_str ( fd, entry->icon_name );
        drun_write_str ( fd, entry->exec );
        drun_write_str ( fd, entry->name );
        drun_write_str ( fd, entry->generic_name );

        drun_write_strv ( fd, entry->categories );
        drun_write_strv ( fd, entry->keywords );

        drun_write_str ( fd, entry->comment );
        drun_write_integer ( fd, (int32_t) entry->type );
    }

    fclose ( fd );
    TICK_N ( "DRUN Write CACHE: end" );
}

/**
 * Read cache file. returns FALSE when success.
 */
static gboolean drun_read_cache ( DRunModePrivateData *pd, const char *cache_file )
{
    if ( cache_file == NULL || config.drun_use_desktop_cache == FALSE ) {
        return TRUE;
    }

    if ( config.drun_reload_desktop_cache ) {
        return TRUE;
    }
    TICK_N ( "DRUN Read CACHE: start" );
    FILE *fd = fopen ( cache_file, "r" );
    if ( fd == NULL ) {
        TICK_N ( "DRUN Read CACHE: stop" );
        return TRUE;
    }

    // Read version.
    uint8_t version = 0;

    if ( fread ( &version, sizeof ( version ), 1, fd ) != 1 ) {
        fclose ( fd );
        g_warning ( "Cache corrupt, ignoring." );
        TICK_N ( "DRUN Read CACHE: stop" );
        return TRUE;
    }

    if ( version != CACHE_VERSION ) {
        fclose ( fd );
        g_warning ( "Cache file wrong version, ignoring." );
        TICK_N ( "DRUN Read CACHE: stop" );
        return TRUE;
    }

    if ( fread ( &( pd->cmd_list_length ), sizeof ( pd->cmd_list_length ), 1, fd ) != 1 ) {
        fclose ( fd );
        g_warning ( "Cache corrupt, ignoring." );
        TICK_N ( "DRUN Read CACHE: stop" );
        return TRUE;
    }
    // set actual length to length;
    pd->cmd_list_length_actual = pd->cmd_list_length;

    pd->entry_list = g_malloc0 ( pd->cmd_list_length_actual * sizeof ( *( pd->entry_list ) ) );

    for ( unsigned int index = 0; index < pd->cmd_list_length; index++ ) {
        DRunModeEntry *entry = &( pd->entry_list[index] );

        drun_read_string ( fd, &( entry->action ) );
        drun_read_string ( fd, &( entry->root ) );
        drun_read_string ( fd, &( entry->path ) );
        drun_read_string ( fd, &( entry->app_id ) );
        drun_read_string ( fd, &( entry->desktop_id ) );
        drun_read_string ( fd, &( entry->icon_name ) );
        drun_read_string ( fd, &( entry->exec ) );
        drun_read_string ( fd, &( entry->name ) );
        drun_read_string ( fd, &( entry->generic_name ) );

        drun_read_stringv ( fd, &( entry->categories ) );
        drun_read_stringv ( fd, &( entry->keywords ) );

        drun_read_string ( fd, &( entry->comment ) );
        int32_t type = 0;
        drun_read_integer ( fd, &( type ) );
        entry->type = type;
    }

    fclose ( fd );
    TICK_N ( "DRUN Read CACHE: stop" );
    return FALSE;
}

static void get_apps ( DRunModePrivateData *pd )
{
    char *cache_file = g_build_filename ( cache_dir, DRUN_DESKTOP_CACHE_FILE, NULL );
    TICK_N ( "Get Desktop apps (start)" );
    if ( drun_read_cache ( pd, cache_file ) ) {
        gchar *dir;
        // First read the user directory.
        dir = g_build_filename ( g_get_user_data_dir (), "applications", NULL );
        walk_dir ( pd, dir, dir );
        g_free ( dir );
        TICK_N ( "Get Desktop apps (user dir)" );
        // Then read thee system data dirs.
        const gchar * const * sys = g_get_system_data_dirs ();
        for ( const gchar * const *iter = sys; *iter != NULL; ++iter ) {
            gboolean unique = TRUE;
            // Stupid duplicate detection, better then walking dir.
            for ( const gchar *const *iterd = sys; iterd != iter; ++iterd ) {
                if ( g_strcmp0 ( *iter, *iterd ) == 0 ) {
                    unique = FALSE;
                }
            }
            // Check, we seem to be getting empty string...
            if ( unique && ( **iter ) != '\0' ) {
                dir = g_build_filename ( *iter, "applications", NULL );
                walk_dir ( pd, dir, dir );
                g_free ( dir );
            }
        }
        TICK_N ( "Get Desktop apps (system dirs)" );
        get_apps_history ( pd );

        g_qsort_with_data ( pd->entry_list, pd->cmd_list_length, sizeof ( DRunModeEntry ), drun_int_sort_list, NULL );

        TICK_N ( "Sorting done." );

        write_cache ( pd, cache_file );
    }
    g_free ( cache_file );
}

static void drun_mode_parse_entry_fields ()
{
    char               *savept = NULL;
    // Make a copy, as strtok will modify it.
    char               *switcher_str = g_strdup ( config.drun_match_fields );
    const char * const sep           = ",#";
    // Split token on ','. This modifies switcher_str.
    for ( unsigned int i = 0; i < DRUN_MATCH_NUM_FIELDS; i++ ) {
        matching_entry_fields[i].enabled = FALSE;
    }
    for ( char *token = strtok_r ( switcher_str, sep, &savept ); token != NULL;
          token = strtok_r ( NULL, sep, &savept ) ) {
        if ( strcmp ( token, "all" ) == 0 ) {
            for ( unsigned int i = 0; i < DRUN_MATCH_NUM_FIELDS; i++ ) {
                matching_entry_fields[i].enabled = TRUE;
            }
            break;
        }
        else {
            gboolean matched = FALSE;
            for ( unsigned int i = 0; i < DRUN_MATCH_NUM_FIELDS; i++ ) {
                const char * entry_name = matching_entry_fields[i].entry_field_name;
                if ( g_ascii_strcasecmp ( token, entry_name ) == 0 ) {
                    matching_entry_fields[i].enabled = TRUE;
                    matched                          = TRUE;
                }
            }
            if ( !matched ) {
                g_warning ( "Invalid entry name :%s", token );
            }
        }
    }
    // Free string that was modified by strtok_r
    g_free ( switcher_str );
}

static int drun_mode_init ( Mode *sw )
{
    if ( mode_get_private_data ( sw ) != NULL ) {
        return TRUE;
    }
    DRunModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
    pd->disabled_entries = g_hash_table_new_full ( g_str_hash, g_str_equal, g_free, NULL );
    mode_set_private_data ( sw, (void *) pd );
    // current destkop
    const char *current_desktop = g_getenv ( "XDG_CURRENT_DESKTOP" );
    pd->current_desktop_list = current_desktop ? g_strsplit ( current_desktop, ":", 0 ) : NULL;

    if ( config.drun_categories && *( config.drun_categories ) ) {
        pd->show_categories = g_strsplit ( config.drun_categories, ",", 0 );
    }

    drun_mode_parse_entry_fields ();
    get_apps ( pd );
    return TRUE;
}
static void drun_entry_clear ( DRunModeEntry *e )
{
    g_free ( e->root );
    g_free ( e->path );
    g_free ( e->app_id );
    g_free ( e->desktop_id );
    if ( e->icon != NULL ) {
        cairo_surface_destroy ( e->icon );
    }
    g_free ( e->icon_name );
    g_free ( e->exec );
    g_free ( e->name );
    g_free ( e->generic_name );
    g_free ( e->comment );
    if ( e->action != DRUN_GROUP_NAME ) {
        g_free ( e->action );
    }
    g_strfreev ( e->categories );
    g_strfreev ( e->keywords );
    if ( e->key_file ) {
        g_key_file_free ( e->key_file );
    }
}

static ModeMode drun_mode_result ( Mode *sw, int mretv, char **input, unsigned int selected_line )
{
    DRunModePrivateData *rmpd = (DRunModePrivateData *) mode_get_private_data ( sw );
    ModeMode            retv  = MODE_EXIT;

    if ( ( mretv & MENU_OK )  ) {
        switch ( rmpd->entry_list[selected_line].type )
        {
        case DRUN_DESKTOP_ENTRY_TYPE_APPLICATION:
            exec_cmd_entry ( &( rmpd->entry_list[selected_line] ) );
            break;
        case DRUN_DESKTOP_ENTRY_TYPE_LINK:
            launch_link_entry ( &( rmpd->entry_list[selected_line] ) );
            break;
        default:
            g_assert_not_reached ();
        }
    }
    else if ( ( mretv & MENU_CUSTOM_INPUT ) && *input != NULL && *input[0] != '\0' ) {
        RofiHelperExecuteContext context     = { .name = NULL };
        gboolean                 run_in_term = ( ( mretv & MENU_CUSTOM_ACTION ) == MENU_CUSTOM_ACTION );
        // FIXME: We assume startup notification in terminals, not in others
        if ( !helper_execute_command ( NULL, *input, run_in_term, run_in_term ? &context : NULL ) ) {
            retv = RELOAD_DIALOG;
        }
    }
    else if ( ( mretv & MENU_ENTRY_DELETE ) && selected_line < rmpd->cmd_list_length ) {
        // Possitive sort index means it is in history.
        if ( rmpd->entry_list[selected_line].sort_index >= 0 ) {
            delete_entry_history ( &( rmpd->entry_list[selected_line] ) );
            drun_entry_clear ( &( rmpd->entry_list[selected_line] ) );
            memmove ( &( rmpd->entry_list[selected_line] ), &rmpd->entry_list[selected_line + 1],
                      sizeof ( DRunModeEntry ) * ( rmpd->cmd_list_length - selected_line - 1 ) );
            rmpd->cmd_list_length--;
        }
        retv = RELOAD_DIALOG;
    } else if ( mretv & MENU_CUSTOM_COMMAND ) {
        retv = ( mretv & MENU_LOWER_MASK );
    }
    return retv;
}
static void drun_mode_destroy ( Mode *sw )
{
    DRunModePrivateData *rmpd = (DRunModePrivateData *) mode_get_private_data ( sw );
    if ( rmpd != NULL ) {
        for ( size_t i = 0; i < rmpd->cmd_list_length; i++ ) {
            drun_entry_clear ( &( rmpd->entry_list[i] ) );
        }
        g_hash_table_destroy ( rmpd->disabled_entries );
        g_free ( rmpd->entry_list );

        g_strfreev ( rmpd->current_desktop_list );
        g_strfreev ( rmpd->show_categories );
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
    DRunModeEntry *dr   = &( pd->entry_list[selected_line] );
    gchar         *cats = NULL;
    if ( dr->categories ) {
        char *tcats = g_strjoinv ( ",", dr->categories );
        if ( tcats ) {
            cats = g_markup_escape_text ( tcats, -1 );
            g_free ( tcats );
        }
    }
    gchar *keywords = NULL;
    if ( dr->keywords ) {
        char *tkeyw = g_strjoinv ( ",", dr->keywords );
        if ( tkeyw ) {
            keywords = g_markup_escape_text ( tkeyw, -1 );
            g_free ( tkeyw );
        }
    }
    // Needed for display.
    char *egn = NULL;
    char *en  = NULL;
    char *ec  = NULL;
    if ( dr->generic_name ) {
        egn = g_markup_escape_text ( dr->generic_name, -1 );
    }
    if ( dr->name ) {
        en = g_markup_escape_text ( dr->name, -1 );
    }
    if ( dr->comment ) {
        ec = g_markup_escape_text ( dr->comment, -1 );
    }

    char *retv = helper_string_replace_if_exists ( config.drun_display_format,
                                                   "{generic}", egn,
                                                   "{name}", en,
                                                   "{comment}", ec,
                                                   "{exec}", dr->exec,
                                                   "{categories}", cats,
                                                   "{keywords}", keywords,
                                                   NULL );
    g_free ( egn );
    g_free ( en );
    g_free ( ec );
    g_free ( cats );
    return retv;
}

static cairo_surface_t *_get_icon ( const Mode *sw, unsigned int selected_line, int height )
{
    DRunModePrivateData *pd = (DRunModePrivateData *) mode_get_private_data ( sw );
    g_return_val_if_fail ( pd->entry_list != NULL, NULL );
    DRunModeEntry       *dr = &( pd->entry_list[selected_line] );
    if ( dr->icon_name == NULL ) {
        return NULL;
    }
    if ( dr->icon_fetch_uid > 0 ) {
        return rofi_icon_fetcher_get ( dr->icon_fetch_uid );
    }
    dr->icon_fetch_uid = rofi_icon_fetcher_query ( dr->icon_name, height );
    return rofi_icon_fetcher_get ( dr->icon_fetch_uid );
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

static int drun_token_match ( const Mode *data, rofi_int_matcher **tokens, unsigned int index )
{
    DRunModePrivateData *rmpd = (DRunModePrivateData *) mode_get_private_data ( data );
    int                 match = 1;
    if ( tokens ) {
        for ( int j = 0; match && tokens != NULL && tokens[j] != NULL; j++ ) {
            int              test        = 0;
            rofi_int_matcher *ftokens[2] = { tokens[j], NULL };
            // Match name
            if ( matching_entry_fields[DRUN_MATCH_FIELD_NAME].enabled ) {
                if ( rmpd->entry_list[index].name ) {
                    test = helper_token_match ( ftokens, rmpd->entry_list[index].name );
                }
            }
            if ( matching_entry_fields[DRUN_MATCH_FIELD_GENERIC].enabled ) {
                // Match generic name
                if ( test == tokens[j]->invert && rmpd->entry_list[index].generic_name ) {
                    test = helper_token_match ( ftokens, rmpd->entry_list[index].generic_name );
                }
            }
            if ( matching_entry_fields[DRUN_MATCH_FIELD_EXEC].enabled ) {
                // Match executable name.
                if ( test == tokens[j]->invert && rmpd->entry_list[index].exec ) {
                    test = helper_token_match ( ftokens, rmpd->entry_list[index].exec );
                }
            }
            if ( matching_entry_fields[DRUN_MATCH_FIELD_CATEGORIES].enabled ) {
                // Match against category.
                if ( test == tokens[j]->invert ) {
                    gchar **list = rmpd->entry_list[index].categories;
                    for ( int iter = 0; test == tokens[j]->invert && list && list[iter]; iter++ ) {
                        test = helper_token_match ( ftokens, list[iter] );
                    }
                }
            }
            if ( matching_entry_fields[DRUN_MATCH_FIELD_KEYWORDS].enabled ) {
                // Match against category.
                if ( test == tokens[j]->invert ) {
                    gchar **list = rmpd->entry_list[index].keywords;
                    for ( int iter = 0; test == tokens[j]->invert && list && list[iter]; iter++ ) {
                        test = helper_token_match ( ftokens, list[iter] );
                    }
                }
            }
            if ( matching_entry_fields[DRUN_MATCH_FIELD_COMMENT].enabled ) {
                // Match executable name.
                if ( test == tokens[j]->invert && rmpd->entry_list[index].comment ) {
                    test = helper_token_match ( ftokens, rmpd->entry_list[index].comment );
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
