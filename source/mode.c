#include <stdio.h>
#include <glib.h>
#include <string.h>
#include "rofi.h"
#include "xrmoptions.h"
#include "x11-helper.h"
#include "mode.h"

// This one should only be in mode implementations.
#include "mode-private.h"

#define LOG_DOMAIN    "Mode"

/**
 * @ingroup MODE
 * @{
 */

int mode_init ( Mode *mode )
{
    g_return_val_if_fail ( mode != NULL, FALSE );
    g_return_val_if_fail ( mode->_init != NULL, FALSE );
    return mode->_init ( mode );
}

void mode_destroy ( Mode *mode )
{
    g_assert ( mode != NULL );
    g_assert ( mode->_destroy != NULL );
    mode->_destroy ( mode );
}

unsigned int mode_get_num_entries ( const Mode *mode )
{
    g_assert ( mode != NULL );
    g_assert ( mode->_get_num_entries != NULL );
    return mode->_get_num_entries ( mode );
}

char * mode_get_display_value ( const Mode *mode, unsigned int selected_line, int *state, GList **list, int get_entry )
{
    g_assert ( mode != NULL );
    g_assert ( state != NULL );
    g_assert ( mode->_get_display_value != NULL );

    return mode->_get_display_value ( mode, selected_line, state, list, get_entry );
}

cairo_surface_t * mode_get_icon ( const Mode *mode, unsigned int selected_line )
{
    g_assert ( mode != NULL );

    if ( mode->_get_icon != NULL ) {
        return mode->_get_icon ( mode, selected_line );
    }
    else {
        return NULL;
    }
}

char * mode_get_completion ( const Mode *mode, unsigned int selected_line )
{
    g_assert ( mode != NULL );
    if ( mode->_get_completion != NULL ) {
        return mode->_get_completion ( mode, selected_line );
    }
    else {
        int state;
        g_assert ( mode->_get_display_value != NULL );
        return mode->_get_display_value ( mode, selected_line, &state, NULL, TRUE );
    }
}

ModeMode mode_result ( Mode *mode, int menu_retv, char **input, unsigned int selected_line )
{
    g_assert ( mode != NULL );
    g_assert ( mode->_result != NULL );
    g_assert ( ( *input ) != NULL );
    return mode->_result ( mode, menu_retv, input, selected_line );
}

int mode_token_match ( const Mode *mode, GRegex **tokens, unsigned int selected_line )
{
    g_assert ( mode != NULL );
    g_assert ( mode->_token_match != NULL );
    return mode->_token_match ( mode, tokens, selected_line );
}

const char *mode_get_name ( const Mode *mode )
{
    g_assert ( mode != NULL );
    return mode->name;
}

void mode_free ( Mode **mode )
{
    g_assert ( mode != NULL );
    g_assert ( ( *mode ) != NULL );
    if ( ( *mode )->free != NULL ) {
        ( *mode )->free ( *mode );
    }
    ( *mode ) = NULL;
}

void *mode_get_private_data ( const Mode *mode )
{
    g_assert ( mode != NULL );
    return mode->private_data;
}

void mode_set_private_data ( Mode *mode, void *pd )
{
    g_assert ( mode != NULL );
    if ( pd != NULL ) {
        g_assert ( mode->private_data == NULL );
    }
    mode->private_data = pd;
}

const char *mode_get_display_name ( const Mode *mode )
{
    if ( mode->display_name != NULL ) {
        return mode->display_name;
    }
    return mode->name;
}

void mode_set_config ( Mode *mode )
{
    snprintf ( mode->cfg_name_key, 128, "display-%s", mode->name );
    config_parser_add_option ( xrm_String, mode->cfg_name_key, (void * *) &( mode->display_name ), "The display name of this browser" );
}

char * mode_preprocess_input ( Mode *mode, const char *input )
{
    if ( mode->_preprocess_input ) {
        return mode->_preprocess_input ( mode, input );
    }
    return g_strdup ( input );
}
char *mode_get_message ( const Mode *mode )
{
    if ( mode->_get_message ) {
        return mode->_get_message ( mode );
    }
    return NULL;
}
/*@}*/
