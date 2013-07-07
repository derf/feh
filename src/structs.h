/* structs.h

Copyright (C) 1999-2003 Tom Gilbert.
Copyright (C) 2010-2011 Daniel Friesel.

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

#ifndef STRUCTS_H
#define STRUCTS_H

typedef struct __winwidget _winwidget;
typedef _winwidget *winwidget;
typedef struct __fehoptions fehoptions;
typedef struct __fehkey fehkey;
typedef struct __fehkb fehkb;
typedef struct __fehbutton fehbutton;
typedef struct __fehbb fehbb;

/* June 2012 structure to encapsulate all the old feh global vars  */
/* from menu.h */
typedef struct _feh_menu feh_menu;
typedef struct _feh_menu_item feh_menu_item;
typedef struct _feh_menu_list feh_menu_list;
/*typedef feh_menu *(*menuitem_func_gen) (feh_menu * m);*/

typedef  void (*StubPtr)( void );      /* def for keypress function pointers */
typedef struct __feh_node feh_node;

/* 'fgv' stands for FehGlobalVars */
typedef struct __feh_global_vars feh_global_vars ;

struct __feh_global_vars {

		/* Imlib stuff */
		Display *disp;
		Visual *vis;
		Colormap cm;
		int depth;
		Atom wmDeleteWindow;

		#ifdef HAVE_LIBXINERAMA
			int num_xinerama_screens;
			XineramaScreenInfo *xinerama_screens;
			int xinerama_screen;
		#endif          /* HAVE_LIBXINERAMA */

		/* Thumbnail sizes */
		int cmdargc;
		char **cmdargv;
		Window root;
		XContext xid_context;
		Screen *scr;

		char *theme;
		int window_num;            /*  which one in windows[] below    */
		winwidget *windows;        /* array of all windows open in feh */

		/* to terminate long-running children with SIGALRM */
		int childpid;

		/* HRABAK keypress stuff  so all funcptr() calls can be void */
		winwidget w;
		feh_menu_item *selected_item;
		feh_menu      *selected_menu;
		char *no_cap;               /* ALL empty captions point here */
		char *old_cap;              /* saves original for edit aborts*/
		char *bind_path;            /* path to feh_bindings.map      */

		/* iterations() first_time settings.  Set inside the doit_again loop */
		int doit_again;             /* controls loop inside main()    */
		int xfd;
		int fdsize;
		double pt;

		/* allow one function ptr to be passed back to main().  Allows return
		 * to feh_main_iteration() and then FORCING the next func call */
		StubPtr ptr;                /* used just for &stub_move_mode_toggle  */
		feh_node *tdselected;       /* currently selected thumbnail */

		/* these three were globals declared in menu.c */
		struct {
			Window cover ;
			feh_menu *root;
			feh_menu_list *list;
		} mnu;

		/* short int *LLMDflags; */      /* cast to set all LLMD.nd flags to zero */

};    /* end of fgv */

/* trick to reset the 16 x 1 bit-field flags in the md->rn->nd */
#define RESET_RN_FLAGS(md)  fgv.LLMDflags=(short int *) &md->rn->nd; *fgv.LLMDflags=0 ;fgv.LLMDflags=NULL;
#define RESET_CN_FLAGS(md)  fgv.LLMDflags=(short int *) &md->cn->nd; *fgv.LLMDflags=0 ;fgv.LLMDflags=NULL;

/*  moved from filelist.h */

/* changed "feh_file" to feh_data" cause the word "file" had LOTS of diff meanings */
typedef struct __feh_data feh_data;
typedef struct __feh_data_info feh_data_info;

struct __feh_data {
	char *filename;
	char *caption;
	char *name;           /* ptr to basename inside filename */
	char *ext;            /* ptr to '.' inside name */

	feh_data_info *info;       /* only set info stuff when needed */
	#ifdef HAVE_LIBEXIF
		ExifData *ed;
	#endif
};

struct __feh_data_info {
	int width;
	int height;
	int size;
	int pixels;
	unsigned char has_alpha;
	char *format;
	char *extension;
	time_t mtime;         /* added jan 7, 2013 */
};

/* May 2012 HRABAK changes */

/* The new feh linkedList structure with a circular link back to the rootnode */

struct __feh_node  {

	void      *data;           /* holds the feh_data struct, plus feh_file_info */
	feh_node  *prev;
	feh_node  *next;

	struct{
		unsigned list0    : 1;            /* list numbers 0-9 for the default     */
		unsigned list1    : 1;            /* action keys 0-9.  Even if no other   */
		unsigned list2    : 1;            /* actions are defined, pressing  0-9   */
		unsigned list3    : 1;            /* keys will add this pic to that list  */
		unsigned list4    : 1;            /* number.  Any lists flagged this way  */
		unsigned list5    : 1;            /* will be written at the end,          */
		unsigned list6    : 1;            /* regardless of the --nowrite-filelist */
		unsigned list7    : 1;            /* option.  Use this to split ONE LONG  */
		unsigned list8    : 1;            /* list into a set of smaller lists.    */
		unsigned list9    : 1;
		unsigned tagged   : 1;            /* if ANY list[0-9] got tagged          */
		unsigned delete   : 1;            /* flagged to delete from OS filesystem */
		unsigned exists   : 1;            /* for thumbnails                       */
		unsigned lchange  : 1;            /* has the LL list changed? if so, write it out */
		unsigned dirty    : 1;            /* do we need to recnt()?               */
		unsigned flag01   : 1;            /* flag01 is available for use          */
		unsigned short x     ;  /* : 16;  */  /* thumbnail upper left x in pixels */
		unsigned short y     ;  /* : 16;  */  /* thumbnail upper left y in pixels */
		unsigned short wide  ;  /* : 16;  */  /* pic (or thumb) width in pixels   */
		unsigned short high  ;  /* : 16;  */  /* pic (or thumb) height in pixels  */
		unsigned short cnt   ;  /* : 16;  */  /* stores the (1 of 56) relative position cnts of each node*/
	} nd;                                /* nd stands for node_data             */
/*	} nd={ .list0=0 }; */             /* attempt to zero out all flags       */

} ;

typedef struct __llMetaData LLMD;    /* LLMD stands for linked list MetaData*/

/* note:  the old filelist_len is in rn.cnt */
struct __llMetaData{
	feh_node * rn;                     /* the root_node (ie the 0th node )     */
	feh_node * cn;                     /* the current node (picture) we are on */
	feh_node * tn;                     /* just a tmp node for assigns and loops*/
} ;


/* As of May 2012, HRABAK cut out ALL the gib_style stuff and replaced it
 * with an array of 5 styles (feh_style struct) inside __fehoptions.
 */
typedef struct _style feh_style;

struct _style {
	struct {
			short r,g,b,a;
			short x_off;
			short y_off;
	} bg;
	struct {
			short r,g,b,a;
			short x_off;
			short y_off;
	} fg;
} ;

/* as of Apr 2013 HRABAK encapsulated all feh font stuff into this */
typedef struct _feh_font feh_font;

struct _feh_font {
		unsigned char wide ;
		unsigned char high ;
		Imlib_Font fn;
		char *name;
};        /* four instances: fn_title,fn_dflt,fn_menu,fn_fulls */

enum misc_flags {

/* controls freeing behavior in feh_ll... functions */
	FREE_NO = 0,    FREE_YES ,

/* used for feh_move_node_to_remove_list(), feh_thumbnail_mark_removed() */
	DELETE_NO = 0,  DELETE_YES , DELETE_ALL , WIN_MAINT_NO , WIN_MAINT_DO,

/* enums to support the array of feh_style structs stored inside fehoptions */
	STYLE_MENU =0,  STYLE_WHITE, STYLE_RED, STYLE_YELLOW, STYLE_CNT ,

/* moved from feh.h.  Used to be one of the many "MODE" types */
	STATE_NORMAL = 0, STATE_PAN, STATE_ZOOM, STATE_ROTATE,STATE_BLUR, STATE_NEXT,

/* these are now the "REAL" mode names */
	MODE_SLIDESHOW = 0, MODE_MULTIWINDOW, MODE_INDEX, MODE_THUMBNAIL, MODE_LOADABLES,
	MODE_UNLOADABLES, MODE_LIST,  MODE_MOVE , MOVE_RESTORE_ORIGINAL,

/* used to set opt.zoom_mode */
	ZOOM_MODE_FILL = 1, ZOOM_MODE_MAX,

/* used for slideshow_change_image(w, SLIDE_PREV, RENDER_YES)  */
	SLIDE_PREV=0 , SLIDE_NEXT , SLIDE_RAND, SLIDE_FIRST,
	SLIDE_LAST, SLIDE_JUMP_FWD, SLIDE_JUMP_BACK, SLIDE_NO_JUMP,

/* used to set (opt.flg.text_bg == TEXT_BG_CLEAR) */
	TEXT_BG_CLEAR = 0, TEXT_BG_TINTED,

/* used to set opt.flg.image_bg = IMAGE_BG_CHECKS */
	IMAGE_BG_CHECKS = 0, IMAGE_BG_BLACK, IMAGE_BG_WHITE,

/* used to control add_file_to_filelist_recursively(argv[optind++], FILELIST_FIRST) */
	FILELIST_FIRST = 0, FILELIST_CONTINUE, FILELIST_LAST ,

/* used to control opt.sort = SORT_FILENAME */
	SORT_NONE = 0, SORT_NAME, SORT_FILENAME, SORT_WIDTH,
	SORT_HEIGHT,  SORT_PIXELS, SORT_SIZE, SORT_FORMAT,

/* for winwidget_render_image(winwidget w, int resize, int force_alias, int sanitize) */
	RESIZE_NO=0,   RESIZE_YES,
	ALIAS_NO=0,    ALIAS_YES,
	SANITIZE_NO=0, SANITIZE_YES,

/* used for slideshow_change_image(w, SLIDE_NEXT, RENDER_YES) */
	RENDER_NO=0, RENDER_YES,

/* used for feh_reload_image(w, RESIZE_YES, FORCE_NEW_YES )   */
	FORCE_NEW_NO=0, FORCE_NEW_YES

};   /* end of enum misc_flags */

/* from timers.h */
typedef struct __fehtimer _fehtimer;
typedef _fehtimer *fehtimer;

struct __fehtimer {
	char *name;
	void (*func) (void *data);
	void *data;
	double in;
	char just_added;
	fehtimer next;
};

/*
 * from utils.h
 * May 2012 HRABAK replaced all the gib_hash() stuff with bhs_hash() stuff.
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


/* May 2012 HRABAK rewrote the feh_string_split() to eliminate
 * gib_list linked list requirement, using an ArrayofWordPointers (awp[])
 * awp[0].linenum               saves total num of words stored
 * awp[0].word;                 ptr to the start of the WHOLE string
 */

typedef struct _wd wd;

struct _wd {                  /* wd stands for word data                   */
	short int linenum;          /* one-based                                 */
	short int wordnum;          /* one-based, wordcnt inside this line       */
	short int len;              /* one-based of this one word                */
	char * word;                /* ptr to the start of the word              */
};

#define FIRST_AWP_GUESS 20      /* initial size for the awp[] array        */

/* used to rewrite feh_wrap_string() using an ArrayofLinePointers (alp[])  */
typedef union _ld ld;          /* 'ld' stands for line data                */

union _ld {
	struct {
		int tot_lines;              /* saves total num of lines in array       */
		int maxwide;                /* max value of ld.wide for all lines      */
		int tothigh;                /* summed height of all lines              */
		} L0;
	struct {
		/* the index in the alp[] array  IS the linenum, so  one-based         */
		short int wide;             /* font-sized width of the whole line      */
		short int high;             /* font-sized height of the whole line     */
		short int len;              /* one-based                               */
		char * line;                /* ptr to the start of the line            */
		} L1;                       /* use "L" not "l" cause "l1" is confusing */
};

#define FIRST_ALP_GUESS 5       /* initial size of the alp[] array         */

/* from thumbnails.h */

/* Jun 24, 2012 No longer used.  All stuffed into feh_node.nd.
 * #define FEH_THUMB(l) ((feh_thumbnail *) l)
 * typedef struct thumbnail {...} feh_thumbnail;
 */

typedef struct thumbmode_data {

	Imlib_Image im_main;        /* base image which all thumbnails are rendered on */
	Imlib_Image im_bg;          /* background for the thumbnails */
	/*feh_node *selected;*/     /* currently selected thumbnail (moved to fgv )    */

	int wide, high, bg_w, bg_h; /* dimensions of the window and bg image */

	int thumb_tot_h;            /* total space needed for a thumbnail including description */
	int text_area_w, text_area_h; /* space needed for thumbnail text description */

	int max_column_w;           /* FIXME: description */
	int thumbnailcount;         /* total thumbs loaded.  Used for info display   */

	int cache_dim;              /* 128 = 128x128 ("normal"), 256 = 256x256 ("large") */
	char *cache_dir;            /* "normal"/"large" (.thumbnails/...) */
	unsigned char vertical;     /* FIXME: vertical in what way? */
	unsigned char gutter;       /* the gutter spacing between thumbs in im_main */
	unsigned char type;         /* controls WIN_TYPE_SINGLE vs THUMBNAIL */
	unsigned char trans_bg;     /* flag for transparent background              */

	                            /* im_main is just im_wide x im_wide             */
	/* int im_wide; */          /* this was always just td.wide                  */
	/* int im_high; */          /* this was always just td.high                  */

} thumbmode_data;


/* June 2012 structs used in the newkeyev keypress processing */

/*  controls the order in which the binding arrays are written to the .map file */
enum bs_which {MENU_PTR ,  MOVE_PTR, FEH_PTR , SUB_PTR };        /* for the bs.last[which] */

/* structure of the funcptr array elements ... */
typedef struct {
	unsigned int symstate;      /* 0xffff <<8 + state, so 0xffff0d */
	StubPtr ptr;                /* a ptr to the stubfunc, like &stub_action_0*/
} Key2Action;

struct _binding_state {
	Key2Action *last[3];        /* tailend of menu_ feh_ and move_actions[] */
	int acnt[4];                /* saves final array cnt for menu_, feh_ move_ and subset*/
	int ss_last;                /* one less than ACT_SUBSET_SIZE */
};


typedef struct {
	char * action;
	int len;
	StubPtr ptr;
} Ptrmap;


typedef struct {
	unsigned int symstate;      /* 0xffff <<8 + state, so 0xffff0d */
	StubPtr ptr;                /* a ptr to the stubfunc, like &stub_action_0*/
	int which;                  /* points to index of feh_actions[] */
} SubsetAction;

#endif      /* STRUCTS_H */
