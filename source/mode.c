#include "rofi.h"
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
    g_assert ( mode->mgrv != NULL );

    return mode->mgrv ( mode, selected_line, state, get_entry );
}
/*@}*/
