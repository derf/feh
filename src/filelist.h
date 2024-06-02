/* filelist.h

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

#ifndef FILELIST_H
#define FILELIST_H

#ifdef HAVE_LIBEXIF
#include <libexif/exif-data.h>
#endif

struct __feh_file {
	char *filename;
	char *caption;
	char *name;

	/* info stuff */
	time_t mtime;
	int size;
	feh_file_info *info;	/* only set when needed */
#ifdef HAVE_LIBEXIF
	ExifData *ed;
#endif
};

struct __feh_file_info {
	int width;
	int height;
	int pixels;
	unsigned char has_alpha;
	char *format;
	char *extension;
};

#define FEH_FILE(l) ((feh_file *) l)

/*
 * PATH_MAX may not be defined on all systems. Since we only use it in for a
 * getcwd call in feh_absolute_path, it isn't really worth the effort to malloc
 * ever-increasing buffers until it fits. Instead, we just set it to 4096 and
 * have --filelist and --bg-* hiccup if the path is larger.
 */
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

enum filelist_recurse { FILELIST_FIRST, FILELIST_CONTINUE, FILELIST_LAST };

enum sort_type {
	SORT_NONE,
	SORT_NAME,
	SORT_FILENAME,
	SORT_DIRNAME,
	SORT_SIZE, // everything after SORT_SIZE requires stat(2) information on the filelist
	SORT_MTIME,
	SORT_WIDTH, // everything after SORT_WIDTH requires preloading the images in the filelist
	SORT_HEIGHT,
	SORT_PIXELS,
	SORT_FORMAT
};

feh_file *feh_file_new(char *filename);
void feh_file_free(feh_file * file);
feh_file_info *feh_file_info_new(void);
void feh_file_info_free(feh_file_info * info);
gib_list *feh_file_rm_and_free(gib_list * list, gib_list * file);
int file_selector_all(const struct dirent *unused);
void add_file_to_filelist_recursively(char *origpath, unsigned char level);
void add_file_to_rm_filelist(char *file);
void delete_rm_files(void);
gib_list *feh_file_info_preload(gib_list * list, int load_images);
int feh_file_stat(feh_file * file);
int feh_file_info_load(feh_file * file, Imlib_Image im);
void feh_file_dirname(char *dst, feh_file * f, int maxlen);
void feh_prepare_filelist(void);
int feh_write_filelist(gib_list * list, char *filename);
gib_list *feh_read_filelist(char *filename);
char *feh_absolute_path(char *path);
gib_list *feh_file_remove_from_list(gib_list * list, gib_list * l);
void feh_save_filelist(void);
char *feh_http_unescape(char * url);

int feh_cmp_name(void *file1, void *file2);
int feh_cmp_dirname(void *file1, void *file2);
int feh_cmp_filename(void *file1, void *file2);
int feh_cmp_mtime(void *file1, void *file2);
int feh_cmp_width(void *file1, void *file2);
int feh_cmp_height(void *file1, void *file2);
int feh_cmp_pixels(void *file1, void *file2);
int feh_cmp_size(void *file1, void *file2);
int feh_cmp_format(void *file1, void *file2);

extern gib_list *filelist;
extern gib_list *original_file_items;
extern int filelist_len;
extern gib_list *current_file;

#endif
