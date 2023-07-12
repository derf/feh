/* options.h

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

#ifndef OPTIONS_H
#define OPTIONS_H

enum on_last_slide_action {
	ON_LAST_SLIDE_RESUME = 0,
	ON_LAST_SLIDE_QUIT,
	ON_LAST_SLIDE_HOLD
};

struct __fehoptions {
	unsigned char multiwindow;
	unsigned char montage;
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
	unsigned char use_conversion_cache;
	unsigned char borderless;
	unsigned char randomize;
	unsigned char jump_on_resort;
	unsigned char full_screen;
	unsigned char draw_filename;
#ifdef HAVE_LIBEXIF
	unsigned char draw_exif;
	unsigned char auto_rotate;
#endif
#ifdef HAVE_INOTIFY
	unsigned char auto_reload;
    int inotify_fd;
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
	unsigned char on_last_slide;
	unsigned char hold_actions[10];
	unsigned char text_bg;
	unsigned char no_fehbg;
	unsigned char keep_zoom_vp;
	unsigned char insecure_ssl;
	unsigned char filter_by_dimensions;
	unsigned char edit;

	char *output_file;
	char *output_dir;
	char *bg_file;
	char *image_bg;
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
	int version_sort;
	int debug;
	int geom_enabled;
	int geom_flags;
	int geom_x;
	int geom_y;
	unsigned int geom_w;
	unsigned int geom_h;
	int offset_flags;
	int offset_x;
	int offset_y;
	int default_zoom;
	int zoom_mode;
	double zoom_rate;
	unsigned char adjust_reload;
	int xinerama_index;
	char *x11_class;
	unsigned long int x11_windowid;

	/* signed in case someone wants to invert scrolling real quick */
	int scroll_step;

	// imlib cache size in mebibytes
	int cache_size;

	unsigned int min_width, min_height, max_width, max_height;

	unsigned char mode;
	unsigned char paused;

	double slideshow_delay;

	signed int conversion_timeout;

	Imlib_Font menu_fn;
};

enum __feh_option {
OPTION_debug = '+',
OPTION_scale_down = '.',
OPTION_max_dimension = '<',
OPTION_min_dimension = '>',
OPTION_title_font = '@',
OPTION_action = 'A',
OPTION_image_bg = 'B',
OPTION_fontpath = 'C',
OPTION_slideshow_delay = 'D',
OPTION_thumb_height = 'E',
OPTION_fullscreen = 'F',
OPTION_draw_actions = 'G',
OPTION_limit_height = 'H',
OPTION_fullindex = 'I',
OPTION_thumb_redraw = 'J',
OPTION_caption_path = 'K',
OPTION_customlist = 'L',
OPTION_menu_font = 'M',
OPTION_no_menus = 'N',
OPTION_output_only = 'O',
OPTION_cache_thumbnails = 'P',
OPTION_reload = 'R',
OPTION_sort = 'S',
OPTION_theme = 'T',
OPTION_loadable = 'U',
OPTION_verbose = 'V',
OPTION_limit_width = 'W',
OPTION_ignore_aspect = 'X',
OPTION_hide_pointer = 'Y',
OPTION_auto_zoom = 'Z',
OPTION_title = '^',
OPTION_alpha = 'a',
OPTION_bg = 'b',
OPTION_draw_filename = 'd',
OPTION_font = 'e',
OPTION_filelist = 'f',
OPTION_geometry = 'g',
OPTION_help = 'h',
OPTION_index = 'i',
OPTION_output_dir = 'j',
OPTION_keep_http = 'k',
OPTION_list = 'l',
OPTION_montage = 'm',
OPTION_reverse = 'n',
OPTION_output = 'o',
OPTION_preload = 'p',
OPTION_quiet = 'q',
OPTION_recursive = 'r',
OPTION_stretch = 's',
OPTION_thumbnails = 't',
OPTION_unloadable = 'u',
OPTION_version = 'v',
OPTION_multiwindow = 'w',
OPTION_borderless = 'x',
OPTION_thumb_width = 'y',
OPTION_randomize = 'z',
OPTION_start_at = '|',
OPTION_thumb_title = '~',
OPTION_bg_title,
OPTION_bg_center,
OPTION_bg_scale,
OPTION_bg_fill,
OPTION_bg_max,
OPTION_zoom,
OPTION_zoom_step,
OPTION_zoom_in_rate,
OPTION_zoom_out_rate,
OPTION_keep_zoom_vp,
OPTION_no_screen_clip,
OPTION_index_info,
OPTION_magick_timeout,
OPTION_action1,
OPTION_action2,
OPTION_action3,
OPTION_action4,
OPTION_action5,
OPTION_action6,
OPTION_action7,
OPTION_action8,
OPTION_action9,
OPTION_no_jump_on_resort,
OPTION_edit,
OPTION_draw_exif,
OPTION_auto_rotate,
OPTION_no_xinerama,
OPTION_draw_tinted,
OPTION_info,
OPTION_force_aliasing,
OPTION_no_fehbg,
OPTION_scroll_step,
OPTION_xinerama_index,
OPTION_insecure,
OPTION_no_recursive,
OPTION_cache_size,
OPTION_on_last_slide,
OPTION_conversion_timeout,
OPTION_version_sort,
OPTION_offset,
OPTION_auto_reload,
OPTION_class,
OPTION_no_conversion_cache,
OPTION_window_id,
};

//typedef enum __fehoption fehoption;

struct __fehkey {
	unsigned int keysyms[3];
	unsigned int keystates[3];
	unsigned int state;
	unsigned int button;
	char *name;
};

enum key_action {
	EVENT_menu_close = 0,
	EVENT_menu_parent,
	EVENT_menu_down,
	EVENT_menu_up,
	EVENT_menu_child,
	EVENT_menu_select,
	EVENT_scroll_left,
	EVENT_scroll_right,
	EVENT_scroll_down,
	EVENT_scroll_up,
	EVENT_scroll_left_page,
	EVENT_scroll_right_page,
	EVENT_scroll_down_page,
	EVENT_scroll_up_page,
	EVENT_prev_img,
	EVENT_next_img,
	EVENT_jump_back,
	EVENT_jump_fwd,
	EVENT_prev_dir,
	EVENT_next_dir,
	EVENT_jump_random,
	EVENT_quit,
	EVENT_close,
	EVENT_remove,
	EVENT_delete,
	EVENT_jump_first,
	EVENT_jump_last,
	EVENT_action_0,
	EVENT_action_1,
	EVENT_action_2,
	EVENT_action_3,
	EVENT_action_4,
	EVENT_action_5,
	EVENT_action_6,
	EVENT_action_7,
	EVENT_action_8,
	EVENT_action_9,
	EVENT_zoom_in,
	EVENT_zoom_out,
	EVENT_zoom_default,
	EVENT_zoom_fit,
	EVENT_zoom_fill,
	EVENT_size_to_image,
	EVENT_render,
	EVENT_toggle_actions,
	EVENT_toggle_aliasing,
	EVENT_toggle_auto_zoom,
#ifdef HAVE_LIBEXIF
	EVENT_toggle_exif,
#endif
	EVENT_toggle_filenames,
	EVENT_toggle_info,
	EVENT_toggle_pointer,
	EVENT_toggle_caption,
	EVENT_toggle_pause,
	EVENT_toggle_menu,
	EVENT_toggle_fullscreen,
	EVENT_reload_image,
	EVENT_save_image,
	EVENT_save_filelist,
	EVENT_orient_1,
	EVENT_orient_3,
	EVENT_flip,
	EVENT_mirror,
	EVENT_reload_minus,
	EVENT_reload_plus,
	EVENT_toggle_keep_vp,
	EVENT_toggle_fixed_geometry,
	EVENT_pan,
	EVENT_zoom,
	EVENT_blur,
	EVENT_rotate,
	EVENT_LIST_END
};

void init_parse_options(int argc, char **argv);
char *feh_string_normalize(char *str);

extern fehoptions opt;

#endif
