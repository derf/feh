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

May 5, 2012 HRABAK conv'd gib_list stuff to feh_node stuff.

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
	feh_file_info *info;	/* only set when needed */
#ifdef HAVE_LIBEXIF
	ExifData *ed;
#endif
};

struct __feh_file_info {
	int width;
	int height;
	int size;
	int pixels;
	unsigned char has_alpha;
	char *format;
	char *extension;
};

#define FEH_FILE(l) ((feh_file *) l)

enum filelist_recurse { FILELIST_FIRST, FILELIST_CONTINUE, FILELIST_LAST };

enum sort_type { SORT_NONE, SORT_NAME, SORT_FILENAME, SORT_WIDTH,
                  SORT_HEIGHT,  SORT_PIXELS, SORT_SIZE, SORT_FORMAT };

enum image_remove { DELETE_NO = 0,  DELETE_YES };

LLMD * init_LLMD( void );
feh_file *feh_file_new(char *filename);
void feh_file_free(feh_file * file);
void feh_file_free_md( LLMD *md );
feh_file_info *feh_file_info_new(void);
void feh_file_info_free(feh_file_info * info);
void feh_file_rm_and_free( LLMD *md );
int file_selector_all(const struct dirent *unused);
void add_file_to_filelist_recursively(char *origpath, unsigned char level);
void feh_file_info_preload( LLMD *md );
int feh_file_info_load(feh_file * file, Imlib_Image im);
void feh_prepare_filelist( LLMD *md );
int feh_write_filelist( LLMD * md, char *filename);
void feh_read_filelist( LLMD *md , char *filename);
char *feh_absolute_path(char *path);
void feh_file_remove_from_list( LLMD *md );
void feh_save_filelist( void );

int feh_cmp_name(const void *file1, const void *file2);
int feh_cmp_filename(const void *file1, const void *file2);
int feh_cmp_width(const void *file1, const void *file2);
int feh_cmp_height(const void *file1, const void *file2);
int feh_cmp_pixels(const void *file1, const void *file2);
int feh_cmp_size(const void *file1, const void *file2);
int feh_cmp_format(const void *file1, const void *file2);

/* encapsulates the old delete_rm_files() here too by adding the delete_flag */
void add_file_to_rm_filelist( LLMD *md, char *file, int delete_flag );

/* these globals have been replaced with the metaData containers
 *extern gib_list *filelist;
 *extern gib_list *original_file_items;
 *extern int filelist_len;
 *extern gib_list *current_file;
*/

/* md stands for metaData.  ofi stands for original_file_items */
extern LLMD *feh_md, *ofi_md;

/*
 ******************************************************************
 * May 5, 2012 HRABAK combined list.c into this filelist.c module *
 ******************************************************************
 */

void init_list_mode(void);
void real_loadables_mode(int loadable);

#endif
