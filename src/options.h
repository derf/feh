/* options.h

Copyright (C) 1999-2003 Tom Gilbert.

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
	unsigned char index_show_name;
	unsigned char index_show_dim;
	unsigned char index_show_size;
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
	unsigned char auto_zoom;
	unsigned char draw_filename;
	unsigned char list;
	unsigned char quiet;
	unsigned char preload;
	unsigned char loadables;
	unsigned char unloadables;
	unsigned char reverse;
	unsigned char no_menus;
	unsigned char scale_down;
	unsigned char builtin_http;
	unsigned char bgmode;
	unsigned char xinerama;
	unsigned char screen_clip;
	unsigned char hide_pointer;
	unsigned char draw_actions;
	unsigned char cache_thumbnails;
	unsigned char cycle_once;
	unsigned char hold_actions[10];

	char *output_file;
	char *output_dir;
	char *bg_file;
	char *font;
	char *title_font;
	char *title;
	char *thumb_title;
	char *actions[10];
	char *fontpath;
	char *filelistfile;
	char *menu_font;
	char *customlist;
	char *menu_bg;
	char *image_bg;
	char *rcfile;
	char *menu_style;
	char *caption_path;
	char *start_list_at;
	char *info_cmd;

	gib_style *menu_style_l;

	unsigned char pan_button;
	unsigned char zoom_button;
	unsigned char menu_button;
	unsigned char menu_ctrl_mask;
	unsigned char prev_button;
	unsigned char next_button;

	unsigned char rotate_button;
	unsigned char blur_button;
	unsigned char reload_button;
	unsigned char no_rotate_ctrl_mask;
	unsigned char no_blur_ctrl_mask;
	unsigned char no_pan_ctrl_mask;

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
	unsigned char adjust_reload;

	unsigned char mode;
	unsigned char paused;

	double slideshow_delay;

	Imlib_Font menu_fn;
};

struct __fehkey {
	int keysyms[3];
	int keystates[3];
};

struct __fehkb {
	struct __fehkey menu_close;
	struct __fehkey menu_parent;
	struct __fehkey menu_down;
	struct __fehkey menu_up;
	struct __fehkey menu_child;
	struct __fehkey menu_select;
	struct __fehkey move_right;
	struct __fehkey prev_img;
	struct __fehkey move_left;
	struct __fehkey next_img;
	struct __fehkey move_up;
	struct __fehkey move_down;
	struct __fehkey jump_back;
	struct __fehkey quit;
	struct __fehkey jump_fwd;
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
	struct __fehkey render;
	struct __fehkey toggle_actions;
	struct __fehkey toggle_filenames;
	struct __fehkey toggle_pointer;
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
	struct __fehkey toggle_fullscreen;
	struct __fehkey reload_minus;
	struct __fehkey reload_plus;
};

void init_parse_options(int argc, char **argv);
char *feh_string_normalize(char *str);

extern fehoptions opt;

#endif
