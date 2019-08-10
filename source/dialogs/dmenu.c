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

/** The log domain of this dialog. */
#define G_LOG_DOMAIN    "Dialogs.DMenu"

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <errno.h>
#include <gio/gio.h>
#include <gio/gunixinputstream.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "rofi.h"
#include "settings.h"
#include "widgets/textbox.h"
#include "dialogs/dmenu.h"
#include "helper.h"
#include "xrmoptions.h"
#include "view.h"
#include "rofi-icon-fetcher.h"

#include "dialogs/dmenuscriptshared.h"


static int dmenu_mode_init ( Mode *sw );
static int dmenu_token_match ( const Mode *sw, rofi_int_matcher **tokens, unsigned int index );
static cairo_surface_t *dmenu_get_icon ( const Mode *sw, unsigned int selected_line, int height );
static char *dmenu_get_message ( const Mode *sw );

static inline unsigned int bitget ( uint32_t *array, unsigned int index )
{
    uint32_t bit = index % 32;
    uint32_t val = array[index / 32];
    return ( val >> bit ) & 1;
}

static inline void bittoggle ( uint32_t *array, unsigned int index )
{
    uint32_t bit = index % 32;
    uint32_t *v  = &array[index / 32];
    *v ^= 1 << bit;
}

typedef struct
{
    /** Settings */
    // Separator.
    char                   separator;

    unsigned int           selected_line;
    char                   *message;
    char                   *format;
    struct rofi_range_pair * urgent_list;
    unsigned int           num_urgent_list;
    struct rofi_range_pair * active_list;
    unsigned int           num_active_list;
    uint32_t               *selected_list;
    unsigned int           num_selected_list;
    unsigned int           do_markup;
    // List with entries.
    DmenuScriptEntry         *cmd_list;
    unsigned int           cmd_list_real_length;
    unsigned int           cmd_list_length;
    unsigned int           only_selected;
    unsigned int           selected_count;

    gchar                  **columns;
    gchar                  *column_separator;
    gboolean               multi_select;

    GCancellable           *cancel;
    gulong                 cancel_source;
    GInputStream           *input_stream;
    GDataInputStream       *data_input_stream;
} DmenuModePrivateData;

static void async_close_callback ( GObject *source_object, GAsyncResult *res, G_GNUC_UNUSED gpointer user_data )
{
    g_input_stream_close_finish ( G_INPUT_STREAM ( source_object ), res, NULL );
    g_debug ( "Closing data stream." );
}

static void read_add ( DmenuModePrivateData * pd, char *data, gsize len )
{
    gsize data_len = len;
    if ( ( pd->cmd_list_length + 2 ) > pd->cmd_list_real_length ) {
        pd->cmd_list_real_length = MAX ( pd->cmd_list_real_length * 2, 512 );
        pd->cmd_list             = g_realloc ( pd->cmd_list, ( pd->cmd_list_real_length ) * sizeof ( DmenuScriptEntry ) );
    }
    // Init.
    pd->cmd_list[pd->cmd_list_length].icon_fetch_uid = 0;
    pd->cmd_list[pd->cmd_list_length].icon_name      = NULL;
    char *end = strchr(data, '\0');
    if ( end != NULL ) {
        data_len = end-data;
        dmenuscript_parse_entry_extras ( NULL, &(pd->cmd_list[pd->cmd_list_length]), end+1, len-data_len);
    }
    char *utfstr = rofi_force_utf8 ( data, data_len );
    pd->cmd_list[pd->cmd_list_length].entry      = utfstr;
    pd->cmd_list[pd->cmd_list_length + 1].entry  = NULL;

    pd->cmd_list_length++;
}
static void async_read_callback ( GObject *source_object, GAsyncResult *res, gpointer user_data )
{
    GDataInputStream     *stream = (GDataInputStream *) source_object;
    DmenuModePrivateData *pd     = (DmenuModePrivateData *) user_data;
    gsize                len;
    char                 *data = g_data_input_stream_read_upto_finish ( stream, res, &len, NULL );
    if ( data != NULL ) {
        // Absorb separator, already in buffer so should not block.
        g_data_input_stream_read_byte ( stream, NULL, NULL );
        read_add ( pd, data, len );
        g_free ( data );
        rofi_view_reload ();

        g_data_input_stream_read_upto_async ( pd->data_input_stream, &( pd->separator ), 1, G_PRIORITY_LOW, pd->cancel,
                                              async_read_callback, pd );
        return;
    }
    else {
        GError *error = NULL;
        // Absorb separator, already in buffer so should not block.
        // If error == NULL end of stream..
        g_data_input_stream_read_byte ( stream, NULL, &error );
        if (  error == NULL ) {
            // Add empty line.
            read_add ( pd, "", 0 );
            rofi_view_reload ();

            g_data_input_stream_read_upto_async ( pd->data_input_stream, &( pd->separator ), 1, G_PRIORITY_LOW, pd->cancel,
                                                  async_read_callback, pd );
            return;
        }
        else {
            g_error_free ( error );
        }
    }
    if ( !g_cancellable_is_cancelled ( pd->cancel ) ) {
        // Hack, don't use get active.
        g_debug ( "Clearing overlay" );
        rofi_view_set_overlay ( rofi_view_get_active (), NULL );
        g_input_stream_close_async ( G_INPUT_STREAM ( stream ), G_PRIORITY_LOW, pd->cancel, async_close_callback, pd );
    }
}

static void async_read_cancel ( G_GNUC_UNUSED GCancellable *cancel, G_GNUC_UNUSED gpointer data )
{
    g_debug ( "Cancelled the async read." );
}

static int get_dmenu_async ( DmenuModePrivateData *pd, int sync_pre_read )
{
    while ( sync_pre_read-- ) {
        gsize len   = 0;
        char  *data = g_data_input_stream_read_upto ( pd->data_input_stream, &( pd->separator ), 1, &len, NULL, NULL );
        if ( data == NULL ) {
            g_input_stream_close_async ( G_INPUT_STREAM ( pd->input_stream ), G_PRIORITY_LOW, pd->cancel, async_close_callback, pd );
            return FALSE;
        }
        g_data_input_stream_read_byte ( pd->data_input_stream, NULL, NULL );
        read_add ( pd, data, len );
        g_free ( data );
    }
    g_data_input_stream_read_upto_async ( pd->data_input_stream, &( pd->separator ), 1, G_PRIORITY_LOW, pd->cancel,
                                          async_read_callback, pd );
    return TRUE;
}
static void get_dmenu_sync ( DmenuModePrivateData *pd )
{
    while  ( TRUE ) {
        gsize len   = 0;
        char  *data = g_data_input_stream_read_upto ( pd->data_input_stream, &( pd->separator ), 1, &len, NULL, NULL );
        if ( data == NULL ) {
            break;
        }
        g_data_input_stream_read_byte ( pd->data_input_stream, NULL, NULL );
        read_add ( pd, data, len );
        g_free ( data );
    }
    g_input_stream_close_async ( G_INPUT_STREAM ( pd->input_stream ), G_PRIORITY_LOW, pd->cancel, async_close_callback, pd );
}

static unsigned int dmenu_mode_get_num_entries ( const Mode *sw )
{
    const DmenuModePrivateData *rmpd = (const DmenuModePrivateData *) mode_get_private_data ( sw );
    return rmpd->cmd_list_length;
}

static gchar * dmenu_format_output_string ( const DmenuModePrivateData *pd, const char *input )
{
    if ( pd->columns == NULL ) {
        return g_strdup ( input );
    }
    char     *retv       = NULL;
    char     ** splitted = g_regex_split_simple ( pd->column_separator, input, G_REGEX_CASELESS, 00 );
    uint32_t ns          = 0;
    for (; splitted && splitted[ns]; ns++ ) {
        ;
    }
    for ( uint32_t i = 0; pd->columns && pd->columns[i]; i++ ) {
        unsigned int index = (unsigned int ) g_ascii_strtoull ( pd->columns[i], NULL, 10 );
        if ( index < ns && index > 0 ) {
            if ( retv == NULL ) {
                retv = g_strdup ( splitted[index - 1] );
            }
            else {
                gchar *t = g_strjoin ( "\t", retv, splitted[index - 1], NULL );
                g_free ( retv );
                retv = t;
            }
        }
    }
    g_strfreev ( splitted );
    return retv ? retv : g_strdup ( "" );
}

static inline unsigned int get_index ( unsigned int length, int index )
{
    if ( index >= 0 ) return index;
    if ( ((unsigned int)-index) <= length ) return (length+index);
    // Out of range.
    return UINT_MAX;
}

static char *get_display_data ( const Mode *data, unsigned int index, int *state, G_GNUC_UNUSED GList **list, int get_entry )
{
    Mode                 *sw    = (Mode *) data;
    DmenuModePrivateData *pd    = (DmenuModePrivateData *) mode_get_private_data ( sw );
    DmenuScriptEntry    *retv = (DmenuScriptEntry *) pd->cmd_list;
    for ( unsigned int i = 0; i < pd->num_active_list; i++ ) {
        unsigned int start = get_index ( pd->cmd_list_length, pd->active_list[i].start );
        unsigned int stop  = get_index ( pd->cmd_list_length, pd->active_list[i].stop );
        if ( index >= start && index <= stop ) {
            *state |= ACTIVE;
        }
    }
    for ( unsigned int i = 0; i < pd->num_urgent_list; i++ ) {
        unsigned int start = get_index ( pd->cmd_list_length, pd->urgent_list[i].start );
        unsigned int stop  = get_index ( pd->cmd_list_length, pd->urgent_list[i].stop );
        if ( index >= start && index <= stop ) {
            *state |= URGENT;
        }
    }
    if ( pd->selected_list && bitget ( pd->selected_list, index ) == TRUE ) {
        *state |= SELECTED;
    }
    if ( pd->do_markup ) {
        *state |= MARKUP;
    }
    return get_entry ? dmenu_format_output_string ( pd, retv[index].entry ) : NULL;
}

static void dmenu_mode_free ( Mode *sw )
{
    if ( mode_get_private_data ( sw ) == NULL ) {
        return;
    }
    DmenuModePrivateData *pd = (DmenuModePrivateData *) mode_get_private_data ( sw );
    if ( pd != NULL ) {
        if ( pd->cancel  ) {
            // If open, cancel reads.
            if ( pd->input_stream && !g_input_stream_is_closed ( pd->input_stream ) ) {
                g_cancellable_cancel ( pd->cancel );
            }
            // This blocks until cancel is done.
            g_cancellable_disconnect ( pd->cancel, pd->cancel_source );
            if ( pd->input_stream ) {
                // Should close the stream if not yet done.
                g_object_unref ( pd->data_input_stream );
                g_object_unref ( pd->input_stream );
            }
            g_object_unref ( pd->cancel );
        }

        for ( size_t i = 0; i < pd->cmd_list_length; i++ ) {
            if ( pd->cmd_list[i].entry ) {
                g_free ( pd->cmd_list[i].entry );
                g_free ( pd->cmd_list[i].icon_name );
            }
        }
        g_free ( pd->cmd_list );
        g_free ( pd->urgent_list );
        g_free ( pd->active_list );
        g_free ( pd->selected_list );

        g_free ( pd );
        mode_set_private_data ( sw, NULL );
    }
}

#include "mode-private.h"
/** dmenu Mode object. */
Mode dmenu_mode =
{
    .name               = "dmenu",
    .cfg_name_key       = "display-combi",
    ._init              = dmenu_mode_init,
    ._get_num_entries   = dmenu_mode_get_num_entries,
    ._result            = NULL,
    ._destroy           = dmenu_mode_free,
    ._token_match       = dmenu_token_match,
    ._get_display_value = get_display_data,
    ._get_icon          = dmenu_get_icon,
    ._get_completion    = NULL,
    ._preprocess_input  = NULL,
    ._get_message       = dmenu_get_message,
    .private_data       = NULL,
    .free               = NULL,
    .display_name       = "dmenu"
};

static int dmenu_mode_init ( Mode *sw )
{
    if ( mode_get_private_data ( sw ) != NULL ) {
        return TRUE;
    }
    mode_set_private_data ( sw, g_malloc0 ( sizeof ( DmenuModePrivateData ) ) );
    DmenuModePrivateData *pd = (DmenuModePrivateData *) mode_get_private_data ( sw );

    pd->separator     = '\n';
    pd->selected_line = UINT32_MAX;

    find_arg_str ( "-mesg", &( pd->message ) );

    // Input data separator.
    find_arg_char ( "-sep", &( pd->separator ) );

    find_arg_uint (  "-selected-row", &( pd->selected_line ) );
    // By default we print the unescaped line back.
    pd->format = "s";

    // Allow user to override the output format.
    find_arg_str ( "-format", &( pd->format ) );
    // Urgent.
    char *str = NULL;
    find_arg_str (  "-u", &str );
    if ( str != NULL ) {
        parse_ranges ( str, &( pd->urgent_list ), &( pd->num_urgent_list ) );
    }
    // Active
    str = NULL;
    find_arg_str (  "-a", &str );
    if ( str != NULL ) {
        parse_ranges ( str, &( pd->active_list ), &( pd->num_active_list ) );
    }

    // DMENU COMPATIBILITY
    find_arg_uint (  "-l", &( config.menu_lines ) );

    str = NULL;
    find_arg_str ( "-window-title", &str );
    if ( str ) {
        dmenu_mode.display_name = str;
    }

    /**
     * Dmenu compatibility.
     * `-b` put on bottom.
     */
    if ( find_arg ( "-b" ) >= 0 ) {
        config.location = 6;
    }
    /* -i case insensitive */
    config.case_sensitive = TRUE;
    if ( find_arg ( "-i" ) >= 0 ) {
        config.case_sensitive = FALSE;
    }
    int fd = STDIN_FILENO;
    str = NULL;
    if ( find_arg_str ( "-input", &str ) ) {
        char *estr = rofi_expand_path ( str );
        fd = open ( str, O_RDONLY );
        if ( fd < 0 ) {
            char *msg = g_markup_printf_escaped ( "Failed to open file: <b>%s</b>:\n\t<i>%s</i>", estr, g_strerror ( errno ) );
            rofi_view_error_dialog ( msg, TRUE );
            g_free ( msg );
            g_free ( estr );
            return TRUE;
        }
        g_free ( estr );
    }
    // If input is stdin, and a tty, do not read as rofi grabs input and therefor blocks.
    if ( !( fd == STDIN_FILENO && isatty ( fd ) == 1 ) ) {
        pd->cancel            = g_cancellable_new ();
        pd->cancel_source     = g_cancellable_connect ( pd->cancel, G_CALLBACK ( async_read_cancel ), pd, NULL );
        pd->input_stream      = g_unix_input_stream_new ( fd, fd != STDIN_FILENO );
        pd->data_input_stream = g_data_input_stream_new ( pd->input_stream );
    }
    gchar *columns = NULL;
    if ( find_arg_str ( "-display-columns", &columns ) ) {
        pd->columns          = g_strsplit ( columns, ",", 0 );
        pd->column_separator = "\t";
        find_arg_str ( "-display-column-separator", &pd->column_separator );
    }
    return TRUE;
}

static int dmenu_token_match ( const Mode *sw, rofi_int_matcher **tokens, unsigned int index )
{
    DmenuModePrivateData *rmpd = (DmenuModePrivateData *) mode_get_private_data ( sw );
    if ( rmpd->do_markup) {
        /** Strip out the markup when matching. */
        char *esc = NULL;
        pango_parse_markup(rmpd->cmd_list[index].entry, -1, 0, NULL, &esc, NULL, NULL);
        if ( esc ) {
            int retv = helper_token_match ( tokens, esc);
            g_free (esc);
            return retv;
        }
        return FALSE;

    } else {
        return helper_token_match ( tokens, rmpd->cmd_list[index].entry );
    }
}
static char *dmenu_get_message ( const Mode *sw )
{
    DmenuModePrivateData *pd = (DmenuModePrivateData *) mode_get_private_data ( sw );
    if ( pd->message ) {
        return g_strdup ( pd->message );
    }
    return NULL;
}
static cairo_surface_t *dmenu_get_icon ( const Mode *sw, unsigned int selected_line, int height )
{
    DmenuModePrivateData *pd = (DmenuModePrivateData *) mode_get_private_data ( sw );
    g_return_val_if_fail ( pd->cmd_list != NULL, NULL );
    DmenuScriptEntry *dr = &( pd->cmd_list[selected_line] );
    if ( dr->icon_name == NULL ) {
        return NULL;
    }
    if ( dr->icon_fetch_uid > 0 ) {
        return rofi_icon_fetcher_get ( dr->icon_fetch_uid );
    }
    dr->icon_fetch_uid = rofi_icon_fetcher_query ( dr->icon_name, height );
    return rofi_icon_fetcher_get ( dr->icon_fetch_uid );
}


static void dmenu_finish ( RofiViewState *state, int retv )
{
    if ( retv == FALSE ) {
        rofi_set_return_code ( EXIT_FAILURE );
    }
    else if ( retv >= 10 ) {
        rofi_set_return_code ( retv );
    }
    else{
        rofi_set_return_code ( EXIT_SUCCESS );
    }
    rofi_view_set_active ( NULL );
    rofi_view_free ( state );
    mode_destroy ( &dmenu_mode );
}

static void dmenu_print_results ( DmenuModePrivateData *pd, const char *input )
{
    DmenuScriptEntry *cmd_list = pd->cmd_list;
    int  seen       = FALSE;
    if ( pd->selected_list != NULL ) {
        for ( unsigned int st = 0; st < pd->cmd_list_length; st++ ) {
            if ( bitget ( pd->selected_list, st ) ) {
                seen = TRUE;
                rofi_output_formatted_line ( pd->format, cmd_list[st].entry , st, input );
            }
        }
    }
    if ( !seen ) {
        const char *cmd = input;
        if ( pd->selected_line != UINT32_MAX ) {
            cmd = cmd_list[pd->selected_line].entry ;
        }
        rofi_output_formatted_line ( pd->format, cmd, pd->selected_line, input );
    }
}

static void dmenu_finalize ( RofiViewState *state )
{
    int                  retv            = FALSE;
    DmenuModePrivateData *pd             = (DmenuModePrivateData *) rofi_view_get_mode ( state )->private_data;
    unsigned int         cmd_list_length = pd->cmd_list_length;
    DmenuScriptEntry       *cmd_list      = pd->cmd_list;

    char                 *input = g_strdup ( rofi_view_get_user_input ( state ) );
    pd->selected_line = rofi_view_get_selected_line ( state );;
    MenuReturn           mretv    = rofi_view_get_return_value ( state );
    unsigned int         next_pos = rofi_view_get_next_position ( state );
    int                  restart  = 0;
    // Special behavior.
    if ( pd->only_selected ) {
        /**
         * Select item mode.
         */
        restart = 1;
        // Skip if no valid item is selected.
        if ( ( mretv & MENU_CANCEL ) == MENU_CANCEL ) {
            // In no custom mode we allow canceling.
            restart = ( find_arg ( "-only-match" ) >= 0 );
        }
        else if ( pd->selected_line != UINT32_MAX ) {
            if ( ( mretv & MENU_CUSTOM_ACTION ) && pd->multi_select ) {
                restart = TRUE;
                if ( pd->selected_list == NULL ) {
                    pd->selected_list = g_malloc0 ( sizeof ( uint32_t ) * ( pd->cmd_list_length / 32 + 1 ) );
                }
                pd->selected_count += ( bitget ( pd->selected_list, pd->selected_line ) ? ( -1 ) : ( 1 ) );
                bittoggle ( pd->selected_list, pd->selected_line );
                // Move to next line.
                pd->selected_line = MIN ( next_pos, cmd_list_length - 1 );
                if ( pd->selected_count > 0 ) {
                    char *str = g_strdup_printf ( "%u/%u", pd->selected_count, pd->cmd_list_length );
                    rofi_view_set_overlay ( state, str );
                    g_free ( str );
                }
                else {
                    rofi_view_set_overlay ( state, NULL );
                }
            }
            else if ( ( mretv & ( MENU_OK | MENU_QUICK_SWITCH ) ) && cmd_list[pd->selected_line].entry != NULL ) {
                dmenu_print_results ( pd, input );
                retv = TRUE;
                if ( ( mretv & MENU_QUICK_SWITCH ) ) {
                    retv = 10 + ( mretv & MENU_LOWER_MASK );
                }
                g_free ( input );
                dmenu_finish ( state, retv );
                return;
            }
            else {
                pd->selected_line = next_pos - 1;
            }
        }
        // Restart
        rofi_view_restart ( state );
        rofi_view_set_selected_line ( state, pd->selected_line );
        if ( !restart ) {
            dmenu_finish ( state, retv );
        }
        return;
    }
    // We normally do not want to restart the loop.
    restart = FALSE;
    // Normal mode
    if ( ( mretv & MENU_OK  ) && pd->selected_line != UINT32_MAX && cmd_list[pd->selected_line].entry != NULL ) {
        if ( ( mretv & MENU_CUSTOM_ACTION ) && pd->multi_select ) {
            restart = TRUE;
            if ( pd->selected_list == NULL ) {
                pd->selected_list = g_malloc0 ( sizeof ( uint32_t ) * ( pd->cmd_list_length / 32 + 1 ) );
            }
            pd->selected_count += ( bitget ( pd->selected_list, pd->selected_line ) ? ( -1 ) : ( 1 ) );
            bittoggle ( pd->selected_list, pd->selected_line );
            // Move to next line.
            pd->selected_line = MIN ( next_pos, cmd_list_length - 1 );
            if ( pd->selected_count > 0 ) {
                char *str = g_strdup_printf ( "%u/%u", pd->selected_count, pd->cmd_list_length );
                rofi_view_set_overlay ( state, str );
                g_free ( str );
            }
            else {
                rofi_view_set_overlay ( state, NULL );
            }
        }
        else {
            dmenu_print_results ( pd, input );
        }
        retv = TRUE;
    }
    // Custom input
    else if ( ( mretv & ( MENU_CUSTOM_INPUT ) ) ) {
        dmenu_print_results ( pd, input );

        retv = TRUE;
    }
    // Quick switch with entry selected.
    else if ( ( mretv & MENU_QUICK_SWITCH ) ) {
        dmenu_print_results ( pd, input );

        restart = FALSE;
        retv    = 10 + ( mretv & MENU_LOWER_MASK );
    }
    g_free ( input );
    if ( restart ) {
        rofi_view_restart ( state );
        rofi_view_set_selected_line ( state, pd->selected_line );
    }
    else {
        dmenu_finish ( state, retv );
    }
}

int dmenu_switcher_dialog ( void )
{
    mode_init ( &dmenu_mode );
    MenuFlags            menu_flags = MENU_NORMAL;
    DmenuModePrivateData *pd        = (DmenuModePrivateData *) dmenu_mode.private_data;
    int                  async      = TRUE;

    // For now these only work in sync mode.
    if ( find_arg ( "-sync" ) >= 0 || find_arg ( "-dump" ) >= 0 || find_arg ( "-select" ) >= 0
         || find_arg ( "-no-custom" ) >= 0 || find_arg ( "-only-match" ) >= 0 || config.auto_select ||
         find_arg ( "-selected-row" ) >= 0 ) {
        async = FALSE;
    }
    // Check if the subsystem is setup for reading, otherwise do not read.
    if ( pd->cancel != NULL ) {
        if ( async ) {
            unsigned int pre_read = 25;
            find_arg_uint ( "-async-pre-read", &pre_read );
            async = get_dmenu_async ( pd, pre_read );
        }
        else {
            get_dmenu_sync ( pd );
        }
    }
    char         *input          = NULL;
    unsigned int cmd_list_length = pd->cmd_list_length;
    DmenuScriptEntry *cmd_list     = pd->cmd_list;

    pd->only_selected = FALSE;
    pd->multi_select  = FALSE;
    if ( find_arg ( "-multi-select" ) >= 0 ) {
        menu_flags       = MENU_INDICATOR;
        pd->multi_select = TRUE;
    }
    if ( find_arg ( "-markup-rows" ) >= 0 ) {
        pd->do_markup = TRUE;
    }
    if ( find_arg ( "-only-match" ) >= 0 || find_arg ( "-no-custom" ) >= 0 ) {
        pd->only_selected = TRUE;
        if ( cmd_list_length == 0 ) {
            return TRUE;
        }
    }
    if ( config.auto_select && cmd_list_length == 1 ) {
        rofi_output_formatted_line ( pd->format, cmd_list[0].entry , 0, config.filter );
        return TRUE;
    }
    if ( find_arg ( "-password" ) >= 0 ) {
        menu_flags |= MENU_PASSWORD;
    }
    /* copy filter string */
    input = g_strdup ( config.filter );

    char *select = NULL;
    find_arg_str ( "-select", &select );
    if ( select != NULL ) {
        rofi_int_matcher **tokens = helper_tokenize ( select, config.case_sensitive );
        unsigned int     i        = 0;
        for ( i = 0; i < cmd_list_length; i++ ) {
            if ( helper_token_match ( tokens, cmd_list[i].entry ) ) {
                pd->selected_line = i;
                break;
            }
        }
        helper_tokenize_free ( tokens );
    }
    if ( find_arg ( "-dump" ) >= 0 ) {
        rofi_int_matcher **tokens = helper_tokenize ( config.filter ? config.filter : "", config.case_sensitive );
        unsigned int     i        = 0;
        for ( i = 0; i < cmd_list_length; i++ ) {
            if ( tokens == NULL || helper_token_match ( tokens, cmd_list[i].entry ) ) {
                rofi_output_formatted_line ( pd->format, cmd_list[i].entry , i, config.filter );
            }
        }
        helper_tokenize_free ( tokens );
        dmenu_mode_free ( &dmenu_mode );
        g_free ( input );
        return TRUE;
    }
    find_arg_str (  "-p", &( dmenu_mode.display_name ) );
    RofiViewState *state = rofi_view_create ( &dmenu_mode, input, menu_flags, dmenu_finalize );
    // @TODO we should do this better.
    if ( async && ( pd->cancel != NULL ) ) {
        rofi_view_set_overlay ( state, "Loading.. " );
    }
    rofi_view_set_selected_line ( state, pd->selected_line );
    rofi_view_set_active ( state );

    return FALSE;
}

void print_dmenu_options ( void )
{
    int is_term = isatty ( fileno ( stdout ) );
    print_help_msg ( "-mesg", "[string]", "Print a small user message under the prompt (uses pango markup)", NULL, is_term );
    print_help_msg ( "-p", "[string]", "Prompt to display left of entry field", NULL, is_term );
    print_help_msg ( "-selected-row", "[integer]", "Select row", NULL, is_term );
    print_help_msg ( "-format", "[string]", "Output format string", "s", is_term );
    print_help_msg ( "-u", "[list]", "List of row indexes to mark urgent", NULL, is_term );
    print_help_msg ( "-a", "[list]", "List of row indexes to mark active", NULL, is_term );
    print_help_msg ( "-l", "[integer] ", "Number of rows to display", NULL, is_term );
    print_help_msg ( "-window-title", "[string] ", "Set the dmenu window title", NULL, is_term );
    print_help_msg ( "-i", "", "Set filter to be case insensitive", NULL, is_term );
    print_help_msg ( "-only-match", "", "Force selection or custom entry", NULL, is_term );
    print_help_msg ( "-no-custom", "", "Don't accept custom entry", NULL, is_term );
    print_help_msg ( "-select", "[string]", "Select the first row that matches", NULL, is_term );
    print_help_msg ( "-password", "", "Do not show what the user inputs. Show '*' instead.", NULL, is_term );
    print_help_msg ( "-markup-rows", "", "Allow and render pango markup as input data.", NULL, is_term );
    print_help_msg ( "-sep", "[char]", "Element separator.", "'\\n'", is_term );
    print_help_msg ( "-input", "[filename]", "Read input from file instead from standard input.", NULL, is_term );
    print_help_msg ( "-sync", "", "Force dmenu to first read all input data, then show dialog.", NULL, is_term );
    print_help_msg ( "-async-pre-read", "[number]", "Read several entries blocking before switching to async mode", "25", is_term );
    print_help_msg ( "-w", "windowid", "Position over window with X11 windowid.", NULL, is_term );
}
