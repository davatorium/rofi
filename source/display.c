#include "keyb.h"
#include <glib.h>

#include "display-internal.h"
#include "display.h"

#include "view.h"

#include "view-internal.h"

static const display_proxy *proxy;

void display_init(const display_proxy *disp_in) {
  g_assert(disp_in != NULL);
  proxy = disp_in;
  view_init(proxy->view());
}

int monitor_active(workarea *mon) {
  if (proxy) {
    return proxy->monitor_active(mon);
  }
  return 0;
}

void display_set_input_focus(guint w) {
  if (proxy) {
    proxy->set_input_focus(w);
  }
}

void display_revert_input_focus(void) {
  if (proxy) {
    proxy->revert_input_focus();
  }
}

gboolean display_setup(GMainLoop *main_loop, NkBindings *bindings) {
  if (proxy) {
    return proxy->setup(main_loop, bindings);
  }
  return FALSE;
}

gboolean display_late_setup(void) {
  if (proxy) {
    return proxy->late_setup();
  }
  return FALSE;
}

void display_early_cleanup(void) {
  if (proxy) {
    proxy->early_cleanup();
  }
}

void display_cleanup(void) {
  if (proxy) {
    proxy->cleanup();
  }
}

void display_dump_monitor_layout(void) {
  if (proxy) {
    proxy->dump_monitor_layout();
  }
}

void display_startup_notification(RofiHelperExecuteContext *context,
                                  GSpawnChildSetupFunc *child_setup,
                                  gpointer *user_data) {
  if (proxy) {
    proxy->startup_notification(context, child_setup, user_data);
  }
}

guint display_scale(void) {
  if (proxy) {
    return proxy->scale();
  }
  return 0;
}

void display_get_clipboard_data(enum clipboard_type type, ClipboardCb callback,
                                void *user_data) {
  if (proxy) {
    proxy->get_clipboard_data(type, callback, user_data);
  }
}

void display_set_fullscreen_mode(void) {
  if (proxy) {
    proxy->set_fullscreen_mode();
  }
}
