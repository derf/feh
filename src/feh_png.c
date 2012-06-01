/* feh_png.c

Copyright (C) 2004 Tom Gilbert.
Copyright (C) 2012      Christopher Hrabak  bhs_hash() stuff

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

#include "feh_png.h"

#include <png.h>

#include <stdio.h>
#include <stdarg.h>

#define FEH_PNG_COMPRESSION 3
#define FEH_PNG_NUM_COMMENTS 4

int feh_png_read_comments( char *file, bhs_node **aHash)
{

	FILE *fp;
	int i, sig_bytes, comments = 0;

	png_structp png_ptr;
	png_infop info_ptr;
	png_textp text_ptr;

	if (!(fp = fopen(file, "rb")))
		return -1;

	if (!(sig_bytes = feh_png_file_is_png(fp))) {
		fclose(fp);
		return -1;
	}

	/* initialize data structures */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		fclose(fp);
		return -1;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
		fclose(fp);
		return -1;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
		return -1;
	}

	/* initialize reading */
	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, sig_bytes);

	png_read_info(png_ptr, info_ptr);

  i = -1;        /* just for the return code incase no comments */
#ifdef PNG_TEXT_SUPPORTED
	png_get_text(png_ptr, info_ptr, &text_ptr, &comments);
	if (comments > 0) {
    /* Note:  The ADDIT_YES is on, but ONLY the three predefined keys will
     * have their ->data member updated.  All attempts to add keys will fail
     * cause we only created this  hash to hold 3 keys.
     */
		for (i = 0; i < comments; i++)
			bhs_hash_get( aHash, text_ptr[i].key, estrdup(text_ptr[i].text), ADDIT_YES );
	}
  i = 0;      /* sets the return code to success */
#endif				/* PNG_TEXT_SUPPORTED */

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(fp);

	return i;
}

/* grab image data from image and write info file with comments ... */
int feh_png_write_png(Imlib_Image image, char *file, ...)
{
	FILE *fp;
	int i, w, h;

	png_structp png_ptr;
	png_infop info_ptr;
	png_color_8 sig_bit;

	DATA32 *ptr;

#ifdef PNG_TEXT_SUPPORTED
	va_list args;
	png_text text[FEH_PNG_NUM_COMMENTS];
	char *pair_key, *pair_text;
#endif				/* PNG_TEXT_SUPPORTED */

	if (!(fp = fopen(file, "wb")))
		return 0;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		return 0;

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
		return 0;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		fclose(fp);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		png_destroy_info_struct(png_ptr, &info_ptr);
		return 0;
	}

	w = gib_imlib_image_get_width(image);
	h = gib_imlib_image_get_height(image);

	png_init_io(png_ptr, fp);

	png_set_IHDR(png_ptr, info_ptr, w, h, 8, PNG_COLOR_TYPE_RGB_ALPHA,
		     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

#ifdef WORDS_BIGENDIAN
	png_set_swap_alpha(png_ptr);
#else				/* !WORDS_BIGENDIAN */
	png_set_bgr(png_ptr);
#endif				/* WORDS_BIGENDIAN */

	sig_bit.red = 8;
	sig_bit.green = 8;
	sig_bit.blue = 8;
	sig_bit.alpha = 8;
	png_set_sBIT(png_ptr, info_ptr, &sig_bit);

#ifdef PNG_TEXT_SUPPORTED
	va_start(args, file);
	for (i = 0; i < FEH_PNG_NUM_COMMENTS; i++) {
		if ((pair_key = va_arg(args, char *))
		    && (pair_text = va_arg(args, char *))) {
			/* got a complete pair, add to info structure */
			text[i].key = pair_key;
			text[i].text = pair_text;
			text[i].compression = PNG_TEXT_COMPRESSION_NONE;
		} else
			break;
	}
	va_end(args);

	if (i > 0)
		png_set_text(png_ptr, info_ptr, text, i);
#endif				/* PNG_TEXT_SUPPORTED */

	png_set_compression_level(png_ptr, FEH_PNG_COMPRESSION);
	png_write_info(png_ptr, info_ptr);
	png_set_shift(png_ptr, &sig_bit);
	png_set_packing(png_ptr);

	/* write image data */
	imlib_context_set_image(image);
	ptr = imlib_image_get_data();
	for (i = 0; i < h; i++, ptr += w)
		png_write_row(png_ptr, (png_bytep) ptr);

	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	png_destroy_info_struct(png_ptr, &info_ptr);

	fclose(fp);

	return 0;
}

/* check PNG signature */
int feh_png_file_is_png(FILE * fp)
{
	unsigned char buf[8];

	if ( fread(buf, 1, 8, fp) > 0 ) {
      if (png_sig_cmp(buf, 0, 8)) {
        return 0;
      }
  }

	return 8;
}
