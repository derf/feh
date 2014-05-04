/* gib_list.h

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

#ifndef GIB_LIST_H
#define GIB_LIST_H

#include <stdarg.h>

#define GIB_LIST(a) ((gib_list*)a)

enum __direction
{ FORWARD, BACK };

typedef struct __gib_list gib_list;

struct __gib_list
{
   void *data;

   gib_list *next;
   gib_list *prev;
};

typedef int (gib_compare_fn) (void *data1, void *data2);

#ifdef __cplusplus
extern "C"
{
#endif

gib_list *gib_list_new(void);
void gib_list_free(gib_list * l);
gib_list *gib_list_add_front(gib_list * root, void *data);
gib_list *gib_list_add_end(gib_list * root, void *data);
/*
gib_list *gib_list_add_at_pos(gib_list * root, int pos, void *data);
gib_list *gib_list_pop_to_end(gib_list * root, gib_list * l);
*/
gib_list *gib_list_unlink(gib_list * root, gib_list * l);
gib_list *gib_list_cat(gib_list * root, gib_list * l);
int gib_list_length(gib_list * l);
gib_list *gib_list_last(gib_list * l);
gib_list *gib_list_first(gib_list * l);
gib_list *gib_list_jump(gib_list * root, gib_list * l, int direction,

                        int num);
gib_list *gib_list_reverse(gib_list * l);
gib_list *gib_list_randomize(gib_list * list);
int gib_list_num(gib_list * root, gib_list * l);
gib_list *gib_list_remove(gib_list * root, gib_list * l);
gib_list *gib_list_sort(gib_list * list, gib_compare_fn cmp);
gib_list *gib_list_sort_merge(gib_list * l1, gib_list * l2,

                              gib_compare_fn cmp);
/*
gib_list *gib_list_nth(gib_list * root, unsigned int num);
*/
unsigned char gib_list_has_more_than_one_item(gib_list * root);
void gib_list_free_and_data(gib_list * l);
/*
gib_list *gib_list_dup(gib_list * list);
gib_list *gib_list_dup_special(gib_list * list,
                               void (*cpy_func) (void **dest, void *data));
gib_list *gib_list_move_down_by_one(gib_list * root, gib_list * l);
gib_list *gib_list_move_up_by_one(gib_list * root, gib_list * l);
*/

gib_list *gib_list_foreach(gib_list *root, void (*fe_func)(gib_list *node, void *data), void *data);
gib_list *gib_list_find(gib_list *root, unsigned char (*find_func)(gib_list *node, void *data), void *data);
gib_list *gib_list_find_by_data(gib_list *root, void *data);

/* don't really belong here, will do for now */
gib_list *gib_string_split(const char *string, const char *delimiter);
/*
char *gib_strjoin(const char *separator, ...);
*/

#ifdef __cplusplus
}
#endif


#endif
