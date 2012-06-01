/* utils.c

Copyright (C) 1999-2003 Tom Gilbert.
Copyright (C) 2012      Christopher Hrabak (for bhs_hash )

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

/* eprintf: print error message and exit */
void eprintf(char *fmt, ...)
{
	va_list args;

	fflush(stdout);
	fputs(PACKAGE " ERROR: ", stderr);

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	if (fmt[0] != '\0' && fmt[strlen(fmt) - 1] == ':')
		fprintf(stderr, " %s", strerror(errno));
	fputs("\n", stderr);
	exit(2);
}

/* weprintf: print warning message and continue */
void weprintf(char *fmt, ...)
{
	va_list args;

	fflush(stdout);
	fputs(PACKAGE " WARNING: ", stderr);

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	if (fmt[0] != '\0' && fmt[strlen(fmt) - 1] == ':')
		fprintf(stderr, " %s", strerror(errno));
	fputs("\n", stderr);
}

/* estrdup: duplicate a string, report if error */
char *_estrdup(char *s)
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
void *_emalloc(size_t n)
{
	void *p;

	p = malloc(n);
	if (p == NULL)
		eprintf("malloc of %u bytes failed:", n);
	return p;
}

/* erealloc: realloc and report if error */
void *_erealloc(void *ptr, size_t n)
{
	void *p;

	p = realloc(ptr, n);
	if (p == NULL)
		eprintf("realloc of %p by %u bytes failed:", ptr, n);
	return p;
}

/* because gib_lib just used free, not an efree variant,
 * Just ignore the extra function call and replace all
 * gib_efree instances with just free()
void _efree(void *p)
{
   free(p);
}
*
*/

char *estrjoin(const char *separator, ...)
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

	if (s) {
		len = strlen(s);
		s = va_arg(args, char *);

		while (s) {
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

		while (s) {
			strcat(string, separator);
			strcat(string, s);
			s = va_arg(args, char *);
		}
	} else
		string = estrdup("");
	va_end(args);

	return string;
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

	/* Massive paranoia ;) */
	if (i > 999998)
		i = 1;

	ppid = getpid();
	snprintf(cppid, sizeof(cppid), "%06ld", (long) ppid);

	/* make sure file doesn't exist */
	do {
		snprintf(num, sizeof(num), "%06ld", i++);
		tmpname = estrjoin("", path, "feh_", cppid, "_", num, "_", basename, NULL);
	}
	while (stat(tmpname, &st) == 0);
	return(tmpname);
}

/* reads file into a string, but limits to 4095 chars and ensures a \0 */
char *ereadfile(char *path)
{
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


/* May 2012 HRABAK rewrote the feh_string_split() to eliminate gib_list
 * linked list requirement.
 */

char * get_next_awp_line( wd awp[] , int linenum, int i_start, int *i_end ){
    /* caller sets the i_start index to begin looking for a full line of
     * text in the awp[] array.  Returns the char *end ptr where it stopped,
     * setting the i_end index.
     */

    int i = i_start;
    char *end = NULL;

    /* incase this line is blank */
    *i_end = i_start;
    if ( linenum < awp[i].linenum )
        return end;

    linenum =  awp[i].linenum;          /* go till it changes */
    while ( awp[i].linenum == linenum ){
        end = awp[i].word + awp[i].len;
        *i_end = i;
        if ( i == awp[0].tot_items )
            return end;
        i++;
    }

    /* ran off the end of linenum */
    return end;

}     /* end of get_next_awp_line() */

wd * feh_string_split( char *s) {
    /* Breaks a string into an array of words (just ptrs), keeping track of
     * line number, word count (within that line) and length of word.
     * I keep the array of "wd" structs malloc'd (never free it) so that same
     * mem is used over and over again.  It can grow but never shrink.
     * If you want to know how many spaces between words, calc the count between
     * the end of prior word and beginning of next word.  Anything over one is
     * multiple spaces.  What about tabs?
     * Returns a ptr to the awp[] array.
     */

    static int guess = FIRST_AWP_GUESS;
    static wd *awp;                  /*  awp stands for Array(of)WordPointers */
    static int i=0;
    int linecnt=1, wordcnt=0, OnWord=0;

    /* I don't like this solution but ....  Initialize it the first time thru  */
    if ( i==0 )
        awp =  malloc( sizeof( wd ) * guess );

    /* start i = -1 so that the first i++ will bring it up to awp[0] */
    for ( i=-1 ; s[0] != '\0'; s++ ) {
        if (OnWord){                 /* currently on a word. go till it ends */
            if ( s[0] == ' ' ) {                    /* word ends */
                awp[i].len = s - awp[i].word;
                OnWord=0;
            } else if ( s[0] == '\n' ){             /* line break */
                awp[i].len = s - awp[i].word;
                linecnt++;                          /* reset line and wordcnt */
                wordcnt=0;
                OnWord=0;
            }
        } else {                      /* OnWord==0.  Are we coming into one? */
            if ( s[0] == ' ' ) {                    /* ignore spaces */
                continue;
            } else if ( s[0] == '\n' ){             /* line break */
                linecnt++;
                wordcnt=0;
            } else {                                /* coming into a word */
                i++;
                if ( i == guess ){
                    guess+=FIRST_AWP_GUESS;
                    awp =  erealloc( awp, sizeof( wd ) * guess );
                }
                wordcnt++;
                awp[i].linenum=linecnt;
                awp[i].wordnum=wordcnt;
                awp[i].word=s;
                /* awp[i].len=1; */
                OnWord=1;
            }   /* end of s[0] == ' ' */
        }       /* end of OnWord */
    }           /* end of for loop */

    /* ran off end of string, so calc the final len */
    if ( OnWord )
        awp[i].len = s - awp[i].word;

    /* This routine passes back a ptr to the awp[] array.  I pass back the
     * total count of items in that array (in the awp[0] element only),
     * meaning that all other tot_items memebers space is wasted.  :(
     */
    awp[0].tot_items = i;                         /* total number of words */

    return awp;

}     /* end of feh_string_split() */


/* May 2012 HRABAK replaced all the gib_hash() stuff with bhs_hash() stuff.
 * bhs stands for BoneHeadSimple.
 */


bhs_node ** bhs_hash_init(  int size  ) {
    /* alloc space for a bhs_hash array of size items, creating the
     * root node (a[0]) to hold the stats for this hash.  Store the array
     * size in the size member.  The h0.cnt member will hold the cnt of how
     * many items are already in the hash array.
     * The union {} h0 allows me to store this root_node data in the same
     * space normally used for the key and data pointers.
     * Because of the 0th (root_node) the items in the hash array propper
     * are one(1)-based.  Therefore the num of items in the hash are h.cnt.
     * The next availabe (empty) node is at h.cnt+1.
     * No provisions for deleting nodes cause feh does not need that.
     */

  int i;
  bhs_node * n = (bhs_node *) emalloc( sizeof(bhs_node ));
  bhs_node **a =(bhs_node **) emalloc( sizeof( bhs_node* ) * ( size +1 ) );
  n->h0.cnt = 0 ;
  n->h0.size  = size;
  a[0] = n;
  for (i = 1; i< size ; i++ ) a[i] = NULL;
  return a;

}     /* end of bhs_hash_init() */

int bhs_hash_get( bhs_node **a , char *key, char *data, int addit ) {
    /* a[] holds ptrs to all the nodes.  Search them all to find key.
     * get() is caled both to set() and get() keys.  It is the addit flag
     * that determines if a new node will be added if key not found.
     * If addit==1, add a new node for this key if not found
     * If addit==1, update ->data even if found
     * If addit==0, return 0 if we found it, don't updt ->data
     * If addit==0, return -1 if not found.  Don't updt ->data
     * If addit==1, and return is -1, THIS is an error!  Most probably
     *              ran out of space in hash array.
     */

  int cnt=((bhs_node *) a[0])->h0.cnt;           /* how many stored already */
  int hsize=((bhs_node *) a[0])->h0.size;        /* max size of hash array  */
  int i, len = strlen( key );

  a[0]->h0.found = a[0]->h0.which = 0;             /* turn off success flags */

  for ( i=1 ; i < cnt ; i++ ) {
      /* search for the key, adding a new node if not found */
      if ( a[i]->h1.len != len )
          continue;
      if 	( strcasecmp(a[i]->h1.key, key) == 0 ) {
          /* found it.  So update the data and leave */
          a[0]->h0.found = 1;                     /* found it */
          a[0]->h0.which = (short ) i;            /* found it at index i */
          if ( addit ) a[i]->h1.data = data;
          return 0;
      }
  }

  /* NOT found.  So do we add a new one? */
  if ( !addit ) return -1;              /* not an error unless addit was 1 too */
  if ( cnt == hsize )
      return -1;                        /* no more room in hash array */
  else {                                /* add a new node */
      bhs_node * n = (bhs_node *) malloc( sizeof(bhs_node ));
      cnt++;
      n->h1.key = key;
      n->h1.len = len;
      n->h1.data = data;
      a[cnt] = n;
      a[0]->h0.found = 0;                     /* did NOT found it */
      a[0]->h0.which = (short ) i;            /* added at index cnt */
      a[0]->h0.cnt = cnt;
  }

  return (cnt );

}     /* end of bhs_hash_get() */

#if 0

void bhs_hash_rpt( bhs_node **a , int addit, int ret_code ) {
    /* Report of what hash_get() did.  This is called right after a called to
     * bhs_hash_get() and is supplied the ret_code from that call.
     * This rpt() logic printf()s what happened and gives the data.
     * If addit==1, then add a new node for this key if not found
     * If addit==1, update ->data even if found
     * If addit==0, return 0 if we found it, don't updt ->data
     * If addit==0, return -1 if not found.  Don't updt ->data
     * If addit==1, and return is -1, then ran out of space in hash array
     */

  int i=0;

  if ( (addit == 1) && ( ret_code > 0 ) )
      printf("Addit was on and a new node was added at %d.\n", ret_code );
  else if ( (addit == 1) && ( ret_code == 0 ) )
      printf("Addit was on and existing node was found at %d.  Data updated.\n", a[0]->h0.which );
  else if ( (addit == 1) && ( ret_code == -1 ) )
      printf("Addit was onn, key not found and NO ROOM to add a new one. This is an error (%d).\n", ret_code );
  else if ( (addit == 0) && ( ret_code == -1 ) )
      printf("Addit was off, key not found, This is not an error (%d).  Data NOT updated.\n", ret_code );
  else if ( (addit == 0) && ( ret_code == 0 ) )
      printf("Addit was off and existing node was found at %d.  Data NOT updated.\n", a[0]->h0.which );
  else
      printf("Some unknown combo of addit and ret_code.  Huh??? Addit is %d, ret_code is %d\n.", addit, ret_code );


  /* just a trigger to print out the entire hash */
  if ( addit > 1 ) {
      printf("Root node info...\n");
      printf("... There are %d hash nodes (Max size is %d).\n" , a[0]->h0.cnt,a[0]->h0.size );
      printf("... Found is %d, which is %d, flag1 is %d\n" , a[0]->h0.found,a[0]->h0.which,a[0]->h0.flag1 );
      for (i = 1; i< addit +1 ; i++ ) {
          if (  a[i] )
              printf("Key #%d (len %d) is '%15s'->'%s'\n" , i, a[i]->h1.len, a[i]->h1.key, a[i]->h1.data  );
          else
              printf("Key #%d is not used.\n" , i );
      }
  }

  return;

}
#endif   /* end of #if 0 */

/* As of May 2012 HRABAK killed all the gib_style stuff and replaced it with a
 * simple feh_style structure.  Only  gib_style_new_from_ascii() was saved
 * and renamed here...
*/

void feh_style_new_from_ascii(char *file, feh_style *s) {
    /* This is only called once in feh to load the menu_style file.
     * All styles are now burried inside the __fehoptions struct.
     */

   int r = 0, g = 0, b = 0, a = 0, x_off = 0, y_off = 0;
   FILE *stylefile;
   char *c;
   int once = 1;
   char current[4096];

   stylefile = fopen(file, "r");
   if ( !stylefile )
      return;

   /* skip initial idenifier line, assigning to c just to quite gcc warnings */
   c = fgets(current, sizeof(current), stylefile);
   while (fgets(current, sizeof(current), stylefile)) {
      if (current[0] == '\n')
         continue;
      if (!strncmp(current, "#NAME", 5))
      {
        /* as of May 2012, ignore the #NAME cause
         * feh ONLY calls this once for menu_style.

         int l;

         l = strlen(current) - 1;
         if (current[l] == '\n')
            current[l] = '\0';
         if (l > 6)
            ret->name = estrdup(current + 6);
        *
        */

         continue;
      }
      else
      {
         /* support EFM style bits */
         c = strtok(current, " ");
         if(!c) continue;
         if (strlen(c) == 2)
         {
            if (!strcmp(c, "ol"))
            {
               r = g = b = 0;
               c = strtok(NULL, " ");
               if(!c) continue;
               x_off = atoi(c);
               c = strtok(NULL, " ");
               if(!c) continue;
               y_off = atoi(c);
            }
            else if (!strcmp(c, "sh"))
            {
               r = g = b = 0;
               c = strtok(NULL, " ");
               if(!c) continue;
               x_off = atoi(c);
               c = strtok(NULL, " ");
               if(!c) continue;
               y_off = atoi(c);
               c = strtok(NULL, " ");
               if(!c) continue;
               a = atoi(c);
            }
            else if (!strcmp(c, "fg"))
            {
               r = g = b = a = 0;
               c = strtok(NULL, " ");
               if(!c) continue;
               x_off = atoi(c);
               c = strtok(NULL, " ");
               if(!c) continue;
               y_off = atoi(c);
            }
         }
         else
         {
            /* our own format */
            r = atoi(c);
            c = strtok(NULL, " ");
            if(!c) continue;
            g = atoi(c);
            c = strtok(NULL, " ");
            if(!c) continue;
            b = atoi(c);
            c = strtok(NULL, " ");
            if(!c) continue;
            a = atoi(c);
            c = strtok(NULL, " ");
            if(!c) continue;
            x_off = atoi(c);
            c = strtok(NULL, " ");
            if(!c) continue;
            y_off = atoi(c);
         }
      }
      if (once ) {
          s->bg.r=r; s->bg.g=g; s->bg.b=b; s->bg.a=a;
          s->bg.x_off=x_off; s->bg.y_off=y_off;
          once=0;
      } else {
          s->fg.r=r; s->fg.g=g; s->fg.b=b; s->fg.a=a;
          s->fg.x_off=x_off; s->fg.y_off=y_off;
      }
   }

   fclose(stylefile);

   return ;

}     /* end of feh_style_new_from_ascii() */

int is_filelist(  char *filename ){
    /* called from feh_read_filelist() to determine if this filename is
     * a filelist (as option -f insinuated).  This is a crude test because
     * HRABAK does not like getting  "No loader for that file format" for
     * every list.  I assume that any chars between ascii 0-8, 14-31 and
     * greater than dec 126 found indicate it is NOT a list.
     * returns 0 on confirmation it IS a filelist, or -1 if not.
     */

    char buf[256];
    FILE *fp;
    int count, i, ret=0;
    unsigned char *c;

    fp = fopen(filename, "r");
    if (!fp)
      return -1;

    count = fread(buf, sizeof(char), sizeof(buf) - 1, fp);
    if ( count > 1 ) {
        /* walk thru every char until I find one outside of my test (guess) range */
        for ( i=0, c=(unsigned char *)buf; i< count ; i++, c++ ){
            if ( c[0] < 9 ){
                ret = -1; break;
            } else if ( c[0] > 126 ) {
                ret = -1; break;
            } else if ( c[0] > 13 && c[0] < 32 ) {
                ret = -1; break;
            }
        }
    }

    fclose(fp);

    return ret;

}   /* end of is_filelist() */