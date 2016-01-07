#include "rofi.h"
#include "xrmoptions.h"
#include "x11-helper.h"
#include "mode.h"

// This one should only be in mode implementations.
#include "mode-private.h"
/**
 * @ingroup MODE
 * @{
 */

void mode_init ( Mode *mode )
{
    g_assert ( mode != NULL );
    g_assert ( mode->_init != NULL );
    mode->_init ( mode );
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

char * mode_get_display_value ( const Mode *mode, unsigned int selected_line, int *state, int get_entry )
{
    g_assert ( mode != NULL );
    g_assert ( state != NULL );
    g_assert ( mode->_get_display_value != NULL );

    return mode->_get_display_value ( mode, selected_line, state, get_entry );
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
        return mode->_get_display_value ( mode, selected_line, &state, TRUE );
    }
}

int mode_is_not_ascii ( const Mode *mode, unsigned int selected_line )
{
    g_assert ( mode != NULL );
    g_assert ( mode->_is_not_ascii != NULL );
    return mode->_is_not_ascii ( mode, selected_line );
}
ModeMode mode_result ( Mode *mode, int menu_retv, char **input, unsigned int selected_line )
{
    g_assert ( mode != NULL );
    g_assert ( mode->_result != NULL );
    g_assert ( (*input) != NULL );
    return mode->_result ( mode, menu_retv, input, selected_line );
}

int mode_token_match ( const Mode *mode, char **tokens, int not_ascii, int case_sensitive, unsigned int selected_line )
{
    g_assert ( mode != NULL );
    g_assert ( mode->_token_match != NULL );
    return mode->_token_match ( mode, tokens, not_ascii, case_sensitive, selected_line );
}

const char *mode_get_name ( const Mode *mode )
{
    g_assert ( mode != NULL );
    return mode->name;
}

void mode_setup_keybinding ( Mode *mode )
{
    g_assert ( mode != NULL );
    mode->keycfg = g_strdup_printf ( "key-%s", mode->name );
    config_parser_add_option ( xrm_String, mode->keycfg, (void * *) &( mode->keystr ), "Keybinding" );
}

int mode_check_keybinding ( const Mode *mode, KeySym key, unsigned int modstate )
{
    g_assert ( mode != NULL );
    if ( mode->keystr != NULL ) {
        if ( mode->modmask == modstate && mode->keysym == key ) {
            return TRUE;
        }
    }
    return FALSE;
}

void mode_free ( Mode **mode )
{
    g_assert ( mode != NULL );
    g_assert ( (*mode) != NULL );
    if ( ( *mode )->keycfg != NULL ) {
        g_free ( ( *mode )->keycfg );
        ( *mode )->keycfg = NULL;
    }
    if ( ( *mode )->free != NULL ) {
        ( *mode )->free ( *mode );
    }
    ( *mode ) = NULL;
}

int mode_grab_key ( Mode *mode, Display *display )
{
    g_assert ( mode != NULL );
    g_assert ( display != NULL );
    if ( mode->keystr != NULL ) {
        x11_parse_key ( mode->keystr, &( mode->modmask ), &( mode->keysym ) );
        if ( mode->keysym != NoSymbol ) {
            x11_grab_key ( display, mode->modmask, mode->keysym );
            return TRUE;
        }
    }
    return FALSE;
}
void mode_ungrab_key ( Mode *mode, Display *display )
{
    g_assert ( mode != NULL );
    g_assert ( display != NULL );
    if ( mode->keystr != NULL ) {
        if ( mode->keysym != NoSymbol ) {
            x11_ungrab_key ( display, mode->modmask, mode->keysym );
        }
    }
}

void mode_print_keybindings ( const Mode *mode )
{
    g_assert ( mode != NULL );
    if ( mode->keystr != NULL ) {
        fprintf ( stdout, "\t* "color_bold "%s"color_reset " on %s\n", mode->name, mode->keystr );
    }
    else {
        fprintf ( stdout, "\t* "color_bold "%s"color_reset " on <unspecified>\n", mode->name );
    }
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
/*@}*/
