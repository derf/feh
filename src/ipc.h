/* ipc.h

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

#ifndef IPC_H
#define IPC_H

#define FEH_IPC_VERSION 0x100;

int feh_ipc_create_socket(void);
int feh_ipc_get_session_id(void);

typedef struct
{
  unsigned int version;
  unsigned int command;
  unsigned int data_length;
}
feh_ipc_client_header;

typedef struct
{
  unsigned int version;
  unsigned int data_length;
}
feh_ipc_server_header;

enum {
  IPC_CMD_QUIT,
  IPC_CMD_FILELIST_NEXT,
  IPC_CMD_FILELIST_PREV,
};

#endif


