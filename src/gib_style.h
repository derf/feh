/* gib_style.h

Copyright (C) 1999,2000 Tom Gilbert.

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


#ifndef GIB_STYLE_H
#define GIB_STYLE_H

#include "gib_list.h"

#define GIB_STYLE(O) ((gib_style *)O)
#define GIB_STYLE_BIT(O) ((gib_style_bit *)O)

typedef struct __gib_style_bit gib_style_bit;
typedef struct __gib_style gib_style;

struct __gib_style_bit
{
   int x_offset, y_offset;
   int r,g,b,a;
};

struct __gib_style
{
   gib_list *bits;
   char *name;
};

#ifdef __cplusplus
extern "C"
{
#endif

gib_style_bit *gib_style_bit_new(int x_offset, int y_offset, int r, int g, int b, int a);
gib_style *gib_style_new(char *name);
void gib_style_bit_free(gib_style_bit *s);
void gib_style_free(gib_style *s);
/*
gib_style *gib_style_dup(gib_style *s);
void gib_dup_style_bit(void **dest, void *data);
*/

#ifdef __cplusplus
}
#endif


#endif
