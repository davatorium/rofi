/**
 * rofi
 *
 * MIT/X11 License
 * Copyright (c) 2012 Sean Pringle <sean.pringle@gmail.com>
 * Modified 2013-2017 Qball Cow <qball@gmpclient.org>
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

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <locale.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xkb.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>
#include <xkbcommon/xkbcommon-x11.h>
#include <sys/types.h>

#include <glib-unix.h>

#include <libgwater-xcb.h>

#include "xcb-internal.h"
#include "xkb-internal.h"

#include "settings.h"
#include "mode.h"
#include "rofi.h"
#include "helper.h"
#include "widgets/textbox.h"
#include "x11-helper.h"
#include "xrmoptions.h"
#include "dialogs/dialogs.h"

#include "view.h"
#include "view-internal.h"

#include "gitconfig.h"

#include "theme.h"

// Pidfile.
char       *pidfile   = NULL;
const char *cache_dir = NULL;

/** List of error messages.*/
GList *list_of_error_msgs = NULL;

void rofi_add_error_message ( GString *str )
{
    list_of_error_msgs = g_list_append ( list_of_error_msgs, str );
}
/** global structure holding the keyboard status */
struct xkb_stuff xkb = {
    .xcb_connection = NULL,
    .context        = NULL,
    .keymap         = NULL,
    .state          = NULL,
    .compose        = {
        .table = NULL,
        .state = NULL
    }
};

/** Path to the configuration file */
char         *config_path = NULL;
/** Array holding all activated modi. */
Mode         **modi = NULL;
/** Number of activated modi in #modi array */
unsigned int num_modi = 0;
/** Current selected mode */
unsigned int curr_switcher = 0;

/** Glib main loop. */
GMainLoop       *main_loop = NULL;
/** GWater xcb source, signalling events from the X server */
GWaterXcbSource *main_loop_source = NULL;

/** Flag indicating we are in dmenu mode. */
static int dmenu_mode = FALSE;
/** Rofi's return code */
int        return_code = EXIT_SUCCESS;

void process_result ( RofiViewState *state );

void rofi_set_return_code ( int code )
{
    return_code = code;
}

unsigned int rofi_get_num_enabled_modi ( void )
{
    return num_modi;
}

const Mode * rofi_get_mode ( unsigned int index )
{
    return modi[index];
}

/**
 * @param name Name of the switcher to lookup.
 *
 * Find the index of the switcher with name.
 *
 * @returns index of the switcher in modi, -1 if not found.
 */
static int switcher_get ( const char *name )
{
    for ( unsigned int i = 0; i < num_modi; i++ ) {
        if ( strcmp ( mode_get_name ( modi[i] ), name ) == 0 ) {
            return i;
        }
    }
    return -1;
}

/**
 * Do needed steps to start showing the gui
 */
static int setup ()
{
    // Create pid file
    int pfd = create_pid_file ( pidfile );
    if ( pfd >= 0 ) {
        // Request truecolor visual.
        x11_create_visual_and_colormap ( );
        textbox_setup ();
    }
    return pfd;
}

/**
 * Teardown the gui.
 */
static void teardown ( int pfd )
{
    // Cleanup font setup.
    textbox_cleanup ( );

    // Release the window.
    release_keyboard ( );
    release_pointer ( );

    // Cleanup view
    rofi_view_cleanup ();
    // Cleanup pid file.
    remove_pid_file ( pfd );
}
static void run_switcher ( ModeMode mode )
{
    // Otherwise check if requested mode is enabled.
    for ( unsigned int i = 0; i < num_modi; i++ ) {
        if ( !mode_init ( modi[i] ) ) {
            rofi_view_error_dialog ( ERROR_MSG ( "Failed to initialize all the modi." ), ERROR_MSG_MARKUP );
            return;
        }
    }
    // Error dialog must have been created.
    if ( rofi_view_get_active () != NULL ) {
        return;
    }
    curr_switcher = mode;
    RofiViewState * state = rofi_view_create ( modi[mode], config.filter, NULL, MENU_PROMPT_COLON, process_result );
    if ( state ) {
        rofi_view_set_active ( state );
    }
    if ( rofi_view_get_active () == NULL ) {
        g_main_loop_quit ( main_loop  );
    }
}
void process_result ( RofiViewState *state )
{
    Mode *sw = state->sw;
    rofi_view_set_active ( NULL );
    if ( sw != NULL ) {
        unsigned int selected_line = rofi_view_get_selected_line ( state );;
        MenuReturn   mretv         = rofi_view_get_return_value ( state );
        char         *input        = g_strdup ( rofi_view_get_user_input ( state ) );
        ModeMode     retv          = mode_result ( sw, mretv, &input, selected_line );
        g_free ( input );

        ModeMode mode = curr_switcher;
        // Find next enabled
        if ( retv == NEXT_DIALOG ) {
            mode = ( mode + 1 ) % num_modi;
        }
        else if ( retv == PREVIOUS_DIALOG ) {
            if ( mode == 0 ) {
                mode = num_modi - 1;
            }
            else {
                mode = ( mode - 1 ) % num_modi;
            }
        }
        else if ( retv == RELOAD_DIALOG ) {
            // do nothing.
        }
        else if ( retv == RESET_DIALOG ) {
            rofi_view_clear_input ( state );
        }
        else if ( retv < MODE_EXIT ) {
            mode = ( retv ) % num_modi;
        }
        else {
            mode = retv;
        }
        if ( mode != MODE_EXIT ) {
            /**
             * Load in the new mode.
             */
            rofi_view_switch_mode ( state, modi[mode] );
            rofi_view_set_active ( state );
            curr_switcher = mode;
            return;
        }
    }
    rofi_view_free ( state );
}

/**
 * Help function.
 */
static void print_main_application_options ( int is_term )
{
    print_help_msg ( "-no-config", "", "Do not load configuration, use default values.", NULL, is_term );
    print_help_msg ( "-v,-version", "", "Print the version number and exit.", NULL, is_term  );
    print_help_msg ( "-dmenu", "", "Start in dmenu mode.", NULL, is_term );
    print_help_msg ( "-display", "[string]", "X server to contact.", "${DISPLAY}", is_term );
    print_help_msg ( "-h,-help", "", "This help message.", NULL, is_term );
    print_help_msg ( "-dump-xresources", "", "Dump the current configuration in Xresources format and exit.", NULL, is_term );
    print_help_msg ( "-dump-xresources-theme", "", "Dump the current color scheme in Xresources format and exit.", NULL, is_term );
    print_help_msg ( "-e", "[string]", "Show a dialog displaying the passed message and exit.", NULL, is_term );
    print_help_msg ( "-markup", "", "Enable pango markup where possible.", NULL, is_term );
    print_help_msg ( "-normal-window", "", "In dmenu mode, behave as a normal window. (experimental)", NULL, is_term );
    print_help_msg ( "-show", "[mode]", "Show the mode 'mode' and exit. The mode has to be enabled.", NULL, is_term );
    print_help_msg ( "-no-lazy-grab", "", "Disable lazy grab that, when fail to grab keyboard, does not block but retry later.", NULL, is_term );
}
static void help ( G_GNUC_UNUSED int argc, char **argv )
{
    int is_term = isatty ( fileno ( stdout ) );
    printf ( "%s usage:\n", argv[0] );
    printf ( "\t%s [-options ...]\n\n", argv[0] );
    printf ( "Command line only options:\n" );
    print_main_application_options ( is_term );
    printf ( "DMENU command line options:\n" );
    print_dmenu_options ();
    printf ( "Global options:\n" );
    print_options ();
    printf ( "\n" );
    x11_dump_monitor_layout ();
    printf ( "\n" );
    printf ( "Compile time options:\n" );
#ifdef WINDOW_MODE
    printf ( "\t* window  %senabled%s\n", is_term ? color_green : "", is_term ? color_reset : "" );
#else
    printf ( "\t* window  %sdisabled%s\n", is_term ? color_red : "", is_term ? color_reset : "" );
#endif
#ifdef ENABLE_DRUN
    printf ( "\t* drun    %senabled%s\n", is_term ? color_green : "", is_term ? color_reset : "" );
#else
    printf ( "\t* drun    %sdisabled%s\n", is_term ? color_red : "", is_term ? color_reset : "" );
#endif
#ifdef TIMINGS
    printf ( "\t* timings %senabled%s\n", is_term ? color_green : "", is_term ? color_reset : "" );
#else
    printf ( "\t* timings %sdisabled%s\n", is_term ? color_red : "", is_term ? color_reset : "" );
#endif
#ifdef ENABLE_GCOV
    printf ( "\t* gcov    %senabled%s\n", is_term ? color_green : "", is_term ? color_reset : "" );
#else
    printf ( "\t* gcov    %sdisabled%s\n", is_term ? color_red : "", is_term ? color_reset : "" );
#endif
#ifdef ENABLE_ASAN
    printf ( "\t* asan    %senabled%s\n", is_term ? color_green : "", is_term ? color_reset : "" );
#else
    printf ( "\t* asan    %sdisabled%s\n", is_term ? color_red : "", is_term ? color_reset : "" );
#endif
    printf ( "\n" );
    printf ( "For more information see: %sman rofi%s\n", is_term ? color_bold : "", is_term ? color_reset : "" );
#ifdef GIT_VERSION
    printf ( "                 Version: %s"GIT_VERSION "%s\n", is_term ? color_bold : "", is_term ? color_reset : "" );
#else
    printf ( "                 Version: %s"VERSION "%s\n", is_term ? color_bold : "", is_term ? color_reset : "" );
#endif
    printf ( "              Bugreports: %s"PACKAGE_BUGREPORT "%s\n", is_term ? color_bold : "", is_term ? color_reset : "" );
    printf ( "                 Support: %s"PACKAGE_URL "%s\n", is_term ? color_bold : "", is_term ? color_reset : "" );
    printf ( "                          %s#rofi @ freenode.net%s\n", is_term ? color_bold : "", is_term ? color_reset : "" );
    if ( find_arg ( "-no-config" ) < 0 ) {
        printf ( "      Configuration file: %s%s%s\n", is_term ? color_bold : "", config_path, is_term ? color_reset : "" );
    }
    else {
        printf ( "      Configuration file: %sDisabled%s\n", is_term ? color_bold : "", is_term ? color_reset : "" );
    }
}

/**
 * Function bound by 'atexit'.
 * Cleanup globally allocated memory.
 */
static void cleanup ()
{
    for ( unsigned int i = 0; i < num_modi; i++ ) {
        mode_destroy ( modi[i] );
    }
    rofi_view_workers_finalize ();
    if ( main_loop != NULL  ) {
        if ( main_loop_source ) {
            g_water_xcb_source_free ( main_loop_source );
        }
        g_main_loop_unref ( main_loop );
        main_loop = NULL;
    }
    // XKB Cleanup
    //
    if ( xkb.compose.state != NULL ) {
        xkb_compose_state_unref ( xkb.compose.state );
        xkb.compose.state = NULL;
    }
    if ( xkb.compose.table != NULL ) {
        xkb_compose_table_unref ( xkb.compose.table );
        xkb.compose.table = NULL;
    }
    if ( xkb.state != NULL ) {
        xkb_state_unref ( xkb.state );
        xkb.state = NULL;
    }
    if ( xkb.keymap != NULL ) {
        xkb_keymap_unref ( xkb.keymap );
        xkb.keymap = NULL;
    }
    if ( xkb.context != NULL ) {
        xkb_context_unref ( xkb.context );
        xkb.context = NULL;
    }

    // Cleanup
    xcb_stuff_wipe ( xcb );

    // Cleaning up memory allocated by the Xresources file.
    config_xresource_free ();
    for ( unsigned int i = 0; i < num_modi; i++ ) {
        mode_free ( &( modi[i] ) );
    }
    g_free ( modi );

    // Cleanup the custom keybinding
    cleanup_abe ();

    g_free ( config_path );

    if ( list_of_error_msgs ) {
        for ( GList *iter = g_list_first ( list_of_error_msgs );
              iter != NULL; iter = g_list_next ( iter ) ) {
            g_string_free ( (GString *) iter->data, TRUE );
        }
        g_list_free ( list_of_error_msgs );
    }

    if ( rofi_theme ) {
        rofi_theme_free ( rofi_theme );
        rofi_theme = NULL;
    }
    TIMINGS_STOP ();
}

/**
 * Parse the switcher string, into internal array of type Mode.
 *
 * String is split on separator ','
 * First the three build-in modi are checked: window, run, ssh
 * if that fails, a script-switcher is created.
 */
static int add_mode ( const char * token )
{
    unsigned int index = num_modi;
    // Resize and add entry.
    modi = (Mode * *) g_realloc ( modi, sizeof ( Mode* ) * ( num_modi + 1 ) );

    // Window switcher.
#ifdef WINDOW_MODE
    if ( strcasecmp ( token, "window" ) == 0 ) {
        modi[num_modi] = &window_mode;
        num_modi++;
    }
    else if ( strcasecmp ( token, "windowcd" ) == 0 ) {
        modi[num_modi] = &window_mode_cd;
        num_modi++;
    }
    else
#endif // WINDOW_MODE
       // SSh dialog
    if ( strcasecmp ( token, "ssh" ) == 0 ) {
        modi[num_modi] = &ssh_mode;
        num_modi++;
    }
    else if ( strcasecmp ( token, mode_get_name ( &help_keys_mode ) ) == 0 ) {
        modi[num_modi] = &help_keys_mode;
        num_modi++;
    }
    // Run dialog
    else if ( strcasecmp ( token, "run" ) == 0 ) {
        modi[num_modi] = &run_mode;
        num_modi++;
    }
#ifdef ENABLE_DRUN
    else if ( strcasecmp ( token, "drun" ) == 0 ) {
        modi[num_modi] = &drun_mode;
        num_modi++;
    }
#endif
    // combi dialog
    else if ( strcasecmp ( token, "combi" ) == 0 ) {
        modi[num_modi] = &combi_mode;
        num_modi++;
    }
    else {
        // If not build in, use custom modi.
        Mode *sw = script_switcher_parse_setup ( token );
        if ( sw != NULL ) {
            modi[num_modi] = sw;
            mode_set_config ( sw );
            num_modi++;
        }
        else{
            // Report error, don't continue.
            fprintf ( stderr, "Invalid script switcher: %s\n", token );
        }
    }
    return ( index == num_modi ) ? -1 : (int) index;
}
static void setup_modi ( void )
{
    const char *const sep     = ",";
    char              *savept = NULL;
    // Make a copy, as strtok will modify it.
    char              *switcher_str = g_strdup ( config.modi );
    // Split token on ','. This modifies switcher_str.
    for ( char *token = strtok_r ( switcher_str, sep, &savept ); token != NULL; token = strtok_r ( NULL, sep, &savept ) ) {
        add_mode ( token );
    }
    // Free string that was modified by strtok_r
    g_free ( switcher_str );
    // We cannot do this in main loop, as we create pointer to string,
    // and re-alloc moves that pointer.
    mode_set_config ( &ssh_mode );
    mode_set_config ( &run_mode );
#ifdef ENABLE_DRUN
    mode_set_config ( &drun_mode );
#endif

#ifdef WINDOW_MODE
    mode_set_config ( &window_mode );
    mode_set_config ( &window_mode_cd );
#endif // WINDOW_MODE
    mode_set_config ( &combi_mode );
}

/**
 * Load configuration.
 * Following priority: (current), X, commandline arguments
 */
static inline void load_configuration ( )
{
    // Load distro default settings
    gchar *etc = g_build_filename ( SYSCONFDIR, "rofi.conf", NULL );
    if ( g_file_test ( etc, G_FILE_TEST_IS_REGULAR ) ) {
        config_parse_xresource_options_file ( etc );
    }
    g_free ( etc );
    // Load in config from X resources.
    config_parse_xresource_options ( xcb );
    config_parse_xresource_options_file ( config_path );
}
static inline void load_configuration_dynamic ( )
{
    // Load in config from X resources.
    config_parse_xresource_options_dynamic ( xcb );
    config_parse_xresource_options_dynamic_file ( config_path );
}

/**
 * Process X11 events in the main-loop (gui-thread) of the application.
 */
static void main_loop_x11_event_handler_view ( xcb_generic_event_t *ev )
{
    RofiViewState *state = rofi_view_get_active ();
    if ( state != NULL ) {
        rofi_view_itterrate ( state, ev, &xkb );
        if ( rofi_view_get_completed ( state ) ) {
            // This menu is done.
            rofi_view_finalize ( state );
            // cleanup
            if ( rofi_view_get_active () == NULL ) {
                g_main_loop_quit ( main_loop );
            }
        }
    }
}
static gboolean main_loop_x11_event_handler ( xcb_generic_event_t *ev, G_GNUC_UNUSED gpointer data )
{
    if ( ev == NULL ) {
        int status = xcb_connection_has_error ( xcb->connection );
        fprintf ( stderr, "The XCB connection to X server had a fatal error: %d\n", status );
        g_main_loop_quit ( main_loop );
        return G_SOURCE_REMOVE;
    }
    uint8_t type = ev->response_type & ~0x80;
    if ( type == xkb.first_event ) {
        switch ( ev->pad0 )
        {
        case XCB_XKB_MAP_NOTIFY:
            xkb_state_unref ( xkb.state );
            xkb_keymap_unref ( xkb.keymap );
            xkb.keymap = xkb_x11_keymap_new_from_device ( xkb.context, xcb->connection, xkb.device_id, 0 );
            xkb.state  = xkb_x11_state_new_from_device ( xkb.keymap, xcb->connection, xkb.device_id );
            break;
        case XCB_XKB_STATE_NOTIFY:
        {
            xcb_xkb_state_notify_event_t *ksne = (xcb_xkb_state_notify_event_t *) ev;
            guint                        modmask;
            xkb_state_update_mask ( xkb.state,
                                    ksne->baseMods,
                                    ksne->latchedMods,
                                    ksne->lockedMods,
                                    ksne->baseGroup,
                                    ksne->latchedGroup,
                                    ksne->lockedGroup );
            modmask = x11_get_current_mask ( &xkb );
            if ( modmask == 0 ) {
                abe_trigger_release ( );

                // Because of abe_trigger, state of rofi can be changed. handle this!
                // Run mainloop on dummy event.
                xcb_generic_event_t dev;
                dev.response_type = 0;
                main_loop_x11_event_handler_view ( &dev );
            }
            break;
        }
        }
        return G_SOURCE_CONTINUE;
    }
    if ( xcb->sndisplay != NULL ) {
        sn_xcb_display_process_event ( xcb->sndisplay, ev );
    }
    main_loop_x11_event_handler_view ( ev );
    return G_SOURCE_CONTINUE;
}

static gboolean main_loop_signal_handler_int ( G_GNUC_UNUSED gpointer data )
{
    // Break out of loop.
    g_main_loop_quit ( main_loop );
    return G_SOURCE_CONTINUE;
}

/** X server error depth. to handle nested errors. */
static int error_trap_depth = 0;
static void error_trap_push ( G_GNUC_UNUSED SnDisplay *display, G_GNUC_UNUSED xcb_connection_t *xdisplay )
{
    ++error_trap_depth;
}

static void error_trap_pop ( G_GNUC_UNUSED SnDisplay *display, xcb_connection_t *xdisplay )
{
    if ( error_trap_depth == 0 ) {
        fprintf ( stderr, "Error trap underflow!\n" );
        exit ( EXIT_FAILURE );
    }

    xcb_flush ( xdisplay );
    --error_trap_depth;
}
/** Retry count of grabbing keyboard. */
unsigned int lazy_grab_retry_count_kb = 0;
/** Retry count of grabbing pointer. */
unsigned int lazy_grab_retry_count_pt = 0;
static gboolean lazy_grab_pointer ( G_GNUC_UNUSED gpointer data )
{
    // After 5 sec.
    if ( lazy_grab_retry_count_pt > ( 5 * 1000 ) ) {
        fprintf ( stderr, "Failed to grab pointer after %u times. Giving up.\n", lazy_grab_retry_count_pt );
        return G_SOURCE_REMOVE;
    }
    if ( take_pointer ( xcb_stuff_get_root_window ( xcb ), 0 ) ) {
        return G_SOURCE_REMOVE;
    }
    lazy_grab_retry_count_pt++;
    return G_SOURCE_CONTINUE;
}
static gboolean lazy_grab_keyboard ( G_GNUC_UNUSED gpointer data )
{
    // After 5 sec.
    if ( lazy_grab_retry_count_kb > ( 5 * 1000 ) ) {
        fprintf ( stderr, "Failed to grab keyboard after %u times. Giving up.\n", lazy_grab_retry_count_kb );
        g_main_loop_quit (  main_loop );
        return G_SOURCE_REMOVE;
    }
    if ( take_keyboard ( xcb_stuff_get_root_window ( xcb ), 0 ) ) {
        return G_SOURCE_REMOVE;
    }
    lazy_grab_retry_count_kb++;
    return G_SOURCE_CONTINUE;
}

static gboolean startup ( G_GNUC_UNUSED gpointer data )
{
    TICK_N ( "Startup" );
    // flags to run immediately and exit
    char      *sname       = NULL;
    char      *msg         = NULL;
    MenuFlags window_flags = MENU_NORMAL;

    if ( find_arg ( "-normal-window" ) >= 0 ) {
        window_flags |= MENU_NORMAL_WINDOW;
    }

    /**
     * Create window (without showing)
     */
    // Try to grab the keyboard as early as possible.
    // We grab this using the rootwindow (as dmenu does it).
    // this seems to result in the smallest delay for most people.
    if ( ( window_flags & MENU_NORMAL_WINDOW ) == 0 ) {
        if ( find_arg ( "-no-lazy-grab" ) >= 0 ) {
            if ( !take_keyboard ( xcb_stuff_get_root_window ( xcb ), 500 ) ) {
                fprintf ( stderr, "Failed to grab keyboard, even after %d uS.", 500 * 1000 );
                g_main_loop_quit ( main_loop );
                return G_SOURCE_REMOVE;
            }
            if ( !take_pointer ( xcb_stuff_get_root_window ( xcb ), 100 ) ) {
                fprintf ( stderr, "Failed to grab mouse pointer, even after %d uS.", 100 * 1000 );
            }
        }
        else {
            if ( !take_keyboard ( xcb_stuff_get_root_window ( xcb ), 0 ) ) {
                g_timeout_add ( 1, lazy_grab_keyboard, NULL );
            }
            if ( !take_pointer ( xcb_stuff_get_root_window ( xcb ), 0 ) ) {
                g_timeout_add ( 1, lazy_grab_pointer, NULL );
            }
        }
    }
    TICK_N ( "Grab keyboard" );
    __create_window ( window_flags );
    TICK_N ( "Create Window" );
    // Parse the keybindings.
    if ( !parse_keys_abe () ) {
        // Error dialog
        return G_SOURCE_REMOVE;
    }
    TICK_N ( "Parse ABE" );
    // Sanity check
    config_sanity_check ( );
    TICK_N ( "Config sanity check" );

    if ( list_of_error_msgs != NULL ) {
        GString *emesg = g_string_new ( "The following errors where detected when starting rofi:\n" );
        GList   *iter  = g_list_first ( list_of_error_msgs );
        int     index  = 0;
        for (; iter != NULL && index < 2; iter = g_list_next ( iter ) ) {
            GString *msg = (GString *) ( iter->data );
            g_string_append ( emesg, "\n\n" );
            g_string_append ( emesg, msg->str );
            index++;
        }
        if ( g_list_length ( iter ) > 1 ) {
            g_string_append_printf ( emesg, "\nThere are <b>%d</b> more errors.", g_list_length ( iter ) - 1 );
        }
        rofi_view_error_dialog ( emesg->str, ERROR_MSG_MARKUP );
        g_string_free ( emesg, TRUE );
        return G_SOURCE_REMOVE;
    }
    // Dmenu mode.
    if ( dmenu_mode == TRUE ) {
        // force off sidebar mode:
        config.sidebar_mode = FALSE;
        int retv = dmenu_switcher_dialog ();
        if ( retv ) {
            rofi_set_return_code ( EXIT_SUCCESS );
            // Directly exit.
            g_main_loop_quit ( main_loop );
        }
    }
    else if ( find_arg_str (  "-e", &( msg ) ) ) {
        int markup = FALSE;
        if ( find_arg ( "-markup" ) >= 0 ) {
            markup = TRUE;
        }
        if (  !rofi_view_error_dialog ( msg, markup ) ) {
            g_main_loop_quit ( main_loop );
        }
    }
    else if ( find_arg_str ( "-show", &sname ) == TRUE ) {
        int index = switcher_get ( sname );
        if ( index < 0 ) {
            // Add it to the list
            index = add_mode ( sname );
            // Complain
            if ( index >= 0 ) {
                fprintf ( stdout, "Mode %s not enabled. Please add it to the list of enabled modi: %s\n",
                          sname, config.modi );
                fprintf ( stdout, "Adding mode: %s\n", sname );
            }
            // Run it anyway if found.
        }
        if ( index >= 0 ) {
            run_switcher ( index );
        }
        else {
            fprintf ( stderr, "The %s switcher has not been enabled\n", sname );
            g_main_loop_quit ( main_loop );
            return G_SOURCE_REMOVE;
        }
    }
    else if ( find_arg ( "-show" ) >= 0 && num_modi > 0 ) {
        run_switcher ( 0 );
    }
    else{
        // Daemon mode
        fprintf ( stderr, "Rofi daemon mode is now removed.\n" );
        fprintf ( stderr, "Please use your window manager binding functionality or xbindkeys to replace it.\n" );
        g_main_loop_quit ( main_loop );
    }

    return G_SOURCE_REMOVE;
}

/**
 * @param argc number of input arguments.
 * @param argv array of the input arguments.
 *
 * Main application entry point.
 *
 * @returns return code of rofi.
 */
int main ( int argc, char *argv[] )
{
    TIMINGS_START ();

    cmd_set_arguments ( argc, argv );

    // Version
    if ( find_arg (  "-v" ) >= 0 || find_arg (  "-version" ) >= 0 ) {
#ifdef GIT_VERSION
        fprintf ( stdout, "Version: "GIT_VERSION "\n" );
#else
        fprintf ( stdout, "Version: "VERSION "\n" );
#endif
        exit ( EXIT_SUCCESS );
    }

    // Detect if we are in dmenu mode.
    // This has two possible causes.
    // 1 the user specifies it on the command-line.
    if ( find_arg (  "-dmenu" ) >= 0 ) {
        dmenu_mode = TRUE;
    }
    // 2 the binary that executed is called dmenu (e.g. symlink to rofi)
    else{
        // Get the base name of the executable called.
        char               *base_name = g_path_get_basename ( argv[0] );
        const char * const dmenu_str  = "dmenu";
        dmenu_mode = ( strcmp ( base_name, dmenu_str ) == 0 );
        // Free the basename for dmenu detection.
        g_free ( base_name );
    }
    TICK ();
    // Get the path to the cache dir.
    cache_dir = g_get_user_cache_dir ();

    if ( g_mkdir_with_parents ( cache_dir, 0700 ) < 0 ) {
        fprintf ( stderr, "Failed to create cache directory: %s\n", strerror ( errno ) );
        return EXIT_FAILURE;
    }

    // Create pid file path.
    const char *path = g_get_user_runtime_dir ();
    if ( path ) {
        if ( g_mkdir_with_parents ( path, 0700 ) < 0 ) {
            fprintf ( stderr, "Failed to create user runtime directory: %s\n", strerror ( errno ) );
            return EXIT_FAILURE;
        }
        pidfile = g_build_filename ( path, "rofi.pid", NULL );
    }
    config_parser_add_option ( xrm_String, "pid", (void * *) &pidfile, "Pidfile location" );

    if ( find_arg ( "-config" ) < 0 ) {
        const char *cpath = g_get_user_config_dir ();
        if ( cpath ) {
            config_path = g_build_filename ( cpath, "rofi", "config", NULL );
        }
    }
    else {
        char *c = NULL;
        find_arg_str ( "-config", &c );
        config_path = rofi_expand_path ( c );
    }

    TICK ();
    // Register cleanup function.
    atexit ( cleanup );

    TICK ();
    if ( setlocale ( LC_ALL, "" ) == NULL ) {
        fprintf ( stderr, "Failed to set locale.\n" );
        return EXIT_FAILURE;
    }

    // Get DISPLAY, first env, then argument.
    // We never modify display_str content.
    char *display_str = ( char *)g_getenv ( "DISPLAY" );
    find_arg_str (  "-display", &display_str );

    xcb->connection = xcb_connect ( display_str, &xcb->screen_nbr );
    if ( xcb_connection_has_error ( xcb->connection ) ) {
        fprintf ( stderr, "Failed to open display: %s", display_str );
        return EXIT_FAILURE;
    }
    TICK_N ( "Open Display" );

    xcb->screen = xcb_aux_get_screen ( xcb->connection, xcb->screen_nbr );

    x11_build_monitor_layout ();

    xcb_intern_atom_cookie_t *ac     = xcb_ewmh_init_atoms ( xcb->connection, &xcb->ewmh );
    xcb_generic_error_t      *errors = NULL;
    xcb_ewmh_init_atoms_replies ( &xcb->ewmh, ac, &errors );
    if ( errors ) {
        fprintf ( stderr, "Failed to create EWMH atoms\n" );
        free ( errors );
    }
    // Discover the current active window manager.
    x11_helper_discover_window_manager();
    TICK_N ( "Setup XCB" );

    if ( xkb_x11_setup_xkb_extension ( xcb->connection, XKB_X11_MIN_MAJOR_XKB_VERSION, XKB_X11_MIN_MINOR_XKB_VERSION,
                                       XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS, NULL, NULL, &xkb.first_event, NULL ) < 0 ) {
        fprintf ( stderr, "cannot setup XKB extension!\n" );
        return EXIT_FAILURE;
    }

    xkb.context = xkb_context_new ( XKB_CONTEXT_NO_FLAGS );
    if ( xkb.context == NULL ) {
        fprintf ( stderr, "cannot create XKB context!\n" );
        return EXIT_FAILURE;
    }
    xkb.xcb_connection = xcb->connection;

    xkb.device_id = xkb_x11_get_core_keyboard_device_id ( xcb->connection );

    enum
    {
        required_events =
            ( XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY |
              XCB_XKB_EVENT_TYPE_MAP_NOTIFY |
              XCB_XKB_EVENT_TYPE_STATE_NOTIFY ),

        required_nkn_details =
            ( XCB_XKB_NKN_DETAIL_KEYCODES ),

        required_map_parts   =
            ( XCB_XKB_MAP_PART_KEY_TYPES |
              XCB_XKB_MAP_PART_KEY_SYMS |
              XCB_XKB_MAP_PART_MODIFIER_MAP |
              XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS |
              XCB_XKB_MAP_PART_KEY_ACTIONS |
              XCB_XKB_MAP_PART_VIRTUAL_MODS |
              XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP ),

        required_state_details =
            ( XCB_XKB_STATE_PART_MODIFIER_BASE |
              XCB_XKB_STATE_PART_MODIFIER_LATCH |
              XCB_XKB_STATE_PART_MODIFIER_LOCK |
              XCB_XKB_STATE_PART_GROUP_BASE |
              XCB_XKB_STATE_PART_GROUP_LATCH |
              XCB_XKB_STATE_PART_GROUP_LOCK ),
    };

    static const xcb_xkb_select_events_details_t details = {
        .affectNewKeyboard  = required_nkn_details,
        .newKeyboardDetails = required_nkn_details,
        .affectState        = required_state_details,
        .stateDetails       = required_state_details,
    };
    xcb_xkb_select_events ( xcb->connection, xkb.device_id, required_events, /* affectWhich */
                            0,                                               /* clear */
                            required_events,                                 /* selectAll */
                            required_map_parts,                              /* affectMap */
                            required_map_parts,                              /* map */
                            &details );

    xkb.keymap = xkb_x11_keymap_new_from_device ( xkb.context, xcb->connection, xkb.device_id, XKB_KEYMAP_COMPILE_NO_FLAGS );
    if ( xkb.keymap == NULL ) {
        fprintf ( stderr, "Failed to get Keymap for current keyboard device.\n" );
        return EXIT_FAILURE;
    }
    xkb.state = xkb_x11_state_new_from_device ( xkb.keymap, xcb->connection, xkb.device_id );
    if ( xkb.state == NULL ) {
        fprintf ( stderr, "Failed to get state object for current keyboard device.\n" );
        return EXIT_FAILURE;
    }

    xkb.compose.table = xkb_compose_table_new_from_locale ( xkb.context, setlocale ( LC_CTYPE, NULL ), 0 );
    if ( xkb.compose.table != NULL ) {
        xkb.compose.state = xkb_compose_state_new ( xkb.compose.table, 0 );
    }
    else {
        fprintf ( stderr, "Failed to get keyboard compose table. Trying to limp on.\n" );
    }

    if ( xcb_connection_has_error ( xcb->connection ) ) {
        fprintf ( stderr, "Connection has error\n" );
        exit ( EXIT_FAILURE );
    }
    x11_setup ( &xkb );
    TICK_N ( "Setup xkb" );
    if ( xcb_connection_has_error ( xcb->connection ) ) {
        fprintf ( stderr, "Connection has error\n" );
        exit ( EXIT_FAILURE );
    }
    main_loop = g_main_loop_new ( NULL, FALSE );

    TICK_N ( "Setup mainloop" );
    // startup not.
    xcb->sndisplay = sn_xcb_display_new ( xcb->connection, error_trap_push, error_trap_pop );
    if ( xcb_connection_has_error ( xcb->connection ) ) {
        fprintf ( stderr, "Connection has error\n" );
        exit ( EXIT_FAILURE );
    }

    if ( xcb->sndisplay != NULL ) {
        xcb->sncontext = sn_launchee_context_new_from_environment ( xcb->sndisplay, xcb->screen_nbr );
    }
    if ( xcb_connection_has_error ( xcb->connection ) ) {
        fprintf ( stderr, "Connection has error\n" );
        exit ( EXIT_FAILURE );
    }
    TICK_N ( "Startup Notification" );
    // Setup keybinding
    setup_abe ();
    TICK_N ( "Setup abe" );

    if ( find_arg ( "-no-config" ) < 0 ) {
        load_configuration ( );
    }
    // Parse command line for settings, independent of other -no-config.
    config_parse_cmd_options ( );

    if ( !dmenu_mode ) {
        // setup_modi
        setup_modi ();
    }

    if ( find_arg ( "-no-config" ) < 0 ) {
        // Reload for dynamic part.
        load_configuration_dynamic ( );
    }
    // Parse command line for settings, independent of other -no-config.
    config_parse_cmd_options_dynamic (  );

    if ( config.theme ) {
        TICK_N ( "Parse theme" );
        if ( rofi_theme_parse_file ( config.theme ) ) {
            // TODO: instantiate fallback theme.?
            rofi_theme_free ( rofi_theme );
            rofi_theme = NULL;
        }
        TICK_N ( "Parsed theme" );
    }
    else {
        rofi_theme_convert_old_theme ( );
    }

    const char ** theme_str = find_arg_strv ( "-theme-str" );
    if ( theme_str ) {
        for ( int index = 0; theme_str && theme_str[index]; index++ ) {
            if ( rofi_theme_parse_string ( theme_str[index] ) ) {
                rofi_theme_free ( rofi_theme );
                rofi_theme = NULL;
            }
        }
        g_free ( theme_str );
    }

    if ( find_arg ( "-dump-theme" ) >= 0 ) {
        rofi_theme_print ( rofi_theme );
        exit ( EXIT_SUCCESS );
    }
    // Dump.
    // catch help request
    if ( find_arg (  "-h" ) >= 0 || find_arg (  "-help" ) >= 0 || find_arg (  "--help" ) >= 0 ) {
        help ( argc, argv );
        exit ( EXIT_SUCCESS );
    }
    if ( find_arg (  "-dump-xresources" ) >= 0 ) {
        config_parse_xresource_dump ();
        exit ( EXIT_SUCCESS );
    }
    if ( find_arg (  "-dump-xresources-theme" ) >= 0 ) {
        config_parse_xresources_theme_dump ();
        exit ( EXIT_SUCCESS );
    }

    main_loop_source = g_water_xcb_source_new_for_connection ( NULL, xcb->connection, main_loop_x11_event_handler, NULL, NULL );

    TICK_N ( "X11 Setup " );

    rofi_view_workers_initialize ();

    // Setup signal handling sources.
    // SIGINT
    g_unix_signal_add ( SIGINT, main_loop_signal_handler_int, NULL );

    g_idle_add ( startup, NULL );

    // Pidfile + visuals
    int pfd = setup ();
    if ( pfd < 0 ) {
        return EXIT_FAILURE;
    }
    // Start mainloop.
    g_main_loop_run ( main_loop );
    teardown ( pfd );
    return return_code;
}
