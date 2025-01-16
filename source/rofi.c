/*
 * rofi
 *
 * MIT/X11 License
 * Copyright © 2012 Sean Pringle <sean.pringle@gmail.com>
 * Copyright © 2013-2023 Qball Cow <qball@gmpclient.org>
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

/** Log domain */
#define G_LOG_DOMAIN "Rofi"

#include "config.h"
#include <errno.h>
#include <gmodule.h>
#include <locale.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sysexits.h>
#include <time.h>
#include <unistd.h>
#include <xcb/xcb.h>

#include <glib-unix.h>

#include <libgwater-xcb.h>

#ifdef USE_NK_GIT_VERSION
#include "nkutils-git-version.h"
#ifdef NK_GIT_VERSION
#define GIT_VERSION NK_GIT_VERSION
#endif
#endif

#include "resources.h"

#include "display.h"
#include "rofi.h"
#include "settings.h"

#include "helper.h"
#include "mode.h"
#include "modes/modes.h"
#include "widgets/textbox.h"
#include "xrmoptions.h"

#include "view-internal.h"
#include "view.h"

#include "rofi-icon-fetcher.h"
#include "theme.h"

#include "timings.h"

// Plugin abi version.
// TODO: move this check to mode.c
#include "mode-private.h"

/** Location of pidfile for this instance. */
char *pidfile = NULL;
/** Location of Cache directory. */
const char *cache_dir = NULL;
/** if the cache_dir string is allocated, keep pointer here so it can be freed.
 */
char *cache_dir_alloc = NULL;

/** List of error messages.*/
GList *list_of_error_msgs = NULL;
/** List of warning messages for the user.*/
GList *list_of_warning_msgs = NULL;

static void rofi_collectmodes_destroy(void);
void rofi_add_error_message(GString *str) {
  list_of_error_msgs = g_list_append(list_of_error_msgs, str);
}
void rofi_clear_error_messages(void) {
  if (list_of_error_msgs) {
    for (GList *iter = g_list_first(list_of_error_msgs); iter != NULL;
         iter = g_list_next(iter)) {
      g_string_free((GString *)iter->data, TRUE);
    }
    g_list_free(list_of_error_msgs);
    list_of_error_msgs = NULL;
  }
}
void rofi_add_warning_message(GString *str) {
  list_of_warning_msgs = g_list_append(list_of_warning_msgs, str);
}
void rofi_clear_warning_messages(void) {
  if (list_of_warning_msgs) {
    for (GList *iter = g_list_first(list_of_warning_msgs); iter != NULL;
         iter = g_list_next(iter)) {
      g_string_free((GString *)iter->data, TRUE);
    }
    g_list_free(list_of_warning_msgs);
    list_of_warning_msgs = NULL;
  }
}

/** Path to the configuration file */
G_MODULE_EXPORT char *config_path = NULL;
/** Path to the configuration file in the new format */
/** Array holding all activated modes. */
Mode **modes = NULL;

/**  List of (possibly uninitialized) modes */
Mode **available_modes = NULL;
/** Length of #num_available_modes */
unsigned int num_available_modes = 0;
/** Number of activated modes in #modes array */
unsigned int num_modes = 0;
/** Current selected mode */
unsigned int curr_mode = 0;

/** Handle to NkBindings object for input devices. */
NkBindings *bindings = NULL;

/** Glib main loop. */
GMainLoop *main_loop = NULL;

/** Flag indicating we are in dmenu mode. */
int rofi_is_in_dmenu_mode = FALSE;
/** Rofi's return code */
int return_code = EXIT_SUCCESS;

void process_result(RofiViewState *state);

void rofi_set_return_code(int code) { return_code = code; }

unsigned int rofi_get_num_enabled_modes(void) { return num_modes; }

const Mode *rofi_get_mode(unsigned int index) { return modes[index]; }

/**
 * @param name Name of the mode to lookup.
 *
 * Find the index of the mode with name.
 *
 * @returns index of the mode in modes, -1 if not found.
 */
static int mode_lookup(const char *name) {
  for (unsigned int i = 0; i < num_modes; i++) {
    if (strcmp(mode_get_name(modes[i]), name) == 0) {
      return i;
    }
  }
  return -1;
}
/**
 * @param name Name of the mode to lookup.
 *
 * Find the index of the mode with name.
 *
 * @returns index of the mode in modes, -1 if not found.
 */
static const Mode *mode_available_lookup(const char *name) {
  for (unsigned int i = 0; i < num_available_modes; i++) {
    if (strcmp(mode_get_name(available_modes[i]), name) == 0) {
      return available_modes[i];
    }
  }
  return NULL;
}

/**
 * Teardown the gui.
 */
static void teardown(int pfd) {
  g_debug("Teardown");
  // Cleanup font setup.
  textbox_cleanup();

  display_early_cleanup();

  // Cleanup view
  rofi_view_cleanup();

  // Cleanup pid file.
  remove_pid_file(pfd);
}
static void run_mode_index(ModeMode mode) {
  // Otherwise check if requested mode is enabled.
  for (unsigned int i = 0; i < num_modes; i++) {
    if (!mode_init(modes[i])) {
      GString *str = g_string_new("Failed to initialize the mode: ");
      g_string_append(str, modes[i]->name);
      g_string_append(str, "\n");

      rofi_view_error_dialog(str->str, ERROR_MSG_MARKUP);
      g_string_free(str, FALSE);
      break;
    }
  }
  // Error dialog must have been created.
  if (rofi_view_get_active() != NULL) {
    return;
  }
  curr_mode = mode;
  RofiViewState *state =
      rofi_view_create(modes[mode], config.filter, 0, process_result);

  // User can pre-select a row.
  if (find_arg("-selected-row") >= 0) {
    unsigned int sr = 0;
    find_arg_uint("-selected-row", &(sr));
    rofi_view_set_selected_line(state, sr);
  }
  if (state) {
    rofi_view_set_active(state);
  }
  if (rofi_view_get_active() == NULL) {
    g_main_loop_quit(main_loop);
  }
}
void process_result(RofiViewState *state) {
  Mode *sw = state->sw;
  //   rofi_view_set_active ( NULL );
  if (sw != NULL) {
    unsigned int selected_line = rofi_view_get_selected_line(state);
    ;
    MenuReturn mretv = rofi_view_get_return_value(state);
    char *input = g_strdup(rofi_view_get_user_input(state));
    ModeMode retv = mode_result(sw, mretv, &input, selected_line);
    {
      if (state->text) {
        if (input == NULL) {
          textbox_text(state->text, "");
        } else if (strcmp(rofi_view_get_user_input(state), input) != 0) {
          textbox_text(state->text, input);
          textbox_cursor_end(state->text);
        }
      }
    }
    g_free(input);

    ModeMode mode = curr_mode;
    // Find next enabled
    if (retv == NEXT_DIALOG) {
      mode = (mode + 1) % num_modes;
    } else if (retv == PREVIOUS_DIALOG) {
      if (mode == 0) {
        mode = num_modes - 1;
      } else {
        mode = (mode - 1) % num_modes;
      }
    } else if (retv == RELOAD_DIALOG) {
      // do nothing.
    } else if (retv == RESET_DIALOG) {
      rofi_view_clear_input(state);
    } else if (retv < MODE_EXIT) {
      mode = (retv) % num_modes;
    } else {
      mode = retv;
    }
    if (mode != MODE_EXIT) {
      /**
       * Load in the new mode.
       */
      rofi_view_switch_mode(state, modes[mode]);
      curr_mode = mode;
      return;
    }
    // On exit, free current view, and pop to one above.
    rofi_view_remove_active(state);
    rofi_view_free(state);
    return;
  }
  //    rofi_view_set_active ( NULL );
  rofi_view_remove_active(state);
  rofi_view_free(state);
}

/**
 * Help function.
 */
static void print_list_of_modes(int is_term) {
  for (unsigned int i = 0; i < num_available_modes; i++) {
    gboolean active = FALSE;
    for (unsigned int j = 0; j < num_modes; j++) {
      if (modes[j] == available_modes[i]) {
        active = TRUE;
        break;
      }
    }
    printf("        • %s%s%s%s\n", active ? "+" : "",
           is_term ? (active ? color_green : color_red) : "",
           available_modes[i]->name, is_term ? color_reset : "");
  }
}
static void print_main_application_options(int is_term) {
  print_help_msg("-config", "[file]", "Load an alternative configuration.",
                 NULL, is_term);
  print_help_msg("-no-config", "",
                 "Do not load configuration, use default values.", NULL,
                 is_term);
  print_help_msg("-v,-version", "", "Print the version number and exit.", NULL,
                 is_term);
  print_help_msg("-dmenu", "", "Start in dmenu mode.", NULL, is_term);
  print_help_msg("-display", "[string]", "X server to contact.", "${DISPLAY}",
                 is_term);
  print_help_msg("-h,-help", "", "This help message.", NULL, is_term);
  print_help_msg("-e", "[string]",
                 "Show a dialog displaying the passed message and exit.", NULL,
                 is_term);
  print_help_msg("-markup", "", "Enable pango markup where possible.", NULL,
                 is_term);
  print_help_msg("-normal-window", "",
                 "Behave as a normal window. (experimental)", NULL, is_term);
  print_help_msg("-transient-window", "",
                 "Behave as a modal dialog that is transient to the currently "
                 "focused window. (experimental)",
                 NULL, is_term);
  print_help_msg("-show", "[mode]",
                 "Show the mode 'mode' and exit. The mode has to be enabled.",
                 NULL, is_term);
  print_help_msg("-no-lazy-grab", "",
                 "Disable lazy grab that, when fail to grab keyboard, does not "
                 "block but retry later.",
                 NULL, is_term);
  print_help_msg("-no-plugins", "", "Disable loading of external plugins.",
                 NULL, is_term);
  print_help_msg("-plugin-path", "",
                 "Directory used to search for rofi plugins. *DEPRECATED*",
                 NULL, is_term);
  print_help_msg("-dump-config", "",
                 "Dump the current configuration in rasi format and exit.",
                 NULL, is_term);
  print_help_msg("-dump-theme", "",
                 "Dump the current theme in rasi format and exit.", NULL,
                 is_term);
  print_help_msg("-list-keybindings", "",
                 "Print a list of current keybindings and exit.", NULL,
                 is_term);
}
static void help(G_GNUC_UNUSED int argc, char **argv) {
  int is_term = isatty(fileno(stdout));
  printf("%s usage:\n", argv[0]);
  printf("\t%s [-options ...]\n\n", argv[0]);
  printf("Command line only options:\n");
  print_main_application_options(is_term);
  printf("DMENU command line options:\n");
  print_dmenu_options();
  printf("Global options:\n");
  print_options();
  printf("\n");
  printf("Detected Window manager:\n");
  char *wm = x11_helper_get_window_manager();
  if (wm) {
    printf("\t• %s\n", wm);
    g_free(wm);
  } else {
    printf("\t• No window manager detected.\n");
  }
  printf("\n");
  display_dump_monitor_layout();
  printf("\n");
  printf("Detected modes:\n");
  print_list_of_modes(is_term);
  printf("\n");
  printf("Detected user scripts:\n");
  script_user_list(is_term);
  printf("\n");
  printf("Compile time options:\n");
  printf("\t• Pango   version %s\n", pango_version_string());
#ifdef WINDOW_MODE
  printf("\t• window  %senabled%s\n", is_term ? color_green : "",
         is_term ? color_reset : "");
#else
  printf("\t• window  %sdisabled%s\n", is_term ? color_red : "",
         is_term ? color_reset : "");
#endif
#ifdef ENABLE_DRUN
  printf("\t• drun    %senabled%s\n", is_term ? color_green : "",
         is_term ? color_reset : "");
#else
  printf("\t• drun    %sdisabled%s\n", is_term ? color_red : "",
         is_term ? color_reset : "");
#endif
#ifdef ENABLE_GCOV
  printf("\t• gcov    %senabled%s\n", is_term ? color_green : "",
         is_term ? color_reset : "");
#else
  printf("\t• gcov    %sdisabled%s\n", is_term ? color_red : "",
         is_term ? color_reset : "");
#endif
#ifdef ENABLE_ASAN
  printf("\t• asan    %senabled%s\n", is_term ? color_green : "",
         is_term ? color_reset : "");
#else
  printf("\t• asan    %sdisabled%s\n", is_term ? color_red : "",
         is_term ? color_reset : "");
#endif
#ifdef XCB_IMDKIT
  printf("\t• imdkit  %senabled%s\n", is_term ? color_green : "",
         is_term ? color_reset : "");
#else
  printf("\t• imdkit  %sdisabled%s\n", is_term ? color_red : "",
         is_term ? color_reset : "");
#endif
  printf("\n");
  printf("For more information see: %sman rofi%s\n", is_term ? color_bold : "",
         is_term ? color_reset : "");
#ifdef GIT_VERSION
  printf("                 Version: %s" GIT_VERSION "%s\n",
         is_term ? color_bold : "", is_term ? color_reset : "");
#else
  printf("                 Version: %s" VERSION "%s\n",
         is_term ? color_bold : "", is_term ? color_reset : "");
#endif
  printf("              Bugreports: %s" PACKAGE_BUGREPORT "%s\n",
         is_term ? color_bold : "", is_term ? color_reset : "");
  printf("                 Support: %s" PACKAGE_URL "%s\n",
         is_term ? color_bold : "", is_term ? color_reset : "");
  printf("                          %s#rofi @ libera.chat%s\n",
         is_term ? color_bold : "", is_term ? color_reset : "");
  if (find_arg("-no-config") < 0) {
    if (config_path) {
      printf("      Configuration file: %s%s%s\n", is_term ? color_bold : "",
             config_path, is_term ? color_reset : "");
    }
  } else {
    printf("      Configuration file: %sDisabled%s\n",
           is_term ? color_bold : "", is_term ? color_reset : "");
  }
  rofi_theme_print_parsed_files(is_term);
}

static void help_print_disabled_mode(const char *mode) {
  int is_term = isatty(fileno(stdout));
  // Only  output to terminal
  if (is_term) {
    fprintf(stderr, "Mode %s%s%s is not enabled. I have enabled it for now.\n",
            color_red, mode, color_reset);
    fprintf(stderr,
            "Please consider adding %s%s%s to the list of enabled modes: "
            "%smodes: [%s%s%s,%s]%s.\n",
            color_red, mode, color_reset, color_green, config.modes,
            color_reset, color_red, mode, color_reset);
  }
}
static void help_print_mode_not_found(const char *mode) {
  GString *str = g_string_new("");
  g_string_printf(
      str, "Mode %s is not found.\nThe following modes are known:\n", mode);
  for (unsigned int i = 0; i < num_available_modes; i++) {
    gboolean active = FALSE;
    for (unsigned int j = 0; j < num_modes; j++) {
      if (modes[j] == available_modes[i]) {
        active = TRUE;
        break;
      }
    }
    g_string_append_printf(str, "        * %s%s\n", active ? "+" : "",
                           available_modes[i]->name);
  }
  rofi_add_error_message(str);
}
static void help_print_no_arguments(void) {

  GString *emesg = g_string_new(
      "<span size=\"x-large\">Rofi is unsure what to show.</span>\n\n");
  g_string_append(emesg, "Please specify the mode you want to show.\n\n");
  g_string_append(
      emesg, "    <b>rofi</b> -show <span color=\"green\">{mode}</span>\n\n");
  g_string_append(emesg, "The following modes are enabled:\n");
  for (unsigned int j = 0; j < num_modes; j++) {
    g_string_append_printf(emesg, "    • <span color=\"green\">%s</span>\n",
                           modes[j]->name);
  }
  g_string_append(emesg, "\nThe following modes can be enabled:\n");
  for (unsigned int i = 0; i < num_available_modes; i++) {
    gboolean active = FALSE;
    for (unsigned int j = 0; j < num_modes; j++) {
      if (modes[j] == available_modes[i]) {
        active = TRUE;
        break;
      }
    }
    if (!active) {
      g_string_append_printf(emesg, "    • <span color=\"red\">%s</span>\n",
                             available_modes[i]->name);
    }
  }
  g_string_append(emesg, "\nTo activate a mode, add it to the list in "
                         "the <span color=\"green\">modes</span> "
                         "setting.\n");
  rofi_view_error_dialog(emesg->str, ERROR_MSG_MARKUP);
  rofi_set_return_code(EXIT_SUCCESS);
}

/**
 * Cleanup globally allocated memory.
 */
static void cleanup(void) {
  for (unsigned int i = 0; i < num_modes; i++) {
    mode_destroy(modes[i]);
  }
  rofi_view_workers_finalize();
  if (main_loop != NULL) {
    g_main_loop_unref(main_loop);
    main_loop = NULL;
  }
  // Cleanup
  display_cleanup();

  nk_bindings_free(bindings);

  // Cleaning up memory allocated by the Xresources file.
  config_xresource_free();
  g_free(modes);

  g_free(config_path);

  rofi_clear_error_messages();
  rofi_clear_warning_messages();

  if (rofi_theme) {
    rofi_theme_free(rofi_theme);
    rofi_theme = NULL;
  }
  TIMINGS_STOP();
  script_mode_cleanup();
  rofi_collectmodes_destroy();
  rofi_icon_fetcher_destroy();

  rofi_theme_free_parsed_files();
  if (rofi_configuration) {
    rofi_theme_free(rofi_configuration);
    rofi_configuration = NULL;
  }
  // Cleanup memory allocated by rofi_expand_path
  if (cache_dir_alloc) {
    g_free(cache_dir_alloc);
    cache_dir_alloc = NULL;
  }
}

/**
 * Collected modes
 */

Mode *rofi_collect_modes_search(const char *name) {
  for (unsigned int i = 0; i < num_available_modes; i++) {
    if (g_strcmp0(name, available_modes[i]->name) == 0) {
      return available_modes[i];
    }
  }
  return NULL;
}
/**
 * @param mode Add mode to list.
 *
 * @returns TRUE when success.
 */
static gboolean rofi_collectmodes_add(Mode *mode) {
  Mode *m = rofi_collect_modes_search(mode->name);
  if (m == NULL) {
    available_modes =
        g_realloc(available_modes, sizeof(Mode *) * (num_available_modes + 1));
    // Set mode.
    available_modes[num_available_modes] = mode;
    num_available_modes++;
    return TRUE;
  }
  return FALSE;
}

static void rofi_collectmodes_dir(const char *base_dir) {
  g_debug("Looking into: %s for plugins", base_dir);
  GDir *dir = g_dir_open(base_dir, 0, NULL);
  if (dir) {
    const char *dn = NULL;
    while ((dn = g_dir_read_name(dir))) {
      if (!g_str_has_suffix(dn, G_MODULE_SUFFIX)) {
        continue;
      }
      char *fn = g_build_filename(base_dir, dn, NULL);
      g_debug("Trying to open: %s plugin", fn);
      GModule *mod =
          g_module_open(fn, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);
      if (mod) {
        Mode *m = NULL;
        if (g_module_symbol(mod, "mode", (gpointer *)&m)) {
          if (m->abi_version != ABI_VERSION) {
            g_warning("ABI version of plugin: '%s' does not match: %08X "
                      "expecting: %08X",
                      dn, m->abi_version, ABI_VERSION);
            g_module_close(mod);
          } else {
            m->module = mod;
            if (!rofi_collectmodes_add(m)) {
              g_module_close(mod);
            }
          }
        } else {
          g_warning("Symbol 'mode' not found in module: %s", dn);
          g_module_close(mod);
        }
      } else {
        g_warning("Failed to open 'mode' plugin: '%s', error: %s", dn,
                  g_module_error());
      }
      g_free(fn);
    }
    g_dir_close(dir);
  }
}

/**
 * Find all available modes.
 */
static void rofi_collect_modes(void) {
#ifdef WINDOW_MODE
  rofi_collectmodes_add(&window_mode);
  rofi_collectmodes_add(&window_mode_cd);
#endif
  rofi_collectmodes_add(&run_mode);
  rofi_collectmodes_add(&ssh_mode);
#ifdef ENABLE_DRUN
  rofi_collectmodes_add(&drun_mode);
#endif
  rofi_collectmodes_add(&combi_mode);
  rofi_collectmodes_add(&help_keys_mode);
  rofi_collectmodes_add(&file_browser_mode);
  rofi_collectmodes_add(&recursive_browser_mode);

  if (find_arg("-no-plugins") < 0) {
    find_arg_str("-plugin-path", &(config.plugin_path));
    g_debug("Parse plugin path: %s", config.plugin_path);
    rofi_collectmodes_dir(config.plugin_path);
    /* ROFI_PLUGIN_PATH */
    const char *path = g_getenv("ROFI_PLUGIN_PATH");
    if (path != NULL) {
      gchar **paths = g_strsplit(path, ":", -1);
      for (unsigned int i = 0; paths[i]; i++) {
        rofi_collectmodes_dir(paths[i]);
      }
      g_strfreev(paths);
    }
  }
  script_mode_gather_user_scripts();
}

/**
 * Setup configuration for config.
 */
static void rofi_collectmodes_setup(void) {
  for (unsigned int i = 0; i < num_available_modes; i++) {
    mode_set_config(available_modes[i]);
  }
}
static void rofi_collectmodes_destroy(void) {
  for (unsigned int i = 0; i < num_available_modes; i++) {
    if (available_modes[i]->module) {
      GModule *mod = available_modes[i]->module;
      available_modes[i] = NULL;
      g_module_close(mod);
    }
    if (available_modes[i]) {
      mode_free(&(available_modes[i]));
    }
  }
  g_free(available_modes);
  available_modes = NULL;
  num_available_modes = 0;
}

/**
 * Parse the mode string, into internal array of type Mode.
 *
 * String is split on separator ','
 * First the three build-in modes are checked: window, run, ssh
 * if that fails, a script-mode is created.
 */
static int add_mode(const char *token) {
  unsigned int index = num_modes;
  // Resize and add entry.
  modes = (Mode **)g_realloc(modes, sizeof(Mode *) * (num_modes + 1));

  Mode *mode = rofi_collect_modes_search(token);
  if (mode) {
    modes[num_modes] = mode;
    num_modes++;
  } else if (script_mode_is_valid(token)) {
    // If not build in, use custom mode.
    Mode *sw = script_mode_parse_setup(token);
    if (sw != NULL) {
      // Add to available list, so combi can find it.
      rofi_collectmodes_add(sw);
      mode_set_config(sw);
      modes[num_modes] = sw;
      num_modes++;
    }
  }
  return (index == num_modes) ? -1 : (int)index;
}
static gboolean setup_modes(void) {
  const char *const sep = ",#";
  char *savept = NULL;
  // Make a copy, as strtok will modify it.
  char *mode_str = g_strdup(config.modes);
  // Split token on ','. This modifies mode_str.
  for (char *token = strtok_r(mode_str, sep, &savept); token != NULL;
       token = strtok_r(NULL, sep, &savept)) {
    if (add_mode(token) == -1) {
      help_print_mode_not_found(token);
    }
  }
  // Free string that was modified by strtok_r
  g_free(mode_str);
  return FALSE;
}

/**
 * Quit rofi mainloop.
 * This will exit program.
 **/
void rofi_quit_main_loop(void) { g_main_loop_quit(main_loop); }

static gboolean main_loop_signal_handler_int(G_GNUC_UNUSED gpointer data) {
  // Break out of loop.
  g_main_loop_quit(main_loop);
  return G_SOURCE_CONTINUE;
}
static void show_error_dialog(void) {
  GString *emesg =
      g_string_new("The following errors were detected when starting rofi:\n");
  GList *iter = g_list_first(list_of_error_msgs);
  int index = 0;
  for (; iter != NULL && index < 2; iter = g_list_next(iter)) {
    GString *msg = (GString *)(iter->data);
    g_string_append(emesg, "\n\n");
    g_string_append(emesg, msg->str);
    index++;
  }
  if (g_list_length(iter) > 1) {
    g_string_append_printf(emesg, "\nThere are <b>%u</b> more errors.",
                           g_list_length(iter) - 1);
  }
  rofi_view_error_dialog(emesg->str, ERROR_MSG_MARKUP);
  g_string_free(emesg, TRUE);
  rofi_set_return_code(EX_DATAERR);
}

static gboolean startup(G_GNUC_UNUSED gpointer data) {
  TICK_N("Startup");
  // flags to run immediately and exit
  char *sname = NULL;
  char *msg = NULL;
  MenuFlags window_flags = MENU_NORMAL;

  if (find_arg("-normal-window") >= 0) {
    window_flags |= MENU_NORMAL_WINDOW;
  }
  if (find_arg("-transient-window") >= 0) {
    window_flags |= MENU_TRANSIENT_WINDOW;
  }
  TICK_N("Grab keyboard");
  __create_window(window_flags);
  TICK_N("Create Window");
  // Parse the keybindings.
  TICK_N("Parse ABE");
  // Sanity check
  config_sanity_check();
  TICK_N("Config sanity check");

  if (list_of_error_msgs != NULL) {
    show_error_dialog();
    return G_SOURCE_REMOVE;
  }
  if (list_of_warning_msgs != NULL) {
    for (GList *iter = g_list_first(list_of_warning_msgs); iter != NULL;
         iter = g_list_next(iter)) {
      fputs(((GString *)iter->data)->str, stderr);
      fputs("\n", stderr);
    }
  }
  // Dmenu mode.
  if (rofi_is_in_dmenu_mode == TRUE) {
    // force off sidebar mode:
    config.sidebar_mode = FALSE;
    int retv = dmenu_mode_dialog();
    if (retv) {
      rofi_set_return_code(EXIT_SUCCESS);
      // Directly exit.
      g_main_loop_quit(main_loop);
    }
  } else if (find_arg_str("-e", &(msg))) {
    int markup = FALSE;
    if (find_arg("-markup") >= 0) {
      markup = TRUE;
    }
    // When we pass -, we read from stdin.
    if (g_strcmp0(msg, "-") == 0) {
      size_t index = 0, i = 0;
      size_t length = 1024;
      msg = malloc(length * sizeof(char));
      while ((i = fread(&msg[index], 1, 1024, stdin)) > 0) {
        index += i;
        length += i;
        msg = realloc(msg, length * sizeof(char));
      }

      msg[index] = 0;

      if (!rofi_view_error_dialog(msg, markup)) {
        g_main_loop_quit(main_loop);
      }
      g_free(msg);
    } else {
      // Normal version
      if (!rofi_view_error_dialog(msg, markup)) {
        g_main_loop_quit(main_loop);
      }
    }
  } else if (find_arg_str("-show", &sname) == TRUE) {
    int index = mode_lookup(sname);
    if (index < 0) {
      // Add it to the list
      index = add_mode(sname);
      // Complain
      if (index >= 0) {
        help_print_disabled_mode(sname);
      }
      // Run it anyway if found.
    }
    if (index >= 0) {
      run_mode_index(index);
    } else {
      help_print_mode_not_found(sname);
      show_error_dialog();
      return G_SOURCE_REMOVE;
    }
  } else if (find_arg("-show") >= 0 && num_modes > 0) {
    run_mode_index(0);
  } else {
    help_print_no_arguments();

    // g_main_loop_quit(main_loop);
  }

  return G_SOURCE_REMOVE;
}

static gboolean take_screenshot_quit(G_GNUC_UNUSED void *data) {
  rofi_capture_screenshot();
  rofi_quit_main_loop();
  return G_SOURCE_REMOVE;
}
static gboolean record(G_GNUC_UNUSED void *data) {
  rofi_capture_screenshot();
  return G_SOURCE_CONTINUE;
}
static void rofi_custom_log_function(const char *log_domain,
                                     G_GNUC_UNUSED GLogLevelFlags log_level,
                                     const gchar *message, gpointer user_data) {
  int fp = GPOINTER_TO_INT(user_data);
  dprintf(fp, "[%s]: %s\n", log_domain == NULL ? "default" : log_domain,
          message);
}
/**
 * @param argc number of input arguments.
 * @param argv array of the input arguments.
 *
 * Main application entry point.
 *
 * @returns return code of rofi.
 */
int main(int argc, char *argv[]) {
  cmd_set_arguments(argc, argv);
  if (find_arg("-log") >= 0) {
    char *logfile = NULL;
    find_arg_str("-log", &logfile);
    if (logfile != NULL) {
      int fp = open(logfile, O_CLOEXEC | O_APPEND | O_CREAT | O_WRONLY,
                    S_IRUSR | S_IWUSR);
      if (fp != -1) {
        g_log_set_default_handler(rofi_custom_log_function,
                                  GINT_TO_POINTER(fp));

      } else {
        g_error("Failed to open logfile '%s': %s.", logfile, strerror(errno));
      }

    } else {
      g_warning("Option '-log' should pass in a filename.");
    }
  }
  TIMINGS_START();

  // Version
  if (find_arg("-v") >= 0 || find_arg("-version") >= 0) {
#ifdef GIT_VERSION
    g_print("Version: " GIT_VERSION "\n");
#else
    g_print("Version: " VERSION "\n");
#endif
    return EXIT_SUCCESS;
  }

  if (find_arg("-rasi-validate") >= 0) {
    char *str = NULL;
    find_arg_str("-rasi-validate", &str);
    if (str != NULL) {
      int retv = rofi_theme_rasi_validate(str);
      cleanup();
      return retv;
    }
    fprintf(stderr, "Usage: %s -rasi-validate my-theme.rasi", argv[0]);
    return EXIT_FAILURE;
  }

  {
    const char *ro_pid = g_getenv("ROFI_OUTSIDE");
    if (ro_pid != NULL) {
      pid_t ro_pidi = (pid_t)g_ascii_strtoll(ro_pid, NULL, 0);
      if (kill(ro_pidi, 0) == 0) {
        printf("Do not launch rofi from inside rofi.\r\n");
        return EXIT_FAILURE;
      }
    }
  }

  // Detect if we are in dmenu mode.
  // This has two possible causes.
  // 1 the user specifies it on the command-line.
  if (find_arg("-dmenu") >= 0) {
    rofi_is_in_dmenu_mode = TRUE;
  }
  // 2 the binary that executed is called dmenu (e.g. symlink to rofi)
  else {
    // Get the base name of the executable called.
    char *base_name = g_path_get_basename(argv[0]);
    const char *const dmenu_str = "dmenu";
    rofi_is_in_dmenu_mode = (strcmp(base_name, dmenu_str) == 0);
    // Free the basename for dmenu detection.
    g_free(base_name);
  }
  TICK();

  // Create pid file path.
  const char *path = g_get_user_runtime_dir();
  if (path) {
    if (g_mkdir_with_parents(path, 0700) < 0) {
      g_warning("Failed to create user runtime directory: %s with error: %s",
                path, g_strerror(errno));
      pidfile = g_build_filename(g_get_home_dir(), ".rofi.pid", NULL);
    } else {
      pidfile = g_build_filename(path, "rofi.pid", NULL);
    }
  }
  config_parser_add_option(xrm_String, "pid", (void **)&pidfile,
                           "Pidfile location");

  /** default configuration */
  if (find_arg("-no-default-config") < 0) {
    GBytes *theme_data = g_resource_lookup_data(
        resources_get_resource(), "/org/qtools/rofi/default_configuration.rasi",
        G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
    if (theme_data) {
      const char *theme = g_bytes_get_data(theme_data, NULL);
      if (rofi_theme_parse_string((const char *)theme)) {
        g_warning("Failed to parse default configuration. Giving up..");
        if (list_of_error_msgs) {
          for (GList *iter = g_list_first(list_of_error_msgs); iter != NULL;
               iter = g_list_next(iter)) {
            g_warning("Error: %s%s%s", color_bold, ((GString *)iter->data)->str,
                      color_reset);
          }
        }
        rofi_configuration = NULL;
        cleanup();
        return EXIT_FAILURE;
      }
      g_bytes_unref(theme_data);
    }
  }

  if (find_arg("-config") < 0) {
    const char *cpath = g_get_user_config_dir();
    if (cpath) {
      config_path = g_build_filename(cpath, "rofi", "config.rasi", NULL);
    }
  } else {
    char *c = NULL;
    find_arg_str("-config", &c);
    config_path = rofi_expand_path(c);
  }

  TICK();
  if (setlocale(LC_ALL, "") == NULL) {
    g_warning("Failed to set locale.");
    cleanup();
    return EXIT_FAILURE;
  }

  TICK_N("Setup Locale");
  rofi_collect_modes();
  TICK_N("Collect MODES");
  rofi_collectmodes_setup();
  TICK_N("Setup MODES");

  main_loop = g_main_loop_new(NULL, FALSE);

  TICK_N("Setup mainloop");

  bindings = nk_bindings_new(0lu);
  TICK_N("NK Bindings");

  if (!display_setup(main_loop, bindings)) {
    g_warning("Connection has error");
    cleanup();
    return EXIT_FAILURE;
  }
  TICK_N("Setup Display");

  // Setup keybinding
  setup_abe();
  TICK_N("Setup abe");

  if (find_arg("-no-config") < 0) {
    // Load distro default settings
    gboolean found_system = FALSE;
    const char *const *dirs = g_get_system_config_dirs();
    if (dirs) {
      for (unsigned int i = 0; !found_system && dirs[i]; i++) {
        /** New format. */
        gchar *etc = g_build_filename(dirs[i], "rofi.rasi", NULL);
        g_debug("Look for default config file: %s", etc);
        if (g_file_test(etc, G_FILE_TEST_IS_REGULAR)) {
          g_debug("Parsing: %s", etc);
          rofi_theme_parse_file(etc);
          found_system = TRUE;
        }
        g_free(etc);
      }
    }
    if (!found_system) {
      /** New format. */
      gchar *etc = g_build_filename(SYSCONFDIR, "rofi.rasi", NULL);
      g_debug("Look for default config file: %s", etc);
      if (g_file_test(etc, G_FILE_TEST_IS_REGULAR)) {
        g_debug("Look for default config file: %s", etc);
        rofi_theme_parse_file(etc);
      }
      g_free(etc);
    }

    if (config_path) {
      // Try to resolve the path.
      extern const char *rasi_theme_file_extensions[];
      char *file2 =
          helper_get_theme_path(config_path, rasi_theme_file_extensions, NULL);
      char *filename = rofi_theme_parse_prepare_file(file2);
      g_free(file2);
      if (filename && g_file_test(filename, G_FILE_TEST_EXISTS)) {
        if (rofi_theme_parse_file(filename)) {
          rofi_theme_free(rofi_theme);
          rofi_theme = NULL;
        }
      }
      g_free(filename);
    }
  }
  find_arg_str("-theme", &(config.theme));
  if (config.theme) {
    TICK_N("Parse theme");
    rofi_theme_reset();
    if (rofi_theme_parse_file(config.theme)) {
      g_warning("Failed to parse theme: \"%s\"", config.theme);
      // TODO: instantiate fallback theme.?
      rofi_theme_free(rofi_theme);
      rofi_theme = NULL;
    }
    TICK_N("Parsed theme");
  }
  // Parse command line for settings, independent of other -no-config.
  if (list_of_error_msgs == NULL) {
    // Only call this when there are no errors.
    // This might clear existing errors.
    config_parse_cmd_options();
  }

  if (rofi_theme == NULL || rofi_theme->num_widgets == 0) {
    g_debug("Failed to load theme. Try to load default: ");
    rofi_theme_parse_string("@theme \"default\"");
  }
  TICK_N("Load cmd config ");

  // Get the path to the cache dir.
  cache_dir = g_get_user_cache_dir();

  if (config.cache_dir != NULL) {
    cache_dir = cache_dir_alloc = rofi_expand_path(config.cache_dir);
  }

  if (g_mkdir_with_parents(cache_dir, 0700) < 0) {
    g_warning("Failed to create cache directory: %s", g_strerror(errno));
    return EXIT_FAILURE;
  }

  /** dirty hack for dmenu compatibility */
  char *windowid = NULL;
  if (!rofi_is_in_dmenu_mode) {
    // setup_modes
    if (setup_modes()) {
      cleanup();
      return EXIT_FAILURE;
    }
    TICK_N("Setup Modes");
  } else {
    // Hack for dmenu compatibility.
    if (find_arg_str("-w", &windowid) == TRUE) {
      config.monitor = g_strdup_printf("wid:%s", windowid);
      windowid = config.monitor;
    }
  }

  /**
   * Make small commandline changes to the current theme.
   */
  const char **theme_str = find_arg_strv("-theme-str");
  if (theme_str) {
    for (int index = 0; theme_str[index]; index++) {
      if (rofi_theme_parse_string(theme_str[index])) {
        g_warning("Failed to parse -theme-str option: \"%s\"",
                  theme_str[index]);
        rofi_theme_free(rofi_theme);
        rofi_theme = NULL;
      }
    }
    g_free(theme_str);
  }

  parse_keys_abe(bindings);
  if (find_arg("-dump-theme") >= 0) {
    rofi_theme_print(rofi_theme);
    cleanup();
    return EXIT_SUCCESS;
  }
  if (find_arg("-dump-processed-theme") >= 0) {
    rofi_theme_parse_process_conditionals();
    rofi_theme_print(rofi_theme);
    cleanup();
    return EXIT_SUCCESS;
  }
  if (find_arg("-dump-config") >= 0) {
    config_parse_dump_config_rasi_format(stdout, FALSE);
    cleanup();
    return EXIT_SUCCESS;
  }
  // Dump.
  // catch help request
  if (find_arg("-h") >= 0 || find_arg("-help") >= 0 ||
      find_arg("--help") >= 0) {
    help(argc, argv);
    cleanup();
    return EXIT_SUCCESS;
  }
  if (find_arg("-list-keybindings") >= 0) {
    int is_term = isatty(fileno(stdout));
    abe_list_all_bindings(is_term);
    return EXIT_SUCCESS;
  }

  unsigned int interval = 1;
  if (find_arg_uint("-record-screenshots", &interval)) {
    g_timeout_add((guint)(1000 / (double)interval), record, NULL);
  }
  if (find_arg_uint("-take-screenshot-quit", &interval)) {
    g_timeout_add(interval, take_screenshot_quit, NULL);
  }
  if (find_arg("-benchmark-ui") >= 0) {
    config.benchmark_ui = TRUE;
  }

  rofi_view_workers_initialize();
  TICK_N("Workers initialize");
  rofi_icon_fetcher_init();
  TICK_N("Icon fetcher initialize");

  gboolean kill_running = FALSE;
  if (find_arg("-replace") >= 0) {
    kill_running = TRUE;
  }
  // Create pid file
  int pfd = create_pid_file(pidfile, kill_running);
  TICK_N("Pid file created");
  if (pfd < 0) {
    cleanup();
    return EXIT_FAILURE;
  }
  textbox_setup();
  TICK_N("Text box setup");

  if (!display_late_setup()) {
    g_warning("Failed to properly finish display setup");
    cleanup();
    return EXIT_FAILURE;
  }
  TICK_N("Setup late Display");

  rofi_theme_parse_process_conditionals();
  rofi_theme_parse_process_links();
  TICK_N("Theme setup");

  // Setup signal handling sources.
  // SIGINT
  g_unix_signal_add(SIGINT, main_loop_signal_handler_int, NULL);

  g_idle_add(startup, NULL);

  // Start mainloop.
  g_main_loop_run(main_loop);
  teardown(pfd);
  cleanup();

  /* dirty hack */
  g_free(windowid);
  return return_code;
}

/** List of error messages.*/
extern GList *list_of_error_msgs;
int rofi_theme_rasi_validate(const char *filename) {
  rofi_theme_parse_file(filename);
  rofi_theme_parse_process_links();
  if (list_of_error_msgs == NULL && list_of_warning_msgs == NULL) {
    return EXIT_SUCCESS;
  }

  for (GList *iter = g_list_first(list_of_error_msgs); iter != NULL;
       iter = g_list_next(iter)) {
    fputs(((GString *)iter->data)->str, stderr);
    fputs("\n", stderr);
  }
  for (GList *iter = g_list_first(list_of_warning_msgs); iter != NULL;
       iter = g_list_next(iter)) {
    fputs(((GString *)iter->data)->str, stderr);
    fputs("\n", stderr);
  }

  return EXIT_FAILURE;
}

const Mode *rofi_get_completer(void) {
  const Mode *index = mode_available_lookup(config.completer_mode);
  if (index != NULL) {
    return index;
  }
  const char *name =
      config.completer_mode == NULL ? "(null)" : config.completer_mode;
  g_warning("Mode: %s not found or is not valid for use as completer.", name);
  return NULL;
}
