/* slideshow.c

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
	gib_list *l = filelist, *last = NULL;
	feh_file *file = NULL;

	for (l = filelist; l && opt.start_list_at; l = l->next) {
		if (!strcmp(opt.start_list_at, FEH_FILE(l->data)->filename)) {
			opt.start_list_at = NULL;
			break;
		}
	}

	if (opt.start_list_at)
		eprintf("--start-at %s: File not found in filelist",
				opt.start_list_at);

	mode = "slideshow";
	for (; l; l = l->next) {
		file = FEH_FILE(l->data);
		if (last) {
			filelist = feh_file_remove_from_list(filelist, last);
			last = NULL;
		}
		current_file = l;
		s = slideshow_create_name(file, NULL);
		if ((w = winwidget_create_from_file(l, s, WIN_TYPE_SLIDESHOW)) != NULL) {
			free(s);
			success = 1;
			winwidget_show(w);
			if (opt.slideshow_delay > 0.0)
				feh_add_timer(cb_slide_timer, w, opt.slideshow_delay, "SLIDE_CHANGE");
			if (opt.reload > 0)
				feh_add_unique_timer(cb_reload_timer, w, opt.reload);
			break;
		} else {
			free(s);
			last = l;
		}
	}
	if (!success)
		show_mini_usage();

	return;
}

void cb_slide_timer(void *data)
{
	slideshow_change_image((winwidget) data, SLIDE_NEXT, 1);
	return;
}

void cb_reload_timer(void *data)
{
	gib_list *l;
	char *current_filename;

	winwidget w = (winwidget) data;

	/* save the current filename for refinding it in new list */
	current_filename = estrdup(FEH_FILE(current_file->data)->filename);

	for (l = filelist; l; l = l->next) {
		feh_file_free(l->data);
		l->data = NULL;
	}
	gib_list_free_and_data(filelist);
	filelist = NULL;
	filelist_len = 0;
	current_file = NULL;

	/* rebuild filelist from original_file_items */
	if (gib_list_length(original_file_items) > 0)
		for (l = gib_list_last(original_file_items); l; l = l->prev)
			add_file_to_filelist_recursively(l->data, FILELIST_FIRST);
	else if (!opt.filelistfile && !opt.bgmode)
		add_file_to_filelist_recursively(".", FILELIST_FIRST);
	
	if (!(filelist_len = gib_list_length(filelist))) {
		eprintf("No files found to reload.");
	}

	/* find the previously current file */
	for (l = filelist; l; l = l->next)
		if (strcmp(FEH_FILE(l->data)->filename, current_filename) == 0) {
			current_file = l;
			break;
		}

	free(current_filename);

	filelist = gib_list_first(gib_list_reverse(filelist));

	if (!current_file)
		current_file = filelist;
	w->file = current_file;

	/* reset window name in case of current file order,
	 * filename, or filelist_length has changed.
	 */
	current_filename = slideshow_create_name(FEH_FILE(current_file->data), w);
	winwidget_rename(w, current_filename);
	free(current_filename);

	feh_reload_image(w, 1, 0);
	feh_add_unique_timer(cb_reload_timer, w, opt.reload);
	return;
}

void feh_reload_image(winwidget w, int resize, int force_new)
{
	char *title, *new_title;
	int len;
	Imlib_Image tmp;
	int old_w, old_h;

	unsigned char tmode =0;
	int tim_x =0;
	int tim_y =0;
	double tzoom =0;

	tmode = w->mode;
	tim_x = w->im_x;
	tim_y = w->im_y;
	tzoom = w->zoom;

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
	if (opt.keep_zoom_vp) {
		/* put back in: */
		w->mode = tmode;
		w->im_x = tim_x;
		w->im_y = tim_y;
		w->zoom = tzoom;
		winwidget_render_image(w, 0, 0);
	} else {
		winwidget_render_image(w, resize, 0);
	}

	winwidget_rename(w, title);
	free(title);
	free(new_title);

	return;
}

void slideshow_change_image(winwidget winwid, int change, int render)
{
	gib_list *last = NULL;
	int i = 0;
	int jmp = 1;
	/* We can't use filelist_len in the for loop, since that changes when we
	 * encounter invalid images.
	 */
	int our_filelist_len = filelist_len;
	char *s;

	unsigned char tmode =0;
	int tim_x =0;
	int tim_y =0;
	double tzoom =0;

	/* Without this, clicking a one-image slideshow reloads it. Not very *
	   intelligent behaviour :-) */
	if (filelist_len < 2 && opt.cycle_once == 0)
		return;

	/* Ok. I do this in such an odd way to ensure that if the last or first *
	   image is not loadable, it will go through in the right direction to *
	   find the correct one. Otherwise SLIDE_LAST would try the last file, *
	   then loop forward to find a loadable one. */
	if (change == SLIDE_FIRST) {
		current_file = gib_list_last(filelist);
		change = SLIDE_NEXT;
	} else if (change == SLIDE_LAST) {
		current_file = filelist;
		change = SLIDE_PREV;
	}

	/* The for loop prevents us looping infinitely */
	for (i = 0; i < our_filelist_len; i++) {
		winwidget_free_image(winwid);
		switch (change) {
		case SLIDE_NEXT:
			current_file = feh_list_jump(filelist, current_file, FORWARD, 1);
			break;
		case SLIDE_PREV:
			current_file = feh_list_jump(filelist, current_file, BACK, 1);
			break;
		case SLIDE_RAND:
			if (filelist_len > 1) {
				current_file = feh_list_jump(filelist, current_file, FORWARD,
					(rand() % (filelist_len - 1)) + 1);
				change = SLIDE_NEXT;
			}
			break;
		case SLIDE_JUMP_FWD:
			if (filelist_len < 5)
				jmp = 1;
			else if (filelist_len < 40)
				jmp = 2;
			else
				jmp = filelist_len / 20;
			if (!jmp)
				jmp = 2;
			current_file = feh_list_jump(filelist, current_file, FORWARD, jmp);
			/* important. if the load fails, we only want to step on ONCE to
			   try the next file, not another jmp */
			change = SLIDE_NEXT;
			break;
		case SLIDE_JUMP_BACK:
			if (filelist_len < 5)
				jmp = 1;
			else if (filelist_len < 40)
				jmp = 2;
			else
				jmp = filelist_len / 20;
			if (!jmp)
				jmp = 2;
			current_file = feh_list_jump(filelist, current_file, BACK, jmp);
			/* important. if the load fails, we only want to step back ONCE to
			   try the previous file, not another jmp */
			change = SLIDE_NEXT;
			break;
		default:
			eprintf("BUG!\n");
			break;
		}

		if (last) {
			filelist = feh_file_remove_from_list(filelist, last);
			last = NULL;
		}

		if (opt.keep_zoom_vp) {
		/* pre loadimage - record settings */
			tmode = winwid->mode;
			tim_x = winwid->im_x;
			tim_y = winwid->im_y;
			tzoom = winwid->zoom;
		}

		if ((winwidget_loadimage(winwid, FEH_FILE(current_file->data)))
		    != 0) {
			winwid->mode = MODE_NORMAL;
			winwid->file = current_file;
			if ((winwid->im_w != gib_imlib_image_get_width(winwid->im))
			    || (winwid->im_h != gib_imlib_image_get_height(winwid->im)))
				winwid->had_resize = 1;
			winwidget_reset_image(winwid);
			winwid->im_w = gib_imlib_image_get_width(winwid->im);
			winwid->im_h = gib_imlib_image_get_height(winwid->im);
			if (opt.keep_zoom_vp) {
				/* put back in: */
				winwid->mode = tmode;
				winwid->im_x = tim_x;
				winwid->im_y = tim_y;
				winwid->zoom = tzoom;
			}
			if (render) {
				if (opt.keep_zoom_vp) {
					winwidget_render_image(winwid, 0, 0);
				} else {
					winwidget_render_image(winwid, 1, 0);
				}
			}

			s = slideshow_create_name(FEH_FILE(current_file->data), winwid);
			winwidget_rename(winwid, s);
			free(s);

			break;
		} else
			last = current_file;
	}
	if (last)
		filelist = feh_file_remove_from_list(filelist, last);

	if (filelist_len == 0)
		eprintf("No more slides in show");

	if (opt.slideshow_delay > 0.0)
		feh_add_timer(cb_slide_timer, winwid, opt.slideshow_delay, "SLIDE_CHANGE");
	return;
}

void slideshow_pause_toggle(winwidget w)
{
	if (!opt.paused) {
		opt.paused = 1;
	} else {
		opt.paused = 0;
	}

	winwidget_rename(w, NULL);
}

char *slideshow_create_name(feh_file * file, winwidget winwid)
{
	char *s = NULL;
	int len = 0;

	if (!opt.title) {
		len = strlen(PACKAGE " [slideshow mode] - ") + strlen(file->filename) + 1;
		s = emalloc(len);
		snprintf(s, len, PACKAGE " [%d of %d] - %s",
			gib_list_num(filelist, current_file) + 1, gib_list_length(filelist), file->filename);
	} else {
		s = estrdup(feh_printf(opt.title, file, winwid));
	}

	return(s);
}

void feh_action_run(feh_file * file, char *action)
{
	if (action) {
		char *sys;
		D(("Running action %s\n", action));
		sys = feh_printf(action, file, NULL);

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
		}
		else
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

char *feh_printf(char *str, feh_file * file, winwidget winwid)
{
	char *c;
	char buf[20];
	static char ret[4096];
	char *filelist_tmppath;

	ret[0] = '\0';
	filelist_tmppath = NULL;

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
			case 'h':
				if (file && (file->info || !feh_file_info_load(file, NULL))) {
					snprintf(buf, sizeof(buf), "%d", file->info->height);
					strcat(ret, buf);
				}
				break;
			case 'l':
				snprintf(buf, sizeof(buf), "%d", gib_list_length(filelist));
				strcat(ret, buf);
				break;
			case 'L':
				if (filelist_tmppath != NULL) {
					strcat(ret, filelist_tmppath);
				} else {
					filelist_tmppath = feh_unique_filename("/tmp/","filelist");
					feh_write_filelist(filelist, filelist_tmppath);
					strcat(ret, filelist_tmppath);
				}
				break;
			case 'm':
				strcat(ret, mode);
				break;
			case 'n':
				if (file)
					strcat(ret, file->name);
				break;
			case 'N':
				if (file)
					strcat(ret, shell_escape(file->name));
				break;
			case 'o':
				if (winwid) {
					snprintf(buf, sizeof(buf), "%d,%d", winwid->im_x,
						winwid->im_y);
					strcat(ret, buf);
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
			case 'r':
				if (winwid) {
					snprintf(buf, sizeof(buf), "%.1f", winwid->im_angle);
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
			case 't':
				if (file && (file->info || !feh_file_info_load(file, NULL))) {
					strcat(ret, file->info->format);
				}
				break;
			case 'u':
				snprintf(buf, sizeof(buf), "%d",
					 current_file != NULL ? gib_list_num(filelist, current_file)
					 + 1 : 0);
				strcat(ret, buf);
				break;
			case 'v':
				strcat(ret, VERSION);
				break;
			case 'V':
				snprintf(buf, sizeof(buf), "%d", getpid());
				strcat(ret, buf);
				break;
			case 'w':
				if (file && (file->info || !feh_file_info_load(file, NULL))) {
					snprintf(buf, sizeof(buf), "%d", file->info->width);
					strcat(ret, buf);
				}
				break;
			case 'z':
				if (winwid) {
					snprintf(buf, sizeof(buf), "%.2f", winwid->zoom);
					strcat(ret, buf);
				}
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
	if (filelist_tmppath != NULL)
		free(filelist_tmppath);
	return(ret);
}

void feh_filelist_image_remove(winwidget winwid, char do_delete)
{
	if (winwid->type == WIN_TYPE_SLIDESHOW) {
		char *s;
		gib_list *doomed;

		doomed = current_file;
		/*
		 * work around feh_list_jump exiting if cycle_once is enabled
		 * and no further files are left (we need to delete first)
		 */
		if (opt.cycle_once && ! doomed->next && do_delete) {
			feh_file_rm_and_free(filelist, doomed);
			exit(0);
		}
		slideshow_change_image(winwid, SLIDE_NEXT, 0);
		if (do_delete)
			filelist = feh_file_rm_and_free(filelist, doomed);
		else
			filelist = feh_file_remove_from_list(filelist, doomed);
		if (!filelist) {
			/* No more images. Game over ;-) */
			winwidget_destroy(winwid);
			return;
		}
		s = slideshow_create_name(FEH_FILE(winwid->file->data), winwid);
		winwidget_rename(winwid, s);
		free(s);
		winwidget_render_image(winwid, 1, 0);
	} else if ((winwid->type == WIN_TYPE_SINGLE)
		   || (winwid->type == WIN_TYPE_THUMBNAIL_VIEWER)) {
		if (do_delete)
			filelist = feh_file_rm_and_free(filelist, winwid->file);
		else
			filelist = feh_file_remove_from_list(filelist, winwid->file);
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

	ungib_imlib_save_image_with_error_return(win->im, tmpname, &err);

	if (err)
		feh_imlib_print_load_error(tmpname, win, err);

	free(tmpname);
	return;
}

gib_list *feh_list_jump(gib_list * root, gib_list * l, int direction, int num)
{
	int i;
	gib_list *ret = NULL;

	if (!root)
		return (NULL);
	if (!l)
		return (root);

	ret = l;

	for (i = 0; i < num; i++) {
		if (direction == FORWARD) {
			if (ret->next) {
				ret = ret->next;
			} else {
				if (opt.cycle_once) {
					exit(0);
				}
				ret = root;
			}
		} else {
			if (ret->prev)
				ret = ret->prev;
			else
				ret = gib_list_last(ret);
		}
	}
	return (ret);
}
