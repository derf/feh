/* main.c

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

#include "feh.h"
#include "filelist.h"
#include "winwidget.h"
#include "timers.h"
#include "options.h"
#include "events.h"
#include "signals.h"
#include "wallpaper.h"
#include <termios.h>

#ifdef HAVE_INOTIFY
#include <sys/inotify.h>
#endif

char **cmdargv = NULL;
int cmdargc = 0;
char *mode = NULL;

int main(int argc, char **argv)
{
	atexit(feh_clean_exit);

	srandom(getpid() * time(NULL) % ((unsigned int) -1));

	setup_signal_handlers();

#ifdef HAVE_LIBMAGIC
	init_magic();
#endif

	init_parse_options(argc, argv);

	init_imlib_fonts();

	if (opt.display) {
		init_x_and_imlib();
		init_keyevents();
		init_buttonbindings();
#ifdef HAVE_INOTIFY
        if (opt.auto_reload) {
            opt.inotify_fd = inotify_init();
            if (opt.inotify_fd < 0) {
                opt.auto_reload = 0;
                weprintf("inotify_init failed:");
                weprintf("Disabling inotify-based auto-reload");
            }
        }
#endif
	}

	feh_event_init();

	if (opt.index)
		init_index_mode();
	else if (opt.multiwindow)
		init_multiwindow_mode();
	else if (opt.list || opt.customlist)
		init_list_mode();
	else if (opt.loadables)
		init_loadables_mode();
	else if (opt.unloadables)
		init_unloadables_mode();
	else if (opt.thumbs)
		init_thumbnail_mode();
	else if (opt.bgmode) {
		feh_wm_set_bg_filelist(opt.bgmode);
		exit(0);
	}
	else if (opt.display){
		/* Slideshow mode is the default. Because it's spiffy */
		opt.slideshow = 1;
		init_slideshow_mode();
	}
	else {
		eprintf("Invalid option combination");
	}

	/* main event loop */
	while (feh_main_iteration(1));

	return(sig_exit);
}

static void feh_process_signal(void)
{
	winwidget winwid = winwidget_get_first_window_of_type(WIN_TYPE_SLIDESHOW);
	int i;
	int signo = sig_received;
	sig_received = 0;

	if (winwid) {
		if (filelist_len > 1) {
			if (signo == SIGUSR1)
				slideshow_change_image(winwid, SLIDE_NEXT, 1);
			else if (signo == SIGUSR2)
				slideshow_change_image(winwid, SLIDE_PREV, 1);
		} else {
			feh_reload_image(winwid, 0, 0);
		}
	} else if (opt.multiwindow) {
		for (i = window_num - 1; i >= 0; i--)
			feh_reload_image(windows[i], 0, 0);
	}
}

/* Return 0 to stop iterating, 1 if ok to continue. */
int feh_main_iteration(int block)
{
	static int first = 1;
	static int xfd = 0;
	static int fdsize = 0;
	static double pt = 0.0;
	XEvent ev;
	struct timeval tval;
	fd_set fdset;
	int count = 0;
	double t1 = 0.0, t2 = 0.0;
	fehtimer ft;

	if (window_num == 0 || sig_exit != 0)
		return(0);

	if (sig_received) {
		feh_process_signal();
	}

	if (first) {
		/* Only need to set these up the first time */
		xfd = ConnectionNumber(disp);
		fdsize = xfd + 1;
		pt = feh_get_time();
		first = 0;
		/*
		 * Only accept commands from stdin if
		 * - stdin is a terminal (otherwise it's probably used as an image / filelist)
		 * - we aren't running in multiwindow mode (cause it's not clear which
		 *   window commands should be applied to in that case)
		 * - we're in the same process group as stdin, AKA we're not running
		 *   in the background. Background processes are stopped with SIGTTOU
		 *   if they try to write to stdout or change terminal attributes. They
		 *   also don't get input from stdin anyway.
		 */
		if (isatty(STDIN_FILENO) && !opt.multiwindow && getpgrp() == (tcgetpgrp(STDIN_FILENO))) {
			setup_stdin();
		}
	}

	/* Timers */
	t1 = feh_get_time();
	t2 = t1 - pt;
	pt = t1;
	while (XPending(disp)) {
		XNextEvent(disp, &ev);
		if (ev_handler[ev.type])
			(*(ev_handler[ev.type])) (&ev);

		if (window_num == 0 || sig_exit != 0)
			return(0);

		if (sig_received) {
			feh_process_signal();
		}
	}
	XFlush(disp);

	feh_redraw_menus();

	FD_ZERO(&fdset);
	FD_SET(xfd, &fdset);
	if (control_via_stdin) {
		FD_SET(STDIN_FILENO, &fdset);
	}
#ifdef HAVE_INOTIFY
    if (opt.auto_reload) {
        FD_SET(opt.inotify_fd, &fdset);
        if (opt.inotify_fd >= fdsize)
            fdsize = opt.inotify_fd + 1;
    }
#endif

	/* Timers */
	ft = first_timer;
	/* Don't do timers if we're zooming/panning/etc or if we are paused */
	if (ft && (opt.mode == MODE_NORMAL) && !opt.paused) {
		D(("There are timers in the queue\n"));
		if (ft->just_added) {
			D(("The first timer has just been added\n"));
			D(("ft->in = %f\n", ft->in));
			ft->just_added = 0;
			t1 = ft->in;
		} else {
			D(("The first timer was not just added\n"));
			t1 = ft->in - t2;
			if (t1 < 0.0)
				t1 = 0.0;
			ft->in = t1;
		}

		XSync(disp, False);
		D(("I next need to action a timer in %f seconds\n", t1));
		/* Only do a blocking select if there's a timer due, or no events
		   waiting */
		if (t1 == 0.0 || (block && !XPending(disp))) {
			tval.tv_sec = (long) t1;
			tval.tv_usec = (long) ((t1 - ((double) tval.tv_sec)) * 1000000);
			if (tval.tv_sec < 0)
				tval.tv_sec = 0;
			if (tval.tv_usec <= 1000)
				tval.tv_usec = 1000;
			errno = 0;
			D(("Performing blocking select - waiting for timer or event\n"));
			count = select(fdsize, &fdset, NULL, NULL, &tval);
			if ((count < 0)
					&& ((errno == ENOMEM) || (errno == EINVAL)
						|| (errno == EBADF)))
				eprintf("Connection to X display lost");
			if (count == 0) {
				/* This means the timer is due to be executed. If count was > 0,
				   that would mean an X event had woken us, we're not interested
				   in that */
				feh_handle_timer();
			}
			/*
			 * Beware: If stdin is not connected, we may end up with xfd == 0.
			 * However, STDIN_FILENO == 0 holds as well in most cases. So we must
			 * check control_via_stdin to avoid mistaking an X11 event for stdin.
			 */
			else if ((count > 0) && control_via_stdin && (FD_ISSET(STDIN_FILENO, &fdset)))
				feh_event_handle_stdin();
#ifdef HAVE_INOTIFY
			else if ((count > 0) && (FD_ISSET(opt.inotify_fd, &fdset)))
                feh_event_handle_inotify();
#endif
		}
	} else {
		/* Don't block if there are events in the queue. That's a bit rude ;-) */
		if (block && !XPending(disp)) {
			errno = 0;
			D(("Performing blocking select - no timers, or zooming\n"));
			count = select(fdsize, &fdset, NULL, NULL, NULL);
			if ((count < 0)
					&& ((errno == ENOMEM) || (errno == EINVAL)
						|| (errno == EBADF)))
				eprintf("Connection to X display lost");
			else if ((count > 0) && control_via_stdin && (FD_ISSET(STDIN_FILENO, &fdset)))
				feh_event_handle_stdin();
#ifdef HAVE_INOTIFY
			else if ((count > 0) && (FD_ISSET(opt.inotify_fd, &fdset)))
                feh_event_handle_inotify();
#endif
		}
	}
	if (window_num == 0 || sig_exit != 0)
		return(0);

	if (sig_received) {
		feh_process_signal();
	}

	return(1);
}

void feh_clean_exit(void)
{
	delete_rm_files();

	free(opt.menu_font);

#ifdef HAVE_INOTIFY
    if (opt.auto_reload)
        if (close(opt.inotify_fd))
            eprintf("inotify close failed");
#endif

	if(disp)
		XCloseDisplay(disp);

#ifdef HAVE_LIBMAGIC
	uninit_magic();
#endif

	/*
	 * Only restore the old terminal settings if
	 * - we changed them in the first place
	 * - stdin still is a terminal (it might have been closed)
	 * - stdin still belongs to us (we might have been detached from the
	 *   controlling terminal, in that case we probably shouldn't be messing
	 *   around with it) <https://github.com/derf/feh/issues/324>
	 */
	if (control_via_stdin && isatty(STDIN_FILENO) && getpgrp() == (tcgetpgrp(STDIN_FILENO)))
		restore_stdin();

	if (opt.filelistfile)
		feh_write_filelist(filelist, opt.filelistfile);

	return;
}
