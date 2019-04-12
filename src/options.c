/* options.c

Copyright (C) 1999-2003 Tom Gilbert.
Copyright (C) 2010-2018 Daniel Friesel.

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
		{"debug"         , 0, 0, '+'},
		{"scale-down"    , 0, 0, '.'},
		{"max-dimension" , 1, 0, '<'},
		{"min-dimension" , 1, 0, '>'},
		{"title-font"    , 1, 0, '@'},
		{"action"        , 1, 0, 'A'},
		{"image-bg"      , 1, 0, 'B'},
		{"fontpath"      , 1, 0, 'C'},
		{"slideshow-delay",1, 0, 'D'},
		{"thumb-height"  , 1, 0, 'E'},
		{"full-screen"   , 0, 0, 'F'}, /* deprecated */
		{"fullscreen"    , 0, 0, 'F'},
		{"draw-actions"  , 0, 0, 'G'},
		{"limit-height"  , 1, 0, 'H'},
		{"fullindex"     , 0, 0, 'I'},
		{"thumb-redraw"  , 1, 0, 'J'},
		{"caption-path"  , 1, 0, 'K'},
		{"customlist"    , 1, 0, 'L'},
		{"menu-font"     , 1, 0, 'M'},
		{"no-menus"      , 0, 0, 'N'},
		{"output-only"   , 1, 0, 'O'},
		{"cache-thumbnails", 0, 0, 'P'},
		{"reload"        , 1, 0, 'R'},
		{"sort"          , 1, 0, 'S'},
		{"theme"         , 1, 0, 'T'},
		{"loadable"      , 0, 0, 'U'},
		{"verbose"       , 0, 0, 'V'},
		{"limit-width"   , 1, 0, 'W'},
		{"ignore-aspect" , 0, 0, 'X'},
		{"hide-pointer"  , 0, 0, 'Y'},
		{"auto-zoom"     , 0, 0, 'Z'},
		{"title"         , 1, 0, '^'},
		{"alpha"         , 1, 0, 'a'},
		{"bg"            , 1, 0, 'b'},
		{"draw-filename" , 0, 0, 'd'},
		{"font"          , 1, 0, 'e'},
		{"filelist"      , 1, 0, 'f'},
		{"geometry"      , 1, 0, 'g'},
		{"help"          , 0, 0, 'h'},
		{"index"         , 0, 0, 'i'},
		{"output-dir"    , 1, 0, 'j'},
		{"keep-http"     , 0, 0, 'k'},
		{"list"          , 0, 0, 'l'},
		{"montage"       , 0, 0, 'm'},
		{"reverse"       , 0, 0, 'n'},
		{"output"        , 1, 0, 'o'},
		{"preload"       , 0, 0, 'p'},
		{"quiet"         , 0, 0, 'q'},
		{"recursive"     , 0, 0, 'r'},
		{"stretch"       , 0, 0, 's'},
		{"thumbnails"    , 0, 0, 't'},
		{"unloadable"    , 0, 0, 'u'},
		{"version"       , 0, 0, 'v'},
		{"multiwindow"   , 0, 0, 'w'},
		{"borderless"    , 0, 0, 'x'},
		{"thumb-width"   , 1, 0, 'y'},
		{"randomize"     , 0, 0, 'z'},
		{"start-at"      , 1, 0, '|'},
		{"thumb-title"   , 1, 0, '~'},
		{"bg-tile"       , 0, 0, 200},
		{"bg-center"     , 0, 0, 201},
		{"bg-scale"      , 0, 0, 202},
		{"zoom"          , 1, 0, 205},
		{"no-screen-clip", 0, 0, 206},
		{"index-info"    , 1, 0, 207},
		{"magick-timeout", 1, 0, 208},
		{"action1"       , 1, 0, 209},
		{"action2"       , 1, 0, 210},
		{"action3"       , 1, 0, 211},
		{"action4"       , 1, 0, 212},
		{"action5"       , 1, 0, 213},
		{"action6"       , 1, 0, 214},
		{"action7"       , 1, 0, 215},
		{"action8"       , 1, 0, 216},
		{"action9"       , 1, 0, 217},
		{"bg-fill"       , 0, 0, 218},
		{"bg-max"        , 0, 0, 219},
		{"no-jump-on-resort", 0, 0, 220},
		{"edit"          , 0, 0, 221},
#ifdef HAVE_LIBEXIF
		{"draw-exif"     , 0, 0, 223},
		{"auto-rotate"   , 0, 0, 242},
#endif
		{"no-xinerama"   , 0, 0, 225},
		{"draw-tinted"   , 0, 0, 229},
		{"info"          , 1, 0, 234},
		{"force-aliasing", 0, 0, 235},
		{"no-fehbg"      , 0, 0, 236},
		{"keep-zoom-vp"  , 0, 0, 237},
		{"scroll-step"   , 1, 0, 238},
		{"xinerama-index", 1, 0, 239},
		{"insecure"      , 0, 0, 240},
		{"no-recursive"  , 0, 0, 241},
		{"cache-size"    , 1, 0, 243},
		{"on-last-slide" , 1, 0, 244},
		{"conversion-timeout" , 1, 0, 245},
		{"version-sort"  , 0, 0, 246},
		{"offset"        , 1, 0, 247},
#ifdef HAVE_INOTIFY
		{"auto-reload"   , 0, 0, 248},
#endif
		{0, 0, 0, 0}
	};
	int optch = 0, cmdx = 0;

	while ((optch = getopt_long(argc, argv, stropts, lopts, &cmdx)) != EOF) {
		D(("Got option, getopt calls it %d, or %c\n", optch, optch));
		switch (optch) {
		case 0:
			break;
		case '+':
			opt.debug = 1;
			break;
		case '<':
			opt.filter_by_dimensions = 1;
			XParseGeometry(optarg, &discard, &discard, &opt.max_width, &opt.max_height);
			if (opt.max_width == 0)
				opt.max_width = UINT_MAX;
			if (opt.max_height == 0)
				opt.max_height = UINT_MAX;
			break;
		case '>':
			opt.filter_by_dimensions = 1;
			XParseGeometry(optarg, &discard, &discard, &opt.min_width, &opt.min_height);
			break;
		case '.':
			opt.scale_down = 1;
			break;
		case '@':
			opt.title_font = estrdup(optarg);
			break;
		case 'A':
			opt.actions[0] = estrdup(optarg);
			break;
		case 'B':
			opt.image_bg = estrdup(optarg);
			break;
		case 'C':
			D(("adding fontpath %s\n", optarg));
			imlib_add_path_to_font_path(optarg);
			break;
		case 'D':
			opt.slideshow_delay = atof(optarg);
			if (opt.slideshow_delay < 0.0) {
				opt.slideshow_delay *= (-1);
				opt.paused = 1;
			} else {
				opt.paused = 0;
			}
			break;
		case 'E':
			opt.thumb_h = atoi(optarg);
			break;
		case 'F':
			opt.full_screen = 1;
			break;
		case 'G':
			opt.draw_actions = 1;
			break;
		case 'H':
			opt.limit_h = atoi(optarg);
			break;
		case 'I':
			opt.index = 1;
			opt.index_info = estrdup("%n\n%S\n%wx%h");
			break;
		case 'J':
			opt.thumb_redraw = atoi(optarg);
			break;
		case 'K':
			opt.caption_path = estrdup(optarg);
			break;
		case 'L':
			opt.customlist = estrdup(optarg);
			opt.display = 0;
			break;
		case 'M':
			free(opt.menu_font);
			opt.menu_font = estrdup(optarg);
			break;
		case 'N':
			opt.no_menus = 1;
			break;
		case 'O':
			opt.output = 1;
			opt.output_file = estrdup(optarg);
			opt.display = 0;
			break;
		case 'P':
			opt.cache_thumbnails = 1;
			break;
		case 'R':
			opt.reload = atof(optarg);
#ifdef HAVE_INOTIFY
			opt.auto_reload = 0;
#endif
			break;
		case 'S':
			if (!strcasecmp(optarg, "name"))
				opt.sort = SORT_NAME;
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
		case 'T':
			theme = estrdup(optarg);
			break;
		case 'U':
			opt.loadables = 1;
			opt.display = 0;
			break;
		case 'V':
			opt.verbose = 1;
			break;
		case 'W':
			opt.limit_w = atoi(optarg);
			break;
		case 'X':
			opt.aspect = 0;
			break;
		case 'Y':
			opt.hide_pointer = 1;
			break;
		case 'Z':
			opt.zoom_mode = ZOOM_MODE_MAX;
			break;
		case '^':
			opt.title = estrdup(optarg);
			break;
		case 'a':
			opt.alpha = 1;
			opt.alpha_level = 255 - atoi(optarg);
			break;
		case 'b':
			opt.bg = 1;
			opt.bg_file = estrdup(optarg);
			break;
		case 'd':
			opt.draw_filename = 1;
			break;
		case 'e':
			opt.font = estrdup(optarg);
			break;
		case 'f':
			if (!strcmp(optarg, "-"))
				opt.filelistfile = estrdup("/dev/stdin");
			else
				opt.filelistfile = estrdup(optarg);
			break;
		case 'g':
			opt.geom_enabled = 1;
			opt.geom_flags = XParseGeometry(optarg, &opt.geom_x,
					&opt.geom_y, &opt.geom_w, &opt.geom_h);
			break;
		case 'h':
			show_usage();
			break;
		case 'i':
			opt.index = 1;
			opt.index_info = estrdup("%n");
			break;
		case 'j':
			opt.output_dir = estrdup(optarg);
			break;
		case 'k':
			opt.keep_http = 1;
			break;
		case 'l':
			opt.list = 1;
			opt.display = 0;
			break;
		case 'm':
			opt.index = 1;
			break;
		case 'n':
			opt.reverse = 1;
			break;
		case 'o':
			opt.output = 1;
			opt.output_file = estrdup(optarg);
			break;
		case 'p':
			opt.preload = 1;
			break;
		case 'q':
			opt.quiet = 1;
			break;
		case 'r':
			opt.recursive = 1;
			break;
		case 's':
			opt.stretch = 1;
			break;
		case 't':
			opt.thumbs = 1;
			opt.index_info = estrdup("%n");
			break;
		case 'u':
			opt.unloadables = 1;
			opt.display = 0;
			break;
		case 'v':
			show_version();
			break;
		case 'w':
			opt.multiwindow = 1;
			break;
		case 'x':
			opt.borderless = 1;
			break;
		case 'y':
			opt.thumb_w = atoi(optarg);
			break;
		case 'z':
			opt.randomize = 1;
			if (opt.sort != SORT_NONE) {
				weprintf("commandline contains --sort and --randomize. "
						"--sort has been unset");
				opt.sort = SORT_NONE;
			}
			break;
		case '|':
			opt.start_list_at = estrdup(optarg);
			break;
		case '~':
			opt.thumb_title = estrdup(optarg);
			break;
		case 200:
			opt.bgmode = BG_MODE_TILE;
			break;
		case 201:
			opt.bgmode = BG_MODE_CENTER;
			break;
		case 202:
			opt.bgmode = BG_MODE_SCALE;
			break;
		case 205:
			if (!strcmp("fill", optarg))
				opt.zoom_mode = ZOOM_MODE_FILL;
			else if (!strcmp("max", optarg))
				opt.zoom_mode = ZOOM_MODE_MAX;
			else
				opt.default_zoom = atoi(optarg);
			break;
		case 206:
			opt.screen_clip = 0;
			break;
		case 207:
			opt.index_info = estrdup(optarg);
			break;
		case 208:
			weprintf("--magick-timeout is deprecated, please use --conversion-timeout instead");
			opt.conversion_timeout = atoi(optarg);
			break;
		case 209:
			opt.actions[1] = estrdup(optarg);
			break;
		case 210:
			opt.actions[2] = estrdup(optarg);
			break;
		case 211:
			opt.actions[3] = estrdup(optarg);
			break;
		case 212:
			opt.actions[4] = estrdup(optarg);
			break;
		case 213:
			opt.actions[5] = estrdup(optarg);
			break;
		case 214:
			opt.actions[6] = estrdup(optarg);
			break;
		case 215:
			opt.actions[7] = estrdup(optarg);
			break;
		case 216:
			opt.actions[8] = estrdup(optarg);
			break;
		case 217:
			opt.actions[9] = estrdup(optarg);
			break;
		case 218:
			opt.bgmode = BG_MODE_FILL;
			break;
		case 219:
			opt.bgmode = BG_MODE_MAX;
			break;
		case 220:
			opt.jump_on_resort = 0;
			break;
		case 221:
			opt.edit = 1;
			break;
#ifdef HAVE_LIBEXIF
		case 223:
			opt.draw_exif = 1;
			break;
		case 242:
			opt.auto_rotate = 1;
			break;
#endif
		case 225:
			opt.xinerama = 0;
			break;
		case 229:
			opt.text_bg = TEXT_BG_TINTED;
			break;
		case 234:
			opt.info_cmd = estrdup(optarg);
			if (opt.info_cmd[0] == ';') {
				opt.draw_info = 0;
				opt.info_cmd++;
			} else {
				opt.draw_info = 1;
			}
			break;
		case 235:
			opt.force_aliasing = 1;
			break;
		case 236:
			opt.no_fehbg = 1;
			break;
		case 237:
			opt.keep_zoom_vp = 1;
			break;
		case 238:
			opt.scroll_step = atoi(optarg);
			break;
		case 239:
			opt.xinerama_index = atoi(optarg);
			break;
		case 240:
			opt.insecure_ssl = 1;
			break;
		case 241:
			opt.recursive = 0;
			break;
		case 243:
			opt.cache_size = atoi(optarg);
			if (opt.cache_size < 0)
				opt.cache_size = 0;
			if (opt.cache_size > 2048)
				opt.cache_size = 2048;
			break;
		case 244:
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
		case 245:
			opt.conversion_timeout = atoi(optarg);
			break;
		case 246:
			opt.version_sort = 1;
			break;
		case 247:
			opt.offset_flags = XParseGeometry(optarg, &opt.offset_x,
					&opt.offset_y, (unsigned int *)&discard, (unsigned int *)&discard);
			break;
#ifdef HAVE_INOTIFY
		case 248:
			opt.auto_reload = 1;
			break;
#endif
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
		if (opt.start_list_at && !path_is_url(opt.start_list_at) && strrchr(opt.start_list_at, '/')) {
			char *target_directory = estrdup(opt.start_list_at);
			char *filename_start = strrchr(target_directory, '/');
			if (filename_start) {
				*filename_start = '\0';
			}
			add_file_to_filelist_recursively(target_directory, FILELIST_FIRST);
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

#if _FILE_OFFSET_BITS == 64
		"stat64 "
#endif

#ifdef HAVE_VERSCMP
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
