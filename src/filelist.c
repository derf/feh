/* filelist.c

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
#include "options.h"

#ifdef HAVE_LIBCURL
#include <curl/curl.h>
#endif

gib_list *filelist = NULL;
gib_list *original_file_items = NULL; /* original file items from argv */
int filelist_len = 0;
gib_list *current_file = NULL;

static gib_list *rm_filelist = NULL;

feh_file *feh_file_new(char *filename)
{
	feh_file *newfile;
	char *s;

	newfile = (feh_file *) emalloc(sizeof(feh_file));
	newfile->caption = NULL;
	newfile->filename = estrdup(filename);
	s = strrchr(filename, '/');
	if (s)
		newfile->name = estrdup(s + 1);
	else
		newfile->name = estrdup(filename);
	newfile->size = -1;
	newfile->mtime = 0;
	newfile->info = NULL;
#ifdef HAVE_LIBEXIF
	newfile->ed = NULL;
#endif
	return(newfile);
}

void feh_file_free(feh_file * file)
{
	if (!file)
		return;
	if (file->filename)
		free(file->filename);
	if (file->name)
		free(file->name);
	if (file->caption)
		free(file->caption);
	if (file->info)
		feh_file_info_free(file->info);
#ifdef HAVE_LIBEXIF
	if (file->ed)
		exif_data_unref(file->ed);
#endif
	free(file);
	return;
}

feh_file_info *feh_file_info_new(void)
{
	feh_file_info *info;


	info = (feh_file_info *) emalloc(sizeof(feh_file_info));

	info->width = 0;
	info->height = 0;
	info->pixels = 0;
	info->has_alpha = 0;
	info->format = NULL;
	info->extension = NULL;

	return(info);
}

void feh_file_info_free(feh_file_info * info)
{
	if (!info)
		return;
	if (info->format)
		free(info->format);
	if (info->extension)
		free(info->extension);
	free(info);
	return;
}

gib_list *feh_file_rm_and_free(gib_list * list, gib_list * l)
{
	unlink(FEH_FILE(l->data)->filename);
	return(feh_file_remove_from_list(list, l));
}

gib_list *feh_file_remove_from_list(gib_list * list, gib_list * l)
{
	feh_file_free(FEH_FILE(l->data));
	D(("filelist_len %d -> %d\n", filelist_len, filelist_len - 1));
	filelist_len--;
	return(gib_list_remove(list, l));
}

int file_selector_all(const struct dirent *unused __attribute__((unused)))
{
  return 1;
}

static void feh_print_stat_error(char *path)
{
	if (opt.quiet)
		return;

	switch (errno) {
	case ENOENT:
	case ENOTDIR:
		weprintf("%s does not exist - skipping", path);
		break;
	case ELOOP:
		weprintf("%s - too many levels of symbolic links - skipping", path);
		break;
	case EACCES:
		weprintf("you don't have permission to open %s - skipping", path);
		break;
	case EOVERFLOW:
		weprintf("Cannot open %s - EOVERFLOW.\n"
			"Recompile with stat64=1 to fix this", path);
		break;
	default:
		weprintf("couldn't open %s", path);
		break;
	}
}

static void add_stdin_to_filelist(void)
{
	char buf[1024];
	size_t readsize;
	char *sfn = estrjoin("_", "/tmp/feh_stdin", "XXXXXX", NULL);
	int fd = mkstemp(sfn);
	FILE *outfile;

	if (fd == -1) {
		free(sfn);
		weprintf("cannot read from stdin: mktemp:");
		return;
	}

	outfile = fdopen(fd, "w");

	if (outfile == NULL) {
		free(sfn);
		weprintf("cannot read from stdin: fdopen:");
		return;
	}

	while ((readsize = fread(buf, sizeof(char), sizeof(buf), stdin)) > 0) {
		if (fwrite(buf, sizeof(char), readsize, outfile) < readsize) {
			free(sfn);
			fclose(outfile);
			return;
		}
	}
	fclose(outfile);

	filelist = gib_list_add_front(filelist, feh_file_new(sfn));
	add_file_to_rm_filelist(sfn);
	free(sfn);
}


/* Recursive */
void add_file_to_filelist_recursively(char *origpath, unsigned char level)
{
	struct stat st;
	char *path;

	if (!origpath || *origpath == '\0')
		return;

	path = estrdup(origpath);
	D(("file is %s\n", path));

	if (level == FILELIST_FIRST) {
		/* First time through, sort out pathname */
		int len = 0;

		len = strlen(path);
		if (path[len - 1] == '/')
			path[len - 1] = '\0';

		if (path_is_url(path)) {
			D(("Adding url %s to filelist\n", path));
			filelist = gib_list_add_front(filelist, feh_file_new(path));
			/* We'll download it later... */
			free(path);
			return;
		} else if ((len == 1) && (path[0] == '-')) {
			D(("Adding temporary file for stdin (-) to filelist\n"));
			add_stdin_to_filelist();
			free(path);
			return;
		} else if (opt.filelistfile) {
			char *newpath = feh_absolute_path(path);

			free(path);
			path = newpath;
		}
	}

	errno = 0;
	if (stat(path, &st)) {
		feh_print_stat_error(path);
		free(path);
		return;
	}

	if ((S_ISDIR(st.st_mode)) && (level != FILELIST_LAST)) {
		struct dirent **de;
		DIR *dir;
		int cnt, n;

		D(("It is a directory\n"));

		if ((dir = opendir(path)) == NULL) {
			if (!opt.quiet)
				weprintf("couldn't open directory %s:", path);
			free(path);
			return;
		}
		n = scandir(path, &de, file_selector_all, alphasort);
		if (n < 0) {
			switch (errno) {
			case ENOMEM:
				weprintf("Insufficient memory to scan directory %s:", path);
				break;
			default:
				weprintf("Failed to scan directory %s:", path);
			}
		} else {
			for (cnt = 0; cnt < n; cnt++) {
				if (strcmp(de[cnt]->d_name, ".")
						&& strcmp(de[cnt]->d_name, "..")) {
					char *newfile;

					newfile = estrjoin("", path, "/", de[cnt]->d_name, NULL);

					/* This ensures we go down one level even if not fully recursive
					   - this way "feh some_dir" expands to some_dir's contents */
					if (opt.recursive)
						add_file_to_filelist_recursively(newfile, FILELIST_CONTINUE);
					else
						add_file_to_filelist_recursively(newfile, FILELIST_LAST);

					free(newfile);
				}
				free(de[cnt]);
			}
			free(de);
		}
		closedir(dir);
	} else if (S_ISREG(st.st_mode)) {
		D(("Adding regular file %s to filelist\n", path));
		filelist = gib_list_add_front(filelist, feh_file_new(path));
	}
	free(path);
	return;
}

void add_file_to_rm_filelist(char *file)
{
	rm_filelist = gib_list_add_front(rm_filelist, feh_file_new(file));
	return;
}

void delete_rm_files(void)
{
	gib_list *l;

	for (l = rm_filelist; l; l = l->next)
		unlink(FEH_FILE(l->data)->filename);
	return;
}

gib_list *feh_file_info_preload(gib_list * list, int load_images)
{
	gib_list *l;
	feh_file *file = NULL;
	gib_list *remove_list = NULL;

	for (l = list; l; l = l->next) {
		file = FEH_FILE(l->data);
		D(("file %p, file->next %p, file->name %s\n", l, l->next, file->name));
		if (load_images) {
			if (feh_file_info_load(file, NULL)) {
				D(("Failed to load file %p\n", file));
				remove_list = gib_list_add_front(remove_list, l);
				if (opt.verbose)
					feh_display_status('x');
			} else if (((unsigned int)file->info->width < opt.min_width)
					|| ((unsigned int)file->info->width > opt.max_width)
					|| ((unsigned int)file->info->height < opt.min_height)
					|| ((unsigned int)file->info->height > opt.max_height)) {
				remove_list = gib_list_add_front(remove_list, l);
				if (opt.verbose)
					feh_display_status('s');
			} else if (opt.verbose)
				feh_display_status('.');
		} else {
			if (feh_file_stat(file)) {
				D(("Failed to stat file %p\n", file));
				remove_list = gib_list_add_front(remove_list, l);
			}
		}
		if (sig_exit) {
			feh_display_status(0);
			exit(sig_exit);
		}
	}
	if (opt.verbose)
		feh_display_status(0);

	if (remove_list) {
		for (l = remove_list; l; l = l->next) {
			feh_file_free(FEH_FILE(((gib_list *) l->data)->data));
			filelist = list = gib_list_remove(list, (gib_list *) l->data);
		}

		gib_list_free(remove_list);
	}

	return(list);
}

int feh_file_stat(feh_file * file)
{
	struct stat st;

	errno = 0;
	if (stat(file->filename, &st)) {
		feh_print_stat_error(file->filename);
		return(1);
	}

	file->mtime = st.st_mtime;

	file->size = st.st_size;

	return(0);
}

int feh_file_info_load(feh_file * file, Imlib_Image im)
{
	int need_free = 1;
	Imlib_Image im1;

	if (feh_file_stat(file))
		return(1);

	D(("im is %p\n", im));

	if (im)
		need_free = 0;

	if (im)
		im1 = im;
	else if (!feh_load_image(&im1, file) || !im1)
		return(1);

	file->info = feh_file_info_new();

	file->info->width = gib_imlib_image_get_width(im1);
	file->info->height = gib_imlib_image_get_height(im1);

	file->info->has_alpha = gib_imlib_image_has_alpha(im1);

	file->info->pixels = file->info->width * file->info->height;

	file->info->format = estrdup(gib_imlib_image_format(im1));

	if (need_free)
		gib_imlib_free_image_and_decache(im1);
	return(0);
}

void feh_file_dirname(char *dst, feh_file * f, int maxlen)
{
	int n = strlen(f->filename) - strlen(f->name);

	/* Give up on long dirnames */
	if (n <= 0 || n >= maxlen) {
		dst[0] = '\0';
		return;
	}

	memcpy(dst, f->filename, n);
	dst[n] = '\0';
}

static inline int strcmp_or_strverscmp(const char *s1, const char *s2)
{
	if (!opt.version_sort)
		return(strcmp(s1, s2));
	else
		return(strverscmp(s1, s2));
}

int feh_cmp_filename(void *file1, void *file2)
{
	return(strcmp_or_strverscmp(FEH_FILE(file1)->filename, FEH_FILE(file2)->filename));
}

int feh_cmp_name(void *file1, void *file2)
{
	return(strcmp_or_strverscmp(FEH_FILE(file1)->name, FEH_FILE(file2)->name));
}

int feh_cmp_dirname(void *file1, void *file2)
{
	char dir1[PATH_MAX], dir2[PATH_MAX];
	int cmp;
	feh_file_dirname(dir1, FEH_FILE(file1), PATH_MAX);
	feh_file_dirname(dir2, FEH_FILE(file2), PATH_MAX);
	if ((cmp = strcmp_or_strverscmp(dir1, dir2)) != 0)
		return(cmp);
	return(feh_cmp_name(file1, file2));
}

/* Return -1 if file1 is _newer_ than file2 */
int feh_cmp_mtime(void *file1, void *file2)
{
	/* gib_list_sort is not stable, so explicitly return 0 as -1 */
	return(FEH_FILE(file1)->mtime >= FEH_FILE(file2)->mtime ? -1 : 1);
}

int feh_cmp_width(void *file1, void *file2)
{
	return((FEH_FILE(file1)->info->width - FEH_FILE(file2)->info->width));
}

int feh_cmp_height(void *file1, void *file2)
{
	return((FEH_FILE(file1)->info->height - FEH_FILE(file2)->info->height));
}

int feh_cmp_pixels(void *file1, void *file2)
{
	return((FEH_FILE(file1)->info->pixels - FEH_FILE(file2)->info->pixels));
}

int feh_cmp_size(void *file1, void *file2)
{
	return((FEH_FILE(file1)->size - FEH_FILE(file2)->size));
}

int feh_cmp_format(void *file1, void *file2)
{
	return(strcmp(FEH_FILE(file1)->info->format, FEH_FILE(file2)->info->format));
}

void feh_prepare_filelist(void)
{
	/*
	 * list and customlist mode as well as the somewhat more fancy sort modes
	 * need access to file infos. Preloading them is also useful for
	 * list/customlist as --min-dimension/--max-dimension may filter images
	 * which should not be processed.
	 * Finally, if --min-dimension/--max-dimension (-> opt.filter_by_dimensions)
	 * is set and we're in thumbnail mode, we need to filter images first so
	 * we can create a properly sized thumbnail list.
	 */
	if (opt.list || opt.preload || opt.customlist || (opt.sort >= SORT_WIDTH)
			|| (opt.filter_by_dimensions && (opt.index || opt.thumbs || opt.bgmode))) {
		/* For these sort options, we have to preload images */
		filelist = feh_file_info_preload(filelist, TRUE);
		if (!gib_list_length(filelist))
			show_mini_usage();
	} else if (opt.sort >= SORT_SIZE) {
		/* For these sort options, we need stat(2) information on the files,
		 * but there is no need to load the images. */
		filelist = feh_file_info_preload(filelist, FALSE);
		if (!gib_list_length(filelist))
			show_mini_usage();
	}

	D(("sort mode requested is: %d\n", opt.sort));
	switch (opt.sort) {
	case SORT_NONE:
		if (opt.randomize) {
			/* Randomize the filename order */
			filelist = gib_list_randomize(filelist);
		} else if (!opt.reverse) {
			/* Let's reverse the list. Its back-to-front right now ;) */
			filelist = gib_list_reverse(filelist);
		}
		break;
	case SORT_NAME:
		filelist = gib_list_sort(filelist, feh_cmp_name);
		break;
	case SORT_FILENAME:
		filelist = gib_list_sort(filelist, feh_cmp_filename);
		break;
	case SORT_DIRNAME:
		filelist = gib_list_sort(filelist, feh_cmp_dirname);
		break;
	case SORT_MTIME:
		filelist = gib_list_sort(filelist, feh_cmp_mtime);
		break;
	case SORT_WIDTH:
		filelist = gib_list_sort(filelist, feh_cmp_width);
		break;
	case SORT_HEIGHT:
		filelist = gib_list_sort(filelist, feh_cmp_height);
		break;
	case SORT_PIXELS:
		filelist = gib_list_sort(filelist, feh_cmp_pixels);
		break;
	case SORT_SIZE:
		filelist = gib_list_sort(filelist, feh_cmp_size);
		break;
	case SORT_FORMAT:
		filelist = gib_list_sort(filelist, feh_cmp_format);
		break;
	default:
		break;
	}

	/* no point reversing a random list */
	if (opt.reverse && (opt.sort != SORT_NONE)) {
		D(("Reversing filelist as requested\n"));
		filelist = gib_list_reverse(filelist);
	}

	return;
}

int feh_write_filelist(gib_list * list, char *filename)
{
	FILE *fp;
	gib_list *l;

	if (!list || !filename || !strcmp(filename, "/dev/stdin"))
		return(0);

	errno = 0;
	if ((fp = fopen(filename, "w")) == NULL) {
		weprintf("can't write filelist %s:", filename);
		return(0);
	}

	for (l = list; l; l = l->next)
		fprintf(fp, "%s\n", (FEH_FILE(l->data)->filename));

	fclose(fp);

	return(1);
}

gib_list *feh_read_filelist(char *filename)
{
	FILE *fp;
	gib_list *list = NULL;
	char s[1024], s1[1024];
	Imlib_Load_Error err = IMLIB_LOAD_ERROR_NONE;
	Imlib_Image tmp_im;
	struct stat st;
	signed short tmp_conversion_timeout;

	if (!filename)
		return(NULL);

	/*
	 * feh_load_image will fail horribly if filename is not seekable
	 */
	tmp_conversion_timeout = opt.conversion_timeout;
	opt.conversion_timeout = -1;
	if (!stat(filename, &st) && S_ISREG(st.st_mode)) {
		tmp_im = imlib_load_image_with_error_return(filename, &err);
		if (err == IMLIB_LOAD_ERROR_NONE) {
			gib_imlib_free_image_and_decache(tmp_im);
			weprintf("Filelist file %s is an image, refusing to use it.\n"
				"Did you mix up -f and -F?", filename);
			opt.filelistfile = NULL;
			return NULL;
		}
	}
	opt.conversion_timeout = tmp_conversion_timeout;

	errno = 0;

	if (!strcmp(filename, "/dev/stdin"))
		fp = stdin;
	else
		fp = fopen(filename, "r");

	if (fp == NULL) {
		/* return quietly, as it's okay to specify a filelist file that doesn't
		   exist. In that case we create it on exit. */
		return(NULL);
	}

	for (; fgets(s, sizeof(s), fp);) {
		D(("Got line '%s'\n", s));
		s1[0] = '\0';
		sscanf(s, "%[^\n]", (char *) &s1);
		if (!(*s1) || (*s1 == '\n'))
			continue;
		D(("Got filename %s from filelist file\n", s1));
		/* Add it to the new list */
		list = gib_list_add_front(list, feh_file_new(s1));
	}
	if (strcmp(filename, "/dev/stdin"))
		fclose(fp);

	return(list);
}

char *feh_absolute_path(char *path)
{
	char cwd[PATH_MAX];
	char fullpath[PATH_MAX];
	char temp[PATH_MAX];
	char *ret;

	if (!path)
		return(NULL);
	if (path[0] == '/' || path_is_url(path))
		return(estrdup(path));
	/* This path is not relative. We're gonna convert it, so that a
	   filelist file can be saved anywhere and feh will still find the
	   images */
	D(("Need to convert %s to an absolute form\n", path));
	/* I SHOULD be able to just use a simple realpath() here, but dumb *
	   old Solaris's realpath doesn't return an absolute path if the
	   path you give it is relative. Linux and BSD get this right... */
	if (getcwd(cwd, sizeof(cwd)) == NULL)
		eprintf("Cannot determine working directory:");
	if ((size_t) snprintf(temp, sizeof(temp), "%s/%s", cwd, path) >= sizeof(temp))
		eprintf("Absolute path for working directory was truncated");
	if (realpath(temp, fullpath) != NULL) {
		ret = estrdup(fullpath);
	} else {
		ret = estrdup(temp);
	}
	D(("Converted path to %s\n", ret));
	return(ret);
}

void feh_save_filelist(void)
{
	char *tmpname;
	char *base_dir = "";

	if (opt.output_dir) {
		base_dir = estrjoin("", opt.output_dir, "/", NULL);
	}

	tmpname = feh_unique_filename(base_dir, "filelist");

	if (opt.output_dir) {
		free(base_dir);
	}

	if (opt.verbose)
		fprintf(stderr, "saving filelist to filename '%s'\n", tmpname);

	feh_write_filelist(filelist, tmpname);
	free(tmpname);
	return;
}

#ifdef HAVE_LIBCURL

char *feh_http_unescape(char *url)
{
	CURL *curl = curl_easy_init();
	if (!curl) {
		return NULL;
	}
	char *tmp_url = curl_easy_unescape(curl, url, 0, NULL);
	char *new_url = estrdup(tmp_url);
	curl_free(tmp_url);
	curl_easy_cleanup(curl);
	return new_url;
}

#else

char *feh_http_unescape(char *url)
{
	return NULL;
}

#endif
