/* options.c

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

#include "feh.h"
#include "filelist.h"
#include "options.h"

static void check_options(void);
static void feh_parse_option_array(int argc, char **argv);
static void feh_parse_environment_options(void);
static void feh_check_theme_options(int arg, char **argv);
static void feh_parse_options_from_string(char *opts);
static void feh_load_options_for_theme(char *theme);
static char *theme;

fehoptions opt;

void
init_parse_options(int argc, char **argv)
{
   D_ENTER(4);

   /* For setting the command hint on X windows */
   cmdargc = argc;
   cmdargv = argv;

   /* Set default options */
   memset(&opt, 0, sizeof(fehoptions));
   opt.display = 1;
   opt.aspect = 1;
   opt.slideshow_delay = -1.0;
   opt.thumb_w = 60;
   opt.thumb_h = 60;
   opt.menu_font = estrdup(DEFAULT_MENU_FONT);
   opt.font = estrdup(DEFAULT_FONT);
   opt.menu_bg = estrdup(PREFIX "/share/feh/images/menubg_default.png");
   opt.menu_style = estrdup(PREFIX "/share/feh/fonts/menu.style");
   opt.menu_border = 4;

   opt.next_button = 1;
   opt.zoom_button = 2;
   opt.menu_button = 3;
   opt.menu_ctrl_mask = 0;
   opt.reload_button = 0;

   opt.rotate_button = 2;
   opt.no_rotate_ctrl_mask = 0;
   opt.blur_button = 1;
   opt.no_blur_ctrl_mask = 0;

   opt.no_jump_on_resort = 0;

   opt.builtin_http = 0;

   opt.xinerama = 0;
   opt.screen_clip = 1;
#ifdef HAVE_LIBXINERAMA
   /* if we're using xinerama, then enable it by default */
   opt.xinerama = 1;
#endif /* HAVE_LIBXINERAMA */

   opt.fmmode = 0;
   
   D(3, ("About to parse env options (if any)\n"));
   /* Check for and parse any options in FEH_OPTIONS */
   feh_parse_environment_options();

   D(3, ("About to parse commandline options\n"));
   /* Parse the cmdline args */
   feh_parse_option_array(argc, argv);

   D(3, ("About to check for theme configuration\n"));
   feh_check_theme_options(argc, argv);

   /* If we have a filelist to read, do it now */
   if (opt.filelistfile)
   {
      /* joining two reverse-sorted lists in this manner works nicely for us
         here, as files specified on the commandline end up at the *end* of
         the combined filelist, in the specified order. */
      D(3, ("About to load filelist from file\n"));
      filelist = gib_list_cat(filelist, feh_read_filelist(opt.filelistfile));
   }

   D(4, ("Options parsed\n"));

   if(opt.bgmode)
      D_RETURN_(4);

   filelist_len = gib_list_length(filelist);
   if (!filelist_len)
      show_mini_usage();

   check_options();

   feh_prepare_filelist();
   D_RETURN_(4);
}

static void
feh_check_theme_options(int arg, char **argv)
{
   D_ENTER(4);
   if (!theme)
   {
      /* This prevents screw up when running src/feh or ./feh */
      char *pos = strrchr(argv[0], '/');

      if (pos)
         theme = estrdup(pos + 1);
      else
         theme = estrdup(argv[0]);
   }
   D(3, ("Theme name is %s\n", theme));

   feh_load_options_for_theme(theme);

   free(theme);
   D_RETURN_(4);
   arg = 0;
}

static void
feh_load_options_for_theme(char *theme)
{
   FILE *fp = NULL;
   char *home;
   char *rcpath = NULL;
   char s[1024], s1[1024], s2[1024];

   D_ENTER(4);

   if (opt.rcfile)
   {
      if ((fp = fopen(opt.rcfile, "r")) == NULL)
      {
         weprintf("couldn't load the specified rcfile %s\n", opt.rcfile);
         D_RETURN_(4);
      }
   }
   else
   {
      home = getenv("HOME");
      if (!home)
         eprintf("D'oh! Please define HOME in your environment!"
                 "It would really help me out...\n");
      rcpath = estrjoin("/", home, ".fehrc", NULL);
      D(3, ("Trying %s for config\n", rcpath));
      fp = fopen(rcpath, "r");

      if (!fp && ((fp = fopen("/etc/fehrc", "r")) == NULL))
      {
         feh_create_default_config(rcpath);

         if ((fp = fopen(rcpath, "r")) == NULL)
            D_RETURN_(4);
      }

      free(rcpath);
   }

   /* Oooh. We have an options file :) */
   for (; fgets(s, sizeof(s), fp);)
   {
      s1[0] = '\0';
      s2[0] = '\0';
      sscanf(s, "%s %[^\n]\n", (char *) &s1, (char *) &s2);
      if (!(*s1) || (!*s2) || (*s1 == '\n') || (*s1 == '#'))
         continue;
      D(5, ("Got theme/options pair %s/%s\n", s1, s2));
      if (!strcmp(s1, theme))
      {
         D(4, ("A match. Using options %s\n", s2));
         feh_parse_options_from_string(s2);
         break;
      }
   }
   fclose(fp);
   D_RETURN_(4);
}

static void
feh_parse_environment_options(void)
{
   char *opts;

   D_ENTER(4);

   if ((opts = getenv("FEH_OPTIONS")) == NULL)
      D_RETURN_(4);

   weprintf
      ("The FEH_OPTIONS configuration method is depreciated and will soon die.\n"
       "Use the .fehrc configuration file instead.");

   /* We definitely have some options to parse */
   feh_parse_options_from_string(opts);
   D_RETURN_(4);
}

/* FIXME This function is a crufty bitch ;) */
static void
feh_parse_options_from_string(char *opts)
{
   char **list = NULL;
   int num = 0;
   char *s;
   char *t;
   char last = 0;
   int inquote = 0;
   int i = 0;

   D_ENTER(4);

   /* So we don't reinvent the wheel (not again, anyway), we use the
      getopt_long function to do this parsing as well. This means it has to
      look like the real argv ;) */

   list = malloc(sizeof(char *));

   list[num++] = estrdup(PACKAGE);

   for (s = opts, t = opts;; t++)
   {
      if ((*t == ' ') && !(inquote))
      {
         *t = '\0';
         num++;
         list = erealloc(list, sizeof(char *) * num);

         list[num - 1] = feh_string_normalize(s);
         s = t + 1;
      }
      else if (*t == '\0')
      {
         num++;
         list = erealloc(list, sizeof(char *) * num);

         list[num - 1] = feh_string_normalize(s);
         break;
      }
      else if (*t == '\"' && last != '\\')
         inquote = !(inquote);
      last = *t;
   }

   feh_parse_option_array(num, list);

   for (i = 0; i < num; i++)
      if (list[i])
         free(list[i]);
   if (list)
      free(list);
   D_RETURN_(4);
}

char *
feh_string_normalize(char *str)
{
   char ret[4096];
   char *s;
   int i = 0;
   char last = 0;

   D_ENTER(4);
   D(4, ("normalizing %s\n", str));
   ret[0] = '\0';

   for (s = str;; s++)
   {
      if (*s == '\0')
         break;
      else if ((*s == '\"') && (last == '\\'))
         ret[i++] = '\"';
      else if ((*s == '\"') && (last == 0))
         ;
      else if ((*s == ' ') && (last == '\\'))
         ret[i++] = ' ';
      else
         ret[i++] = *s;

      last = *s;
   }
   if (i && ret[i - 1] == '\"')
      ret[i - 1] = '\0';
   else
      ret[i] = '\0';
   D(4, ("normalized to %s\n", ret));

   D_RETURN(4, estrdup(ret));
}

static void
feh_parse_option_array(int argc, char **argv)
{
   static char stropts[] =
      "a:A:b:BcC:dD:e:E:f:Fg:GhH:iIj:klL:mM:nNo:O:pqQrR:sS:tT:uUvVwW:xXy:zZ1:2:4:56:78:90:.@:^:~:):|:_:+:";
   static struct option lopts[] = {
      /* actions */
      {"help", 0, 0, 'h'},                  /* okay */
      {"version", 0, 0, 'v'},               /* okay */
      /* toggles */
      {"montage", 0, 0, 'm'},               /* okay */
      {"collage", 0, 0, 'c'},               /* okay */
      {"index", 0, 0, 'i'},                 /* okay */
      {"fullindex", 0, 0, 'I'},             /* okay */
      {"verbose", 0, 0, 'V'},               /* okay */
      {"borderless", 0, 0, 'x'},            /* okay */
      {"keep-http", 0, 0, 'k'},             /* okay */
      {"stretch", 0, 0, 's'},               /* okay */
      {"multiwindow", 0, 0, 'w'},           /* okay */
      {"recursive", 0, 0, 'r'},             /* okay */
      {"randomize", 0, 0, 'z'},             /* okay */
      {"list", 0, 0, 'l'},                  /* okay */
      {"quiet", 0, 0, 'q'},                 /* okay */
      {"loadables", 0, 0, 'U'},             /* okay */
      {"unloadables", 0, 0, 'u'},           /* okay */
      {"no-menus", 0, 0, 'N'},
      {"full-screen", 0, 0, 'F'},
      {"auto-zoom", 0, 0, 'Z'},
      {"ignore-aspect", 0, 0, 'X'},
      {"draw-filename", 0, 0, 'd'},
      {"preload", 0, 0, 'p'},
      {"reverse", 0, 0, 'n'},
      {"thumbnails", 0, 0, 't'},
      {"wget-timestamp", 0, 0, 'G'},
      {"builtin", 0, 0, 'Q'},
      {"menu-ctrl-mask", 0, 0, '5'},        /* okay */
      {"scale-down", 0, 0, '.'},            /* okay */
      {"no-rotate-ctrl-mask", 0, 0, '7'},
      {"no-blur-ctrl-mask", 0, 0, '9'},
      {"no-jump-on-resort",0,0,220},
      {"hide-pointer",0,0,221},
      /* options with values */
      {"output", 1, 0, 'o'},                /* okay */
      {"output-only", 1, 0, 'O'},           /* okay */
      {"action", 1, 0, 'A'},                /* okay */
      {"limit-width", 1, 0, 'W'},           /* okay */
      {"limit-height", 1, 0, 'H'},          /* okay */
      {"reload", 1, 0, 'R'},                /* okay */
      {"alpha", 1, 0, 'a'},                 /* okay */
      {"sort", 1, 0, 'S'},                  /* okay */
      {"theme", 1, 0, 'T'},                 /* okay */
      {"filelist", 1, 0, 'f'},              /* okay */
      {"customlist", 1, 0, 'L'},            /* okay */
      {"geometry", 1, 0, 'g'},              /* okay */
      {"menu-font", 1, 0, 'M'},
      {"thumb-width", 1, 0, 'y'},
      {"thumb-height", 1, 0, 'E'},
      {"slideshow-delay", 1, 0, 'D'},
      {"font", 1, 0, 'e'},
      {"title-font", 1, 0, '@'},
      {"title", 1, 0, '^'},
      {"thumb-title", 1, 0, '~'},
      {"bg", 1, 0, 'b'},
      {"fontpath", 1, 0, 'C'},
      {"menu-bg", 1, 0, ')'},
      {"next-button", 1, 0, '1'},
      {"zoom-button", 1, 0, '2'},
      {"menu-button", 1, 0, '4'},
      {"rotate-button", 1, 0, '6'},
      {"blur-button", 1, 0, '8'},
      {"reload-button", 1, 0, '0'},
      {"start-at", 1, 0, '|'},
      {"rcfile", 1, 0, '_'},
      {"debug-level", 1, 0, '+'},
      {"output-dir", 1, 0, 'j'},
      {"bg-tile", 1, 0, 200},
      {"bg-center", 1, 0, 201},
      {"bg-scale", 1, 0, 202},
      {"bg-seamless", 1, 0, 203},
      {"menu-style", 1, 0, 204},
      {"zoom", 1, 0, 205},
      {"xinerama", 1, 0, 206},
      {"screen-clip", 1, 0, 207},
      {"menu-border", 1, 0, 208},
      {"caption-path", 1, 0, 209},
      {"action1", 1, 0, 210},
      {"action2", 1, 0, 211},
      {"action3", 1, 0, 212},
      {"action4", 1, 0, 213},
      {"action5", 1, 0, 214},
      {"action6", 1, 0, 215},
      {"action7", 1, 0, 216},
      {"action8", 1, 0, 217},
      {"action9", 1, 0, 218},
      {"fmmode", 0, 0, 219},
      {"draw-actions", 0, 0, 222},
      {"cache-thumbnails", 0, 0, 223},
      {"cycle-once", 0, 0, 224},
      {0, 0, 0, 0}
   };
   int optch = 0, cmdx = 0, i = 0;
   int geomret;

   D_ENTER(4);

   /* Now to pass some optionarinos */
   while ((optch = getopt_long(argc, argv, stropts, lopts, &cmdx)) != EOF)
   {
      D(5, ("Got option, getopt calls it %d, or %c\n", optch, optch));
      switch (optch)
      {
        case 0:
           break;
        case 'h':
           show_usage();
           break;
        case 'v':
           show_version();
           break;
        case 'm':
           opt.index = 1;
           opt.index_show_name = 0;
           opt.index_show_size = 0;
           opt.index_show_dim = 0;
           break;
        case 'c':
           opt.collage = 1;
           break;
        case 'i':
           opt.index = 1;
           opt.index_show_name = 1;
           opt.index_show_size = 0;
           opt.index_show_dim = 0;
           break;
        case '.':
           opt.scale_down = 1;
           break;
        case 'I':
           opt.index = 1;
           opt.index_show_name = 1;
           opt.index_show_size = 1;
           opt.index_show_dim = 1;
           break;
        case 'l':
           opt.list = 1;
           break;
        case 'G':
           opt.wget_timestamp = 1;
           break;
        case 'Q':
           opt.builtin_http = 1;
           break;
        case 'L':
           opt.customlist = estrdup(optarg);
           break;
        case 'M':
           free(opt.menu_font);
           opt.menu_font = estrdup(optarg);
           break;
        case '+':
           opt.debug_level = atoi(optarg);
           break;
        case 'n':
           opt.reverse = 1;
           break;
        case 'g':
           opt.geom_flags = XParseGeometry(optarg, &opt.geom_x, &opt.geom_y, &opt.geom_w, &opt.geom_h);
           break;
        case 'N':
           opt.no_menus = 1;
           break;
        case 'V':
           opt.verbose = 1;
           break;
        case 'q':
           opt.quiet = 1;
           break;
        case 'x':
           opt.borderless = 1;
           break;
        case 'k':
           opt.keep_http = 1;
           break;
        case 's':
           opt.stretch = 1;
           break;
        case 'w':
           opt.multiwindow = 1;
           break;
        case 'r':
           opt.recursive = 1;
           break;
        case 'z':
           opt.randomize = 1;
           break;
        case 'd':
           opt.draw_filename = 1;
           break;
        case 'F':
           opt.full_screen = 1;
           break;
        case 'Z':
           opt.auto_zoom = 1;
           break;
        case 'U':
           opt.loadables = 1;
           break;
        case 'u':
           opt.unloadables = 1;
           break;
        case 'p':
           opt.preload = 1;
           break;
        case 'X':
           opt.aspect = 0;
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
           else
           {
              weprintf
                 ("Unrecognised sort mode \"%s\". Defaulting to sort by filename",
                  optarg);
              opt.sort = SORT_FILENAME;
           }
           break;
        case 'o':
           opt.output = 1;
           opt.output_file = estrdup(optarg);
           break;
        case 'O':
           opt.output = 1;
           opt.output_file = estrdup(optarg);
           opt.display = 0;
           break;
        case 'T':
           theme = estrdup(optarg);
           break;
        case 'C':
           D(3, ("adding fontpath %s\n", optarg));
           imlib_add_path_to_font_path(optarg);
           break;
        case 'e':
           opt.font = estrdup(optarg);
           break;
        case '@':
           opt.title_font = estrdup(optarg);
           break;
        case '^':
           opt.title = estrdup(optarg);
           break;
        case '~':
           opt.thumb_title = estrdup(optarg);
           break;
        case 'b':
           opt.bg = 1;
           opt.bg_file = estrdup(optarg);
           break;
        case '_':
           opt.rcfile = estrdup(optarg);
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
        case ')':
           free(opt.menu_bg);
           opt.menu_bg = estrdup(optarg);
           break;
        case 'D':
           opt.slideshow_delay = atof(optarg);
           break;
        case 'R':
           opt.reload = atoi(optarg);
           break;
        case 'a':
           opt.alpha = 1;
           opt.alpha_level = 255 - atoi(optarg);
           break;
        case 'f':
           opt.filelistfile = estrdup(optarg);
           break;
        case '1':
           opt.next_button = atoi(optarg);
           break;
        case '2':
           opt.zoom_button = atoi(optarg);
           break;
        case '4':
           opt.menu_button = atoi(optarg);
           break;
        case '5':
           opt.menu_ctrl_mask = 1;
           break;
        case '6':
           opt.rotate_button = atoi(optarg);
           break;
        case '7':
           opt.no_rotate_ctrl_mask = 1;
           break;
        case '8':
           opt.blur_button = atoi(optarg);
           break;
        case '9':
           opt.no_blur_ctrl_mask = 1;
           break;
        case '|':
           opt.start_list_at = atoi(optarg);
           break;
        case '0':
           opt.reload_button = atoi(optarg);
           break;
        case 't':
           opt.thumbs = 1;
           opt.index_show_name = 1;
           opt.index_show_size = 0;
           opt.index_show_dim = 0;
           break;
        case 'j':
           opt.output_dir = estrdup(optarg);
           break;
        case 200:
           opt.bgmode = BG_MODE_TILE;
           opt.output_file = estrdup(optarg);
           break;
        case 201:
           opt.bgmode = BG_MODE_CENTER;
           opt.output_file = estrdup(optarg);
           break;
        case 202:
           opt.bgmode = BG_MODE_SCALE;
           opt.output_file = estrdup(optarg);
           break;
        case 203:
           opt.bgmode = BG_MODE_SEAMLESS;
           opt.output_file = estrdup(optarg);
           break;
        case 204:
           free(opt.menu_style);
           opt.menu_style = estrdup(optarg);
           break;
        case 205:
           opt.default_zoom = atoi(optarg);
           break;
        case 206:
           opt.xinerama = atoi(optarg);
           break;
        case 207:
           opt.screen_clip = atoi(optarg);
           break;
        case 208:
           opt.menu_border = atoi(optarg);
           break; 
        case 209:
           opt.caption_path = estrdup(optarg);
           break; 
        case 210:
           opt.actions[1] = estrdup(optarg);
           break; 
        case 211:
           opt.actions[2] = estrdup(optarg);
           break; 
        case 212:
           opt.actions[3] = estrdup(optarg);
           break; 
        case 213:
           opt.actions[4] = estrdup(optarg);
           break; 
        case 214:
           opt.actions[5] = estrdup(optarg);
           break; 
        case 215:
           opt.actions[6] = estrdup(optarg);
           break; 
        case 216:
           opt.actions[7] = estrdup(optarg);
           break; 
        case 217:
           opt.actions[8] = estrdup(optarg);
           break; 
        case 218:
           opt.actions[9] = estrdup(optarg);
           break; 
        case 220:
           opt.no_jump_on_resort = 1;
           break;
        case 221:
           opt.hide_pointer = 1;
           break;
        case 219:
           opt.fmmode = 1; 
           opt.sort = SORT_FILENAME;
           break;
        case 222:
           opt.draw_actions = 1; 
           break;
        case 223:
           opt.cache_thumbnails = 1;
           break;
        case 224:
           opt.cycle_once = 1;
           break;
        default:
           break;
      }
   }

   /* Now the leftovers, which must be files */
   if (optind < argc)
   {
      while (optind < argc)
      {
         /* If recursive is NOT set, but the only argument is a directory
            name, we grab all the files in there, but not subdirs */
         add_file_to_filelist_recursively(argv[optind++], FILELIST_FIRST);
      }
   }

   /* So that we can safely be called again */
   optind = 1;
   D_RETURN_(4);
}


static void
check_options(void)
{
   D_ENTER(4);
   if ((opt.index + opt.collage) > 1)
   {
      weprintf("you can't use collage mode and index mode together.\n"
               "   I'm going with index");
      opt.collage = 0;
   }

   if (opt.full_screen && opt.multiwindow)
   {
      weprintf
         ("you shouldn't combine multiwindow mode with full-screen mode,\n"
          "   Multiwindow mode has been disabled.");
      opt.multiwindow = 0;
   }

   if (opt.list && (opt.multiwindow || opt.index || opt.collage))
   {
      weprintf("list mode can't be combined with other processing modes,\n"
               "   list mode disabled.");
      opt.list = 0;
   }

   if (opt.sort && opt.randomize)
   {
      weprintf("You cant sort AND randomize the filelist...\n"
               "randomize mode has been unset\n");
      opt.randomize = 0;
   }

   if (opt.loadables && opt.unloadables)
   {
      weprintf("You cant show loadables AND unloadables...\n"
               "you might as well use ls ;)\n"
               "loadables only will be shown\n");
      opt.unloadables = 0;
   }

   if (opt.thumb_title && (!opt.thumbs))
   {
      weprintf("Doesn't make sense to set thumbnail title when not in\n"
               "thumbnail mode.\n");
      free(opt.thumb_title);
      opt.thumb_title = NULL;
   }
   D_RETURN_(4);
}

void
show_version(void)
{
   printf(PACKAGE " version " VERSION "\n");
   exit(0);
}

void
show_mini_usage(void)
{
   fprintf(stdout,
           PACKAGE " - No loadable images specified.\nUse " PACKAGE
           " --help for detailed usage information\n");
   exit(0);
}

void
show_usage(void)
{
   fprintf(stdout,
"Usage : " PACKAGE " [OPTIONS]... FILES...\n"
" Where a FILE is an image file or a directory.\n"
" Multiple files are supported.\n"
" Urls are supported. They must begin with http:// or ftp:// and you must\n"
" have wget installed to download the files for viewing.\n"
" Options can also be specified in the in the feh configuration file. See\n"
" man feh for more details\n"
" -h, --help                display this help and exit\n"
" -v, --version             output version information and exit\n"
" -V, --verbose             output useful information, progress bars, etc\n"
" -q, --quiet               Don't report non-fatal errors for failed loads\n"
"                           Verbose and quiet modes are not mutually exclusive,\n"
"                           the first controls informational messages, the\n"
"                           second only errors.\n"
" -T, --theme THEME         Load options from config file with name THEME\n"
"                           see man feh for more info.\n"
"     --rcfile FILE         Use FILE to parse themes and options from,\n"
"                           instead of the default ~/.fehrc, /etc/fehrc files.\n"
" -r, --recursive           Recursively expand any directories in FILE to\n"
"                           the content of those directories. (Take it easy)\n"
" -z, --randomize           When viewing multiple files in a slideshow,\n"
"                           randomise the file list before displaying\n"
" --no-jump-on-resort       Don't jump to the first image when the filelist\n"
"                           is resorted.\n"
" -g, --geometry STRING     Limit (and don't change) the window size. Takes\n"
"                           an X-style geometry string like 640x480.\n"
"                           Note that larger images will be zoomed out to fit\n"
"                           but you can see them at 1:1 by clicking the zoom\n"
"                           button.\n"
" -f, --filelist FILE       This option is similar to the playlists used by\n"
"                           music software. If FILE exists, it will be read\n"
"                           for a list of files to load, in the order they\n"
"                           appear. The format is a list of image filenames,\n"
"                           absolute or relative to the current directory,\n"
"                           one filename per line.\n"
"                           If FILE doesn't exist, it will be created from the\n"
"                           internal filelist at the end of a viewing session.\n"
"                           This is best used to store the results of complex\n"
"                           sorts (-Spixels for example) for later viewing.\n"
"                           Any changes to the internal filelist (such as\n"
"                           deleting a file or it being pruned for being\n"
"                           unloadable) will be saved to FILE when feh exits.\n"
"                           You can add files to filelists by specifying them\n"
"                           on the commandline when also specifying the list.\n"
" -p, --preload             Preload images. This doesn't mean hold them in\n"
"                           RAM, it means run through and eliminate unloadable\n"
"                           images first. Otherwise they will be removed as you\n"
"                           flick through.\n"
"     --scale-down          Automatically scale down images too big for the\n"
"                           screen. Currently only works with -P\n"
" -F, --full-screen         Make the window fullscreen\n"
" -Z, --auto-zoom           Zoom picture to screen size in fullscreen mode,\n"
"                           is affected by the option --stretch\n"
"     --zoom PERCENT        Zooms images by a PERCENT, when in full screen\n"
"                           mode or when window geometry is fixed. If combined\n"
"                           with --auto-zoom, zooming will be limited to the\n"
"                           the size.\n"
" -w, --multiwindow         Disable slideshow mode. With this setting,\n"
"                           instead of opening multiple files in slideshow\n"
"                           mode, multiple windows will be opened.\n"
" -x, --borderless          Create borderless windows\n"
" -d, --draw-filename       Draw the filename at the top-left of the image.\n"
"     --title TITLE         Use TITLE as the window title in slideshow mode.\n"
" -D, --slideshow-delay NUM For slideshow mode, specifies time delay (seconds,\n"
"                           can be a decimal) between automatically changing\n"
"                           slides.\n"
"     --cycle-once          exit feh after one loop through a slideshow\n"
" -R, --reload NUM          Use this option to tell feh to reload an image\n"
"                           after NUM seconds. Useful for viewing webcams\n"
"                           via http, or even on your local machine.\n"
" -Q, --builtin             Use builtin http grabber to grab remote files\n"
"                           instead of wget.\n"
"                           mechanism, useful if don't have wget.\n"
" -k, --keep-http           When viewing files using http, feh normally\n"
"                           deletes the local copies after viewing, or,\n"
"                           if caching, on exit. This option prevents this\n"
"                           so that you get to keep the local copies.\n"
"                           They will be in the current working directory\n"
"                           with \"feh\" in the name.\n"
"     --caption-path PATH   Path to directory containing image captions.\n"
"                           This turns on caption viewing, and if captions\n"
"                           are found in PATH, which is relative to the\n"
"                           directory of each image, they are overlayed\n"
"                           on the displayed image.\n"
"                           e.g with caption path \"captions\", and viewing\n"
"                           image images/foo.jpg, caption will be looked for\n"
"                           as \"images/captions/foo.jpg.txt\"\n"
" -j, --output-dir          Output directory for saved files.  Really only\n"
"                           useful with the -k flag.\n"
" -G, --wget-timestamp      When viewing http images with reload set (eg\n"
"                           webcams), try to only reload the image if the\n"
"                           remote file has changed.\n"
" -l, --list                Don't display images. Analyse them and display an\n"
"                           'ls' style listing. Useful in scripts hunt out\n"
"                           images of a certain size/resolution/type etc.\n"
" -L, --customlist FORMAT   Use FORMAT as the format specifier for list\n"
"                           output. FORMAT is a printf-like string containing\n"
"                           image info specifiers. See FORMAT SPECIFIERS.\n"
" -U, --loadable            Don't display images. Just print out their name\n"
"                           if imlib2 can successfully load them.\n"
" -u, --unloadable          Don't display images. Just print out their name\n"
"                           if imlib2 can NOT successfully load them.\n"
" -S, --sort SORT_TYPE      The file list may be sorted according to image\n"
"                           parameters. Allowed sort types are: name,\n"
"                           filename, width, height, pixels, size, format.\n"
"                           For sort modes other than name or filename, a\n"
"                           preload run will be necessary, causing a delay\n"
"                           proportional to the number of images in the list\n"
" -n, --reverse             Reverse the sort order. Use this to invert the order\n"
"                           of the filelist. Eg to sort in reverse width order,\n"
"                           use -nSwidth\n"
" -A, --action ACTION       Specify a string as an action to perform on the\n"
"                           image. In slideshow or multiwindow modes, the action\n"
"                           in list mode, or loadable|unloadable modes, the\n"
"                           action will be run for each file.\n"
"                           The action will be executed by /bin/sh. Use\n"
"                           format specifiers to refer to image info. See\n"
"                           FORMAT SPECIFIERS for examples\n"
"                           Eg. -A \"mv %%f ~/images/%%n\"\n"
"                           In slideshow mode, the next image will be shown\n"
"                           after running the command, in multiwindow mode,\n"
"                           the window will be closed.\n"
"     --action1 ACTION      These extra action options allow you to specify\n"
"     --action2 ACTION      multiple additional actions which can be invoked\n"
"     ...                   using the appropriate number key 1-9\n"
"     --action9 ACTION\n"
" -m, --montage             Enable montage mode. Montage mode creates a new\n"
"                           image consisting of a grid of thumbnails of the\n"
"                           images specified using FILE... When montage mode\n"
"                           is selected, certain other options become\n"
"                           available. See MONTAGE MODE OPTIONS\n"
" -c, --collage             Same as montage mode, but the thumbnails are\n"
"                           distributed randomly. You must specify width and\n"
"                           height or supply a background image or both\n"
" -i, --index               Enable Index mode. Index mode is similar to\n"
"                           montage mode, and accepts the same options. It\n"
"                           creates an index print of thumbails, printing the\n"
"                           images name beneath each thumbnail. Index mode\n"
"                           enables certain other options, see INDEX MODE\n"
"                           OPTIONS\n"
" -t, --thumbnails          As --index, but clicking an image will open it in\n"
"                           a new viewing window\n"
"     --cache-thumbnails    Enable thumbnail caching\n"
" -I, --fullindex           Same as index mode, but below each thumbnail you\n"
"                           get image name, size and dimensions\n"
"     --bg-tile FILE\n"
"     --bg-center FILE\n"
"     --bg-scale FILE\n"
"     --bg-seamless FILE    Set your desktop background to FILE. Feh can\n"
"                           use enlightenment IPC if you are running it,\n"
"                           or will fall back to X methods.\n"
"                           Feh stores the commandline necessary to restore\n"
"                           the background you chose in ~/.fehbg. So to have\n"
"                           feh-set backgrounds restored when you restart X,\n"
"                           add the line \"eval `cat $HOME/.fehbg`\" to your\n"
"                           X startup script (e.g. ~/.xsession). Note that\n"
"                           you only need to do this for non E window\n"
"                           managers.\n"
"     --fontpath PATH       Specify an extra directory to look in for fonts,\n"
"                           can be used multiple times to add multiple paths.\n"
" -M, --menu-font FONT      Use FONT for the font in menus.\n"
"     --menu-style FILE     Use FILE as the style descriptor for menu text.\n"
"     --menu-bg BG          Use BG for the background image in menus.\n"
"     --menu-border INT     Specify number of pixels that define the menu\n"
"                           background's border. Borders are not stretched\n"
"                           when images are scaled.\n"
" -N, --no-menus            Don't load or show any menus.\n"
" -1, --next-button B       Use button B to advance to the next image in any\n"
"                           mode (defaults to 1, usually the left button).\n"
" -2, --zoom-button B       Use button B to zoom the current image in any\n"
"                           mode (defaults to 2, usually the middle button).\n"
" -4, --menu-button B       Use CTRL+Button B to activate the menu in any\n"
"                           mode.  Set to 0 for any button.  This option\n"
"                           is disabled if the -N or --no-menus option is set\n"
"                           (defaults to 3, usually the right button).\n"
" -5, --menu-ctrl-mask      Require CTRL+Button for menu activation in\n"
"                           any mode (default=off).\n"
" -6, --rotate-button B     Use CTRL+Button B to rotate the current image in\n"
"                           any mode (default=2).\n"
" -7, --no-rotate-ctrl-mask Don't require CTRL+Button for rotation in\n"
"                           any mode -- just use the button (default=off).\n"
" -8, --blur-button B       Use CTRL+Button B to blur the current image in\n"
"                           any mode (default=1).\n"
" -9, --no-blur-ctrl-mask   Don't require CTRL+Button for blurring in\n"
"                           any mode -- just use the button (default=off).\n"
"     --xinerama [0|1]      Enable/disable Xinerama support.  Has no effect\n"
"                           unless you have an Xinerama compiled in.\n"
"     --screen-clip [0|1]   Enable/disable window clipping based on screen\n"
"                           size.  WARNING: with this option disabled,\n"
"                           image windows could become very large, making\n"
"                           them unmanageable in certain window managers.\n"
"     --hide-pointer        In full screen mode, hide the X mouse pointer.\n"
" FORMAT SPECIFIERS\n"
"                           %%f image path/filename\n"
"                           %%n image name\n"
"                           %%s image size (bytes)\n"
"                           %%p image pixel size\n"
"                           %%w image width\n"
"                           %%h image height\n"
"                           %%t image format\n"
"                           %%P prints feh\n"
"                           %%v prints the version\n"
"                           %%m prints the mode (slideshow, multiwindow...)\n"
"                           %%l prints the total number of files in the filelist\n"
"                           %%u prints the current file number\n"
"                           \\n prints a newline\n"
"                           Eg. feh -A \"mv %%f ~/images/%%n\" *\n"
" MONTAGE MODE OPTIONS\n"
" -X, --ignore-aspect       By default, the montage thumbnails will retain\n"
"                           their aspect ratios, while fitting in --thumb-width\n"
"                           and --thumb-height. This option will force them to\n"
"                           be the size set by --thumb-width and --thumb-height\n"
"                           This will prevent any whitespace in the final\n"
"                           montage\n"
" -s, --stretch             Normally, if an image is smaller than the specified\n"
"                           thumbnail size, it will not be enlarged. If this\n"
"                           option is set, the image will be scaled up to fit\n"
"                           the thumbnail size. (Aspect ratio will be maintained\n"
"                           unless --ignore-aspect is specified)\n"
" -y, --thumb-width NUM     Set thumbnail width in pixels\n"
" -E, --thumb-height NUM    Set thumbnail height in pixels\n"
"                           Thumbnails default to 20x20 pixels\n"
" -W, --limit-width NUM     Limit the width of the montage in pixels\n"
" -H, --limit-height NUM    Limit the height of the montage in pixels\n"
"                           These options can be used together (to define the\n"
"                           image size exactly), or separately. If only one is\n"
"                           specified, theother is calculated from the number\n"
"                           of files specified and the size of the thumbnails.\n"
"                           The default is to limit width to 800 pixels and\n"
"                           calculate the height\n"
" -b, --bg FILE|trans       Use FILE as a background for your montage. With\n"
"                           this option specified, the size of the montage will\n"
"                           default to the size of FILE if no size restrictions\n"
"                           are specified. Alternatively, if FILE is \"trans\",\n"
"                           make the background transparent.\n"
" -a, --alpha NUM           When drawing thumbnails onto the background, apply\n"
"                           them with a transparency level of NUM (0-255).\n"
" -o FILE                   Save the created montage to FILE\n"
" -O FILE                   Just save the created montage to FILE\n"
"                           WITHOUT displaying it (use in scripts)\n"
" INDEX MODE OPTIONS\n"
" -e FONT                   Use FONT to print the information under each\n"
"                           thumbnail. FONT should be defined in the form\n"
"                           fontname/size(points). eg -e myfont/12\n"
" -t, --title-font FONT     Use FONT to print a title on the index, if no\n"
"                           font is specified, a title will not be printed\n"
" SLIDESHOW KEYS\n"
" The default mode for viewing mulitple images is Slideshow mode\n"
" When viewing a slideshow, the following keys may be used:\n"
" p, P, <BACKSPACE>, <LEFT>  Goto previous slide\n"
" n, N, <SPACE>, <RIGHT>     Goto next slide\n"
" r, R                       Reload image (good for webcams)\n"
" v, V                       Toggle fullscreen\n"
" m, M                       Show popup menu\n"
" c, C                       Caption entry mode. If --caption-path has been\n"
"                            specified, then this enables caption editing.\n"
"                            The caption will turn yellow and be editable,\n"
"                            hit enter to confirm and save the caption, or\n"
"                            hit escape to cancel and revert the caption.\n"
" w, W                       Size window to current image dimensions\n"
" h, H                       Pause the slideshow (only useful when using\n"
" s, S                       Save current image to unique filename\n"
" f, F                       Save current filelist to unique filename\n"
"                            timed reloading or image changes)\n"
" <, >                       In place editing, rotate 90 degrees right/left\n"
" <HOME>                     Goto first slide\n"
" <END>                      Goto last slide\n"
" <ESCAPE>                   Quit the slideshow\n"
" +, =                       Increase reload delay\n"
" -, _                       Decrease reload delay\n"
" <DELETE>                   Remove the currently viewed file from the filelist\n"
" <CTRL+DELETE>              Delete the currently viewed file and remove it\n"
"                            from the filelist\n"
" x, X                       Close current window\n"
" q, Q                       Quit the slideshow\n"
" <KEYPAD LEFT>              Move the image to the left\n"
" <KEYPAD RIGHT>             Move the image to the right\n"
" <KEYPAD +>                 Zoom in\n"
" <KEYPAD ->                 Zoom out\n"
" <KEYPAD *>                 Zoom to 100%%\n"
" <KEYPAD />                 Zoom to fit the window\n"
" <ENTER>,0                  Run action specified by --action option\n"
" 1-9                        Run action 1-9 specified by --action[1-9] options\n"
"\n"
" MOUSE ACTIONS\n"
" When viewing an image, a click of mouse button 1 moves to the next image\n"
" (slideshow mode only), a drag of mouse button 1 pans the image, if the\n"
" viewable window is smaller than the image, button 2 zooms (click and drag\n"
" left->right to zoom in, right->left to zoom out, click once to restore\n"
" 1x zoom), and mouse button 3 pans.\n"
" Ctrl+button 1 blurs or sharpens the image (drag left to blur and right to\n"
" sharpen).  Ctrl+button 2 rotates the image around the center point.\n"
" Button 3 activates the context-sensitive menu.  Buttons can be redefined\n"
" with the -1 through -9 (or --*-button) cmdline flags.  All you people\n"
" with million button mice can remove the ctrl mask with the --no-*-ctrl-mask\n"
" options.\n" "\n" "See 'man feh' for more detailed information\n"
"\n"
"This program is free software see the file COPYING for licensing info.\n"
"Copyright Tom Gilbert (and various contributors) 1999-2003\n"
"Email bugs to <feh_sucks@linuxbrit.co.uk>\n");
   exit(0);
}

void
feh_create_default_config(char *rcfile)
{
   FILE *fp;

   D_ENTER(4);

   if ((fp = fopen(rcfile, "w")) == NULL)
   {
      weprintf("Unable to create default config file %s\n", rcfile);
      D_RETURN_(4);
   }

   fprintf(fp,
           "# Feh configuration file.\n"
           "# Lines starting with # are comments. Don't use comments mid-line.\n"
           "\n" "# Feh expects to find this as ~/.fehrc or /etc/fehrc\n"
           "# If both are available, ~/.fehrc will be used\n" "\n"
           "# Options defined in theme_name/options pairs.\n"
           "# Separate themename and options by whitespace.\n" "\n"
           "# There are two ways of specifying the theme. Either use feh -Tthemename,\n"
           "# or use a symbolic link to feh with the name of the theme. eg\n"
           "# ln -s `which feh` ~/bin/mkindex\n"
           "# Now when you run 'mkindex', feh will load the config specified for the\n"
           "# mkindex theme.\n" "\n" "# ======================\n"
           "# Some examples of usage\n" "# ======================\n" "\n"
           "# Set the default feh options to be recursive and verbose\n"
           "# feh -rV\n" "\n"
           "# Multiple options can of course be used. They should all be on one line\n"
           "# imagemap -rV --quiet -W 400 -H 300 --thumb-width 40 --thumb-height 30\n"
           "\n" "# ================================================\n"
           "# Here I set some useful themes for you to try out\n"
           "# ================================================\n" "\n"
           "# Webcam mode, simply specify the url(s).\n"
           "# e.g. feh -Twebcam http://cam1 http://cam2\n"
           "webcam --multiwindow --reload 20\n" "\n"
           "# Create an index of the current directory. This version uses . as the\n"
           "# current dir, so you don't even need any commandline arguments.\n"
           "mkindex -iVO index.jpg .\n" "\n" "# More ambitious version...\n"
           "imgidx -iVO .fehindex.jpg --limit-width 1200 --thumb-width 90 --thumb-height 90 .\n"
           "\n" "# Show a presentation\n"
           "present --full-screen --sort name\n"
           "\n"
           "# Booth mode ;-)\n"
           "booth --full-screen --hide-pointer --slideshow-delay 20\n"
           "\n"
           "# Screw xscreensaver, use feh =)\n"
           "screensave --full-screen --randomize --slideshow-delay 5\n" "\n"
           "# Add <img> tags to your html with ease :-)\n"
           "newimg -q -L \"<img src=\\\"%%f\\\" alt=\\\"%%n\\\" border=\\\"0\\\" width=\\\"%%w\\\" height=\\\"%%h\\\">\"\n"
           "\n"
           "# Different menus\n"
           "chrome --menu-bg " PREFIX" /share/feh/images/menubg_chrome.png\n"
           "brushed --menu-bg " PREFIX "/share/feh/images/menubg_brushed.png\n"
           "pastel --menu-bg " PREFIX "/share/feh/images/menubg_pastel.png\n"
           "aluminium --menu-bg " PREFIX "/share/feh/images/menubg_aluminium.png\n"
           "wood --menu-bg " PREFIX "/share/feh/images/menubg_wood.png\n"
           "aqua --menu-bg " PREFIX "/share/feh/images/menubg_aqua.png\n"
           "sky --menu-bg " PREFIX "/share/feh/images/menubg_sky.png\n"
           "orange --menu-bg " PREFIX "/share/feh/images/menubg_orange.png\n"
           "light --menu-bg " PREFIX "/share/feh/images/menubg_light.png\n"
           "black --menu-bg " PREFIX "/share/feh/images/menubg_black.png --menu-style " PREFIX "/share/feh/fonts/black.style\n"
           "britney --menu-bg " PREFIX "/share/feh/images/menubg_britney.png\n");
   fclose(fp);

   D_RETURN_(4);
}
