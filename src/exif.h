/* exif.h

Copyright (C) 2012      Dennis Real.

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

#ifndef EXIF_H
#define EXIF_H

#include <libexif/exif-data.h>

#define EXIF_MAX_DATA 1024
#define EXIF_STD_BUF_LEN 128

extern void exif_trim_spaces(char *str);
extern void exif_get_tag(ExifData * d, ExifIfd ifd, ExifTag tag,
			 char *buffer, unsigned int maxsize);
extern void exif_get_tag_content(ExifData * d, ExifIfd ifd, ExifTag tag,
				 char *buffer, unsigned int maxsize);
extern void exif_get_mnote_tag(ExifData * d, unsigned int tag,
			       char *buffer, unsigned int maxsize);
extern void exif_get_gps_coords(ExifData * ed, char *buffer,
				unsigned int maxsize);
extern ExifData *exif_get_data(char *path);
extern void exif_get_info(ExifData * ed, char *buffer,
			  unsigned int maxsize);

#endif
