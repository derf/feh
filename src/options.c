/* options.c

Copyright (C) 1999-2003 Tom Gilbert.
Copyright (C) 2010-2011 Daniel Friesel.
Copyright (C) 2012      Christopher Hrabak

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
#include "feh_ll.h"
#include "options.h"

static void feh_getopt_theme(int argc, char **argv);
static void feh_check_theme_options(char **argv);
static void feh_parse_options_from_string(char *opts);
static void feh_load_options_for_theme(char *theme);
static void show_usage(void);
static void show_version(void);
void init_all_styles( feh_style (*s)[] );
/*static char *theme;*/

void init_all_fgv(){  /* 'fgv' stands for fehGlobalVars. */

		/* initialize all members to zero. */
		memset(&fgv, 0, sizeof(feh_global_vars));

		fgv.no_cap = "Caption entry mode - Hit ESC to cancel";
		fgv.ptr    = NULL;        /* function ptr for &stub_move_mode_toggle */

		fgv.mnu.cover = 0;
		fgv.mnu.root  = NULL;
		fgv.mnu.list  = NULL;

}   /* end of init_all_fgv() */


void init_all_styles( feh_style (*s)[] ){
		/* loads the set of default sytle settings used in feh.
		* Historically, menu and caption styles were the only two, but
		* HRABAK added several more to encapsulate the five diff ways
		* that feh called the feh_imlib_text_draw() function.
		* See the style[] array stored inside fehoptions.
		* See the misc_flags enum in structs.h.  That enum  IS the index to
		* this styles array, so DON'T screw up the order.
		* This internal order matches the external menu.style file like ...
		* #Style
		* #NAME Menu
		* 0 0 0 64 1 1                 #bg, r g b x_off y_off
		* 127 0 0 255 0 0              #fg, r g b x_off y_off
		*/


	/* load defaults into the many style structs */
	int a[ STYLE_CNT ][12]={
				/* bg r,g,b,a     fg r,g,b,a                   enum name    */
				{0,0,0,64 ,1,1,   127,0,    0,255,0,0} ,      /* STYLE_MENU */
				{0,0,0,255,1,1,   255,255,255,255,0,0} ,      /* STYLE_WHITE */
				{0,0,0,255,1,1,   255,0,    0,255,0,0} ,      /* STYLE_RED */
				{0,0,0,255,1,1,   205,205, 50,255,0,0 }  };   /* STYLE_YELLOW */

	int i;

	for (i=0; i<STYLE_CNT  ; i++ ){
		(*s)[i].bg.r     = a[i][0];
		(*s)[i].bg.g     = a[i][1];
		(*s)[i].bg.b     = a[i][2];
		(*s)[i].bg.a     = a[i][3];
		(*s)[i].bg.x_off = a[i][4];
		(*s)[i].bg.y_off = a[i][5];

		(*s)[i].fg.r     = a[i][6];
		(*s)[i].fg.g     = a[i][7];
		(*s)[i].fg.b     = a[i][8];
		(*s)[i].fg.a     = a[i][9];
		(*s)[i].fg.x_off = a[i][10];
		(*s)[i].fg.y_off = a[i][11];

	}

	return;

}     /* end of init_all_styles() */


void init_parse_options(int argc, char **argv){
		/* Note. magick_timeout default used to be 5.  HRABAK changed to
		 * -1 (off) cause any recursive load against a large set of files
		 * that require convert, seems to hang.
		 */

	/* TODO: sort these to match declaration of __fehoptions */

	/* For setting the command hint on X windows */
	fgv.cmdargc = argc;
	fgv.cmdargv = argv;

	/* Set default options.  Note:  The memset(0) clears all to 0 */
	memset(&opt, 0, sizeof(fehoptions));
	opt.flg.display     = 1;
	opt.flg.aspect      = 1;
	opt.magick_timeout = -1;              /* old default was 5 */
	opt.thumb_w         = 60;
	opt.thumb_h         = 60;
	opt.thumb_redraw    = 10;
	opt.menu_bg = estrdup(PREFIX "/share/feh/images/menubg_default.png");
/*	opt.menu_style = estrdup(PREFIX "/share/feh/fonts/menu.style"); */
	opt.flg.write_filelist = 1;           /* write filelist on exit? Yes=1 */
	opt.flg.no_actions     = 1;           /* just need to know if we have ANY */

	init_all_styles( &opt.style );

	opt.flg.mode =MODE_SLIDESHOW;         /* this modes list is in enum order */
	opt.modes[MODE_SLIDESHOW]   = "slideshow";
	opt.modes[MODE_MULTIWINDOW] = "multiwindow";
	opt.modes[MODE_INDEX]       = "index";
	opt.modes[MODE_THUMBNAIL]   = "thumbnail";
	opt.modes[MODE_LOADABLES]   = "loadables";
	opt.modes[MODE_UNLOADABLES] = "unloadables";
	opt.modes[MODE_LIST]        = "list";
	opt.modes[MODE_MOVE]        = "move";

	opt.flg.jump_on_resort = 1;

	opt.flg.screen_clip    = 1;
#ifdef HAVE_LIBXINERAMA
	/* if we're using xinerama, then enable it by default */
	opt.flg.xinerama       = 1;
#endif       /* HAVE_LIBXINERAMA */

	feh_getopt_theme(argc, argv);

	D(("About to check for theme configuration\n"));
	feh_check_theme_options(argv);


	return;
}

static void feh_check_theme_options(char **argv)
{
	if (!fgv.theme) {
		/* This prevents screw up when running src/feh or ./feh */
		char *pos = strrchr(argv[0], '/');

		if (pos)
			fgv.theme = estrdup(pos + 1);
		else
			fgv.theme = estrdup(argv[0]);
	}
	D(("Theme name is %s\n", fgv.theme));

	feh_load_options_for_theme(fgv.theme);

	free(fgv.theme);
	return;
}

static void feh_load_options_for_theme(char *theme)
{
	FILE *fp = NULL;
	char *home      = getenv("HOME");
	char *confbase  = getenv("XDG_CONFIG_HOME");
	char *rcpath    = mobs(4);
	char *oldrcpath = mobs(4);
	char *s         = mobs(4);   /* match  MOBS_NSIZE(4) below */
	char *s1;                    /* reuse    rcpath buf*/
	char *s2;                    /* reuse oldrcpath buf*/
	int cont = 0;
	int bspos;

	if (!home)
		eprintf("You have no HOME, %sread themes",ERR_CANNOT);

	STRCAT_2ITEMS(oldrcpath,home, "/.fehrc");

	if (confbase)
		STRCAT_2ITEMS(rcpath,confbase,"/feh/themes");
	else
		STRCAT_2ITEMS(rcpath,home, "/.config/feh/themes");

	fp = fopen(rcpath, "r");

	if (!fp && ((fp = fopen(oldrcpath, "r")) != NULL))
		weprintf("The theme config file was moved from ~/.fehrc to "
			"~/.config/feh/themes. Run\n"
			"    mkdir -p ~/.config/feh; mv ~/.fehrc ~/.config/feh/themes\n"
			"to fix this.");

	if (!fp && ((fp = fopen("/etc/feh/themes", "r")) == NULL))
		return;

	/* Oooh. We have an options file :) */
  s1 = rcpath; s2 = oldrcpath;    /* just reuse existing buffers */
	for (; fgets(s, MOBS_NSIZE(4), fp);) {
		s1[0] = '\0';
		s2[0] = '\0';

		if (cont) {
			sscanf(s, " %[^\n]\n", (char *) s2);
			if (!*s2)
				break;
			D(("Got continued options %s\n", s2));
		} else {
			sscanf(s, "%s %[^\n]\n", (char *) s1, (char *) s2);
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
	int inquote = 0;
	int i = 0;

	/* So we don't reinvent the wheel (not again, anyway), we use the
	   getopt_long function to do this parsing as well. This means it has to
	   look like the real argv ;) */

	list[num++] = estrdup(PACKAGE);

	for (s = opts, t = opts;; t++) {

		if (num > 64)
			eprintf(PACKAGE " does not support more than 64 words per "
					"theme definition.\n Please shorten your lines.");

		if ((*t == ' ') && !(inquote)) {
			*t = '\0';
			num++;

			list[num - 1] = feh_string_normalize(s);
			s = t + 1;
		} else if (*t == '\0') {
			num++;

			list[num - 1] = feh_string_normalize(s);
			break;
		} else if (*t == '\"' && last != '\\')
			inquote = !(inquote);
		last = *t;
	}

	feh_parse_option_array(num, list);

	for (i = 0; i < num; i++)
		if (list[i])
			free(list[i]);
	return;
}

char *feh_string_normalize(char *str)
{
	char *ret = mobs(3);
	char *s;
	int i = 0;
	char last = 0;

	D(("normalizing %s\n", str));
	/* ret[0] = '\0'; */      /* mobs() does this */

	for (s = str;; s++) {
		if (*s == '\0')
			break;
		else if ((*s == '\"') && (last == '\\'))
			ret[i++] = '\"';
		else if ((*s == '\"') && (last == 0));
		else if ((*s == ' ') && (last == '\\'))
			ret[i++] = ' ';
		else
			ret[i++] = *s;

		last = *s;
	}
	if (i && (ret[i - 1] == '\"'))
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
			fgv.theme = estrdup(optarg);
	}

	opterr = 1;
	optind = 0;
}

void feh_parse_option_array(int argc, char **argv )
{
	static char stropts[] =
		"a:A:b:B:cC:dD:e:E:f:Fg:GhH:iIj:J:kK:lL:mM:nNo:O:pPqrR:sS:tT:uUvVwW:xXy:YzZ"
		".@:^:~:):|:#:+:";

	/* (*name, has_arg, *flag, val) See: struct option in getopts.h */
	static struct option lopts[] = {
		{"help"          , 0, 0, 'h'},
		{"version"       , 0, 0, 'v'},
		/* {"montage"       , 0, 0, 'm'}, */   /* killed jul 2012 */
		{"index"         , 0, 0, 'i'},
		{"fullindex"     , 0, 0, 'I'},
		{"verbose"       , 0, 0, 'V'},
		{"borderless"    , 0, 0, 'x'},
		{"keep-http"     , 0, 0, 'k'},
		{"stretch"       , 0, 0, 's'},
		{"multiwindow"   , 0, 0, 'w'},
		{"recursive"     , 0, 0, 'r'},
		{"randomize"     , 0, 0, 'z'},
		{"list"          , 0, 0, 'l'},
		{"quiet"         , 0, 0, 'q'},
		{"loadable"      , 0, 0, 'U'},
		{"unloadable"    , 0, 0, 'u'},
		{"no-menus"      , 0, 0, 'N'},
		{"full-screen"   , 0, 0, 'F'}, /* deprecated */
		{"fullscreen"    , 0, 0, 'F'},
		{"auto-zoom"     , 0, 0, 'Z'},
		{"ignore-aspect" , 0, 0, 'X'},
		{"draw-filename" , 0, 0, 'd'},
		{"preload"       , 0, 0, 'p'},
		{"reverse"       , 0, 0, 'n'},
		{"thumbnails"    , 0, 0, 't'},
		{"scale-down"    , 0, 0, '.'},
		{"no-jump-on-resort", 0, 0, 220},
		{"hide-pointer"  , 0, 0, 'Y'},
		{"draw-actions"  , 0, 0, 'G'},
		{"cache-thumbnails", 0, 0, 'P'},
		{"cycle-once"    , 0, 0, 224},
		{"no-xinerama"   , 0, 0, 225},
		{"draw-tinted"   , 0, 0, 229},
#ifdef HAVE_LIBEXIF
		{"draw-exif"     , 0, 0, 223},
#endif

		{"output"        , 1, 0, 'o'},
		{"output-only"   , 1, 0, 'O'},
		{"action"        , 1, 0, 'A'},
		{"limit-width"   , 1, 0, 'W'},
		{"limit-height"  , 1, 0, 'H'},
		{"reload"        , 1, 0, 'R'},
		{"alpha"         , 1, 0, 'a'},
		{"sort"          , 1, 0, 'S'},
		{"theme"         , 1, 0, 'T'},
		{"filelist"      , 1, 0, 'f'},
		{"customlist"    , 1, 0, 'L'},
		{"geometry"      , 1, 0, 'g'},
		{"menu-font"     , 1, 0, 'M'},
		{"thumb-width"   , 1, 0, 'y'},
		{"thumb-height"  , 1, 0, 'E'},
		{"slideshow-delay",1, 0, 'D'},
		{"font"          , 1, 0, 'e'},
		{"title-font"    , 1, 0, '@'},
		{"title"         , 1, 0, '^'},
		{"thumb-title"   , 1, 0, '~'},
		{"bg"            , 1, 0, 'b'},
		{"fontpath"      , 1, 0, 'C'},
		{"menu-bg"       , 1, 0, ')'},
		{"image-bg"      , 1, 0, 'B'},
		{"start-at"      , 1, 0, '|'},
		{"start-at-num"  , 1, 0, '#'},      /* same as start-at but @ number, not name */
		{"debug"         , 0, 0, '+'},
		{"output-dir"    , 1, 0, 'j'},
		{"bg-tile"       , 0, 0, 200},
		{"bg-center"     , 0, 0, 201},
		{"bg-scale"      , 0, 0, 202},
		{"menu-style"    , 1, 0, 204},
		{"zoom"          , 1, 0, 205},
		{"no-screen-clip", 0, 0, 206},
		{"index-info"    , 1, 0, 207},
		{"magick-timeout", 1, 0, 208},
		{"caption-path"  , 1, 0, 'K'},
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
		{"thumb-redraw"  , 1, 0, 'J'},
		{"info"          , 1, 0, 234},
		{"force-aliasing", 0, 0, 235},
		{"no-fehbg"      , 0, 0, 236},
		{"nowrite-filelist" , 0, 0, 237}, /* don't write filelist at the end      */
		{"dname"         , 0, 0, 238},    /* like -d but only name                */
		{"dno-ext"       , 0, 0, 239},    /* like --dname but no extension either */

		{0, 0, 0, 0}
	};
	int optch = 0, cmdx = 0;

	/* Now to pass some optionarinos */
	while ((optch = getopt_long(argc, argv, stropts, lopts, &cmdx)) != EOF) {
		D(("Got option, getopt calls it %d, or %c\n", optch, optch));
		switch (optch) {
		case 0:
			break;
		case 'h':
			show_usage();
			break;
		case 'v':
			show_version();
			break;
		case 'm':
			opt.flg.index = 1;
			break;
		case 'i':
			opt.flg.index = 1;
			opt.index_info = estrdup("%n");
			break;
		case '.':
			opt.flg.scale_down = 1;
			break;
		case 'I':
			opt.flg.index = 1;
			opt.index_info = estrdup("%n\n%S\n%wx%h");
			break;
		case 'l':
			opt.flg.list = 1;
			opt.flg.display = 0;
			break;
		case 'L':
			opt.customlist = estrdup(optarg);
			opt.flg.display = 0;
			break;
		case 'M':
			opt.fn_menu.name = estrdup(optarg);
			break;
		case '+':
			opt.debug = 1;
			break;
		case 'n':
			opt.flg.reverse = 1;
			break;
		case 'g':
			opt.geom_flags = XParseGeometry(optarg, &opt.geom_x, &opt.geom_y, &opt.geom_w, &opt.geom_h);
			break;
		case 'N':
			opt.flg.no_menus = 1;
			break;
		case 'V':
			opt.flg.verbose = 1;
			break;
		case 'q':
			opt.flg.quiet = 1;
			break;
		case 'x':
			opt.flg.borderless = 1;
			break;
		case 'k':
			opt.flg.keep_http = 1;
			break;
		case 's':
			opt.flg.stretch = 1;
			break;
		case 'w':
			opt.flg.multiwindow = 1;
			break;
		case 'r':
			opt.flg.recursive = 1;
			break;
		case 'z':
			opt.flg.randomize = 1;
			break;
		case 'd':
			opt.flg.draw_filename = 1;
			break;
		case 'F':
			opt.flg.full_screen = 1;
			break;
		case 'Z':
			opt.zoom_mode = ZOOM_MODE_MAX;
			break;
		case 'U':
			opt.flg.loadables = 1;
			opt.flg.display = 0;
			break;
		case 'u':
			opt.flg.unloadables = 1;
			opt.flg.display = 0;
			break;
		case 'p':
			opt.flg.preload = 1;
			break;
		case 'X':
			opt.flg.aspect = 0;
			break;
		case 'S':
			if (!strcasecmp(optarg, "name"))
				opt.sort = SORT_NAME;
			else if (!strcasecmp(optarg, "filename"))
				opt.sort = SORT_FILENAME;
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
			break;
		case 'o':
			opt.output_file = estrdup(optarg);
			break;
		case 'O':
			opt.output_file = estrdup(optarg);
			opt.flg.display = 0;
			break;
		case 'T':
			fgv.theme = estrdup(optarg);
			break;
		case 'C':
			D(("adding fontpath %s\n", optarg));
			imlib_add_path_to_font_path(optarg);
			break;
		case 'e':
			opt.fn_dflt.name = estrdup(optarg);
			break;
		case '@':
			opt.fn_title.name = estrdup(optarg);
			break;
		case '^':
			opt.title = estrdup(optarg);
			break;
		case '~':
			opt.thumb_title = estrdup(optarg);
			break;
		case 'b':
			opt.flg.bg = 1;
			opt.bg_file = estrdup(optarg);
			break;
		case 'A':
			opt.actions[0] = estrdup(optarg);
			break;
		case 'W':
			opt.limit_w = atoi(optarg);
			break;
		case 'H':
			opt.limit_h = atoi(optarg);
			break;
		case 'y':
			opt.thumb_w = atoi(optarg);
			break;
		case 'E':
			opt.thumb_h = atoi(optarg);
			break;

/*		case ')':
			free(opt.menu_bg);
			opt.menu_bg = estrdup(optarg);
			weprintf("The --menu-bg option is deprecated and will be removed by 2012");
			break;
*/
		case 'B':
			if (!strcmp(optarg, "checks"))
				opt.flg.image_bg = IMAGE_BG_CHECKS;
			else if (!strcmp(optarg, "white"))
				opt.flg.image_bg = IMAGE_BG_WHITE;
			else if (!strcmp(optarg, "black"))
				opt.flg.image_bg = IMAGE_BG_BLACK;
			else
				weprintf("Unknown argument to --image-bg: %s", optarg);
			break;
		case 'D':
			opt.slideshow_delay = atof(optarg);
			if (opt.slideshow_delay < 0.0) {
				opt.slideshow_delay *= (-1);
				opt.flg.paused = 1;
			}
			break;
		case 'R':
			opt.reload = atoi(optarg);
			break;
		case 'a':
			opt.flg.alpha = 1;
			opt.alpha_level = 255 - atoi(optarg);
			break;
		case 'f':
			if (!strcmp(optarg, "-"))
				opt.filelistfile = estrdup("/dev/stdin");
			else
				opt.filelistfile = estrdup(optarg);
			break;
		case '|':
			opt.start_at_name = estrdup(optarg);
			break;
		case '#':
			opt.start_at_num = atoi(optarg);
			break;
		case 't':
			opt.flg.thumbnail = 1;
			opt.index_info = estrdup("%n");
			break;
		case 'j':
			opt.output_dir = estrdup(optarg);
			break;
		case 200:
			opt.flg.bgmode = CB_BG_TILED;
			break;
		case 201:
			opt.flg.bgmode = CB_BG_CENTERED;
			break;
		case 202:
			opt.flg.bgmode = CB_BG_SCALED;
			break;
		case 218:
			opt.flg.bgmode = CB_BG_FILLED;
			break;
		case 219:
			opt.flg.bgmode = CB_BG_MAX;
			break;
/*
		case 204:
			free(opt.menu_style);
			opt.menu_style = estrdup(optarg);
			weprintf("The --menu-style option is deprecated and will be removed by 2012");
			break;
*/
		case 205:
			if (!strcmp("fill", optarg))
				opt.zoom_mode = ZOOM_MODE_FILL;
			else if (!strcmp("max", optarg))
				opt.zoom_mode = ZOOM_MODE_MAX;
			else
				opt.default_zoom = atoi(optarg);
			break;
		case 206:
			opt.flg.screen_clip = 0;
			break;
		case 207:
			opt.index_info = estrdup(optarg);
			break;
		case 208:
			opt.magick_timeout = atoi(optarg);
			break;
		case 'K':
			opt.caption_path = estrdup(optarg);
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
		case 220:
			opt.flg.jump_on_resort = 0;
			break;
		case 'Y':
			opt.flg.hide_pointer = 1;
			break;
		case 'G':
			opt.flg.draw_actions = 1;
			break;
		case 'P':
			opt.flg.cache_thumbnails = 1;
			break;
#ifdef HAVE_LIBEXIF
		case 223:
			opt.flg.draw_exif = 1;
			break;
#endif
		case 224:
			opt.flg.cycle_once = 1;
			break;
		case 225:
			opt.flg.xinerama = 0;
			break;
		case 229:
			opt.flg.text_bg = TEXT_BG_TINTED;
			break;
		case 'J':
			opt.thumb_redraw = atoi(optarg);
			break;
		case 234:
			opt.info_cmd = estrdup(optarg);
			opt.flg.draw_info = 1;
			break;
		case 235:
			opt.force_aliasing = 1;
			break;
		case 236:
			opt.flg.no_fehbg = 1;
			break;
		case 237:
			opt.flg.write_filelist = 0;
			break;
		case 238:
			opt.flg.draw_filename = opt.flg.draw_name = 1;
			break;
		case 239:
			opt.flg.draw_filename = opt.flg.draw_name = opt.flg.draw_no_ext = 1;
			break;
		default:
			break;
		}
	}

	/* Now the leftovers, which must be files */
	reload_logic( argc, argv, optind );

	/* So that we can safely be called again */
	optind = 0;
	return;
}    /* end of feh_parse_option_array() */

void check_options(void)
{
	int i;
	for (i = 0; i < 10; i++) {
		if (opt.actions[i] ) {
			opt.flg.no_actions =0;      /* just need to know if we have ANY */
			break;
		}
	}

	if (opt.flg.full_screen && opt.flg.multiwindow) {
		eprintf("%s%sfullscreen with --multiwindow",ERR_CANNOT, ERR_COMBINE);
	}

	if (opt.flg.list && (opt.flg.multiwindow || opt.flg.index  )) {
		eprintf("%s%slist with other modes",ERR_CANNOT, ERR_COMBINE);
	}

	if (opt.sort && opt.flg.randomize) {
		weprintf("%s%ssort AND randomize! randomize was unset\n",ERR_CANNOT, ERR_COMBINE);
		opt.flg.randomize = 0;
	}

	if (opt.flg.loadables && opt.flg.unloadables) {
		eprintf("%s%sloadable with --unloadable",ERR_CANNOT, ERR_COMBINE);
	}

	if (opt.start_at_name && opt.start_at_num ) {
		eprintf("%s%sstart-at with --start-at-num",ERR_CANNOT, ERR_COMBINE);
	}

	/* allow a %z fmt spec to disable ANY info display */
	if ( opt.index_info ){
		if ( !opt.index_info[0] ){          /* empty string     */
			opt.index_info=NULL;              /* this is the flag */
		} else {
			for (i = 1; opt.index_info[i] != 0; i++) {
				if (opt.index_info[i] == 'z')
					if (opt.index_info[i-1] == '%') {
						opt.index_info=NULL;        /* this is the flag */
						break;
					}
			}
		}
	}

	/* Apr 2013 HRABAK changed the whole idea of modes in feh to simplify
	 * the "on the fly" toggling between "modes".  Historically, feh used
	 * the opt flags (like opt.slideshow, opt.thumbnail etc) to determine what
	 * "mode" it was in, yet the word "mode" held the name of that mode.
	 * The HRABAK change uses the individual opt flags just to load the
	 * options passed on the command line.  Then I change to a opt.mode
	 * value (the individual opt flags are no longer used after this point).
	 * and the rest of feh behavior is based on this new opt.mode setting.
	 * The opt.mode_original keeps a copy of the original setting, so you can
	 * toggle to slideshow mode (and move_mode) and back again.
	 * MODE_SLIDESHOW is the odd-ball.  There is no command line param to
	 * request slideshow - it is the default.  So MODE_SLIDESHOW is defined
	 * as zero, so if any other modes are spec'd, slideshow gets over ridden.
	 * Only one mode survives this conv from opt flags to opt.mode, so the
	 * last one in this list wins in the event they request multiples.
	 *
	 * opt.flg.mode controls both the mode behavior AND the coresponding text
	 * display.  The first four modes in this hierarchy are the display modes,
	 * ie the ones you can toggle between "on the fly".  The last three modes
	 * are blind modes (no display).
	 */

	 /* set the default       */ opt.flg.mode = MODE_SLIDESHOW;
	 /* if (opt.slideshow_delay   ) opt.flg.mode = MODE_SLIDESHOW; */  /* redundant */
	 if (opt.flg.multiwindow   ) opt.flg.mode = MODE_MULTIWINDOW;
	 if (opt.flg.index         ) opt.flg.mode = MODE_INDEX;
	 if (opt.flg.thumbnail     ) opt.flg.mode = MODE_THUMBNAIL;

	/* the blind set */
	 if (opt.flg.loadables     ) opt.flg.mode = MODE_LOADABLES;
	 if (opt.flg.unloadables   ) opt.flg.mode = MODE_UNLOADABLES;
	 if (opt.flg.list || opt.customlist  ) opt.flg.mode = MODE_LIST;

	/* save the original "mode" to toggle back to.  If mode is ONLY slideshow
	 * then set the mode_original to allow toggling into thumb mode
	 */
	opt.flg.mode_original = ( opt.flg.mode == MODE_SLIDESHOW ) ?
	                          MODE_THUMBNAIL : opt.flg.mode ;


	return;
}   /* end of check_options() */

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

#if _FILE_OFFSET_BITS == 64
		"stat64 "
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
