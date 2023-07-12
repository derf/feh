/* wallpaper.h

Copyright (C) 1999-2003 Tom Gilbert.
Copyright (C) 2010-2020 Birte Kristina Friesel.

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

#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <X11/Xfuncproto.h>
#include <X11/Intrinsic.h>	/* Xlib, Xutil, Xresource, Xfuncproto */

#define IPC_TIMEOUT    ((char *) 1)
#define IPC_FAKE       ((char *) 2)	/* Faked IPC */

#define enl_ipc_sync()  do {                                           \
       char *reply = enl_send_and_wait("nop");                         \
       if ((reply != IPC_FAKE) && (reply != IPC_TIMEOUT))              \
         free(reply);                                                  \
   } while (0)

extern Window ipc_win;
extern Atom ipc_atom;

extern Window enl_ipc_get_win(void);
extern void enl_ipc_send(char *);
extern char *enl_wait_for_reply(void);
extern char *enl_ipc_get(const char *);
extern char *enl_send_and_wait(char *);
extern void feh_wm_set_bg(char *fil, Imlib_Image im, int centered, int scaled,
		int fill, int desktop, int for_screen);
extern int feh_wm_get_num_desks(void);
extern signed char feh_wm_get_wm_is_e(void);
void feh_wm_set_bg_filelist(unsigned char bgmode);

#endif
