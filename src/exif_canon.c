/* exif_canon.c

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

#ifdef HAVE_LIBEXIF

#include <stdio.h>
#include <libexif/exif-data.h>

#include "feh.h"
#include "debug.h"
#include "exif.h"
#include "exif_canon.h"



/* get interesting canon maker note tags in readable form */
void exc_get_mnote_canon_tags(ExifData * ed, unsigned int tag,
			      char *buffer, unsigned int maxsize)
{
	/* char buf[EXIF_STD_BUF_LEN];

	   buf[0] = '\0';
	   exif_get_tag(ed, EXIF_IFD_EXIF, EXIF_TAG_FLASH, buf, sizeof(buf));
	   exif_trim_spaces(buf); */

	switch (tag) {
	default:
		{
			/* normal makernote tags without special treatment */
			exif_get_mnote_tag(ed, tag, buffer, maxsize);
		}
		break;
	}


	return;
}

#endif
