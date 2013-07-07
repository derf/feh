/* menu.c

Copyright (C) 1999-2003 Tom Gilbert.
Copyright (C) 2010-2011 Daniel Friesel.
Copyright (C) 2013      Christopher Hrabak

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

* Apr 2013 HRABAK changes read all menu items from ONE definition table
* "mnu_defs" (this file).  Gone are the specific feh_menu_init_????()
* functions, replaced by feh_menu_init_from_mnu_defs( mnu_code), which
* recursively loops thru the table to gen the requested menu (mnu_code) and
* all its sub_menus.  Menus (and sub_menus) are no longer "named" with text
* strings, but use an enum code found in mnu_lvl.
* To add a new menu, do the following...
*
*  1) Create a new enum mnu_lvl code for your new menu.
*  2) Create new enum mnu_lvl code for any new sub_menus.  Note that
*     all sub_menu codes follow the SUB_MNU_LVL_BEG code.
*  3) Create new enum mnu_lvl code for any new CB_action codes.  Note that
*     all CB_-type codes follow the CB_LVL_BEG code.
*  4) Add new switch/case blocks in feh_menu_cb() for your new CB_-type codes.
*
* One final note on winwidgets.  feh_menu's used to store the winwidget w
* as a struct member.  But the m->w was not assigned until
* winwidget_show_menu() was called.  And you had to force any submenus to
* inherit that m->w setting.  No more.  fgv.w is a global winwidget used for
* all events, keypress, motion, buttonPress etc.

*/

#include "feh.h"
#include "thumbnail.h"
#include "wallpaper.h"
#include "winwidget.h"
#include "feh_ll.h"
#include "options.h"
/*
 * All three moved inside of fgv.mnu
		Window menu_cover = 0;
		feh_menu *menu_root = NULL;
		static feh_menu_list *menus = NULL;
*/
static void feh_menu_cb(feh_menu * m, feh_menu_item * i);
static void feh_menu_cb_opt_fullscreen(feh_menu * m, feh_menu_item * i);
static feh_menu_item *feh_menu_add_entry(feh_menu * m, unsigned int idx,
                                   unsigned short data , int call_special);
static void feh_menu_special( feh_menu_item *mi ,
                              unsigned int defs_idx );
static void feh_menu_load_desktops(feh_menu * m, feh_menu_item *parent_mi,
                              unsigned int defs_idx );
static char * get_desk_ptr( int setflag);
static feh_menu * feh_menu_new( enum mnu_lvl mnu_code );
static void feh_menu_show_at_submenu(feh_menu * m, feh_menu * parent_m, feh_menu_item * i);
static void feh_menu_entry_get_size(feh_menu_item * i, int *w, int *h);
static void feh_menu_calc_size(feh_menu * m);
static void feh_menu_draw_item(feh_menu_item * i, Imlib_Image im, int ox, int oy);
static void feh_menu_redraw(feh_menu * m);
static void feh_menu_move(feh_menu * m, int x, int y);
static void feh_menu_draw_to_buf(feh_menu * m, Imlib_Image im, int ox, int oy);
static void feh_menu_draw_menu_bg(feh_menu * m, Imlib_Image im, int ox, int oy);
static void feh_menu_draw_submenu_at(int x, int y, Imlib_Image dst, int ox, int oy);
static void feh_menu_draw_separator_at(int x, int y, int w, int h, Imlib_Image dst, int ox, int oy);
static void feh_menu_item_draw_at(int x, int y, int w, int h, Imlib_Image dst, int ox, int oy, int selected);
static void feh_menu_draw_toggle_at(int x, int y, int w, int h, Imlib_Image dst, int ox, int oy, int on);
static void feh_menu_free(feh_menu * m);
static char * feh_nofile_check( feh_menu * m );
static int feh_menu_exclude( unsigned int defs_idx);
static void ck_mode_after_sort(winwidget w);

typedef struct _mnu_items {
	enum mnu_lvl     code;        /* code "name" of the menu   */
	enum mnu_lvl     sub_code;    /* submenu or CB_action code */
	enum mnu_lvl     flag;        /* is this a sub menu?       */
} mnu_items;

mnu_items mnu_defs[] = {
	/*   code         ,   sub_code         , flag          */
	{ MENU_MAIN       , SUB_FILE          , IS_SUB_MENU      } ,
	{ MENU_MAIN       , SUB_SORT          , IS_SUB_MENU      } ,
	{ MENU_MAIN       , SUB_INFO          , IS_SUB_MENU      } ,
	{ MENU_MAIN       , SUB_OPTIONS       , IS_SUB_MENU      } ,
	{ MENU_MAIN       , CB_SEPARATOR      , IS_CB_ACTION     } ,
	{ MENU_MAIN       , SUB_HELP          , IS_SUB_MENU      } ,
	{ MENU_MAIN       , CB_CLOSE          , IS_CB_ACTION     } ,
	{ MENU_MAIN       , CB_EXIT           , IS_CB_ACTION     } ,

	/* all the submenus follow   */
	{ SUB_FILE        , CB_RESET          , IS_CB_ACTION     } ,
	{ SUB_FILE        , CB_FIT            , IS_CB_ACTION     } ,
	{ SUB_FILE        , CB_RELOAD         , IS_CB_ACTION     } ,
	{ SUB_FILE        , CB_SAVE_IMAGE     , IS_CB_ACTION     } ,
	{ SUB_FILE        , CB_SAVE_FILELIST  , IS_CB_ACTION     } ,
	{ SUB_FILE        , SUB_EDIT_IN_PLACE , IS_SUB_MENU      } ,
	{ SUB_FILE        , SUB_BACKGROUND    , IS_SUB_MENU      } ,
	{ SUB_FILE        , CB_SEPARATOR      , IS_CB_ACTION     } ,
	{ SUB_FILE        , CB_REMOVE         , IS_CB_ACTION     } ,
	{ SUB_FILE        , SUB_DELETE        , IS_SUB_MENU      } ,

	/* the old common() set */
	{ SUB_SORT        , CB_SORT_FILENAME  , IS_CB_ACTION     } ,
	{ SUB_SORT        , CB_SORT_IMAGENAME , IS_CB_ACTION     } ,
	{ SUB_SORT        , CB_SORT_FILESIZE  , IS_CB_ACTION     } ,
	{ SUB_SORT        , CB_SORT_RANDOMIZE , IS_CB_ACTION     } ,
	{ SUB_SORT        , CB_SORT_REVERSE   , IS_CB_ACTION     } ,

	{ SUB_DELETE      , CB_DEL_OK         , IS_CB_ACTION     } ,

	{ SUB_EDIT_IN_PLACE , CB_ROTATE90CW   , IS_CB_ACTION     } ,
	{ SUB_EDIT_IN_PLACE , CB_ROTATE180    , IS_CB_ACTION     } ,
	{ SUB_EDIT_IN_PLACE , CB_ROTATE90CCW  , IS_CB_ACTION     } ,

	{ SUB_BACKGROUND  , CB_BG_TILED       ,  IS_CB_ACTION    } ,
	{ SUB_BACKGROUND  , CB_BG_SCALED      ,  IS_CB_ACTION    } ,
	{ SUB_BACKGROUND  , CB_BG_CENTERED    ,  IS_CB_ACTION    } ,
	{ SUB_BACKGROUND  , CB_BG_FILLED      ,  IS_CB_ACTION    } ,

	/* This next set used only when multiple desktops.
	 * special() swaps in SUB_BG_DESK for SUB_BACKGROUND  */
	{ SUB_BG_DESK     , SUB_BG_TILED       ,  IS_SUB_MENU   } ,
	{ SUB_BG_DESK     , SUB_BG_SCALED      ,  IS_SUB_MENU   } ,
	{ SUB_BG_DESK     , SUB_BG_CENTERED    ,  IS_SUB_MENU   } ,
	{ SUB_BG_DESK     , SUB_BG_FILLED      ,  IS_SUB_MENU   } ,

	/* then this set is repeated once for each desktop_num  */
	{ SUB_BG_TILED    , CB_BG_TILED         ,  IS_CB_ACTION  } ,
	{ SUB_BG_SCALED   , CB_BG_SCALED        ,  IS_CB_ACTION  } ,
	{ SUB_BG_CENTERED , CB_BG_CENTERED      ,  IS_CB_ACTION  } ,
	{ SUB_BG_FILLED   , CB_BG_FILLED        ,  IS_CB_ACTION  } ,

	/* SUB_INFO is programmatically filled in */
	{ SUB_INFO        , CB_IMAGE_FILE        , IS_CB_ACTION  } ,
	{ SUB_INFO        , CB_IMAGE_SIZE        , IS_CB_ACTION  } ,
	{ SUB_INFO        , CB_IMAGE_DIMS        , IS_CB_ACTION  } ,
	{ SUB_INFO        , CB_IMAGE_TYPE        , IS_CB_ACTION  } ,
	{ SUB_INFO        , CB_IMAGE_DATE        , IS_CB_ACTION  } ,
	{ SUB_INFO        , CB_LIST0_9FLAGS      , IS_CB_ACTION  } ,

	/* SUB_OPTIONS are programmatically toggled */
	{ SUB_OPTIONS     , CB_OPT_AUTO_ZOOM     , IS_CB_ACTION  } ,
	{ SUB_OPTIONS     , CB_OPT_FREEZE_WINDOW , IS_CB_ACTION  } ,
	{ SUB_OPTIONS     , CB_OPT_FULLSCREEN    , IS_CB_ACTION  } ,
	{ SUB_OPTIONS     , CB_OPT_BIG_MENUS     , IS_CB_ACTION  } ,
	{ SUB_OPTIONS     , CB_SEPARATOR         , IS_CB_ACTION  } ,
	{ SUB_OPTIONS     , CB_OPT_DRAW_FILENAME , IS_CB_ACTION  } ,
	{ SUB_OPTIONS     , CB_OPT_DRAW_ACTIONS  , IS_CB_ACTION  } ,
	{ SUB_OPTIONS     , CB_OPT_KEEP_HTTP     , IS_CB_ACTION  } ,

	{ SUB_HELP        , CB_HELP_FEH          , IS_CB_ACTION  } ,
	{ SUB_HELP        , CB_HELP_KEYS         , IS_CB_ACTION  } ,

	/* the rest of these are bogus menus, like for yes/no dialogs */
	{ MENU_MM_ESC_OK  , SUB_MM_ESC           , IS_SUB_MENU   } ,
	{ MENU_MM_ESC_OK  , CB_MM_DROPIT         , IS_CB_ACTION  } ,
	{ MENU_MM_ESC_OK  , CB_MM_BACK           , IS_CB_ACTION  } ,

	{ SUB_MM_ESC      , CB_MM_ESC_IGNORE     , IS_CB_ACTION  } ,
	{ SUB_MM_ESC      , CB_MM_ESC_KEEP       , IS_CB_ACTION  } ,

};

#define MNU_DEFS_COUNT      ( sizeof( mnu_defs )/ sizeof( mnu_items ) )

/* presize an array to hold the text used for each menu item display.  Note:
 * the "+ 10" makes 10 additional slots that will be used to hold the
 * (up-to) 10 desktops for setting backgrounds.
 */
char * mnu_txt[ MNU_TOP_LVL_END + 10];


/* **************************
 *   Begin functions        *
 * **************************/
void feh_menu_init_mnu_txt( void ){
		/* called once from main().  Holds all the text strings to
		 * display inside menus.  Internal references just to
		 * eliminate duplicate strings.
		 */

	mnu_txt[ CB_CLOSE ]                = "Close" ;
	mnu_txt[ CB_DEL_OK ]               = "Confirm" ;
	mnu_txt[ CB_ROTATE90CW ]           = "Rotate 90 CW" ;
	mnu_txt[ CB_ROTATE180 ]            = "Rotate 180" ;
	mnu_txt[ CB_ROTATE90CCW ]          = "Rotate 90 CCW" ;
	mnu_txt[ CB_EXIT ]                 = "Exit" ;
	mnu_txt[ CB_FIT ]                  = "Resize Window" ;
	mnu_txt[ CB_OPT_AUTO_ZOOM ]        = "Auto-Zoom" ;
	mnu_txt[ CB_OPT_DRAW_ACTIONS ]     = "Draw Actions" ;
	mnu_txt[ CB_OPT_DRAW_FILENAME ]    = "Draw Filename" ;
	mnu_txt[ CB_OPT_FREEZE_WINDOW ]    = "Freeze Window Size" ;
	mnu_txt[ CB_OPT_FULLSCREEN ]       = "Fullscreen" ;
	mnu_txt[ CB_OPT_BIG_MENUS ]        = "Large Menus" ;
	mnu_txt[ CB_OPT_KEEP_HTTP ]        = "Keep HTTP Files" ;
	mnu_txt[ CB_RELOAD ]               = "Reload" ;
	mnu_txt[ CB_REMOVE ]               = "Hide" ;
	mnu_txt[ CB_RESET ]                = "Reset" ;
	mnu_txt[ CB_SAVE_FILELIST ]        = "Save List" ;
	mnu_txt[ CB_SAVE_IMAGE ]           = "Save Image" ;
	mnu_txt[ CB_SORT_FILENAME ]        = "By File Name" ;
	mnu_txt[ CB_SORT_FILESIZE ]        = "By File Size" ;
	mnu_txt[ CB_SORT_IMAGENAME ]       = "By Image Name" ;
	mnu_txt[ CB_SORT_RANDOMIZE ]       = "Randomize" ;
	mnu_txt[ CB_SORT_REVERSE ]         = "Reverse" ;
	mnu_txt[ SUB_BACKGROUND ]          = ERR_BACKGROUND;
	mnu_txt[ SUB_BG_DESK ]             = mnu_txt[ SUB_BACKGROUND ];
	mnu_txt[ SUB_DELETE ]              = "Delete";
	mnu_txt[ SUB_EDIT_IN_PLACE ]       = "Edit in Place";
	mnu_txt[ CB_BG_TILED ]             = "Set Tiled" ;
	mnu_txt[ CB_BG_SCALED ]            = "Set Scaled" ;
	mnu_txt[ CB_BG_CENTERED ]          = "Set Centered" ;
	mnu_txt[ CB_BG_FILLED ]            = "Set Filled" ;
	mnu_txt[ SUB_BG_TILED   ]          = mnu_txt[ CB_BG_TILED  ];
	mnu_txt[ SUB_BG_SCALED  ]          = mnu_txt[ CB_BG_SCALED ];
	mnu_txt[ SUB_BG_CENTERED ]         = mnu_txt[ CB_BG_CENTERED ];
	mnu_txt[ SUB_BG_FILLED ]           = mnu_txt[ CB_BG_FILLED ];
	mnu_txt[ SUB_FILE ]                = "File";
	mnu_txt[ SUB_INFO ]                = "Image Info";
	mnu_txt[ SUB_OPTIONS ]             = "Options";
	mnu_txt[ SUB_SORT ]                = "Sort List";
	/* mnu_txt[ CB_SEPARATOR ]  */   /* no text, just a CB_-type flag */
	mnu_txt[ SUB_MM_ESC]               = "Escape";
	mnu_txt[ CB_MM_ESC_IGNORE]         = "Ignore Move";
	mnu_txt[ CB_MM_ESC_KEEP ]          = "Keep move-list";
	mnu_txt[ CB_MM_DROPIT ]            = "Dropit";
	mnu_txt[ CB_MM_BACK ]              = "Return to MoveMode";

	mnu_txt[ SUB_HELP ]                = "Help";
	mnu_txt[ CB_HELP_FEH ]             = "feh";
	mnu_txt[ CB_HELP_KEYS ]            = "key bindings";

}  /* end of init_mnu_txt */

static int feh_menu_exclude( unsigned int defs_idx){
		/* teh old feh menu system had several diff menu sets.  Ther eis ONLY
		 * MUNU_MAIN now and any eclusions to that menu set is done here all
		 * dependant on MODE_xxx codes.  returns 1 to indicate exclude this one.
		 */
	#define MACRO_CODE             mnu_defs[ defs_idx ].code
	#define MACRO_SUB_CODE         mnu_defs[ defs_idx ].sub_code

	if ( opt.flg.mode == MODE_THUMBNAIL ){
		if ( !fgv.tdselected ){
			/* exclude the whole SUB_FILE menu cause no pic is selected */
			if ( MACRO_SUB_CODE == SUB_FILE ||
			     MACRO_SUB_CODE == SUB_INFO )
				return 1;
		} else {
			/* we have a pic selected  */
			if ( MACRO_SUB_CODE == SUB_EDIT_IN_PLACE  ||
			     MACRO_SUB_CODE == SUB_BACKGROUND  )
				return 1;
		}
	} else if ( opt.flg.mode == MODE_MULTIWINDOW ){
		if ( MACRO_SUB_CODE == SUB_EDIT_IN_PLACE ||
		     MACRO_SUB_CODE == SUB_BACKGROUND    ||
		     MACRO_SUB_CODE == SUB_SORT    )
			return 1;
	}
	return 0;
}   /* end of feh_menu_exclude() */


/* recursive */
int feh_menu_init_from_mnu_defs( enum mnu_lvl mnu_code , int first_time ){
		/* winwidget_show_menu() calls this routine to gen the MENU_MAIN, as
		 * this is the ONLY menu used in feh.  caller requests an enum mnu_code
		 * and this routine loads it from the mnu_defs table.  feh destroys all
		 * menus after each use so each recursive call to this routine just adds
		 * to the list.  This routine genns all items for the requested mnu_code,
		 * plus ALL sub_menus it depends on. returns 1 on error.
		 *
		 * feh_menu *m(s) are now "named" as enum codes, not text.  Same with
		 * all sub_menu "names".  Therefor searching for a matching sub_menu
		 * no longer requires a strcmp(), just a simple int == test.
		 * The label "mnu" is purposeful to indicate this is not the old
		 * feh menu logic.
		 */

	enum mnu_lvl sub_code, flag;
	unsigned int i, foundit=0;
	feh_menu_item *mi;
	feh_menu *m;

	m = feh_menu_new( mnu_code );

	if (first_time ){
		fgv.mnu.root = m;
		first_time = 0;
	}
	/* all menus and sub_menus are defined in the mnu_defs table */
	for ( i=0 ; i < MNU_DEFS_COUNT ; i++ ){
		if ( mnu_defs[i].code == mnu_code ){
			/* mnu_code repeats for sub_code (item) */
			foundit  = 1;
			sub_code = mnu_defs[i].sub_code;
			flag     = mnu_defs[i].flag;

			if ( feh_menu_exclude( i ) )
				continue;   /* ignore this one and get the next */

			mi = feh_menu_add_entry(m, i , 0 , 1);

			/* add_entry() can change (redefine) mnu_defs[] codes so read
			 * codes from the mi (not mnu_defs) incase they changed */
			sub_code = mi->sub_code;
			flag     = mi->flag;

			if ( flag == IS_SUB_MENU  ) {
				/* recursively build that sub_menu now */
				if ( feh_menu_init_from_mnu_defs( sub_code, 0 ) == 1 )
						eprintf("%s menu load for sub_code %d !",ERR_FAILED, sub_code);
			} else if ( flag == IS_CB_ACTION ) {
				/* end of the line, as this is the action to perform.
				 * BUT!  This subset requires the "Desktop 0" lists */
				if ( (mi->code ==  SUB_BG_TILED    )
					|| (mi->code ==  SUB_BG_SCALED   )
					|| (mi->code ==  SUB_BG_CENTERED )
					|| (mi->code ==  SUB_BG_FILLED   ) ){
					feh_menu_load_desktops(m, mi, i );
				}
				continue;
			} else {
				/* somebody goofed in building the mnu_defs table */
				eprintf("Error found in mnu_defs table!");
			}
		}  /* mnu_code not found so loop again */
	}

return !foundit;

}  /* end of feh_menu_init_from_mnu_defs() */

static void feh_menu_special( feh_menu_item *mi ,
                              unsigned int defs_idx ){
		/* This allows special treatment for any code in mnu_defs[] table.
		 * 1) all toggle logic
		 * 2) Image Info ( including running feh_ll_load_data_info() )
		 * 3) Swapping SUB_BG_DESK for SUB_BACKGROUND when multi desktops
		 * The net result is to fill in diff text in the mnu_txt[]
		 * array so add_entry() can still do its thing unchanged.
		 */
	enum mnu_lvl sub_code = mnu_defs[ defs_idx ].sub_code;

	if ( sub_code == CB_OPT_AUTO_ZOOM ){
		mi->is_toggle = TRUE;
		MENU_ITEM_TOGGLE_SET(mi, opt.zoom_mode);
	} else if ( sub_code == CB_OPT_FREEZE_WINDOW ){
		mi->is_toggle = TRUE;
		MENU_ITEM_TOGGLE_SET(mi, opt.geom_flags);
	} else if ( sub_code == CB_OPT_FULLSCREEN ){
		mi->is_toggle = TRUE;
		MENU_ITEM_TOGGLE_SET(mi, fgv.w->full_screen);
	} else if ( sub_code == CB_OPT_BIG_MENUS ){
		mi->is_toggle = TRUE;
		MENU_ITEM_TOGGLE_SET(mi, opt.flg.big_menus);
	} else if ( sub_code == CB_OPT_DRAW_FILENAME ){
		mi->is_toggle = TRUE;
		MENU_ITEM_TOGGLE_SET(mi, opt.flg.draw_filename);
	} else if ( sub_code == CB_OPT_DRAW_ACTIONS ){
		mi->is_toggle = TRUE;
		MENU_ITEM_TOGGLE_SET(mi, opt.flg.draw_actions);
	} else if ( sub_code == CB_OPT_KEEP_HTTP ){
		mi->is_toggle = TRUE;
		MENU_ITEM_TOGGLE_SET(mi, opt.flg.keep_http);
	} else if ( sub_code == CB_IMAGE_FILE ){
		mnu_txt[ CB_IMAGE_FILE ] =  mobs(1);
		STRCAT_2ITEMS( mnu_txt[ CB_IMAGE_FILE ] , "  File : ",
		              NODE_NAME(fgv.w->node));
	} else if ( (sub_code == CB_IMAGE_SIZE ) &&
		          (NODE_INFO(fgv.w->node)) ){
		mnu_txt[ CB_IMAGE_SIZE ] = mobs(1);
		sprintf( mnu_txt[ CB_IMAGE_SIZE ] , " Size: %dKb",
		         NODE_INFO(fgv.w->node)->size / SIZE_1024 );

	} else if ( (sub_code == CB_IMAGE_DIMS ) &&
		          (NODE_INFO(fgv.w->node)) ){
		mnu_txt[ CB_IMAGE_DIMS ] = mobs(1);
		sprintf( mnu_txt[ CB_IMAGE_DIMS ], "Dims: %dx%d",
		         NODE_INFO(fgv.w->node)->width,
		         NODE_INFO(fgv.w->node)->height);

	} else if ( (sub_code == CB_IMAGE_TYPE ) &&
		          (NODE_INFO(fgv.w->node)) ){
		mnu_txt[ CB_IMAGE_TYPE ] = mobs(1);
		STRCAT_2ITEMS(mnu_txt[ CB_IMAGE_TYPE ], "Type: ",
		         NODE_INFO(fgv.w->node)->format);

	} else if ( (sub_code == CB_IMAGE_DATE ) &&
		          (NODE_INFO(fgv.w->node)) ){
		char *tmp, *buffer;                 /* Date/time added Jan 2013 */
		buffer = mnu_txt[ CB_IMAGE_DATE ] = mobs(1);
		memset( buffer , 0 , MOBS_NSIZE(1) );
		/*ZERO_OUT( buffer );*/             /* can use cause sizeof(buf)==4 */
		tmp = buffer + 70;                  /* just a scratch area */
		strcpy(tmp,ctime(&NODE_INFO(fgv.w->node)->mtime));  /* Tue Jul  7 00:58:18 2009\n */
		strncpy(buffer,tmp+11,9);           /* 00:58:18   */
		strncat(buffer,tmp+0,11);           /* Tue Jul  7 */
		strncat(buffer,tmp+20,4);           /* 2009       */

	} else if ( (sub_code == CB_LIST0_9FLAGS ) ){
		char *buffer;                       /* LIST[0-9] flag display added May 2013 */
		short int *flag, bit_pos;
		buffer = mnu_txt[ CB_LIST0_9FLAGS ] = mobs(1);
		strcpy( buffer, "Flags " );
		memset( buffer+5 , '-' , 21 );      /* "Flags -------------------"*/
		buffer[28] = buffer[29] = '\0';
		flag = (short int *) &fgv.w->node->nd;    /* all cn->nd.list[0-9]  */
		for ( bit_pos = 0; bit_pos < 10 ; bit_pos ++){
			if (*flag & (1<<bit_pos))         /* "Flags ----2---4---6-7---9"*/
				(buffer+8)[ bit_pos * 2] = bit_pos["0123456789"];
		}

	} else if (( sub_code == SUB_INFO ) &&
		        (!NODE_INFO(fgv.w->node)) ) {
		/* force an image reload each time if thumbnail mode */
		feh_ll_load_data_info(NODE_DATA(fgv.w->node),
		         opt.flg.mode==MODE_THUMBNAIL ? NULL :fgv.w->im );

	} else if (sub_code == SUB_BACKGROUND ) {
		/* if we have multi desktops, then use SUB_BG_DESK sub menu instead.
		 * I have always wanted to use this obfuscated code idea and this
		 * was a perfect fit.
		 */
		int num_desks = feh_wm_get_num_desks(); /*+11;*/ /*for testing a max of 10 */
		if ( num_desks > 1 ) {
			int idx=0;
			char *ptr = get_desk_ptr(1);      /* set it */

			mi->sub_code = SUB_BG_DESK;
			/* make all the extra "Desktop [0-9]" text entries
			 * for feh_menu_load_desktops() to use */
			for (idx=0; idx<num_desks; idx++){
				ptr = stpcpy( ptr, "Desktop ");
				ptr[0] = idx["0123456789"];
				ptr[1]=ptr[2]=ptr[3]='\0';
				ptr +=3;
			}
		}
	}

}  /* end of feh_menu_special() */

static void feh_menu_load_desktops(feh_menu * m,
                              feh_menu_item *parent_mi,
                              unsigned int defs_idx ){
		/* Only called when multiple desktops are found.
		 * Every request gets the same list of num_desks items.  Note:
		 * Save the orig CB code into o_sub_code and then store a
		 * bogus ptr into sub_code so it points to Desktop [0-9]  */
	feh_menu_item *mi;
	int data_idx=0;
	char *ptr = get_desk_ptr(0);
	do {
		if ( data_idx == 0 ){                        /* re-text the parent item */
			parent_mi->o_sub_code = parent_mi->sub_code;
			parent_mi->sub_code   = MNU_TOP_LVL_END + data_idx;
			mnu_txt[ parent_mi->sub_code ] = ptr;      /* pointing to Desktop [0-9] */
		} else {
			mi = feh_menu_add_entry(m, defs_idx , data_idx, 0 );
			mi->o_sub_code = mi->sub_code;      /* save the real one  */
			mi->sub_code   = MNU_TOP_LVL_END + mi->data;   /* bogus code   */
			mnu_txt[ mi->sub_code ] = ptr;      /* pointing to Desktop [0-9] */
		}
		while ( *ptr++ != '\0' );
		ptr++;                  /* step past the double NULL byte */
		data_idx++;
	} while ( *ptr != '\0' );

}   /* end of feh_menu_load_desktops() */

static char * get_desk_ptr( int setflag){
	/* just to share desk_ptr between two routines */
	static char *desk_ptr;
	if (setflag )
		desk_ptr = mobs(2);
return desk_ptr;
}

static feh_menu * feh_menu_new( enum mnu_lvl mnu_code ){
		/* creates and returns ptr to a new feh_menu *m "named" mnu_code.
		 */

	feh_menu *m;
	XSetWindowAttributes attr;
	feh_menu_list *l;
	static Imlib_Image bg = NULL;
	static Imlib_Border border;

	m = (feh_menu *) ecalloc(sizeof(feh_menu));

	attr.backing_store = NotUseful;
	attr.override_redirect = True;
	attr.colormap = fgv.cm;
	attr.border_pixel = 0;
	attr.background_pixmap = None;
	attr.save_under = False;
	attr.do_not_propagate_mask = True;

	m->win = XCreateWindow(
			fgv.disp, fgv.root, 1, 1, 1, 1, 0, fgv.depth, InputOutput, fgv.vis,
			CWOverrideRedirect | CWSaveUnder | CWBackingStore
			| CWColormap | CWBackPixmap | CWBorderPixel | CWDontPropagate, &attr);
	XSelectInput(fgv.disp, m->win,
			ButtonPressMask | ButtonReleaseMask | EnterWindowMask
			| LeaveWindowMask | PointerMotionMask | ButtonMotionMask);

	/* using calloc() zeros out all members */
	m->code = mnu_code;         /* the menu "name" */
	m->needs_redraw = 1;

	l = emalloc(sizeof(feh_menu_list));
	l->menu = m;
	l->next = fgv.mnu.list;
	fgv.mnu.list = l;

	if (!bg) {
		feh_load_image_char(&bg, opt.menu_bg);
		if (bg) {
			border.left = border.right = border.top = border.bottom
				= 4;
			imlib_context_set_image(bg);
			imlib_image_set_border(&border);
		}
	}

	if (bg)
		m->bg = gib_imlib_clone_image(bg);

	return m;

}   /* end of feh_menu_new() */

static feh_menu_item *feh_menu_add_entry(feh_menu * m, unsigned int idx,
                                   unsigned short data, int call_special ){
		/* all mnu_def codes get passed as a sub_code.  Only if it is a CB_-type
		* code will it be used as the final action to perform rather than a submenu
		*/
	feh_menu_item *mi, *ptr;

	mi = (feh_menu_item *) emalloc(sizeof(feh_menu_item));

	mi->next      = NULL;
	mi->prev      = NULL;
	mi->state     = MENU_ITEM_STATE_NORMAL;

	mi->code      = mnu_defs[idx].code;
	mi->sub_code  = mnu_defs[idx].sub_code;
	mi->flag      = mnu_defs[idx].flag;
	mi->data      = data;
	mi->is_toggle = FALSE;
	mi->o_sub_code= 0;

	/* now add in all the program gennerated stuff, noting that
	 * it COULD swap in replacements for sub_code and flag.
	 * BACKGROUND is swapped for SUB_BG_DESK when multi desktops.
	 */
	if ( call_special )
		feh_menu_special( mi , idx );

	if (!m->items)
		m->items = mi;
	else {
		for (ptr = m->items; ptr; ptr = ptr->next) {
			if (!ptr->next) {
				ptr->next = mi;
				mi->prev = ptr;
				break;
			}
		}
	}
	m->calc = 1;
	return(mi);

}  /* end of feh_menu_add_entry() */

static void feh_menu_free(feh_menu * m)
{
	feh_menu_item *i;
	feh_menu_list *l, *pl = NULL;

	XDestroyWindow(fgv.disp, m->win);
	if (m->pmap)
		XFreePixmap(fgv.disp, m->pmap);
	if (m->updates)
		imlib_updates_free(m->updates);
	for (i = m->items; i;) {
		feh_menu_item *ii;

		ii = i;
		i = i->next;
		free(ii);
	}

	for (l = fgv.mnu.list; l; l = l->next) {
		if (l->menu == m) {
			if (pl)
				pl->next = l->next;
			else
				fgv.mnu.list = l->next;
			free(l);
			break;
		}
		pl = l;
	}
	if (m->bg)
		gib_imlib_free_image_and_decache(m->bg);

	free(m);

	return;
}

feh_menu_item *feh_menu_find_selected(feh_menu * m)
{
	feh_menu_item *i;

	D(("menu %p\n", m));

	for (i = m->items; i; i = i->next) {
		if (MENU_ITEM_IS_SELECTED(i))
			return(i);
	}
	return(NULL);
}

feh_menu_item *feh_menu_find_selected_r(feh_menu * m, feh_menu ** parent)
{
	feh_menu_item *i, *ii;
	feh_menu *mm;

	D(("menu %p\n", m));

	for (i = m->items; i; i = i->next) {
		if (MENU_ITEM_IS_SELECTED(i)) {
			if (parent)
				*parent = m;
			return(i);
		} else if (i->sub_code) {
			mm = feh_menu_find(i->sub_code);
			if (mm) {
				ii = feh_menu_find_selected_r(mm, parent);
				if (ii)
					return(ii);
			}
		}
	}
	if (parent)
		*parent = m;
	return(NULL);
}

void feh_menu_select_next(feh_menu * selected_menu, feh_menu_item * selected_item)
{
	feh_menu_item *i;
	if (!selected_item) {
		/* jump to first item, select it */
		feh_menu_select(selected_menu, selected_menu->items);
	} else {
		i = selected_item;
		while (1) {
			i = i->next;
			if (!i)
				i = selected_menu->items;
			if ( i->sub_code ) {
				break;
			}
		}
		feh_menu_deselect_selected(selected_menu);
		feh_menu_select(selected_menu, i);
	}
}

void feh_menu_select_prev(feh_menu * selected_menu, feh_menu_item * selected_item)
{
	feh_menu_item *i, *ii;
	if (!selected_item) {
		/* jump to last item, select it */
		for (i = selected_menu->items; i->next; i = i->next);
		feh_menu_select(selected_menu, i);
	} else {
		i = selected_item;
		while (1) {
			i = i->prev;
			if (!i) {
				i = selected_menu->items;
				for (ii = selected_menu->items; ii->next; ii = ii->next);
				i = ii;
			}
			if ( i->sub_code ) {
				break;
			}
		}
		feh_menu_deselect_selected(selected_menu);
		feh_menu_select(selected_menu, i);
	}
}

void feh_menu_select_parent(feh_menu * selected_menu)
{
	feh_menu *m;
	feh_menu_item *i;
	/* find the parent menu's item which refers to this menu's name */
	if (selected_menu->prev) {
		m = selected_menu->prev;
		for (i = m->items; i; i = i->next) {
			if ( i->sub_code == selected_menu->code )
				break;
		}
		/* shouldn't ever happen */
		if (i == NULL)
			i = m->items;
		feh_menu_deselect_selected(selected_menu);
		feh_menu_select(m, i);
	}
}

void feh_menu_select_submenu(feh_menu * selected_menu)
{
	if (selected_menu->next) {
		feh_menu_deselect_selected(selected_menu);
		feh_menu_select(selected_menu->next, selected_menu->next->items);
	}
}

void feh_menu_item_activate(feh_menu * m, feh_menu_item * i)
{
	/* watch out for this. I put it this way around so the menu
	   goes away *before* we perform the action, if we start
	   freeing menus on hiding, it will break ;-) */
	if ((i) && (i->sub_code) && (i->flag == IS_CB_ACTION) ) {
		feh_menu_hide(fgv.mnu.root, False);
		feh_main_iteration(0);
		feh_menu_cb(m, i);
		feh_menu_free(m);
	}
}

feh_menu_item *feh_menu_find_at_xy(feh_menu * m, int x, int y)
{
	feh_menu_item *i;

	D(("looking for menu item at %d,%d\n", x, y));
	for (i = m->items; i; i = i->next) {
		if (XY_IN_RECT(x, y, i->x, i->y, i->w, i->h)) {
			D(("Found an item\n"));
			return(i);
		}
	}
	D(("didn't find an item\n"));
	return(NULL);
}

void feh_menu_deselect_selected(feh_menu * m)
{
	feh_menu_item *i;

	if (!m)
		return;

	i = feh_menu_find_selected(m);
	if (i) {
		D(("found a selected menu, deselecting it\n"));
		MENU_ITEM_SET_NORMAL(i);
		m->updates = imlib_update_append_rect(m->updates, i->x, i->y, i->w, i->h);
		m->needs_redraw = 1;
	}
	return;
}

void feh_menu_select(feh_menu * m, feh_menu_item * i)
{
	MENU_ITEM_SET_SELECTED(i);
	m->updates = imlib_update_append_rect(m->updates, i->x, i->y, i->w, i->h);
	m->needs_redraw = 1;
	if (m->next) {
		m->next->prev = NULL;
		feh_menu_hide(m->next, TRUE);
		m->next = NULL;
	}
	if (i->sub_code) {
		feh_menu *mm;

		mm = feh_menu_find(i->sub_code);
		if (mm)
			feh_menu_show_at_submenu(mm, m, i);
	}
	return;
}

void feh_menu_show_at(feh_menu * m, int x, int y)
{
	if (m->calc)
		feh_menu_calc_size(m);
	if (!fgv.mnu.cover) {
		XSetWindowAttributes attr;

		D(("creating menu cover window\n"));
		attr.override_redirect = True;
		attr.do_not_propagate_mask = True;
		fgv.mnu.cover = XCreateWindow(
				fgv.disp, fgv.root, 0, 0, fgv.scr->width,
				fgv.scr->height, 0, 0, InputOnly, fgv.vis,
				CWOverrideRedirect | CWDontPropagate, &attr);
		XSelectInput(fgv.disp, fgv.mnu.cover,
				KeyPressMask | ButtonPressMask |
				ButtonReleaseMask | EnterWindowMask |
				LeaveWindowMask | PointerMotionMask | ButtonMotionMask);

		XRaiseWindow(fgv.disp, fgv.mnu.cover);
		XMapWindow(  fgv.disp, fgv.mnu.cover);
		fgv.mnu.root = m;
		XUngrabPointer(fgv.disp, CurrentTime);
		XSetInputFocus(fgv.disp, fgv.mnu.cover, RevertToPointerRoot, CurrentTime);
	}
	m->visible = 1;
	XMoveWindow(fgv.disp, m->win, x, y);
	m->x = x;
	m->y = y;
	XRaiseWindow(fgv.disp, m->win);
	feh_menu_redraw(m);
	XMapWindow(fgv.disp, m->win);
	return;
}

void feh_menu_show_at_xy(feh_menu * m, winwidget w, int x, int y)
{
	if (!m)
		return;

	if (m->calc)
		feh_menu_calc_size(m);
	m->w = w;
	if ((x + m->wide) > fgv.scr->width)
		x = fgv.scr->width - m->wide;
	if ((y + m->high) > fgv.scr->height)
		y = fgv.scr->height - m->high;

#if 0
/* #ifdef HAVE_LIBXINERAMA */
/* this doesn't work correctly :( -- pabs */
	if (opt.flg.xinerama && xinerama_screens) {
		if ((x + m->wide) > xinerama_screens[xinerama_screen].width)
			x = xinerama_screens[xinerama_screen].width - m->wide;
		if ((y + m->high) > xinerama_screens[xinerama_screen].height)
			y = xinerama_screens[xinerama_screen].height - m->high;

	}
#endif				/* HAVE_LIBXINERAMA */

	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	feh_menu_move(m, x, y);
	feh_menu_show(m);
	return;
}

static void feh_menu_show_at_submenu(feh_menu * m, feh_menu * parent_m, feh_menu_item * i)
{
	int mx, my;

	if (!m)
		return;

	if (m->calc)
		feh_menu_calc_size(m);
	mx = parent_m->x + parent_m->wide;
	my = parent_m->y + i->y - FEH_MENU_PAD_TOP;
	m->w = parent_m->w;
	parent_m->next = m;
	m->prev = parent_m;
	feh_menu_move(m, mx, my);
	feh_menu_show(m);
	return;
}

static void feh_menu_move(feh_menu * m, int x, int y)
{
	if (!m)
		return;

	if (m->visible)
		XMoveWindow(fgv.disp, m->win, x, y);

	m->x = x;
	m->y = y;
	return;
}

void feh_menu_slide_all_menus_relative(int dx, int dy)
{
	int i;
	feh_menu_list *m;
	double vector_len = 0;
	int stepx = 0;
	int stepy = 0;

	vector_len = sqrt(dx * dx + dy * dy);
	if (vector_len) {
		if (dx)
			stepx = rint(dx / vector_len);

		if (dy)
			stepy = rint(dy / vector_len);

	}
	for (i = 0; i < vector_len; i++) {
		for (m = fgv.mnu.list; m; m = m->next) {
			if (m->menu->visible)
				feh_menu_move(m->menu, m->menu->x + stepx, m->menu->y + stepy);

		}
		XWarpPointer(fgv.disp, None, None, 0, 0, 0, 0, stepx, stepy);
	}
	return;
}

void feh_menu_hide(feh_menu * m, int free_menu ){
		/* fonts for menus are set once upon entry, and returned
		 * to the fn_dflt here when menus are closed and freed
		 */

	if (!m->visible)
		return;
	if (m->next) {              /* this takes care of freeing fgv.mnu.list LL */
		m->next->prev = NULL;
		feh_menu_hide(m->next, free_menu);
		m->next = NULL;
	}
	if (m == fgv.mnu.root) {
		if (fgv.mnu.cover) {
			D(("DESTROYING menu cover\n"));
			XDestroyWindow(fgv.disp, fgv.mnu.cover);
			fgv.mnu.cover = 0;
		}
		fgv.mnu.root = NULL;
		opt.fn_ptr = &opt.fn_dflt;
	}
	m->visible = 0;
	XUnmapWindow(fgv.disp, m->win);
	/* no menus are gen'd on the fly, so never free them here */
	feh_menu_deselect_selected(m);

	return;
}

void feh_menu_show(feh_menu * m)
{
	if (!m)
		return;
	feh_menu_show_at(m, m->x, m->y);
	return;
}

static void feh_menu_entry_get_size(feh_menu_item * i, int *w, int *h){
		/* sub_code holds either a submenu or action to display
		 */
	int tw, th;
	char *text = mnu_txt[ i->sub_code ];

	if (text) {
		gib_imlib_get_text_size( text, &opt.style[ STYLE_MENU ], &tw, &th, IMLIB_TEXT_TO_RIGHT);
		*w = tw + FEH_MENUITEM_PAD_LEFT + FEH_MENUITEM_PAD_RIGHT;
		*h = th + FEH_MENUITEM_PAD_TOP + FEH_MENUITEM_PAD_BOTTOM;
	} else {
		*w = FEH_MENUITEM_PAD_LEFT + FEH_MENUITEM_PAD_RIGHT;
		*h = FEH_MENUITEM_PAD_TOP + FEH_MENUITEM_PAD_BOTTOM;
	}

	return;
}

static void feh_menu_calc_size(feh_menu * m)
{
	int prev_w, prev_h;
	feh_menu_item *i;
	int j = 0, count = 0, max_w = 0, max_h = 0, next_w = 0;
	int toggle_w = 1;

	prev_w = m->wide;
	prev_h = m->high;
	m->calc = 0;

	for (i = m->items; i; i = i->next) {
		int w, h;

		feh_menu_entry_get_size(i, &w, &h);
		if (w > max_w)
			max_w = w;
		if (h > max_h)
			max_h = h;
		if (i->sub_code) {
			next_w = FEH_MENU_SUBMENU_W;
			if (FEH_MENU_SUBMENU_H > max_h)
				max_h = FEH_MENU_SUBMENU_H;
		}
		if (i->is_toggle) {
			toggle_w = FEH_MENU_TOGGLE_W + FEH_MENU_TOGGLE_PAD;
			if (FEH_MENU_TOGGLE_H > max_h)
				max_h = FEH_MENU_TOGGLE_H;
		}
		count++;
	}

	m->high = FEH_MENU_PAD_TOP;
	for (i = m->items; i; i = i->next) {
		i->x = FEH_MENU_PAD_LEFT;
		i->y = m->high;
		i->w = max_w + toggle_w + next_w;
		i->toggle_x = 1;
		i->text_x = i->toggle_x + toggle_w;
		i->sub_x = i->text_x + max_w;
		if (mnu_txt[ i->sub_code ])          /* the text to display */
			i->h = max_h;
		else
			i->h = FEH_MENU_SEP_MAX_H;
		m->high += i->h;
		j++;
	}
	m->high += FEH_MENU_PAD_BOTTOM;
	m->wide = next_w + toggle_w + max_w + FEH_MENU_PAD_LEFT + FEH_MENU_PAD_RIGHT;

	if ((prev_w != m->wide) || (prev_h != m->high)) {
		if (m->pmap)
			XFreePixmap(fgv.disp, m->pmap);
		m->pmap = 0;
		m->needs_redraw = 1;
		XResizeWindow(fgv.disp, m->win, m->wide, m->high);
		m->updates = imlib_update_append_rect(m->updates, 0, 0, m->wide, m->high);
	}
	D(("menu size calculated. w=%d h=%d\n", m->wide, m->high));

	/* Make sure bg is same size */
	if (m->bg) {
		int bg_w, bg_h;

		bg_w = gib_imlib_image_get_width(m->bg);
		bg_h = gib_imlib_image_get_height(m->bg);

		if (m->wide != bg_w || m->high != bg_h) {
			Imlib_Image newim = feh_imlib_image_make_n_fill_text_bg(m->wide, m->high,0);
			D(("resizing bg to %dx%d\n", m->wide, m->high));

			gib_imlib_blend_image_onto_image(newim, m->bg,
			        0, 0, 0, bg_w, bg_h, 0, 0,
			        m->wide, m->high, 0, 0, 1);
			gib_imlib_free_image_and_decache(m->bg);
			m->bg = newim;
		}
	}

	return;
}

static void feh_menu_draw_item(feh_menu_item * i, Imlib_Image im, int ox, int oy)
{
	char *text = mnu_txt[ i->sub_code ];

	D(("drawing item %p (text %s)\n", i, text));

	if ( i->sub_code == CB_SEPARATOR) {
		D(("separator item\n"));
		feh_menu_draw_separator_at(i->x, i->y, i->w, i->h, im, ox, oy);

	} else if (text) {
		D(("text %sselected item\n",MENU_ITEM_IS_SELECTED(i)? " ":"un"));
		feh_menu_item_draw_at(i->x, i->y, i->w, i->h,
		           im, ox, oy, MENU_ITEM_IS_SELECTED(i));

		/* draw text */
		feh_imlib_text_draw(im, &opt.style[ STYLE_MENU ],
		                    i->x - ox + i->text_x,
		                    i->y - oy + FEH_MENUITEM_PAD_TOP,
		                    text, IMLIB_TEXT_TO_RIGHT);

		if (i->flag == IS_SUB_MENU ) {
			D(("submenu item\n"));
			feh_menu_draw_submenu_at(i->x + i->sub_x,
						 i->y +
						 FEH_MENUITEM_PAD_TOP +
						 ((i->h -
						   FEH_MENUITEM_PAD_TOP -
						   FEH_MENUITEM_PAD_BOTTOM
						   -
						   FEH_MENU_SUBMENU_H) /
						  2), im, ox, oy);
		}
		if (i->is_toggle) {
			D(("toggleable item\n"));
			feh_menu_draw_toggle_at(i->x + i->toggle_x,
						i->y +
						FEH_MENUITEM_PAD_TOP +
						((i->h -
						  FEH_MENUITEM_PAD_TOP -
						  FEH_MENUITEM_PAD_BOTTOM -
						  FEH_MENU_TOGGLE_H) / 2),
						FEH_MENU_TOGGLE_W, FEH_MENU_TOGGLE_H, im, ox, oy, MENU_ITEM_IS_ON(i));
		}
	}
	return;
}

static void feh_menu_redraw(feh_menu * m)
{
	Imlib_Updates u, uu;

	if ((!m->needs_redraw) || (!m->visible) || (!m->updates))
		return;
	m->needs_redraw = 0;
	if (!m->pmap)
		m->pmap = XCreatePixmap(fgv.disp, m->win, m->wide, m->high, fgv.depth);
	XSetWindowBackgroundPixmap(fgv.disp, m->win, m->pmap);

	u = imlib_updates_merge_for_rendering(m->updates, m->wide, m->high);
	m->updates = NULL;
	if (u) {
		D(("I have updates to render\n"));
		for (uu = u; u; u = imlib_updates_get_next(u)) {
			int x, y, w, h;
			Imlib_Image im;

			imlib_updates_get_coordinates(u, &x, &y, &w, &h);
			D(("update coords %d,%d %d*%d\n", x, y, w, h));
			im = feh_imlib_image_make_n_fill_text_bg( w, h, 0 );
			feh_menu_draw_to_buf(m, im, x, y);
			gib_imlib_render_image_on_drawable(m->pmap, im, x, y, 1, 0, 0);
			gib_imlib_free_image(im);
			XClearArea(fgv.disp, m->win, x, y, w, h, False);
		}
		imlib_updates_free(uu);
	}
	return;
}

feh_menu *feh_menu_find(enum mnu_lvl mnu_code )
{
	feh_menu_list *l;

	for (l = fgv.mnu.list; l; l = l->next) {
		if ( l->menu->code == mnu_code )
			return(l->menu);
	}
	return(NULL);
}

static void feh_menu_draw_to_buf(feh_menu * m, Imlib_Image im, int ox, int oy)
{
	feh_menu_item *i;
	int w, h;

	w = gib_imlib_image_get_width(im);
	h = gib_imlib_image_get_height(im);

	feh_menu_draw_menu_bg(m, im, ox, oy);

	for (i = m->items; i; i = i->next) {
		if (RECTS_INTERSECT(i->x, i->y, i->w, i->h, ox, oy, w, h))
			feh_menu_draw_item(i, im, ox, oy);
	}
	return;
}

static void feh_menu_draw_menu_bg(feh_menu * m, Imlib_Image im, int ox, int oy)
{
	int w, h;

	w = gib_imlib_image_get_width(im);
	h = gib_imlib_image_get_height(im);

	if (m->bg)
		gib_imlib_blend_image_onto_image(im, m->bg, 0, ox, oy, w, h, 0, 0, w, h, 0, 0, 0);
	else
		gib_imlib_image_fill_rectangle(im, 0, 0, w, h, 205, 203, 176, 255);

	return;
}

static void feh_menu_draw_toggle_at(int x, int y, int w, int h, Imlib_Image dst, int ox, int oy, int on)
{
	x -= ox;
	y -= oy;
	if (on)
		gib_imlib_image_fill_rectangle(dst, x, y, w, h, 0, 0, 0, 255);
	else
		gib_imlib_image_draw_rectangle(dst, x, y, w, h, 0, 0, 0, 255);
	return;
}

static void feh_menu_draw_submenu_at(int x, int y, Imlib_Image dst, int ox, int oy)
{
	ImlibPolygon poly;

	x -= ox;
	y -= oy;

	imlib_context_set_image(dst);
	poly = imlib_polygon_new();
	imlib_polygon_add_point(poly, x + 2, y + 5);
	imlib_polygon_add_point(poly, x + 5, y + 7);
	imlib_polygon_add_point(poly, x + 2, y + 11);
	imlib_context_set_color(0, 0, 0, 60);
	imlib_image_fill_polygon(poly);
	imlib_polygon_free(poly);


	poly = imlib_polygon_new();
	imlib_polygon_add_point(poly, x, y + 3);
	imlib_polygon_add_point(poly, x + 3, y + 6);
	imlib_polygon_add_point(poly, x, y + 9);
	imlib_context_set_color(0, 0, 0, 255);
	imlib_image_fill_polygon(poly);
	imlib_polygon_free(poly);

	return;
}

static void feh_menu_draw_separator_at(int x, int y, int w, int h, Imlib_Image dst, int ox, int oy)
{
	gib_imlib_image_fill_rectangle(dst, x - ox + 2, y - oy + 2, w - 4, h - 4, 0, 0, 0, 255);
	return;
}

static void feh_menu_item_draw_at(int x, int y, int w, int h, Imlib_Image dst, int ox, int oy, int selected)
{
	imlib_context_set_image(dst);
	if (selected)
		gib_imlib_image_fill_rectangle(dst, x - ox, y - oy, w, h, 255, 255, 255, 178);
	return;
}

void feh_raise_all_menus(void)
{
	feh_menu_list *l;

	for (l = fgv.mnu.list; l; l = l->next) {
		if (l->menu->visible)
			XRaiseWindow(fgv.disp, l->menu->win);
	}
	return;
}

void feh_redraw_menus(void)
{
	feh_menu_list *l;

	for (l = fgv.mnu.list; l; l = l->next) {
		if (l->menu->needs_redraw)
			feh_menu_redraw(l->menu);
	}

	return;
}

feh_menu *feh_menu_get_from_window(Window win)
{
	feh_menu_list *l;

	for (l = fgv.mnu.list; l; l = l->next)
		if (l->menu->win == win)
			return(l->menu);
	return(NULL);
}

static void feh_menu_cb_opt_fullscreen(feh_menu * m, feh_menu_item * i)
{
	int curr_screen = 0;

	MENU_ITEM_TOGGLE(i);
	if (MENU_ITEM_IS_ON(i))
		m->w->full_screen = TRUE;
	else
		m->w->full_screen = FALSE;

#ifdef HAVE_LIBXINERAMA
	if (opt.flg.xinerama && fgv.xinerama_screens) {
		int i, rect[4];

		winwidget_get_geometry(m->w, rect);
		for (i = 0; i < fgv.num_xinerama_screens; i++) {
			fgv.xinerama_screen = 0;
			if (XY_IN_RECT(rect[0], rect[1],
				       fgv.xinerama_screens[i].x_org,
				       fgv.xinerama_screens[i].y_org,
				       fgv.xinerama_screens[i].width,
				       fgv.xinerama_screens[i].height)) {
				curr_screen = fgv.xinerama_screen = i;
				break;
			}

		}
		if (getenv("XINERAMA_SCREEN"))
			curr_screen = fgv.xinerama_screen =
				atoi(getenv("XINERAMA_SCREEN"));
	}
#endif				/* HAVE_LIBXINERAMA */

	winwidget_destroy_xwin(m->w);
	winwidget_create_window(m->w, m->w->im_w, m->w->im_h);

	winwidget_render_image(m->w, 1, 0, SANITIZE_NO);
	winwidget_show(m->w);

#ifdef HAVE_LIBXINERAMA
	/* if we have xinerama and we're using it, then full screen the window
	 * on the head that the window was active on */
	if (m->w->full_screen == TRUE && opt.flg.xinerama && fgv.xinerama_screens) {
		fgv.xinerama_screen = curr_screen;
		winwidget_move(m->w, fgv.xinerama_screens[curr_screen].x_org,
		               fgv.xinerama_screens[curr_screen].y_org);
	}
#endif				/* HAVE_LIBXINERAMA */
}

static char * feh_nofile_check( feh_menu * m ){
	/* service func for the CB_BG_settings.  Returns either an abs path or NULL */
	if ( (opt.flg.mode == MODE_SLIDESHOW) || (opt.flg.mode == MODE_MULTIWINDOW) )
		return ( feh_absolute_path(NODE_FILENAME(m->w->node)) );
	else
		return NULL;

}

static void feh_menu_cb(feh_menu * m, feh_menu_item * i) {
		/* HRABAK.  Because menus are table driven, I needed a way to handle
		 * setting BG on multiple desktops.  The solution was to stuff a bogus
		 * sub_code for each desktop to get the right text to display, then
		 * tfall back the original "o_sub_code" which is the action to
		 * perform here.
		 */

		enum mnu_lvl action = i->sub_code;       /* will always be a CB_ code */
		unsigned short data = i->data;

	if ( i->o_sub_code )        /* restore the original CB_BG_xxx code */
		action = i->o_sub_code;

	switch (action) {
		/* all the CB_BG_xxxx tests handle both file and no_file versions */
		case CB_BG_TILED:
		case CB_BG_SCALED:
		case CB_BG_CENTERED:
		case CB_BG_FILLED:
			feh_wm_set_bg( feh_nofile_check(m), m->w->im, action, data, 0);
			break;
		case CB_CLOSE:
			winwidget_destroy(m->w);
			break;
		case CB_EXIT:
			winwidget_destroy_all();
			break;
		case CB_RESET:
			if (m->w->has_rotated) {
				m->w->im_w = gib_imlib_image_get_width(m->w->im);
				m->w->im_h = gib_imlib_image_get_height(m->w->im);
				winwidget_resize(m->w, m->w->im_w, m->w->im_h);
			}
			winwidget_reset_image(m->w);
			winwidget_render_image(m->w, 1, 0, SANITIZE_NO);
			break;
		case CB_RELOAD:
			if ( m->w->type != WIN_TYPE_THUMBNAIL )
				feh_reload_image(m->w, RESIZE_NO, FORCE_NEW_YES );
			break;
		case CB_REMOVE:
			if (opt.flg.mode == MODE_THUMBNAIL )
				feh_thumbnail_mark_removed(m->w->node, DELETE_NO );
			feh_move_node_to_remove_list(m->w, DELETE_NO, WIN_MAINT_DO);
			break;
		case CB_DEL_OK:
			if (opt.flg.mode == MODE_THUMBNAIL )
				feh_thumbnail_mark_removed(m->w->node, DELETE_YES );
			feh_move_node_to_remove_list(m->w, DELETE_YES, WIN_MAINT_DO);
			break;
		case CB_SORT_FILENAME:
			feh_ll_qsort( feh_md , feh_cmp_filename);
			ck_mode_after_sort(m->w);
			break;
		case CB_SORT_IMAGENAME:
			feh_ll_qsort( feh_md , feh_cmp_name);
			ck_mode_after_sort(m->w);
			break;
		case CB_SORT_FILESIZE:
			feh_ll_qsort( feh_md , feh_cmp_size);
			ck_mode_after_sort(m->w);
			break;
		case CB_SORT_RANDOMIZE:
			feh_ll_randomize( feh_md );
			ck_mode_after_sort(m->w);
			break;
		case CB_SORT_REVERSE:
			feh_ll_reverse( feh_md );
			ck_mode_after_sort(m->w);
			break;
		case CB_FIT:
			winwidget_size_to_image(m->w);
			break;
		case CB_ROTATE90CW:
			feh_edit_inplace(m->w, 1 );
			break;
		case CB_ROTATE180:
			feh_edit_inplace(m->w, 2 );
			break;
		case CB_ROTATE90CCW:
			feh_edit_inplace(m->w, 3 );
			break;
		case CB_SAVE_IMAGE:
			slideshow_save_image(m->w);
			break;
		case CB_SAVE_FILELIST:
			stub_save_filelist();
			break;
		case CB_OPT_DRAW_FILENAME:
			MENU_ITEM_TOGGLE(i);
			opt.flg.draw_filename = !opt.flg.draw_filename;
			winwidget_rerender_all(0);
			break;
		case CB_OPT_DRAW_ACTIONS:
			MENU_ITEM_TOGGLE(i);
			opt.flg.draw_actions = !opt.flg.draw_actions;
			winwidget_rerender_all(0);
			break;
		case CB_OPT_KEEP_HTTP:
			MENU_ITEM_TOGGLE(i);
			if (MENU_ITEM_IS_ON(i))
				opt.flg.keep_http = TRUE;
			else
				opt.flg.keep_http = FALSE;
			break;
		case CB_OPT_FREEZE_WINDOW:
			MENU_ITEM_TOGGLE(i);
			if (MENU_ITEM_IS_ON(i)) {
				opt.geom_flags = (WidthValue | HeightValue);
				opt.geom_w = m->w->wide;
				opt.geom_h = m->w->high;
			} else {
				opt.geom_flags = 0;
			}
			break;
		case CB_OPT_FULLSCREEN:
			feh_menu_cb_opt_fullscreen(m, i);
			break;
		case CB_OPT_BIG_MENUS:
			MENU_ITEM_TOGGLE(i);
			opt.flg.big_menus = !opt.flg.big_menus;
			break;
		case CB_OPT_AUTO_ZOOM:
			MENU_ITEM_TOGGLE(i);
			if (MENU_ITEM_IS_ON(i))
				opt.zoom_mode = ZOOM_MODE_MAX;
			else
				opt.zoom_mode = 0;
			winwidget_rerender_all(1);
			break;
		/* all the CB_MM_xxx stuff can only be active in mm */
		case CB_MM_DROPIT:
		case CB_MM_ESC_IGNORE:
		case CB_MM_ESC_KEEP:
			move_mode_exit(  action );
			break;
		case CB_HELP_FEH:
			help4feh( FEH_GENERAL_HELP );
			break;
		case CB_HELP_KEYS:
			help4feh( FEH_BINDINGS_HELP );
			break;
		case CB_MM_BACK:                   /* ignored and return to mm */
		default:                           /* just to quiet the compiler */
			break;
	}
	return;
}

static void ck_mode_after_sort(winwidget w){
		/* if MODE_THUMB, then rebuild all thumbs */
	if ( opt.flg.mode == MODE_THUMBNAIL ){
		fgv.doit_again = 1;
		fgv.tdselected = NULL;
		winwidget_destroy_all();
	} else            /* slideshow-type mode */
		slideshow_change_image(w,
		      opt.flg.jump_on_resort ? SLIDE_FIRST : SLIDE_NO_JUMP,
		      RENDER_YES);
}

