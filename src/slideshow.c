/* slideshow.c

Copyright (C) 1999-2003 Tom Gilbert.
Copyright (C) 2010-2011 Daniel Friesel.
Copyright (C) 2012      Christopher Hrabak

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
#include "feh_ll.h"
#include "winwidget.h"
#include "options.h"

void init_slideshow_mode(LLMD *md )
{
	winwidget w = NULL;
	int success = 0;
	char *s = NULL;

	opt.flg.mode = MODE_SLIDESHOW ;

	if ( md->cn == md->rn )
		md->cn = md->rn->next;

	/* using the tn for this loop allows returning to ss_mode
	 * on the selected pix after stub_toggle_thumb_mode() */
	for (md->tn = md->rn->next ;
	    (md->tn != md->rn ) && opt.start_at_name ;
	     md->tn = md->tn->next) {
		if (!strcmp(opt.start_at_name, NODE_FILENAME(md->tn))) {
				opt.start_at_name = NULL;       /* found it */
				md->cn = md->tn;
				break;
		}
	}

	if (opt.start_at_name)
		weprintf("--start-at '%s': Not found.",opt.start_at_name);

	/* using --start-at-num you can start the ss at a specific [45 of 200] place */
	if (opt.start_at_num)
		feh_ll_nth( md, opt.start_at_num  );

	for (; md->cn != md->rn ; md->cn = md->cn->next) {
		/* make sure this one can be loaded, else find the next one that can be */
		s = slideshow_create_name( md );       /* static buff */

		if ((w = winwidget_create_from_file(md, md->cn, s, WIN_TYPE_SLIDESHOW)) == NULL) {
			/* load failed so remove md->cn */
			feh_move_node_to_remove_list(NULL, DELETE_NO, WIN_MAINT_NO );
		} else {
			success = 1;
			/* these 2 cause you can change to mm w/o ret to feh_main_iteration() */
			fgv.w = w;
			fgv.tdselected = NULL;
			winwidget_show(w);
			if (opt.slideshow_delay > 0.0)
				feh_add_timer(cb_slide_timer, w, opt.slideshow_delay, "SLIDE_CHANGE");

			break;
		}
	}

	if (!success)
		show_mini_usage();

	return;

}     /* end of init_slideshow_mode() */

void cb_slide_timer(void *data)
{
	slideshow_change_image((winwidget) data, SLIDE_NEXT, RENDER_YES);
	return;
}

void cb_reload_timer(void ){
		/* This just sets the stage for an exit back to main()
		 * to "doit_again", letting the normal load logic do its thing.
		 */

	/* save the current filename for refinding it in new list */
	if ( opt.start_at_name )
		free( opt.start_at_name);

	if (feh_md->cn == feh_md->rn )
		opt.start_at_name = NULL;
	else
		opt.start_at_name = estrdup(NODE_FILENAME(feh_md->cn));

	feh_ll_free_md( feh_md );           /* cleans the whole list in one shot  */
	fgv.doit_again = 1;                   /* the real reload is done from main()*/
	winwidget_destroy_all();              /* triggers the exit from iterations()*/

	return;

}     /* end of cb_reload_timer() */



void reload_logic( int argc, char **argv, int optind ){
		/* Apr 2013 HRABAK completely new reload logic.  Save the optind at
		 * the end of parse_options() here.  Each subsequent call can then
		 * reuse that ptr to reload any files at the end of the command line
		 * option parameters.  After that, you just return to main and
		 * "doit_again".
		 * This is the ONLY place that feh_add_unique_timer() is called.
		 */

	static int old_optind = 0;

	if (old_optind == 0 )       /* first time call so save it */
		old_optind = optind;
	else
		optind = old_optind;

	/* First and subsequent calls use that old_optint to reload the tail
	 * end of the command line options list, where the leftovers are
	 * assumed to be file names */
	if (optind < argc) {
		while (optind < argc) {
			/* If recursive is NOT set, but the only argument is a directory
			   name, we grab all the files in there, but not subdirs */
			add_file_to_filelist_recursively(argv[optind++], FILELIST_FIRST);
		}
	}
	else if ( !opt.filelistfile && !opt.flg.bgmode)
		add_file_to_filelist_recursively(".", FILELIST_FIRST);

	if (opt.reload)             /* trigger the next reload */
			feh_add_unique_timer(cb_reload_timer, opt.reload);

}   /* end of reload_logic()  */

void feh_reload_image(winwidget w, int resize, int force_new)
{
	Imlib_Image tmp;
	char *title = mobs(2);
	char *new_title = mobs(2);
	int old_w, old_h;

	if (!w->node) {
		im_weprintf(w, "%sreload, this image has no file associated with it.", ERR_CANNOT);
		winwidget_render_image(w, 0, 0, SANITIZE_NO);
		return;
	}

	D(("resize %d, force_new %d\n", resize, force_new));

	if ( !(NODE_CAPTION(w->node) == fgv.no_cap
			|| NODE_CAPTION(w->node) == NULL ) )
			free(NODE_CAPTION(w->node));

	NODE_CAPTION(w->node) = NULL;

	sprintf(new_title, "Reloading: %s", w->name);
	strcpy(title, w->name );
	winwidget_update_title(w, new_title);

	old_w = gib_imlib_image_get_width(w->im);
	old_h = gib_imlib_image_get_height(w->im);

	/*
	 * If we don't free the old image before loading the new one, Imlib2's
	 * caching will get in our way.
	 * However, if --reload is used (force_new == 0), we want to continue if
	 * the new image cannot be loaded, so we must not free the old image yet.
	 */
	if (force_new)
		winwidget_free_image(w);

	if ((feh_load_image(&tmp, NODE_DATA(w->node))) == 0) {
		if (force_new)
			eprintf("%s to reload image\n",ERR_FAILED);
		else {
			im_weprintf(w, "%sreload image. Is it still there?",ERR_CANNOT);
			winwidget_render_image(w, 0, 0, SANITIZE_NO);
		}
		winwidget_update_title(w, title);
		return;
	}

	if (!resize && ((old_w != gib_imlib_image_get_width(tmp)) ||
			(old_h != gib_imlib_image_get_height(tmp))))
		resize = 1;

	if (!force_new)
		winwidget_free_image(w);

	w->im = tmp;
	winwidget_reset_image(w);

	if ((w->im_w != gib_imlib_image_get_width(w->im))
			|| (w->im_h != gib_imlib_image_get_height(w->im)))
		w->had_resize = 1;
	if (w->has_rotated) {
		Imlib_Image temp;

		temp = gib_imlib_create_rotated_image(w->im, 0.0);
		w->im_w = gib_imlib_image_get_width(temp);
		w->im_h = gib_imlib_image_get_height(temp);
		gib_imlib_free_image_and_decache(temp);
	} else {
		w->im_w = gib_imlib_image_get_width(w->im);
		w->im_h = gib_imlib_image_get_height(w->im);
	}

	winwidget_render_image(w, resize, 0, SANITIZE_NO);
	winwidget_update_title(w, title);
	return;
}

enum misc_flags test_first_last( LLMD *md , enum misc_flags jmp_code){
		/* Ok. I do this in such an odd way to ensure that if the last or first
		* image is not loadable, it will go through in the right direction to
		* find the correct one. Otherwise SLIDE_LAST would try the last file,
		* then loop forward to find a loadable one.
		*/

	if (jmp_code == SLIDE_FIRST) {
		md->cn = md->rn->prev;
		jmp_code = SLIDE_NEXT;
	} else if (jmp_code == SLIDE_LAST) {
		md->cn = md->rn->next;
		jmp_code = SLIDE_PREV;
	}
return jmp_code;
}   /* end of test_first_last() */

void slideshow_change_image(winwidget w,
                            enum misc_flags jmp_code,
                            enum misc_flags render){
		/* HRABAK added SLIDE_NO_JUMP to just refresh md->cn in the winwid */

	LLMD *md = w->md;           /* save incase the window gets recreated     */

	md->tn = md->cn;            /* save the current one to know when to stop */

	/* Without this, clicking a one-image slideshow reloads it. Not very
	 * intelligent behaviour :-) .  True.  But HRABAK mar 2013 needs a refresh
	 * if deleting (or moving) all but one pic in the list*/
	if (md->rn->nd.cnt < 2
		&& opt.flg.cycle_once == 0
		&& jmp_code < SLIDE_NO_JUMP ) return;

	jmp_code = test_first_last( md , jmp_code);

	/* The do loop prevents us looping infinitely */
	do {
		winwidget_free_image(w);
		switch (jmp_code) {
		case SLIDE_NEXT:
			feh_list_jump( md , SLIDE_NEXT );
			break;
		case SLIDE_PREV:
			feh_list_jump( md , SLIDE_PREV );
			break;
		case SLIDE_RAND:
			if ( md->rn->nd.cnt > 1) {
        feh_list_jump( md , SLIDE_RAND );
				jmp_code = SLIDE_NEXT;
			}
			break;
		case SLIDE_JUMP_FWD:
			feh_list_jump( md , SLIDE_JUMP_FWD);
			/* important. if the load fails, we only want to step fwd ONCE to
			   try the next file, not another jmp */
			jmp_code = SLIDE_NEXT;
			break;
		case SLIDE_JUMP_BACK:
			feh_list_jump( md , SLIDE_JUMP_BACK );
			/* important. if the load fails, we only want to step back ONCE to
			   try the previous file, not another jmp */
			jmp_code = SLIDE_PREV;
			break;
		case SLIDE_NO_JUMP:       /* do nothing but refresh winwid */
			break;
		default:
			eprintf("BUG!\n");
			break;
		}

		if ((winwidget_loadimage(w, NODE_DATA(md->cn ))) != 0) {
			w->md   = md;
			w->node = md->cn;
			if ((w->im_w != gib_imlib_image_get_width(w->im))
					|| (w->im_h != gib_imlib_image_get_height(w->im)))
					w->had_resize = 1;
			winwidget_reset_image(w);
			w->im_w = gib_imlib_image_get_width(w->im);
			w->im_h = gib_imlib_image_get_height(w->im);
			if (render)
				winwidget_render_image(w, 1, 0, SANITIZE_NO);

			winwidget_update_title(w, slideshow_create_name( md) );

			break;        /* out of the for loop */
		} else          /* removes feh_md->cn */
			feh_move_node_to_remove_list(NULL, DELETE_NO, WIN_MAINT_NO );

	} while (  md->tn != md->cn );          /* avoids inf loop */

	if ( md->rn->nd.cnt == 0)
		eprintf("No more slides in show");

	if (opt.slideshow_delay > 0.0)
		feh_add_timer(cb_slide_timer, w, opt.slideshow_delay, "SLIDE_CHANGE");
	return;

}     /* end of slideshow_change_image() */

char *slideshow_create_name(LLMD *md){
    /* returns s, which points to static buffer
     * Caller does NOT free it.
     */
	char *s = mobs(2);

	if ( opt.title ) {
		s = feh_printf(opt.title, NODE_DATA(md->cn));
	} else {
			/* we want filename, but what part(s)? */
			char *name, *last1=NULL;
			name = NODE_FILENAME(md->cn);
			if (opt.flg.draw_name) {
				name = NODE_NAME(md->cn);
				if ( opt.flg.draw_no_ext )
					if ( ( last1 = NODE_EXT(md->cn))  )
							*last1 = '\0';
			}

			sprintf(s, PACKAGE " [%d of %d] - %s",
					    md->cn->nd.cnt,
					    md->rn->nd.cnt,
					    name );
			if ( last1 ) *last1 = '.';     /* restore the name+extension */

	}

	return(s);
}

void feh_action_run( feh_node *node, char *action) {
		/* Apr 2013 HRABAK killed the hold_actions array.  Just look
		 * at the leading char and if ';', this is a hold action.
		 */

	if (action) {
		char *sys;
		if ( action[0] == ';' )
				action += 1;          /* just jump past the "hold" indicator */

		D(("Running action %s\n", action));
		sys = feh_printf(action, NODE_DATA(node) );

		if (opt.flg.verbose &&
		  !(opt.flg.mode == MODE_LIST) &&
		   !opt.customlist)
			fprintf(stderr, "Running action -->%s<--\n", sys);

		esystem(sys);
	}
	return;
}

void shell_escape(char *input, char *ret) {
		/* accept the original buffer and just append the
		* modified input in place.
		*/
	unsigned int in = 0;
	char *out;

	if ( ( out = strchr( ret, '\0') ) == NULL ) out = ret;
	*out++ = '\'';

	for (in = 0; input[in] ; in++) {
		if (input[in] == '\'') {
			*out++ = '\'';
			*out++ = '"';
			*out++ = '\'';
			*out++ = '"';
			*out++ = '\'';
		}	else
			*out++ = input[in];
	}
	*out++ = '\'';
	*out++ = '\0';

}

char *format_size(int size)
{
	static char ret[5];
	char units[] = {' ', 'k', 'M', 'G', 'T'};
	unsigned char postfix = 0;
	while (size >= 1000) {
		size /= 1000;
		postfix++;
	}
	snprintf(ret, 5, "%3d%c", size, units[postfix]);
	return ret;
}

char *feh_printf(char *str, feh_data * data){
		/* only run feh_ll_load_data_info() once per call.  If !file, or
		 * feh_ll_load_data_info() fails, nothing gets returned to caller.
		 * ret is a huge buffer so snprintf() check is not necessary.
		 */

	int  found_it;
	char *ret = mobs(0);        /* mobs() already '\0' trunc'd      */
	char *tail = ret;           /* points to the tail of ret buffer */
	char *c, *last1 = NULL;

	for (c = str; *c != '\0'; c++) {
		if ((*c == '%') && (*(c+1) != '\0')) {
			c++;
			found_it=1;
			if (data ) {
				/* allow dropping off the extension from the filename */
				if ( opt.flg.draw_no_ext && ( last1 = data->ext) ) *last1 = '\0';
				if (*c == 'f')        { tail = stpcpy(tail, data->filename);
				} else if (*c == 'F') { shell_escape(data->filename, ret);
				} else if (*c == 'n') { tail = stpcpy(tail, data->name);
				} else if (*c == 'N') { shell_escape(data->name, ret);
				} else {
						if (!data->info){   /* just check once */
							if ( feh_ll_load_data_info(data, NULL) )    /*  oops!  failed load... */
								if (*c=='w'||*c=='h'||*c=='s'||*c=='S'||*c=='p'||*c=='t') continue;
						}       /* falls thru to the rest */
						if (*c == 'w')        {tail += sprintf(tail , "%d", data->info->width);
						} else if (*c == 'h') {tail += sprintf(tail , "%d", data->info->height);
						} else if (*c == 's') {tail += sprintf(tail , "%d", data->info->size);
						} else if (*c == 'S') {tail  = stpcpy(tail, format_size(data->info->size));
						} else if (*c == 'p') {tail += sprintf(tail , "%d", data->info->pixels);
						} else if (*c == 't') {tail  = stpcpy(tail, data->info->format);
						} else found_it=0;
				}
				if ( last1 ) *last1 = '.';      /* restore the name+extension */
				if ( found_it ) continue;       /* ok to loop again */
			}   /* end of if(data) */

			/* did not find the data-dependant opts, so test the rest.
			 * Even if it WAS a data dependant opt, but no data, I assume this
			 * if an unlikely event cause we are supposed to ONLY have good data members.
			 * found_it is still == 1 at this point.
			 */
			if (*c == 'v')        {tail  = stpcpy(tail, VERSION);
			} else if (*c == 'm') {tail  = stpcpy(tail, opt.modes[opt.flg.mode]);
			} else if (*c == 'l') {tail += sprintf(tail , "%d", feh_md->rn->nd.cnt );
			} else if (*c == 'u') {tail += sprintf(tail , "%d", feh_md->cn->nd.cnt );
			} else if (*c == '%') {*tail++ = '%';
			} else {
						weprintf("Unrecognized format specifier %%%c", *c);
						*tail++ = *(c - 1); /* add it verbatum */
						*tail++ = *c ;
						found_it=0;
			}
			if (found_it) continue;
			/* end of switch for a % prefixed code */
		} else if ((*c == '\\') && (*(c+1) != '\0')) {
			c++;
			if (*c == 'n') {
				*tail++ = '\n';       /* add a new-line */
			} else {
				*tail++ = *(c - 1);   /* add it verbatum */
				*tail++ = *c ;
			}
		} else
			*tail++ = *c ;
	}
	tail[0]=tail[1]='\0';       /* double NULL terminated */
	return(ret);
}    /* end of feh_printf() */

void feh_move_node_to_remove_list(winwidget w,
                                  enum misc_flags delete_flag ,
                                  enum misc_flags w_maint_flag ) {
		/* deals only with the md->cn node (picture)
		* feh_ll_unlink() sets the new cn to ->next after remove
		* As of May 2013 HRABAK moves this node to rm_md which will
		* be processed at the end by feh_clean_exit() rather than
		* removing it on the spot.
		*/

	if ( rm_md == NULL )
			return;

	if ( delete_flag == DELETE_ALL){
			/* ONLY called by feh_clean_exit() so no need to free anything
			 * as this is the end of the feh session.
			*/
			for ( rm_md->cn  = rm_md->rn->next ;
			      rm_md->cn != rm_md->rn ;
			      rm_md->cn  = rm_md->cn->next)
				if (rm_md->cn->nd.delete) unlink(NODE_FILENAME(rm_md->cn));

	}else {           /* this is an add request */
			/*if ( feh_md->rn->nd.cnt > 1 ) { */      /* don't delete the last pic */
			if ( feh_md->cn != feh_md->rn ) {
				feh_md->tn = feh_md->cn;                /* tmp storage */
				feh_ll_unlink( feh_md, FREE_NO );
				feh_md->tn->nd.delete = delete_flag ;   /* 0 or 1 */
				/* just relink from feh_md LL to rm_md LL */
				feh_ll_link_at_end( rm_md , feh_md->tn);
				feh_ll_recnt( feh_md );
				feh_md->rn->nd.lchange = 1;
			}
	}

	/* do we need to update any window stuff? */
	if ( w_maint_flag != WIN_MAINT_DO )
		return;

	if (w->type == WIN_TYPE_SLIDESHOW) {
		if ( LL_CNT(w->md) == 0 ) {
			/* No more images. Game over ;-) */
			winwidget_destroy(w);
			return;
		}
		slideshow_change_image(w, SLIDE_NO_JUMP, RENDER_YES);
	} else {
		fgv.tdselected = NULL;
		if ((w->type == WIN_TYPE_SINGLE)||
		    (w->type == WIN_TYPE_THUMBNAIL_VIEWER))
			winwidget_destroy(w);
	}
}   /* end of feh_move_node_to_remove_list() */

void slideshow_save_image(winwidget w)
{
	Imlib_Load_Error err;
	char *tmpname;

	if (w->node) {
		tmpname = feh_unique_filename("", NODE_NAME(w->node ));
	} else if (opt.flg.mode) {
		char tmp[32];
		STRCAT_2ITEMS(tmp,opt.modes[opt.flg.mode], ".png");
		tmpname = feh_unique_filename("", tmp);
	} else {
		tmpname = feh_unique_filename("", "noname.png");
	}

	if (opt.flg.verbose)
		printf("saving image to filename '%s'\n", tmpname);

	/* XXX gib_imlib_save_image_with_error_return breaks with *.XXX and
	 * similar because it tries to set the image format, which only works
	 * with .xxx .
	 * So we leave that part out.
	 */
	imlib_context_set_image(w->im);
	imlib_save_image_with_error_return(tmpname, &err);

	if (err)
		im_weprintf(w, "%ssave image %s:",ERR_CANNOT, tmpname);

	return;
}

void  feh_list_jump( LLMD *md , enum misc_flags jmp_code ){
		/* handle the simple SLIDE_NEXT and _PREV cases here,
		* else hand it off to feh_ll_nth.
		* Sets md->cn (current_node) to the jumped-to position.
		*/

	int i  =5;        /* used to be 20, but 5 makes it a 20% jump */
	int num=1;

	if ( jmp_code == SLIDE_PREV ) {
		md->cn = md->cn->prev;
		if ( md->cn == md->rn )
			md->cn = md->rn->prev;
		return;
	}

	if ( jmp_code == SLIDE_NEXT ) {
		md->cn = md->cn->next;
		if ( md->cn == md->rn )
			md->cn = md->rn->next;
		if (opt.flg.cycle_once) { exit(0); }
		return;
	}

	/* more than a +/-1 jump, so calc how much to jump */
	if ( jmp_code > SLIDE_RAND ) {        /* a 20% jump request */
			num = (md->rn->nd.cnt / i ) + 1;  /* round up */
		if ( jmp_code == SLIDE_JUMP_BACK )
			num *= -1;
	} else if ( jmp_code == SLIDE_RAND ) {
			num = ( rand() % ( md->rn->nd.cnt - 1)) + 1;
			/*jmp_code = SLIDE_NEXT;*/
	}

	num += md->cn->nd.cnt;                /* make it absolute  */
	if ( num > md->rn->nd.cnt )
		num -= md->rn->nd.cnt;    /* allow to wrap around the rn */
	if ( num < 1 )
		num += md->rn->nd.cnt;    /* allow to wrap around the rn */
	feh_ll_nth( md, num );

	return ;

}     /* end of feh_list_jump() */
