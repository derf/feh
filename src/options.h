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

struct __fehoptions {
	unsigned char multiwindow;
	unsigned char montage;
	unsigned char collage;
	unsigned char index;
	unsigned char thumbs;
	unsigned char slideshow;
	unsigned char recursive;
	unsigned char output;
	unsigned char verbose;
	unsigned char display;
	unsigned char bg;
	unsigned char alpha;
	unsigned char alpha_level;
	unsigned char aspect;
	unsigned char stretch;
	unsigned char keep_http;
	unsigned char borderless;
	unsigned char randomize;
	unsigned char jump_on_resort;
	unsigned char full_screen;
	unsigned char draw_filename;
#ifdef HAVE_LIBEXIF
	unsigned char draw_exif;
	unsigned char auto_rotate;
#endif
	unsigned char list;
	unsigned char quiet;
	unsigned char preload;
	unsigned char loadables;
	unsigned char unloadables;
	unsigned char reverse;
	unsigned char no_menus;
	unsigned char scale_down;
	unsigned char bgmode;
	unsigned char xinerama;
	unsigned char screen_clip;
	unsigned char hide_pointer;
	unsigned char draw_actions;
	unsigned char draw_info;
	unsigned char cache_thumbnails;
	unsigned char cycle_once;
	unsigned char hold_actions[10];
	unsigned char text_bg;
	unsigned char image_bg;
	unsigned char no_fehbg;
	unsigned char keep_zoom_vp;
	unsigned char insecure_ssl;

	char *output_file;
	char *output_dir;
	char *bg_file;
	char *font;
	char *title_font;
	char *title;
	char *thumb_title;
	char *actions[10];
	char *action_titles[10];
	char *fontpath;
	char *filelistfile;
	char *menu_font;
	char *customlist;
	char *menu_bg;
	char *caption_path;
	char *start_list_at;
	char *info_cmd;
	char *index_info;

	int force_aliasing;
	int thumb_w;
	int thumb_h;
	int limit_w;
	int limit_h;
	unsigned int thumb_redraw;
	double reload;
	int sort;
	int debug;
	int geom_flags;
	int geom_x;
	int geom_y;
	unsigned int geom_w;
	unsigned int geom_h;
	int default_zoom;
	int zoom_mode;
	unsigned char adjust_reload;
	int xinerama_index;

	/* signed in case someone wants to invert scrolling real quick */
	int scroll_step;

	unsigned int min_width, min_height, max_width, max_height;

	unsigned char mode;
	unsigned char paused;

	double slideshow_delay;

	signed short magick_timeout;

	Imlib_Font menu_fn;
};

struct __fehkey {
	unsigned int keysyms[3];
	unsigned int keystates[3];
	unsigned int state;
	unsigned int button;
};

struct __fehkb {
	struct __fehkey menu_close;
	struct __fehkey menu_parent;
	struct __fehkey menu_down;
	struct __fehkey menu_up;
	struct __fehkey menu_child;
	struct __fehkey menu_select;
	struct __fehkey scroll_right;
	struct __fehkey prev_img;
	struct __fehkey scroll_left;
	struct __fehkey next_img;
	struct __fehkey scroll_up;
	struct __fehkey scroll_down;
	struct __fehkey scroll_right_page;
	struct __fehkey scroll_left_page;
	struct __fehkey scroll_up_page;
	struct __fehkey scroll_down_page;
	struct __fehkey jump_back;
	struct __fehkey quit;
	struct __fehkey jump_fwd;
	struct __fehkey prev_dir;
	struct __fehkey next_dir;
	struct __fehkey remove;
	struct __fehkey delete;
	struct __fehkey jump_first;
	struct __fehkey jump_last;
	struct __fehkey action_0;
	struct __fehkey action_1;
	struct __fehkey action_2;
	struct __fehkey action_3;
	struct __fehkey action_4;
	struct __fehkey action_5;
	struct __fehkey action_6;
	struct __fehkey action_7;
	struct __fehkey action_8;
	struct __fehkey action_9;
	struct __fehkey zoom_in;
	struct __fehkey zoom_out;
	struct __fehkey zoom_default;
	struct __fehkey zoom_fit;
	struct __fehkey zoom_fill;
	struct __fehkey render;
	struct __fehkey toggle_actions;
	struct __fehkey toggle_filenames;
#ifdef HAVE_LIBEXIF
	struct __fehkey toggle_exif;
#endif
	struct __fehkey toggle_info;
	struct __fehkey toggle_pointer;
	struct __fehkey toggle_aliasing;
	struct __fehkey jump_random;
	struct __fehkey toggle_caption;
	struct __fehkey toggle_pause;
	struct __fehkey reload_image;
	struct __fehkey save_image;
	struct __fehkey save_filelist;
	struct __fehkey size_to_image;
	struct __fehkey toggle_menu;
	struct __fehkey close;
	struct __fehkey orient_1;
	struct __fehkey orient_3;
	struct __fehkey flip;
	struct __fehkey mirror;
	struct __fehkey toggle_fullscreen;
	struct __fehkey reload_minus;
	struct __fehkey reload_plus;
	struct __fehkey toggle_keep_vp;
	struct __fehkey pan;
	struct __fehkey zoom;
	struct __fehkey reload;
	struct __fehkey blur;
	struct __fehkey rotate;
};

void init_parse_options(int argc, char **argv);
char *feh_string_normalize(char *str);

extern fehoptions opt;

#endif
