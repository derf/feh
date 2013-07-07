/* index.c

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


/* void init_index_mode(void)
 * Mar 28, 2013 HRABAK combined with init_thumbnail_mode()
 */

void index_calculate_height( LLMD *md, int w, int *h, int *tot_thumb_h)
{
	int x = 0, y = 0;
	int fw = 0, fh = 0;
	int text_area_w = 0, text_area_h = 0;

	for (md->tn = md->rn->next ;  md->tn != md->rn ; md->tn = md->tn->next) {
		text_area_w = opt.thumb_w;
		if (opt.index_info) {
			get_index_string_dim(NODE_DATA(md->tn), &fw, &fh);
			if (fw > text_area_w)
				text_area_w = fw;
			if (fh > text_area_h) {
				text_area_h = fh + 5;      /* between-row padding?  Has no effect */
				*tot_thumb_h = opt.thumb_h + text_area_h;
			}
		}

		if (text_area_w > opt.thumb_w)
			text_area_w += 5;            /* this is hz padding */

		if ((x > w - text_area_w)) {   /* need to add a new row */
			x = 0;
			y += *tot_thumb_h;
		}

		x += text_area_w;
	}
	*h = y + *tot_thumb_h;
}

void index_calculate_width( LLMD *md, int *w, int h, int *tot_thumb_h)
{
	int x = 0, y = 0;
	int fw = 0, fh = 0;
	int text_area_w = 0, text_area_h = 0;
	int max_column_w = 0;

	for ( md->tn = md->rn->next ;  md->tn != md->rn ; md->tn = md->tn->next) {
		text_area_w = opt.thumb_w;
		/* Calc width of text */
		if (opt.index_info) {
			get_index_string_dim(NODE_DATA(md->tn), &fw, &fh);
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

void get_index_string_dim(feh_data *data,  int *fw, int *fh){
		/* nobody calls this UNLESS opt.index_info != NULL, so that check was
		 * removed. Prior to Mar 2013 this routine created a bogus
		 * file = feh_ll_new_data("foo").  That kludge has been eliminated.
		 */


	static ld *alp;                  /* alp stands for Array(of)LinePointers */

	*fw = *fh = 0;

	alp = feh_wrap_string(feh_printf(opt.index_info, data), NULL, opt.thumb_w * 3);

  if ( alp[0].L0.tot_lines ){
			*fw = alp[0].L0.maxwide;
			*fh = alp[0].L0.tothigh + ( 2 * alp[0].L0.tot_lines );
	}

	return;

}


void create_index_title_string(winwidget w, thumbmode_data td){
		/* Apr 2013 HRABAK moved this code out of init_thumbnail_mode */

	int fw, fh, fx, fy;
	char *str = mobs(1);
	feh_font *old_fn_ptr = opt.fn_ptr;            /* to restore after the call  */

	opt.fn_ptr = &opt.fn_title;

	sprintf(str, PACKAGE " index - %d thumbnails, %d by %d pixels",
	                       td.thumbnailcount, td.wide, td.high);

	gib_imlib_get_text_size( str, NULL, &fw, &fh, IMLIB_TEXT_TO_RIGHT);
	fx = (td.wide - fw) >> 1;
	fy = td.high - fh - 2;
	feh_imlib_text_draw(td.im_main, &opt.style[ STYLE_WHITE ],
		                  fx, fy, str, IMLIB_TEXT_TO_RIGHT );

	opt.fn_ptr = old_fn_ptr;
	if (opt.flg.display)
		winwidget_render_image(w, 0, 1, SANITIZE_NO);

}

/* Jul 1, 2012 Hrabak moved init_multiwindow_mode() here, as it was
 * the only member of the multiwindow.c.
 */

void init_multiwindow_mode(LLMD *md)
{
	winwidget w = NULL;
	char *s = mobs(2);

	for (md->cn = md->rn->next ;  md->cn != md->rn ; md->cn = md->cn->next) {
		if (opt.title)
			s = feh_printf(opt.title, NODE_DATA(md->cn));
		else
			STRCAT_2ITEMS(s, PACKAGE " - ", NODE_FILENAME(md->cn));

		if ((w = winwidget_create_from_file(md, md->cn, s, WIN_TYPE_SINGLE)) != NULL) {
			winwidget_show(w);
			if (!feh_main_iteration(0))
				exit(0);
		} else {
			D(("EEEK. Couldn't load image in multiwindow mode. "
						"I 'm not sure if this is a problem\n"));
		}

	}

	return;
}
