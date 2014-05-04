/* gib_style.c

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

#include "gib_style.h"
#include "gib_utils.h"

gib_style *
gib_style_new(char *name)
{
   gib_style *s = NULL;

   s = malloc(sizeof(gib_style));

   memset(s, 0, sizeof(gib_style));
   if (name)
      s->name = strdup(name);

   return (s);
}

void
gib_style_free(gib_style * s)
{
   if (s)
   {
      if (s->name)
         free(s->name);
      if (s->bits)
      {
         gib_list *l;

         l = s->bits;
         while (l)
         {
            gib_style_bit_free((gib_style_bit *) l->data);
            l = l->next;
         }
         gib_list_free(s->bits);
      }
      free(s);
   }
   return;
}

gib_style_bit *
gib_style_bit_new(int x_offset, int y_offset, int r, int g, int b, int a)
{
   gib_style_bit *sb;

   sb = malloc(sizeof(gib_style_bit));
   memset(sb, 0, sizeof(gib_style_bit));

   sb->x_offset = x_offset;
   sb->y_offset = y_offset;
   sb->r = r;
   sb->g = g;
   sb->b = b;
   sb->a = a;

   return (sb);
}

void
gib_style_bit_free(gib_style_bit * s)
{
   if (s)
      free(s);
   return;
}

gib_style *
gib_style_dup(gib_style * s)
{
   gib_style *ret;

   ret = gib_style_new(s->name);
   ret->bits = gib_list_dup_special(s->bits, gib_dup_style_bit);

   return (ret);
}

void
gib_dup_style_bit(void **dest, void *data)
{
   *dest = malloc(sizeof(gib_style_bit));
   memcpy(*dest, data, sizeof(gib_style_bit));

   return;
}

void
gib_style_save_ascii(gib_style * style, char *file)
{
   FILE *stylefile;
   gib_list *l;
   gib_style_bit *b;
   
   if (!style || !style->bits)
      return;

   stylefile = fopen(file, "w");
   if (stylefile)
   {
      fprintf(stylefile, "#Style\n");
      fprintf(stylefile, "#NAME %s\n", style->name);
      l = style->bits;
      while (l)
      {
         b = (gib_style_bit *) l->data;
         fprintf(stylefile, "%d %d %d %d %d %d\n", b->r, b->g, b->b, b->a,
                 b->x_offset, b->y_offset);
         l = l->next;
      }
   }
   fclose(stylefile);
}

gib_style *
gib_style_new_from_ascii(char *file)
{
   FILE *stylefile;
   char current[4096];
   char *s;
   gib_style *ret = NULL;

   stylefile = fopen(file, "r");
   if (stylefile)
   {
      int r = 0, g = 0, b = 0, a = 0, x_off = 0, y_off = 0;

      ret = gib_style_new(NULL);
      /* skip initial idenifier line */
      fgets(current, sizeof(current), stylefile);
      while (fgets(current, sizeof(current), stylefile))
      {
         if (current[0] == '\n')
            continue;
         if (!strncmp(current, "#NAME", 5))
         {
            int l;

            l = strlen(current) - 1;
            if (current[l] == '\n')
               current[l] = '\0';
            if (l > 6)
               ret->name = strdup(current + 6);
            continue;
         }
         else
         {
            /* support EFM style bits */
            s = strtok(current, " ");
            if(!s) continue;
            if (strlen(s) == 2)
            {
               if (!strcmp(s, "ol"))
               {
                  r = g = b = 0;
                  s = strtok(NULL, " ");
                  if(!s) continue;
                  x_off = atoi(s);
                  s = strtok(NULL, " ");
                  if(!s) continue;
                  y_off = atoi(s);
               }
               else if (!strcmp(s, "sh"))
               {
                  r = g = b = 0;
                  s = strtok(NULL, " ");
                  if(!s) continue;
                  x_off = atoi(s);
                  s = strtok(NULL, " ");
                  if(!s) continue;
                  y_off = atoi(s);
                  s = strtok(NULL, " ");
                  if(!s) continue;
                  a = atoi(s);
               }
               else if (!strcmp(s, "fg"))
               {
                  r = g = b = a = 0;
                  s = strtok(NULL, " ");
                  if(!s) continue;
                  x_off = atoi(s);
                  s = strtok(NULL, " ");
                  if(!s) continue;
                  y_off = atoi(s);
               }
            }
            else
            {
               /* our own format */
               r = atoi(s);
               s = strtok(NULL, " ");
               if(!s) continue;
               g = atoi(s);
               s = strtok(NULL, " ");
               if(!s) continue;
               b = atoi(s);
               s = strtok(NULL, " ");
               if(!s) continue;
               a = atoi(s);
               s = strtok(NULL, " ");
               if(!s) continue;
               x_off = atoi(s);
               s = strtok(NULL, " ");
               if(!s) continue;
               y_off = atoi(s);
            }
         }
         ret->bits =
            gib_list_add_end(ret->bits,
                               gib_style_bit_new(x_off, y_off, r, g, b, a));
      }
      fclose(stylefile);
   }

   return (ret);
}
