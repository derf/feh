/* utils.c

Copyright (C) 1999-2003 Tom Gilbert.
Copyright (C) 2012      Christopher Hrabak (bhs_hash, read_map )

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
		eprintf("estrdup(\"%.20s\") %s:", s,ERR_FAILED);
	strcpy(t, s);
	return t;
}

/* emalloc: malloc and report if error */
void *_emalloc(size_t n)
{
	void *p;

	p = malloc(n);
	if (p == NULL)
		eprintf("malloc of %u bytes %s:", n,ERR_FAILED);
	return p;
}

/* ecalloc: calloc and report if error.  Sets mem to zeros */
void *_ecalloc(size_t n)
{
	void *p;
	p = calloc(1,n);
	if (p == NULL)
		eprintf("calloc of %u bytes %s:", n,ERR_FAILED);
	return p;
}

/* erealloc: realloc and report if error */
void *_erealloc(void *ptr, size_t n)
{
	void *p;

	p = realloc(ptr, n);
	if (p == NULL)
		eprintf("realloc of %p by %u bytes %s:", ptr, n,ERR_FAILED);
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

char *feh_unique_filename(char *path, char *basename)
{
	char *tmpname = mobs(2);    /* static - don't free */
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
		STRCAT_7ITEMS(tmpname,path,"feh_",cppid,"_",num,"_",basename);
	}
	while (stat(tmpname, &st) == 0);
	return(tmpname);
}

char *ereadfile(char *path) {
		/* Was originally used to read a max of 4096 bytes, but, in feh it is
		* ONLY called to read in caption->file, so this can be much smaller.
		*/
	char *buffer = mobs(8);    /* SIZE_1024 */
	FILE *fp;
	int count;

	fp = fopen(path, "r");
	if (!fp)
		return NULL;

	count = fread(buffer, sizeof(char), SIZE_1024 - 1, fp);
	if (buffer[count - 1] == '\n')
		buffer[count - 1] = '\0';
	else
		buffer[count] = '\0';

	fclose(fp);

	return estrdup(buffer);
}

void esystem( char *cmd ) {
		/* a general system call, with error report.  */
	if ( system( cmd ) == -1 )
			weprintf(" system(%s) %s to run:", cmd,ERR_FAILED);

}

/* May 2012 HRABAK rewrote the feh_string_split() to eliminate gib_list
 * linked list requirement.
 */

wd * feh_string_split( char *s) {
		/* Breaks a string into an array of words (just ptrs), keeping track of
		* line number, word count (within that line) and length of word.
		* The array of "wd" structs is malloc'd once (never freed) so that same
		* mem is used over and over again.  It can grow but never shrink.
		* awp[0].linenum holds the array depth, then awp[1] holds 1st word
		* If you want to know how many spaces between words, calc the count between
		* the end of prior word and beginning of next word.  Anything over one is
		* multiple spaces.  What about tabs?
		* Returns a ptr to the awp[] array.
		*/

	static int guess = FIRST_AWP_GUESS;
	static wd *awp;                  /*  awp stands for Array(of)WordPointers */
	static int i=0;
	int linecnt=1, wordcnt=0, OnWord=0;

	/* Initialize it the first time thru  */
	if ( i==0 )
			awp =  malloc( sizeof( wd ) * guess );

	awp[0].word = s;                 /* saves the start of the whole string */
	for ( i=0 ; s[0] != '\0'; s++ ) {
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
							/* awp[i].len=1;      */
							OnWord=1;
					}   /* end of s[0] == ' ' */
			}       /* end of OnWord      */
	}           /* end of for loop    */

	/* ran off end of string, so calc the final len */
	if ( OnWord )
			awp[i].len = s - awp[i].word;

	/* if the last char in the string was a "\n" (newline) then add a final
	 * blank "word" so the height spacing is accounted for.
	 */
	if ( *(s-1) == '\n' ){             /* line break */
		i++;
		if ( i == guess ){
				guess+=FIRST_AWP_GUESS;
				awp =  erealloc( awp, sizeof( wd ) * guess );
		}
		awp[i].linenum=linecnt;
		awp[i].wordnum=wordcnt;
		awp[i].word=s;
	}

	/* This routine passes back a ptr to the awp[] array.  awp is 1-based
	* so awp[1] holds the first word in the string.
	* awp[0].linenum holds the tot_items (depth) of that array
	* awp[0].word points to the beginning of the entire string
	*/
	awp[0].linenum = i;       /* total number of array elements */

	return awp;

}     /* end of feh_string_split() */


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
			if ( i == awp[0].linenum )
					return end;
			i++;
	}

	/* ran off the end of linenum */
	return end;

}     /* end of get_next_awp_line() */

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
		* get() is called both to set() and get() keys.  It is the addit flag
		* that determines if a new node will be added if key not found.
		* If addit==1, add a new node for this key if not found
		* If addit==1, update ->data even if found
		* If addit==0, return 0 if we found it, don't updt ->data
		* If addit==0, return -1 if not found.  Don't updt ->data
		* If addit==1, and return is -1, THIS is an error!  Most probably
		*              ran out of space in hash array.
		* Warning! I do NOT make a copy of the data, just store a ptr to it.
		* So the caller is responsible for passing a persistent data ptr.
		*/

	int cnt  =((bhs_node *) a[0])->h0.cnt;         /* how many stored already */
	int hsize=((bhs_node *) a[0])->h0.size;        /* max size of hash array  */
	int i, len = strlen( key );

	a[0]->h0.found = a[0]->h0.which = 0;             /* turn off success flags */

	for ( i=1 ; i <= cnt ; i++ ) {
			/* search for the key, adding a new node if not found */
			if ( a[i]->h1.len != len )
					continue;
			/* why did the old feh use strcasecmp()? */
			if ( strcasecmp(a[i]->h1.key, key) == 0 ) {
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
			a[0]->h0.found = 0;                /* did NOT found it */
			a[0]->h0.which = (short ) i;       /* added at index cnt */
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

#if 0
/* do we really want to load MENU_STYLE anymore ? */
void feh_style_new_from_ascii(char *file, feh_style *s) {
		/* This is only called once in feh to load the menu_style file.
		* All styles are now burried inside the __fehoptions struct.
		*/

	int r = 0, g = 0, b = 0, a = 0, x_off = 0, y_off = 0;
	FILE *stylefile;
	char *c;
	int once = 1;
	char *current = mobs(8);     /* SIZE_1024 */

	stylefile = fopen(file, "r");
	if ( !stylefile )
		return;

	/* skip initial identifier line, assigning to c just to quite gcc warnings */
	c = fgets(current, SIZE_1024, stylefile);
	while (fgets(current, SIZE_1024, stylefile)) {
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

#endif   /* enf of #if 0 */

int feh_is_filelist(  char *filename ){
		/* called from feh_read_filelist() to determine if this filename is
		* a filelist (as option -f insinuated).  This is a crude test because
		* HRABAK does not like getting  "No loader for that file format" for
		* every list.  I assume that any chars found between ascii 0-8, 14-31
		* or more than dec 126  indicate it is NOT a list.
		* returns 0 on confirmation it IS a filelist, or -1 if not.
		*/

	char *buf = mobs(2);    /* SIZE_256 */
	FILE *fp;
	int count, i, ret=0;
	unsigned char *c;

	fp = fopen(filename, "r");
	if (!fp)
		return -1;

	count = fread(buf, sizeof(char), SIZE_256 - 1, fp);
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

}   /* end of feh_is_filelist() */


#define MOBS_BUF_SIZE  6144          /* way overkill?  */
/* #define MOBS_ROW_SIZE  128 */     /* moved to feh.h */
#define MOBS_MAP_CNT  (int)( MOBS_BUF_SIZE /  MOBS_ROW_SIZE )

char * mobs( int rows ) {
		/* 'mobs' stands for MiniObStack.  It callocs a block of mem on
		* the heap and doles out ptrs to that memory to the caller.
		* This is meant for short term temporary string creation and
		* avoids that memory thrashing of malloc() and free() inside
		* a loop. 'flag' controls if you want it locked for longer,
		* but not implemented yet.
		* returns a ptr to a block of memory (rows*128)bytes long.
		* If rows==0, then the caller wants BIG chunk, which is
		* reserved at the top.  All others start half way down.
		* calloc() assures that obs is all cleared to NULL at start.
		*/

	/*    static char map[ MOBS_MAP_CNT ]; */
	static char *obs, *sentinal, *ret, *err ="mobs() fatal! ";
	static unsigned int i=1, last_obs, next_obs=0;
	static unsigned int half_obs = MOBS_MAP_CNT / 2;

	/* first-time thru for initialization */
	if ( i ){
			obs = (char *) ecalloc( (MOBS_BUF_SIZE + 4)* sizeof( char ) );

			/* set bits in the last 4 bytes to be all 11111111's */
			i = MOBS_BUF_SIZE;
			sentinal = obs + i;
			obs[i]=obs[i+1]=obs[i+2]=obs[i+3]=(char)0xff;
			i = 0;  /* turn off the first time flag */
			next_obs = half_obs;
	}

	/* check the sentinel bits each time to see if we ran off the back */
	if ( sentinal[0] != (char)0xff )
		eprintf( "%s Buffer overflow.", err);

	/* caller has requested number of "rows" within this block.
		* If it doesn't fit, I have to start at the top again.
		* If rows==0, then start them at the VERY top.
		*/
	if ( rows ) {
			if ( (next_obs + rows) > MOBS_MAP_CNT ){
					/* start at the half-way mark again. */
					if ( rows > MOBS_MAP_CNT )
							eprintf( "%s Out-of-range ", err);
					last_obs = half_obs;
			} else
					last_obs = next_obs;

			/* this will be the next place to start */
			next_obs = last_obs + rows;
			ret = ( obs + ( last_obs * MOBS_ROW_SIZE ) );
	} else {
			ret = obs ;           /* this is the whole kahuna */
	}

	ret[0] = ret[1] = '\0';   /* ready for strcat */

	return ret;

}   /* end of mobs() */

/* June 2012 consolidated all the read_map.c stuff here */
extern Key2Action *menu_actions, *feh_actions, *move_actions;         /* see newkeyev.c */
extern SubsetAction act_subset[];                                     /* see newkeyev.c */
extern StubPtr funcptr[];
extern struct _binding_state bs ;



int read_map_from_disc( void ) {
		/* called from init_keyevents().  See write_map2disc() for details.  Reads the
		* binary feh_bindings.map and loads this directly into the four arrays ...
		*      menu_actions[], move_actions[], feh_actions[] and act_subset[]
		* This routine is called twice.  If it fails on the first try, then the caller
		* shells out to run feh_init, then tries again to actually load the map.
		*/

	int iErr=-1;
	size_t len=0, size;
	time_t t1,t2;
	char *buf   = mobs(2);
	char *mpath = mobs(2);
	struct stat st;
	FILE *fs=NULL;
	int map_info[4];

	/* find where the configuration files are kept. */
	if ( find_config_files( FEH_BINDINGS_MAP, mpath, &st )==0) {     /* look for the external map  file */
			t2=st.st_mtime;
			if ( find_config_files( FEH_EXTERNAL_KEYS_FILE , buf, &st )==0) {  /* look for the external keys file */
					t1=st.st_mtime;
					if (t1>t2 ) return(iErr);                                 /* force a feh_init run */
			}
	} else return (iErr);                                             /* force a feh_init run */

	/* we have the .map file and it is up-to-date so use it. */
	fs=fopen( mpath, "r" );
	if (! fs) return(iErr);

	/* read the map_info header first; an array of four (int) in enum() order */
	size=1;
	len=fread(map_info, sizeof(map_info), size, fs);
	if (len != size ) return (iErr*2);

	/* malloc a new menu_actions[] and fill it */
	size=map_info[MENU_PTR] ;
	bs.acnt[ MENU_PTR ]=size;
	menu_actions=malloc( sizeof( Key2Action) * size );
	len=fread(menu_actions, sizeof( Key2Action ), size , fs);
	if (len != size ) return (iErr*3);

	/* malloc a new move_actions[] and fill it */
	size=map_info[MOVE_PTR] ;
	bs.acnt[ MOVE_PTR ]=size;
	move_actions=malloc( sizeof( Key2Action) * size );
	len=fread(move_actions, sizeof( Key2Action ), size , fs);
	if (len != size ) return (iErr*4);

	/* malloc a new feh_actions[] and fill it */
	size=map_info[FEH_PTR] ;
	bs.acnt[ FEH_PTR ]=size;
	feh_actions=malloc( sizeof( Key2Action) * size );
	len=fread(feh_actions, sizeof( Key2Action ), size , fs);
	if (len != size ) return (iErr*4);

	/* act_subset is always a fixed (static) size.   fill it */
	size=map_info[SUB_PTR] ;
	len=fread(act_subset, sizeof( SubsetAction ), size , fs);
	if (len != size ) return (iErr*5);

	fclose (fs);

	/* post the real funcptr's to all four of these _actions[] arrays.
	* The function ptrs loaded from the .map are just the INDEX values
	* to the real function ptrs stored in the funcptr[] array.
	* Reuse the "size" var just as an index var.
	*/
	for ( size=0; size < (unsigned)bs.acnt[ MENU_PTR ] ; size++ )
			menu_actions[size].ptr=funcptr[(int)menu_actions[size].ptr];

	for ( size=0; size < (unsigned)bs.acnt[ MOVE_PTR ] ; size++ )
			move_actions[size].ptr=funcptr[(int)move_actions[size].ptr];

	for ( size=0; size < (unsigned)bs.acnt[ FEH_PTR ] ; size++ )
			feh_actions[size].ptr=funcptr[(int)feh_actions[size].ptr];

	for ( size=0; size < (unsigned)bs.acnt[ SUB_PTR ] ; size++ )
			act_subset[size].ptr=funcptr[(int)act_subset[size].ptr];


	return ( 0 );

}   /* end of read_map_from_disc() */



int find_config_files( const char * name, char *buf , struct stat *st ) {
		/* called by init_keyevents().  Looks thru the hierarchy of paths for the
		* first hit on finding the keys file.  char *name will either be keys or
		* feh_bindings.map.  Returns 0 on success of statting the file, else -1.
		* The search hierarchy is XDG_CONFIG_HOME/.config/feh/keys then
		* getenv("HOME")/.config/feh/keys, or lastly /etc/feh/keys
		*/

	char *home = NULL;

	buf[0]=buf[1]='\0';             /* no telling what's in there. */
	strcat(buf, "/etc");            /* if no other path is found, this is default */
	home = getenv("XDG_CONFIG_HOME");

	if (home) {
		buf[0]=buf[1]='\0';           /* start at the beg */
		strcat(buf, home);
	} else {
		home = getenv("HOME");
		if (home) {
				buf[0]=buf[1]='\0';       /* start at the beg */
				STRCAT_2ITEMS(buf, home, ".config" );
		} else
				eprintf("No HOME in environment\n");
	}

	/* buf will either be /etc or home, so tack on the name we are looking for */
	strcat(buf, "/feh/" );
	fgv.bind_path = estrdup(buf);       /* used to find help files */
	strcat(buf, name );

	return ( stat( buf, st ) ) ;        /* stat fills struct stat *st and returns success==0 */

}   /* end of find_config_files() */



