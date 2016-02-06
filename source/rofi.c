/**
 * rofi
 *
 * MIT/X11 License
 * Copyright (c) 2012 Sean Pringle <sean.pringle@gmail.com>
 * Modified 2013-2016 Qball Cow <qball@gmpclient.org>
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
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <locale.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <glib-unix.h>

#include <cairo.h>
#include <cairo-xlib.h>

#define SN_API_NOT_YET_FROZEN
#include <libsn/sn.h>

#include "settings.h"
#include "mode.h"
#include "rofi.h"
#include "helper.h"
#include "textbox.h"
#include "x11-helper.h"
#include "x11-event-source.h"
#include "xrmoptions.h"
#include "dialogs/dialogs.h"

gboolean          daemon_mode = FALSE;
// Pidfile.
char              *pidfile           = NULL;
const char        *cache_dir         = NULL;
SnDisplay         *sndisplay         = NULL;
SnLauncheeContext *sncontext         = NULL;
Display           *display           = NULL;
char              *display_str       = NULL;
char              *config_path       = NULL;
unsigned int      normal_window_mode = FALSE;
// Array of modi.
Mode              **modi   = NULL;
unsigned int      num_modi = 0;
// Current selected switcher.
unsigned int      curr_switcher = 0;

GThreadPool       *tpool               = NULL;
GMainLoop         *main_loop           = NULL;
GSource           *main_loop_source    = NULL;
gboolean          quiet                = FALSE;
RofiViewState     *current_active_menu = NULL;

static void process_result ( RofiViewState *state );
gboolean main_loop_x11_event_handler ( G_GNUC_UNUSED gpointer data );

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

#include "view.h"
#include "view-internal.h"

/**
 * @param key the Key to match
 * @param modstate the modifier state to match
 *
 * Match key and modifier state against modi.
 *
 * @return the index of the switcher that matches the key combination
 * specified by key and modstate. Returns -1 if none was found
 */
extern unsigned int NumlockMask;
int locate_switcher ( KeySym key, unsigned int modstate )
{
    // ignore annoying modifiers
    unsigned int modstate_filtered = modstate & ~( LockMask | NumlockMask );
    for ( unsigned int i = 0; i < num_modi; i++ ) {
        if ( mode_check_keybinding ( modi[i], key, modstate_filtered ) ) {
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
        create_visual_and_colormap ( display );
        textbox_setup ( display );
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
    release_keyboard ( display );

    // Cleanup view
    rofi_view_cleanup ();
    // Cleanup pid file.
    remove_pid_file ( pfd );
}

/**
 * Start dmenu mode.
 */
static int run_dmenu ()
{
    int ret_state = EXIT_FAILURE;
    int pfd       = setup ();
    if ( pfd < 0 ) {
        return ret_state;
    }

    // Dmenu modi has a return state.
    ret_state = dmenu_switcher_dialog ();
    teardown ( pfd );
    return ret_state;
}

static int pfd = -1;
static void run_switcher ( ModeMode mode )
{
    pfd = setup ();
    if ( pfd < 0 ) {
        return;
    }
    // Otherwise check if requested mode is enabled.
    for ( unsigned int i = 0; i < num_modi; i++ ) {
        if ( !mode_init ( modi[i] ) ) {
            error_dialog ( ERROR_MSG ( "Failed to initialize all the modi." ), ERROR_MSG_MARKUP );
            teardown ( pfd );
            return;
        }
    }
    char          *input  = g_strdup ( config.filter );
    char          *prompt = g_strdup_printf ( "%s:", mode_get_name ( modi[mode] ) );
    curr_switcher = mode;
    RofiViewState * state = rofi_view_create ( modi[mode], input, prompt, NULL, MENU_NORMAL );
    state->finalize = process_result;
    rofi_view_set_active ( state );
    g_free ( prompt );
}
static void process_result ( RofiViewState *state )
{
    unsigned int selected_line = rofi_view_get_selected_line ( state );;
    MenuReturn   mretv         = rofi_view_get_return_value ( state );
    char         *input        = g_strdup ( rofi_view_get_user_input ( state ) );
    rofi_view_set_active ( NULL );
    rofi_view_free ( state );
    ModeMode retv = mode_result ( modi[curr_switcher], mretv, &input, selected_line );

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
    else if ( retv < MODE_EXIT ) {
        mode = ( retv ) % num_modi;
    }
    else {
        mode = retv;
    }
    if ( mode != MODE_EXIT ) {
        char          *prompt = g_strdup_printf ( "%s:", mode_get_name ( modi[mode] ) );
        curr_switcher = mode;
        RofiViewState * state = rofi_view_create ( modi[mode], input, prompt, NULL, MENU_NORMAL );
        state->finalize = process_result;
        g_free ( prompt );
        // TODO FIX
        //g_return_val_if_fail ( state != NULL, MODE_EXIT );
        rofi_view_set_active ( state );
        g_free ( input );
        main_loop_x11_event_handler ( NULL );
        return;
    }
    // Cleanup
    g_free ( input );
    for ( unsigned int i = 0; i < num_modi; i++ ) {
        mode_destroy ( modi[i] );
    }
    // cleanup
    teardown ( pfd );
    if ( !daemon_mode ) {
        g_main_loop_quit ( main_loop );
    }
}

int show_error_message ( const char *msg, int markup )
{
    int pfd = setup ();
    if ( pfd < 0 ) {
        return EXIT_FAILURE;
    }
    error_dialog ( msg, markup );
    teardown ( pfd );
    // TODO this looks incorrect.
    g_main_loop_quit ( main_loop );
    return EXIT_SUCCESS;
}

/**
 * Function that listens for global key-presses.
 * This is only used when in daemon mode.
 */
static void handle_keypress ( XEvent *ev )
{
    int    index;
    KeySym key = XkbKeycodeToKeysym ( display, ev->xkey.keycode, 0, 0 );
    index = locate_switcher ( key, ev->xkey.state );
    if ( index >= 0 ) {
        run_switcher ( index );
    }
    else {
        fprintf ( stderr,
                  "Warning: Unhandled keypress in global keyhandler, keycode = %u mask = %u\n",
                  ev->xkey.keycode,
                  ev->xkey.state );
    }
}

/**
 * Help function.
 */
static void print_main_application_options ( void )
{
    int is_term = isatty ( fileno ( stdout ) );
    print_help_msg ( "-no-config", "", "Do not load configuration, use default values.", NULL, is_term );
    print_help_msg ( "-quiet", "", "Suppress information messages.", NULL, is_term );
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
}
static void help ( G_GNUC_UNUSED int argc, char **argv )
{
    printf ( "%s usage:\n", argv[0] );
    printf ( "\t%s [-options ...]\n\n", argv[0] );
    printf ( "Command line only options:\n" );
    print_main_application_options ();
    printf ( "DMENU command line options:\n" );
    print_dmenu_options ();
    printf ( "Global options:\n" );
    print_options ();
    printf ( "\n" );
    printf ( "For more information see: man rofi\n" );
    printf ( "Version:    "VERSION "\n" );
    printf ( "Bugreports: "PACKAGE_BUGREPORT "\n" );
}

static void release_global_keybindings ()
{
    for ( unsigned int i = 0; i < num_modi; i++ ) {
        mode_ungrab_key ( modi[i], display );
    }
}
static int grab_global_keybindings ()
{
    int key_bound = FALSE;
    for ( unsigned int i = 0; i < num_modi; i++ ) {
        if ( mode_grab_key ( modi[i], display ) ) {
            key_bound = TRUE;
        }
    }
    return key_bound;
}

/**
 * Function bound by 'atexit'.
 * Cleanup globally allocated memory.
 */
static void cleanup ()
{
    if ( tpool ) {
        g_thread_pool_free ( tpool, TRUE, FALSE );
        tpool = NULL;
    }
    if ( main_loop != NULL  ) {
        if ( main_loop_source ) {
            g_source_destroy ( main_loop_source );
        }
        g_main_loop_unref ( main_loop );
        main_loop = NULL;
    }
    if ( daemon_mode ) {
        release_global_keybindings ();
        if ( !quiet ) {
            fprintf ( stdout, "Quit from daemon mode.\n" );
        }
    }
    // Cleanup
    if ( display != NULL ) {
        if ( sncontext != NULL ) {
            sn_launchee_context_unref ( sncontext );
            sncontext = NULL;
        }
        if ( sndisplay != NULL ) {
            sn_display_unref ( sndisplay );
            sndisplay = NULL;
        }
        XCloseDisplay ( display );
        display = NULL;
    }

    // Cleaning up memory allocated by the Xresources file.
    config_xresource_free ();
    for ( unsigned int i = 0; i < num_modi; i++ ) {
        mode_free ( &( modi[i] ) );
    }
    g_free ( modi );

    // Cleanup the custom keybinding
    cleanup_abe ();

    g_free ( config_path );

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
    // Run dialog
    else if ( strcasecmp ( token, "run" ) == 0 ) {
        modi[num_modi] = &run_mode;
        num_modi++;
    }
    else if ( strcasecmp ( token, "drun" ) == 0 ) {
        modi[num_modi] = &drun_mode;
        num_modi++;
    }
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
            token = NULL;
        }
    }
    return ( index == num_modi ) ? -1 : (int) index;
}
static void setup_modi ( void )
{
    char *savept = NULL;
    // Make a copy, as strtok will modify it.
    char *switcher_str = g_strdup ( config.modi );
    // Split token on ','. This modifies switcher_str.
    for ( char *token = strtok_r ( switcher_str, ",", &savept ); token != NULL; token = strtok_r ( NULL, ",", &savept ) ) {
        add_mode ( token );
    }
    // Free string that was modified by strtok_r
    g_free ( switcher_str );
    // We cannot do this in main loop, as we create pointer to string,
    // and re-alloc moves that pointer.
    for ( unsigned int i = 0; i < num_modi; i++ ) {
        mode_setup_keybinding ( modi[i] );
    }
    mode_set_config ( &ssh_mode );
    mode_set_config ( &run_mode );
    mode_set_config ( &drun_mode );

#ifdef WINDOW_MODE
    mode_set_config ( &window_mode );
    mode_set_config ( &window_mode_cd );
#endif // WINDOW_MODE
    mode_set_config ( &combi_mode );
}

/**
 * @param display Pointer to the X connection to use.
 * Load configuration.
 * Following priority: (current), X, commandline arguments
 */
static inline void load_configuration ( Display *display )
{
    // Load in config from X resources.
    config_parse_xresource_options ( display );
    config_parse_xresource_options_file ( config_path );

    // Parse command line for settings.
    config_parse_cmd_options ( );
}
static inline void load_configuration_dynamic ( Display *display )
{
    // Load in config from X resources.
    config_parse_xresource_options_dynamic ( display );
    config_parse_xresource_options_dynamic_file ( config_path );
    config_parse_cmd_options_dynamic (  );
}

static void print_global_keybindings ()
{
    fprintf ( stdout, "listening to the following keys:\n" );
    for ( unsigned int i = 0; i < num_modi; i++ ) {
        mode_print_keybindings ( modi[i] );
    }
}

static void reload_configuration ()
{
    if ( find_arg ( "-no-config" ) < 0 ) {
        TICK ();
        // Reset the color cache
        color_cache_reset ();
        // We need to open a new connection to X11, otherwise we get old
        // configuration
        Display *temp_display = XOpenDisplay ( display_str );
        if ( temp_display ) {
            load_configuration ( temp_display );
            load_configuration_dynamic ( temp_display );

            // Sanity check
            config_sanity_check ( temp_display );
            parse_keys_abe ();
            XCloseDisplay ( temp_display );
        }
        else {
            fprintf ( stderr, "Failed to get a new connection to the X11 server. No point in continuing.\n" );
            abort ();
        }
        TICK_N ( "Load config" );
    }
}

/**
 * Process X11 events in the main-loop (gui-thread) of the application.
 */
gboolean main_loop_x11_event_handler ( G_GNUC_UNUSED gpointer data )
{
    if ( current_active_menu != NULL ) {
        while ( XPending ( display ) ) {
            XEvent ev;
            // Read event, we know this won't block as we checked with XPending.
            XNextEvent ( display, &ev );
            rofi_view_itterrate ( current_active_menu, &ev );
        }
        if ( rofi_view_get_completed ( current_active_menu ) ) {
            // This menu is done.
            rofi_view_finalize ( current_active_menu );
        }
        return G_SOURCE_CONTINUE;
    }
    // X11 produced an event. Consume them.
    while ( XPending ( display ) ) {
        XEvent ev;
        // Read event, we know this won't block as we checked with XPending.
        XNextEvent ( display, &ev );
        if ( sndisplay != NULL ) {
            sn_display_process_event ( sndisplay, &ev );
        }
        // If we get an event that does not belong to a window:
        // Ignore it.
        if ( ev.xany.window == None ) {
            continue;
        }
        // If keypress, handle it.
        if ( ev.type == KeyPress ) {
            handle_keypress ( &ev );
        }
    }
    return G_SOURCE_CONTINUE;
}

/**
 * Process signals in the main-loop (gui-thread) of the application.
 *
 * returns TRUE when mainloop should be stopped.
 */
static gboolean main_loop_signal_handler_hup ( G_GNUC_UNUSED gpointer data )
{
    fprintf ( stdout, "Reload configuration\n" );
    // Release the keybindings.
    release_global_keybindings ();
    // Reload config
    reload_configuration ();
    // Grab the possibly new keybindings.
    grab_global_keybindings ();
    // We need to flush, otherwise the first key presses are not caught.
    XFlush ( display );
    return G_SOURCE_CONTINUE;
}
static gboolean main_loop_signal_handler_int ( G_GNUC_UNUSED gpointer data )
{
    // Break out of loop.
    g_main_loop_quit ( main_loop );
    return G_SOURCE_CONTINUE;
}

static gboolean main_loop_signal_handler_usr1 ( G_GNUC_UNUSED gpointer data )
{
    config_parse_xresource_dump ();
    return G_SOURCE_CONTINUE;
}

static int error_trap_depth = 0;
static void error_trap_push ( G_GNUC_UNUSED SnDisplay *display, G_GNUC_UNUSED Display   *xdisplay )
{
    ++error_trap_depth;
}

static void error_trap_pop ( G_GNUC_UNUSED SnDisplay *display, Display   *xdisplay )
{
    if ( error_trap_depth == 0 ) {
        fprintf ( stderr, "Error trap underflow!\n" );
        exit ( EXIT_FAILURE );
    }

    XSync ( xdisplay, False ); /* get all errors out of the queue */
    --error_trap_depth;
}
static gboolean delayed_start ( G_GNUC_UNUSED gpointer data )
{
    // Force some X Events to be handled.. seems the only way to get a reliable startup.
    rofi_view_queue_redraw ();
    main_loop_x11_event_handler ( NULL );
    // rofi_view_queue_redraw();
    return FALSE;
}
int main ( int argc, char *argv[] )
{
    TIMINGS_START ();

    cmd_set_arguments ( argc, argv );
    // Quiet flag
    quiet = ( find_arg ( "-quiet" ) >= 0 );
    // Version
    if ( find_arg (  "-v" ) >= 0 || find_arg (  "-version" ) >= 0 ) {
        fprintf ( stdout, "Version: "VERSION "\n" );
        exit ( EXIT_SUCCESS );
    }

    // Detect if we are in dmenu mode.
    // This has two possible causes.
    // 1 the user specifies it on the command-line.
    int dmenu_mode = FALSE;
    if ( find_arg (  "-dmenu" ) >= 0 ) {
        dmenu_mode = TRUE;
    }
    // 2 the binary that executed is called dmenu (e.g. symlink to rofi)
    else{
        // Get the base name of the executable called.
        char *base_name = g_path_get_basename ( argv[0] );
        dmenu_mode = ( strcmp ( base_name, "dmenu" ) == 0 );
        // Free the basename for dmenu detection.
        g_free ( base_name );
    }
    TICK ();
    // Get the path to the cache dir.
    cache_dir = g_get_user_cache_dir ();

    // Create pid file path.
    const char *path = g_get_user_runtime_dir ();
    if ( path ) {
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
    // Get DISPLAY, first env, then argument.
    display_str = getenv ( "DISPLAY" );
    find_arg_str (  "-display", &display_str );

    if ( setlocale ( LC_ALL, "" ) == NULL ) {
        fprintf ( stderr, "Failed to set locale.\n" );
        return EXIT_FAILURE;
    }

    if ( !XSupportsLocale () ) {
        fprintf ( stderr, "X11 does not support locales\n" );
        return EXIT_FAILURE;
    }
    if ( XSetLocaleModifiers ( "@im=none" ) == NULL ) {
        fprintf ( stderr, "Failed to set locale modifier.\n" );
        return EXIT_FAILURE;
    }
    if ( !( display = XOpenDisplay ( display_str ) ) ) {
        fprintf ( stderr, "cannot open display!\n" );
        return EXIT_FAILURE;
    }
    TICK_N ( "Open Display" );

    main_loop = g_main_loop_new ( NULL, FALSE );

    TICK_N ( "Setup mainloop" );
    // startup not.
    sndisplay = sn_display_new ( display, error_trap_push, error_trap_pop );

    if ( sndisplay != NULL ) {
        sncontext = sn_launchee_context_new_from_environment ( sndisplay, DefaultScreen ( display ) );
    }
    TICK_N ( "Startup Notification" );

    // Initialize Xresources subsystem.
    config_parse_xresource_init ();
    TICK_N ( "Initialize Xresources system" );
    // Setup keybinding
    setup_abe ();
    TICK_N ( "Setup abe" );

    if ( find_arg ( "-no-config" ) < 0 ) {
        load_configuration ( display );
    }
    if ( !dmenu_mode ) {
        // setup_modi
        setup_modi ();
    }
    else {
        // Add dmenu options.
        config_parser_add_option ( xrm_Char, "sep", (void * *) &( config.separator ), "Element separator" );
    }
    if ( find_arg ( "-no-config" ) < 0 ) {
        // Reload for dynamic part.
        load_configuration_dynamic ( display );
    }

    x11_setup ( display );

    TICK_N ( "X11 Setup " );
    // Sanity check
    config_sanity_check ( display );
    TICK_N ( "Config sanity check" );
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
    main_loop_source = x11_event_source_new ( display );
    x11_event_source_set_callback ( main_loop_source, main_loop_x11_event_handler );
    // Parse the keybindings.
    parse_keys_abe ();
    TICK_N ( "Parse ABE" );
    char *msg = NULL;
    if ( find_arg_str (  "-e", &( msg ) ) ) {
        int markup = FALSE;
        if ( find_arg ( "-markup" ) >= 0 ) {
            markup = TRUE;
        }
        return show_error_message ( msg, markup );
    }

    // Create thread pool
    GError *error = NULL;
    tpool = g_thread_pool_new ( rofi_view_call_thread, NULL, config.threads, FALSE, &error );
    if ( error == NULL ) {
        // Idle threads should stick around for a max of 60 seconds.
        g_thread_pool_set_max_idle_time ( 60000 );
        // We are allowed to have
        g_thread_pool_set_max_threads ( tpool, config.threads, &error );
    }
    // If error occured during setup of pool, tell user and exit.
    if ( error != NULL ) {
        char *msg = g_strdup_printf ( "Failed to setup thread pool: '%s'", error->message );
        show_error_message ( msg, FALSE );
        g_free ( msg );
        g_error_free ( error );
        return EXIT_FAILURE;
    }

    TICK_N ( "Setup Threadpool" );
    // Dmenu mode.
    if ( dmenu_mode == TRUE ) {
        normal_window_mode = find_arg ( "-normal-window" ) >= 0;
        // force off sidebar mode:
        config.sidebar_mode = FALSE;
        int retv = run_dmenu ();

        // User canceled the operation.
        if ( retv == FALSE ) {
            return EXIT_FAILURE;
        }
        else if ( retv >= 10 ) {
            return retv;
        }
        return EXIT_SUCCESS;
    }

    // Setup signal handling sources.
    // SIGHup signal.
    g_unix_signal_add ( SIGHUP, main_loop_signal_handler_hup, NULL );
    // SIGINT
    g_unix_signal_add ( SIGINT, main_loop_signal_handler_int, NULL );
    // SIGUSR1
    g_unix_signal_add ( SIGUSR1, main_loop_signal_handler_usr1, NULL );
    // flags to run immediately and exit
    char *sname = NULL;
    if ( find_arg_str ( "-show", &sname ) == TRUE ) {
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
            g_idle_add ( delayed_start, GINT_TO_POINTER ( index ) );
        }
        else {
            fprintf ( stderr, "The %s switcher has not been enabled\n", sname );
            return EXIT_FAILURE;
        }
    }
    else{
        // Daemon mode, Listen to key presses..
        if ( !grab_global_keybindings () ) {
            fprintf ( stderr, "Rofi was launched in daemon mode, but no key-binding was specified.\n" );
            fprintf ( stderr, "Please check the manpage on how to specify a key-binding.\n" );
            fprintf ( stderr, "The following modi are enabled and keys can be specified:\n" );
            for ( unsigned int i = 0; i < num_modi; i++ ) {
                const char *name = mode_get_name ( modi[i] );
                fprintf ( stderr, "\t* "color_bold "%s"color_reset ": -key-%s <key>\n", name, name );
            }
            // Cleanup
            return EXIT_FAILURE;
        }
        if ( !quiet ) {
            fprintf ( stdout, "Rofi is launched in daemon mode.\n" );
            print_global_keybindings ();
        }

        // done starting deamon.

        if ( sncontext != NULL ) {
            sn_launchee_context_complete ( sncontext );
        }
        daemon_mode = TRUE;
        XSelectInput ( display, DefaultRootWindow ( display ), KeyPressMask );
        XFlush ( display );
    }

    // Start mainloop.
    g_main_loop_run ( main_loop );

    return EXIT_SUCCESS;
}
