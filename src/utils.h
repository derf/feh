/* utils.h

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

#ifndef UTILS_H
#define UTILS_H

#ifndef __GNUC__
# define __attribute__(x)
#endif

void eprintf(char *fmt, ...) __attribute__ ((noreturn));
void weprintf(char *fmt, ...);
char *_estrdup(char *s);
void *_emalloc(size_t n);
void *_erealloc(void *ptr, size_t n);
char *estrjoin(const char *separator, ...);
char *stroflen(char, int);
char *feh_unique_filename(char *path, char *basename);
char *ereadfile(char *path);
char *feh_get_tmp_dir(void);
char *feh_get_user_name(void);

#define ESTRAPPEND(a,b) \
  {\
    char *____newstr;\
    if (!(a)) {\
      a = estrdup(b);\
    } else {\
      ____newstr = emalloc(strlen(a) + strlen(b) + 1);\
      strcpy(____newstr, (a));\
      strcat(____newstr, (b));\
      free(a);\
      (a) = ____newstr;\
    }\
  }

#define ESTRAPPEND_CHAR(a,b) \
  {\
    char *____newstr;\
    int ____len;\
    if (!(a)) {\
      (a) = emalloc(2);\
      (a)[0] = (b);\
      (a)[1] = '\0';\
    } else {\
      ____len = strlen((a));\
      ____newstr = emalloc(____len + 2);\
      strcpy(____newstr, (a));\
      ____newstr[____len] = (b);\
      ____newstr[____len+1] = '\0';\
      free(a);\
      (a) = ____newstr;\
    }\
  }
    
#define ESTRTRUNC(string,chars) \
  {\
    int ____len;\
    if (string) {\
      ____len = strlen(string);\
      if (____len >= (chars)) {\
        (string)[strlen(string) - chars] = '\0';\
      }\
    }\
  }


#endif
