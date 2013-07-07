/* imlib.c

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

May 2012 HRABAK combined the gib_imlib.c code into this module, and renamed
the gib_imlib.h into imlib.h.

*/

#include "feh.h"
#include "winwidget.h"
#include "options.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#ifdef HAVE_LIBCURL
#include <curl/curl.h>
#endif

#ifdef HAVE_LIBEXIF
#include "exif.h"
#endif

static char *feh_http_load_image(char *url);
static char *feh_magick_load_image(char *filename);

#ifdef HAVE_LIBXINERAMA
void init_xinerama(void)
{
	if (opt.flg.xinerama && XineramaIsActive(fgv.disp)) {
		int major, minor;
		if (getenv("XINERAMA_SCREEN"))
			fgv.xinerama_screen = atoi(getenv("XINERAMA_SCREEN"));
		else
			fgv.xinerama_screen = 0;
		XineramaQueryVersion(fgv.disp, &major, &minor);
		fgv.xinerama_screens = XineramaQueryScreens(fgv.disp, &fgv.num_xinerama_screens);
	}
}
#endif				/* HAVE_LIBXINERAMA */

void init_feh_fonts(void){
		/* As of Apr 2013 HRABAK encapsulated all the font stuff here.  Fonts get
		 * loaded once, stored into opt.big||dflt||menu, the opt.fn_ptr points to
		 * the one we want and no other func has to load or check or worry about
		 * it.  Place all the #defines here cause they should NOT be used anywhere
		 * else. init_parse_options() has NULL'd out all members B4 this call.
		 */

#define DEFAULT_FONT_MENU  "yudit/10"
#define DEFAULT_FONT       "yudit/11"
#define DEFAULT_FONT_BIG   "yudit/12"
#define DEFAULT_FONT_TITLE "yudit/14"
#define DEFAULT_FONT_FEP   "yudit/30"


	imlib_add_path_to_font_path(".");
	imlib_add_path_to_font_path(PREFIX "/share/feh/fonts");

	/* walk thru and load the four instances of the opt.font family */
	if ( !opt.fn_dflt.name ) opt.fn_dflt.name = DEFAULT_FONT;
		gib_imlib_load_font(&opt.fn_dflt);
	if ( !opt.fn_menu.name ) opt.fn_menu.name = DEFAULT_FONT_MENU;
		gib_imlib_load_font(&opt.fn_menu);
	if ( !opt.fn_fulls.name ) opt.fn_fulls.name = DEFAULT_FONT_BIG;
		gib_imlib_load_font(&opt.fn_fulls);
	opt.fn_fep.name = DEFAULT_FONT_FEP;
		gib_imlib_load_font(&opt.fn_fep);

	/* "--title-font FONT" option is really a flag and a font.  If user
	 * supplies a font, then create_index_title_string() is called.
	 * So leave this blank UNLESS the user supplies a font.
	 */
	if ( opt.fn_title.name )
		gib_imlib_load_font(&opt.fn_title);

	/* opt.fn_ptr is always set to the dflt font unless w->full_screen.
	 * The ONLY func that should change this is stub_toggle_fullscreen()
	 */
	opt.fn_ptr =  opt.flg.full_screen ? &opt.fn_fulls : &opt.fn_dflt;

	return;
}    /* end of init_feh_fonts()  */

void init_x_and_imlib(void)
{
	fgv.disp = XOpenDisplay(NULL);
	if (!fgv.disp)
		eprintf("%s%sX display. It *is* running??", ERR_CANNOT, ERR_OPEN);
	fgv.vis  = DefaultVisual(fgv.disp, DefaultScreen(fgv.disp));
	fgv.depth= DefaultDepth(fgv.disp, DefaultScreen(fgv.disp));
	fgv.cm   = DefaultColormap(fgv.disp, DefaultScreen(fgv.disp));
	fgv.root = RootWindow(fgv.disp, DefaultScreen(fgv.disp));
	fgv.scr  = ScreenOfDisplay(fgv.disp, DefaultScreen(fgv.disp));
	fgv.xid_context = XUniqueContext();

#ifdef HAVE_LIBXINERAMA
	init_xinerama();
#endif				/* HAVE_LIBXINERAMA */

	imlib_context_set_display(fgv.disp);
	imlib_context_set_visual(fgv.vis);
	imlib_context_set_colormap(fgv.cm);
	imlib_context_set_color_modifier(NULL);
	imlib_context_set_progress_function(NULL);
	imlib_context_set_operation(IMLIB_OP_COPY);
	fgv.wmDeleteWindow = XInternAtom(fgv.disp, "WM_DELETE_WINDOW", False);

	/* Initialise random numbers */
	srand(getpid() * time(NULL) % ((unsigned int) -1));

	return;
}

int feh_load_image_char(Imlib_Image * im, char *filename)
{
	feh_data *data;
	int i;

	data = feh_ll_new_data(filename);
	i = feh_load_image(im, data);
	feh_ll_free_data(data);
	return(i);
}

int feh_load_image(Imlib_Image * im, feh_data * data)
{
	Imlib_Load_Error err;
	enum { SRC_IMLIB, SRC_HTTP, SRC_MAGICK } image_source = SRC_IMLIB;
	char *tmpname = NULL;
	char *real_filename = NULL;

	D(("filename is %s, image is %p\n", data->filename, im));

	if (!data || !data->filename)
		return 0;

	/* Handle URLs */
	if ((!strncmp(data->filename, "http://" , 7))
	 || (!strncmp(data->filename, "https://", 8))
	 || (!strncmp(data->filename, "ftp://"  , 6))) {
		image_source = SRC_HTTP;

		tmpname = feh_http_load_image(data->filename);      /* static */
	}
	else
		*im = imlib_load_image_with_error_return(data->filename, &err);

	if ((err == IMLIB_LOAD_ERROR_UNKNOWN)
			|| (err == IMLIB_LOAD_ERROR_NO_LOADER_FOR_FILE_FORMAT)) {
		image_source = SRC_MAGICK;
		tmpname = feh_magick_load_image(data->filename);    /* static */
	}

	if (image_source != SRC_IMLIB) {
		if (tmpname == NULL)
			return 0;

		*im = imlib_load_image_with_error_return(tmpname, &err);
		if (im) {
			real_filename = data->filename;
			data->filename = tmpname;
			feh_ll_load_data_info(data, *im);
			data->filename = real_filename;
#ifdef HAVE_LIBEXIF
			data->ed = exif_get_data(tmpname);
#endif
		}
		if    ((opt.flg.mode == MODE_SLIDESHOW)
		    && (opt.reload == 0)
		    && (image_source != SRC_MAGICK)) {
			free(data->filename);
			data->filename = estrdup(tmpname);

			if (!opt.flg.keep_http)
				feh_move_node_to_remove_list(NULL, DELETE_YES, WIN_MAINT_NO );
		}
		else if ((image_source == SRC_MAGICK) || !opt.flg.keep_http)
			unlink(tmpname);

	}

	if ((err) || (!im)) {
		if (opt.flg.verbose && !opt.flg.quiet) {
			fputs("\n", stdout);
			opt.flg.reset_output = 1;
		}
		if (err == IMLIB_LOAD_ERROR_OUT_OF_FILE_DESCRIPTORS)
			eprintf("%s - Out of file descriptors while loading", data->filename);
		else if (!opt.flg.quiet) {
			switch (err) {
			case IMLIB_LOAD_ERROR_FILE_DOES_NOT_EXIST:
				weprintf("%s - File does not exist", data->filename);
				break;
			case IMLIB_LOAD_ERROR_FILE_IS_DIRECTORY:
				weprintf("%s - Directory specified for image filename", data->filename);
				break;
			case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_READ:
				weprintf("%s - No read access", data->filename);
				break;
			case IMLIB_LOAD_ERROR_UNKNOWN:
			case IMLIB_LOAD_ERROR_NO_LOADER_FOR_FILE_FORMAT:
				weprintf("%s - No Imlib2 loader for that file format", data->filename);
				break;
			case IMLIB_LOAD_ERROR_PATH_TOO_LONG:
				weprintf("%s - Path specified is too long", data->filename);
				break;
			case IMLIB_LOAD_ERROR_PATH_COMPONENT_NON_EXISTANT:
				weprintf("%s - Path component does not exist", data->filename);
				break;
			case IMLIB_LOAD_ERROR_PATH_COMPONENT_NOT_DIRECTORY:
				weprintf("%s - Path component is not a directory", data->filename);
				break;
			case IMLIB_LOAD_ERROR_PATH_POINTS_OUTSIDE_ADDRESS_SPACE:
				weprintf("%s - Path points outside address space", data->filename);
				break;
			case IMLIB_LOAD_ERROR_TOO_MANY_SYMBOLIC_LINKS:
				weprintf("%s - Too many levels of symbolic links", data->filename);
				break;
			case IMLIB_LOAD_ERROR_OUT_OF_MEMORY:
				weprintf("While loading %s - Out of memory", data->filename);
				break;
			case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_WRITE:
				weprintf("%s - %swrite to directory", data->filename,ERR_CANNOT);
				break;
			case IMLIB_LOAD_ERROR_OUT_OF_DISK_SPACE:
				weprintf("%s - %swrite - out of disk space", data->filename,ERR_CANNOT);
				break;
			default:
				weprintf("While loading %s - Unknown error (%d)",
						data->filename, err);
				break;
			}
		}
		D(("Load *failed*\n"));
		return(0);
	}

#ifdef HAVE_LIBEXIF
	data->ed = exif_get_data(data->filename);
#endif

	D(("Loaded ok\n"));
	return(1);
}

static char *feh_magick_load_image(char *filename)
{
	char argv_fd[12];
	char *basename;
	char *sfn;
	int fd = -1, devnull = -1;
	int status;

	if (opt.magick_timeout < 0)
		return NULL;

	basename = strrchr(filename, '/');

	if (basename == NULL)
		basename = filename;
	else
		basename++;

	sfn = feh_unique_filename("/tmp/", basename); /* static */

	if (strlen(sfn) > (NAME_MAX-6))
		sfn[NAME_MAX-7] = '\0';

	strcat(sfn,"_XXXXXX");

	fd = mkstemp(sfn);

	if (fd == -1)
		return NULL;

	snprintf(argv_fd, sizeof(argv_fd), "png:fd:%d", fd);

	if ((fgv.childpid = fork()) == 0) {

		/* discard convert output */
		devnull = open("/dev/null", O_WRONLY);
		dup2(devnull, 0);
		dup2(devnull, 1);
		dup2(devnull, 2);

		/*
		 * convert only accepts SIGINT via killpg, a normal kill doesn't work
		 */
		setpgid(0, 0);

		execlp("convert", "convert", filename, argv_fd, NULL);
		exit(1);
	}
	else {
		alarm(opt.magick_timeout);
		waitpid(fgv.childpid, &status, 0);
		alarm(0);
		if (!WIFEXITED(status) || (WEXITSTATUS(status) != 0)) {
			close(fd);
			unlink(sfn);
			sfn = NULL;

			if (!opt.flg.quiet) {
				if (WIFSIGNALED(status))
					weprintf("%s - Conversion took too long, skipping",
						filename);
				else
					weprintf("%s - No loader for that file format",
						filename);
			}

			/*
			 * Reap child.  The previous waitpid call was interrupted by
			 * alarm, but convert doesn't terminate immediately.
			 * XXX
			 * normally, if (WIFSIGNALED(status)) waitpid(fgv.childpid, &status, 0);
			 * would suffice. However, as soon as feh has its own window,
			 * this doesn't work anymore and the following workaround is
			 * required. Hm.
			 */
			waitpid(-1, &status, 0);
		}
		fgv.childpid = 0;
	}

	return sfn;
}

#ifdef HAVE_LIBCURL

static char *feh_http_load_image(char *url)
{
	CURL *curl;
	CURLcode res;
	char *sfn;
	FILE *sfp;
	int fd = -1;
	char *ebuff;
	char *basename;
	char *path = NULL;          /* FIXME?  no storage space for path??? */

	if (opt.flg.keep_http) {
		if (opt.output_dir)
			path = opt.output_dir;
		else
			path = "";
	} else
		path = "/tmp/";

	curl = curl_easy_init();
	if (!curl) {
		weprintf("%slibcurl initialization failure", ERR_OPEN_URL );
		return NULL;
	}

	basename = strrchr(url, '/') + 1;
	sfn = feh_unique_filename(path, basename);    /* static */

	if (strlen(sfn) > (NAME_MAX-6))
		sfn[NAME_MAX-7] = '\0';

	strcat(sfn,"_XXXXXX");

	fd = mkstemp(sfn);
	if (fd != -1) {
		sfp = fdopen(fd, "w+");
		if (sfp != NULL) {
#ifdef DEBUG
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
#endif
			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, sfp);
			ebuff = emalloc(CURL_ERROR_SIZE);
			curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, ebuff);
			curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

			res = curl_easy_perform(curl);
			curl_easy_cleanup(curl);
			if (res != CURLE_OK) {
				weprintf("%s%s",ERR_OPEN_URL, ebuff);
				unlink(sfn);
				close(fd);
				sfn = NULL;
			}

			free(ebuff);
			fclose(sfp);
			return sfn;
		} else {
			weprintf("%sfdopen %s:",ERR_OPEN_URL,ERR_FAILED);
			unlink(sfn);
			close(fd);
		}
	} else {
		weprintf("%smkstemp %s:", ERR_OPEN_URL,ERR_FAILED);
	}
	curl_easy_cleanup(curl);
	return NULL;
}

#else				/* HAVE_LIBCURL */

char *feh_http_load_image(char *url)
{
	weprintf(
		"%sload image %s\n Please recompile with libcurl support",
		ERR_CANNOT,
		url
	);
	return NULL;
}

#endif				/* HAVE_LIBCURL */

Imlib_Image feh_imlib_image_make_n_fill_text_bg( int w, int h, int alpha){
		/* combined imlib_create_image() and feh_imlib_image_fill_text_bg()
		 * to collect and simplify the error processing.
		 */
	Imlib_Image im = NULL;

	if ( !( im = imlib_create_image(w, h)))
		eprintf("%screate image. Out of memory?", ERR_CANNOT);

	gib_imlib_image_set_has_alpha(im, alpha);

	imlib_context_set_blend(0);

	gib_imlib_image_fill_rectangle(im, 0, 0, w, h, 0, 0, 0,
	    (opt.flg.text_bg == TEXT_BG_CLEAR)? 0 : 127 );

	imlib_context_set_blend(1);

	return im;
}


void feh_draw_zoom(winwidget w)
{
	int tw = 0, th = 0;
	Imlib_Image im = NULL;
	char buf[100];

	if (!w->im)
		return;

	opt.fn_ptr = w->full_screen ? &opt.fn_fulls : &opt.fn_dflt;

	snprintf(buf, sizeof(buf), "%.0f%%, %dx%d", w->zoom * 100,
			(int) (w->im_w * w->zoom),
			(int) (w->im_h * w->zoom));

	/* Work out how high the font is */
	gib_imlib_get_text_size( buf, NULL, &tw, &th, IMLIB_TEXT_TO_RIGHT);

	tw += 3;
	th += 3;
	im = feh_imlib_image_make_n_fill_text_bg( tw, th, 1);

	feh_imlib_text_draw(im, &opt.style[ STYLE_WHITE ],
		                  1, 1, buf, IMLIB_TEXT_TO_RIGHT);
	gib_imlib_render_image_on_drawable(w->bg_pmap, im, 0, w->high - th, 1, 1, 0);
	gib_imlib_free_image_and_decache(im);
	return;
}

void im_weprintf(winwidget w, char *fmt, ...)
{
	va_list args;
	char *errstr = mobs(8);   /* SIZE_1024 */

	fflush(stdout);
	fputs(PACKAGE " WARNING: ", stderr);

	va_start(args, fmt);
	vsnprintf(errstr, SIZE_1024, fmt, args);
	va_end(args);

	if (w)
		w->errstr = errstr;

	fputs(errstr, stderr);
	if (fmt[0] != '\0' && fmt[strlen(fmt) - 1] == ':')
		fprintf(stderr, " %s", strerror(errno));
	fputs("\n", stderr);
}


void feh_draw_errstr(winwidget w)
{
	int tw = 0, th = 0;
	Imlib_Image im = NULL;

	if (!w->im)
		return;

	opt.fn_ptr = &opt.fn_dflt;

	/* Work out how high the font is */
	gib_imlib_get_text_size( w->errstr, NULL, &tw, &th, IMLIB_TEXT_TO_RIGHT);

	tw += 3;
	th += 3;
	im = feh_imlib_image_make_n_fill_text_bg( tw, th, 1 );

	feh_imlib_text_draw(im, &opt.style[ STYLE_RED ],
		                  1, 1, w->errstr, IMLIB_TEXT_TO_RIGHT);
	w->errstr = NULL;       /* static mobs() now , don't free */
	gib_imlib_render_image_on_drawable(w->bg_pmap, im, 0, w->high - th, 1, 1, 0);
	gib_imlib_free_image_and_decache(im);
}

void feh_draw_filename(winwidget w){
		/* the caller wants the filename to show, so no need to test */

	static char *last1 = NULL;
	static char  *name = NULL;
	char *s = mobs(1);
	int tw = 0, th = 0, nw = 0;
	Imlib_Image im = NULL;

	if ( !w->node || !NODE_DATA(w->node)
	              || !NODE_FILENAME(w->node) )
		return;

	opt.fn_ptr =  w->full_screen ? &opt.fn_fulls : &opt.fn_dflt;

	/* we want filename, but what part(s)? */
	name = NODE_FILENAME(w->node);
	if (opt.flg.draw_name) {
		name = NODE_NAME(w->node);
		if ( opt.flg.draw_no_ext )
			if ( ( last1 = NODE_EXT(w->node))  )
					*last1 = '\0';
	}

	/* Work out how high the font is */
	gib_imlib_get_text_size( name, NULL, &tw, &th, IMLIB_TEXT_TO_RIGHT);

	if ( LL_CNT(feh_md) > 1) {
		sprintf(s, "%d of %d", CN_CNT(feh_md), LL_CNT(feh_md) );
		gib_imlib_get_text_size( s, NULL, &nw, NULL, IMLIB_TEXT_TO_RIGHT);

		if (nw > tw)
			tw = nw;
	}

	tw += 3;
	th += 3;
	im = feh_imlib_image_make_n_fill_text_bg( tw, 2 * th, 1);

	feh_imlib_text_draw(im, &opt.style[ STYLE_WHITE ],
	                    1, 1, name, IMLIB_TEXT_TO_RIGHT);
	if ( last1 ){
			*last1 = '.';     /* restore the name+extension */
			last1 = NULL;
	}

	if (s[0] != '\0' ) {
		feh_imlib_text_draw(im, &opt.style[ STYLE_WHITE ],
		                    1, th , s, IMLIB_TEXT_TO_RIGHT);
	}

	gib_imlib_render_image_on_drawable(w->bg_pmap, im, 0, 0, 1, 1, 0);
	gib_imlib_free_image_and_decache(im);

	return;
}

#ifdef HAVE_LIBEXIF
void feh_draw_exif(winwidget w)
{
	int width = 0, height = 0, line_width = 0, line_height = 0;
	Imlib_Image im = NULL;
	int no_lines = 0, i;
	int pos = 0;
	int pos2 = 0;
	char *info_buf[ SIZE_128 ];    /* max 128 lines */
	char *info_line = mobs(2);     /* SIZE_256,  max 256 chars per line */

	char buffer[MAX_EXIF_DATA];


	if ( !w->node || !NODE_DATA(w->node)
	              || !NODE_FILENAME(w->node) )
		return;

	buffer[0] = '\0';
	exif_get_info(NODE_DATA(w->node)->ed, buffer, MAX_EXIF_DATA);

	opt.fn_ptr =  w->full_screen ? &opt.fn_fulls : &opt.fn_dflt;

	if (buffer == NULL)
	{
		STRCAT_2ITEMS(buffer, ERR_FAILED, " to run exif command");
		gib_imlib_get_text_size( &buffer[0], NULL, &width, &height,
		                         IMLIB_TEXT_TO_RIGHT);
		no_lines = 1;
	}
	else
	{

		while ( (no_lines < SIZE_128 ) && (pos < MAX_EXIF_DATA) )
		{
			pos2 = 0;
			while ( pos2 < SIZE_256 )
			{
				if ( (buffer[pos] != '\n')
						  && (buffer[pos] != '\0') )
				{
					info_line[pos2] = buffer[pos];
				}
				else if ( buffer[pos] == '\0' )
				{
					pos = MAX_EXIF_DATA;          /* all data seen */
					info_line[pos2] = '\0';
				}
				else
				{
					info_line[pos2] = '\0';       /* line finished, cont with next line*/
					pos++;
					break;
				}

				pos++;
				pos2++;
			}

			gib_imlib_get_text_size( info_line, NULL, &line_width,
			                         &line_height, IMLIB_TEXT_TO_RIGHT);

			if (line_height > height)
				height = line_height;
			if (line_width > width)
				width = line_width;
			info_buf[no_lines] = estrdup(info_line);

			no_lines++;
		}
	}

	if (no_lines == 0)
		return;

	height *= no_lines;
	width += 4;

	im = feh_imlib_image_make_n_fill_text_bg( width, height, 1 );

	for (i = 0; i < no_lines; i++)
	{
		feh_imlib_text_draw(im, &opt.style[ STYLE_WHITE ] ,
		                    1, (i * line_height) + 1,
		                    info_buf[i], IMLIB_TEXT_TO_RIGHT);
	}

	gib_imlib_render_image_on_drawable(w->bg_pmap, im, 0, w->high - height, 1, 1, 0);

	gib_imlib_free_image_and_decache(im);
	return;

}
#endif      /* HAVE_LIBEXIF */

void feh_draw_info(winwidget w)
{
	int width = 0, height = 0, line_width = 0, line_height = 0;
	Imlib_Image im = NULL;
	int no_lines = 0, i;
	char *info_cmd;
	char *info_line = mobs(2);    /* SIZE_256 */
	char *info_buf[ SIZE_128 ];
	FILE *info_pipe;


	if ( !w->node || !NODE_DATA(w->node)
	            || !NODE_FILENAME(w->node) )
		return;

	opt.fn_ptr = w->full_screen ? &opt.fn_fulls : &opt.fn_dflt;

	info_cmd = feh_printf(opt.info_cmd, NODE_DATA(w->node) );

	info_pipe = popen(info_cmd, "r");

	if (!info_pipe) {
		STRCAT_2ITEMS(info_buf[0], ERR_FAILED, " to run info command");
		gib_imlib_get_text_size( info_buf[0], NULL, &width, &height,
		                         IMLIB_TEXT_TO_RIGHT);
		no_lines = 1;
	}
	else {
		while ((no_lines < SIZE_128 ) && fgets(info_line, SIZE_256, info_pipe)) {
			if (info_line[strlen(info_line)-1] == '\n')
				info_line[strlen(info_line)-1] = '\0';

			gib_imlib_get_text_size( info_line, NULL, &line_width,
			                         &line_height, IMLIB_TEXT_TO_RIGHT);

			if (line_height > height)
				height = line_height;
			if (line_width > width)
				width = line_width;

			info_buf[no_lines] = estrdup(info_line);

			no_lines++;
		}
		pclose(info_pipe);
	}

	if (no_lines == 0)
		return;

	height *= no_lines;
	width += 4;

	im = feh_imlib_image_make_n_fill_text_bg( width, height, 1 );

	for (i = 0; i < no_lines; i++) {
		feh_imlib_text_draw(im, &opt.style[ STYLE_WHITE ],
		                  1, (i * line_height) + 1, info_buf[i],
		                  IMLIB_TEXT_TO_RIGHT );

		free(info_buf[i]);
	}

	gib_imlib_render_image_on_drawable(w->bg_pmap,im,0,w->high - height, 1, 1, 0);

	gib_imlib_free_image_and_decache(im);
	return;
}

char *build_caption_filename(feh_data * data, short create_dir)
{
	static struct stat cdir_stat;
	char *caption_dir = mobs(2);

	/* data->name is already at the last '/' in the filename */
	if ( data->name == data->filename ){
			/* no path */
			caption_dir[0] = '.' ; caption_dir[1] = '\0';
	} else {
			strcpy( caption_dir, data->filename );
			caption_dir[ data->name - data->filename -1 ] = '\0';
	}

	if ( opt.caption_path[0] != '/' )
			strcat( caption_dir, "/" );
	strcat( caption_dir, opt.caption_path );

	D(("cp %s, cdir %s\n", opt.caption_path, caption_dir))

	if (stat(caption_dir, &cdir_stat) == -1) {
		if (!create_dir)
			return NULL;
		if (mkdir(caption_dir, 0755) == -1)
			eprintf("%s to create caption directory %s:",ERR_FAILED, caption_dir);
	} else if (!S_ISDIR(cdir_stat.st_mode))
			eprintf("Caption directory (%s) exists, but is not a directory.",
								caption_dir);

	strcat( caption_dir, "/");
	strcat( caption_dir, data->name);
	strcat( caption_dir, ".txt");

	return caption_dir;
}


void feh_draw_caption(winwidget w) {
		/* To avoid re-stat()ing the caption when you zoom a captionless pic, HRABAK
		* set ALL empty captions to "Caption entry mode - Hit ESC to cancel".
		* Don't forget this function is called from winwidget_render_image(), so
		* without this "fix" the captions file would be stat()d like 30 times a
		* second.
		*/

	static char *start, *end;        /* substr markers in alp[] */
	static char last1;               /* save the final char */
	static ld *alp;                  /* alp stands for Array(of)LinePointers */
	int i = 0;                       /* index to walk thru alp[] array */
	int style_flag = STYLE_WHITE;    /* to toggle between WHITE and YELLOW */

	int tw = 0, th = 0, ww;          /* unused var  hh; */
	int x, y;
	Imlib_Image im = NULL;

	if ( !w->node || !NODE_DATA(w->node)
	              || !NODE_FILENAME(w->node) )
		return;

	if (!NODE_CAPTION(w->node) ) {
		/* read caption from file */
		char *caption_filename = build_caption_filename(NODE_DATA(w->node), 0);
		if (caption_filename)
			NODE_CAPTION(w->node) = ereadfile(caption_filename);
	}

	if (NODE_CAPTION(w->node) == NULL)
			NODE_CAPTION(w->node) = fgv.no_cap;          /* they all point to this one */

	if ( !w->caption_entry && NODE_CAPTION(w->node) == fgv.no_cap )
		return;

	opt.fn_ptr = w->full_screen ? &opt.fn_fulls : &opt.fn_dflt;

	/* ALL captions now point to something valid */
	alp = feh_wrap_string( NODE_CAPTION(w->node), NULL, w->wide );

	if ( alp[0].L0.tot_lines == 0 ) return;

	/* we don't want the caption overlay larger than our window */
	th = alp[0].L0.tothigh + alp[0].L0.tot_lines + 2;
	tw = alp[0].L0.maxwide +1 ;
	if (th > w->high)
		th = w->high;
	if (tw > w->wide)
		tw = w->wide;

	im = feh_imlib_image_make_n_fill_text_bg( tw, th, 1 );

	x = 0;
	y = 0;
	if (w->caption_entry) style_flag = STYLE_YELLOW ;

	for (i=1; i<= alp[0].L0.tot_lines ; i++ ) {
		/* loops ONCE thru the lines and prints them */
		ww = alp[i].L1.wide;
		x = (tw - ww) / 2;

		start = alp[i].L1.line;
		end   = alp[i].L1.line + alp[i].L1.len;
		/* null term this substring b4 the call ...*/
		last1 = end[0];  end[0]   = '\0';
		feh_imlib_text_draw(im, &opt.style[ style_flag ],
		                    x, y, start, IMLIB_TEXT_TO_RIGHT);
		/* ... then restore that last char afterwards */
		end[0] = last1;
		y += alp[i].L1.high + 2;	/* line spacing */
	}

	gib_imlib_render_image_on_drawable(w->bg_pmap, im, (w->wide - tw) / 2, w->high - th, 1, 1, 0);
	gib_imlib_free_image_and_decache(im);

	return;
}

/* unsigned char reset_output = 0;*/

void feh_display_status(char stat)
{
	static int i = 0;
	static int init_len = 0;
	int j = 0;

	D(("first node %p, node->next %p\n", feh_md->rn->next, feh_md->rn->next->next ));

	if (!stat) {
		putc('\n', stdout);
		init_len = 0;
		i = 0;
		return;
	}

	if (!init_len)
		init_len = LL_CNT(feh_md);

	if (i) {
		if (opt.flg.reset_output) {
			/* There's just been an error message. Unfortunate ;) */
			for (j = 0; j < (((i % 50) + ((i % 50) / 10)) + 7); j++)
				putc(' ', stdout);
		}

		if (!(i % 50)) {
			int len = LL_CNT(feh_md);

			fprintf(stdout, " %5d/%d (%d)\n[%3d%%] ",
					i, init_len, len, ((int) ((float) i / init_len * 100)));

		} else if ((!(i % 10)) && (!opt.flg.reset_output))
			putc(' ', stdout);

		opt.flg.reset_output = 0;
	} else
		fputs("[  0%] ", stdout);

	fprintf(stdout, "%c", stat);
	fflush(stdout);
	i++;
	return;
}

void feh_edit_inplace(winwidget w, int op)
{
	int ret;
	Imlib_Image old;

	if ( !w->node || !NODE_DATA(w->node) || !NODE_FILENAME(w->node) )
		return;

	if (!strcmp(gib_imlib_image_format(w->im), "jpeg")) {
		feh_edit_inplace_lossless(w, op);
		feh_reload_image(w, RESIZE_YES, FORCE_NEW_YES);
		return;
	}

	ret = feh_load_image(&old, NODE_DATA(w->node));
	if (ret) {
		if (op == INPLACE_EDIT_FLIP) {
			imlib_context_set_image(old);
			imlib_image_flip_vertical();
		} else if (op == INPLACE_EDIT_MIRROR) {
			imlib_context_set_image(old);
			imlib_image_flip_horizontal();
		} else
			gib_imlib_image_orientate(old, op);
		gib_imlib_save_image(old, NODE_FILENAME(w->node));
		gib_imlib_free_image(old);
		feh_reload_image(w, RESIZE_YES, FORCE_NEW_YES );
	} else {
		im_weprintf(w, "%s to load image from disk to edit it in place",ERR_FAILED);
	}

	return;
}

/* May 2012 HRABAK rewrote feh_wrap_string() to eliminate gib_list
 * linked list requirement and remove redundant gib_imlib_get_text_size()
 * calls.
 */

ld * feh_wrap_string( char *text, feh_style *s,  int w ){
		/* takes a (possibly) multi-line text and breaks it into lines that will
		* fit inside the image w(idth) constraint, for a given font and style.
		* The wide and high calcs for each line are saved with the line so the
		* caller can just feh_imlib_text_draw() without having to recalc those
		* values again.  Returns an array of the lines (alp[]) to the caller.
		* alp[0] element holds the metadata for all the lines in this text.
		* Note:  When I pass a substring to get_text_size(), I just jam in a
		* NULL termination mid-string (in the original), then restore that
		* one char (last1) after the call.
		*/

	static int guess = FIRST_ALP_GUESS;
	static ld *alp;                  /*  alp stands for Array(of)LinePointers */
	static wd *awp;                  /*  awp stands for Array(of)WordPointers */

	int linenum=0;                   /* index for alp[] */
	int i_start, i_end;              /* indexs for awp[] */
	int lw, lh=0;
	char *start, *end;
	char last1;                      /* save the final char */

/* adds a new element to the alp[] array. should this be an inline func?  */
#define ADD_THIS_TO_ALP { if ( linenum == guess ){ \
                              guess+=FIRST_ALP_GUESS; \
                              alp =  erealloc( alp, sizeof( ld ) * guess ); \
                          } \
                          alp[ linenum ].L1.wide = lw; \
                          alp[ linenum ].L1.high = lh; \
                          alp[ linenum ].L1.line = start; \
                          alp[ linenum ].L1.len  = end - start; \
                          if ( lw > alp[0].L0.maxwide ){ \
                              alp[0].L0.maxwide  = lw; \
                          } \
                          alp[0].L0.tothigh += lh; \
                          alp[0].L0.tot_lines = linenum; \
                        }

	/* I don't like this solution but ....  Initialize it the first time thru  */
	if ( linenum==0 )
			alp =  malloc( sizeof( ld ) * guess );

	awp = feh_string_split(text);

	/* incase no text lines */
	alp[0].L0.maxwide = alp[0].L0.tothigh = alp[0].L0.tot_lines = 0;

	/* prime the pump with the first line */
	linenum = 1; i_start = 1; start = awp[0].word;
	end = get_next_awp_line( awp , linenum,  i_start, &i_end );

	while (1) {

			if ( end == NULL ){
					/* an empty line, so use the default wide/high */
					lw = opt.fn_ptr->wide;  lh = opt.fn_ptr->high;
					end = start;
					i_end--;
			} else {
					/* null term this substring b4 the call ...*/
					last1 = end[0];  end[0]   = '\0' ;  start = awp[i_start].word;
					gib_imlib_get_text_size( start, s, &lw, &lh, IMLIB_TEXT_TO_RIGHT);
					/* ... then restore that last char afterwards */
					end[0] = last1;
			}

			if ( lw <= w ) {      /* it fits */
					ADD_THIS_TO_ALP
					linenum++;      /* ready for the next line */
					i_start = i_end +1;
					if ( i_start > awp[0].linenum ) break;
					start = awp[i_start].word;
					end = get_next_awp_line( awp , linenum,  i_start, &i_end );
			} else {              /* did not fit, so drop words off the end */
					if ( i_start == i_end ){
							/* not even room for the FIRST word.
							* So mash it in, resetting wide to fit */
							if ( w < lw) w = lw;
							ADD_THIS_TO_ALP
							linenum++;      /* ready for the next line */
							i_start++;
							if ( i_start > awp[0].linenum ) break;
							end = get_next_awp_line( awp , linenum,  i_start, &i_end );
							continue;
					}

					i_end--;        /*  drop one word off the end and try again */
					end = awp[i_end].word + awp[i_end].len;

			}     /* end if lw <=w */
	}         /* end of infinite while() loop */

	return alp;

}     /* end of feh_wrap_string() */

void feh_edit_inplace_lossless(winwidget w, int op)
{
	char *file_str  = mobs(2);
	int pid, status;
	char op_name[]  = "rotate";     /* message */
	char op_op[]    = "-rotate";    /* jpegtran option */
	char op_value[] = "horizontal"; /* jpegtran option's value */

	if (op == INPLACE_EDIT_FLIP) {
		strcpy(op_name,  "flip");
		strcpy(op_op,    "-flip");
		strcpy(op_value, "vertical");
	} else if (op == INPLACE_EDIT_MIRROR) {
		strcpy(op_name,  "mirror");
		strcpy(op_op,    "-flip");
	} else
		snprintf(op_value, 4, "%d", 90 * op);

	strcpy(file_str, NODE_FILENAME(w->node));

	if ((pid = fork()) < 0) {
		im_weprintf(w, "lossless %s: fork %s:", op_name, ERR_FAILED);
		exit(1);
	} else if (pid == 0) {

		execlp("jpegtran", "jpegtran", "-copy", "all", op_op, op_value,
				"-outfile", file_str, file_str, NULL);

		im_weprintf(w, "lossless %s: Is 'jpegtran' installed? %s to exec:", op_name,ERR_FAILED);
		exit(1);
	} else {
		waitpid(pid, &status, 0);

		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
			im_weprintf(w,
					"lossless %s: Got exitcode %d from jpegtran."
					" Commandline was: "
					"jpegtran -copy all %s %s -outfile %s %s",
					op_name, status >> 8, op_op, op_value, file_str, file_str);
			return;
		}
	}
}

void feh_draw_actions(winwidget w)
{
	static char *heading = "defined actions:";
	static char digits[] = "0123456789 : ";
	Imlib_Image im = NULL;
	int tw, th, th_offset, max_tw, max_th, i, cur_action;
	char *line = mobs(1);

	if (opt.flg.no_actions )
		return;

	if ( !w->node || !NODE_DATA(w->node) || !NODE_FILENAME(w->node) )
			return;

	opt.fn_ptr = w->full_screen ? &opt.fn_fulls : &opt.fn_dflt;

	tw = th = th_offset = max_tw = max_th = i = cur_action = 0;
	gib_imlib_get_text_size( heading, NULL, &max_tw, &max_th, IMLIB_TEXT_TO_RIGHT);

	/* Check for the widest line */
	for (i = 0; i < MAX_ACTIONS ; i++) {
		if (opt.actions[i]) {     /* print the leading ";" (hold flag) too */
			gib_imlib_get_text_size(opt.actions[i], NULL, &tw, &th,
			                        IMLIB_TEXT_TO_RIGHT);
			if (tw > max_tw)
				max_tw = tw;

			max_th += th + 3;       /* vert padding */
		}
	}

	/* (3*opt.fn_ptr->wide) is aprox size of "0: "  prefix */
	max_tw += 3 + (opt.fn_ptr->wide * 3);
	max_th += 3;

	/* This depends on feh_draw_filename internals...
	 * should be FIXME some time.  Maybe save the actual th_offset.
	 */
	if (opt.flg.draw_filename)
		th_offset = opt.fn_ptr->high * 2;

	im = feh_imlib_image_make_n_fill_text_bg( max_tw, max_th, 1);

	feh_imlib_text_draw(im, &opt.style[ STYLE_YELLOW ],
	                    1, 1, heading, IMLIB_TEXT_TO_RIGHT);

	for (i = 0; i < MAX_ACTIONS ; i++) {
		if (opt.actions[i]) {
			cur_action++;
			digits[10] = digits[i];           /* makes the "0: " string */
			STRCAT_2ITEMS( line, digits+10,opt.actions[i]);

			feh_imlib_text_draw(im, &opt.style[ STYLE_WHITE ],
			                    1, (cur_action * opt.fn_ptr->high) + 1, line,
			                    IMLIB_TEXT_TO_RIGHT );
		}
	}

	gib_imlib_render_image_on_drawable(w->bg_pmap, im, 0, 0 + th_offset, 1, 1, 0);
	gib_imlib_free_image_and_decache(im);

	return;

}


/* As of May 25, 2012 HRABAK moved all the old gib_imlib.c code here and
 * killed gib_imlib.c and .h files.  This was the last move to remove
 * any gib_??? named files from feh.
 */

int
gib_imlib_image_get_width(Imlib_Image im)
{
   imlib_context_set_image(im);
   return imlib_image_get_width();
}

int
gib_imlib_image_get_height(Imlib_Image im)
{
   imlib_context_set_image(im);
   return imlib_image_get_height();
}

int
gib_imlib_image_has_alpha(Imlib_Image im)
{
   imlib_context_set_image(im);
   return imlib_image_has_alpha();
}

void
gib_imlib_free_image_and_decache(Imlib_Image im)
{
   imlib_context_set_image(im);
   imlib_free_image_and_decache();
}

void
gib_imlib_free_image(Imlib_Image im)
{
   imlib_context_set_image(im);
   imlib_free_image();
}

void
gib_imlib_render_image_on_drawable(Drawable d, Imlib_Image im, int x, int y,
                                   char dither, char blend, char alias)
{
   imlib_context_set_image(im);
   imlib_context_set_drawable(d);
   imlib_context_set_anti_alias(alias);
   imlib_context_set_dither(dither);
   imlib_context_set_blend(blend);
   imlib_context_set_angle(0);
   imlib_render_image_on_drawable(x, y);
}

void
gib_imlib_render_image_on_drawable_at_size(Drawable d, Imlib_Image im, int x,
                                           int y, int w, int h, char dither,
                                           char blend, char alias)
{
   imlib_context_set_image(im);
   imlib_context_set_drawable(d);
   imlib_context_set_anti_alias(alias);
   imlib_context_set_dither(dither);
   imlib_context_set_blend(blend);
   imlib_context_set_angle(0);
   imlib_render_image_on_drawable_at_size(x, y, w, h);
}


void
gib_imlib_render_image_part_on_drawable_at_size(Drawable d, Imlib_Image im,
                                                int sx, int sy, int sw,
                                                int sh, int dx, int dy,
                                                int dw, int dh, char dither,
                                                char blend, char alias)
{
   imlib_context_set_image(im);
   imlib_context_set_drawable(d);
   imlib_context_set_anti_alias(alias);
   imlib_context_set_dither(dither);
   imlib_context_set_blend(blend);
   imlib_context_set_angle(0);
   imlib_render_image_part_on_drawable_at_size(sx, sy, sw, sh, dx, dy, dw,
                                               dh);
}

void
gib_imlib_render_image_part_on_drawable_at_size_with_rotation(Drawable d,
                                                              Imlib_Image im,
                                                              int sx, int sy,
                                                              int sw, int sh,
                                                              int dx, int dy,
                                                              int dw, int dh,
                                                              double angle,
                                                              char dither,
                                                              char blend,
                                                              char alias)
{
   Imlib_Image new_im;

   imlib_context_set_image(im);
   imlib_context_set_drawable(d);
   imlib_context_set_anti_alias(alias);
   imlib_context_set_dither(dither);
   imlib_context_set_angle(angle);
   imlib_context_set_blend(blend);
   new_im = imlib_create_rotated_image(angle);
   imlib_context_set_image(new_im);
   imlib_render_image_part_on_drawable_at_size(sx, sy, sw, sh, dx, dy, dw,
                                               dh);
   imlib_free_image_and_decache();
}

void
gib_imlib_image_fill_rectangle(Imlib_Image im, int x, int y, int w, int h,
                               int r, int g, int b, int a)
{
   imlib_context_set_image(im);
   imlib_context_set_color(r, g, b, a);
   imlib_image_fill_rectangle(x, y, w, h);
}


void
gib_imlib_image_draw_rectangle(Imlib_Image im, int x, int y, int w, int h,
                               int r, int g, int b, int a)
{
   imlib_context_set_image(im);
   imlib_context_set_color(r, g, b, a);
   imlib_image_draw_rectangle(x, y, w, h);
}


void
feh_imlib_text_draw(Imlib_Image im, feh_style *s, int x, int y,
                    char *text, Imlib_Text_Direction dir )
{
    /* HRABAK's rewrite now requires each call to supply a feh_style,
     * making the old EXTRA r,b,g,a params superfluous.
     * Also assumes that the off_sets are in a small range (+/-5)
     * as no longer do I calc max/min offsets.
     */

    imlib_context_set_image(im);
    imlib_context_set_font(opt.fn_ptr->fn);
    imlib_context_set_direction(dir);

    /* draw the text first with the background setting ... */
    imlib_context_set_color(s->bg.r, s->bg.g, s->bg.b, s->bg.a);
    imlib_text_draw(x + s->bg.x_off, y + s->bg.y_off, text);
    /* ... then write the fg over the top of that */
    imlib_context_set_color(s->fg.r, s->fg.g, s->fg.b, s->fg.a);
    imlib_text_draw(x + s->fg.x_off, y + s->fg.y_off, text);

  return;

}       /* end of feh_imlib_text_draw() */



void
gib_imlib_get_text_size( char *text, feh_style *s,
                         int *w, int *h, Imlib_Text_Direction dir)
{
	/* As of Apr 2013 HRABAK uses the global opt.fn_ptr->fn rather than
	 * passing "Imlib_Font fn" all the way down the call chain.  For this
	 * func alone, the call stack is 6 deep and "Imlib_Font fn" is not
	 * needed in the 5 intervening calls :-)
	 */
   imlib_context_set_font( opt.fn_ptr->fn );
   imlib_context_set_direction(dir);
   imlib_get_text_size(text, w, h);
   if (s) {     /* I think I can kill this whole thing */
      if (h)
         *h += s->bg.y_off;
      if (w)
         *w += s->bg.x_off;
   }
}

Imlib_Image gib_imlib_clone_image(Imlib_Image im)
{
   imlib_context_set_image(im);
   return imlib_clone_image();
}

char *
gib_imlib_image_format(Imlib_Image im)
{
   imlib_context_set_image(im);
   return imlib_image_format();
}

void
gib_imlib_blend_image_onto_image(Imlib_Image dest_image,
                                 Imlib_Image source_image, char merge_alpha,
                                 int sx, int sy, int sw, int sh, int dx,
                                 int dy, int dw, int dh, char dither,
                                 char blend, char alias)
{
   imlib_context_set_image(dest_image);
   imlib_context_set_anti_alias(alias);
   imlib_context_set_dither(dither);
   imlib_context_set_blend(blend);
   imlib_context_set_angle(0);
   imlib_blend_image_onto_image(source_image, merge_alpha, sx, sy, sw, sh, dx,
                                dy, dw, dh);
}

Imlib_Image gib_imlib_create_cropped_scaled_image(Imlib_Image im, int sx,
                                                  int sy, int sw, int sh,
                                                  int dw, int dh, char alias)
{
   imlib_context_set_image(im);
   imlib_context_set_anti_alias(alias);
   return imlib_create_cropped_scaled_image(sx, sy, sw, sh, dw, dh);
}

void
gib_imlib_apply_color_modifier_to_rectangle(Imlib_Image im, int x, int y,
                                            int w, int h, DATA8 * rtab,
                                            DATA8 * gtab, DATA8 * btab,
                                            DATA8 * atab)
{
   Imlib_Color_Modifier cm;

   imlib_context_set_image(im);
   cm = imlib_create_color_modifier();
   imlib_context_set_color_modifier(cm);
   imlib_set_color_modifier_tables(rtab, gtab, btab, atab);
   imlib_apply_color_modifier_to_rectangle(x, y, w, h);
   imlib_free_color_modifier();
}

void
gib_imlib_image_set_has_alpha(Imlib_Image im, int alpha)
{
   imlib_context_set_image(im);
   imlib_image_set_has_alpha(alpha);
}

void
gib_imlib_save_image(Imlib_Image im, char *file)
{
   char *tmp;

   imlib_context_set_image(im);
   tmp = strrchr(file, '.');
   if (tmp)
   {
     char *p, *pp;
     p = estrdup(tmp + 1);
     pp = p;
     while(*pp) {
       *pp = tolower(*pp);
       pp++;
     }
     imlib_image_set_format(p);
     free(p);
   }
   imlib_save_image(file);
}

void
gib_imlib_save_image_with_error_return(Imlib_Image im, char *file,
                                       Imlib_Load_Error * error_return)
{
   char *tmp;

   imlib_context_set_image(im);
   tmp = strrchr(file, '.');
   if (tmp)
      imlib_image_set_format(tmp + 1);
   imlib_save_image_with_error_return(file, error_return);
}

void
gib_imlib_image_draw_line(Imlib_Image im, int x1, int y1, int x2, int y2,
                          char make_updates, int r, int g, int b, int a)
{
   imlib_context_set_image(im);
   imlib_context_set_color(r, g, b, a);
   imlib_image_draw_line(x1, y1, x2, y2, make_updates);
}

Imlib_Image gib_imlib_create_rotated_image(Imlib_Image im, double angle)
{
   imlib_context_set_image(im);
   return (imlib_create_rotated_image(angle));
}

void
gib_imlib_image_blur(Imlib_Image im, int radius)
{
   imlib_context_set_image(im);
   imlib_image_blur(radius);
}

void
gib_imlib_image_sharpen(Imlib_Image im, int radius)
{
   imlib_context_set_image(im);
   imlib_image_sharpen(radius);
}


void gib_imlib_load_font(feh_font *fn_ptr){
		/* This fills the already reserved feh_font structure in opt.
		 */

	int w,h;

	fn_ptr->fn = imlib_load_font(fn_ptr->name);
	if (!fn_ptr->fn) {
		weprintf("%Sload font %s, attempting to fall back to fixed.",ERR_CANNOT, fn_ptr->name);
		if ((fn_ptr->fn = imlib_load_font("fixed"))){
			fn_ptr->name = estrdup("fixed");
		} else {
			weprintf("%s to even load fixed! Attempting to find any font.",ERR_FAILED);
			if ((fn_ptr->fn = imlib_load_font("*"))) {
				fn_ptr->name = estrdup("anything");
			} else eprintf("FATAL! Could NOT find any font.");
		}
	}

	/* something got loaded, so calc the default spacings */
	opt.fn_ptr = fn_ptr;        /* all imlib calls now use opt.fn_ptr */
	gib_imlib_get_text_size( "M", NULL, &w, &h, IMLIB_TEXT_TO_RIGHT);
	fn_ptr->wide = (unsigned char) w;
	fn_ptr->high = (unsigned char) h;

}

void gib_imlib_image_orientate(Imlib_Image im, int orientation)
{
  imlib_context_set_image(im);
  imlib_image_orientate(orientation);
}

/*  end of the old gib_imlib.c code */