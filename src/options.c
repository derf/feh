/* options.c

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

#include <strings.h>
#include "feh.h"
#include "filelist.h"
#include "options.h"

static void check_options(void);
static void feh_getopt_theme(int argc, char **argv);
static void feh_parse_option_array(int argc, char **argv, int finalrun);
static void feh_check_theme_options(char **argv);
static void feh_parse_options_from_string(char *opts);
static void feh_load_options_for_theme(char *theme);
static void show_usage(void);
static void show_version(void);
static char *theme;

fehoptions opt;

void init_parse_options(int argc, char **argv)
{
	/* TODO: sort these to match declaration of __fehoptions */

	/* For setting the command hint on X windows */
	cmdargc = argc;
	cmdargv = argv;

	/* Set default options */
	memset(&opt, 0, sizeof(fehoptions));
	opt.display = 1;
	opt.aspect = 1;
	opt.slideshow_delay = 0.0;
	opt.conversion_timeout = -1;
	opt.thumb_w = 60;
	opt.thumb_h = 60;
	opt.thumb_redraw = 10;
	opt.scroll_step = 20;
	opt.menu_font = estrdup(DEFAULT_MENU_FONT);
	opt.font = NULL;
	opt.max_height = opt.max_width = UINT_MAX;

	opt.zoom_rate = 1.25;

	opt.start_list_at = NULL;
	opt.jump_on_resort = 1;

	opt.screen_clip = 1;
	opt.cache_size = 4;
#ifdef HAVE_LIBXINERAMA
	/* if we're using xinerama, then enable it by default */
	opt.xinerama = 1;
	opt.xinerama_index = -1;
#endif				/* HAVE_LIBXINERAMA */
#ifdef HAVE_INOTIFY
	opt.auto_reload = 1;
#endif				/* HAVE_INOTIFY */
	opt.use_conversion_cache = 1;

	feh_getopt_theme(argc, argv);

	D(("About to check for theme configuration\n"));
	feh_check_theme_options(argv);

	D(("About to parse commandline options\n"));
	/* Parse the cmdline args */
	feh_parse_option_array(argc, argv, 1);

	/* If we have a filelist to read, do it now */
	if (opt.filelistfile) {
		/* joining two reverse-sorted lists in this manner works nicely for us
		   here, as files specified on the commandline end up at the *end* of
		   the combined filelist, in the specified order. */
		D(("About to load filelist from file\n"));
		filelist = gib_list_cat(filelist, feh_read_filelist(opt.filelistfile));
	}

	D(("Options parsed\n"));

	filelist_len = gib_list_length(filelist);
	if (!filelist_len)
		show_mini_usage();

	check_options();

	feh_prepare_filelist();
	return;
}

static void feh_check_theme_options(char **argv)
{
	if (!theme) {
		/* This prevents screw up when running src/feh or ./feh */
		char *pos = strrchr(argv[0], '/');

		if (pos)
			theme = estrdup(pos + 1);
		else
			theme = estrdup(argv[0]);
	}
	D(("Theme name is %s\n", theme));

	feh_load_options_for_theme(theme);

	free(theme);
	return;
}

static void feh_load_options_for_theme(char *theme)
{
	FILE *fp = NULL;
	char *home = getenv("HOME");
	char *rcpath = NULL;
	char *oldrcpath = NULL;
	char *confbase = getenv("XDG_CONFIG_HOME");
	// s, s1 and s2 must always have identical size
	char s[1024], s1[1024], s2[1024];
	int cont = 0;
	int bspos;

	if (confbase)
		rcpath = estrjoin("/", confbase, "feh/themes", NULL);
	else if (home)
		rcpath = estrjoin("/", home, ".config/feh/themes", NULL);
	else {
		weprintf("You have no HOME, cannot read configuration");
		return;
	}

	oldrcpath = estrjoin("/", home, ".fehrc", NULL);

	fp = fopen(rcpath, "r");

	free(rcpath);

	if (!fp && ((fp = fopen(oldrcpath, "r")) != NULL))
		weprintf("The theme config file was moved from ~/.fehrc to "
			"~/.config/feh/themes. Run\n"
			"    mkdir -p ~/.config/feh; mv ~/.fehrc ~/.config/feh/themes\n"
			"to fix this.");

	free(oldrcpath);

	if (!fp && ((fp = fopen("/etc/feh/themes", "r")) == NULL))
		return;

	/* Oooh. We have an options file :) */
	for (; fgets(s, sizeof(s), fp);) {
		s1[0] = '\0';
		s2[0] = '\0';

		if (cont) {
			/*
			 * fgets ensures that s contains no more than 1023 characters
			 * (+ 1 null byte)
			 */
			sscanf(s, " %[^\n]\n", (char *) &s2);
			if (!*s2)
				break;
			D(("Got continued options %s\n", s2));
		} else {
			/*
			 * fgets ensures that s contains no more than 1023 characters
			 * (+ 1 null byte)
			 */
			sscanf(s, "%s %[^\n]\n", (char *) &s1, (char *) &s2);
			if (!(*s1) || (!*s2) || (*s1 == '\n') || (*s1 == '#')) {
				cont = 0;
				continue;
			}
			D(("Got theme/options pair %s/%s\n", s1, s2));
		}

		if (!strcmp(s1, theme) || cont) {

			bspos = strlen(s2)-1;

			if (s2[bspos] == '\\') {
				D(("Continued line\n"));
				s2[bspos] = '\0';
				cont = 1;
				/* A trailing whitespace confuses the option parser */
				if (bspos && (s2[bspos-1] == ' '))
					s2[bspos-1] = '\0';
			} else
				cont = 0;

			D(("A match. Using options %s\n", s2));
			feh_parse_options_from_string(s2);

			if (!cont)
				break;
		}
	}
	fclose(fp);
	return;
}

/* FIXME This function is a crufty bitch ;) */
static void feh_parse_options_from_string(char *opts)
{
	char *list[sizeof(char *) * 64];
	int num = 0;
	char *s;
	char *t;
	char last = 0;
	char inquote = 0;
	int i = 0;

	/* So we don't reinvent the wheel (not again, anyway), we use the
	   getopt_long function to do this parsing as well. This means it has to
	   look like the real argv ;) */

	list[num++] = estrdup(PACKAGE);

	for (s = opts, t = opts;; t++) {

		if (num > 64)
			eprintf(PACKAGE " does not support more than 64 words per "
					"theme definition.\n Please shorten your lines.");

		if ((*t == ' ') && !inquote) {
			*t = '\0';
			num++;

			list[num - 1] = feh_string_normalize(s);
			s = t + 1;
		} else if (*t == '\0') {
			num++;

			list[num - 1] = feh_string_normalize(s);
			break;
		} else if ((*t == inquote) && (last != '\\')) {
			inquote = 0;
		} else if (((*t == '\"') || (*t == '\'')) && (last != '\\') && !inquote)
			inquote = *t;
		last = *t;
	}

	feh_parse_option_array(num, list, 0);

	for (i = 0; i < num; i++)
		if (list[i])
			free(list[i]);
	return;
}

char *feh_string_normalize(char *str)
{
	char ret[4096];
	char *s;
	int i = 0;
	char last = 0;

	D(("normalizing %s\n", str));
	ret[0] = '\0';

	for (s = str;; s++) {
		if (*s == '\0')
			break;
		else if ((*s == '\"') && (last == '\\'))
			ret[i++] = '\"';
		else if ((*s == '\"') && (last == 0));
		else if ((*s == '\'') && (last == '\\'))
			ret[i++] = '\'';
		else if ((*s == '\'') && (last == 0));
		else if ((*s == ' ') && (last == '\\'))
			ret[i++] = ' ';
		else
			ret[i++] = *s;

		last = *s;
	}
	if (i && ((ret[i - 1] == '\"') || (ret[i - 1] == '\'')))
		ret[i - 1] = '\0';
	else
		ret[i] = '\0';
	D(("normalized to %s\n", ret));

	return(estrdup(ret));
}

static void feh_getopt_theme(int argc, char **argv)
{
	static char stropts[] = "-T:";
	static struct option lopts[] = {
		{"theme", 1, 0, 'T'},
		{0, 0, 0, 0}
	};
	int optch = 0, cmdx = 0;

	opterr = 0;

	while ((optch = getopt_long(argc, argv, stropts, lopts, &cmdx)) != EOF) {
		if (optch == 'T')
			theme = estrdup(optarg);
	}

	opterr = 1;
	optind = 0;
}

static void feh_parse_option_array(int argc, char **argv, int finalrun)
{
	int discard;
	static char stropts[] =
		"a:A:b:B:C:dD:e:E:f:Fg:GhH:iIj:J:kK:lL:mM:nNo:O:pPqrR:sS:tT:uUvVwW:xXy:YzZ"
		".@:^:~:|:+:<:>:";

	/* (*name, has_arg, *flag, val) See: struct option in getopts.h */
	static struct option lopts[] = {
		{"debug"         , 0, 0, OPTION_debug},
		{"scale-down"    , 0, 0, OPTION_scale_down},
		{"max-dimension" , 1, 0, OPTION_max_dimension},
		{"min-dimension" , 1, 0, OPTION_min_dimension},
		{"title-font"    , 1, 0, OPTION_title_font},
		{"action"        , 1, 0, OPTION_action},
		{"image-bg"      , 1, 0, OPTION_image_bg},
		{"fontpath"      , 1, 0, OPTION_fontpath},
		{"slideshow-delay",1, 0, OPTION_slideshow_delay},
		{"thumb-height"  , 1, 0, OPTION_thumb_height},
		{"full-screen"   , 0, 0, OPTION_fullscreen}, /* deprecated */
		{"fullscreen"    , 0, 0, OPTION_fullscreen},
		{"draw-actions"  , 0, 0, OPTION_draw_actions},
		{"limit-height"  , 1, 0, OPTION_limit_height},
		{"fullindex"     , 0, 0, OPTION_fullindex},
		{"thumb-redraw"  , 1, 0, OPTION_thumb_redraw},
		{"caption-path"  , 1, 0, OPTION_caption_path},
		{"customlist"    , 1, 0, OPTION_customlist},
		{"menu-font"     , 1, 0, OPTION_menu_font},
		{"no-menus"      , 0, 0, OPTION_no_menus},
		{"output-only"   , 1, 0, OPTION_output_only},
		{"cache-thumbnails", 0, 0, OPTION_cache_thumbnails},
		{"reload"        , 1, 0, OPTION_reload},
		{"sort"          , 1, 0, OPTION_sort},
		{"theme"         , 1, 0, OPTION_theme},
		{"loadable"      , 0, 0, OPTION_loadable},
		{"verbose"       , 0, 0, OPTION_verbose},
		{"limit-width"   , 1, 0, OPTION_limit_width},
		{"ignore-aspect" , 0, 0, OPTION_ignore_aspect},
		{"hide-pointer"  , 0, 0, OPTION_hide_pointer},
		{"auto-zoom"     , 0, 0, OPTION_auto_zoom},
		{"title"         , 1, 0, OPTION_title},
		{"alpha"         , 1, 0, OPTION_alpha},
		{"bg"            , 1, 0, OPTION_bg},
		{"draw-filename" , 0, 0, OPTION_draw_filename},
		{"font"          , 1, 0, OPTION_font},
		{"filelist"      , 1, 0, OPTION_filelist},
		{"geometry"      , 1, 0, OPTION_geometry},
		{"help"          , 0, 0, OPTION_help},
		{"index"         , 0, 0, OPTION_index},
		{"output-dir"    , 1, 0, OPTION_output_dir},
		{"keep-http"     , 0, 0, OPTION_keep_http},
		{"list"          , 0, 0, OPTION_list},
		{"montage"       , 0, 0, OPTION_montage},
		{"reverse"       , 0, 0, OPTION_reverse},
		{"output"        , 1, 0, OPTION_output},
		{"preload"       , 0, 0, OPTION_preload},
		{"quiet"         , 0, 0, OPTION_quiet},
		{"recursive"     , 0, 0, OPTION_recursive},
		{"stretch"       , 0, 0, OPTION_stretch},
		{"thumbnails"    , 0, 0, OPTION_thumbnails},
		{"unloadable"    , 0, 0, OPTION_unloadable},
		{"version"       , 0, 0, OPTION_version},
		{"multiwindow"   , 0, 0, OPTION_multiwindow},
		{"borderless"    , 0, 0, OPTION_borderless},
		{"thumb-width"   , 1, 0, OPTION_thumb_width},
		{"randomize"     , 0, 0, OPTION_randomize},
		{"start-at"      , 1, 0, OPTION_start_at},
		{"thumb-title"   , 1, 0, OPTION_thumb_title},
		{"bg-tile"       , 0, 0, OPTION_bg_title},
		{"bg-center"     , 0, 0, OPTION_bg_center},
		{"bg-scale"      , 0, 0, OPTION_bg_scale},
		{"zoom"          , 1, 0, OPTION_zoom},
		{"zoom-step"     , 1, 0, OPTION_zoom_step},
		{"no-screen-clip", 0, 0, OPTION_no_screen_clip},
		{"index-info"    , 1, 0, OPTION_index_info},
		{"magick-timeout", 1, 0, OPTION_magick_timeout},
		{"action1"       , 1, 0, OPTION_action1},
		{"action2"       , 1, 0, OPTION_action2},
		{"action3"       , 1, 0, OPTION_action3},
		{"action4"       , 1, 0, OPTION_action4},
		{"action5"       , 1, 0, OPTION_action5},
		{"action6"       , 1, 0, OPTION_action6},
		{"action7"       , 1, 0, OPTION_action7},
		{"action8"       , 1, 0, OPTION_action8},
		{"action9"       , 1, 0, OPTION_action9},
		{"bg-fill"       , 0, 0, OPTION_bg_fill},
		{"bg-max"        , 0, 0, OPTION_bg_max},
		{"no-jump-on-resort", 0, 0, OPTION_no_jump_on_resort},
		{"edit"          , 0, 0, OPTION_edit},
#ifdef HAVE_LIBEXIF
		{"draw-exif"     , 0, 0, OPTION_draw_exif},
		{"auto-rotate"   , 0, 0, OPTION_auto_rotate},
#endif
		{"no-xinerama"   , 0, 0, OPTION_no_xinerama},
		{"draw-tinted"   , 0, 0, OPTION_draw_tinted},
		{"info"          , 1, 0, OPTION_info},
		{"force-aliasing", 0, 0, OPTION_force_aliasing},
		{"no-fehbg"      , 0, 0, OPTION_no_fehbg},
		{"keep-zoom-vp"  , 0, 0, OPTION_keep_zoom_vp},
		{"scroll-step"   , 1, 0, OPTION_scroll_step},
		{"xinerama-index", 1, 0, OPTION_xinerama_index},
		{"insecure"      , 0, 0, OPTION_insecure},
		{"no-recursive"  , 0, 0, OPTION_no_recursive},
		{"cache-size"    , 1, 0, OPTION_cache_size},
		{"on-last-slide" , 1, 0, OPTION_on_last_slide},
		{"conversion-timeout" , 1, 0, OPTION_conversion_timeout},
		{"version-sort"  , 0, 0, OPTION_version_sort},
		{"offset"        , 1, 0, OPTION_offset},
#ifdef HAVE_INOTIFY
		{"auto-reload"   , 0, 0, OPTION_auto_reload},
#endif
		{"class"         , 1, 0, OPTION_class},
		{"no-conversion-cache", 0, 0, OPTION_no_conversion_cache},
		{"window-id", 1, 0, OPTION_window_id},
		{0, 0, 0, 0}
	};
	int optch = 0, cmdx = 0;

	while ((optch = getopt_long(argc, argv, stropts, lopts, &cmdx)) != EOF) {
		D(("Got option, getopt calls it %d, or %c\n", optch, optch));
		switch (optch) {
		case 0:
			break;
		case OPTION_debug:
			opt.debug = 1;
			break;
		case OPTION_max_dimension:
			opt.filter_by_dimensions = 1;
			XParseGeometry(optarg, &discard, &discard, &opt.max_width, &opt.max_height);
			if (opt.max_width == 0)
				opt.max_width = UINT_MAX;
			if (opt.max_height == 0)
				opt.max_height = UINT_MAX;
			break;
		case OPTION_min_dimension:
			opt.filter_by_dimensions = 1;
			XParseGeometry(optarg, &discard, &discard, &opt.min_width, &opt.min_height);
			break;
		case OPTION_scale_down:
			opt.scale_down = 1;
			break;
		case OPTION_title_font:
			opt.title_font = estrdup(optarg);
			break;
		case OPTION_action:
			opt.actions[0] = estrdup(optarg);
			break;
		case OPTION_image_bg:
			opt.image_bg = estrdup(optarg);
			break;
		case OPTION_fontpath:
			D(("adding fontpath %s\n", optarg));
			imlib_add_path_to_font_path(optarg);
			break;
		case OPTION_slideshow_delay:
			opt.slideshow_delay = atof(optarg);
			if (opt.slideshow_delay < 0.0) {
				opt.slideshow_delay *= (-1);
				opt.paused = 1;
			} else {
				opt.paused = 0;
			}
			break;
		case OPTION_thumb_height:
			opt.thumb_h = atoi(optarg);
			break;
		case OPTION_fullscreen:
			opt.full_screen = 1;
			break;
		case OPTION_draw_actions:
			opt.draw_actions = 1;
			break;
		case OPTION_limit_height:
			opt.limit_h = atoi(optarg);
			break;
		case OPTION_fullindex:
			opt.index = 1;
			opt.index_info = estrdup("%n\n%S\n%wx%h");
			break;
		case OPTION_thumb_redraw:
			opt.thumb_redraw = atoi(optarg);
			break;
		case OPTION_caption_path:
			opt.caption_path = estrdup(optarg);
			break;
		case OPTION_customlist:
			opt.customlist = estrdup(optarg);
			opt.display = 0;
			break;
		case OPTION_menu_font:
			free(opt.menu_font);
			opt.menu_font = estrdup(optarg);
			break;
		case OPTION_no_menus:
			opt.no_menus = 1;
			break;
		case OPTION_output_only:
			opt.output = 1;
			opt.output_file = estrdup(optarg);
			opt.display = 0;
			break;
		case OPTION_cache_thumbnails:
			opt.cache_thumbnails = 1;
			break;
		case OPTION_reload:
			opt.reload = atof(optarg);
			opt.use_conversion_cache = 0;
#ifdef HAVE_INOTIFY
			opt.auto_reload = 0;
#endif
			break;
		case OPTION_sort:
			if (!strcasecmp(optarg, "name"))
				opt.sort = SORT_NAME;
			else if (!strcasecmp(optarg, "none"))
				opt.sort = SORT_NONE;
			else if (!strcasecmp(optarg, "filename"))
				opt.sort = SORT_FILENAME;
			else if (!strcasecmp(optarg, "dirname"))
				opt.sort = SORT_DIRNAME;
			else if (!strcasecmp(optarg, "mtime"))
				opt.sort = SORT_MTIME;
			else if (!strcasecmp(optarg, "width"))
				opt.sort = SORT_WIDTH;
			else if (!strcasecmp(optarg, "height"))
				opt.sort = SORT_HEIGHT;
			else if (!strcasecmp(optarg, "pixels"))
				opt.sort = SORT_PIXELS;
			else if (!strcasecmp(optarg, "size"))
				opt.sort = SORT_SIZE;
			else if (!strcasecmp(optarg, "format"))
				opt.sort = SORT_FORMAT;
			else {
				weprintf("Unrecognised sort mode \"%s\". Defaulting to "
						"sort by filename", optarg);
				opt.sort = SORT_FILENAME;
			}
			if (opt.randomize) {
				weprintf("commandline contains --randomize and --sort. "
						"--randomize has been unset");
				opt.randomize = 0;
			}
			break;
		case OPTION_theme:
			theme = estrdup(optarg);
			break;
		case OPTION_loadable:
			opt.loadables = 1;
			opt.display = 0;
			break;
		case OPTION_verbose:
			opt.verbose = 1;
			break;
		case OPTION_limit_width:
			opt.limit_w = atoi(optarg);
			break;
		case OPTION_ignore_aspect:
			opt.aspect = 0;
			break;
		case OPTION_hide_pointer:
			opt.hide_pointer = 1;
			break;
		case OPTION_auto_zoom:
			opt.zoom_mode = ZOOM_MODE_MAX;
			break;
		case OPTION_title:
			opt.title = estrdup(optarg);
			break;
		case OPTION_alpha:
			opt.alpha = 1;
			opt.alpha_level = 255 - atoi(optarg);
			break;
		case OPTION_bg:
			opt.bg = 1;
			opt.bg_file = estrdup(optarg);
			break;
		case OPTION_draw_filename:
			opt.draw_filename = 1;
			break;
		case OPTION_font:
			opt.font = estrdup(optarg);
			break;
		case OPTION_filelist:
			if (!strcmp(optarg, "-"))
				opt.filelistfile = estrdup("/dev/stdin");
			else
				opt.filelistfile = estrdup(optarg);
			break;
		case OPTION_geometry:
			opt.geom_enabled = 1;
			opt.geom_flags = XParseGeometry(optarg, &opt.geom_x,
					&opt.geom_y, &opt.geom_w, &opt.geom_h);
			break;
		case OPTION_help:
			show_usage();
			break;
		case OPTION_index:
			opt.index = 1;
			opt.index_info = estrdup("%n");
			break;
		case OPTION_output_dir:
			opt.output_dir = estrdup(optarg);
			break;
		case OPTION_keep_http:
			opt.keep_http = 1;
			break;
		case OPTION_list:
			opt.list = 1;
			opt.display = 0;
			break;
		case OPTION_montage:
			opt.index = 1;
			break;
		case OPTION_reverse:
			opt.reverse = 1;
			break;
		case OPTION_output:
			opt.output = 1;
			opt.output_file = estrdup(optarg);
			break;
		case OPTION_preload:
			opt.preload = 1;
			break;
		case OPTION_quiet:
			opt.quiet = 1;
			break;
		case OPTION_recursive:
			opt.recursive = 1;
			break;
		case OPTION_stretch:
			opt.stretch = 1;
			break;
		case OPTION_thumbnails:
			opt.thumbs = 1;
			opt.index_info = estrdup("%n");
			break;
		case OPTION_unloadable:
			opt.unloadables = 1;
			opt.display = 0;
			break;
		case OPTION_version:
			show_version();
			break;
		case OPTION_multiwindow:
			opt.multiwindow = 1;
			break;
		case OPTION_borderless:
			opt.borderless = 1;
			break;
		case OPTION_thumb_width:
			opt.thumb_w = atoi(optarg);
			break;
		case OPTION_randomize:
			opt.randomize = 1;
			if (opt.sort != SORT_NONE) {
				weprintf("commandline contains --sort and --randomize. "
						"--sort has been unset");
				opt.sort = SORT_NONE;
			}
			break;
		case OPTION_start_at:
			opt.start_list_at = estrdup(optarg);
			break;
		case OPTION_thumb_title:
			opt.thumb_title = estrdup(optarg);
			break;
		case OPTION_bg_title:
			opt.bgmode = BG_MODE_TILE;
			break;
		case OPTION_bg_center:
			opt.bgmode = BG_MODE_CENTER;
			break;
		case OPTION_bg_scale:
			opt.bgmode = BG_MODE_SCALE;
			break;
		case OPTION_zoom:
			if (!strcmp("fill", optarg))
				opt.zoom_mode = ZOOM_MODE_FILL;
			else if (!strcmp("max", optarg))
				opt.zoom_mode = ZOOM_MODE_MAX;
			else
				opt.default_zoom = atoi(optarg);
			break;
		case OPTION_no_screen_clip:
			opt.screen_clip = 0;
			break;
		case OPTION_index_info:
			opt.index_info = estrdup(optarg);
			break;
		case OPTION_magick_timeout:
			weprintf("--magick-timeout is deprecated, please use --conversion-timeout instead");
			opt.conversion_timeout = atoi(optarg);
			break;
		case OPTION_action1:
			opt.actions[1] = estrdup(optarg);
			break;
		case OPTION_action2:
			opt.actions[2] = estrdup(optarg);
			break;
		case OPTION_action3:
			opt.actions[3] = estrdup(optarg);
			break;
		case OPTION_action4:
			opt.actions[4] = estrdup(optarg);
			break;
		case OPTION_action5:
			opt.actions[5] = estrdup(optarg);
			break;
		case OPTION_action6:
			opt.actions[6] = estrdup(optarg);
			break;
		case OPTION_action7:
			opt.actions[7] = estrdup(optarg);
			break;
		case OPTION_action8:
			opt.actions[8] = estrdup(optarg);
			break;
		case OPTION_action9:
			opt.actions[9] = estrdup(optarg);
			break;
		case OPTION_bg_fill:
			opt.bgmode = BG_MODE_FILL;
			break;
		case OPTION_bg_max:
			opt.bgmode = BG_MODE_MAX;
			break;
		case OPTION_no_jump_on_resort:
			opt.jump_on_resort = 0;
			break;
		case OPTION_edit:
			opt.edit = 1;
			break;
#ifdef HAVE_LIBEXIF
		case OPTION_draw_exif:
			opt.draw_exif = 1;
			break;
		case OPTION_auto_rotate:
#if defined(IMLIB2_VERSION_MAJOR) && defined(IMLIB2_VERSION_MINOR) && defined(IMLIB2_VERSION_MICRO) && (IMLIB2_VERSION_MAJOR > 1 || IMLIB2_VERSION_MINOR > 7 || IMLIB2_VERSION_MICRO >= 5)
			weprintf("This feh release was built with Imlib2 version %d.%d.%d, which transparently adjusts for image orientation according to EXIF data.", IMLIB2_VERSION_MAJOR, IMLIB2_VERSION_MINOR, IMLIB2_VERSION_MICRO);
			weprintf("--auto-rotate would rotate an already correctly oriented image, resulting in incorrect orientation. It has been disabled in this build. Rebuild feh with Imlib2 <1.7.5 to enable --auto-rotate.");
#else
			opt.auto_rotate = 1;
#endif
			break;
#endif
		case OPTION_no_xinerama:
			opt.xinerama = 0;
			break;
		case OPTION_draw_tinted:
			opt.text_bg = TEXT_BG_TINTED;
			break;
		case OPTION_info:
			opt.info_cmd = estrdup(optarg);
			if (opt.info_cmd[0] == ';') {
				opt.draw_info = 0;
				opt.info_cmd++;
			} else {
				opt.draw_info = 1;
			}
			break;
		case OPTION_force_aliasing:
			opt.force_aliasing = 1;
			break;
		case OPTION_no_fehbg:
			opt.no_fehbg = 1;
			break;
		case OPTION_keep_zoom_vp:
			opt.keep_zoom_vp = 1;
			break;
		case OPTION_scroll_step:
			opt.scroll_step = atoi(optarg);
			break;
		case OPTION_xinerama_index:
			opt.xinerama_index = atoi(optarg);
			break;
		case OPTION_insecure:
			opt.insecure_ssl = 1;
			break;
		case OPTION_no_recursive:
			opt.recursive = 0;
			break;
		case OPTION_cache_size:
			opt.cache_size = atoi(optarg);
			if (opt.cache_size < 0)
				opt.cache_size = 0;
			if (opt.cache_size > 2048)
				opt.cache_size = 2048;
			break;
		case OPTION_on_last_slide:
			if (!strcmp(optarg, "quit")) {
				opt.on_last_slide = ON_LAST_SLIDE_QUIT;
			} else if (!strcmp(optarg, "hold")) {
				opt.on_last_slide = ON_LAST_SLIDE_HOLD;
			} else if (!strcmp(optarg, "resume")) {
				opt.on_last_slide = ON_LAST_SLIDE_RESUME;
			} else {
				weprintf("Unrecognized on-last-slide action \"%s\"."
						"Supported actions: hold, resume, quit\n", optarg);
			}
			break;
		case OPTION_conversion_timeout:
			opt.conversion_timeout = atoi(optarg);
			break;
		case OPTION_version_sort:
			opt.version_sort = 1;
			break;
		case OPTION_offset:
			opt.offset_flags = XParseGeometry(optarg, &opt.offset_x,
					&opt.offset_y, (unsigned int *)&discard, (unsigned int *)&discard);
			break;
#ifdef HAVE_INOTIFY
		case OPTION_auto_reload:
			opt.auto_reload = 1;
			break;
#endif
		case OPTION_class:
			opt.x11_class = estrdup(optarg);
			break;
		case OPTION_no_conversion_cache:
			opt.use_conversion_cache = 0;
			break;
		case OPTION_window_id:
			opt.x11_windowid = strtol(optarg, NULL, 0);
			break;
		case OPTION_zoom_step:
			opt.zoom_rate = atof(optarg);
			if ((opt.zoom_rate <= 0)) {
				weprintf("Zooming disabled due to --zoom-step=%f", opt.zoom_rate);
				opt.zoom_rate = 1.0;
			} else {
				opt.zoom_rate = 1 + ((float)opt.zoom_rate / 100);
			}
			break;
		default:
			break;
		}
	}

	/* Now the leftovers, which must be files */
	if (optind < argc) {
		while (optind < argc) {
			if (opt.reload)
				original_file_items = gib_list_add_front(original_file_items, estrdup(argv[optind]));
			/* If recursive is NOT set, but the only argument is a directory
			   name, we grab all the files in there, but not subdirs */
			add_file_to_filelist_recursively(argv[optind++], FILELIST_FIRST);
		}
	}
	else if (finalrun && !opt.filelistfile && !opt.bgmode) {
		/*
		 * if --start-at is a non-local URL (i.e., does not start with file:///),
		 * behave as if "feh URL" was called (there is no directory we can load)
		 */
		if (opt.start_list_at && path_is_url(opt.start_list_at) && (strlen(opt.start_list_at) <= 8 || strncmp(opt.start_list_at, "file:///", 8) != 0)) {
			add_file_to_filelist_recursively(opt.start_list_at, FILELIST_FIRST);
		/*
		 * Otherwise, make "feh --start-at dir/file.jpg" behave like
		 * "feh --start-at dir/file.jpg dir".
		 */
		} else if (opt.start_list_at && strrchr(opt.start_list_at, '/')) {
			/*
			 * feh can't candle urlencoded path components ("some%20dir" etc).
			 * Use libcurl to unescape them if --start-at is file://...
			 */
			if (strlen(opt.start_list_at) > 8 && strncmp(opt.start_list_at, "file:///", 8) == 0) {
				char *unescaped_path = feh_http_unescape(opt.start_list_at);
				if (unescaped_path != NULL) {
					free(opt.start_list_at);
					opt.start_list_at = estrdup(unescaped_path + 7);
					free(unescaped_path);
				} else {
					char *new_path = estrdup(opt.start_list_at + 7);
					free(opt.start_list_at);
					opt.start_list_at = new_path;
				}
			}
			char *target_directory = estrdup(opt.start_list_at);
			char *filename_start = strrchr(target_directory, '/');
			if (filename_start) {
				*filename_start = '\0';
			}
			add_file_to_filelist_recursively(target_directory, FILELIST_FIRST);
			original_file_items = gib_list_add_front(original_file_items, estrdup(target_directory));
			free(target_directory);
		} else {
			add_file_to_filelist_recursively(".", FILELIST_FIRST);
		}
	}

	/* So that we can safely be called again */
	optind = 0;
	return;
}

static void check_options(void)
{
	int i;
	char *endptr;

	for (i = 0; i < 10; i++) {
		if (opt.actions[i] && !opt.hold_actions[i] && (opt.actions[i][0] == ';')) {
			opt.hold_actions[i] = 1;
			opt.actions[i] = opt.actions[i] + 1;
		}
		opt.action_titles[i] = opt.actions[i];
		if (opt.actions[i] && (opt.actions[i][0] == '[')) {
			if (((endptr = strchr(opt.actions[i], ']')) != NULL)
					&& (opt.actions[i][1] != ' ')) {
				opt.action_titles[i] = opt.actions[i] + 1;
				opt.actions[i] = endptr + 1;
				*endptr = 0;
			}
		}
	}

	if (opt.full_screen && opt.multiwindow) {
		eprintf("You cannot combine --fullscreen with --multiwindow");
	}

	if (opt.list && (opt.multiwindow || opt.index)) {
		eprintf("You cannot combine --list with other modes");
	}

	if (opt.loadables && opt.unloadables) {
		eprintf("You cannot combine --loadable with --unloadable");
	}

	return;
}

static void show_version(void)
{
	puts(PACKAGE " version " VERSION);
	puts("Compile-time switches: "

#ifdef HAVE_LIBCURL
		"curl "
#endif

#ifdef DEBUG
		"debug "
#endif

#ifdef HAVE_LIBEXIF
		"exif "
#endif

#ifdef HAVE_INOTIFY
		"inotify "
#endif

#ifdef INCLUDE_HELP
		"help "
#endif

#ifdef HAVE_LIBMAGIC
		"magic "
#endif

#if _FILE_OFFSET_BITS == 64
		"stat64 "
#endif

#ifdef HAVE_STRVERSCMP
         "verscmp "
#endif

#ifdef HAVE_LIBXINERAMA
		"xinerama "
#endif

	);

	exit(0);
}

void show_mini_usage(void)
{
	fputs(PACKAGE ": No loadable images specified.\n"
#ifdef INCLUDE_HELP
		"See '" PACKAGE " --help' or 'man " PACKAGE "' for detailed usage information\n",
#else
		"See 'man " PACKAGE "' for detailed usage information\n",
#endif
		stderr);
	exit(1);
}

static void show_usage(void)
{
	fputs(
#ifdef INCLUDE_HELP
#include "help.inc"
#else
	"See 'man " PACKAGE "'\n"
#endif
	, stdout);
	exit(0);
}
