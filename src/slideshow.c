/* slideshow.c

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
#include "timers.h"
#include "winwidget.h"
#include "options.h"
#include "signals.h"

void init_slideshow_mode(void)
{
	winwidget w = NULL;
	int success = 0;
	gib_list *l = filelist, *last = NULL;

	/*
	 * In theory, --start-at FILENAME is simple: Look for a file called
	 * FILENAME, start the filelist there, done.
	 *
	 * In practice, there are cases where this isn't sufficient. For instance,
	 * a user running 'feh --start-at hello.jpg /tmp' will expect feh to start
	 * at /tmp/hello.jpg, as if they had used
	 * 'feh --start-at /tmp/hello.jpg /tmp'. Similarly, XDG Desktop files
	 * may lead to the invocation 'feh --start-at /tmp/hello.jpg .' in /tmp,
	 * expecting the behaviour of 'feh --start-at ./hello.jpg .'.
	 *
	 * Since a good user experience is not about being technically correct, but
	 * about delivering the expected behaviour, we do some fuzzy matching
	 * here. In the worst case, this will cause --start-at to start at the
	 * wrong file.
	 */

	// Try finding an exact filename match first
	for (; l && opt.start_list_at; l = l->next) {
		if (!strcmp(opt.start_list_at, FEH_FILE(l->data)->filename)) {
			free(opt.start_list_at);
			opt.start_list_at = NULL;
			break;
		}
	}

	/*
	 * If it didn't work (opt.start_list_at is still set): Fall back to
	 * comparing just the filenames without directory prefixes. This may lead
	 * to false positives, but for now that's just the way it is.
	 */
	if (opt.start_list_at) {
		char *current_filename;
		char *start_at_filename = strrchr(opt.start_list_at, '/');
		if (start_at_filename) {
			start_at_filename++; // We only care about the part after the '/'
		} else {
			start_at_filename = opt.start_list_at;
		}
		for (l = filelist; l && opt.start_list_at; l = l->next) {
			current_filename = strrchr(FEH_FILE(l->data)->filename, '/');
			if (current_filename) {
				current_filename++; // We only care about the part after the '/'
			} else {
				current_filename = FEH_FILE(l->data)->filename;
			}
			if (!strcmp(start_at_filename, current_filename)) {
				free(opt.start_list_at);
				opt.start_list_at = NULL;
				break;
			}
		}
	}

	// If that didn't work either, we're out of luck.
	if (opt.start_list_at)
		eprintf("--start-at %s: File not found in filelist",
				opt.start_list_at);

	if (!opt.title)
		opt.title = PACKAGE " [%u of %l] - %f";

	mode = "slideshow";
	for (; l; l = l->next) {
		if (last) {
			filelist = feh_file_remove_from_list(filelist, last);
			last = NULL;
		}
		current_file = l;
		if ((w = winwidget_create_from_file(l, WIN_TYPE_SLIDESHOW)) != NULL) {
			success = 1;
			winwidget_show(w);
			if (opt.slideshow_delay > 0.0)
				feh_add_timer(cb_slide_timer, w, opt.slideshow_delay, "SLIDE_CHANGE");
			if (opt.reload > 0)
				feh_add_unique_timer(cb_reload_timer, w, opt.reload);
			break;
		} else {
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

	/*
	 * multi-window mode has no concept of a "current file" and
	 * dynamically adding/removing windows is not implemented at the moment.
	 * So don't reload filelists in multi-window mode.
	 */
	if (current_file != NULL) {
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

		if (opt.filelistfile) {
			filelist = gib_list_cat(filelist, feh_read_filelist(opt.filelistfile));
		}

		if (!(filelist_len = gib_list_length(filelist))) {
			eprintf("No files found to reload.");
		}

		feh_prepare_filelist();

		/* find the previously current file */
		for (l = filelist; l; l = l->next)
			if (strcmp(FEH_FILE(l->data)->filename, current_filename) == 0) {
				current_file = l;
				break;
			}

		free(current_filename);

		if (!current_file)
			current_file = filelist;
		w->file = current_file;
	}

	feh_reload_image(w, 1, 0);
	feh_add_unique_timer(cb_reload_timer, w, opt.reload);
	return;
}

void slideshow_change_image(winwidget winwid, int change, int render)
{
	gib_list *last = NULL;
	gib_list *previous_file = current_file;
	int i = 0;
	int jmp = 1;
	/* We can't use filelist_len in the for loop, since that changes when we
	 * encounter invalid images.
	 */
	int our_filelist_len = filelist_len;

	if (opt.slideshow_delay > 0.0)
		feh_add_timer(cb_slide_timer, winwid, opt.slideshow_delay, "SLIDE_CHANGE");

	/* Without this, clicking a one-image slideshow reloads it. Not very *
	   intelligent behaviour :-) */
	if (filelist_len < 2 && opt.on_last_slide != ON_LAST_SLIDE_QUIT)
		return;

	/* Ok. I do this in such an odd way to ensure that if the last or first *
	   image is not loadable, it will go through in the right direction to *
	   find the correct one. Otherwise SLIDE_LAST would try the last file, *
	   then loop forward to find a loadable one. */
	if (change == SLIDE_FIRST) {
		current_file = gib_list_last(filelist);
		change = SLIDE_NEXT;
		previous_file = NULL;
	} else if (change == SLIDE_LAST) {
		current_file = filelist;
		change = SLIDE_PREV;
		previous_file = NULL;
	}

	/* The for loop prevents us looping infinitely */
	for (i = 0; i < our_filelist_len; i++) {
		winwidget_free_image(winwid);
#ifdef HAVE_LIBEXIF
		/*
		 * An EXIF data chunk requires up to 50 kB of space. For large and
		 * long-running slideshows, this would acculumate gigabytes of
		 * EXIF data after a few days. We therefore do not cache EXIF data
		 * in slideshows.
		 */
		if (FEH_FILE(winwid->file->data)->ed) {
			exif_data_unref(FEH_FILE(winwid->file->data)->ed);
			FEH_FILE(winwid->file->data)->ed = NULL;
		}
#endif
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
					(random() % (filelist_len - 1)) + 1);
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
		case SLIDE_JUMP_NEXT_DIR:
			{
				char old_dir[PATH_MAX], new_dir[PATH_MAX];
				int j;

				feh_file_dirname(old_dir, FEH_FILE(current_file->data), PATH_MAX);

				for (j = 0; j < our_filelist_len; j++) {
					current_file = feh_list_jump(filelist, current_file, FORWARD, 1);
					feh_file_dirname(new_dir, FEH_FILE(current_file->data), PATH_MAX);
					if (strcmp(old_dir, new_dir) != 0)
						break;
				}
			}
			change = SLIDE_NEXT;
			break;
		case SLIDE_JUMP_PREV_DIR:
			{
				char old_dir[PATH_MAX], new_dir[PATH_MAX];
				int j;

				/* Start the search from the previous file in case we are on
				   the first file of a directory */
				current_file = feh_list_jump(filelist, current_file, BACK, 1);
				feh_file_dirname(old_dir, FEH_FILE(current_file->data), PATH_MAX);

				for (j = 0; j < our_filelist_len; j++) {
					current_file = feh_list_jump(filelist, current_file, BACK, 1);
					feh_file_dirname(new_dir, FEH_FILE(current_file->data), PATH_MAX);
					if (strcmp(old_dir, new_dir) != 0)
						break;
				}

				/* Next file is the first entry of prev_dir */
				current_file = feh_list_jump(filelist, current_file, FORWARD, 1);
			}
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

		if (opt.on_last_slide == ON_LAST_SLIDE_HOLD && previous_file &&
			((current_file == filelist && change == SLIDE_NEXT) ||
			(previous_file == filelist && change == SLIDE_PREV))) {
				current_file = previous_file;
		}

		if (winwidget_loadimage(winwid, FEH_FILE(current_file->data))) {
			int w = gib_imlib_image_get_width(winwid->im);
			int h = gib_imlib_image_get_height(winwid->im);
			if (feh_should_ignore_image(winwid->im)) {
				last = current_file;
				continue;
			}
			winwid->mode = MODE_NORMAL;
			winwid->file = current_file;
			if ((winwid->im_w != w) || (winwid->im_h != h))
				winwid->had_resize = 1;
			winwidget_reset_image(winwid);
			winwid->im_w = w;
			winwid->im_h = h;
			if (render) {
				winwidget_render_image(winwid, 1, 0);
			}
			break;
		} else
			last = current_file;
	}
	if (last)
		filelist = feh_file_remove_from_list(filelist, last);

	if (filelist_len == 0)
		eprintf("No more slides in show");

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

void feh_action_run(feh_file * file, char *action, winwidget winwid)
{
	if (action) {
		char *sys;
		D(("Running action %s\n", action));
		sys = feh_printf(action, file, winwid);

		if (opt.verbose && !opt.list && !opt.customlist)
			fprintf(stderr, "Running action -->%s<--\n", sys);
		if (system(sys) == -1)
			perror("running action via system() failed");
	}
	return;
}

char *format_size(double size)
{
	static char ret[5];
	char units[] = {' ', 'k', 'M', 'G', 'T'};
	unsigned char postfix = 0;
	while (size >= 1000) {
		size /= 1000;
		postfix++;
	}
	snprintf(ret, 5, "%3.0f%c", size, units[postfix]);
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
	gib_list *f;

	for (c = str; *c != '\0'; c++) {
		if ((*c == '%') && (*(c+1) != '\0')) {
			c++;
			switch (*c) {
			case 'a':
				if (opt.paused == 1) {
				   strncat(ret, "paused", sizeof(ret) - strlen(ret) - 1);
				}
				else {
				   strncat(ret, "playing", sizeof(ret) - strlen(ret) - 1);
				}
				break;
			case 'f':
				if (file)
					strncat(ret, file->filename, sizeof(ret) - strlen(ret) - 1);
				break;
			case 'F':
				if (file)
					strncat(ret, shell_escape(file->filename), sizeof(ret) - strlen(ret) - 1);
				break;
			case 'g':
				if (winwid) {
					snprintf(buf, sizeof(buf), "%d,%d", winwid->w, winwid->h);
					strncat(ret, buf, sizeof(ret) - strlen(ret) - 1);
				}
				break;
			case 'h':
				if (file && (file->info || !feh_file_info_load(file, NULL))) {
					snprintf(buf, sizeof(buf), "%d", file->info->height);
					strncat(ret, buf, sizeof(ret) - strlen(ret) - 1);
				}
				break;
			case 'l':
				snprintf(buf, sizeof(buf), "%d", gib_list_length(filelist));
				strncat(ret, buf, sizeof(ret) - strlen(ret) - 1);
				break;
			case 'L':
				if (filelist_tmppath != NULL) {
					strncat(ret, filelist_tmppath, sizeof(ret) - strlen(ret) - 1);
				} else {
					filelist_tmppath = feh_unique_filename("/tmp/","filelist");
					feh_write_filelist(filelist, filelist_tmppath);
					strncat(ret, filelist_tmppath, sizeof(ret) - strlen(ret) - 1);
				}
				break;
			case 'm':
				strncat(ret, mode, sizeof(ret) - strlen(ret) - 1);
				break;
			case 'n':
				if (file)
					strncat(ret, file->name, sizeof(ret) - strlen(ret) - 1);
				break;
			case 'N':
				if (file)
					strncat(ret, shell_escape(file->name), sizeof(ret) - strlen(ret) - 1);
				break;
			case 'o':
				if (winwid) {
					snprintf(buf, sizeof(buf), "%d,%d", winwid->im_x,
						winwid->im_y);
					strncat(ret, buf, sizeof(ret) - strlen(ret) - 1);
				}
				break;
			case 'p':
				if (file && (file->info || !feh_file_info_load(file, NULL))) {
					snprintf(buf, sizeof(buf), "%d", file->info->pixels);
					strncat(ret, buf, sizeof(ret) - strlen(ret) - 1);
				}
				break;
			case 'P':
				if (file && (file->info || !feh_file_info_load(file, NULL))) {
					strncat(ret, format_size(file->info->pixels), sizeof(ret) - strlen(ret) - 1);
				}
				break;
			case 'r':
				if (winwid) {
					snprintf(buf, sizeof(buf), "%.1f", winwid->im_angle);
					strncat(ret, buf, sizeof(ret) - strlen(ret) - 1);
				}
				break;
			case 's':
				if (file && (file->size >= 0 || !feh_file_stat(file))) {
					snprintf(buf, sizeof(buf), "%d", file->size);
					strncat(ret, buf, sizeof(ret) - strlen(ret) - 1);
				}
				break;
			case 'S':
				if (file && (file->size >= 0 || !feh_file_stat(file))) {
					strncat(ret, format_size(file->size), sizeof(ret) - strlen(ret) - 1);
				}
				break;
			case 't':
				if (file && (file->info || !feh_file_info_load(file, NULL))) {
					strncat(ret, file->info->format, sizeof(ret) - strlen(ret) - 1);
				}
				break;
			case 'u':
				f = current_file ? current_file : gib_list_find_by_data(filelist, file);
				snprintf(buf, sizeof(buf), "%d", f ? gib_list_num(filelist, f) + 1 : 0);
				strncat(ret, buf, sizeof(ret) - strlen(ret) - 1);
				break;
			case 'v':
				strncat(ret, VERSION, sizeof(ret) - strlen(ret) - 1);
				break;
			case 'V':
				snprintf(buf, sizeof(buf), "%d", getpid());
				strncat(ret, buf, sizeof(ret) - strlen(ret) - 1);
				break;
			case 'w':
				if (file && (file->info || !feh_file_info_load(file, NULL))) {
					snprintf(buf, sizeof(buf), "%d", file->info->width);
					strncat(ret, buf, sizeof(ret) - strlen(ret) - 1);
				}
				break;
			case 'W':
				if (winwid) {
					snprintf(buf, sizeof(buf), "%dx%d+%d+%d", winwid->w, winwid->h, winwid->x, winwid->y);
					strncat(ret, buf, sizeof(ret) - strlen(ret) - 1);
				}
				break;
			case 'z':
				if (winwid) {
					snprintf(buf, sizeof(buf), "%.2f", winwid->zoom);
					strncat(ret, buf, sizeof(ret) - strlen(ret) - 1);
				} else {
					strncat(ret, "1.00", sizeof(ret) - strlen(ret) - 1);
				}
				break;
			case 'Z':
				if (winwid) {
					snprintf(buf, sizeof(buf), "%f", winwid->zoom);
					strncat(ret, buf, sizeof(ret) - strlen(ret) - 1);
				}
				break;
			case '%':
				strncat(ret, "%", sizeof(ret) - strlen(ret) - 1);
				break;
			default:
				weprintf("Unrecognized format specifier %%%c", *c);
				if ((strlen(ret) + 3) < sizeof(ret))
					strncat(ret, c - 1, 2);
				break;
			}
		} else if ((*c == '\\') && (*(c+1) != '\0') && ((strlen(ret) + 3) < sizeof(ret))) {
			c++;
			switch (*c) {
			case 'n':
				strcat(ret, "\n");
				break;
			default:
				strncat(ret, c - 1, 2);
				break;
			}
		} else if ((strlen(ret) + 2) < sizeof(ret))
			strncat(ret, c, 1);
	}
	if (filelist_tmppath != NULL)
		free(filelist_tmppath);
	return(ret);
}

void feh_filelist_image_remove(winwidget winwid, char do_delete)
{
	if (winwid->type == WIN_TYPE_SLIDESHOW) {
		gib_list *doomed;

		doomed = current_file;
		/*
		 * work around feh_list_jump exiting if ON_LAST_SLIDE_QUIT is set
		 * and no further files are left (we need to delete first)
		 */
		if (opt.on_last_slide == ON_LAST_SLIDE_QUIT && ! doomed->next && do_delete) {
			feh_file_rm_and_free(filelist, doomed);
			exit(0);
		}
		if (doomed->next) {
			slideshow_change_image(winwid, SLIDE_NEXT, 0);
		}
		else {
			slideshow_change_image(winwid, SLIDE_PREV, 0);
		}
		if (do_delete)
			filelist = feh_file_rm_and_free(filelist, doomed);
		else
			filelist = feh_file_remove_from_list(filelist, doomed);
		if (!filelist) {
			/* No more images. Game over ;-) */
			winwidget_destroy(winwid);
			return;
		}
		winwidget_render_image(winwid, 1, 0);
	} else if ((winwid->type == WIN_TYPE_SINGLE)
		   || (winwid->type == WIN_TYPE_THUMBNAIL_VIEWER)) {
		if (do_delete)
			filelist = feh_file_rm_and_free(filelist, winwid->file);
		else
			filelist = feh_file_remove_from_list(filelist, winwid->file);
		winwid->file = NULL;
		winwidget_destroy(winwid);
	}
}

void slideshow_save_image(winwidget win)
{
	char *tmpname;
	Imlib_Load_Error err;
	char *base_dir = "";
	if (opt.output_dir) {
		base_dir = estrjoin("", opt.output_dir, "/", NULL);
	}

	if (win->file) {
		tmpname = feh_unique_filename(base_dir, FEH_FILE(win->file->data)->name);
	} else if (mode) {
		char *tmp;
		tmp = estrjoin(".", mode, "png", NULL);
		tmpname = feh_unique_filename(base_dir, tmp);
		free(tmp);
	} else {
		tmpname = feh_unique_filename(base_dir, "noname.png");
	}

	if (opt.output_dir) {
		free(base_dir);
	}

	if (opt.verbose)
		fprintf(stderr, "saving image to filename '%s'\n", tmpname);

	gib_imlib_save_image_with_error_return(win->im, tmpname, &err);

	if (err)
		feh_print_load_error(tmpname, win, err, LOAD_ERROR_IMLIB);

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
				if (opt.on_last_slide == ON_LAST_SLIDE_QUIT) {
					exit(0);
				}
				if (opt.randomize) {
					/* Randomize the filename order */
					filelist = gib_list_randomize(filelist);
					ret = filelist;
				} else {
					ret = root;
				}
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
