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

void feh_handle_signal(int);

void setup_signal_handlers()
{
	struct sigaction feh_sh;
	sigset_t feh_ss;
	D_ENTER(4);

	if (
		(sigemptyset(&feh_ss) == -1) ||
		(sigaddset(&feh_ss, SIGUSR1) == -1) ||
		(sigaddset(&feh_ss, SIGUSR2) == -1))
	{
		weprintf("Failed to set up signal mask, SIGUSR1/2 won't work");
		D_RETURN_(4);
	}

	feh_sh.sa_handler = feh_handle_signal;
	feh_sh.sa_mask = feh_ss;

	if (
		(sigaction(SIGUSR1, &feh_sh, NULL) == -1) ||
		(sigaction(SIGUSR2, &feh_sh, NULL) == -1))
	{
		weprintf("Failed to set up signal handler, SIGUSR1/2 won't work");
		D_RETURN_(4);
	}

	D_RETURN_(4);
}

void feh_handle_signal(int signo)
{
	D_ENTER(4);
	winwidget winwid
		= winwidget_get_first_window_of_type(WIN_TYPE_SLIDESHOW);

	if (winwid) {
		if (signo == SIGUSR1)
			slideshow_change_image(winwid, SLIDE_NEXT);
		else if (signo == SIGUSR2)
			slideshow_change_image(winwid, SLIDE_PREV);
	}

	D_RETURN_(4);
}
