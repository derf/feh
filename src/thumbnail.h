/* thumbnail.h

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

#ifndef THIMBNAIL_H
#define THIMBNAIL_H

#include "feh.h"
#include "winwidget.h"

int feh_thumbnail_get_file_from_coords(LLMD *md, int x, int y);
void feh_thumbnail_mark_removed(feh_node * node, enum misc_flags delete_flag );
void feh_thumbnail_create_im_main( winwidget *w, LLMD *md );
void feh_index2outputfile(void );
int feh_thumbnail_get_thumbnail(Imlib_Image * image, feh_node * node, int * orig_w, int * orig_h);
int feh_thumbnail_generate(Imlib_Image * image, feh_data * data, char *thumb_file, char *uri, int * orig_w, int * orig_h);
int feh_thumbnail_get_generated(Imlib_Image * image, feh_data * data, char * thumb_file, int * orig_w, int * orig_h);
char *feh_thumbnail_get_name(char *uri);
char *feh_thumbnail_get_name_uri(char *name);
char *feh_thumbnail_get_name_md5(char *uri);
void feh_thumbnail_show_fullsize( LLMD *md , feh_node *node);
void feh_thumbnail_select(winwidget w, enum misc_flags jmp_code);
feh_data *feh_thumbnail_get_selected_file();

/* int feh_thumbnail_setup_thumbnail_dir(void); used to return status */
void feh_thumbnail_setup_thumbnail_dir(void);

#endif
