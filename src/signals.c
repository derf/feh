/* signals.c

Copyright (C) 2010-2020 by Birte Kristina Friesel

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
#include "filelist.h"
#include "winwidget.h"
#include "options.h"

void feh_handle_signal(int);
volatile int sig_received = 0;
volatile int sig_exit = 0;

void setup_signal_handlers(void)
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
		(sigaddset(&feh_ss, SIGINT) == -1) ||
		(sigaddset(&feh_ss, SIGTTIN) == -1))
	{
		weprintf("Failed to set up signal masks");
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
		(sigaction(SIGINT, &feh_sh, NULL) == -1) ||
		(sigaction(SIGTTIN, &feh_sh, NULL) == -1))
	{
		weprintf("Failed to set up signal handler");
		return;
	}

	return;
}

void feh_handle_signal(int signo)
{
	switch (signo) {
		case SIGALRM:
			if (childpid)
				killpg(childpid, SIGINT);
			return;
		case SIGTTIN:
			// we were probably backgrounded while we were running
			control_via_stdin = 0;
			return;
		case SIGINT:
		case SIGTERM:
		case SIGQUIT:
			if (childpid)
				killpg(childpid, SIGINT);
			sig_exit = 128 + signo;
			return;
	}

	sig_received = signo;
}
