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
#define G_LOG_DOMAIN    "Dialogs.Script"

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include "rofi.h"
#include "dialogs/script.h"
#include "helper.h"

#include "widgets/textbox.h"

#include "mode-private.h"

#include "rofi-icon-fetcher.h"

#include "dialogs/dmenuscriptshared.h"

typedef struct
{
    /** ID of the current script. */
    unsigned int           id;
    /** List of visible items. */
    DmenuScriptEntry       *cmd_list;
    /** length list of visible items. */
    unsigned int           cmd_list_length;

    /** Urgent list */
    struct rofi_range_pair * urgent_list;
    unsigned int           num_urgent_list;
    /** Active list */
    struct rofi_range_pair * active_list;
    unsigned int           num_active_list;
    /** Configuration settings. */
    char                   *message;
    char                   *prompt;
    gboolean               do_markup;
    char                   delim;
    /** no custom */
    gboolean               no_custom;
} ScriptModePrivateData;

/**
 * Shared function between DMENU and Script mode.
 */
void dmenuscript_parse_entry_extras ( G_GNUC_UNUSED Mode *sw, DmenuScriptEntry *entry, char *buffer, size_t length )
{
    gchar **extras = g_strsplit ( buffer, "\x1f", -1 );
    gchar **extra;
    for ( extra = extras; *extra != NULL && *( extra + 1 ) != NULL; extra += 2 ) {
        gchar *key   = *extra;
        gchar *value = *( extra + 1 );
        if ( strcasecmp ( key, "icon" ) == 0 ) {
            entry->icon_name = value;
        }
        else if ( strcasecmp ( key, "meta" ) == 0 ) {
            entry->meta = value;
        }
        else if ( strcasecmp ( key, "info" ) == 0 ) {
            entry->info = value;
        }
        else if ( strcasecmp ( key, "nonselectable" ) == 0 ) {
            entry->nonselectable = strcasecmp ( value, "true" ) == 0;
            g_free ( value );
        }
        else {
            g_free ( value );
        }
        g_free ( key );
    }
    g_free ( extras );
}

/**
 * End of shared functions.
 */

static void parse_header_entry ( Mode *sw, char *line, ssize_t length )
{
    ScriptModePrivateData *pd        = (ScriptModePrivateData *) sw->private_data;
    ssize_t               length_key = 0;//strlen ( line );
    while ( length_key < length && line[length_key] != '\x1f' ) {
        length_key++;
    }

    if ( ( length_key + 1 ) < length ) {
        line[length_key] = '\0';
        char *value = line + length_key + 1;
        if ( strcasecmp ( line, "message" ) == 0 ) {
            g_free ( pd->message );
            pd->message = strlen ( value ) ? g_strdup ( value ) : NULL;
        }
        else if ( strcasecmp ( line, "prompt" ) == 0 ) {
            g_free ( pd->prompt );
            pd->prompt       = g_strdup  ( value );
            sw->display_name = pd->prompt;
        }
        else if ( strcasecmp ( line, "markup-rows" ) == 0 ) {
            pd->do_markup = ( strcasecmp ( value, "true" ) == 0 );
        }
        else if ( strcasecmp ( line, "urgent" ) == 0 ) {
            parse_ranges ( value, &( pd->urgent_list ), &( pd->num_urgent_list ) );
        }
        else if ( strcasecmp ( line, "active" ) == 0 ) {
            parse_ranges ( value, &( pd->active_list ), &( pd->num_active_list ) );
        }
        else if ( strcasecmp ( line, "delim" ) == 0 ) {
            pd->delim = helper_parse_char ( value );
        }
        else if ( strcasecmp ( line, "no-custom" ) == 0 ) {
            pd->no_custom = ( strcasecmp ( value, "true" ) == 0 );
        }
    }
}

static DmenuScriptEntry *execute_executor ( Mode *sw, char *arg, unsigned int *length, int value, DmenuScriptEntry *entry )
{
    ScriptModePrivateData *pd    = (ScriptModePrivateData *) sw->private_data;
    int                   fd     = -1;
    GError                *error = NULL;
    DmenuScriptEntry      *retv  = NULL;
    char                  **argv = NULL;
    int                   argc   = 0;
    *length = 0;

    // Environment
    char ** env = g_get_environ ();

    char *str_value = g_strdup_printf ( "%d", value );
    env = g_environ_setenv ( env, "ROFI_RETV", str_value, TRUE );
    g_free ( str_value );

    str_value = g_strdup_printf ( "%d", (int) getpid () );
    env       = g_environ_setenv ( env, "ROFI_OUTSIDE", str_value, TRUE );
    g_free ( str_value );

    if ( entry && entry->info ) {
        env = g_environ_setenv ( env, "ROFI_INFO", entry->info, TRUE );
    }

    if ( g_shell_parse_argv ( sw->ed, &argc, &argv, &error ) ) {
        argv           = g_realloc ( argv, ( argc + 2 ) * sizeof ( char* ) );
        argv[argc]     = g_strdup ( arg );
        argv[argc + 1] = NULL;
        g_spawn_async_with_pipes ( NULL, argv, env, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL, &fd, NULL, &error );
    }
    g_strfreev ( env );
    if ( error != NULL ) {
        char *msg = g_strdup_printf ( "Failed to execute: '%s'\nError: '%s'", (char *) sw->ed, error->message );
        rofi_view_error_dialog ( msg, FALSE );
        g_free ( msg );
        // print error.
        g_error_free ( error );
        fd = -1;
    }
    if ( fd >= 0 ) {
        FILE *inp = fdopen ( fd, "r" );
        if ( inp ) {
            char    *buffer       = NULL;
            size_t  buffer_length = 0;
            ssize_t read_length   = 0;
            size_t  actual_size   = 0;
            while ( ( read_length = getdelim ( &buffer, &buffer_length, pd->delim, inp ) ) > 0 ) {
                // Filter out line-end.
                if ( buffer[read_length - 1] == pd->delim ) {
                    buffer[read_length - 1] = '\0';
                }
                if ( buffer[0] == '\0' ) {
                    parse_header_entry ( sw, &buffer[1], read_length - 1 );
                }
                else {
                    if ( actual_size < ( ( *length ) + 2 ) ) {
                        actual_size += 256;
                        retv         = g_realloc ( retv, ( actual_size ) * sizeof ( DmenuScriptEntry ) );
                    }
                    size_t buf_length = strlen ( buffer ) + 1;
                    retv[( *length )].entry          = g_memdup ( buffer, buf_length );
                    retv[( *length )].icon_name      = NULL;
                    retv[( *length )].meta           = NULL;
                    retv[( *length )].info           = NULL;
                    retv[( *length )].icon_fetch_uid = 0;
                    retv[( *length )].nonselectable  = FALSE;
                    if ( buf_length > 0 && ( read_length > (ssize_t) buf_length )  ) {
                        dmenuscript_parse_entry_extras ( sw, &( retv[( *length )] ), buffer + buf_length, read_length - buf_length );
                    }
                    retv[( *length ) + 1].entry = NULL;
                    ( *length )++;
                }
            }
            if ( buffer ) {
                free ( buffer );
            }
            if ( fclose ( inp ) != 0 ) {
                g_warning ( "Failed to close stdout off executor script: '%s'",
                            g_strerror ( errno ) );
            }
        }
    }
    g_strfreev ( argv );
    return retv;
}

static void script_switcher_free ( Mode *sw )
{
    if ( sw == NULL ) {
        return;
    }
    g_free ( sw->name );
    g_free ( sw->ed );
    g_free ( sw );
}

static int script_mode_init ( Mode *sw )
{
    if ( sw->private_data == NULL ) {
        ScriptModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
        pd->delim        = '\n';
        sw->private_data = (void *) pd;
        pd->cmd_list     = execute_executor ( sw, NULL, &( pd->cmd_list_length ), 0, NULL );
    }
    return TRUE;
}
static unsigned int script_mode_get_num_entries ( const Mode *sw )
{
    const ScriptModePrivateData *rmpd = (const ScriptModePrivateData *) sw->private_data;
    return rmpd->cmd_list_length;
}

static void script_mode_reset_highlight ( Mode *sw )
{
    ScriptModePrivateData *rmpd = (ScriptModePrivateData *) sw->private_data;

    rmpd->num_urgent_list = 0;
    g_free ( rmpd->urgent_list );
    rmpd->urgent_list     = NULL;
    rmpd->num_active_list = 0;
    g_free ( rmpd->active_list );
    rmpd->active_list = NULL;
}

static ModeMode script_mode_result ( Mode *sw, int mretv, char **input, unsigned int selected_line )
{
    ScriptModePrivateData *rmpd      = (ScriptModePrivateData *) sw->private_data;
    ModeMode              retv       = MODE_EXIT;
    DmenuScriptEntry      *new_list  = NULL;
    unsigned int          new_length = 0;

    if ( ( mretv & MENU_CUSTOM_COMMAND ) ) {
        //retv = 1+( mretv & MENU_LOWER_MASK );
        script_mode_reset_highlight ( sw );
        if ( selected_line != UINT32_MAX ) {
            new_list = execute_executor ( sw, rmpd->cmd_list[selected_line].entry, &new_length, 10 + ( mretv & MENU_LOWER_MASK ), &( rmpd->cmd_list[selected_line] ) );
        }
        else {
            if ( rmpd->no_custom == FALSE ) {
                new_list = execute_executor ( sw, *input, &new_length, 10 + ( mretv & MENU_LOWER_MASK ), NULL );
            }
            else {
                return RELOAD_DIALOG;
            }
        }
    }
    else if ( ( mretv & MENU_OK ) && rmpd->cmd_list[selected_line].entry != NULL ) {
        if ( rmpd->cmd_list[selected_line].nonselectable ) {
            return RELOAD_DIALOG;
        }
        script_mode_reset_highlight ( sw );
        new_list = execute_executor ( sw, rmpd->cmd_list[selected_line].entry, &new_length, 1, &( rmpd->cmd_list[selected_line] ) );
    }
    else if ( ( mretv & MENU_CUSTOM_INPUT ) && *input != NULL && *input[0] != '\0' ) {
        if ( rmpd->no_custom == FALSE ) {
            script_mode_reset_highlight ( sw );
            new_list = execute_executor ( sw, *input, &new_length, 2, NULL );
        }
        else {
            return RELOAD_DIALOG;
        }
    }

    // If a new list was generated, use that an loop around.
    if ( new_list != NULL ) {
        for ( unsigned int i = 0; i < rmpd->cmd_list_length; i++ ) {
            g_free ( rmpd->cmd_list[i].entry );
            g_free ( rmpd->cmd_list[i].icon_name );
            g_free ( rmpd->cmd_list[i].meta );
        }
        g_free ( rmpd->cmd_list );

        rmpd->cmd_list        = new_list;
        rmpd->cmd_list_length = new_length;
        retv                  = RESET_DIALOG;
    }
    return retv;
}

static void script_mode_destroy ( Mode *sw )
{
    ScriptModePrivateData *rmpd = (ScriptModePrivateData *) sw->private_data;
    if ( rmpd != NULL ) {
        for ( unsigned int i = 0; i < rmpd->cmd_list_length; i++ ) {
            g_free ( rmpd->cmd_list[i].entry );
            g_free ( rmpd->cmd_list[i].icon_name );
            g_free ( rmpd->cmd_list[i].meta );
        }
        g_free ( rmpd->cmd_list );
        g_free ( rmpd->message );
        g_free ( rmpd->prompt );
        g_free ( rmpd->urgent_list );
        g_free ( rmpd->active_list );
        g_free ( rmpd );
        sw->private_data = NULL;
    }
}
static inline unsigned int get_index ( unsigned int length, int index )
{
    if ( index >= 0 ) {
        return index;
    }
    if ( ( (unsigned int) -index ) <= length ) {
        return length + index;
    }
    // Out of range.
    return UINT_MAX;
}
static char *_get_display_value ( const Mode *sw, unsigned int selected_line, G_GNUC_UNUSED int *state, G_GNUC_UNUSED GList **list, int get_entry )
{
    ScriptModePrivateData *pd = sw->private_data;
    for ( unsigned int i = 0; i < pd->num_active_list; i++ ) {
        unsigned int start = get_index ( pd->cmd_list_length, pd->active_list[i].start );
        unsigned int stop  = get_index ( pd->cmd_list_length, pd->active_list[i].stop );
        if ( selected_line >= start && selected_line <= stop ) {
            *state |= ACTIVE;
        }
    }
    for ( unsigned int i = 0; i < pd->num_urgent_list; i++ ) {
        unsigned int start = get_index ( pd->cmd_list_length, pd->urgent_list[i].start );
        unsigned int stop  = get_index ( pd->cmd_list_length, pd->urgent_list[i].stop );
        if ( selected_line >= start && selected_line <= stop ) {
            *state |= URGENT;
        }
    }
    if ( pd->do_markup ) {
        *state |= MARKUP;
    }
    return get_entry ? g_strdup ( pd->cmd_list[selected_line].entry ) : NULL;
}

static int script_token_match ( const Mode *sw, rofi_int_matcher **tokens, unsigned int index )
{
    ScriptModePrivateData *rmpd = sw->private_data;
    int                   match = 1;
    if ( tokens ) {
        for ( int j = 0; match && tokens != NULL && tokens[j] != NULL; j++ ) {
            rofi_int_matcher *ftokens[2] = { tokens[j], NULL };
            int              test        = 0;
            test = helper_token_match ( ftokens, rmpd->cmd_list[index].entry );
            if ( test == tokens[j]->invert && rmpd->cmd_list[index].meta ) {
                test = helper_token_match ( ftokens, rmpd->cmd_list[index].meta );
            }

            if ( test == 0 ) {
                match = 0;
            }
        }
    }
    return match;
}
static char *script_get_message ( const Mode *sw )
{
    ScriptModePrivateData *pd = sw->private_data;
    return g_strdup ( pd->message );
}
static cairo_surface_t *script_get_icon ( const Mode *sw, unsigned int selected_line, int height )
{
    ScriptModePrivateData *pd = (ScriptModePrivateData *) mode_get_private_data ( sw );
    g_return_val_if_fail ( pd->cmd_list != NULL, NULL );
    DmenuScriptEntry      *dr = &( pd->cmd_list[selected_line] );
    if ( dr->icon_name == NULL ) {
        return NULL;
    }
    if ( dr->icon_fetch_uid > 0 ) {
        return rofi_icon_fetcher_get ( dr->icon_fetch_uid );
    }
    dr->icon_fetch_uid = rofi_icon_fetcher_query ( dr->icon_name, height );
    return rofi_icon_fetcher_get ( dr->icon_fetch_uid );
}

#include "mode-private.h"
Mode *script_switcher_parse_setup ( const char *str )
{
    Mode              *sw    = g_malloc0 ( sizeof ( *sw ) );
    char              *endp  = NULL;
    char              *parse = g_strdup ( str );
    unsigned int      index  = 0;
    const char *const sep    = ":";
    for ( char *token = strtok_r ( parse, sep, &endp ); token != NULL; token = strtok_r ( NULL, sep, &endp ) ) {
        if ( index == 0 ) {
            sw->name = g_strdup ( token );
        }
        else if ( index == 1 ) {
            sw->ed = (void *) rofi_expand_path ( token );
        }
        index++;
    }
    g_free ( parse );
    if ( index == 2 ) {
        sw->free               = script_switcher_free;
        sw->_init              = script_mode_init;
        sw->_get_num_entries   = script_mode_get_num_entries;
        sw->_result            = script_mode_result;
        sw->_destroy           = script_mode_destroy;
        sw->_token_match       = script_token_match;
        sw->_get_message       = script_get_message;
        sw->_get_icon          = script_get_icon;
        sw->_get_completion    = NULL,
        sw->_preprocess_input  = NULL,
        sw->_get_display_value = _get_display_value;

        return sw;
    }
    fprintf ( stderr, "The script command '%s' has %u options, but needs 2: <name>:<script>.", str, index );
    script_switcher_free ( sw );
    return NULL;
}

gboolean script_switcher_is_valid ( const char *token )
{
    return strchr ( token, ':' ) != NULL;
}
