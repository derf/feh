/* timers.c

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

#include "feh.h"
#include "options.h"
#include "timers.h"

fehtimer first_timer = NULL;

void
feh_handle_timer(void)
{
   fehtimer ft;

   D_ENTER(4);
   if (!first_timer)
   {
      D(3,("No timer to handle, returning\n"));
      D_RETURN_(4);
   }
   ft = first_timer;
   first_timer = first_timer->next;
   D(3,("Executing timer function now\n"));
   (*(ft->func)) (ft->data);
   D(3,("Freeing the timer\n"));
   if (ft && ft->name)
      free(ft->name);
   if (ft)
      free(ft);
   D_RETURN_(4);
}

double
feh_get_time(void)
{
   struct timeval timev;

   D_ENTER(4);
   gettimeofday(&timev, NULL);
   D_RETURN(4,(double) timev.tv_sec + (((double) timev.tv_usec) / 1000000));
}

void
feh_remove_timer(char *name)
{
   fehtimer ft, ptr, pptr;

   D_ENTER(4);
   D(3,("removing %s\n", name));
   pptr = NULL;
   ptr = first_timer;
   while (ptr)
   {
      D(3,("Stepping through event list\n"));
      ft = ptr;
      if (!strcmp(ft->name, name))
      {
         D(3,("Found it. Removing\n"));
         if (pptr)
            pptr->next = ft->next;
         else
            first_timer = ft->next;
         if (ft->next)
            ft->next->in += ft->in;
         if (ft->name)
            free(ft->name);
         if (ft)
            free(ft);
         D_RETURN_(4);
      }
      pptr = ptr;
      ptr = ptr->next;
   }
   D_RETURN_(4);
}

void
feh_add_timer(void (*func) (void *data), void *data, double in, char *name)
{
   fehtimer ft, ptr, pptr;
   double tally;

   D_ENTER(4);
   D(3,("adding timer %s for %f seconds time\n", name, in));
   feh_remove_timer(name);
   ft = malloc(sizeof(_fehtimer));
   ft->next = NULL;
   ft->func = func;
   ft->data = data;
   ft->name = estrdup(name);
   ft->just_added = 1;
   ft->in = in;
   D(3,("ft->in = %f\n", ft->in));
   tally = 0.0;
   if (!first_timer)
   {
      D(3,("No first timer\n"));
      first_timer = ft;
   }
   else
   {
      D(3,("There is a first timer\n"));
      pptr = NULL;
      ptr = first_timer;
      tally = 0.0;
      while (ptr)
      {
         tally += ptr->in;
         if (tally > in)
         {
            tally -= ptr->in;
            ft->next = ptr;
            if (pptr)
               pptr->next = ft;
            else
               first_timer = ft;
            ft->in -= tally;
            if (ft->next)
               ft->next->in -= ft->in;
            D_RETURN_(4);
         }
         pptr = ptr;
         ptr = ptr->next;
      }
      if (pptr)
         pptr->next = ft;
      else
         first_timer = ft;
      ft->in -= tally;
   }
   D(3,("ft->in = %f\n", ft->in));
   D_RETURN_(4);
}

void
feh_add_unique_timer(void (*func) (void *data), void *data, double in)
{
   static long i = 0;
   char evname[20];

   D_ENTER(4);
   snprintf(evname, sizeof(evname), "T_%ld", i);
   D(4,("adding timer with unique name %s\n", evname));
   feh_add_timer(func, data, in, evname);
   i++;
   /* Mega paranoia ;) */
   if (i > 1000000)
      i = 0;
   D_RETURN_(4);
}
