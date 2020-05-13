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
#define G_LOG_DOMAIN    "Dialogs.Combi"

#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <rofi.h>
#include "settings.h"
#include "helper.h"

#include <dialogs/dialogs.h>
#include <pango/pango.h>
#include "mode-private.h"
#include <theme.h>
#include "widgets/textbox.h"

/**
 * Combi Mode
 */
typedef struct
{
    Mode     *mode;
    gboolean disable;
	gboolean print_newline;
} CombiMode;

typedef struct
{
    // List of (combined) entries.
    unsigned int cmd_list_length;
    // List to validate where each switcher starts.
    unsigned int *starts;
    unsigned int *lengths;
    // List of switchers to combine.
    unsigned int num_switchers;
    CombiMode    *switchers;
} CombiModePrivateData;

static void combi_mode_parse_switchers ( Mode *sw )
{
    CombiModePrivateData *pd     = mode_get_private_data ( sw );
    char                 *savept = NULL;
    // Make a copy, as strtok will modify it.
    char                 *switcher_str = g_strdup ( config.combi_modi );
    const char * const   sep           = ",#";
    // Split token on ','. This modifies switcher_str.

	GHashTable *ht;
	ht = g_hash_table_new ( g_str_hash, g_str_equal );

    for ( char *token = strtok_r ( switcher_str, sep, &savept ); token != NULL;
          token = strtok_r ( NULL, sep, &savept ) ) {
        // Resize and add entry.
        pd->switchers = (CombiMode  *) g_realloc ( pd->switchers,
                                                   sizeof ( CombiMode ) * ( pd->num_switchers + 1 ) );

        Mode *mode = rofi_collect_modi_search ( token );
        if (  mode ) {
            pd->switchers[pd->num_switchers].disable = FALSE;
            pd->switchers[pd->num_switchers].mode  = mode;
            pd->switchers[pd->num_switchers].print_newline  = TRUE;
			g_hash_table_insert( ht, token,  &( pd->switchers[pd->num_switchers++] ) );
			/* g_hash_table_insert( ht, token,  pd->switchers + pd->num_switchers ); */
        }
        else {
            // If not build in, use custom switchers.
            Mode *sw = script_switcher_parse_setup ( token );
            if ( sw != NULL ) {
                pd->switchers[pd->num_switchers].disable = FALSE;
                pd->switchers[pd->num_switchers].mode  = sw;
				pd->switchers[pd->num_switchers].print_newline  = TRUE;
				g_hash_table_insert( ht, token,  &( pd->switchers[pd->num_switchers++] ) );
            }
            else {
                // Report error, don't continue.
                g_warning ( "Invalid script switcher: %s", token );
                token = NULL;
            }
        }
    }

    savept = NULL;
    for ( char *token = strtok_r ( config.combi_no_linebreak_modi, sep, &savept ); token != NULL;
          token = strtok_r ( NULL, sep, &savept ) ) {
		CombiMode *mode = g_hash_table_lookup ( ht, token );
		if ( mode != NULL ) {
			mode->print_newline = FALSE;
		}
		else {
			g_warning ( "%s is in -combi-no-linebreak-modi but not in -combi-modi.", token );
		}
	}
    // Free string that was modified by strtok_r
    g_free ( switcher_str );
	g_hash_table_destroy( ht );
}

static int combi_mode_init ( Mode *sw )
{
    if ( mode_get_private_data ( sw ) == NULL ) {
        CombiModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
        mode_set_private_data ( sw, (void *) pd );
        combi_mode_parse_switchers ( sw );
        pd->starts  = g_malloc0 ( sizeof ( int ) * pd->num_switchers );
        pd->lengths = g_malloc0 ( sizeof ( int ) * pd->num_switchers );
        for ( unsigned int i = 0; i < pd->num_switchers; i++ ) {
            if ( !mode_init ( pd->switchers[i].mode ) ) {
                return FALSE;
            }
        }
        if ( pd->cmd_list_length == 0 ) {
            pd->cmd_list_length = 0;
            for ( unsigned int i = 0; i < pd->num_switchers; i++ ) {
                unsigned int length = mode_get_num_entries ( pd->switchers[i].mode );
                pd->starts[i]        = pd->cmd_list_length;
                pd->lengths[i]       = length;
                pd->cmd_list_length += length;
            }
        }
    }
    return TRUE;
}
static unsigned int combi_mode_get_num_entries ( const Mode *sw )
{
    const CombiModePrivateData *pd    = (const CombiModePrivateData *) mode_get_private_data ( sw );
    unsigned int               length = 0;
    for ( unsigned int i = 0; i < pd->num_switchers; i++ ) {
        unsigned int entries = mode_get_num_entries ( pd->switchers[i].mode );
        pd->starts[i]  = length;
        pd->lengths[i] = entries;
        length        += entries;
    }
    return length;
}
static void combi_mode_destroy ( Mode *sw )
{
    CombiModePrivateData *pd = (CombiModePrivateData *) mode_get_private_data ( sw );
    if ( pd != NULL ) {
        g_free ( pd->starts );
        g_free ( pd->lengths );
        // Cleanup switchers.
        for ( unsigned int i = 0; i < pd->num_switchers; i++ ) {
            mode_destroy ( pd->switchers[i].mode );
        }
        g_free ( pd->switchers );
        g_free ( pd );
        mode_set_private_data ( sw, NULL );
    }
}
static ModeMode combi_mode_result ( Mode *sw, int mretv, char **input, unsigned int selected_line )
{
    CombiModePrivateData *pd = mode_get_private_data ( sw );

    if ( input[0][0] == '!' ) {
        int     switcher = -1;
        // Implement strchrnul behaviour.
        char    *eob     = g_utf8_strchr ( input[0], -1,' ' );
        if ( eob == NULL ) {
            eob = &(input[0][strlen(input[0])]);
        }
        ssize_t bang_len = g_utf8_pointer_to_offset ( input[0], eob ) - 1;
        if ( bang_len > 0 ) {
            for ( unsigned i = 0; switcher == -1 && i < pd->num_switchers; i++ ) {
                const char *mode_name    = mode_get_name ( pd->switchers[i].mode );
                size_t     mode_name_len = g_utf8_strlen ( mode_name, -1 );
                if ( (size_t) bang_len <= mode_name_len && utf8_strncmp ( &input[0][1], mode_name, bang_len ) == 0 ) {
                    switcher = i;
                }
            }
        }
        if ( switcher >= 0 ) {
            if ( eob[0] == ' ' ) {
                char *n = eob + 1;
                return mode_result ( pd->switchers[switcher].mode, mretv, &n,
                                     selected_line - pd->starts[switcher] );
            }
            return MODE_EXIT;
        }
    }
    if ( mretv & MENU_QUICK_SWITCH ) {
        return mretv & MENU_LOWER_MASK;
    }

	unsigned offset = 0;
    for ( unsigned i = 0; i < pd->num_switchers; i++ ) {
		if ( pd->switchers[i].disable ) {
			offset += pd->lengths[i];
		}
        if ( selected_line >= pd->starts[i] &&
             selected_line < ( pd->starts[i] + pd->lengths[i] ) ) {
            return mode_result ( pd->switchers[i].mode, mretv, input, selected_line - pd->starts[i] );
        }
    }
    return MODE_EXIT;
}
static int combi_mode_match ( const Mode *sw, rofi_int_matcher **tokens, unsigned int index )
{
    CombiModePrivateData *pd = mode_get_private_data ( sw );
    for ( unsigned i = 0; i < pd->num_switchers; i++ ) {
        if ( pd->switchers[i].disable ) {
            continue;
        }
        if ( index >= pd->starts[i] && index < ( pd->starts[i] + pd->lengths[i] ) ) {
            return mode_token_match ( pd->switchers[i].mode, tokens, index - pd->starts[i] );
        }
    }
    return 0;
}
static char * combi_mgrv ( const Mode *sw, unsigned int selected_line, int *state, GList **attr_list, int get_entry )
{
    CombiModePrivateData *pd = mode_get_private_data ( sw );
    if ( !get_entry ) {
        for ( unsigned i = 0; i < pd->num_switchers; i++ ) {
            if ( selected_line >= pd->starts[i] && selected_line < ( pd->starts[i] + pd->lengths[i] ) ) {
                mode_get_display_value ( pd->switchers[i].mode, selected_line - pd->starts[i], state, attr_list, FALSE );
                return NULL;
            }
        }
        return NULL;
    }
    for ( unsigned i = 0; i < pd->num_switchers; i++ ) {
        if ( selected_line >= pd->starts[i] && selected_line < ( pd->starts[i] + pd->lengths[i] ) ) {
            char       * retv;
            char       * str  = retv = mode_get_display_value ( pd->switchers[i].mode, selected_line - pd->starts[i], state, attr_list, TRUE );
            const char *dname = mode_get_display_name ( pd->switchers[i].mode );
			char *format_str = g_strdup( config.combi_display_format );
            if ( !config.combi_hide_mode_prefix ) {
				if ( !(*state & MARKUP) && config.markup_combi ) {
					// Mode does not use markup, but we want to, so escape output
					char * tmp_str = g_markup_escape_text( str, -1 );
					g_free(str);
					str = tmp_str;
					*state |= MARKUP;
				}
				if ( (*state & MARKUP) && !config.markup_combi ) {
					// Mode does use markup, but we did not want to, so escape pattern string
					char * tmp_str = g_markup_escape_text( format_str, -1 );
					g_free(format_str);
					format_str = tmp_str;
				}
				char *dname_markup = g_markup_escape_text ( dname, -1 );
				char *opt_linebreak = g_strdup(pd->switchers[i].print_newline ? "\n" : config.combi_no_linebreak_str);
				retv = helper_string_replace_if_exists( format_str,
					"{mode}", dname_markup,
					"{linebreak}", opt_linebreak,
					"{element}", str,
				    NULL );
                g_free ( str );
				g_free ( dname_markup );
				g_free ( opt_linebreak );
				g_free ( format_str );
            }

            if ( attr_list != NULL ) {
                ThemeWidget *wid = rofi_theme_find_widget ( sw->name, NULL, TRUE );
                Property    *p   = rofi_theme_find_property ( wid, P_COLOR, pd->switchers[i].mode->name, TRUE );
                if ( p != NULL ) {
                    PangoAttribute *pa = pango_attr_foreground_new (
                        p->value.color.red * 65535,
                        p->value.color.green * 65535,
                        p->value.color.blue * 65535 );
                    pa->start_index = PANGO_ATTR_INDEX_FROM_TEXT_BEGINNING;
                    pa->end_index   = strlen ( dname );
                    *attr_list      = g_list_append ( *attr_list, pa );
                }
            }
            return retv;
        }
    }

    return NULL;
}
static char * combi_get_completion ( const Mode *sw, unsigned int index )
{
    CombiModePrivateData *pd = mode_get_private_data ( sw );
    for ( unsigned i = 0; i < pd->num_switchers; i++ ) {
        if ( index >= pd->starts[i] && index < ( pd->starts[i] + pd->lengths[i] ) ) {
            char *comp  = mode_get_completion ( pd->switchers[i].mode, index - pd->starts[i] );
            char *mcomp = g_strdup_printf ( "!%s %s", mode_get_name ( pd->switchers[i].mode ), comp );
            g_free ( comp );
            return mcomp;
        }
    }
    // Should never get here.
    g_assert_not_reached ();
    return NULL;
}

static cairo_surface_t * combi_get_icon ( const Mode *sw, unsigned int index, int height )
{
    CombiModePrivateData *pd = mode_get_private_data ( sw );
    for ( unsigned i = 0; i < pd->num_switchers; i++ ) {
        if ( index >= pd->starts[i] && index < ( pd->starts[i] + pd->lengths[i] ) ) {
            cairo_surface_t *icon = mode_get_icon ( pd->switchers[i].mode, index - pd->starts[i], height );
            return icon;
        }
    }
    return NULL;
}

static char * combi_preprocess_input ( Mode *sw, const char *input )
{
    CombiModePrivateData *pd = mode_get_private_data ( sw );
    for ( unsigned i = 0; i < pd->num_switchers; i++ ) {
        pd->switchers[i].disable = FALSE;
    }
    if ( input != NULL && input[0] == '!' ) {
        // Implement strchrnul behaviour.
        const char    *eob     = g_utf8_strchr ( input, -1, ' ' );
        if ( eob == NULL ) {
            // Set it to end.
            eob = &(input[strlen(input)]);
        }
        ssize_t bang_len = g_utf8_pointer_to_offset ( input, eob ) - 1;
        if ( bang_len > 0 ) {
            for ( unsigned i = 0; i < pd->num_switchers; i++ ) {
                const char *mode_name    = mode_get_name ( pd->switchers[i].mode );
                size_t     mode_name_len = g_utf8_strlen ( mode_name, -1 );
                if ( !( (size_t) bang_len <= mode_name_len && utf8_strncmp ( &input[1], mode_name, bang_len ) == 0 ) ) {
                    // No match.
                    pd->switchers[i].disable = TRUE;
                }
            }
            if ( eob[0] == '\0' || eob[1] == '\0' ) {
                return NULL;
            }
            return g_strdup ( eob + 1 );
        }
    }
    return g_strdup ( input );
}

Mode combi_mode =
{
    .name               = "combi",
    .cfg_name_key       = "display-combi",
    ._init              = combi_mode_init,
    ._get_num_entries   = combi_mode_get_num_entries,
    ._result            = combi_mode_result,
    ._destroy           = combi_mode_destroy,
    ._token_match       = combi_mode_match,
    ._get_completion    = combi_get_completion,
    ._get_display_value = combi_mgrv,
    ._get_icon          = combi_get_icon,
    ._preprocess_input  = combi_preprocess_input,
    .private_data       = NULL,
    .free               = NULL
};
