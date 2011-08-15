/* filelist.c

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
#include "options.h"

gib_list *filelist = NULL;
gib_list *original_file_items = NULL; // original file items from argv
int filelist_len = 0;
gib_list *current_file = NULL;
extern int errno;

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
	newfile->info = NULL;
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
	free(file);
	return;
}

feh_file_info *feh_file_info_new(void)
{
	feh_file_info *info;


	info = (feh_file_info *) emalloc(sizeof(feh_file_info));

	info->width = 0;
	info->height = 0;
	info->size = 0;
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

/* Recursive */
void add_file_to_filelist_recursively(char *origpath, unsigned char level)
{
	struct stat st;
	char *path;

	if (!origpath)
		return;

	path = estrdup(origpath);
	D(("file is %s\n", path));

	if (level == FILELIST_FIRST) {
		/* First time through, sort out pathname */
		int len = 0;

		len = strlen(path);
		if (path[len - 1] == '/')
			path[len - 1] = '\0';

		if ((!strncmp(path, "http://", 7))
				|| (!strncmp(path, "https://", 8))
				|| (!strncmp(path, "ftp://", 6))) {
			/* Its a url */
			D(("Adding url %s to filelist\n", path));
			filelist = gib_list_add_front(filelist, feh_file_new(path));
			/* We'll download it later... */
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
		/* Display useful error message */
		switch (errno) {
		case ENOENT:
		case ENOTDIR:
			if (!opt.quiet)
				weprintf("%s does not exist - skipping", path);
			break;
		case ELOOP:
			if (!opt.quiet)
				weprintf("%s - too many levels of symbolic links - skipping", path);
			break;
		case EACCES:
			if (!opt.quiet)
				weprintf("you don't have permission to open %s - skipping", path);
			break;
		default:
			if (!opt.quiet)
				weprintf("couldn't open %s", path);
			break;
		}
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
		    if (!opt.quiet)
		      weprintf("Insufficient memory to scan directory %s:", path);
		    break;
		  default:
		    if (!opt.quiet)
		      weprintf("Failed to scan directory %s:", path);
		  }
		}

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

gib_list *feh_file_info_preload(gib_list * list)
{
	gib_list *l;
	feh_file *file = NULL;
	gib_list *remove_list = NULL;

	if (opt.verbose)
		fputs(PACKAGE " - preloading...\n", stdout);

	for (l = list; l; l = l->next) {
		file = FEH_FILE(l->data);
		D(("file %p, file->next %p, file->name %s\n", l, l->next, file->name));
		if (feh_file_info_load(file, NULL)) {
			D(("Failed to load file %p\n", file));
			remove_list = gib_list_add_front(remove_list, l);
			if (opt.verbose)
				feh_display_status('x');
		} else if (opt.verbose)
			feh_display_status('.');
	}
	if (opt.verbose)
		fputs("\n", stdout);

	if (remove_list) {
		for (l = remove_list; l; l = l->next)
			filelist = list = gib_list_remove(list, (gib_list *) l->data);

		gib_list_free(remove_list);
	}

	return(list);
}

int feh_file_info_load(feh_file * file, Imlib_Image im)
{
	struct stat st;
	int need_free = 1;
	Imlib_Image im1;

	D(("im is %p\n", im));

	if (im)
		need_free = 0;

	errno = 0;
	if (stat(file->filename, &st)) {
		/* Display useful error message */
		switch (errno) {
		case ENOENT:
		case ENOTDIR:
			if (!opt.quiet)
				weprintf("%s does not exist - skipping", file->filename);
			break;
		case ELOOP:
			if (!opt.quiet)
				weprintf("%s - too many levels of symbolic links - skipping", file->filename);
			break;
		case EACCES:
			if (!opt.quiet)
				weprintf("you don't have permission to open %s - skipping", file->filename);
			break;
		default:
			if (!opt.quiet)
				weprintf("couldn't open %s ", file->filename);
			break;
		}
		return(1);
	}

	if (im)
		im1 = im;
	else if (!feh_load_image(&im1, file))
		return(1);

	if (!im1)
		return(1);

	file->info = feh_file_info_new();

	file->info->width = gib_imlib_image_get_width(im1);
	file->info->height = gib_imlib_image_get_height(im1);

	file->info->has_alpha = gib_imlib_image_has_alpha(im1);

	file->info->pixels = file->info->width * file->info->height;

	file->info->format = estrdup(gib_imlib_image_format(im1));

	file->info->size = st.st_size;

	if (need_free && im1)
		gib_imlib_free_image_and_decache(im1);
	return(0);
}

int feh_cmp_filename(void *file1, void *file2)
{
	return(strcmp(FEH_FILE(file1)->filename, FEH_FILE(file2)->filename));
}

int feh_cmp_name(void *file1, void *file2)
{
	return(strcmp(FEH_FILE(file1)->name, FEH_FILE(file2)->name));
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
	return((FEH_FILE(file1)->info->size - FEH_FILE(file2)->info->size));
}

int feh_cmp_format(void *file1, void *file2)
{
	return(strcmp(FEH_FILE(file1)->info->format, FEH_FILE(file2)->info->format));
}

void feh_prepare_filelist(void)
{
	if (opt.list || opt.customlist || (opt.sort > SORT_FILENAME)
			|| opt.preload) {
		/* For these sort options, we have to preload images */
		filelist = feh_file_info_preload(filelist);
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

	if (!list || !filename)
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
	Imlib_Image im1;

	if (!filename)
		return(NULL);

	/* try and load the given filelist as an image, cowardly refuse to
	 * overwrite an image with a filelist. (requested by user who did feh -df *
	 * when he meant feh -dF *, as it overwrote the first image with the
	 * filelist).
	 */
	if (feh_load_image_char(&im1, filename)) {
		weprintf(
				"The file you specified as a filelist to read - %s - appears to be an image. Ignoring it (this is a common mistake).\n",
				filename);
		opt.filelistfile = NULL;
		return(NULL);
	}

	errno = 0;
	if ((fp = fopen(filename, "r")) == NULL) {
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
	if (path[0] == '/')
		return(estrdup(path));
	/* This path is not relative. We're gonna convert it, so that a
	   filelist file can be saved anywhere and feh will still find the
	   images */
	D(("Need to convert %s to an absolute form\n", path));
	/* I SHOULD be able to just use a simple realpath() here, but dumb * 
	   old Solaris's realpath doesn't return an absolute path if the
	   path you give it is relative. Linux and BSD get this right... */
	getcwd(cwd, sizeof(cwd));
	snprintf(temp, sizeof(temp), "%s/%s", cwd, path);
	if (realpath(temp, fullpath) != NULL) {
		ret = estrdup(fullpath);
	} else {
		ret = estrdup(temp);
	}
	D(("Converted path to %s\n", ret));
	return(ret);
}

void feh_save_filelist()
{
	char *tmpname;

	tmpname = feh_unique_filename("", "filelist");

	if (!opt.quiet)
		printf("saving filelist to filename '%s'\n", tmpname);

	feh_write_filelist(filelist, tmpname);
	free(tmpname);
	return;
}
