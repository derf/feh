/* main.c

Copyright (C) 1999-2003 Tom Gilbert.
Copyright (C) 2010-2011 Daniel Friesel.

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
#include "wallpaper.h"

/* these two hold almost all the feh global flags and variables         */
fehoptions opt;
feh_global_vars fgv;              /* 'fgv' stands for FehGlobalVars     */
/* global metaData containers for the two main linkedLists              */
LLMD *feh_md, *rm_md;             /* 'LLMD' stands for LinkListMetaData */

int main(int argc, char **argv){
		/* Apr 2013 HRABAK allows --reload to just repeat the inital load
		 * logic by looping back thru main().  This also allows me to toggle,
		 * on the fly, between slideshow and thumbnail modes.
		 */

	int first_time =1;

	atexit(feh_clean_exit);
	imlib_set_cache_size(1048 * 1024 * 20);         /* a 20mb cache     */

	/* initialize all feh globals */
	init_all_fgv();
	feh_md = init_LLMD();       /* replaces the old filelist            */
	rm_md  = init_LLMD();       /* replaces the old rm_filelist         */
	feh_menu_init_mnu_txt();    /* text for ALL menu items              */

	setup_signal_handlers();

	fgv.doit_again = 1;         /* first time thru                      */
	while(fgv.doit_again){

		if (first_time){
			init_parse_options(argc, argv);
			D(("About to parse commandline options\n"));
			feh_parse_option_array(argc, argv );        /* Parse the cmdline args */
			D(("Options parsed\n"));
			check_options();
			/* note:  Everything from here on uses opt.flg.mode rather
			 * than the individual opt.flg.thumbnail type flags  */
			init_feh_fonts();         /* do this after opt parse */
			feh_read_filelist( feh_md , opt.filelistfile );
		} else if (opt.reload )
			reload_logic(argc, argv, 0 );


		if ( feh_prepare_filelist( feh_md ))
			show_mini_usage();

		if (first_time) {
			if (opt.flg.display) {
				init_x_and_imlib();
				init_keyevents();
				init_buttonbindings();
				/* these two used to be static iteration() values */
				fgv.xfd = ConnectionNumber(fgv.disp);
				fgv.fdsize = fgv.xfd + 1;
			}
			feh_event_init();
		}

		if (opt.flg.mode == MODE_INDEX || opt.flg.mode == MODE_THUMBNAIL )
			init_thumbnail_mode( feh_md , opt.flg.mode );
		else if (opt.flg.mode == MODE_MULTIWINDOW)
			init_multiwindow_mode( feh_md );
		else if (opt.flg.mode == MODE_LIST )              /* customlist too */
			init_list_mode( feh_md );
		else if (opt.flg.mode == MODE_LOADABLES || opt.flg.mode == MODE_UNLOADABLES )
			real_loadables_mode( feh_md );
		else if (opt.flg.bgmode) {
			if ( LL_CNT( feh_md ) == 0)
				eprintf("No files specified for background setting");
			feh_wm_set_bg(NULL, NULL, opt.flg.bgmode, 0, 1);
			exit(0);
		}	else {
			/* Slideshow mode is the default. Because it's spiffy */
			init_slideshow_mode(feh_md);
		}

		/* main event loop */
		first_time=0;
		fgv.doit_again=0;
		fgv.pt = feh_get_time();

		/* like a macro processor to FORCE the next func call */
		if ( fgv.ptr ) {
			StubPtr fptr = fgv.ptr;
			fgv.ptr = NULL;
			fptr();       /* this is the forced call */
		}

		while (feh_main_iteration(1));

	}  /* end of doit_again */

	return(0);
}   /* end of main() */

int feh_main_iteration(int block){
		/* Return 0 to stop iterating, 1 if ok to continue.
		 * the orig "xfd, fdsize, pt" vars are now fgv globals
		 */

	XEvent ev;
	struct timeval tval;
	fd_set fdset;
	int count = 0;
	double t1 = 0.0, t2 = 0.0;
	fehtimer ft;

	if (fgv.window_num == 0)
		return(0);

	/* Timers */
	t1 = feh_get_time();
	t2 = t1 - fgv.pt;
	fgv.pt = t1;
	while (XPending(fgv.disp)) {
		XNextEvent(fgv.disp, &ev);
		if (ev_handler[ev.type])
			(*(ev_handler[ev.type])) (&ev);

		if (fgv.window_num == 0)
			return(0);
	}
	XFlush(fgv.disp);

	feh_redraw_menus();

	FD_ZERO(&fdset);
	FD_SET(fgv.xfd, &fdset);

	/* Timers */
	ft = first_timer;
	/* Don't do timers if we're zooming/panning/etc or if we are paused */
	if (ft && (opt.flg.state == STATE_NORMAL) && !opt.flg.paused) {
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

		XSync(fgv.disp, False);
		D(("I next need to action a timer in %f seconds\n", t1));
		/* Only do a blocking select if there's a timer due, or no events
		   waiting */
		if (t1 == 0.0 || (block && !XPending(fgv.disp))) {
			tval.tv_sec = (long) t1;
			tval.tv_usec = (long) ((t1 - ((double) tval.tv_sec)) * 1000000);
			if (tval.tv_sec < 0)
				tval.tv_sec = 0;
			if (tval.tv_usec <= 1000)
				tval.tv_usec = 1000;
			errno = 0;
			D(("Performing blocking select - waiting for timer or event\n"));
			count = select(fgv.fdsize, &fdset, NULL, NULL, &tval);
			if ((count < 0)
					&& ((errno == ENOMEM) || (errno == EINVAL)
						|| (errno == EBADF)))
				eprintf("Connection to X display lost");
			if ((ft) && (count == 0)) {
				/* This means the timer is due to be executed. If count was > 0,
				   that would mean an X event had woken us, we're not interested
				   in that */
				feh_handle_timer();
			}
		}
	} else {
		/* Don't block if there are events in the queue. That's a bit rude ;-) */
		if (block && !XPending(fgv.disp)) {
			errno = 0;
			D(("Performing blocking select - no timers, or zooming\n"));
			count = select(fgv.fdsize, &fdset, NULL, NULL, NULL);
			if ((count < 0)
					&& ((errno == ENOMEM) || (errno == EINVAL)
						|| (errno == EBADF)))
				eprintf("Connection to X display lost");
		}
	}
	if (fgv.window_num == 0)
		return(0);

	return(1);
}

void feh_clean_exit(void){
		/* write out lots of lists B4 cleaning out those flagged for
		 * deletion.  Note:  The [0-9] codes passed to write_filelist()
		 * are the real bit possitions for the nd.(flags) members.  But
		 * integers 10,11 and 12 are bogus codes used like an enum.
		 * The .SSL extention stands for SlideShowList.
		 */

	#define WRITE_FILE_LIST      10
	#define WRITE_REMOVE_LIST    11
	#define WRITE_DELETE_LIST    12

	if ( opt.flg.write_filelist ){
		/* you can avoid writing ANY of these lists with the
		 * --nowrite-filelist cmdline option.
		 */

		/* write out the filelist LL ONLY if it changed */
		if (opt.filelistfile && LL_CHANGED(feh_md))
			feh_write_filelist( feh_md , opt.filelistfile, WRITE_FILE_LIST );

		/* if any were tagged for (remove/delete) write those lists too.
		* Note: feh_dele.SSL will be empty if just --preload removed some.
		*/
		if (rm_md->rn->nd.cnt ){
			feh_write_filelist( rm_md , "feh_remv.SSL", WRITE_REMOVE_LIST );
			feh_write_filelist( rm_md , "feh_dele.SSL", WRITE_DELETE_LIST );
		}

		/* if any of the nd.list[0-9] have been tagged, write out those lists
		* to filenames like feh[0-9]list.SSL.  It is up to the user to do
		* something with those feh?list.SSL lists before the next run.
		* feh does nothing with them except write them.
		*/
		if ( feh_md->rn->nd.tagged ){
			if ( feh_md->rn->nd.list0 ) feh_write_filelist( feh_md ,NULL,0 );
			if ( feh_md->rn->nd.list1 ) feh_write_filelist( feh_md ,NULL,1 );
			if ( feh_md->rn->nd.list2 ) feh_write_filelist( feh_md ,NULL,2 );
			if ( feh_md->rn->nd.list3 ) feh_write_filelist( feh_md ,NULL,3 );
			if ( feh_md->rn->nd.list4 ) feh_write_filelist( feh_md ,NULL,4 );
			if ( feh_md->rn->nd.list5 ) feh_write_filelist( feh_md ,NULL,5 );
			if ( feh_md->rn->nd.list6 ) feh_write_filelist( feh_md ,NULL,6 );
			if ( feh_md->rn->nd.list7 ) feh_write_filelist( feh_md ,NULL,7 );
			if ( feh_md->rn->nd.list8 ) feh_write_filelist( feh_md ,NULL,8 );
			if ( feh_md->rn->nd.list9 ) feh_write_filelist( feh_md ,NULL,9 );
		}
	}

	/* used to be delete_rm_files() */
	feh_move_node_to_remove_list(NULL, DELETE_ALL, WIN_MAINT_NO );

	return;
}   /* end of feh_clean_exit() */
