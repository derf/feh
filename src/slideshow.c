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
#include "filelist.h"
#include "timers.h"
#include "winwidget.h"
#include "options.h"
#include "signals.h"

void init_slideshow_mode(void)
{
	winwidget w = NULL;
	int success = 0;
	char *s = NULL;
	feh_node *l;

	for (l = feh_md->rn->next ; ( l != feh_md->rn ) && opt.start_list_at ; l = l->next) {
		if (!strcmp(opt.start_list_at, FEH_FILE(l->data)->filename)) {
        opt.start_list_at = NULL;       /* found it */
        break;
		}
	}

	if (opt.start_list_at)
		eprintf("--start-at %s: File not found in filelist",
				opt.start_list_at);


	mode = "slideshow";

	for (; l != feh_md->rn ; l = l->next) {
    /* make sure this one can be loaded, else find the next one that can be */
    feh_md->cn = l;          /* 	current_file = l */
		s = slideshow_create_name( FEH_FILE(l->data) );

		if ((w = winwidget_create_from_file(l, s, WIN_TYPE_SLIDESHOW)) == NULL) {
    	free(s);
 			feh_file_remove_from_list( feh_md );     /* l failed so remove it */
		} else {
			free(s);
			success = 1;
			winwidget_show(w);
			if (opt.slideshow_delay > 0.0)
				feh_add_timer(cb_slide_timer, w, opt.slideshow_delay, "SLIDE_CHANGE");
			else if (opt.reload > 0)
				feh_add_unique_timer(cb_reload_timer, w, opt.reload);
			break;
		}
	}

	if (!success)
		show_mini_usage();

	return;

}     /* end of init_slideshow_mode() */

void cb_slide_timer(void *data)
{
	slideshow_change_image((winwidget) data, SLIDE_NEXT, 1);
	return;
}

void cb_reload_timer(void *data)
{
	feh_node *l;
	char *current_filename;

	winwidget w = (winwidget) data;

	/* save the current filename for refinding it in new list */
	current_filename = estrdup(FEH_FILE(feh_md->cn->data)->filename);

  feh_file_free_md( feh_md );          /* cleans the whole list in one shot */

	/* rebuild filelist from original_file_items */
	if ( ofi_md->rn->nd.cnt > 0)
      for (l = ofi_md->rn->next ; l != ofi_md->rn ; l = l->next )
        add_file_to_filelist_recursively(l->data, FILELIST_FIRST);
	else if (!opt.filelistfile && !opt.bgmode)
      add_file_to_filelist_recursively(".", FILELIST_FIRST);

	if ( feh_md->rn->nd.cnt == 0  )
      eprintf("No files found to reload.");


	/* find the previously current file */
  for (l = ofi_md->rn->next ; l != ofi_md->rn ; l = l->next )
		if (strcmp(FEH_FILE(l->data)->filename, current_filename) == 0) {
			feh_md->cn = l;          /* current_file = l; */
			break;
		}

	free(current_filename);

	/* old code had to reverse the list here.  Not necessary anymore.*/

	if ( feh_md->cn == feh_md->rn )
      feh_md->cn = feh_md->rn->next;
	w->file = feh_md->cn ;

	/* reset window name in case of current file order,
	 * filename, or filelist_length has changed.
	 */
	current_filename = slideshow_create_name(FEH_FILE(FEH_LL_CUR_DATA(feh_md)));
	winwidget_rename(w, current_filename);
	free(current_filename);

	feh_reload_image(w, 1, 0);
	feh_add_unique_timer(cb_reload_timer, w, opt.reload);
	return;

}     /* end of cb_reload_timer() */

void feh_reload_image(winwidget w, int resize, int force_new)
{
	char *title, *new_title;
	int len;
	Imlib_Image tmp;
	int old_w, old_h;

	if (!w->file) {
		im_weprintf(w, "couldn't reload, this image has no file associated with it.");
		winwidget_render_image(w, 0, 0);
		return;
	}

	D(("resize %d, force_new %d\n", resize, force_new));

	free(FEH_FILE(w->file->data)->caption);
	FEH_FILE(w->file->data)->caption = NULL;

	len = strlen(w->name) + sizeof("Reloading: ") + 1;
	new_title = emalloc(len);
	snprintf(new_title, len, "Reloading: %s", w->name);
	title = estrdup(w->name);
	winwidget_rename(w, new_title);

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

	if ((feh_load_image(&tmp, FEH_FILE(w->file->data))) == 0) {
		if (force_new)
			eprintf("failed to reload image\n");
		else {
			im_weprintf(w, "Couldn't reload image. Is it still there?");
			winwidget_render_image(w, 0, 0);
		}
		winwidget_rename(w, title);
		free(title);
		free(new_title);
		return;
	}

	if (!resize && ((old_w != gib_imlib_image_get_width(tmp)) ||
			(old_h != gib_imlib_image_get_height(tmp))))
		resize = 1;

	if (!force_new)
		winwidget_free_image(w);

	w->im = tmp;
	winwidget_reset_image(w);

	w->mode = MODE_NORMAL;
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
	winwidget_render_image(w, resize, 0);

	winwidget_rename(w, title);
	free(title);
	free(new_title);

	return;
}

void slideshow_change_image(winwidget winwid, int change, int render)
{
	feh_node *last = NULL;
	int i = 0;

	/* We can't use filelist_len in the for loop, since that changes when we
	 * encounter invalid images.
	 */
	int our_filelist_len = FEH_LL_LEN( feh_md );
	char *s;

	/* Without this, clicking a one-image slideshow reloads it. Not very *
	   intelligent behaviour :-) */
	if (our_filelist_len < 2 && opt.cycle_once == 0)
		return;

	/* Ok. I do this in such an odd way to ensure that if the last or first *
	   image is not loadable, it will go through in the right direction to *
	   find the correct one. Otherwise SLIDE_LAST would try the last file, *
	   then loop forward to find a loadable one. */
	if (change == SLIDE_FIRST) {
		feh_md->cn = feh_md->rn->prev;
		change = SLIDE_NEXT;
	} else if (change == SLIDE_LAST) {
		feh_md->cn = feh_md->rn->next;
		change = SLIDE_PREV;
	}

	/* The for loop prevents us looping infinitely */
	for (i = 0; i < our_filelist_len; i++) {
		winwidget_free_image(winwid);
		switch (change) {
		case SLIDE_NEXT:
			feh_list_jump( feh_md , SLIDE_NEXT );
			break;
		case SLIDE_PREV:
			feh_list_jump( feh_md , SLIDE_PREV );
			break;
		case SLIDE_RAND:
			if ( feh_md->rn->nd.cnt > 1) {
        feh_list_jump( feh_md , SLIDE_RAND );
				change = SLIDE_NEXT;
			}
			break;
		case SLIDE_JUMP_FWD:
			feh_list_jump( feh_md , SLIDE_JUMP_FWD);
			/* important. if the load fails, we only want to step fwd ONCE to
			   try the next file, not another jmp */
			change = SLIDE_NEXT;
			break;
		case SLIDE_JUMP_BACK:
			feh_list_jump( feh_md , SLIDE_JUMP_BACK );
			/* important. if the load fails, we only want to step back ONCE to
			   try the previous file, not another jmp */
			change = SLIDE_PREV;
			break;
		default:
			eprintf("BUG!\n");
			break;
		}

		if (last) {
      feh_md->cn = last;
			feh_file_remove_from_list( feh_md );
			last = NULL;
		}

		if ((winwidget_loadimage(winwid, FEH_FILE(feh_md->cn->data)))
		    != 0) {
			winwid->mode = MODE_NORMAL;
			winwid->file = feh_md->cn;
			if ((winwid->im_w != gib_imlib_image_get_width(winwid->im))
			    || (winwid->im_h != gib_imlib_image_get_height(winwid->im)))
				winwid->had_resize = 1;
			winwidget_reset_image(winwid);
			winwid->im_w = gib_imlib_image_get_width(winwid->im);
			winwid->im_h = gib_imlib_image_get_height(winwid->im);
			if (render)
				winwidget_render_image(winwid, 1, 0);

			s = slideshow_create_name(FEH_FILE(feh_md->cn->data));
			winwidget_rename(winwid, s);
			free(s);

			break;
		} else
			last = feh_md->cn ;
	}
	if (last) {
      feh_md->cn = last;
			feh_file_remove_from_list( feh_md );
			last = NULL;
  }

	if ( feh_md->rn->nd.cnt == 0)
		eprintf("No more slides in show");

	if (opt.slideshow_delay > 0.0)
		feh_add_timer(cb_slide_timer, winwid, opt.slideshow_delay, "SLIDE_CHANGE");
	return;

}     /* end of slideshow_change_image() */

void slideshow_pause_toggle(winwidget w)
{
	if (!opt.paused) {
		opt.paused = 1;
	} else {
		opt.paused = 0;
	}

	winwidget_rename(w, NULL);
}

char *slideshow_create_name(feh_file * file)
{
	char *s = NULL;
	int len = 0;

	if (!opt.title) {
		len = strlen(PACKAGE " [slideshow mode] - ") + strlen(file->filename) + 1;
		s = emalloc(len);
		snprintf(s, len, PACKAGE " [%d of %d] - %s",
              feh_md->cn->nd.cnt,
              feh_md->rn->nd.cnt,
              file->filename);
	} else {
		s = estrdup(feh_printf(opt.title, file));
	}

	return(s);
}

void feh_action_run(feh_file * file, char *action)
{
	if (action) {
		char *sys;
		D(("Running action %s\n", action));
		sys = feh_printf(action, file);

		if (opt.verbose && !opt.list && !opt.customlist)
			fprintf(stderr, "Running action -->%s<--\n", sys);

		system(sys);
	}
	return;
}

char *shell_escape(char *input)
{
	static char ret[1024];
	unsigned int out = 0, in = 0;

	ret[out++] = '\'';
	for (in = 0; input[in] && (out < (sizeof(ret) - 7)); in++) {
		if (input[in] == '\'') {
			ret[out++] = '\'';
			ret[out++] = '"';
			ret[out++] = '\'';
			ret[out++] = '"';
			ret[out++] = '\'';
		}	else
			ret[out++] = input[in];
	}
	ret[out++] = '\'';
	ret[out++] = '\0';

	return ret;
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

char *feh_printf(char *str, feh_file * file)
{
	char *c;
	char buf[20];
	static char ret[4096];

	ret[0] = '\0';

	for (c = str; *c != '\0'; c++) {
		if ((*c == '%') && (*(c+1) != '\0')) {
			c++;
			switch (*c) {
			case 'f':
				if (file)
					strcat(ret, file->filename);
				break;
			case 'F':
				if (file)
					strcat(ret, shell_escape(file->filename));
				break;
			case 'n':
				if (file)
					strcat(ret, file->name);
				break;
			case 'N':
				if (file)
					strcat(ret, shell_escape(file->name));
				break;
			case 'w':
				if (file && (file->info || !feh_file_info_load(file, NULL))) {
					snprintf(buf, sizeof(buf), "%d", file->info->width);
					strcat(ret, buf);
				}
				break;
			case 'h':
				if (file && (file->info || !feh_file_info_load(file, NULL))) {
					snprintf(buf, sizeof(buf), "%d", file->info->height);
					strcat(ret, buf);
				}
				break;
			case 's':
				if (file && (file->info || !feh_file_info_load(file, NULL))) {
					snprintf(buf, sizeof(buf), "%d", file->info->size);
					strcat(ret, buf);
				}
				break;
			case 'S':
				if (file && (file->info || !feh_file_info_load(file, NULL))) {
					strcat(ret, format_size(file->info->size));
				}
				break;
			case 'p':
				if (file && (file->info || !feh_file_info_load(file, NULL))) {
					snprintf(buf, sizeof(buf), "%d", file->info->pixels);
					strcat(ret, buf);
				}
				break;
			case 'P':
				if (file && (file->info || !feh_file_info_load(file, NULL))) {
					strcat(ret, format_size(file->info->pixels));
				}
				break;
			case 't':
				if (file && (file->info || !feh_file_info_load(file, NULL))) {
					strcat(ret, file->info->format);
				}
				break;
			case 'v':
				strcat(ret, VERSION);
				break;
			case 'm':
				strcat(ret, mode);
				break;
			case 'l':
				snprintf(buf, sizeof(buf), "%d", feh_md->rn->nd.cnt );
				strcat(ret, buf);
				break;
			case 'u':
				snprintf(buf, sizeof(buf), "%d", feh_md->cn->nd.cnt );
				strcat(ret, buf);
				break;
			case '%':
				strcat(ret, "%");
				break;
			default:
				weprintf("Unrecognized format specifier %%%c", *c);
				strncat(ret, c - 1, 2);
				break;
			}
		} else if ((*c == '\\') && (*(c+1) != '\0')) {
			c++;
			switch (*c) {
			case 'n':
				strcat(ret, "\n");
				break;
			default:
				strncat(ret, c - 1, 2);
				break;
			}
		} else
			strncat(ret, c, 1);
	}
	return(ret);
}

void feh_filelist_image_remove(winwidget winwid, char do_delete) {
    /* deals only with the feh_md->cn node (picture)
     * But then what is cn set to once it gets deleted here???
     */

	if (winwid->type == WIN_TYPE_SLIDESHOW) {
		char *s;

		slideshow_change_image(winwid, SLIDE_NEXT, 0);
		if (do_delete ==  DELETE_YES )
			feh_file_rm_and_free( feh_md );        /* rm the md->cn */
		else
			feh_file_remove_from_list( feh_md );   /* just take md->cn out of the list */
		if ( FEH_LL_LEN(feh_md) == 0 ) {
			/* No more images. Game over ;-) */
			winwidget_destroy(winwid);
			return;
		}
		s = slideshow_create_name(FEH_FILE(winwid->file->data));
		winwidget_rename(winwid, s);
		free(s);
		winwidget_render_image(winwid, 1, 0);
	} else if ((winwid->type == WIN_TYPE_SINGLE)
		   || (winwid->type == WIN_TYPE_THUMBNAIL_VIEWER)) {
 		if (do_delete ==  DELETE_YES )
			feh_file_rm_and_free( feh_md );
		else
			feh_file_remove_from_list( feh_md );
		winwidget_destroy(winwid);
	}
}

void slideshow_save_image(winwidget win)
{
	char *tmpname;
	Imlib_Load_Error err;

	if (win->file) {
		tmpname = feh_unique_filename("", FEH_FILE(win->file->data)->name);
	} else if (mode) {
		char *tmp;
		tmp = estrjoin(".", mode, "png", NULL);
		tmpname = feh_unique_filename("", tmp);
		free(tmp);
	} else {
		tmpname = feh_unique_filename("", "noname.png");
	}

	if (opt.verbose)
		printf("saving image to filename '%s'\n", tmpname);

	/* XXX gib_imlib_save_image_with_error_return breaks with *.XXX and
	 * similar because it tries to set the image format, which only works
	 * with .xxx .
	 * So we leave that part out.
	 */
	imlib_context_set_image(win->im);
	imlib_save_image_with_error_return(tmpname, &err);

	if (err)
		im_weprintf(win, "Can't save image %s:", tmpname);

	free(tmpname);
	return;
}

void  feh_list_jump( LLMD *md , int direction_code ){
    /* always sets the md->cn (current_node) to the jumped-to slide.
     */

	int i, num=1;
	feh_node *l;

	if ( md->cn == md->rn )
      md->cn = md->rn->next;           /* this should never happen */

  l = md->cn;

  /* calc which way and how much */
  if ( direction_code > SLIDE_RAND ) {
      /* a 20% jump request */
			if ( md->rn->nd.cnt < 5)
				num = 1;
			else if ( md->rn->nd.cnt < 40)
				num = 2;
			else
				num = md->rn->nd.cnt / 20;
			if ( num == 0 )
				num = 2;
  } else if ( direction_code == SLIDE_RAND ) {
				num = ( rand() % ( md->rn->nd.cnt - 1)) + 1;
        direction_code = SLIDE_NEXT;
  }

	if ( direction_code == SLIDE_NEXT || direction_code == SLIDE_JUMP_FWD ) {
      for (i = 0; i < num; i++) {
          if ( l->next != md->rn ) {
            l = l->next;
          } else {
            if (opt.cycle_once) { exit(0); }
            l = md->rn->next;
          }
      }
  } else {
      for (i = 0; i < num; i++) {
          if ( l->prev != md->rn )
            l = l->prev;
          else
            l = md->rn->prev;
      }
	}

  md->cn = l;

	return ;

}     /* end of feh_list_jump() */
