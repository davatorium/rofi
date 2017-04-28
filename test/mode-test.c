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

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <glib.h>
#include <string.h>

#include <mode.h>
#include <mode-private.h>
#include <dialogs/help-keys.h>
#include <xkbcommon/xkbcommon.h>
#include <keyb.h>
unsigned int test =0;
#define TASSERT( a )    {                                 \
        assert ( a );                                     \
        printf ( "Test %3i passed (%s)\n", ++test, # a ); \
}

#define TASSERTE( a, b )    {                                                            \
        if ( ( a ) == ( b ) ) {                                                          \
            printf ( "Test %i passed (%s == %s) (%u == %u)\n", ++test, # a, # b, a, b ); \
        }else {                                                                          \
            printf ( "Test %i failed (%s == %s) (%u != %u)\n", ++test, # a, # b, a, b ); \
            abort ( );                                                                   \
        }                                                                                \
}
void rofi_add_error_message ( GString *msg )
{
}
int monitor_active ( void *d )
{
    return 0;
}
int rofi_view_error_dialog ( const char *msg, G_GNUC_UNUSED int markup )
{
    fputs ( msg, stderr );
    return TRUE;
}
int textbox_get_estimated_char_height ( void );
int textbox_get_estimated_char_height ( void )
{
    return 16;
}
void rofi_view_get_current_monitor ( int *width, int *height )
{

}
void * rofi_view_get_active ( void )
{
    return NULL;
}
gboolean rofi_view_trigger_action ( void *state, KeyBindingAction action )
{
}
gboolean x11_parse_key ( const char *combo, unsigned int *mod, xkb_keysym_t *key, gboolean *release, GString *msg )
{
    return TRUE;
}


int main ( G_GNUC_UNUSED int argc, G_GNUC_UNUSED char **argv )
{
    setup_abe ();
    TASSERTE ( mode_init ( &help_keys_mode ), TRUE );

    unsigned int rows = mode_get_num_entries ( &help_keys_mode);
    TASSERTE ( rows , 64);
    
    for ( int i =0; i < rows; i++  ){
        int state = 0;
        GList *list = NULL;
        char *v = mode_get_display_value ( &help_keys_mode, i, &state, &list, TRUE ); 
        TASSERT ( v != NULL );
        g_free ( v );
        v = mode_get_display_value ( &help_keys_mode, i, &state, &list, FALSE ); 
        TASSERT ( v == NULL );
    }

    mode_destroy ( &help_keys_mode );
}
