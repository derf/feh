/* utils.c

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
#include "debug.h"
#include "options.h"

static char *feh_user_name = NULL;
static char *feh_tmp_dir = NULL;

/* eprintf: print error message and exit */
void
eprintf(char *fmt, ...)
{
   va_list args;

   fflush(stdout);
   fprintf(stderr, "%s ERROR: ", PACKAGE);

   va_start(args, fmt);
   vfprintf(stderr, fmt, args);
   va_end(args);

   if (fmt[0] != '\0' && fmt[strlen(fmt) - 1] == ':')
      fprintf(stderr, " %s", strerror(errno));
   fprintf(stderr, "\n");
   exit(2);
}

/* weprintf: print warning message and continue */
void
weprintf(char *fmt, ...)
{
   va_list args;

   fflush(stdout);
   fprintf(stderr, "%s WARNING: ", PACKAGE);

   va_start(args, fmt);
   vfprintf(stderr, fmt, args);
   va_end(args);

   if (fmt[0] != '\0' && fmt[strlen(fmt) - 1] == ':')
      fprintf(stderr, " %s", strerror(errno));
   fprintf(stderr, "\n");
}

/* estrdup: duplicate a string, report if error */
char *
_estrdup(char *s)
{
   char *t;

   if (!s)
      return NULL;

   t = (char *) malloc(strlen(s) + 1);
   if (t == NULL)
      eprintf("estrdup(\"%.20s\") failed:", s);
   strcpy(t, s);
   return t;
}

/* emalloc: malloc and report if error */
void *
_emalloc(size_t n)
{
   void *p;

   p = malloc(n);
   if (p == NULL)
      eprintf("malloc of %u bytes failed:", n);
   return p;
}

/* erealloc: realloc and report if error */
void *
_erealloc(void *ptr, size_t n)
{
   void *p;

   p = realloc(ptr, n);
   if (p == NULL)
      eprintf("realloc of %p by %u bytes failed:", ptr, n);
   return p;
}

char *
estrjoin(const char *separator, ...)
{
   char *string, *s;
   va_list args;
   int len;
   int separator_len;

   if (separator == NULL)
      separator = "";

   separator_len = strlen(separator);
   va_start(args, separator);
   s = va_arg(args, char *);

   if (s)
   {
      len = strlen(s);
      s = va_arg(args, char *);

      while (s)
      {
         len += separator_len + strlen(s);
         s = va_arg(args, char *);
      }
      va_end(args);
      string = malloc(sizeof(char) * (len + 1));

      *string = 0;
      va_start(args, separator);
      s = va_arg(args, char *);

      strcat(string, s);
      s = va_arg(args, char *);

      while (s)
      {
         strcat(string, separator);
         strcat(string, s);
         s = va_arg(args, char *);
      }
   }
   else
      string = estrdup("");
   va_end(args);

   return string;
}

char *
stroflen(char c, int l)
{
   static char buf[1024];
   int i = 0;

   buf[0] = '\0';
   while (l--)
      buf[i++] = c;
   buf[i] = '\0';
   return buf;
}

/* free the result please */
char *feh_unique_filename(char *path, char *basename)
{
   char *tmpname;
   char num[10];
   char cppid[10];
   static long int i = 1;
   struct stat st;
   pid_t ppid;

   D_ENTER(4);
   /* Massive paranoia ;) */
   if (i > 999998)
      i = 1;

   ppid = getpid();
   snprintf(cppid, sizeof(cppid), "%06ld", (long)ppid);

   /* make sure file doesn't exist */
   do
   {
      snprintf(num, sizeof(num), "%06ld", i++);
      tmpname =
         estrjoin("", path, "feh_", cppid, "_", num, "_",
                  basename, NULL);
   }
   while (stat(tmpname, &st) == 0);
   D_RETURN(4, tmpname);
}

/* reads file into a string, but limits o 4095 chars and ensures a \0 */
char *ereadfile(char *path) {
  char buffer[4096];
  FILE *fp;
  int count;

  fp = fopen(path, "r");
  if (!fp)
    return NULL;

  count = fread(buffer, sizeof(char), sizeof(buffer) - 1, fp);
  if (buffer[count - 1] == '\n')
    buffer[count - 1] = '\0';
  else
    buffer[count] = '\0';
  
  fclose(fp);

  return estrdup(buffer);  
}

char *feh_get_tmp_dir(void) {
  char *tmp;
  if (feh_tmp_dir)
    return feh_tmp_dir;
  tmp = getenv("TMPDIR");
  if (!tmp)
    tmp = getenv("TMP");
  if (!tmp)
    tmp = getenv("TEMP");
  if (!tmp)
    tmp = "/tmp";
  feh_tmp_dir = estrdup(tmp);
  return feh_tmp_dir;
}

char *feh_get_user_name(void) {
    struct passwd *pw = NULL;

    if (feh_user_name)
      return feh_user_name;
    setpwent ();
    pw = getpwuid (getuid ());
    endpwent ();
    feh_user_name = estrdup(pw->pw_name);
    return feh_user_name;
}
