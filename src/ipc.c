/* ipc.c

Copyright (C) 1999-2003 Tom Gilbert.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies of the Software and its documentation and acknowledgment shall be
given in the documentation and software packages that this Software was
used.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include "feh.h"
#include "debug.h"
#include "options.h"

static int session_id = 0;
static char *socket_name;
static int socket_fd = 0;

int feh_ipc_create_socket(void) {
  struct sockaddr_un saddr;
  int i;

  if ((socket_fd = socket(AF_UNIX, SOCK_STREAM, 0)) != -1) {
    for (i = 0; ; i++) {
      saddr.sun_family = AF_UNIX;
      snprintf(saddr.sun_path, 108, "%s/feh_%s.%d", feh_get_tmp_dir(), feh_get_user_name(), i);
/*
      if (!feh_remote_is_running(i)) {
        if ((unlink(saddr.sun_path) == -1) && errno != ENOENT) {
          close(socket_fd);
          eprintf("feh_ipc_create_socket: failed to unlink %s:", saddr.sun_path);
        }
      } else {
        continue;
        }
*/
      if (bind(socket_fd, (struct sockaddr *) &saddr, sizeof(saddr)) != -1) {
        session_id = i;
        socket_name = estrdup(saddr.sun_path);
        listen(socket_fd, 50);
        break;
      } else {
        close(socket_fd);
        eprintf("feh_ipc_create_socket: failed to bind %s to a socket:", saddr.sun_path);
      }
    }
  } else {
    eprintf("feh_ipc_create_socket: failed to open socket:");
  }
}

int feh_ipc_get_session_id(void) {
  return session_id;
}

void feh_ipc_cleanup(void) {
  close(socket_fd);
  unlink(socket_name);
  free(socket_name);
}

static void feh_ipc_write_packet(int fd, void *data, int length) {
  feh_ipc_server_header header;
  header.version = FEH_IPC_VERSION;
  header.data_length = length;
  if (data && length > 0) {
    write(fd, data, length);
  }
}

static void feh_ipc_write_int(int fd, int val) {
  feh_ipc_write_packet(fd, &val, sizeof(int));
}

static void feh_ipc_write_string(int fd, char *string) {
  feh_ipc_write_packet(fd, &string, string ? strlen(string) + 1 : 0);
}


