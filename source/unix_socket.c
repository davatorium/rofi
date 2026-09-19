//
// Created by xlp on 19/09/2026.
//

#include "../include/unix_socket.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <glib-unix.h>

#include "keyb.h"
#include "view.h"

static int server_socket_fd = -1;

static gboolean unix_socket_read_callback(GIOChannel *source, GIOCondition condition, gpointer data) {
  if (condition & G_IO_IN) {
    int client_fd = accept(g_io_channel_unix_get_fd(source), NULL, NULL);
    if (client_fd != -1) {
      char buffer[1024];
      ssize_t count = read(client_fd, buffer, sizeof(buffer) - 1);
      if (count > 0) {
        buffer[count] = '\0';

        guint action_id = key_binding_get_action_from_name(buffer);
        if (action_id != UINT32_MAX) {
          rofi_view_trigger_action(NULL, SCOPE_GLOBAL, action_id);
        }
        else {
          g_print("Received unknown command via socket: %s\n", buffer);
        }
      }
      close(client_fd);
    }
  }
  return TRUE;
}

void unix_socket_setup(void) {
  struct sockaddr_un addr;

  server_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (server_socket_fd == -1) {
    g_warning("Failed to create unix socket");
    return;
  }

  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_UNIX;

  const char *runtime_dir = g_get_user_runtime_dir();
  snprintf(addr.sun_path, sizeof(addr.sun_path), "%s/rofi-%d.sock", runtime_dir, getpid());

  if (bind(server_socket_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
    g_warning("Failed to bind unix socket to %s", addr.sun_path);
    close(server_socket_fd);
    server_socket_fd = -1;
    return;
  }

  if (listen(server_socket_fd, 5) == -1) {
    g_warning("Failed to listen on unix socket");
    close(server_socket_fd);
    server_socket_fd = -1;
  }
  else {
    GIOChannel *channel = g_io_channel_unix_new(server_socket_fd);
    g_io_add_watch(channel, G_IO_IN, unix_socket_read_callback, NULL);
  }
}

void unix_socket_cleanup(void) {
  if (server_socket_fd == -1) {
    return;
  }
  close(server_socket_fd);
  const char *runtime_dir = g_get_user_runtime_dir();
  char path[100];
  snprintf(path, sizeof(path), "%s/rofi-%d.sock", runtime_dir, getpid());
  unlink(path);
}