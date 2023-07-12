/* debug.h

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

#ifndef DEBUG_H
#define DEBUG_H

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#define emalloc(a) malloc(a)
#define estrdup(a) strdup(a)
#define erealloc(a,b) realloc(a,b)
#else
#define emalloc(a) _emalloc(a)
#define estrdup(a) _estrdup(a)
#define erealloc(a,b) _erealloc(a,b)
#endif

#ifdef DEBUG
#ifdef __GNUC__
#define D(a) \
{ \
	if (opt.debug) { \
		printf("%-12s +%-4u %-20s : ",__FILE__,__LINE__,__FUNCTION__); \
		printf a; \
		fflush(stdout); \
	} \
  }
#else					/* __GNUC__ */
#define D(a) \
{ \
	if (opt.debug) { \
		printf("%-12s +%-4u : ",__FILE__,__LINE__); \
		printf a; \
		fflush(stdout); \
	} \
}
#endif					/* __GNUC__ */
#else					/* DEBUG */
#define D(a)
#endif					/* DEBUG */

#endif					/* DEBUG_H */
