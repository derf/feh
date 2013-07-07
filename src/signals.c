/* signals.c

Copyright (C) 2010 by Daniel Friesel

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
#include "winwidget.h"
#include "options.h"

void feh_handle_signal(int);

void setup_signal_handlers()
{
	struct sigaction feh_sh;
	sigset_t feh_ss;
	if (
		(sigemptyset(&feh_ss) == -1) ||
		(sigaddset(&feh_ss, SIGUSR1) == -1) ||
		(sigaddset(&feh_ss, SIGUSR2) == -1) ||
		(sigaddset(&feh_ss, SIGALRM) == -1) ||
		(sigaddset(&feh_ss, SIGTERM) == -1) ||
		(sigaddset(&feh_ss, SIGQUIT) == -1) ||
		(sigaddset(&feh_ss, SIGINT) == -1))
	{
		weprintf("%s%smasks",ERR_FAILED, ERR_TO_SET_UP_SIGNAL);
		return;
	}

	feh_sh.sa_handler = feh_handle_signal;
	feh_sh.sa_mask    = feh_ss;
	feh_sh.sa_flags   = 0;

	if (
		(sigaction(SIGUSR1, &feh_sh, NULL) == -1) ||
		(sigaction(SIGUSR2, &feh_sh, NULL) == -1) ||
		(sigaction(SIGALRM, &feh_sh, NULL) == -1) ||
		(sigaction(SIGTERM, &feh_sh, NULL) == -1) ||
		(sigaction(SIGQUIT, &feh_sh, NULL) == -1) ||
		(sigaction(SIGINT, &feh_sh, NULL) == -1))
	{
		weprintf("%s%shandler",ERR_FAILED, ERR_TO_SET_UP_SIGNAL);
		return;
	}

	return;
}

void feh_handle_signal(int signo)
{
	winwidget w;
	int i;

	switch (signo) {
		case SIGALRM:
			if (fgv.childpid)
				killpg(fgv.childpid, SIGINT);
			return;
		case SIGINT:
		case SIGTERM:
		case SIGQUIT:
			if (fgv.childpid)
				killpg(fgv.childpid, SIGINT);
			exit(128 + signo);
	}

	w = winwidget_get_first_window_of_type(WIN_TYPE_SLIDESHOW);

	if (w) {
		if (signo == SIGUSR1)
			slideshow_change_image(w, SLIDE_NEXT, RENDER_YES);
		else if (signo == SIGUSR2)
			slideshow_change_image(w, SLIDE_PREV, RENDER_YES);
	} else if (opt.flg.mode == MODE_MULTIWINDOW) {
		for (i = fgv.window_num - 1; i >= 0; i--)
			feh_reload_image(fgv.windows[i],RESIZE_NO, FORCE_NEW_NO );
	}

	return;
}
