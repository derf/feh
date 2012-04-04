/* imlib.c

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

#include "feh.h"
#include "filelist.h"
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

Display *disp = NULL;
Visual *vis = NULL;
Screen *scr = NULL;
Colormap cm;
int depth;
Atom wmDeleteWindow;
XContext xid_context = 0;
Window root = 0;

/* Xinerama support */
#ifdef HAVE_LIBXINERAMA
XineramaScreenInfo *xinerama_screens = NULL;
int xinerama_screen;
int num_xinerama_screens;
#endif				/* HAVE_LIBXINERAMA */

int childpid = 0;

static char *feh_http_load_image(char *url);
static char *feh_magick_load_image(char *filename);

#ifdef HAVE_LIBXINERAMA
void init_xinerama(void)
{
	if (opt.xinerama && XineramaIsActive(disp)) {
		int major, minor;
		if (getenv("XINERAMA_SCREEN"))
			xinerama_screen = atoi(getenv("XINERAMA_SCREEN"));
		else
			xinerama_screen = 0;
		XineramaQueryVersion(disp, &major, &minor);
		xinerama_screens = XineramaQueryScreens(disp, &num_xinerama_screens);
	}
}
#endif				/* HAVE_LIBXINERAMA */

void init_imlib_fonts(void)
{
	/* Set up the font stuff */
	imlib_add_path_to_font_path(".");
	imlib_add_path_to_font_path(PREFIX "/share/feh/fonts");

	return;
}

void init_x_and_imlib(void)
{
	disp = XOpenDisplay(NULL);
	if (!disp)
		eprintf("Can't open X display. It *is* running, yeah?");
	vis = DefaultVisual(disp, DefaultScreen(disp));
	depth = DefaultDepth(disp, DefaultScreen(disp));
	cm = DefaultColormap(disp, DefaultScreen(disp));
	root = RootWindow(disp, DefaultScreen(disp));
	scr = ScreenOfDisplay(disp, DefaultScreen(disp));
	xid_context = XUniqueContext();

#ifdef HAVE_LIBXINERAMA
	init_xinerama();
#endif				/* HAVE_LIBXINERAMA */

	imlib_context_set_display(disp);
	imlib_context_set_visual(vis);
	imlib_context_set_colormap(cm);
	imlib_context_set_color_modifier(NULL);
	imlib_context_set_progress_function(NULL);
	imlib_context_set_operation(IMLIB_OP_COPY);
	wmDeleteWindow = XInternAtom(disp, "WM_DELETE_WINDOW", False);

	/* Initialise random numbers */
	srand(getpid() * time(NULL) % ((unsigned int) -1));

	return;
}

int feh_load_image_char(Imlib_Image * im, char *filename)
{
	feh_file *file;
	int i;

	file = feh_file_new(filename);
	i = feh_load_image(im, file);
	feh_file_free(file);
	return(i);
}

int feh_load_image(Imlib_Image * im, feh_file * file)
{
	Imlib_Load_Error err;
	enum { SRC_IMLIB, SRC_HTTP, SRC_MAGICK } image_source = SRC_IMLIB;
	char *tmpname = NULL;
	char *real_filename = NULL;

	D(("filename is %s, image is %p\n", file->filename, im));

	if (!file || !file->filename)
		return 0;

	/* Handle URLs */
	if ((!strncmp(file->filename, "http://", 7)) || (!strncmp(file->filename, "https://", 8))
			|| (!strncmp(file->filename, "ftp://", 6))) {
		image_source = SRC_HTTP;

		tmpname = feh_http_load_image(file->filename);
	}
	else
		*im = imlib_load_image_with_error_return(file->filename, &err);

	if ((err == IMLIB_LOAD_ERROR_UNKNOWN)
			|| (err == IMLIB_LOAD_ERROR_NO_LOADER_FOR_FILE_FORMAT)) {
		image_source = SRC_MAGICK;
		tmpname = feh_magick_load_image(file->filename);
	}

	if (image_source != SRC_IMLIB) {
		if (tmpname == NULL)
			return 0;

		*im = imlib_load_image_with_error_return(tmpname, &err);
		if (im) {
			real_filename = file->filename;
			file->filename = tmpname;
			feh_file_info_load(file, *im);
			file->filename = real_filename;
#ifdef HAVE_LIBEXIF
			file->ed = exif_get_data(tmpname);
#endif
		}
		if ((opt.slideshow) && (opt.reload == 0) && (image_source != SRC_MAGICK)) {
			free(file->filename);
			file->filename = estrdup(tmpname);

			if (!opt.keep_http)
				add_file_to_rm_filelist(tmpname);
		}
		else if ((image_source == SRC_MAGICK) || !opt.keep_http)
			unlink(tmpname);

		free(tmpname);
	}

	if ((err) || (!im)) {
		if (opt.verbose && !opt.quiet) {
			fputs("\n", stdout);
			reset_output = 1;
		}
		if (err == IMLIB_LOAD_ERROR_OUT_OF_FILE_DESCRIPTORS)
			eprintf("%s - Out of file descriptors while loading", file->filename);
		else if (!opt.quiet) {
			switch (err) {
			case IMLIB_LOAD_ERROR_FILE_DOES_NOT_EXIST:
				weprintf("%s - File does not exist", file->filename);
				break;
			case IMLIB_LOAD_ERROR_FILE_IS_DIRECTORY:
				weprintf("%s - Directory specified for image filename", file->filename);
				break;
			case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_READ:
				weprintf("%s - No read access", file->filename);
				break;
			case IMLIB_LOAD_ERROR_UNKNOWN:
			case IMLIB_LOAD_ERROR_NO_LOADER_FOR_FILE_FORMAT:
				weprintf("%s - No Imlib2 loader for that file format", file->filename);
				break;
			case IMLIB_LOAD_ERROR_PATH_TOO_LONG:
				weprintf("%s - Path specified is too long", file->filename);
				break;
			case IMLIB_LOAD_ERROR_PATH_COMPONENT_NON_EXISTANT:
				weprintf("%s - Path component does not exist", file->filename);
				break;
			case IMLIB_LOAD_ERROR_PATH_COMPONENT_NOT_DIRECTORY:
				weprintf("%s - Path component is not a directory", file->filename);
				break;
			case IMLIB_LOAD_ERROR_PATH_POINTS_OUTSIDE_ADDRESS_SPACE:
				weprintf("%s - Path points outside address space", file->filename);
				break;
			case IMLIB_LOAD_ERROR_TOO_MANY_SYMBOLIC_LINKS:
				weprintf("%s - Too many levels of symbolic links", file->filename);
				break;
			case IMLIB_LOAD_ERROR_OUT_OF_MEMORY:
				weprintf("While loading %s - Out of memory", file->filename);
				break;
			case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_WRITE:
				weprintf("%s - Cannot write to directory", file->filename);
				break;
			case IMLIB_LOAD_ERROR_OUT_OF_DISK_SPACE:
				weprintf("%s - Cannot write - out of disk space", file->filename);
				break;
			default:
				weprintf("While loading %s - Unknown error (%d)",
						file->filename, err);
				break;
			}
		}
		D(("Load *failed*\n"));
		return(0);
	}

#ifdef HAVE_LIBEXIF
	file->ed = exif_get_data(file->filename);
#endif		

	D(("Loaded ok\n"));
	return(1);
}

static char *feh_magick_load_image(char *filename)
{
	char argv_fd[12];
	char *basename;
	char *tmpname;
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

	tmpname = feh_unique_filename("/tmp/", basename);

	if (strlen(tmpname) > (NAME_MAX-6))
		tmpname[NAME_MAX-7] = '\0';

	sfn = estrjoin("_", tmpname, "XXXXXX", NULL);
	free(tmpname);

	fd = mkstemp(sfn);

	if (fd == -1)
		return NULL;

	snprintf(argv_fd, sizeof(argv_fd), "png:fd:%d", fd);

	if ((childpid = fork()) == 0) {

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
		waitpid(childpid, &status, 0);
		alarm(0);
		if (!WIFEXITED(status) || (WEXITSTATUS(status) != 0)) {
			close(fd);
			unlink(sfn);
			free(sfn);
			sfn = NULL;

			if (!opt.quiet) {
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
			 * normally, if (WIFSIGNALED(status)) waitpid(childpid, &status, 0);
			 * would suffice. However, as soon as feh has its own window,
			 * this doesn't work anymore and the following workaround is
			 * required. Hm.
			 */
			waitpid(-1, &status, 0);
		}
		childpid = 0;
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
	char *tmpname;
	char *basename;
	char *path = NULL;

	if (opt.keep_http) {
		if (opt.output_dir)
			path = opt.output_dir;
		else
			path = "";
	} else
		path = "/tmp/";

	curl = curl_easy_init();
	if (!curl) {
		weprintf("open url: libcurl initialization failure");
		return NULL;
	}

	basename = strrchr(url, '/') + 1;
	tmpname = feh_unique_filename(path, basename);

	if (strlen(tmpname) > (NAME_MAX-6))
		tmpname[NAME_MAX-7] = '\0';
	
	sfn = estrjoin("_", tmpname, "XXXXXX", NULL);
	free(tmpname);

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
				weprintf("open url: %s", ebuff);
				unlink(sfn);
				close(fd);
				free(sfn);
				sfn = NULL;
			}

			free(ebuff);
			fclose(sfp);
			return sfn;
		} else {
			weprintf("open url: fdopen failed:");
			unlink(sfn);
			free(sfn);
			close(fd);
		}
	} else {
		weprintf("open url: mkstemp failed:");
		free(sfn);
	}
	curl_easy_cleanup(curl);
	return NULL;
}

#else				/* HAVE_LIBCURL */

char *feh_http_load_image(char *url)
{
	weprintf(
		"Cannot load image %s\n Please recompile with libcurl support",
		url
	);
	return NULL;
}

#endif				/* HAVE_LIBCURL */

void feh_imlib_image_fill_text_bg(Imlib_Image im, int w, int h)
{
	gib_imlib_image_set_has_alpha(im, 1);

	imlib_context_set_blend(0);

	if (opt.text_bg == TEXT_BG_CLEAR)
		gib_imlib_image_fill_rectangle(im, 0, 0, w, h, 0, 0, 0, 0);
	else
		gib_imlib_image_fill_rectangle(im, 0, 0, w, h, 0, 0, 0, 127);

	imlib_context_set_blend(1);
}

static Imlib_Font feh_load_font(winwidget w)
{
	static Imlib_Font fn = NULL;

	if (opt.font)
		fn = gib_imlib_load_font(opt.font);

	if (!fn) {
		if (w && w->full_screen)
			fn = gib_imlib_load_font(DEFAULT_FONT_BIG);
		else
			fn = gib_imlib_load_font(DEFAULT_FONT);
	}

	if (!fn) {
		eprintf("Couldn't load font to draw a message");
	}

	return fn;
}


void feh_draw_zoom(winwidget w)
{
	static Imlib_Font fn = NULL;
	int tw = 0, th = 0;
	Imlib_Image im = NULL;
	char buf[100];

	if (!w->im)
		return;

	fn = feh_load_font(w);

	snprintf(buf, sizeof(buf), "%.0f%%, %dx%d", w->zoom * 100,
			(int) (w->im_w * w->zoom), (int) (w->im_h * w->zoom));

	/* Work out how high the font is */
	gib_imlib_get_text_size(fn, buf, NULL, &tw, &th, IMLIB_TEXT_TO_RIGHT);

	tw += 3;
	th += 3;
	im = imlib_create_image(tw, th);
	if (!im)
		eprintf("Couldn't create image. Out of memory?");

	feh_imlib_image_fill_text_bg(im, tw, th);

	gib_imlib_text_draw(im, fn, NULL, 2, 2, buf, IMLIB_TEXT_TO_RIGHT, 0, 0, 0, 255);
	gib_imlib_text_draw(im, fn, NULL, 1, 1, buf, IMLIB_TEXT_TO_RIGHT, 255, 255, 255, 255);
	gib_imlib_render_image_on_drawable(w->bg_pmap, im, 0, w->h - th, 1, 1, 0);
	gib_imlib_free_image_and_decache(im);
	return;
}

void im_weprintf(winwidget w, char *fmt, ...)
{
	va_list args;
	char *errstr = emalloc(1024);

	fflush(stdout);
	fputs(PACKAGE " WARNING: ", stderr);

	va_start(args, fmt);
	vsnprintf(errstr, 1024, fmt, args);
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
	static Imlib_Font fn = NULL;
	int tw = 0, th = 0;
	Imlib_Image im = NULL;

	if (!w->im)
		return;

	fn = feh_load_font(NULL);

	/* Work out how high the font is */
	gib_imlib_get_text_size(fn, w->errstr, NULL, &tw, &th, IMLIB_TEXT_TO_RIGHT);

	tw += 3;
	th += 3;
	im = imlib_create_image(tw, th);
	if (!im)
		eprintf("Couldn't create errstr image. Out of memory?");

	feh_imlib_image_fill_text_bg(im, tw, th);

	gib_imlib_text_draw(im, fn, NULL, 2, 2, w->errstr, IMLIB_TEXT_TO_RIGHT, 0, 0, 0, 255);
	gib_imlib_text_draw(im, fn, NULL, 1, 1, w->errstr, IMLIB_TEXT_TO_RIGHT, 255, 0, 0, 255);
	free(w->errstr);
	w->errstr = NULL;
	gib_imlib_render_image_on_drawable(w->bg_pmap, im, 0, w->h - th, 1, 1, 0);
	gib_imlib_free_image_and_decache(im);
}

void feh_draw_filename(winwidget w)
{
	static Imlib_Font fn = NULL;
	int tw = 0, th = 0, nw = 0;
	Imlib_Image im = NULL;
	char *s = NULL;
	int len = 0;

	if ((!w->file) || (!FEH_FILE(w->file->data))
			|| (!FEH_FILE(w->file->data)->filename))
		return;

	fn = feh_load_font(w);

	/* Work out how high the font is */
	gib_imlib_get_text_size(fn, FEH_FILE(w->file->data)->filename, NULL, &tw,
			&th, IMLIB_TEXT_TO_RIGHT);

	if (gib_list_length(filelist) > 1) {
		len = snprintf(NULL, 0, "%d of %d",  gib_list_length(filelist),
				gib_list_length(filelist)) + 1;
		s = emalloc(len);
		snprintf(s, len, "%d of %d", gib_list_num(filelist, current_file) +
				1, gib_list_length(filelist));

		gib_imlib_get_text_size(fn, s, NULL, &nw, NULL, IMLIB_TEXT_TO_RIGHT);

		if (nw > tw)
			tw = nw;
	}

	tw += 3;
	th += 3;
	im = imlib_create_image(tw, 2 * th);
	if (!im)
		eprintf("Couldn't create image. Out of memory?");

	feh_imlib_image_fill_text_bg(im, tw, 2 * th);

	gib_imlib_text_draw(im, fn, NULL, 2, 2, FEH_FILE(w->file->data)->filename,
			IMLIB_TEXT_TO_RIGHT, 0, 0, 0, 255);
	gib_imlib_text_draw(im, fn, NULL, 1, 1, FEH_FILE(w->file->data)->filename,
			IMLIB_TEXT_TO_RIGHT, 255, 255, 255, 255);

	if (s) {
		gib_imlib_text_draw(im, fn, NULL, 2, th + 1, s, IMLIB_TEXT_TO_RIGHT, 0, 0, 0, 255);
		gib_imlib_text_draw(im, fn, NULL, 1, th, s, IMLIB_TEXT_TO_RIGHT, 255, 255, 255, 255);
		free(s);
	}

	gib_imlib_render_image_on_drawable(w->bg_pmap, im, 0, 0, 1, 1, 0);

	gib_imlib_free_image_and_decache(im);
	return;
}

#ifdef HAVE_LIBEXIF  
void feh_draw_exif(winwidget w)
{
	static Imlib_Font fn = NULL;
	int width = 0, height = 0, line_width = 0, line_height = 0;
	Imlib_Image im = NULL;
	int no_lines = 0, i;
	int pos = 0;
	int pos2 = 0;
	char info_line[256];
	char *info_buf[128];
	char buffer[EXIF_MAX_DATA];

	if ( (!w->file) || (!FEH_FILE(w->file->data))
			 || (!FEH_FILE(w->file->data)->filename) )
	{
		return;
	}


	buffer[0] = '\0';
	exif_get_info(FEH_FILE(w->file->data)->ed, buffer, EXIF_MAX_DATA);

	fn = feh_load_font(w);

	if (buffer == NULL) 
	{
		snprintf(buffer, EXIF_MAX_DATA, "%s", estrdup("Failed to run exif command"));
		gib_imlib_get_text_size(fn, &buffer[0], NULL, &width, &height, IMLIB_TEXT_TO_RIGHT);
		no_lines = 1;
	}
	else 
	{

		while ( (no_lines < 128) && (pos < EXIF_MAX_DATA) )
		{
			/* max 128 lines */
			pos2 = 0;
			while ( pos2 < 256 ) /* max 256 chars per line */
			{
				if ( (buffer[pos] != '\n')
				      && (buffer[pos] != '\0') )
				{
			    info_line[pos2] = buffer[pos];
			  }
			  else if ( buffer[pos] == '\0' )
			  {
			    pos = EXIF_MAX_DATA; /* all data seen */
			    info_line[pos2] = '\0';
				}
			  else
			  {
			  	info_line[pos2] = '\0'; /* line finished, continue with next line*/

			    pos++;
			    break;
			  }
			        
			   pos++;
			   pos2++;  
			}

			gib_imlib_get_text_size(fn, info_line, NULL, &line_width,
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

	im = imlib_create_image(width, height);
	if (!im)
	{
		eprintf("Couldn't create image. Out of memory?");
	}

	feh_imlib_image_fill_text_bg(im, width, height);

	for (i = 0; i < no_lines; i++) 
	{
		gib_imlib_text_draw(im, fn, NULL, 2, (i * line_height) + 2,
				info_buf[i], IMLIB_TEXT_TO_RIGHT, 0, 0, 0, 255);
		gib_imlib_text_draw(im, fn, NULL, 1, (i * line_height) + 1,
				info_buf[i], IMLIB_TEXT_TO_RIGHT, 255, 255, 255, 255);

	}

	gib_imlib_render_image_on_drawable(w->bg_pmap, im, 0, w->h - height, 1, 1, 0);

	gib_imlib_free_image_and_decache(im);
	return;

}
#endif

void feh_draw_info(winwidget w)
{
	static Imlib_Font fn = NULL;
	int width = 0, height = 0, line_width = 0, line_height = 0;
	Imlib_Image im = NULL;
	int no_lines = 0, i;
	char *info_cmd;
	char info_line[256];
	char *info_buf[128];
	FILE *info_pipe;

	if ((!w->file) || (!FEH_FILE(w->file->data))
			|| (!FEH_FILE(w->file->data)->filename))
		return;

	fn = feh_load_font(w);

	info_cmd = feh_printf(opt.info_cmd, FEH_FILE(w->file->data));

	info_pipe = popen(info_cmd, "r");

	if (!info_pipe) {
		info_buf[0] = estrdup("Failed to run info command");
		gib_imlib_get_text_size(fn, info_buf[0], NULL, &width, &height, IMLIB_TEXT_TO_RIGHT);
		no_lines = 1;
	}
	else {
		while ((no_lines < 128) && fgets(info_line, 256, info_pipe)) {
			if (info_line[strlen(info_line)-1] == '\n')
				info_line[strlen(info_line)-1] = '\0';

			gib_imlib_get_text_size(fn, info_line, NULL, &line_width,
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

	im = imlib_create_image(width, height);
	if (!im)
		eprintf("Couldn't create image. Out of memory?");

	feh_imlib_image_fill_text_bg(im, width, height);

	for (i = 0; i < no_lines; i++) {
		gib_imlib_text_draw(im, fn, NULL, 2, (i * line_height) + 2,
				info_buf[i], IMLIB_TEXT_TO_RIGHT, 0, 0, 0, 255);
		gib_imlib_text_draw(im, fn, NULL, 1, (i * line_height) + 1,
				info_buf[i], IMLIB_TEXT_TO_RIGHT, 255, 255, 255, 255);

		free(info_buf[i]);
	}

	gib_imlib_render_image_on_drawable(w->bg_pmap, im, 0,
			w->h - height, 1, 1, 0);

	gib_imlib_free_image_and_decache(im);
	return;
}

char *build_caption_filename(feh_file * file, short create_dir)
{
	char *caption_filename;
	char *s, *dir, *caption_dir;
	struct stat cdir_stat;
	s = strrchr(file->filename, '/');
	if (s) {
		dir = estrdup(file->filename);
		s = strrchr(dir, '/');
		*s = '\0';
	} else {
		dir = estrdup(".");
	}

	caption_dir = estrjoin("/", dir, opt.caption_path, NULL);

	D(("dir %s, cp %s, cdir %s\n", dir, opt.caption_path, caption_dir))

	if (stat(caption_dir, &cdir_stat) == -1) {
		if (!create_dir)
			return NULL;
		if (mkdir(caption_dir, 0755) == -1)
			eprintf("Failed to create caption directory %s:", caption_dir);
	} else if (!S_ISDIR(cdir_stat.st_mode))
		eprintf("Caption directory (%s) exists, but is not a directory.",
			caption_dir);

	free(caption_dir);

	caption_filename = estrjoin("", dir, "/", opt.caption_path, "/", file->name, ".txt", NULL);
	free(dir);
	return caption_filename;
}

void feh_draw_caption(winwidget w)
{
	static Imlib_Font fn = NULL;
	int tw = 0, th = 0, ww, hh;
	int x, y;
	Imlib_Image im = NULL;
	char *p;
	gib_list *lines, *l;
	static gib_style *caption_style = NULL;
	feh_file *file;

	if (!w->file) {
		return;
	}
	file = FEH_FILE(w->file->data);
	if (!file->filename) {
		return;
	}

	if (!file->caption) {
		char *caption_filename;
		caption_filename = build_caption_filename(file, 0);
		if (caption_filename)
			/* read caption from file */
			file->caption = ereadfile(caption_filename);
		else
			file->caption = estrdup("");
		free(caption_filename);
	}

	if (file->caption == NULL) {
		/* caption file is not there, we want to cache that, otherwise we'll stat
		 * the damn file every time we render the image. Reloading an image will
		 * always cause the caption to be reread though so we're safe to do so.
		 * (Before this bit was added, when zooming a captionless image with
		 * captions enabled, the captions file would be stat()d like 30 times a
		 * second) - don't forget this function is called from
		 * winwidget_render_image().
		 */
		file->caption = estrdup("");
	}

	if (*(file->caption) == '\0' && !w->caption_entry)
		return;

	caption_style = gib_style_new("caption");
	caption_style->bits = gib_list_add_front(caption_style->bits,
		gib_style_bit_new(0, 0, 0, 0, 0, 0));
	caption_style->bits = gib_list_add_front(caption_style->bits,
		gib_style_bit_new(1, 1, 0, 0, 0, 255));

	fn = feh_load_font(w);

	if (*(file->caption) == '\0') {
		p = estrdup("Caption entry mode - Hit ESC to cancel");
		lines = feh_wrap_string(p, w->w, fn, NULL);
		free(p);
	} else
		lines = feh_wrap_string(file->caption, w->w, fn, NULL);

	if (!lines)
		return;

	/* Work out how high/wide the caption is */
	l = lines;
	while (l) {
		p = (char *) l->data;
		gib_imlib_get_text_size(fn, p, caption_style, &ww, &hh, IMLIB_TEXT_TO_RIGHT);
		if (ww > tw)
			tw = ww;
		th += hh;
		if (l->next)
			th += 1;	/* line spacing */
		l = l->next;
	}

	/* we don't want the caption overlay larger than our window */
	if (th > w->h)
		th = w->h;
	if (tw > w->w)
		tw = w->w;

	im = imlib_create_image(tw, th);
	if (!im)
		eprintf("Couldn't create image. Out of memory?");

	feh_imlib_image_fill_text_bg(im, tw, th);

	l = lines;
	x = 0;
	y = 0;
	while (l) {
		p = (char *) l->data;
		gib_imlib_get_text_size(fn, p, caption_style, &ww, &hh, IMLIB_TEXT_TO_RIGHT);
		x = (tw - ww) / 2;
		if (w->caption_entry && (*(file->caption) == '\0'))
			gib_imlib_text_draw(im, fn, caption_style, x, y, p,
				IMLIB_TEXT_TO_RIGHT, 255, 255, 127, 255);
		else if (w->caption_entry)
			gib_imlib_text_draw(im, fn, caption_style, x, y, p,
				IMLIB_TEXT_TO_RIGHT, 255, 255, 0, 255);
		else
			gib_imlib_text_draw(im, fn, caption_style, x, y, p,
				IMLIB_TEXT_TO_RIGHT, 255, 255, 255, 255);

		y += hh + 1;	/* line spacing */
		l = l->next;
	}

	gib_imlib_render_image_on_drawable(w->bg_pmap, im, (w->w - tw) / 2, w->h - th, 1, 1, 0);
	gib_imlib_free_image_and_decache(im);
	gib_list_free_and_data(lines);
	return;
}

unsigned char reset_output = 0;

void feh_display_status(char stat)
{
	static int i = 0;
	static int init_len = 0;
	int j = 0;

	D(("filelist %p, filelist->next %p\n", filelist, filelist->next));

	if (!stat) {
		putc('\n', stdout);
		init_len = 0;
		i = 0;
		return;
	}

	if (!init_len)
		init_len = gib_list_length(filelist);

	if (i) {
		if (reset_output) {
			/* There's just been an error message. Unfortunate ;) */
			for (j = 0; j < (((i % 50) + ((i % 50) / 10)) + 7); j++)
				putc(' ', stdout);
		}

		if (!(i % 50)) {
			int len = gib_list_length(filelist);

			fprintf(stdout, " %5d/%d (%d)\n[%3d%%] ",
					i, init_len, len, ((int) ((float) i / init_len * 100)));

		} else if ((!(i % 10)) && (!reset_output))
			putc(' ', stdout);

		reset_output = 0;
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
	if (!w->file || !w->file->data || !FEH_FILE(w->file->data)->filename)
		return;

	if (!strcmp(gib_imlib_image_format(w->im), "jpeg")) {
		feh_edit_inplace_lossless(w, op);
		feh_reload_image(w, 1, 1);
		return;
	}

	ret = feh_load_image(&old, FEH_FILE(w->file->data));
	if (ret) {
		if (op == INPLACE_EDIT_FLIP) {
			imlib_context_set_image(old);
			imlib_image_flip_vertical();
		} else if (op == INPLACE_EDIT_MIRROR) {
			imlib_context_set_image(old);
			imlib_image_flip_horizontal();
		} else
			gib_imlib_image_orientate(old, op);
		gib_imlib_save_image(old, FEH_FILE(w->file->data)->filename);
		gib_imlib_free_image(old);
		feh_reload_image(w, 1, 1);
	} else {
		im_weprintf(w, "failed to load image from disk to edit it in place");
	}

	return;
}

gib_list *feh_wrap_string(char *text, int wrap_width, Imlib_Font fn, gib_style * style)
{
	gib_list *ll, *lines = NULL, *list = NULL, *words;
	gib_list *l = NULL;
	char delim[2] = { '\n', '\0' };
	int w, line_width;
	int tw, th;
	char *p, *pp;
	char *line = NULL;
	char *temp;
	int space_width = 0, m_width = 0, t_width = 0, new_width = 0;

	lines = gib_string_split(text, delim);

	if (wrap_width) {
		gib_imlib_get_text_size(fn, "M M", style, &t_width, NULL, IMLIB_TEXT_TO_RIGHT);
		gib_imlib_get_text_size(fn, "M", style, &m_width, NULL, IMLIB_TEXT_TO_RIGHT);
		space_width = t_width - (2 * m_width);
		w = wrap_width;
		l = lines;
		while (l) {
			line_width = 0;
			p = (char *) l->data;
			/* quick check to see if whole line fits okay */
			gib_imlib_get_text_size(fn, p, style, &tw, &th, IMLIB_TEXT_TO_RIGHT);
			if (tw <= w) {
				list = gib_list_add_end(list, estrdup(p));
			} else if (strlen(p) == 0) {
				list = gib_list_add_end(list, estrdup(""));
			} else if (!strcmp(p, " ")) {
				list = gib_list_add_end(list, estrdup(" "));
			} else {
				words = gib_string_split(p, " ");
				if (words) {
					ll = words;
					while (ll) {
						pp = (char *) ll->data;
						if (strcmp(pp, " ")) {
							gib_imlib_get_text_size
							    (fn, pp, style, &tw, &th, IMLIB_TEXT_TO_RIGHT);
							if (line_width == 0)
								new_width = tw;
							else
								new_width = line_width + space_width + tw;
							if (new_width <= w) {
								/* add word to line */
								if (line) {
									int len;

									len = strlen(line)
									    + strlen(pp)
									    + 2;
									temp = emalloc(len);
									snprintf(temp, len, "%s %s", line, pp);
									free(line);
									line = temp;
								} else {
									line = estrdup(pp);
								}
								line_width = new_width;
							} else if (line_width == 0) {
								/* can't fit single word in :/
								   increase width limit to width of word
								   and jam the bastard in anyhow */
								w = tw;
								line = estrdup(pp);
								line_width = new_width;
							} else {
								/* finish this line, start next and add word there */
								if (line) {
									list = gib_list_add_end(list, estrdup(line));
									free(line);
									line = NULL;
								}
								line = estrdup(pp);
								line_width = tw;
							}
						}
						ll = ll->next;
					}
					if (line) {
						/* finish last line */
						list = gib_list_add_end(list, estrdup(line));
						free(line);
						line = NULL;
						line_width = 0;
					}
					gib_list_free_and_data(words);
				}
			}
			l = l->next;
		}
		gib_list_free_and_data(lines);
		lines = list;
	}
	return lines;
}

void feh_edit_inplace_lossless(winwidget w, int op)
{
	char *filename = FEH_FILE(w->file->data)->filename;
	int len = strlen(filename) + 1;
	char *file_str = emalloc(len);
	int pid, status;
	char op_name[]  = "rotate";     /* message */
	char op_op[]    = "-rotate";    /* jpegtran option */
	char op_value[] = "horizontal"; /* jpegtran option's value */

	if (op == INPLACE_EDIT_FLIP) {
		sprintf(op_name,  "flip");
		sprintf(op_op,    "-flip");
		sprintf(op_value, "vertical");
	} else if (op == INPLACE_EDIT_MIRROR) {
		sprintf(op_name,  "mirror");
		sprintf(op_op,    "-flip");
	} else
		snprintf(op_value, 4, "%d", 90 * op);

	snprintf(file_str, len, "%s", filename);

	if ((pid = fork()) < 0) {
		im_weprintf(w, "lossless %s: fork failed:", op_name);
		exit(1);
	} else if (pid == 0) {

		execlp("jpegtran", "jpegtran", "-copy", "all", op_op, op_value,
				"-outfile", file_str, file_str, NULL);

		im_weprintf(w, "lossless %s: Is 'jpegtran' installed? Failed to exec:", op_name);
		exit(1);
	} else {
		waitpid(pid, &status, 0);

		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
			im_weprintf(w,
					"lossless %s: Got exitcode %d from jpegtran."
					" Commandline was: "
					"jpegtran -copy all %s %s -outfile %s %s",
					op_name, status >> 8, op_op, op_value, file_str, file_str);
			free(file_str);
			return;
		}
	}
	free(file_str);
}

void feh_draw_actions(winwidget w)
{
	static Imlib_Font fn = NULL;
	int tw = 0, th = 0;
	int th_offset = 0;
	int max_tw = 0;
	int line_th = 0;
	Imlib_Image im = NULL;
	int i = 0;
	int num_actions = 0;
	int cur_action = 0;
	char index[2];
	char *line;

	/* Count number of defined actions. This method sucks a bit since it needs
	 * to be changed if the number of actions changes, but at least it doesn't
	 * miss actions 2 to 9 if action1 isn't defined
	 */
	for (i = 0; i < 10; i++) {
		if (opt.actions[i])
			num_actions++;
	}

	if (num_actions == 0)
		return;

	if ((!w->file) || (!FEH_FILE(w->file->data))
			|| (!FEH_FILE(w->file->data)->filename))
		return;

	fn = feh_load_font(w);

	gib_imlib_get_text_size(fn, "defined actions:", NULL, &tw, &th, IMLIB_TEXT_TO_RIGHT);
/* Check for the widest line */
	max_tw = tw;

	for (i = 0; i < 10; i++) {
		if (opt.actions[i]) {
			line = emalloc(strlen(opt.actions[i]) + 5);
			strcpy(line, "0: ");
			line = strcat(line, opt.actions[i]);
			gib_imlib_get_text_size(fn, line, NULL, &tw, &th, IMLIB_TEXT_TO_RIGHT);
			free(line);
			if (tw > max_tw)
				max_tw = tw;
		}
	}

	tw = max_tw;
	tw += 3;
	th += 3;
	line_th = th;
	th = (th * num_actions) + line_th;

	/* This depends on feh_draw_filename internals...
	 * should be fixed some time
	 */
	if (opt.draw_filename)
		th_offset = line_th * 2;

	im = imlib_create_image(tw, th);
	if (!im)
		eprintf("Couldn't create image. Out of memory?");

	feh_imlib_image_fill_text_bg(im, tw, th);

	gib_imlib_text_draw(im, fn, NULL, 2, 2, "defined actions:", IMLIB_TEXT_TO_RIGHT, 0, 0, 0, 255);
	gib_imlib_text_draw(im, fn, NULL, 1, 1, "defined actions:", IMLIB_TEXT_TO_RIGHT, 255, 255, 255, 255);

	for (i = 0; i < 10; i++) {
		if (opt.actions[i]) {
			cur_action++;
			line = emalloc(strlen(opt.actions[i]) + 5);
			sprintf(index, "%d", i);
			strcpy(line, index);
			strcat(line, ": ");
			strcat(line, opt.actions[i]);

			gib_imlib_text_draw(im, fn, NULL, 2,
					(cur_action * line_th) + 2, line,
					IMLIB_TEXT_TO_RIGHT, 0, 0, 0, 255);
			gib_imlib_text_draw(im, fn, NULL, 1,
					(cur_action * line_th) + 1, line,
					IMLIB_TEXT_TO_RIGHT, 255, 255, 255, 255);
			free(line);
		}
	}

	gib_imlib_render_image_on_drawable(w->bg_pmap, im, 0, 0 + th_offset, 1, 1, 0);

	gib_imlib_free_image_and_decache(im);
	return;
}
