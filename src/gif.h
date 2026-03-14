/* gif.h

Animated GIF support for feh.

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

#ifndef GIF_H
#define GIF_H

#ifdef HAVE_LIBGIF

#include <Imlib2.h>
#include "structs.h"

struct gif_anim {
	Imlib_Image *frames;
	int *delays;        /* delay per frame in centiseconds */
	int frame_count;
	int current_frame;
	int loop_count;     /* 0 = infinite, >0 = finite */
	int loops_done;
};

gif_anim *gif_load(const char *filename);
void gif_free(gif_anim *anim);
void gif_animation_start(winwidget w);
void gif_animation_stop(winwidget w);

#endif /* HAVE_LIBGIF */
#endif /* GIF_H */
