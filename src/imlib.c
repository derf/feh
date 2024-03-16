/* imlib.c

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

#include "feh.h"
#include "filelist.h"
#include "signals.h"
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

#ifdef HAVE_LIBMAGIC
#include <magic.h>

magic_t magic = NULL;
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

gib_hash* conversion_cache = NULL;

int childpid = 0;

static int feh_file_is_raw(char *filename);
static char *feh_http_load_image(char *url);
static char *feh_dcraw_load_image(char *filename);
static char *feh_magick_load_image(char *filename);

#ifdef HAVE_LIBXINERAMA
void init_xinerama(void)
{
	if (opt.xinerama && XineramaIsActive(disp)) {
		int major, minor, px, py, i;

		/* discarded */
		Window dw;
		int di;
		unsigned int du;

		XineramaQueryVersion(disp, &major, &minor);
		xinerama_screens = XineramaQueryScreens(disp, &num_xinerama_screens);

		if (opt.xinerama_index >= 0)
			xinerama_screen = opt.xinerama_index;
		else {
			xinerama_screen = 0;
			XQueryPointer(disp, root, &dw, &dw, &px, &py, &di, &di, &du);
			for (i = 0; i < num_xinerama_screens; i++) {
				if (XY_IN_RECT(px, py,
							xinerama_screens[i].x_org,
							xinerama_screens[i].y_org,
							xinerama_screens[i].width,
							xinerama_screens[i].height)) {
					xinerama_screen = i;
					break;
				}
			}
		}
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

	imlib_set_cache_size(opt.cache_size * 1024 * 1024);

	return;
}

int feh_should_ignore_image(Imlib_Image * im)
{
	if (opt.filter_by_dimensions) {
		unsigned int w = gib_imlib_image_get_width(im);
		unsigned int h = gib_imlib_image_get_height(im);
		if (w < opt.min_width || w > opt.max_width || h < opt.min_height || h > opt.max_height) {
			return 1;
		}
	}
	return 0;
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

void feh_print_load_error(char *file, winwidget w, Imlib_Load_Error err, enum feh_load_error feh_err)
{
	if (err == IMLIB_LOAD_ERROR_OUT_OF_FILE_DESCRIPTORS)
		eprintf("%s - Out of file descriptors while loading", file);
	else if (!opt.quiet || w) {
		switch (feh_err) {
			case LOAD_ERROR_IMLIB:
				// handled in the next switch/case statement
				break;
			case LOAD_ERROR_IMAGEMAGICK:
				im_weprintf(w, "%s - No ImageMagick loader for that file format", file);
				break;
			case LOAD_ERROR_CURL:
				im_weprintf(w, "%s - libcurl was unable to retrieve the file", file);
				break;
			case LOAD_ERROR_DCRAW:
				im_weprintf(w, "%s - Unable to open preview via dcraw", file);
				break;
			case LOAD_ERROR_MAGICBYTES:
				im_weprintf(w, "%s - Does not look like an image (magic bytes missing)", file);
				break;
		}
		if (feh_err != LOAD_ERROR_IMLIB) {
			return;
		}
		switch (err) {
		case IMLIB_LOAD_ERROR_FILE_DOES_NOT_EXIST:
			im_weprintf(w, "%s - File does not exist", file);
			break;
		case IMLIB_LOAD_ERROR_FILE_IS_DIRECTORY:
			im_weprintf(w, "%s - Directory specified for image filename", file);
			break;
		case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_READ:
			im_weprintf(w, "%s - No read access", file);
			break;
		case IMLIB_LOAD_ERROR_UNKNOWN:
		case IMLIB_LOAD_ERROR_NO_LOADER_FOR_FILE_FORMAT:
			im_weprintf(w, "%s - No Imlib2 loader for that file format", file);
			break;
		case IMLIB_LOAD_ERROR_PATH_TOO_LONG:
			im_weprintf(w, "%s - Path specified is too long", file);
			break;
		case IMLIB_LOAD_ERROR_PATH_COMPONENT_NON_EXISTANT:
			im_weprintf(w, "%s - Path component does not exist", file);
			break;
		case IMLIB_LOAD_ERROR_PATH_COMPONENT_NOT_DIRECTORY:
			im_weprintf(w, "%s - Path component is not a directory", file);
			break;
		case IMLIB_LOAD_ERROR_PATH_POINTS_OUTSIDE_ADDRESS_SPACE:
			im_weprintf(w, "%s - Path points outside address space", file);
			break;
		case IMLIB_LOAD_ERROR_TOO_MANY_SYMBOLIC_LINKS:
			im_weprintf(w, "%s - Too many levels of symbolic links", file);
			break;
		case IMLIB_LOAD_ERROR_OUT_OF_MEMORY:
			im_weprintf(w, "While loading %s - Out of memory", file);
			break;
		case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_WRITE:
			im_weprintf(w, "%s - Cannot write to directory", file);
			break;
		case IMLIB_LOAD_ERROR_OUT_OF_DISK_SPACE:
			im_weprintf(w, "%s - Cannot write - out of disk space", file);
			break;
#if defined(IMLIB2_VERSION_MAJOR) && defined(IMLIB2_VERSION_MINOR) && (IMLIB2_VERSION_MAJOR > 1 || IMLIB2_VERSION_MINOR > 7)
		case IMLIB_LOAD_ERROR_IMAGE_READ:
			im_weprintf(w, "%s - Invalid image file", file);
			break;
		case IMLIB_LOAD_ERROR_IMAGE_FRAME:
			im_weprintf(w, "%s - Requested frame not in image", file);
			break;
#endif
		default:
			im_weprintf(w, "While loading %s - Unknown error (%d)",
					file, err);
			break;
		}
	}
}

#ifdef HAVE_LIBMAGIC
void uninit_magic(void)
{
	if (!magic) {
		return;
	}

	magic_close(magic);
	magic = NULL;
}
void init_magic(void)
{
	if (getenv("FEH_SKIP_MAGIC")) {
		return;
	}

	if (!(magic = magic_open(MAGIC_NONE))) {
		weprintf("unable to initialize magic library\n");
		return;
	}

	if (magic_load(magic, NULL) != 0) {
		weprintf("cannot load magic database: %s\n", magic_error(magic));
		uninit_magic();
	}
}

/*
 * This is a workaround for an Imlib2 regression, causing unloadable image
 * detection to be excessively slow (and, thus, causing feh to hang for a while
 * when encountering an unloadable image). We use magic byte detection to
 * avoid calling Imlib2 for files it probably cannot handle. See
 * <https://phab.enlightenment.org/T8739> and
 * <https://github.com/derf/feh/issues/505>.
 */
int feh_is_image(feh_file * file, int magic_flags)
{
	const char * mime_type = NULL;

	if (!magic) {
		return 1;
	}

	magic_setflags(magic, MAGIC_MIME_TYPE | MAGIC_SYMLINK | magic_flags);
	mime_type = magic_file(magic, file->filename);

	if (!mime_type) {
		return 0;
	}

	D(("file %s has mime type: %s\n", file->filename, mime_type));

	if (strncmp(mime_type, "image/", 6) == 0) {
		return 1;
	}

	/* no infinite loop on compressed content, please */
	if (magic_flags) {
		return 0;
	}

	/* imlib2 supports loading compressed images, let's have a look inside */
	if (strcmp(mime_type, "application/gzip") == 0 ||
	    strcmp(mime_type, "application/x-bzip2") == 0 ||
	    strcmp(mime_type, "application/x-xz") == 0) {
		return feh_is_image(file, MAGIC_COMPRESS);
	}

	return 0;
}
#else
int feh_is_image(__attribute__((unused)) feh_file * file, __attribute__((unused)) int magic_flags)
{
	return 1;
}
#endif

int feh_load_image(Imlib_Image * im, feh_file * file)
{
	Imlib_Load_Error err = IMLIB_LOAD_ERROR_NONE;
	enum feh_load_error feh_err = LOAD_ERROR_IMLIB;
	enum { SRC_IMLIB, SRC_HTTP, SRC_MAGICK, SRC_DCRAW } image_source = SRC_IMLIB;
	char *tmpname = NULL;
	char *real_filename = NULL;

	D(("filename is %s, image is %p\n", file->filename, im));

	if (!file || !file->filename)
		return 0;

	if (path_is_url(file->filename)) {
		image_source = SRC_HTTP;

		if ((tmpname = feh_http_load_image(file->filename)) == NULL) {
			feh_err = LOAD_ERROR_CURL;
			err = IMLIB_LOAD_ERROR_FILE_DOES_NOT_EXIST;
		}
	}
	else {
		if (feh_is_image(file, 0)) {
			*im = imlib_load_image_with_error_return(file->filename, &err);
		} else {
			feh_err = LOAD_ERROR_MAGICBYTES;
			err = IMLIB_LOAD_ERROR_NO_LOADER_FOR_FILE_FORMAT;
		}
	}

	if (opt.conversion_timeout >= 0 && (
			(err == IMLIB_LOAD_ERROR_UNKNOWN) ||
			(err == IMLIB_LOAD_ERROR_NO_LOADER_FOR_FILE_FORMAT))) {
		if (feh_file_is_raw(file->filename)) {
			image_source = SRC_DCRAW;
			tmpname = feh_dcraw_load_image(file->filename);
			if (!tmpname) {
				feh_err = LOAD_ERROR_DCRAW;
			}
		} else {
			image_source = SRC_MAGICK;
			feh_err = LOAD_ERROR_IMLIB;
			tmpname = feh_magick_load_image(file->filename);
			if (!tmpname) {
				feh_err = LOAD_ERROR_IMAGEMAGICK;
			}
		}
	}

	if (tmpname) {
		*im = imlib_load_image_with_error_return(tmpname, &err);
		if (!err && im) {
			real_filename = file->filename;
			file->filename = tmpname;

			/*
			 * feh does not associate a non-native image with its temporary
			 * filename and may delete the temporary file right after loading.
			 * To ensure that it is still aware of image size, dimensions, etc.,
			 * file_info is preloaded here. To avoid a memory leak when loading
			 * a non-native file multiple times in a slideshow, the file_info
			 * struct is freed first. If file->info is not set, feh_file_info_free
			 * is a no-op.
			 */
			feh_file_info_free(file->info);
			feh_file_info_load(file, *im);

			file->filename = real_filename;
#ifdef HAVE_LIBEXIF
			/*
			 * if we're called from within feh_reload_image, file->ed is already
			 * populated.
			 */
			if (file->ed) {
				exif_data_unref(file->ed);
			}
			file->ed = exif_data_new_from_file(tmpname);
#endif
		}
		if (!opt.use_conversion_cache && ((image_source != SRC_HTTP) || !opt.keep_http))
			unlink(tmpname);
		// keep_http already performs an add_file_to_rm_filelist call
		else if (opt.use_conversion_cache && !opt.keep_http)
			// add_file_to_rm_filelist duplicates tmpname
			add_file_to_rm_filelist(tmpname);

		if (!opt.use_conversion_cache)
			free(tmpname);
	} else if (im) {
#ifdef HAVE_LIBEXIF
		/*
			* if we're called from within feh_reload_image, file->ed is already
			* populated.
			*/
		if (file->ed) {
			exif_data_unref(file->ed);
		}
		file->ed = exif_data_new_from_file(file->filename);
#endif
	}

	if ((err) || (!im)) {
		if (opt.verbose && !opt.quiet) {
			fputs("\n", stderr);
			reset_output = 1;
		}
		feh_print_load_error(file->filename, NULL, err, feh_err);
		D(("Load *failed*\n"));
		return(0);
	}

	/*
	 * By default, Imlib2 unconditionally loads a cached file without checking
	 * if it was modified on disk. However, feh (or rather its users) should
	 * expect image changes to appear at the next reload. So we tell Imlib2 to
	 * always check the file modification time and only use a cached image if
	 * the mtime was not changed. The performance penalty is usually negligible.
	 */
	imlib_context_set_image(*im);
	imlib_image_set_changes_on_disk();

#ifdef HAVE_LIBEXIF
	int orientation = 0;
	if (file->ed) {
		ExifByteOrder byteOrder = exif_data_get_byte_order(file->ed);
		ExifEntry *exifEntry = exif_data_get_entry(file->ed, EXIF_TAG_ORIENTATION);
		if (exifEntry && opt.auto_rotate) {
			orientation = exif_get_short(exifEntry->data, byteOrder);
		}
	}

	if (orientation == 2)
		gib_imlib_image_flip_horizontal(*im);
	else if (orientation == 3)
		gib_imlib_image_orientate(*im, 2);
	else if (orientation == 4)
		gib_imlib_image_flip_vertical(*im);
	else if (orientation == 5) {
		gib_imlib_image_orientate(*im, 3);
		gib_imlib_image_flip_vertical(*im);
	}
	else if (orientation == 6)
		gib_imlib_image_orientate(*im, 1);
	else if (orientation == 7) {
		gib_imlib_image_orientate(*im, 3);
		gib_imlib_image_flip_horizontal(*im);
	}
	else if (orientation == 8)
		gib_imlib_image_orientate(*im, 3);
#endif

	D(("Loaded ok\n"));
	return(1);
}

void feh_reload_image(winwidget w, int resize, int force_new)
{
	char *new_title;
	int len;
	Imlib_Image tmp;
	int old_w, old_h;

	if (!w->file) {
		im_weprintf(w, "couldn't reload, this image has no file associated with it.");
		winwidget_render_image(w, 0, 0);
		return;
	}

	D(("resize %d, force_new %d\n", resize, force_new));

	free(FEH_FILE(w->file->data)->caption);
	FEH_FILE(w->file->data)->caption = NULL;

	len = strlen(w->name) + sizeof("Reloading: ") + 1;
	new_title = emalloc(len);
	snprintf(new_title, len, "Reloading: %s", w->name);
	winwidget_rename(w, new_title);
	free(new_title);

	old_w = gib_imlib_image_get_width(w->im);
	old_h = gib_imlib_image_get_height(w->im);

	/*
	 * If we don't free the old image before loading the new one, Imlib2's
	 * caching will get in our way.
	 * However, if --reload is used (force_new == 0), we want to continue if
	 * the new image cannot be loaded, so we must not free the old image yet.
	 */
	if (force_new)
		winwidget_free_image(w);

	// if it's an external image, our own cache will also get in your way
	char *sfn;
	if (opt.use_conversion_cache && conversion_cache && (sfn = gib_hash_get(conversion_cache, FEH_FILE(w->file->data)->filename)) != NULL) {
		free(sfn);
		gib_hash_set(conversion_cache, FEH_FILE(w->file->data)->filename, NULL);
	}

	if ((feh_load_image(&tmp, FEH_FILE(w->file->data))) == 0) {
		if (force_new)
			eprintf("failed to reload image\n");
		else {
			im_weprintf(w, "Couldn't reload image. Is it still there?");
			winwidget_render_image(w, 0, 0);
		}
		return;
	}

	if (!resize && ((old_w != gib_imlib_image_get_width(tmp)) ||
			(old_h != gib_imlib_image_get_height(tmp))))
		resize = 1;

	if (!force_new)
		winwidget_free_image(w);

	w->im = tmp;
	winwidget_reset_image(w);

	w->mode = MODE_NORMAL;
	if ((w->im_w != gib_imlib_image_get_width(w->im))
	    || (w->im_h != gib_imlib_image_get_height(w->im)))
		w->had_resize = 1;
	if (w->has_rotated) {
		Imlib_Image temp;

		temp = gib_imlib_create_rotated_image(w->im, 0.0);
		w->im_w = gib_imlib_image_get_width(temp);
		w->im_h = gib_imlib_image_get_height(temp);
		gib_imlib_free_image_and_decache(temp);
	} else {
		w->im_w = gib_imlib_image_get_width(w->im);
		w->im_h = gib_imlib_image_get_height(w->im);
	}
	winwidget_render_image(w, resize, 0);

	return;
}

static int feh_file_is_raw(char *filename)
{
	childpid = fork();
	if (childpid == -1) {
		perror("fork");
		return 0;
	}

	if (childpid == 0) {
		int devnull = open("/dev/null", O_WRONLY);
		dup2(devnull, 1);
		dup2(devnull, 2);
		execlp("dcraw", "dcraw", "-i", filename, NULL);
		_exit(1);
	} else {
		int status;
		do {
			waitpid(childpid, &status, WUNTRACED);
			if (WIFEXITED(status)) {
				return !WEXITSTATUS(status);
			}
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}

	return 0;
}

static char *feh_dcraw_load_image(char *filename)
{
	char *basename;
	char *tmpname;
	char *sfn;
	int fd = -1;

	if (opt.use_conversion_cache) {
		if (!conversion_cache)
			conversion_cache = gib_hash_new();
		if ((sfn = gib_hash_get(conversion_cache, filename)) != NULL)
			return sfn;
	}

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

	if (fd == -1) {
		free(sfn);
		return NULL;
	}

	childpid = fork();
	if (childpid == -1) {
		weprintf("%s: Can't load with dcraw. Fork failed:", filename);
		unlink(sfn);
		free(sfn);
		close(fd);
		return NULL;
	} else if (childpid == 0) {
		dup2(fd, STDOUT_FILENO);
		close(fd);

		alarm(opt.conversion_timeout);
		execlp("dcraw", "dcraw", "-c", "-e", filename, NULL);
		_exit(1);
	}

	int status;
	waitpid(-1, &status, 0);
	if (WIFSIGNALED(status)) {
		unlink(sfn);
		free(sfn);
		sfn = NULL;
		if (!opt.quiet)
			weprintf("%s - Conversion took too long, skipping", filename);
	}

	if ((sfn != NULL) && opt.use_conversion_cache)
		gib_hash_set(conversion_cache, filename, sfn);

	return sfn;
}

static char *feh_magick_load_image(char *filename)
{
	char *argv_fn;
	char *basename;
	char *tmpname;
	char *sfn;
	char tempdir[] = "/tmp/.feh-magick-tmp-XXXXXX";
	int fd = -1, devnull = -1;
	int status;
	char created_tempdir = 0;

	if (opt.use_conversion_cache) {
		if (!conversion_cache)
			conversion_cache = gib_hash_new();
		if ((sfn = gib_hash_get(conversion_cache, filename)) != NULL)
			return sfn;
	}

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

	if (fd == -1) {
		free(sfn);
		return NULL;
	}

	/*
	 * We could use png:fd:(whatever mkstemp returned) as target filename
	 * for convert, but this seems to be broken in some ImageMagick versions.
	 * So we resort to png:(sfn) instead.
	 */
	argv_fn = estrjoin(":", "png", sfn, NULL);

	/*
	 * By default, ImageMagick saves (occasionally lots of) temporary files
	 * in /tmp. It doesn't remove them if it runs into a timeout and is killed
	 * by us, no matter whether we use SIGINT, SIGTERM or SIGKILL. So, unless
	 * MAGICK_TMPDIR has already been set by the user, we create our own
	 * temporary directory for ImageMagick and remove its contents at the end of
	 * this function.
	 */
	if (getenv("MAGICK_TMPDIR") == NULL) {
		if (mkdtemp(tempdir) == NULL) {
			weprintf("%s: ImageMagick may leave temporary files in /tmp. mkdtemp failed:", filename);
		} else {
			created_tempdir = 1;
		}
	}

	if ((childpid = fork()) < 0) {
		weprintf("%s: Can't load with imagemagick. Fork failed:", filename);
		unlink(sfn);
		free(sfn);
		sfn = NULL;
	}
	else if (childpid == 0) {

		devnull = open("/dev/null", O_WRONLY);
		dup2(devnull, 0);
		if (opt.quiet) {
			/* discard convert output */
			dup2(devnull, 1);
			dup2(devnull, 2);
		}

		/*
		 * convert only accepts SIGINT via killpg, a normal kill doesn't work
		 */
		setpgid(0, 0);

		if (created_tempdir) {
			// no error checking - this is a best-effort code path
			setenv("MAGICK_TMPDIR", tempdir, 0);
		}

		execlp("convert", "convert", filename, argv_fn, NULL);
		_exit(1);
	}
	else {
		alarm(opt.conversion_timeout);
		waitpid(childpid, &status, 0);
		kill(childpid, SIGKILL);
		if (opt.conversion_timeout > 0 && !alarm(0)) {
			unlink(sfn);
			free(sfn);
			sfn = NULL;

			if (!opt.quiet) {
				weprintf("%s: Conversion took too long, skipping", filename);
			}
		}
		close(fd);
		childpid = 0;
	}

	if (created_tempdir) {
		DIR *dir;
		struct dirent *de;
		if ((dir = opendir(tempdir)) == NULL) {
			weprintf("%s: Cannot remove temporary ImageMagick files from %s:", filename, tempdir);
		} else {
			while ((de = readdir(dir)) != NULL) {
				if (de->d_name[0] != '.') {
					char *temporary_file_name = estrjoin("/", tempdir, de->d_name, NULL);
					/*
					 * We assume that ImageMagick only creates temporary files and
					 * not directories.
					 */
					if (unlink(temporary_file_name) == -1) {
						weprintf("unlink %s:", temporary_file_name);
					}
					free(temporary_file_name);
				}
			}
			if (rmdir(tempdir) == -1) {
				weprintf("rmdir %s:", tempdir);
			}
		}
		closedir(dir);
	}

	free(argv_fn);

	if ((sfn != NULL) && opt.use_conversion_cache)
		gib_hash_set(conversion_cache, filename, sfn);

	return sfn;
}

#ifdef HAVE_LIBCURL

#if LIBCURL_VERSION_NUM >= 0x072000 /* 07.32.0 */
static int curl_quit_function(void *clientp,  curl_off_t dltotal,  curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
#else
static int curl_quit_function(void *clientp,  double dltotal,  double dlnow, double ultotal, double ulnow)
#endif
{
	// ignore "unused parameter" warnings
	(void)clientp;
	(void)dltotal;
	(void)dlnow;
	(void)ultotal;
	(void)ulnow;
	if (sig_exit) {
		/*
		 * The user wants to quit feh. Tell libcurl to abort the transfer and
		 * return control to the main loop, where we can quit gracefully.
		 */
		return 1;
	}
	return 0;
}

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

	if (opt.use_conversion_cache) {
		if (!conversion_cache)
			conversion_cache = gib_hash_new();
		if ((sfn = gib_hash_get(conversion_cache, url)) != NULL)
			return sfn;
	}

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

#ifdef HAVE_MKSTEMPS
	tmpname = estrjoin("_", "feh_curl_XXXXXX", basename, NULL);

	if (strlen(tmpname) > NAME_MAX) {
		tmpname[NAME_MAX] = '\0';
	}
#else
	if (strlen(basename) > NAME_MAX-7) {
		tmpname = estrdup("feh_curl_XXXXXX");
	} else {
		tmpname = estrjoin("_", "feh_curl", basename, "XXXXXX", NULL);
	}
#endif

	sfn = estrjoin("", path, tmpname, NULL);
	free(tmpname);

	D(("sfn is %s\n", sfn))

#ifdef HAVE_MKSTEMPS
	fd = mkstemps(sfn, strlen(basename) + 1);
#else
	fd = mkstemp(sfn);
#endif

	if (fd != -1) {
		sfp = fdopen(fd, "w+");
		if (sfp != NULL) {
#ifdef DEBUG
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
#endif
			/*
			 * Do not allow requests to take longer than 30 minutes.
			 * This should be sufficiently high to accommodate use cases with
			 * unusually high latencies, while at the same time avoiding
			 * feh hanging indefinitely in unattended slideshows.
			 */
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1800);
			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, sfp);
			ebuff = emalloc(CURL_ERROR_SIZE);
			curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, ebuff);
			curl_easy_setopt(curl, CURLOPT_USERAGENT, PACKAGE "/" VERSION);
			curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
#if LIBCURL_VERSION_NUM >= 0x072000 /* 07.32.0 */
			curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, curl_quit_function);
#else
			curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, curl_quit_function);
#endif
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
			if (opt.insecure_ssl) {
				curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
				curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
			} else if (getenv("CURL_CA_BUNDLE") != NULL) {
				// Allow the user to specify custom CA certificates.
				curl_easy_setopt(curl, CURLOPT_CAINFO,
						getenv("CURL_CA_BUNDLE"));
			}

			res = curl_easy_perform(curl);
			curl_easy_cleanup(curl);
			if (res != CURLE_OK) {
				if (res != CURLE_ABORTED_BY_CALLBACK) {
					weprintf("open url: %s", ebuff);
				}
				unlink(sfn);
				close(fd);
				free(sfn);
				sfn = NULL;
			}

			free(ebuff);
			fclose(sfp);
			if (opt.use_conversion_cache)
				gib_hash_set(conversion_cache, url, sfn);
			return sfn;
		} else {
			weprintf("open url: fdopen failed:");
			unlink(sfn);
			free(sfn);
			close(fd);
		}
	} else {
#ifdef HAVE_MKSTEMPS
		weprintf("open url: mkstemps failed:");
#else
		weprintf("open url: mkstemp failed:");
#endif
		free(sfn);
	}
	curl_easy_cleanup(curl);
	return NULL;
}

#else				/* HAVE_LIBCURL */

char *feh_http_load_image(char *url)
{
	weprintf(
		"Cannot load image %s\nPlease recompile feh with libcurl support",
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
	if (!w)
		free(errstr);
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
		if (w->file)
			snprintf(s, len, "%d of %d", gib_list_num(filelist, w->file) +
					1, gib_list_length(filelist));
		else
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

	if (buffer[0] == '\0')
	{
		snprintf(buffer, EXIF_MAX_DATA, "%s", "Failed to run exif command");
		gib_imlib_get_text_size(fn, buffer, NULL, &width, &height, IMLIB_TEXT_TO_RIGHT);
		info_buf[no_lines] = estrdup(buffer);
		no_lines++;
	}
	else
	{

		while ( (no_lines < 128) && (pos < EXIF_MAX_DATA) )
		{
			/* max 128 lines */
			pos2 = 0;
			while ( pos2 < 255 ) /* max 255 chars + 1 null byte per line */
			{
				if ( (buffer[pos] != '\n')
				      && (buffer[pos] != '\0') )
				{
					info_line[pos2] = buffer[pos];
				}
				else if ( buffer[pos] == '\0' )
				{
					pos = EXIF_MAX_DATA; /* all data seen */
					break;
				}
				else
				{
					pos++; /* line finished, continue with next line*/
					break;
				}

				pos++;
				pos2++;
			}
			info_line[pos2] = '\0';

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
		free(info_buf[i]);

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

	info_cmd = feh_printf(opt.info_cmd, FEH_FILE(w->file->data), w);

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
		putc('\n', stderr);
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
				putc(' ', stderr);
		}

		if (!(i % 50)) {
			int len = gib_list_length(filelist);

			fprintf(stderr, " %5d/%d (%d)\n[%3d%%] ",
					i, init_len, len, ((int) ((float) i / init_len * 100)));

		} else if ((!(i % 10)) && (!reset_output))
			putc(' ', stderr);

		reset_output = 0;
	} else
		fputs("[  0%] ", stderr);

	fprintf(stderr, "%c", stat);
	fflush(stderr);
	i++;
	return;
}

void feh_edit_inplace(winwidget w, int op)
{
	int tmp;
	Imlib_Image old = NULL;
	Imlib_Load_Error err = IMLIB_LOAD_ERROR_NONE;
	if (!w->file || !w->file->data || !FEH_FILE(w->file->data)->filename)
		return;

	if (!opt.edit) {
		imlib_context_set_image(w->im);
		if (op == INPLACE_EDIT_FLIP)
			imlib_image_flip_vertical();
		else if (op == INPLACE_EDIT_MIRROR)
			imlib_image_flip_horizontal();
		else {
			imlib_image_orientate(op);
			if(op != 2) {
				tmp = w->im_w;
				w->im_w = w->im_h;
				w->im_h = tmp;
			}
			if (FEH_FILE(w->file->data)->info) {
				FEH_FILE(w->file->data)->info->width = w->im_w;
				FEH_FILE(w->file->data)->info->height = w->im_h;
			}
		}
		winwidget_render_image(w, 1, 0);
		return;
	}

	// Imlib2 <= 1.5 returns "jpeg", Imlib2 >= 1.6 uses "jpg"
	if ((!strcmp(gib_imlib_image_format(w->im), "jpeg")
				|| !strcmp(gib_imlib_image_format(w->im), "jpg"))
			&& !path_is_url(FEH_FILE(w->file->data)->filename)) {
		feh_edit_inplace_lossless(w, op);
		feh_reload_image(w, 1, 1);
		return;
	}

	old = imlib_load_image_with_error_return(FEH_FILE(w->file->data)->filename, &err);

	if ((old != NULL) && (err == IMLIB_LOAD_ERROR_NONE)) {
		imlib_context_set_image(old);
		if (op == INPLACE_EDIT_FLIP)
			imlib_image_flip_vertical();
		else if (op == INPLACE_EDIT_MIRROR)
			imlib_image_flip_horizontal();
		else
			imlib_image_orientate(op);
		gib_imlib_save_image_with_error_return(old,
			FEH_FILE(w->file->data)->filename, &err);
		gib_imlib_free_image(old);
		if (err)
			feh_print_load_error(FEH_FILE(w->file->data)->filename,
				w, err, LOAD_ERROR_IMLIB);
		feh_reload_image(w, 1, 1);
	} else {
		/*
		 * Image was opened using curl/magick or has been deleted after
		 * opening it
		 */
		imlib_context_set_image(w->im);
		if (op == INPLACE_EDIT_FLIP)
			imlib_image_flip_vertical();
		else if (op == INPLACE_EDIT_MIRROR)
			imlib_image_flip_horizontal();
		else {
			imlib_image_orientate(op);
			tmp = w->im_w;
			w->im_w = w->im_h;
			w->im_h = tmp;
			if (FEH_FILE(w->file->data)->info) {
				FEH_FILE(w->file->data)->info->width = w->im_w;
				FEH_FILE(w->file->data)->info->height = w->im_h;
			}
		}
		im_weprintf(w, "unable to edit in place. Changes have not been saved.");
		winwidget_render_image(w, 1, 0);
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
	int devnull = -1;
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
		free(file_str);
		return;
	}
	else if (pid == 0) {

		execlp("jpegtran", "jpegtran", "-copy", "all", op_op, op_value,
				"-outfile", file_str, file_str, NULL);

		weprintf("lossless %s: Is 'jpegtran' installed? Failed to exec:", op_name);
		_exit(1);
	}
	else {
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
	if ((pid = fork()) < 0) {
		im_weprintf(w, "lossless %s: fork failed while updating EXIF tags:", op_name);
	}
	else if (pid == 0) {

		/* discard normal output */
		devnull = open("/dev/null", O_WRONLY);
		dup2(devnull, 1);

		execlp("jpegexiforient", "jpegexiforient", "-1", file_str, NULL);
		weprintf("lossless %s: Failed to exec jpegexiforient:", op_name);
		_exit(1);
	}
	else {
		waitpid(pid, &status, 0);

		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
			im_weprintf(w,
					"lossless %s: Failed to update EXIF orientation tag:"
					" jpegexiforient returned %d",
					op_name, status >> 8);
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
	char index[3];
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
			line = emalloc(strlen(opt.action_titles[i]) + 5);
			strcpy(line, "0: ");
			line = strcat(line, opt.action_titles[i]);
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
		if (opt.action_titles[i]) {
			cur_action++;
			line = emalloc(strlen(opt.action_titles[i]) + 5);
			sprintf(index, "%d", i);
			strcpy(line, index);
			strcat(line, ": ");
			strcat(line, opt.action_titles[i]);

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
