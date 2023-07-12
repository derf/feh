/* index.c

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
#include "options.h"
#include "index.h"


/* TODO Break this up a bit ;) */
/* TODO s/bit/lot */
void init_index_mode(void)
{
	Imlib_Load_Error err;
	Imlib_Image im_main;
	Imlib_Image im_temp;
	int w = 800, h = 600, ww = 0, hh = 0, www, hhh, xxx, yyy;
	int x = 0, y = 0;
	int bg_w = 0, bg_h = 0;
	winwidget winwid = NULL;
	Imlib_Image bg_im = NULL, im_thumb = NULL;
	int tot_thumb_h;
	int text_area_h = 50;
	int title_area_h = 0;
	Imlib_Font fn = NULL;
	Imlib_Font title_fn = NULL;
	int text_area_w = 0;
	int tw = 0, th = 0;
	int fw, fh;
	int vertical = 0;
	int max_column_w = 0;
	int thumbnailcount = 0;
	gib_list *l = NULL, *last = NULL;
	feh_file *file = NULL;
	int lineno;
	unsigned char trans_bg = 0;
	int index_image_width, index_image_height;
	gib_list *line, *lines;

	if (opt.montage) {
		mode = "montage";
	} else {
		mode = "index";
	}

	if (opt.font)
		fn = gib_imlib_load_font(opt.font);

	if (!fn)
		fn = gib_imlib_load_font(DEFAULT_FONT);

	if (opt.title_font) {

		title_fn = gib_imlib_load_font(opt.title_font);
		if (!title_fn)
			title_fn = gib_imlib_load_font(DEFAULT_FONT_TITLE);

		gib_imlib_get_text_size(title_fn, "W", NULL, &fw, &fh, IMLIB_TEXT_TO_RIGHT);
		title_area_h = fh + 4;
	} else
		title_fn = gib_imlib_load_font(DEFAULT_FONT_TITLE);

	if ((!fn) || (!title_fn))
		eprintf("Error loading fonts");

	/* Work out how tall the font is */
	gib_imlib_get_text_size(fn, "W", NULL, &tw, &th, IMLIB_TEXT_TO_RIGHT);
	get_index_string_dim(NULL, fn, &fw, &fh);
	/* For now, allow room for the right number of lines with small gaps */
	text_area_h = fh + 5;

	/* This includes the text area for index data */
	tot_thumb_h = opt.thumb_h + text_area_h;

	/* Use bg image dimensions for default size */
	if (opt.bg && opt.bg_file) {
		if (!strcmp(opt.bg_file, "trans"))
			trans_bg = 1;
		else {
			D(("Time to apply a background to blend onto\n"));
			if (feh_load_image_char(&bg_im, opt.bg_file) != 0) {
				bg_w = gib_imlib_image_get_width(bg_im);
				bg_h = gib_imlib_image_get_height(bg_im);
			}
		}
	}

	if (!opt.limit_w && !opt.limit_h) {
		if (bg_im) {
			opt.limit_w = bg_w;
			opt.limit_h = bg_h;
		} else
			opt.limit_w = 800;
	}

	/* Here we need to whiz through the files, and look at the filenames and
	   info in the selected font, work out how much space we need, and
	   calculate the size of the image we will require */

	if (opt.limit_w) {
		w = opt.limit_w;

		index_calculate_height(fn, w, &h, &tot_thumb_h);

		if (opt.limit_h) {
			if (h > opt.limit_h)
				weprintf(
					"The image size you specified (%dx%d) is not large\n"
					"enough to hold all %d thumbnails. To fit all the thumbnails,\n"
					"either decrease their size, choose a smaller font,\n"
					"or use a larger image (like %dx%d)",
					opt.limit_w, opt.limit_h, filelist_len, w, h);
			h = opt.limit_h;
		}
	} else if (opt.limit_h) {
		vertical = 1;
		h = opt.limit_h;

		index_calculate_width(fn, &w, h, &tot_thumb_h);
	}

	index_image_width = w;
	index_image_height = h + title_area_h;
	im_main = imlib_create_image(index_image_width, index_image_height);

	if (!im_main) {
		if (index_image_height >= 32768 || index_image_width >= 32768) {
			eprintf("Failed to create %dx%d pixels (%d MB) index image.\n"
					"This is probably due to Imlib2 issues when dealing with images larger than 32k x 32k pixels.",
					index_image_width, index_image_height, index_image_width * index_image_height * 4 / (1024*1024));
		} else {
			eprintf("Failed to create %dx%d pixels (%d MB) index image. Do you have enough RAM?",
					index_image_width, index_image_height, index_image_width * index_image_height * 4 / (1024*1024));
		}
	}

	if (bg_im)
		gib_imlib_blend_image_onto_image(im_main, bg_im,
						 gib_imlib_image_has_alpha
						 (bg_im), 0, 0, bg_w, bg_h, 0, 0, w, h, 1, 0, 0);
	else if (trans_bg) {
		gib_imlib_image_fill_rectangle(im_main, 0, 0, w, h + title_area_h, 0, 0, 0, 0);
		gib_imlib_image_set_has_alpha(im_main, 1);
	} else {
		/* Colour the background */
		gib_imlib_image_fill_rectangle(im_main, 0, 0, w, h + title_area_h, 0, 0, 0, 255);
	}

	if (opt.display) {
		winwid = winwidget_create_from_image(im_main, WIN_TYPE_SINGLE);
		winwidget_rename(winwid, PACKAGE " [index mode]");
		winwidget_show(winwid);
	}

	for (l = filelist; l; l = l->next) {
		file = FEH_FILE(l->data);
		if (last) {
			filelist = feh_file_remove_from_list(filelist, last);
			last = NULL;
		}
		D(("About to load image %s\n", file->filename));
		if (feh_load_image(&im_temp, file) != 0) {
			if (opt.verbose)
				feh_display_status('.');
			D(("Successfully loaded %s\n", file->filename));
			www = opt.thumb_w;
			hhh = opt.thumb_h;
			ww = gib_imlib_image_get_width(im_temp);
			hh = gib_imlib_image_get_height(im_temp);
			thumbnailcount++;

			if (opt.aspect) {
				double ratio = 0.0;

				/* Keep the aspect ratio for the thumbnail */
				ratio = ((double) ww / hh) / ((double) www / hhh);

				if (ratio > 1.0)
					hhh = opt.thumb_h / ratio;
				else if (ratio != 1.0)
					www = opt.thumb_w * ratio;
			}

			if ((!opt.stretch) && ((www > ww) || (hhh > hh))) {
				/* Don't make the image larger unless stretch is specified */
				www = ww;
				hhh = hh;
			}

			im_thumb = gib_imlib_create_cropped_scaled_image(im_temp, 0, 0, ww, hh, www, hhh, 1);
			gib_imlib_free_image_and_decache(im_temp);

			if (opt.alpha) {
				DATA8 atab[256];

				D(("Applying alpha options\n"));
				gib_imlib_image_set_has_alpha(im_thumb, 1);
				memset(atab, opt.alpha_level, sizeof(atab));
				gib_imlib_apply_color_modifier_to_rectangle
				    (im_thumb, 0, 0, www, hhh, NULL, NULL, NULL, atab);
			}

			text_area_w = opt.thumb_w;
			/* Now draw on the info text */
			if (opt.index_info) {
				get_index_string_dim(file, fn, &fw, &fh);
				if (fw > text_area_w)
					text_area_w = fw;
			}
			if (text_area_w > opt.thumb_w)
				text_area_w += 5;

			if (vertical) {
				if (text_area_w > max_column_w)
					max_column_w = text_area_w;
				if (y > h - tot_thumb_h) {
					y = 0;
					x += max_column_w;
					max_column_w = 0;
				}
				if (x > w - text_area_w)
					break;
			} else {
				if (x > w - text_area_w) {
					x = 0;
					y += tot_thumb_h;
				}
				if (y > h - tot_thumb_h)
					break;
			}

			/* center image relative to the text below it (if any) */
			xxx = x + ((text_area_w - www) / 2);
			yyy = y;

			if (opt.aspect)
				yyy += (opt.thumb_h - hhh) / 2;

			/* Draw now */
			gib_imlib_blend_image_onto_image(im_main, im_thumb,
							 gib_imlib_image_has_alpha
							 (im_thumb), 0, 0,
							 www, hhh, xxx,
							 yyy, www, hhh, 1, gib_imlib_image_has_alpha(im_thumb), 0);

			gib_imlib_free_image_and_decache(im_thumb);

			lineno = 0;
			if (opt.index_info) {
				line = lines = feh_wrap_string(create_index_string(file),
						opt.thumb_w * 3, fn, NULL);

				while (line) {
					gib_imlib_get_text_size(fn, (char *) line->data,
							NULL, &fw, &fh, IMLIB_TEXT_TO_RIGHT);
					gib_imlib_text_draw(im_main, fn, NULL,
							x + ((text_area_w - fw) >> 1),
							y + opt.thumb_h + (lineno++ * (th + 2)) + 2,
							(char *) line->data,
							IMLIB_TEXT_TO_RIGHT, 255, 255, 255, 255);
					line = line->next;
				}
				gib_list_free_and_data(lines);
			}

			if (vertical)
				y += tot_thumb_h;
			else
				x += text_area_w;

		} else {
			if (opt.verbose)
				feh_display_status('x');
			last = l;
		}
		if (opt.display) {
			winwidget_render_image(winwid, 0, 0);
			if (!feh_main_iteration(0))
				exit(0);
		}
	}
	if (opt.verbose)
		putc('\n', stderr);

	if (opt.title_font) {
		int fw, fh, fx, fy;
		char *s;

		s = create_index_title_string(thumbnailcount, w, h);
		gib_imlib_get_text_size(title_fn, s, NULL, &fw, &fh, IMLIB_TEXT_TO_RIGHT);
		fx = (index_image_width - fw) >> 1;
		fy = index_image_height - fh - 2;
		gib_imlib_text_draw(im_main, title_fn, NULL, fx, fy, s, IMLIB_TEXT_TO_RIGHT, 255, 255, 255, 255);

		if (opt.display)
			winwidget_render_image(winwid, 0, 0);
	}

	if (opt.output && opt.output_file) {
		char output_buf[1024];

		if (opt.output_dir)
			snprintf(output_buf, 1024, "%s/%s", opt.output_dir, opt.output_file);
		else {
			strncpy(output_buf, opt.output_file, 1023);
			output_buf[1023] = '\0';
		}

		gib_imlib_save_image_with_error_return(im_main, output_buf, &err);
		if (err) {
			feh_print_load_error(output_buf, im_main, err, LOAD_ERROR_IMLIB);
		}
		else if (opt.verbose) {
			int tw, th;

			tw = gib_imlib_image_get_width(im_main);
			th = gib_imlib_image_get_height(im_main);
			fprintf(stderr, PACKAGE " - File saved as %s\n", output_buf);
			fprintf(stderr,
				"    - Image is %dx%d pixels and contains %d thumbnails\n", tw, th, thumbnailcount);
		}
	}

	if (!opt.display)
		gib_imlib_free_image_and_decache(im_main);

	return;
}

void index_calculate_height(Imlib_Font fn, int w, int *h, int *tot_thumb_h)
{
	gib_list *l;
	feh_file *file = NULL;
	int x = 0, y = 0;
	int fw = 0, fh = 0;
	int text_area_w = 0, text_area_h = 0;

	for (l = filelist; l; l = l->next) {
		file = FEH_FILE(l->data);
		text_area_w = opt.thumb_w;
		if (opt.index_info) {
			get_index_string_dim(file, fn, &fw, &fh);
			if (fw > text_area_w)
				text_area_w = fw;
			if (fh > text_area_h) {
				text_area_h = fh + 5;
				*tot_thumb_h = opt.thumb_h + text_area_h;
			}
		}

		if (text_area_w > opt.thumb_w)
			text_area_w += 5;

		if ((x > w - text_area_w)) {
			x = 0;
			y += *tot_thumb_h;
		}

		x += text_area_w;
	}
	*h = y + *tot_thumb_h;
}

void index_calculate_width(Imlib_Font fn, int *w, int h, int *tot_thumb_h)
{
	gib_list *l;
	feh_file *file = NULL;
	int x = 0, y = 0;
	int fw = 0, fh = 0;
	int text_area_w = 0, text_area_h = 0;
	int max_column_w = 0;

	for (l = filelist; l; l = l->next) {
		file = FEH_FILE(l->data);
		text_area_w = opt.thumb_w;
		/* Calc width of text */
		if (opt.index_info) {
			get_index_string_dim(file, fn, &fw, &fh);
			if (fw > text_area_w)
				text_area_w = fw;
			if (fh > text_area_h) {
				text_area_h = fh + 5;
				*tot_thumb_h = opt.thumb_h + text_area_h;
			}
		}
		if (text_area_w > opt.thumb_w)
			text_area_w += 5;

		if (text_area_w > max_column_w)
			max_column_w = text_area_w;

		if ((y > h - *tot_thumb_h)) {
			y = 0;
			x += max_column_w;
			max_column_w = 0;
		}

		y += *tot_thumb_h;
	}
	*w = x + text_area_w;
}

void get_index_string_dim(feh_file *file, Imlib_Font fn, int *fw, int *fh)
{
	int line_w, line_h;
	char fake_file = 0;
	gib_list *line, *lines;
	int max_w = 0, total_h = 0;

	if (!opt.index_info) {
		*fw = 0;
		*fh = 0;
		return;
	}

	/* called with file = NULL in the setup phase.
	 * We need a fake file, otherwise feh_printf will remove format specifiers,
	 * leading e.g. to a 0x0 report for index_dim = "%n".
	 */
	if (file == NULL) {
		fake_file = 1;
		file = feh_file_new("foo");
		file->info = feh_file_info_new();
	}

	line = lines = feh_wrap_string(create_index_string(file), opt.thumb_w * 3, fn, NULL);

	while (line) {
		gib_imlib_get_text_size(fn, (char *) line->data,
			NULL, &line_w, &line_h, IMLIB_TEXT_TO_RIGHT);

		if (line_w > max_w)
			max_w = line_w;
		total_h += line_h + 2;

		line = line->next;
	}

	gib_list_free_and_data(lines);
	if (fake_file)
		feh_file_free(file);

	*fw = max_w;
	*fh = total_h;
	return;
}

char *create_index_string(feh_file * file)
{
	return feh_printf(opt.index_info, file, NULL);
}

char *create_index_title_string(int num, int w, int h)
{
	static char str[50];

	snprintf(str, sizeof(str), PACKAGE " index - %d thumbnails, %d by %d pixels", num, w, h);
	return(str);
}
