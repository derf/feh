/* thumbnail.c

Copyright (C) 1999-2003 Tom Gilbert.
Copyright (C) 2010-2011 Daniel Friesel.
Copyright (C) 2012-2013 Christopher Hrabak  bhs_hash() stuff


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
#include "thumbnail.h"
#include "md5.h"
#include "feh_ll.h"


static thumbmode_data td;

/* TODO Break this up a bit ;) */
/* TODO s/bit/bit more */

void init_thumbnail_mode( LLMD *md, int mode ){
		/* HRABAK  Mar 27, 2013 thumbnail display is just a diff view of the
		 * feh_md LL, so you can toggle between display modes on the fly.
		 * This routine is for thumbnail and index displays.  And because it now
		 * accepts a LLMD *md, I could use this to display the list of pix selected
		 * during a move_mode.  See newkeyev.c
		 */

	Imlib_Image im_thumb = NULL;
	Imlib_Image im_temp;
	int ww = 0, hh = 0, www, hhh, xxx, yyy;
	int x = 0, y = 0;
	int orig_w, orig_h;
	winwidget w = NULL;
	int tw = 0, th = 0;
	int fw, fh;
	int lineno;
	unsigned int thumb_counter = 0;

	static char *start, *end;        /* substr markers in alp[] */
	static char last1;               /* save the final char */
	static ld *alp;                  /* alp stands for Array(of)LinePointers */
	int i = 0;                       /* index to walk thru alp[] array */

	/* initialize thumbnail mode data */
	td.im_main   = NULL;
	td.im_bg     = NULL;

	td.wide = 800;         /* thumb_mode had 640 */
	td.high = 600;         /* thumb_mode had 480*/
	td.bg_w = 0;
	td.bg_h = 0;
	td.thumb_tot_h = 0;
	td.text_area_w = 0;
	td.text_area_h = 0;    /* index has =50 */

	td.vertical = 0;
	td.max_column_w = 0;
	td.gutter = 5;         /* spacing between thumbs */
	td.trans_bg = 0;
	td.thumbnailcount = 0;

	/* index and thumbnail modes handled here */
	td.type = (( mode == MODE_INDEX ) ? WIN_TYPE_SINGLE : WIN_TYPE_THUMBNAIL);
	opt.flg.mode = mode ;

	/* Use the default settings for this font */
	tw = opt.fn_dflt.wide;  th = opt.fn_dflt.high;

	if (opt.index_info)
		get_index_string_dim( NODE_DATA(md->rn->next ), &fw, &fh);
	else {
		fw = opt.fn_dflt.wide;
		fh = 0;
	}
	td.text_area_h = fh + td.gutter;

	/* This includes the text area for index data */
	td.thumb_tot_h = opt.thumb_h + td.text_area_h;

	/* Use bg image dimensions for default size */
	if (opt.flg.bg && opt.bg_file) {
		if (!strcmp(opt.bg_file, "trans"))
			td.trans_bg = 1;
		else {
			D(("Time to apply a background to blend onto\n"));
			if (feh_load_image_char(&td.im_bg, opt.bg_file) != 0) {
				td.bg_w = gib_imlib_image_get_width(td.im_bg);
				td.bg_h = gib_imlib_image_get_height(td.im_bg);
			}
		}
	}

	/* calc size and create im_main for the main window and entries */
	feh_thumbnail_create_im_main( &w , md );  /* sets wide/high */

	/* we need ~/.thumbnails/normal dir for storing permanent thumbnails */
	if ( ( mode == MODE_THUMBNAIL ) && opt.flg.cache_thumbnails)
			feh_thumbnail_setup_thumbnail_dir();

	for (md->cn = md->rn->next ;  md->cn != md->rn ; md->cn = md->cn->next) {
		D(("About to load image %s\n", NODE_FILENAME( md->cn) ));
		if (feh_thumbnail_get_thumbnail(&im_temp, md->cn, &orig_w, &orig_h) != 0) {
			if (opt.flg.verbose)
				feh_display_status('.');
			D(("Successfully loaded %s\n", NODE_FILENAME( md->cn)));
			www = opt.thumb_w;
			hhh = opt.thumb_h;
			ww = gib_imlib_image_get_width(im_temp);
			hh = gib_imlib_image_get_height(im_temp);

			if (!orig_w) {
				orig_w = ww;
				orig_h = hh;
			}

			td.thumbnailcount++;
			imlib_context_set_blend(gib_imlib_image_has_alpha(im_temp)?1:0);

			if (opt.flg.aspect) {
				double ratio = 0.0;

				/* Keep the aspect ratio for the thumbnail */
				ratio = ((double) ww / hh) / ((double) www / hhh);

				if (ratio > 1.0)
					hhh = opt.thumb_h / ratio;
				else if (ratio != 1.0)
					www = opt.thumb_w * ratio;
			}

			if ((!opt.flg.stretch) && ((www > ww) || (hhh > hh))) {
				/* Don't make the image larger unless stretch is specified */
				www = ww;
				hhh = hh;
			}

			im_thumb = gib_imlib_create_cropped_scaled_image
			               (im_temp, 0, 0, ww, hh, www, hhh, 1);
			gib_imlib_free_image_and_decache(im_temp);

			if (opt.flg.alpha) {
				DATA8 atab[256];

				D(("Applying alpha options\n"));
				gib_imlib_image_set_has_alpha(im_thumb, 1);
				memset(atab, opt.alpha_level, sizeof(atab));
				gib_imlib_apply_color_modifier_to_rectangle
				    (im_thumb, 0, 0, www, hhh, NULL, NULL, NULL, atab);
			}

			td.text_area_w = opt.thumb_w;
			/* Now draw on the info text */
			if (opt.index_info) {
				get_index_string_dim(NODE_DATA( md->cn), &fw, &fh);
				if (fw > td.text_area_w)
					td.text_area_w = fw;
				if (fh > td.text_area_h) {
					td.text_area_h = fh + td.gutter;
					td.thumb_tot_h = opt.thumb_h + td.text_area_h;
				}
			}
			if (td.text_area_w > opt.thumb_w)
				td.text_area_w += td.gutter;

			if (td.vertical) {
				if (td.text_area_w > td.max_column_w)
					td.max_column_w = td.text_area_w;
				if (y > td.high - td.thumb_tot_h) {
					y = 0;
					x += td.max_column_w;
					td.max_column_w = 0;
				}
				if (x > td.wide - td.text_area_w)
					break;
			} else {
				if (x > td.wide - td.text_area_w) {
					x = 0;
					y += td.thumb_tot_h;
				}
				if (y > td.high - td.thumb_tot_h)
					break;
			}

			/* center image relative to the text below it (if any) */
			xxx = x + ((td.text_area_w - www) / 2);
			yyy = y;

			if (opt.flg.aspect)
				yyy += (opt.thumb_h - hhh) / 2;

			/* Draw now.  Reuse lineno to hold has_alpha setting */
			lineno = gib_imlib_image_has_alpha(im_thumb);
			gib_imlib_blend_image_onto_image(td.im_main,
					      im_thumb, lineno, 0, 0,
					      www, hhh, xxx,
					      yyy, www, hhh, 1, lineno, 0);
			gib_imlib_free_image_and_decache(im_thumb);

			/* no more sep thumbnail list.  All thumb data now inside md */
			md->cn->nd.x    = xxx;
			md->cn->nd.y    = yyy;
			md->cn->nd.wide = www;
			md->cn->nd.high = hhh;
			md->cn->nd.exists = 1;

			lineno = 0;
			if (opt.index_info) {
					alp = feh_wrap_string( feh_printf(opt.index_info, NODE_DATA( md->cn)),
					                       NULL, opt.thumb_w * 3);

					for (i=1; i<= alp[0].L0.tot_lines ; i++ ) {
							fw = alp[i].L1.wide;
							fh = alp[i].L1.high;
							start = alp[i].L1.line;
							end   = alp[i].L1.line + alp[i].L1.len;
							/* null term this substring b4 the call ...*/
							last1 = end[0];  end[0]   = '\0';

							feh_imlib_text_draw(td.im_main, &opt.style[ STYLE_WHITE ],
							                 x + ((td.text_area_w - fw) >> 1),
							                 y + opt.thumb_h + (lineno++ * (th + 2)) + 2,
							                 start, IMLIB_TEXT_TO_RIGHT );
							/* ... then restore that last char afterwards */
							end[0] = last1;
					}
			}

			if (td.vertical)
				y += td.thumb_tot_h;
			else
				x += td.text_area_w;
		} else {
			if (opt.flg.verbose)
				feh_display_status('x');
			/* remove the feh_md->cn */
			feh_move_node_to_remove_list(NULL, DELETE_NO, WIN_MAINT_NO );
		}

		if (opt.flg.display) {
			/* thumb_counter is unsigned, so no need to catch overflows */
			if (++thumb_counter == opt.thumb_redraw) {
				winwidget_render_image(w, 0, 1, SANITIZE_NO);
				thumb_counter = 0;
				w->errstr = mobs(1);
				sprintf(w->errstr, PACKAGE " [%d of %d] ...",
				        md->cn->nd.cnt,
				        md->rn->nd.cnt);
				winwidget_update_title(w, NULL );
			}
			if (!feh_main_iteration(0))
				exit(0);
		}
	}

	if (thumb_counter != 0){
		winwidget_render_image(w, 0, 1, SANITIZE_NO);
		winwidget_update_title(w, NULL );
	}

	if (opt.flg.verbose)
		putc('\n', stdout);

	if (opt.fn_title.fn )
		create_index_title_string( w, td);

	if (opt.output_file)
		feh_index2outputfile();

	if (!opt.flg.display)
		gib_imlib_free_image_and_decache(td.im_main);

	if ( md->cn == md->rn )
			md->cn = md->rn->next;        /* make#1 the current */


	return;

}   /* end of init_thumbnail_mode() */

int feh_thumbnail_get_file_from_coords(LLMD *md, int x, int y)
{

	for (md->tn = md->rn->next ;  md->tn != md->rn ; md->tn = md->tn->next) {
		if (XY_IN_RECT(x, y, md->tn->nd.x, md->tn->nd.y, md->tn->nd.wide, md->tn->nd.high)) {
			if (md->tn->nd.exists) {
				md->cn = md->tn;
				return 0;
			}
		}
	}

	D(("No matching %d %d\n", x, y));
	return 1;
}

void feh_thumbnail_mark_removed(feh_node * node, enum misc_flags delete_flag )
{
	winwidget w;

	w = winwidget_get_first_window_of_type(WIN_TYPE_THUMBNAIL);
	if (w) {
		int tw, th, first =0 , third = 255;

		if (delete_flag){
			first = third;
			third = 0;
		}
		gib_imlib_image_fill_rectangle(w->im, node->nd.x, node->nd.y,
				node->nd.wide, node->nd.high, first, 0, third, 150);

		tw = opt.fn_ptr->wide;  th = opt.fn_ptr->high;     /* just use dflt */
		feh_imlib_text_draw(w->im, &opt.style[ STYLE_YELLOW ],
				node->nd.x + ((node->nd.wide - tw) / 2),
				node->nd.y + ((node->nd.high - th) / 2), "X", IMLIB_TEXT_TO_RIGHT );
		winwidget_render_image(w, 0, 1, SANITIZE_NO);
	}
	node->nd.exists = 0;     /* do I make this the ->cn? */

	return;
}

void feh_thumbnail_create_im_main( winwidget *w, LLMD *md ){
		/* Apr 2013 HRABAK.  This used to be called feh_thumbnail_calculate_geometry()
		 * Renamed cause all the create logic from the body of init_thumbnail_mode()
		 * was moved here.
		 */
	char *s = mobs(2);

	if (!opt.limit_w && !opt.limit_h) {
		if (td.im_bg) {
			opt.limit_w = td.bg_w;
			opt.limit_h = td.bg_h;
		} else
			opt.limit_w = 800;
	}

	/* Here we need to whiz through the files, and look at the filenames and
	* info in the selected font, work out how much space we need, and
	* calculate the size of the image we will require */

	if (opt.limit_w) {
		td.wide = opt.limit_w;

		index_calculate_height(md, td.wide, &td.high, &td.thumb_tot_h);

		if (opt.limit_h) {
			if (td.high> opt.limit_h)
				weprintf(
					"The image size you specified (%dx%d) is not large\n"
					"enough to hold all %d thumbnails. To fit all\n"
					"the thumnails, either decrease their size, choose a smaller font,\n"
					"or use a larger image (like %dx%d)",
					opt.limit_w, opt.limit_h, LL_CNT( md ), opt.limit_w, td.high);
			td.high = opt.limit_h;
		}
	} else if (opt.limit_h) {
		td.vertical = 1;
		td.high = opt.limit_h;

		index_calculate_width( md, &td.wide, td.high, &td.thumb_tot_h);
	}

	/* all this logic was imported from the body of init_thumbnail_mode() */
	td.im_main = feh_imlib_image_make_n_fill_text_bg( td.wide, td.high,1);

	if (td.im_bg)
		gib_imlib_blend_image_onto_image(td.im_main, td.im_bg,
				      gib_imlib_image_has_alpha(td.im_bg),
				      0, 0, td.bg_w, td.bg_h, 0, 0,
				      td.wide, td.high, 1, 0, 0);
	else if (td.trans_bg) {
		gib_imlib_image_fill_rectangle(td.im_main,
		                              0, 0, td.wide, td.high, 0, 0, 0, 0);
		gib_imlib_image_set_has_alpha(td.im_main, 1);
	} else {          /* Colour the background */
		gib_imlib_image_fill_rectangle(td.im_main,
		                               0, 0, td.wide,td.high, 0, 0, 0, 255);
	}

	/* Create title. Now using a static buff */
	if (opt.flg.display) {
		if (opt.title)
			s = feh_printf(opt.title, NULL);    /* also static buff */
		else
			STRCAT_3ITEMS(s, PACKAGE " [", opt.modes[opt.flg.mode]," mode]");

		*w = winwidget_create_from_image(md ,td.im_main, s, td.type );
		winwidget_show(*w);
	}

}  /* end of feh_thumbnail_create_im_main() */

void feh_index2outputfile(void ){
		/* Apr 2013 HRABAK pulled this code out of init_thumbnail_mode() */
		char *output_buf = mobs(2);

		if (opt.output_dir)
			STRCAT_3ITEMS(output_buf, opt.output_dir, "/", opt.output_file);
		else
			strcat(output_buf, opt.output_file);

		gib_imlib_save_image(td.im_main, output_buf);
		if (opt.flg.verbose) {
			int tw, th;

			tw = gib_imlib_image_get_width(td.im_main);
			th = gib_imlib_image_get_height(td.im_main);
			fprintf(stdout, PACKAGE " - File saved as %s\n", output_buf);
			fprintf(stdout,
					"    - Image is %dx%d pixels and contains %d thumbnails\n",
					tw, th, td.thumbnailcount);
		}

}   /* end of feh_index2outputfile() */

int feh_thumbnail_get_thumbnail(Imlib_Image * image,
		                            feh_node * node, int * orig_w, int * orig_h)
{
	int status = 0;
	char *thumb_file = NULL, *uri = NULL;

	*orig_w = 0;
	*orig_h = 0;

	if (!node || !NODE_DATA(node) || !NODE_FILENAME(node) )
		return (0);

	if (opt.flg.cache_thumbnails) {
		uri = feh_thumbnail_get_name_uri(NODE_FILENAME(node));   /* static */
		thumb_file = feh_thumbnail_get_name(uri);                /* static */
		status = feh_thumbnail_get_generated(image, NODE_DATA(node), thumb_file,
			orig_w, orig_h);

		if (!status)
			status = feh_thumbnail_generate(image, NODE_DATA(node), thumb_file, uri,
				orig_w, orig_h);

		D(("uri is %s, thumb_file is %s\n", uri, thumb_file));
	} else
		status = feh_load_image(image, NODE_DATA(node));

	return status;
}

char *feh_thumbnail_get_name(char *uri)
{
	char *home = NULL,  *md5_name = NULL;
	char *thumb_file = mobs(2);
	/* FIXME: make sure original file isn't under ~/.thumbnails */

	md5_name = feh_thumbnail_get_name_md5(uri);       /* points to a static buff */

	home = getenv("HOME");
	if (home) {
		STRCAT_5ITEMS(thumb_file,home, "/.thumbnails/",td.cache_dir,"/",md5_name);
	}

	return thumb_file;
}

char *feh_thumbnail_get_name_uri(char *name)
{
	char *cwd, *uri = mobs(1);

	/* FIXME: what happens with http, https, and ftp? MTime etc */
	if ((strncmp(name, "http://", 7) != 0) &&
			(strncmp(name, "https://", 8) != 0) && (strncmp(name, "ftp://", 6) != 0)
			&& (strncmp(name, "file://", 7) != 0)) {

		/* make sure it's an absoulte path . FIXME: add support for ~,
		 * need to investigate if it's expanded somewhere else before
		 * adding (unecessary) code */
		if (name[0] != '/') {
			cwd = getcwd(NULL, 0);
			STRCAT_4ITEMS(uri,"file://", cwd,"/", name);
			free(cwd);
		} else {
			STRCAT_2ITEMS(uri,"file://",name);
		}
	} else
		strcat(uri,name);

	return uri;
}

char *feh_thumbnail_get_name_md5(char *uri)
{
	int i;
	char *pos;
	char  *md5_name = mobs(1);
	md5_state_t pms;
	md5_byte_t digest[16];

	/* generate the md5 sum */
	md5_init(&pms);
	md5_append(&pms, (unsigned char *)uri, strlen(uri));
	md5_finish(&pms, digest);

	/* print the md5 as hex to a string , md5 + .png + '\0' */
	for (i = 0, pos = md5_name; i < 16; i++, pos += 2) {
		sprintf(pos, "%02x", digest[i]);
	}
	strcat(pos, ".png");

	return md5_name;
}

int feh_thumbnail_generate(Imlib_Image * image, feh_data * data,
		char *thumb_file, char *uri, int * orig_w, int * orig_h)
{
	int w, h, thumb_w, thumb_h;
	Imlib_Image im_temp;
	struct stat sb;
	char c_width[8], c_height[8];

	if (feh_load_image(&im_temp, data) != 0) {
		*orig_w = w = gib_imlib_image_get_width(im_temp);
		*orig_h = h = gib_imlib_image_get_height(im_temp);
		thumb_w = td.cache_dim;
		thumb_h = td.cache_dim;

		if ((w > td.cache_dim) || (h > td.cache_dim)) {
			double ratio = (double) w / h;
			if (ratio > 1.0)
				thumb_h = td.cache_dim / ratio;
			else if (ratio != 1.0)
				thumb_w = td.cache_dim * ratio;
		}

		*image = gib_imlib_create_cropped_scaled_image(im_temp, 0, 0, w, h,
				thumb_w, thumb_h, 1);

		if (!stat(data->filename, &sb)) {
			char *c_mtime = mobs(2);
			sprintf(c_mtime, "%d", (int)sb.st_mtime);
			snprintf(c_width, 8, "%d", w);
			snprintf(c_height, 8, "%d", h);
			feh_png_write_png(*image, thumb_file, "Thumb::URI", uri,
					"Thumb::MTime", c_mtime,
					"Thumb::Image::Width", c_width,
					"Thumb::Image::Height", c_height);
		}

		gib_imlib_free_image_and_decache(im_temp);

		return (1);
	}

	return (0);
}

int feh_thumbnail_get_generated(Imlib_Image * image, feh_data * data,
                          char *thumb_file, int * orig_w, int * orig_h)
{
	struct stat sb;
	char *c_mtime=NULL, *c_width=NULL, *c_height=NULL;
	time_t mtime = 0;
	bhs_node **aHash;
	int i=0, bhs_size = 3;             /* change this if you add Thumb::URI */
	char * ts1 = "Thumb::MTime";
	char * ts2 = "Thumb::Image::Width";
	char * ts3 = "Thumb::Image::Height";

	if (!stat(data->filename, &sb)) {

		/* create aHash[], prefilling with just the three keys we care about.
		* When feh_png_read_comments() fills in the ->data part, any attempts
		* to add a new hash key fail cause the hash is only 3 keys deep.
		*/
		aHash = bhs_hash_init( bhs_size  );
		i    += bhs_hash_get( aHash , ts1, NULL, ADDIT_YES );
		i    += bhs_hash_get( aHash , ts2, NULL, ADDIT_YES );
		i    += bhs_hash_get( aHash , ts3, NULL, ADDIT_YES );
		if ( i != 6 )
				eprintf( "%s to store hash keys! ",ERR_FAILED);

		/* then fill the data that goes with these three keys. */

		if ( feh_png_read_comments(thumb_file, aHash ) == 0 ) {
			if ( bhs_hash_get(aHash, ts1 , NULL , ADDIT_NO ))
					c_mtime  = (char *) aHash[aHash[0]->h0.which]->h1.data;
			if ( bhs_hash_get(aHash, ts2 , NULL , ADDIT_NO ))
					c_width  = (char *) aHash[aHash[0]->h0.which]->h1.data;
			if ( bhs_hash_get(aHash, ts3 , NULL , ADDIT_NO ))
					c_height  = (char *) aHash[aHash[0]->h0.which]->h1.data;

			if (c_mtime != NULL)
				mtime = (time_t) strtol(c_mtime, NULL, 10);
			if (c_width != NULL)
				*orig_w = atoi(c_width);
			if (c_height != NULL)
				*orig_h = atoi(c_height);

			/* free all the data elements */
			for ( i=1; i< bhs_size+1 ; i++ ) {
					if ( aHash[i] )
							free( aHash[i]->h1.data );        /* FIXME could be a problem? */
			}

			/* FIXME: should we bother about Thumb::URI? */
			if (mtime == sb.st_mtime ) {
				feh_load_image_char(image, thumb_file);
				return (1);
			}
		}
	}

	return (0);
}

void feh_thumbnail_show_fullsize( LLMD *md , feh_node *node){
		/* the caller has set node to the one we want. */

	winwidget w = NULL;
	char *s;

	if (!opt.thumb_title)
		s = NODE_NAME( node );
	else
		s = feh_printf(opt.thumb_title, NODE_DATA( node ) );

	w = winwidget_get_first_window_of_type(WIN_TYPE_THUMBNAIL_VIEWER);
	if (!w) {
		w = winwidget_create_from_file(md, node ,s, WIN_TYPE_THUMBNAIL_VIEWER);
		if (w)
			winwidget_show(w);
	} else if ( NODE_DATA( w->node) != NODE_DATA( node ) ) {
		w->node = node;
		winwidget_update_title(w, s);
		feh_reload_image(w, RESIZE_YES, FORCE_NEW_YES );
	}
}

void feh_thumbnail_select(winwidget w, enum misc_flags jmp_code){
		/* the caller has already set md->cn to the one we want,
		* - OR - is requesting a jump to a new thumbnail.
		*/
	Imlib_Image origwin;

	if ( jmp_code < SLIDE_NO_JUMP ){
			/* shared with slideshow_change_image()  */
			jmp_code = test_first_last( w->md , jmp_code);
			feh_list_jump( w->md , jmp_code );
	}

	/* either way, md->cn IS the one we want to display */
	if ( w->md->cn == fgv.tdselected) return;    /* already on it */
	if ( w->md->cn == w->md->rn  )    return;    /* never happen? */

	w->node = w->md->cn;
	origwin = w->im;
	w->im = gib_imlib_clone_image(origwin);
	gib_imlib_image_fill_rectangle(w->im,
			w->md->cn->nd.x, w->md->cn->nd.y, w->md->cn->nd.wide,
			w->md->cn->nd.high, 50, 50, 255, 100);
	gib_imlib_image_draw_rectangle(w->im,
			w->md->cn->nd.x, w->md->cn->nd.y, w->md->cn->nd.wide,
			w->md->cn->nd.high, 255, 255, 255, 255);
	gib_imlib_image_draw_rectangle(w->im,
			w->md->cn->nd.x + 1, w->md->cn->nd.y + 1,
			w->md->cn->nd.wide - 2, w->md->cn->nd.high - 2,
			0, 0, 0, 255);
	gib_imlib_image_draw_rectangle(w->im,
			w->md->cn->nd.x + 2, w->md->cn->nd.y + 2,
			w->md->cn->nd.wide - 4, w->md->cn->nd.high - 4,
			255, 255, 255, 255);
	winwidget_render_image(w, 0, 0, SANITIZE_NO);
	gib_imlib_free_image_and_decache(w->im);
	w->im = origwin;
	fgv.tdselected = w->node = w->md->cn;

}   /* end of feh_thumbnail_select() */

void feh_thumbnail_setup_thumbnail_dir(void){
		/* Apr 2013 HRABAK combined all the setup found inside
		 * init_thumbnail_mode to make the  ~/.thumbnails/normal directory
		 */
	int status = 0;     /* just ignored */
	struct stat sb;
	char *dir = mobs(1);
	char *home;

	if (opt.thumb_w > opt.thumb_h)
		td.cache_dim = opt.thumb_w;
	else
		td.cache_dim = opt.thumb_h;

	if (td.cache_dim > SIZE_256) {
		/* No caching as specified by standard. Sort of. */
		opt.flg.cache_thumbnails = 0;
	} else if (td.cache_dim > SIZE_128) {
		td.cache_dim = SIZE_256;
		td.cache_dir = estrdup("large");
	} else {
		td.cache_dim = SIZE_128;
		td.cache_dir = estrdup("normal");
	}

	if ( (home = getenv("HOME")) == NULL )
		return ;                  /* status; */

	strcpy( dir, home );
	strcat( dir, "/.thumbnails/" );
	home = strrchr( dir, '\0' );         /* reuse the home var */
	strcat( dir, td.cache_dir );

	if (!stat(dir, &sb)) {
		if (S_ISDIR(sb.st_mode))
			status = 1;
		else
			weprintf("%s should be a directory", dir);
	} else {
		home--;                     /* move behind the '/'   */
		home[0] = '\0';             /* kill the td.cache_dir */
		if (stat(dir, &sb) != 0) {
			if (mkdir(dir, 0700) == -1)
				weprintf("unable to create directory %s", dir );
		}

		home[0] = '/';             /* put it back and try again */
		if (mkdir(dir, 0700) == -1)
			weprintf("unable to create directory %s", dir);
		else
			status = 1;
	}

	return ;                     /* status; */
}
