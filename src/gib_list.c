/* gib_list.c

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

#include <time.h>
#include "gib_list.h"
#include "utils.h"
#include "debug.h"

gib_list *
gib_list_new(void)
{
   gib_list *l;

   l = (gib_list *) emalloc(sizeof(gib_list));
   l->data = NULL;
   l->next = NULL;
   l->prev = NULL;
   return (l);
}

void
gib_list_free(gib_list * l)
{
   gib_list *ll;

   if (!l)
      return;

   while (l)
   {
      ll = l;
      l = l->next;
      free(ll);
   }

   return;
}

void
gib_list_free_and_data(gib_list * l)
{
   gib_list *ll;

   if (!l)
      return;

   while (l)
   {
      ll = l;
      l = l->next;
      free(ll->data);
      free(ll);
   }
   return;
}

#if 0
gib_list *
gib_list_dup(gib_list * list)
{
   gib_list *ret = NULL;

   if (list)
   {
      gib_list *last;

      ret = gib_list_new();
      ret->data = list->data;
      last = ret;
      list = list->next;
      while (list)
      {
         last->next = gib_list_new();
         last->next->prev = last;
         last = last->next;
         last->data = list->data;
         list = list->next;
      }
   }
   return (ret);
}

gib_list *
gib_list_dup_special(gib_list * list,
                     void (*cpy_func) (void **dest, void *data))
{
   gib_list *ret = NULL;

   if (list)
   {
      gib_list *last;

      ret = gib_list_new();
      cpy_func(&(ret->data), list->data);
      last = ret;
      list = list->next;
      while (list)
      {
         last->next = gib_list_new();
         last->next->prev = last;
         last = last->next;
         cpy_func(&(last->data), list->data);
         list = list->next;
      }
   }
   return (ret);
}
#endif

gib_list *
gib_list_add_front(gib_list * root, void *data)
{
   gib_list *l;

   l = gib_list_new();
   l->next = root;
   l->data = data;
   if (root)
      root->prev = l;
   return (l);
}

gib_list *
gib_list_add_end(gib_list * root, void *data)
{
   gib_list *l, *last;

   last = gib_list_last(root);
   l = gib_list_new();
   l->prev = last;
   l->data = data;
   if (last)
   {
      last->next = l;
      return (root);
   }
   else
   {
      return (l);
   }
}

#if 0
gib_list *
gib_list_add_at_pos(gib_list * root, int pos, void *data)
{
   gib_list *l, *top;

   if (pos == gib_list_length(root))
      root = gib_list_add_end(root, data);
   else if (pos == 0)
      root = gib_list_add_front(root, data);
   else
   {
      top = gib_list_nth(root, pos);

      if (!top)
         return (root);

      l = gib_list_new();
      l->next = top;
      l->prev = top->prev;
      l->data = data;
      if (top->prev)
         top->prev->next = l;

      top->prev = l;
   }
   return (root);
}

gib_list *
gib_list_move_up_by_one(gib_list * root, gib_list * l)
{
   if (l || l->prev)
      root = gib_list_move_down_by_one(root, l->prev);
   return (root);
}

gib_list *
gib_list_move_down_by_one(gib_list * root, gib_list * l)
{
   gib_list *temp;

   if (!l || !l->next)
      return (root);

   /* store item we link next to */
   temp = l->next;
   /* remove from list */
   root = gib_list_unlink(root, l);
   /* add back one before */
   l->next = temp->next;
   l->prev = temp;
   if (temp->next)
      temp->next->prev = l;
   temp->next = l;

   return (root);
}
#endif


unsigned char
gib_list_has_more_than_one_item(gib_list * root)
{
   if (root->next)
      return (1);
   else
      return (0);
}

#if 0
gib_list *
gib_list_pop_to_end(gib_list * root, gib_list * l)
{
   root = gib_list_unlink(root, l);
   root = gib_list_add_end(root, l->data);
   free(l);

   return (root);
}
#endif

gib_list *
gib_list_cat(gib_list * root, gib_list * l)
{
   gib_list *last;

   if (!l)
      return (root);
   if (!root)
      return (l);
   last = gib_list_last(root);
   last->next = l;
   l->prev = last;
   return (root);
}

int
gib_list_length(gib_list * l)
{
   int length;

   length = 0;
   while (l)
   {
      length++;
      l = l->next;
   }
   return (length);
}

gib_list *
gib_list_last(gib_list * l)
{
   if (l)
   {
      while (l->next)
         l = l->next;
   }
   return (l);
}

gib_list *
gib_list_first(gib_list * l)
{
   if (l)
   {
      while (l->prev)
         l = l->prev;
   }
   return (l);
}

gib_list *
gib_list_jump(gib_list * root, gib_list * l, int direction, int num)
{
   int i;
   gib_list *ret = NULL;

   if (!root)
      return (NULL);
   if (!l)
      return (root);

   ret = l;

   for (i = 0; i < num; i++)
   {
      if (direction == FORWARD)
      {
         if (ret->next)
            ret = ret->next;
         else
            ret = root;
      }
      else
      {
         if (ret->prev)
            ret = ret->prev;
         else
            ret = gib_list_last(ret);
      }
   }
   return (ret);
}

gib_list *
gib_list_reverse(gib_list * l)
{
   gib_list *last;

   last = NULL;
   while (l)
   {
      last = l;
      l = last->next;
      last->next = last->prev;
      last->prev = l;
   }
   return (last);
}

gib_list *
gib_list_randomize(gib_list * list)
{
   int len, r, i;
   gib_list **farray, *f, *t;

   if (!list)
      return (NULL);
   len = gib_list_length(list);
   if (len <= 1)
      return (list);
   farray = (gib_list **) emalloc(sizeof(gib_list *) * len);
   for (f = list, i = 0; f; f = f->next, i++)
   {
      farray[i] = f;
   }
   for (i = 0; i < len - 1; i++)
   {
      r = i + random() / (RAND_MAX / (len - i) + 1 );
      t = farray[r];
      farray[r] = farray[i];
      farray[i] = t;
   }
   list = farray[0];
   list->prev = NULL;
   list->next = farray[1];
   for (i = 1, f = farray[1]; i < len - 1; i++, f = f->next)
   {
      f->prev = farray[i - 1];
      f->next = farray[i + 1];
   }
   f->prev = farray[len - 2];
   f->next = NULL;
   free(farray);
   return (list);
}

int
gib_list_num(gib_list * root, gib_list * l)
{
   int i = 0;

   while (root)
   {
      if (root == l)
         return (i);
      i++;
      root = root->next;
   }
   return (-1);
}

gib_list *
gib_list_unlink(gib_list * root, gib_list * l)
{
   if (!l)
      return (root);

   if ((!root) || ((l == root) && (!l->next)))
      return (NULL);

   if (l->prev)
      l->prev->next = l->next;
   if (l->next)
      l->next->prev = l->prev;
   if (root == l)
      root = root->next;
   return (root);
}


gib_list *
gib_list_remove(gib_list * root, gib_list * l)
{
   root = gib_list_unlink(root, l);
   free(l);
   return (root);
}

gib_list *
gib_list_sort(gib_list * list, gib_compare_fn cmp)
{
   gib_list *l1, *l2;

   if (!list)
      return (NULL);
   if (!list->next)
      return (list);

   l1 = list;
   l2 = list->next;

   while ((l2 = l2->next) != NULL)
   {
      if ((l2 = l2->next) == NULL)
         break;
      l1 = l1->next;
   }
   l2 = l1->next;
   l1->next = NULL;

   return (gib_list_sort_merge
           (gib_list_sort(list, cmp), gib_list_sort(l2, cmp), cmp));
}

gib_list *
gib_list_sort_merge(gib_list * l1, gib_list * l2, gib_compare_fn cmp)
{
   gib_list list, *l, *lprev;

   l = &list;
   lprev = NULL;

   while (l1 && l2)
   {
      if (cmp(l1->data, l2->data) < 0)
      {
         l->next = l1;
         l = l->next;
         l->prev = lprev;
         lprev = l;
         l1 = l1->next;
      }
      else
      {
         l->next = l2;
         l = l->next;
         l->prev = lprev;
         lprev = l;
         l2 = l2->next;
      }
   }
   l->next = l1 ? l1 : l2;
   l->next->prev = l;

   return (list.next);
}

#if 0
gib_list *
gib_list_nth(gib_list * root, unsigned int num)
{
   unsigned int i;
   gib_list *l;

   if (num > (unsigned int) gib_list_length(root))
      return (gib_list_last(root));
   l = root;
   for (i = 0; l; ++i)
   {
      if (i == num)
         return (l);
      l = l->next;
   }
   return (root);
}
#endif

gib_list *
gib_list_foreach(gib_list *root, void (*fe_func)(gib_list *node, void *data), void *data)
{
	gib_list *i, *next = NULL;
	for (i=root; i; i=next) {
		next=i->next;
		fe_func(i, data);
	}
	return root;
}

gib_list *
gib_list_find(gib_list *root, unsigned char (*find_func)(gib_list *node, void *data), void *data)
{
	gib_list *i = NULL;
	for (i=root; i; i=i->next)
		if (find_func(i,data))
			return i;

	return NULL;
}

static unsigned char gib_list_find_by_data_callback(gib_list *list, void *data)
{
	return (list->data==data);
}

gib_list *
gib_list_find_by_data(gib_list *root, void *data)
{
	return gib_list_find(root, gib_list_find_by_data_callback, data);
}

gib_list *
gib_string_split(const char *string, const char *delimiter)
{
   gib_list *string_list = NULL;
   char *s;
   unsigned int n = 1;

   if (!string || !delimiter)
      return NULL;

   s = strstr(string, delimiter);
   if (s)
   {
      unsigned int delimiter_len = strlen(delimiter);

      do
      {
         unsigned int len;
         char *new_string;

         len = s - string;
         new_string = emalloc(sizeof(char) * (len + 1));

         strncpy(new_string, string, len);
         new_string[len] = 0;
         string_list = gib_list_add_front(string_list, new_string);
         n++;
         string = s + delimiter_len;
         s = strstr(string, delimiter);
      }
      while (s);
   }
   if (*string)
   {
      n++;
      string_list = gib_list_add_front(string_list, strdup((char *)string));
   }

   string_list = gib_list_reverse(string_list);

   return string_list;
}
