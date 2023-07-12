/* thumbnail.h

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

#ifndef THIMBNAIL_H
#define THIMBNAIL_H

#include "feh.h"
#include "filelist.h"
#include "winwidget.h"

#define FEH_THUMB(l) ((feh_thumbnail *) l)

typedef struct thumbnail {
	int x;
	int y;
	int w;
	int h;
	feh_file *file;
	unsigned char exists;
	struct feh_thumbnail *next;
} feh_thumbnail;

typedef struct thumbmode_data {
	Imlib_Image im_main;     /* base image which all thumbnails are rendered on */
	Imlib_Image im_bg;       /* background for the thumbnails */

	Imlib_Font font_main;    /* font used for file info */
	Imlib_Font font_title;   /* font used for title */

	int w, h, bg_w, bg_h;    /* dimensions of the window and bg image */

	int thumb_tot_h;         /* total space needed for a thumbnail including description */
	int text_area_w, text_area_h; /* space needed for thumbnail description */

	int max_column_w;        /* FIXME: description */
	int vertical;            /* == !opt.limit_w && opt.limit_h */

	int cache_thumbnails;    /* use cached thumbnails from ~/.thumbnails */
	int cache_dim;           /* 128 = 128x128 ("normal"), 256 = 256x256 ("large") */
	char *cache_dir;         /* "normal"/"large" (.thumbnails/...) */
	feh_thumbnail *selected;     /* currently selected thumbnail */

} thumbmode_data;

feh_thumbnail *feh_thumbnail_new(feh_file * fil, int x, int y, int w, int h);
feh_file *feh_thumbnail_get_file_from_coords(int x, int y);
feh_thumbnail *feh_thumbnail_get_thumbnail_from_coords(int x, int y);
feh_thumbnail *feh_thumbnail_get_from_file(feh_file * file);
void feh_thumbnail_mark_removed(feh_file * file, int deleted);

void feh_thumbnail_calculate_geometry(void);

int feh_thumbnail_get_thumbnail(Imlib_Image * image, feh_file * file, int * orig_w, int * orig_h);
int feh_thumbnail_generate(Imlib_Image * image, feh_file * file, char *thumb_file, char *uri, int * orig_w, int * orig_h);
int feh_thumbnail_get_generated(Imlib_Image * image, feh_file * file, char * thumb_file, int * orig_w, int * orig_h);
char *feh_thumbnail_get_name(char *uri);
char *feh_thumbnail_get_name_uri(char *name);
char *feh_thumbnail_get_name_md5(char *uri);
void feh_thumbnail_show_fullsize(feh_file *thumbfile);
void feh_thumbnail_select(winwidget winwid, feh_thumbnail *thumbnail);
void feh_thumbnail_select_next(winwidget winwid, int jump);
void feh_thumbnail_select_prev(winwidget winwid, int jump);
void feh_thumbnail_show_selected(void);
feh_file *feh_thumbnail_get_selected_file(void);

int feh_thumbnail_setup_thumbnail_dir(void);

#endif
