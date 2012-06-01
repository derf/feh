/* utils.h

Copyright (C) 1999-2003 Tom Gilbert.
Copyright (C) 2012      Christopher Hrabak (for bhs_hash, feh_string_split )

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
char *feh_unique_filename(char *path, char *basename);
char *ereadfile(char *path);

/* May 4, 2012 HRABAK combined from gib_list.c */

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



/* May 2012 HRABAK replaced all the gib_hash() stuff with bhs_hash() stuff.
 * bhs stands for BoneHeadSimple.
 */

enum _bhs_hash_flags { ADDIT_NO = 0, ADDIT_YES };

typedef union _bhs_hash bhs_node;

union _bhs_hash {
  struct {
      short cnt;              /* how many already in the hash array */
      short found;            /* if get() found then ==1, else 0 */
      short which;            /* if get() found key, then which a[i] was it? */
      short flag1;            /* open */
      int size;               /* max number of nodes avaliable to hold hash items */
    } h0;
  struct {
      char *key;
      char *data;
      int len;                /* length of the key string for strcmp() speedup */
    } h1;
};

bhs_node ** bhs_hash_init(  int size  );
int bhs_hash_get( bhs_node **a , char *key, char *data, int addit );
void bhs_hash_rpt( bhs_node **a , int addit, int ret_code );


/* May 2012 HRABAK rewrote the feh_string_split() to eliminate
 * gib_list linked list requirement.
 */

typedef struct _wd wd;

struct _wd {                    /* wd stands for word data */
    short int tot_items;        /* saves total num of words stored */
    short int linenum;          /* one-based */
    short int wordnum;          /* one-based */
    short int len;              /* one-based */
    char * word;                /* ptr to the start of the word */
};

#define FIRST_AWP_GUESS 20      /* initial size for the awp[] array */

wd * feh_string_split( char *s);
char * get_next_awp_line( wd awp[] , int linenum, int i_start, int *i_end );
void feh_style_new_from_ascii(char *file, feh_style *s);
int is_filelist(  char *filename );

typedef union _ld ld;

union _ld {
  struct {
    int tot_lines;              /* saves total num of lines in array */
    int maxwide;                /* max value of ld.wide for all lines */
    int tothigh;                /* summed height of all lines */
    } L0;
  struct {
    /* short int linenum; the index IS the linenum, so  one-based */
    short int wide;             /* font-sized width of the whole line */
    short int high;             /* font-sized height of the whole line */
    short int len;              /* one-based */
    char * line;                /* ptr to the start of the line */
    } L1;                       /* use "L" not "l" cause "l1" is confusing */
};

#define FIRST_ALP_GUESS 5       /* initial size of the alp[] array */

#endif     /* end of #ifndef UTILS_H */
