/* options.h

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

#ifndef OPTIONS_H
#define OPTIONS_H

/* As of May 2012, HRABAK cut out ALL the gib_style stuff and replaced it
 * with an array of feh_style structs  inside __fehoptions.
 * See structs.h for struct feh_style.
 * As of Apr 2013 HRABAK encapsulated all feh fonts here in struct _feh_font
 */

struct __fehoptions {         /* montage and collage killed Jul 2012 */
	struct {                    /* flags for all on/off settings        */
		unsigned list           : 1;
		unsigned index          : 1;
		unsigned thumbnail      : 1;
		unsigned multiwindow    : 1;
		unsigned full_screen    : 1;
		unsigned unloadables    : 1;
		unsigned loadables      : 1;
		unsigned randomize      : 1;
		unsigned recursive      : 1;
		unsigned verbose        : 1;
		unsigned display        : 1;
		unsigned bg             : 1;
		unsigned alpha          : 1;
		unsigned aspect         : 1;
		unsigned stretch        : 1;
		unsigned keep_http      : 1;
		unsigned borderless     : 1;
		unsigned jump_on_resort : 1;
		unsigned draw_exif      : 1;      /*  only used when HAVE_LIBEXIF  */
		unsigned quiet          : 1;
		unsigned preload        : 1;
		unsigned reverse        : 1;
		unsigned no_menus       : 1;
		unsigned big_menus      : 1;      /* large font for menus          */
		unsigned scale_down     : 1;
		unsigned xinerama       : 1;
		unsigned screen_clip    : 1;
		unsigned hide_pointer   : 1;
		unsigned no_actions     : 1;      /* defaults to "1" meaning no actions    */
		unsigned draw_actions   : 1;
		unsigned draw_info      : 1;
		unsigned draw_filename  : 1;
		unsigned draw_name      : 1;
		unsigned draw_no_ext    : 1;
		unsigned cache_thumbnails   : 1;   /* use cached thumbnails from ~/.thumbnails */
		unsigned cycle_once     : 1;
		unsigned no_fehbg       : 1;
		unsigned write_filelist : 1;       /* OK to write filelist at exit? */
		unsigned reset_output   : 1;       /* was a naked global b4 */
		unsigned paused         : 1;
		unsigned text_bg        : 1;      /* OK at 1bit.  Only on/off              */
							/* MULTI-BIT values */
		unsigned image_bg       : 2;      /* holds 0, 1, 2                         */
		unsigned state          : 3;      /* holds 0 thru 5 used for panning etc.  */
		unsigned mode           : 4;      /* holds 0 thru 9. ptr to modes[]        */
		unsigned mode_original  : 4;      /* copy of the above mode setting        */
		unsigned bgmode         : 8;      /* holds 23 thru 27                      */
	} flg;                            /* flg stands for flags */

	char *modes[8];                      /* mode names.  opt.flg.mode is the index */

	unsigned char alpha_level;    /* zero thru 255 */

	char *output_file;
	char *output_dir;
	char *bg_file;
	char *title;
	char *thumb_title;
	char *actions[ MAX_ACTIONS ];
	char *fontpath;
	char *filelistfile;
	char *customlist;
	char *menu_bg;
	/* char *menu_style; */
	char *caption_path;
	char *start_at_name;
	char *info_cmd;
	char *index_info;

	feh_style style[STYLE_CNT];
	feh_font *fn_ptr;                        /* points to one of the five below  */
	feh_font fn_dflt;                        /* used to be the default "font"    */
	feh_font fn_title;                       /* used to be title_font            */
	feh_font fn_menu;                        /* used to be the menu_font         */
	feh_font fn_fulls;                       /* used to be the big_font          */
	feh_font fn_fep;                         /* used inside the move_mode scrn   */

	int force_aliasing;
	int thumb_w;
	int thumb_h;
	int limit_w;
	int limit_h;
	unsigned int thumb_redraw;
	int reload;
	int sort;
	int debug;
	int geom_flags;
	int geom_x;
	int geom_y;
	unsigned int geom_w;
	unsigned int geom_h;
	int default_zoom;
	int zoom_mode;
	unsigned int start_at_num;            /* start_at a numbered item in the list */

	double slideshow_delay;

	/* default this to -1 (off) cause big recurvive loads can take forever otherwise */
	signed short magick_timeout;

};

struct __fehbutton {
	int modifier;
	char button;
};

struct __fehbb {
	struct __fehbutton pan;
	struct __fehbutton zoom;
	struct __fehbutton reload;
	struct __fehbutton prev;
	struct __fehbutton next;
	struct __fehbutton menu;
	struct __fehbutton blur;
	struct __fehbutton rotate;
};

void init_parse_options(int argc, char **argv);
char *feh_string_normalize(char *str);
void check_options(void);
void feh_parse_option_array(int argc, char **argv );


/* extern fehoptions opt;   moved to feh.h */

#endif
