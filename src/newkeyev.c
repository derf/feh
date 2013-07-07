/* newkeyev.c

Copyright (C) 1999-2003 Tom Gilbert.
Copyright (C) 2010-2011 Daniel Friesel.
Copyright (C) 2012      Christopher Hrabak   LastUpdt Apr 12, 2013 cah

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

 * This is the replacement for the original keyevents.c module.
 * As of Mar 2012, HRABAK rewrote the keybindings and keypress logic.
 * See feh_init.h and feh_init.c for more detail.
 * The feh_event_handle_keypress() function (here) has been modified
 * to support this new logic flow.
 *
 * This file contains all the functions that begin with stub_<name> where
 * <name> is the name of the actions listed in the feh_init.h file. This new
 * logic stores pointers to these stub_<name> functions inside one of three
 * keybinding arrays; either menu_, move_ or feh_actions[].  Each runtime keypress
 * is unique at this granualarity and calls the appropriate stub_function directly.
 *
 * Most of the real function calls were just repackaged from the old
 * feh_event_handle_keypress() routine and wrapped in the new stub_<name>.
 *
 */

#include "feh.h"
#include "newkeyev.h"
#include "feh_ll.h"
#include "options.h"
#include "winwidget.h"
#include "thumbnail.h"

/* Note: Mode keys look like C-Left for <Control>+Left.
 * C for <Control>, A for <Alt>, S for <Shift>.
 * Combos work ... C-A-S-Left for <Control>+<Alt>+<Shift>+Left
 * all key codes for caption editing are hard coded; ie NOT modifiable
 * all menu keys get bound into menu_actions[], all move_mode bindings
 * are in  move_actions[] and the rest are in feh_actions[].
 *
 * HRABAK wrote two diff funcs to search ???_actions[] arrays.  One uses the
 * stock C-lang bsearch() call.  The other works like a one-node b-tree.
 * #define either BSEARCH_SEARCH or BTREE_SEARCH (which is 2x faster)
 * BTREE_SEARCH gives CPU time of 46.00 sec for 1069999893 searches.
 * vs 101.95 sec for 1069999893 searches using bsearch().  The 46sec is
 * more than 5x faster than the original feh_event_handle_keypress() search.
 */

#define BTREE_SEARCH


/* ***************** *
 * Global variables  *
 * ***************** */

/* save all the binding array states here, bs is short for bindings_state */
struct _binding_state bs = { .ss_last=ACT_SUBSET_SIZE-1, .acnt[3]=ACT_SUBSET_SIZE } ;
SubsetAction act_subset[ ACT_SUBSET_SIZE  ];                  /* subset of feh_actions */
Key2Action *menu_actions, *feh_actions, *move_actions;        /* 3 main binding arrays */

/* all the static knowledge about the move state is stored here.  OTTR stands
 * for OneToTheRight, as this is the pic - OTTR - of the one pic (or the list)
 * you are about to move.  This lets me ret2pos after the move.  mov_md is a LL
 * of pix to be moved. ret2List[] (a list of feh_nodes) remembers the OTTRs
 * of previous moves so you can ret2pos (fwd or back) at anytime.
 */

struct {
	unsigned char isDropped;    /* flag if they try to get out of move_mode without dropping */
	feh_node *cn;               /* set the  FIRST time we enter move_mode                    */
	feh_node *OTTR;             /* only set when move_to() is initiated                      */
	feh_node *ret2List[32];     /* the OTTR stack, with "top" being the LAST entry, "0" based*/
	LLMD *mov_md;               /* the linked list of pix to be moved                        */
	winwidget prev_w;           /* saves the winwidget the moment we enter move_mode         */
}  mvInfo;

/* for the ret2stack() logic */
enum ret2 { RET2PUSH, RET2GET, RET2FWD, RET2BAK, RET2IGNORE };

/* The FEP structure (stands for FiveEasyPieces) holds a thumbnail image
 * of the five pix to be displayed in the move_mode (mm) window.  All thumbs
 * are in a square fmt, calc'd to fit the mm window.  The upper left corner
 * x/y of each square (Zone) is in pixels.  init_FEP() calcs these to fit.
 */

typedef struct {
	int x;                      /* top left x corner of image, in pixels       */
	feh_node *n;                /* feh_md node of pic in this Zone             */
} Zone;

struct {                      /* Store the full image data for all five pics */
	struct {
		/* these get set inside init_FEP() */
		/* default font for mm_image is opt.fn_fep */
		/* char fs_norm;  */      /* font size (normal) assuming a FS screen     */
		/* char fs_med;   */      /* font size (medium) if screen is smaller     */
		/* char fs_tiny;  */      /* font size (smallest) if screen is way small */

		/* these get set during runtime             */
		int y4c;                  /* upper left y pos for center pic, in pixels  */
		int y;                    /* upper left y pos for all other pix          */
		int wide;                    /* thumb wide(th) pixels (they are square)     */
		/*int high;*/             /* h(eight) of thumb in pixels.  same as wide  */
		int max_w;                /* maxsize (width)  of a fullscreen (Xwindow)  */
		int max_h;                /* maxsize (height) of a fullscreen (Xwindow)  */
		Imlib_Image rn_image;     /* the data "image" for the md->rn "pic"       */
		Imlib_Image com_image;    /* one common image to blend onto the mm_image */
		Imlib_Image mm_image;     /* the composite (FS) image to blend INTO      */
		winwidget w;              /* FS winwidget window to hold the mm_image    */
		/*Window   win;*/         /* FS window to hold the mm_image              */
	} dflt;                     /* default settings for all FEP members        */
	Zone ll;                    /* ll  for the pic left of left of center      */
	Zone  l;                    /* l   for the pic left of center              */
	Zone  c;                    /* c   for the center pic                      */
	Zone  r;                    /* r   for the pic right of center             */
	Zone rr;                    /* rr  for the pic right of right of center    */
	Zone tl;                    /* top left holds a help cheat sheet           */
	Zone tr;                    /* top right displays the mov_md LL contents   */
} FEP={ .dflt.w=NULL };
/*} FEP;  */                  /* FEP stands for FiveEasyPieces               */

/* Non-stub_<name> type function prototypes ... */
static void edit_caption( KeySym keysym, int state );
static void feh_event_invoke_action( unsigned char action);
static void ret2stack( enum ret2  pp, enum ret2  direction );
static void fillFEPnodes( feh_node *ottr );
static void init_FEP( void );
static void fillFEPimages( void);
static void fill_one_FEP( Zone z, int y, LLMD *md);
static void make_rn_image( void );
static int get_pos_from_user( LLMD *md) ;
static StubPtr getPtr4actions( unsigned int symstate, enum bs_which which, Key2Action actions[] );
StubPtr getPtr4feh( unsigned int symstate ) ;
static void reDrawMoveModeScrn( feh_node *ottr );
static void toggle_mode( int requested_mode );
static void show_cnt_on_title( winwidget w );
/* void help4feh( const char *help_name);  moved to menu.h */
static void remove_or_delete( enum misc_flags delete_flag );
static void zoom_it( double factor );
static void img_jumper( enum misc_flags jmp_code );

/* ***************** *
 * Begin functions   *
 * ***************** */

/* ******************************************************************** *
 *   These are all the sub_<name'd> functions arranged alphabetically   *
 *   All menu_ funcs begin "menu_".  All move_ funcs begin "move_"      *
 *   Anything else falls into the general feh_actions category          *
 * ******************************************************************** */

void stub_add2move_list( void ) {
	 /* defaults to C-Spacebar key in slideshow mode to tag a pic for moving.
		* No new storage is required.  Just unlink this node from feh_md and
		* relink it into the mov_md list.
		*/

	if ( opt.flg.mode == MODE_INDEX
	  || opt.flg.mode == MODE_THUMBNAIL )
	  return;

	if ( mvInfo.mov_md == NULL )
		mvInfo.mov_md = init_LLMD();        /* holds the move linked list */
	/* else if ( fgv.w->md->rn->nd.cnt < 2) */
	else if ( feh_md->rn->nd.cnt < 2)
		return;                             /* can't tag the last one     */


	/* save the current node B4 I unlink it */
	mvInfo.mov_md->tn = fgv.w->md->cn;

	if ( mvInfo.mov_md->tn != fgv.w->node )
		printf("OOps! w.node was NOT equal to w.cn\n");

	mvInfo.OTTR = mvInfo.mov_md->tn->next;   /* OK even if it is md->rn  */

	/* leave atleast one pic in the list */
	if ( fgv.w->md->rn->nd.cnt > 1 ) {
		feh_ll_unlink( fgv.w->md, FREE_NO );     /* unlinks md->cn */
		feh_ll_link_at_end( mvInfo.mov_md , mvInfo.mov_md->tn );
		mvInfo.mov_md->cn = mvInfo.mov_md->tn;   /* why? */
		feh_ll_recnt( fgv.w->md );
		slideshow_change_image(fgv.w, SLIDE_NO_JUMP, RENDER_YES );
		mvInfo.isDropped = 0;
	}

	return ;

}   /* end of stub_add2move_list() */

/* any action also flags the node's list[0-9] flag */
void stub_action_0( void ){ feh_event_invoke_action(0);}
void stub_action_1( void ){ feh_event_invoke_action(1);}
void stub_action_2( void ){ feh_event_invoke_action(2);}
void stub_action_3( void ){ feh_event_invoke_action(3);}
void stub_action_4( void ){ feh_event_invoke_action(4);}
void stub_action_5( void ){ feh_event_invoke_action(5);}
void stub_action_6( void ){ feh_event_invoke_action(6);}
void stub_action_7( void ){ feh_event_invoke_action(7);}
void stub_action_8( void ){ feh_event_invoke_action(8);}
void stub_action_9( void ){ feh_event_invoke_action(9);}

void stub_close( void ){ winwidget_destroy(fgv.w);}

void stub_delete( void ){ remove_or_delete( DELETE_YES  );}

void stub_enter_move_mode( void ){
		/* This is the entrance to move_mode, allowed from ANY other mode.
		* Always force them into slideshow_mode B4 entering the move screen,
		* AND leave them in ss_mode when they leave.  They can always
		* toggle back to the original mode later.  The mm screen is created,
		* FEP filled and displayed and then returns to the EventHandler
		*/

	if ( opt.flg.mode != MODE_SLIDESHOW ){
		/* change to ss_mode so they can select one */
		if ((opt.flg.mode == MODE_THUMBNAIL) && fgv.tdselected )
				fgv.ptr = &stub_enter_move_mode;     /* one IS selected so allow mm */
		toggle_mode( MODE_SLIDESHOW);
		return;
	}

	if (opt.slideshow_delay > 0.0) {
		opt.flg.paused = 1;
		winwidget_update_title(fgv.w, NULL);
	}

	/* must reduce any fullscreen to a window so mm scrn can show */
	if  ( fgv.w->full_screen ) {
			stub_toggle_fullscreen();
			feh_main_iteration(0);
	}

	if ( (feh_md->rn->nd.cnt < 2) &&
		 ( (mvInfo.mov_md == NULL)  ||
		   (mvInfo.mov_md->rn->nd.cnt == 0) ) ) {
		fgv.w->errstr = "Cannot move a one item list.";
		winwidget_update_title(fgv.w, NULL );
		XBell( fgv.disp, 100);
		return;
	}

	/* going from slideshow to move_mode. */
	opt.flg.mode  = MODE_MOVE ;
	mvInfo.prev_w = fgv.w;                /* for when we return to ss_mode   */
	mvInfo.cn     = feh_md->cn;

	/* If mov_md LL is empty, then the (current_node) becomes its only item. */
	if ( mvInfo.mov_md==NULL || mvInfo.mov_md->rn->nd.cnt == 0 )
		stub_add2move_list();

	FEP.c.n = mvInfo.mov_md->rn->next;    /* the center (ref) pic */
	init_FEP();       /* create the mm_image which is one fullscreen-window  */

	/* FEP.c.n is assigned each time we entered move_mode */
	fill_one_FEP( FEP.c , FEP.dflt.y4c, mvInfo.mov_md  );
	/* from now on, md->cn CAN be set to the md->rn       */
	feh_md->cn = mvInfo.OTTR;
	reDrawMoveModeScrn( feh_md->cn );

}   /* end of stub_enter_move_mode() */

void move_mode_exit( enum mnu_lvl flag ){
		/* called from dropit() and cancel() to control the
		* exit from, and the hiding of, the move_mode window.
		*/
	int flag_dropit = 0;

	if ( flag == CB_MM_ESC_IGNORE ) {
		/* caller wants to leave w/o dropit().  Can't do that!
		 * So dropit() at the front and let them deal with it later */
		FEP.l.n = feh_md->rn;
		FEP.r.n = feh_md->rn->next;
		flag_dropit = 1;
	}	else if ( flag == CB_MM_DROPIT ) {   /* called from menu action */
		feh_md->cn = mvInfo.cn;
		flag_dropit = 1;
	}
	/* if ( flag == CB_MM_ESC_KEEP ), do nothing. don't clear mov_md LL */

	if ( flag_dropit ){
		/* user is ready to DROP the mov_md LL into its new position.
		* mov_md is a LL, so just link in the head and tail to feh_md LL.
		* Purge the mov_md LL when done and return to the slideshow scrn
		* at the spot where this moved pic (FEP.c) is now.
		* mov_md LL is guaranteed to have at least one member.
		*/

		/* the mov_md LL becomes the entire list when feh_md is empty */
		if ( feh_md->rn->next == feh_md->rn )
				FEP.r.n = FEP.l.n = feh_md->rn;

		/* mov_md LL drops between FEP.l.n and FEP.r.n even if either is feh_md->rn */
		FEP.l.n->next = mvInfo.mov_md->rn->next;          /* first pic in mov list  */
		mvInfo.mov_md->rn->next->prev = FEP.l.n;
		mvInfo.mov_md->rn->prev->next = FEP.r.n;
		FEP.r.n->prev = mvInfo.mov_md->rn->prev;          /* last pic in mov list   */

		/* clear out the mov_md LL */
		mvInfo.mov_md->rn->next = mvInfo.mov_md->rn->prev = mvInfo.mov_md->rn;
		mvInfo.mov_md->cn = mvInfo.mov_md->rn;
		mvInfo.mov_md->rn->nd.cnt = 0;
		mvInfo.isDropped=1;
		feh_ll_recnt( feh_md );               /* this is a good time to recount     */
		feh_md->rn->nd.lchange=1;
		fgv.tdselected = NULL;                /* just incase c-m from thumbmode     */

	}   /* end of flag_dropit==1 */

	/* leaving move_mode */
	winwidget_hide(FEP.dflt.w);
	opt.flg.mode = MODE_SLIDESHOW;      /* always return to ss_mode */
	fgv.w = mvInfo.prev_w;
	if  ( opt.flg.full_screen ) {
			stub_toggle_fullscreen();
			feh_main_iteration(0);
	}

	slideshow_change_image(fgv.w, SLIDE_NO_JUMP, RENDER_YES);

	/* if they escaped and saved the mov_md LL, show them the LL count */
	show_cnt_on_title( fgv.w );

}  /* end of move_mode_exit() */

static void reDrawMoveModeScrn( feh_node *ottr ) {
	 /* called once at the start of move_mode and everytime move_to()
		* repositions the drop point.  Note that the feh_md->cn is the one
		* we were on when move_mode was entered, but from then on, move_to()
		* resets that ->cn to be the place where the mov_md LL will be dropped.
		* Set up the 5-pic-window for move_mode with the first item in the mov_md
		* LL as the middle (frozen) pic, flanked by the two pics that would have
		* been left of cn and two that would have been right.
		*/

	fillFEPnodes( ottr  );

	/* remake the rn_image everytime incase they added/deleted some pix */
	make_rn_image();

	/* load all the image stuff for all FEP members.*/
	fillFEPimages();

	/* display the finished product */
	imlib_context_set_blend(0);
	imlib_context_set_image(FEP.dflt.mm_image);
	imlib_context_set_drawable(FEP.dflt.w->bg_pmap);
	winwidget_show(FEP.dflt.w);           /* this maps it to the X11 disp */
	imlib_render_image_on_drawable(0,0);  /* takes the whole window       */
	show_cnt_on_title( FEP.dflt.w );
	/* MUST clear the whole win cause c-f to flip changes the center pic  */
	XClearWindow(fgv.disp, FEP.dflt.w->win);
	XFlush(fgv.disp);

}   /* end of reDrawMoveModeScrn()  */

static void show_cnt_on_title( winwidget w ){
	/* if they escaped and saved the mov_md LL, show them the LL count */
	if ( mvInfo.mov_md->rn->nd.cnt ){
		w->errstr = mobs(1);
		sprintf(w->errstr , "MoveList contains %d items.  ",mvInfo.mov_md->rn->nd.cnt );
		winwidget_update_title(w, NULL );
	}
}

static void init_FEP( void ) {
		/* creates the mm_image from scratch, defaulting to use a fullscreen
		* Allow user to pass a feh runtime param to request a smaller than FS
		* mm window.
		*/

	if (FEP.dflt.mm_image==NULL){
		FEP.dflt.max_w = fgv.scr->width  - 10;  /* allows for borders */
		FEP.dflt.max_h = fgv.scr->height - 42;

		/* all the pix Zones are square.  So fit 4 sq Zones inside max_w,
		* saving dflt.w for the sq's size. leave 5% sliver open in center
		* and a two-pixel space between squares
		*/
		FEP.dflt.wide = ((FEP.dflt.max_w * 0.95)/4) -3;
		/* fit the first sq in at the lower left corner of win */
		FEP.dflt.y = FEP.dflt.max_h - FEP.dflt.wide ;
		FEP.ll.x   = 0;             /* flush left */
		/* move right, skip 2pixel space and do it again */
		FEP.l.x    = FEP.ll.x + 2 + FEP.dflt.wide ;
		/* start at the lower right corner and work backwards */
		FEP.rr.x   = FEP.dflt.max_w - FEP.dflt.wide ;
		FEP.r.x    = FEP.rr.x - 2   - FEP.dflt.wide ;
		/* center the "center" pic both vert and horizontally */
		FEP.c.x    = (FEP.dflt.max_w - FEP.dflt.wide )/2;
		if ( FEP.dflt.y <=  FEP.dflt.wide )
				FEP.dflt.y4c = 0;       /* stuff it at the top  */
		else                        /* center it vertically */
				FEP.dflt.y4c = (FEP.dflt.y - FEP.dflt.wide )/2;

		/* create the mm_image to hold ALL 7 Zones */
		FEP.dflt.mm_image = feh_imlib_image_make_n_fill_text_bg( FEP.dflt.max_w, FEP.dflt.max_h,0);

	}   /* end of FEP.dflt.mm_image==NULL */

	if ( FEP.dflt.w==NULL ){
		FEP.dflt.w = winwidget_create_from_image(feh_md, FEP.dflt.mm_image,
	               "[ move_mode ] allows you to rearrange a slide show list.",
	               WIN_TYPE_MOVE_MODE);
	}

	/* imlib_context_set_drawable(FEP.dflt.w->win); */
	imlib_context_set_drawable(FEP.dflt.w->bg_pmap);
	imlib_context_set_image(FEP.dflt.w->im);
	imlib_context_set_blend(0);
	imlib_context_set_font(opt.fn_fep.fn);
	imlib_context_set_direction(IMLIB_TEXT_TO_DOWN);
	imlib_context_set_color(255,255,255,255);  /* white */    /*(0, 0, 0, 255) black text */
	imlib_text_draw(  FEP.dflt.max_w * 0.48, (FEP.dflt.max_h * 0.4), "   >    >    >    >");

	/* need to add the tl and tr zones */

}   /* end of init_FEP() */

static void make_rn_image( void ){
		/* create an image from scratch that holds the meta data stored in ->rn.
		* If one FEP Zone includes the rn, display this image.
		*/
	char * text = mobs(1);
	int  y_off=50;       /* holds #of pixels from the topLeft  */
	int  text_w, text_h;
	/*int x_off=2*/

	/* create the rn_image (it is square as are all the other FEP Zones */
	FEP.dflt.rn_image = feh_imlib_image_make_n_fill_text_bg( FEP.dflt.wide, FEP.dflt.wide,0);

	imlib_context_set_blend(0);
	imlib_context_set_image(FEP.dflt.rn_image);
	/* write the metadata from the root_node onto this image */
	gib_imlib_image_fill_rectangle(FEP.dflt.rn_image, 0, 0,
				FEP.dflt.wide, FEP.dflt.wide, 171,159,69,255); /* is  tan */
				/* (205,205, 50,255) is  yellow */
	imlib_context_set_font(opt.fn_fep.fn);
	imlib_context_set_direction(IMLIB_TEXT_TO_RIGHT);
	imlib_context_set_color(0, 0, 0, 255);          /* black text */
	strcpy( text, "root_node" );
	imlib_get_text_size(text, &text_w, &text_h);
	imlib_text_draw(  (FEP.dflt.wide - text_w)* 0.5, y_off , text);
	sprintf( text, " %6d pix total", feh_md->rn->nd.cnt );
	imlib_get_text_size(text, &text_w, &text_h);
	imlib_text_draw(  (FEP.dflt.wide - text_w)* 0.5, y_off+=text_h , text);
	y_off +=(text_h);
	strcpy( text, "<< last   /   first >>" );
	imlib_get_text_size(text, &text_w, &text_h);
	imlib_text_draw(  (FEP.dflt.wide - text_w)* 0.5, (y_off+=text_h), text);
	/*strcpy( text, " <- last pic" );
	imlib_text_draw(  x_off, (y_off+=text_h), text);*/

}   /* end of make_rn_image() */


static void fillFEPnodes( feh_node * ottr ){
    /* the moment you enter into mm, the slideshow advances to the NEXT pic,
     * meaning that the feh_md->cn is really now OTTR of the one you are
     * moving.  So set this as OTTR in the mm screen, then flesh one more TTR
     * and two TTL.  If ANY of these settings are the rn, don't worry cause
     * that "pic" will be a special display.  move_to() changes feh_md->cn
     * which becomes the new "center" for this filling.
     */

  FEP.l.n    = ottr->prev;
  FEP.ll.n   = FEP.l.n->prev;

  /*  The frozen FEP.c.n  was assigned at the moment we entered move_mode */

  FEP.r.n    = ottr;
  FEP.rr.n   = FEP.r.n->next;

}  /* end of fillFEPnodes() */

static void fillFEPimages( void){
	 /* all the node info has been filled in, so now fill in the images data.
		* store the FEP pics in a low resolution so the memory footprint is low.
		*/

	/*  fill_one_FEP( FEP.c , FEP.dflt.y4c );   moved to stub_move_mode_toggle() */
	fill_one_FEP( FEP.ll, FEP.dflt.y , feh_md  );
	fill_one_FEP( FEP.l , FEP.dflt.y , feh_md  );
	fill_one_FEP( FEP.r , FEP.dflt.y , feh_md  );
	fill_one_FEP( FEP.rr, FEP.dflt.y , feh_md  );

}   /* end of fillFEPimages() */

static void fill_one_FEP( Zone z, int y, LLMD *md ) {
		/* caller passes one of the five FEP Zones to fill with a thumbnail image
		* The caller has already selected the geometry and x/y coords.  As each
		* Zone is a square, there are blank bars t/b for landscape and l/r for
		* portrait pix.  reuse on Imlib image for all zones.
		*/

	static char text[100];
	int x_off=0,y_off=0,wide,high;
	float w_ratio=1, h_ratio=1;
	char *last1;

	if ( z.n == feh_md->rn ){
			FEP.dflt.com_image = FEP.dflt.rn_image;
			wide = high = FEP.dflt.wide;
	} else {
			/* FIXME.  Jump in here to pull from cached thumbnails */
			if ( !( FEP.dflt.com_image = imlib_load_image(NODE_FILENAME(z.n)) )  ){
					weprintf("%s - %s to load %s.", opt.modes[opt.flg.mode],ERR_FAILED, NODE_FILENAME(z.n));
					return;
			}
			/* paste this image in the middle of the sq Zone */
			imlib_context_set_image(FEP.dflt.com_image);
			wide  = imlib_image_get_width();
			high  = imlib_image_get_height();
			if ( high > wide ){
					/* portrait */
					h_ratio=1;
					w_ratio=(float)wide/high;
					/* x_off  =(float)(FEP.dflt.wide * w_ratio )/5; */
					x_off  = ( FEP.dflt.wide  - (FEP.dflt.wide * w_ratio ))/2;
			} else {
					/* landscape */
					w_ratio=1;
					h_ratio=(float)high/wide;
					/* y_off  =(float)(FEP.dflt.wide * h_ratio )/5; */
					y_off  = ( FEP.dflt.wide  - (FEP.dflt.wide * h_ratio ))/2;
			}
	}

	/* blend this image onto the mm_image */
	imlib_context_set_blend(0);
	imlib_context_set_image(FEP.dflt.mm_image);
	imlib_context_set_color(0,0,0,255);  /* black */
	/* if you want tan bg use this ... */
	/* imlib_context_set_color(171,159,69,255); */  /* jan 9 */
	imlib_image_fill_rectangle(z.x, y, FEP.dflt.wide, FEP.dflt.wide );
	/* this uses the gradient_range for each sq Zone */
	/* imlib_image_fill_color_range_rectangle(z.x, y, FEP.dflt.wide, FEP.dflt.wide,0); */
	imlib_blend_image_onto_image(FEP.dflt.com_image, 0, 0, 0, wide, high,
		     z.x+x_off, y+y_off , FEP.dflt.wide*w_ratio, FEP.dflt.wide*h_ratio );
	/*printf("x_off is %d, w_ratio = %f, y_off is %d, h_ratio = %f\n", x_off, w_ratio, y_off, h_ratio);*/
	if ( FEP.dflt.com_image == FEP.dflt.rn_image ) return;

	/* not the rn_image, so free it */
	imlib_context_set_image(FEP.dflt.com_image);
	imlib_free_image();

	/* write filename (no-ext) directly onto the mm_image */
	if ( ( last1 = NODE_EXT(z.n))  )
		*last1 = '\0';

	snprintf(text, sizeof(text)-2, " [%d of %d] - %s",
			    z.n->nd.cnt,
			    md->rn->nd.cnt,
			    NODE_NAME(z.n));
	if ( last1 ) *last1 = '.';     /* restore the name+extension */

	imlib_context_set_image(FEP.dflt.mm_image);
	imlib_context_set_font(opt.fn_dflt.fn);
	imlib_context_set_color(255,255,255,255);  /* white */    /*(0, 0, 0, 255) black text */
	imlib_context_set_direction(IMLIB_TEXT_TO_RIGHT);
	imlib_text_draw(  z.x, y, text);

}   /* end of show_one_FEP() */

void stub_flip( void ){
	feh_edit_inplace(fgv.w, INPLACE_EDIT_FLIP);
}

void stub_help4bindings(void ){
		/* complete help screen to display ALL the current key bindings and action
		* commands.  This is preprocessed by feh_init and stored in feh_bindings.htm.
		*/
	help4feh( FEH_BINDINGS_HELP );
}

void stub_help4feh(void ){
	/* complete help screen to display the stock feh.1 (man page in html)*/
	help4feh( FEH_GENERAL_HELP );
}

extern void help4feh( const char *help_name){
		/* Note:  In fullscreen mode, the browser window will not display,
		* so first toggle out of FS, then display the help screen.
		*/

	char *cmd = mobs(1);
	if  ( fgv.w->full_screen ) {
			stub_toggle_fullscreen();
			feh_main_iteration(0);
	}
	STRCAT_3ITEMS(cmd, "$DEFAULTBROWSER ", fgv.bind_path, help_name );
	esystem( cmd );
}

static void img_jumper( enum misc_flags jmp_code ){
		/* called by all the stub_jump_xxx() */
	if (opt.flg.mode == MODE_SLIDESHOW)
		slideshow_change_image(fgv.w, jmp_code, RENDER_YES);
	else if (fgv.w->type == WIN_TYPE_THUMBNAIL)
		feh_thumbnail_select(fgv.w, jmp_code);
}

void stub_jump_back(  void ){ img_jumper(SLIDE_JUMP_BACK);}
void stub_jump_first( void ){ img_jumper(SLIDE_FIRST);}
void stub_jump_fwd(   void ){ img_jumper(SLIDE_JUMP_FWD);}
void stub_jump_last(  void ){ img_jumper(SLIDE_LAST);}
void stub_jump_random(void ){ img_jumper(SLIDE_RAND);}


/* ***************************************************** *
 *   All the menu_<name'd> functions go here.            *
 *   These use the global fgv.selected_menu and          *
 *   fgv.selected_item members                           *
 * ***************************************************** */

void stub_menu_child() {      /* can inline*/
		feh_menu_select_submenu(fgv.selected_menu);
}

void stub_menu_close() {      /* cannot inline */
		feh_menu_hide(fgv.mnu.root, True);
}

void stub_menu_down() {       /* can inline */
		feh_menu_select_next(fgv.selected_menu, fgv.selected_item);
}

void stub_menu_parent() {     /* can inline */
		feh_menu_select_parent(fgv.selected_menu);
}

void stub_menu_select() {     /* cannot inline */
		feh_menu_item_activate(fgv.selected_menu, fgv.selected_item);
}

void stub_menu_up() {         /* can inline */
		feh_menu_select_prev(fgv.selected_menu, fgv.selected_item);
}


/* ********************************************* *
 *   All the move_<name'd> functions go here.    *
 * ********************************************* */

void stub_move_cancel( void ){
		/* User trying to leave move_mode without dropit(), so display a
		* bogus menu used as a dialog box.  the user's selection controls
		* the exit from mm. See feh_menu_cb().
		*/

	opt.fn_ptr = &opt.fn_fulls;

	if ( feh_menu_init_from_mnu_defs( MENU_MM_ESC_OK, 1 ) == 1 )
		eprintf("%s menu load for sub_code %d !",ERR_FAILED, MENU_MM_ESC_OK );

	/* place the popup to the right of the center pic */
	feh_menu_show_at_xy(fgv.mnu.root, FEP.dflt.w,
	                    FEP.c.x+(FEP.dflt.wide)+5,
	                    FEP.dflt.y4c+(FEP.dflt.wide/2));

}   /* end of stub_move_cancel() */

void stub_move_dropit( void ){ move_mode_exit(  CB_MM_DROPIT ); }

void stub_move_flip_list( void ){
		/* active only during move_mode, reverses the mov_md LL - AND - changes
		* the FEP.c.n in the move scrn to be the first pic in the list
		*/
	feh_ll_reverse( mvInfo.mov_md );
	FEP.c.n = mvInfo.mov_md->rn->next;
	fill_one_FEP( FEP.c , FEP.dflt.y4c, mvInfo.mov_md );
	reDrawMoveModeScrn( feh_md->cn );       /* cause the FEP.c.n changed */

}

void stub_move_ret2pos( void ) {
		/* After the mov_md LL has been dropped into a new location, this returns
		* the user to the place he started BEFORE initiating the move.  This is
		* a stack, so he can ret2pos() for many sebsequent moves.
		* See stub_ret2posFwd for the logic to jump AHEAD in the stack.
		*/

	if ( opt.flg.mode == MODE_MOVE )      /* dropit and ret2pos */
			if ( ! mvInfo.isDropped )
					move_mode_exit(  CB_MM_DROPIT );

	/* we are now out of move_mode, so just ret2pos .  If the mvInfo.OTTR is set
	* then we can just ret2pos on that one.  Else we have to look in the stack.
	* Note.  OTTR could be md->rn, but never store an rn node on the stack.
	* If it is the rn, then change it to be OTTL so that they stay at the tail
	* end of the list rather than jumping to the start.
	*/
	if ( mvInfo.OTTR ) {
		if ( mvInfo.OTTR == feh_md->rn )
			mvInfo.OTTR = feh_md->rn->prev;           /* Note:  this is OTTL now */
		feh_md->cn = mvInfo.OTTR;
		ret2stack( RET2PUSH, RET2IGNORE );
		/* OTTR is NULLd out after the push  */
	} else ret2stack( RET2GET, RET2BAK );

}     /* end of stub_move_ret2pos()   */

static void ret2stack( enum ret2  pp, enum ret2  direction ) {
		/* give accessd to the OTTR stack.  The "pp" is a push/pop flag while the
		* direction flag controls going Fwd or Back in the stack.  This sets the cn
		* and refreshes the screen.
		*/

	static int ptr=-1, cnt=0, max=(sizeof(mvInfo.ret2List) / sizeof(mvInfo.ret2List[0]))-4;

	if ( cnt == max ) {
			/* stack is full, so cut it in half */
			int i=max/2, j;
			ptr-=i;
			for ( j=0; i < max ; j++, i++ ){
					mvInfo.ret2List[j]= mvInfo.ret2List[i];
					mvInfo.ret2List[i]=NULL;
			}
			cnt=j;
	}

	if ( pp == RET2PUSH ) {
			if ( mvInfo.OTTR ) {
					ptr++;
					mvInfo.ret2List[ptr]=mvInfo.OTTR;
					mvInfo.OTTR=NULL;
			}
	} else {    /* the is a get request */
			if ( direction == RET2FWD && ptr < cnt ) ptr++;
			if ( direction == RET2BAK && ptr > 0   ) ptr--;
	}

	/* everything falls thru to here */
	if ( ptr > cnt ) cnt=ptr;

	feh_md->cn = mvInfo.ret2List[ptr];
	slideshow_change_image(fgv.w, SLIDE_NO_JUMP, RENDER_YES);

}     /* end of ret2stack()     */


void move_to( int where) {
		/* Called by ALL the move_to_??() commands while the mm scrn is displayed.
		* So the pix two TTR and two TTL of the center are displayed. "where"
		* holds either a hard-coded position number, or +/-1 for simple position
		* swaps, or a percentage of the total list, or 0 for beg , or 100 for end.
		* Note that the md->rn is a valid node to land on, cause it is visible in
		* the mm screen.
		*/

	/* set cn (could be rn) based on the "where" request */
	if ( where == -1 )
			feh_md->cn = feh_md->cn->prev;
	else if ( where == 1 )
			feh_md->cn = feh_md->cn->next;
	else if ( where == 0 )
			feh_md->cn = feh_md->rn->next;    /* beginning    */
	else if ( where == -9 )
			get_pos_from_user( feh_md );
	else if ( where == 100 )
			feh_md->cn = feh_md->rn;          /* the last one */
	else if ( where > 9 ){                /* 10% thru 90% jumps */
			where *= feh_md->rn->nd.cnt;
			feh_ll_nth( feh_md,  ( where+60)/100 );     /* round up  */
	}else
			feh_md->cn = feh_md->cn->next;         /* just a safe default */

	/* the cn (current_node) has now been set. Ready for a reDraw.  */
	if ( opt.flg.mode == MODE_MOVE )
		reDrawMoveModeScrn( feh_md->cn );
	else {  /* the diff between mm and ss_modes is mm accepts cn==rn */
		if ( feh_md->cn == feh_md->rn )
				feh_md->cn = feh_md->rn->prev;
		img_jumper(SLIDE_NO_JUMP);
	}

}   /* end of move_to() */

void stub_move_one_bak( void ) {  move_to(-1); }
void stub_move_one_fwd( void ) {  move_to(1);  }
void stub_move_to_10(   void ) {  move_to(10); }
void stub_move_to_20(   void ) {  move_to(20); }
void stub_move_to_30(   void ) {  move_to(30); }
void stub_move_to_40(   void ) {  move_to(40); }
void stub_move_to_50(   void ) {  move_to(50); }
void stub_move_to_60(   void ) {  move_to(60); }
void stub_move_to_70(   void ) {  move_to(70); }
void stub_move_to_80(   void ) {  move_to(80); }
void stub_move_to_90(   void ) {  move_to(90); }
void stub_move_to_beg(  void ) {  move_to(0);  }
void stub_move_to_end(  void ) {  move_to(100);}
void stub_move_to_pos(  void ) {  move_to(-9); }

static int get_pos_from_user( LLMD *md) {
		/* pop a dialog to get the users request to move to a specific node in the
		* list.  That relative (34of200) position could change with each move.
		* so save the node along with the pos and jump to that node if the
		* user REUSES that position.
		* returns 0 if nothing happened, else 1 to say we got input.
		* the new position is stored in feh_md->cn
		*/

	/* get the user's position request, always a positive# */
	/* turn it into a code for passing to move_to() */
	md->cn = md->cn->next;

	return 1;           /* just a bogus till i get this working */
}

/*   stub_move_help( void ){}  **** ReMapped **** */

/* ********************************************* *
 *   Back to the remaining feh_action funcs      *
 * ********************************************* */

void stub_mirror( void ){ feh_edit_inplace(fgv.w, INPLACE_EDIT_MIRROR);}

void stub_orient_1( void ){ feh_edit_inplace(fgv.w, 1);}
void stub_orient_3( void ){ feh_edit_inplace(fgv.w, 3);}

void stub_next_img( void ){ img_jumper(SLIDE_NEXT);}
void stub_prev_img( void ){ img_jumper(SLIDE_PREV);}

void stub_quit( void ){	winwidget_destroy_all();}
void stub_reload_image( void ){ feh_reload_image(fgv.w, RESIZE_NO, FORCE_NEW_NO );}

void stub_reload_minus( void ){
#ifdef HRABAK
		if (opt.flg.mode == MODE_SLIDESHOW) {
				if ( opt.slideshow_delay > 2 )
						opt.slideshow_delay--;
				else
						opt.slideshow_delay/=2;
					weprintf("SS-delay is %.2f seconds.", opt.slideshow_delay);
		}
#endif     /*  HRABAK */
		if (opt.reload > 1)
			opt.reload--;
		else if (opt.flg.verbose)
			weprintf("set RELOAD between 1 and %d seconds.", SLIDESHOW_RELOAD_MAX);

}

void stub_reload_plus( void ){
#ifdef HRABAK
		if (opt.flg.mode == MODE_SLIDESHOW) {
				if ( opt.slideshow_delay < 1 ){
						opt.slideshow_delay*=2;
						if ( opt.slideshow_delay == 0 ) opt.slideshow_delay = 1 ;
				} else
						opt.slideshow_delay++;
				weprintf("SS+delay is %.2f seconds.", opt.slideshow_delay);
		}
#endif     /*  HRABAK */
		if (opt.reload < SLIDESHOW_RELOAD_MAX)
			opt.reload++;
		else if (opt.flg.verbose)
			weprintf("set RELOAD between 1 and %d seconds.", SLIDESHOW_RELOAD_MAX);

}

void stub_remove( void ){ remove_or_delete( DELETE_NO );}

void stub_render( void ){
	if (fgv.w->type == WIN_TYPE_THUMBNAIL)
		feh_thumbnail_show_fullsize( fgv.w->md, fgv.w->node);
	else
		winwidget_render_image(fgv.w, 0, 0, SANITIZE_NO);

}

/* void stub_ret2pos( void ){}   **** ReMapped *** */
void stub_ret2posFwd( void ){
		/* See move_ret2pos().  This allows the user to navigate Fwd
		* in the stack, BUT only while he is not in move_mode
		*/
	ret2stack(RET2GET, RET2FWD );

}

void stub_rot_ccw_fast( void ){
		weprintf("rot_ccw_fast.");
}

void stub_rot_ccw_slow( void ){
		weprintf("rot_ccw_slow.");
}

void stub_rot_cw_fast( void ){
		weprintf("rot_cw_fast.");
}

void stub_rot_cw_slow( void ){
		weprintf("rot_cw_slow.");
}

void stub_save_filelist( void ){
		/* just inline the old feh_save_filelist() */

	char *tmpname = feh_unique_filename("", "filelist");

	if (opt.flg.verbose)
		printf("saving filelist as '%s'\n", tmpname);

	feh_write_filelist( feh_md , tmpname, 10 );

}

void stub_save_image( void ){ slideshow_save_image(fgv.w);}

void stub_scroll_down( void ){
	fgv.w->im_y -= 20;
	winwidget_render_image(fgv.w, 0, 1, SANITIZE_YES);
}

void stub_scroll_down_page( void ){
	fgv.w->im_y -= fgv.w->high;
	winwidget_render_image(fgv.w, 0, 0, SANITIZE_YES);
}

void stub_scroll_left( void ){
	fgv.w->im_x += 20;
	winwidget_render_image(fgv.w, 0, 1, SANITIZE_YES);
}

void stub_scroll_left_page( void ){
	fgv.w->im_x += fgv.w->wide;
	winwidget_render_image(fgv.w, 0, 0, SANITIZE_YES);
}

void stub_scroll_right( void ){
	fgv.w->im_x -= 20;
	winwidget_render_image(fgv.w, 0, 1, SANITIZE_YES);
}

void stub_scroll_right_page( void ){
	fgv.w->im_x -= fgv.w->wide;
	winwidget_render_image(fgv.w, 0, 0, SANITIZE_YES);
}

void stub_scroll_up( void ){
	fgv.w->im_y += 20;
	winwidget_render_image(fgv.w, 0, 1, SANITIZE_YES);
}

void stub_scroll_up_page( void ){
	fgv.w->im_y += fgv.w->high;
	winwidget_render_image(fgv.w, 0, 0, SANITIZE_YES);
}

void stub_size_to_image( void ){ winwidget_size_to_image(fgv.w);}

void stub_toggle_actions( void ){
	opt.flg.draw_actions = !opt.flg.draw_actions;
	winwidget_rerender_all(0);
}

void stub_toggle_aliasing( void ){
	opt.force_aliasing = !opt.force_aliasing;
	winwidget_render_image(fgv.w, 0, 0, SANITIZE_NO);
}

void stub_toggle_caption( void ){
	if (opt.caption_path){
			/* editing captions in slideshow mode does not make any sense
			* at all; this is just in case someone accidentally does it...
			* Hrabak does it all the time :).
			*/
			char *tmp = mobs(3);    /* use a tmp buff for all caption edits */

			if (opt.slideshow_delay > 0.0) opt.flg.paused = 1;
			fgv.w->caption_entry = 1;
			fgv.old_cap = NODE_CAPTION(fgv.w->node);
			strcat( tmp, fgv.old_cap );
			NODE_CAPTION(fgv.w->node) = tmp;
			winwidget_render_image(fgv.w, 0, 0, SANITIZE_NO);
			/* zero out the buff, ready for a new caption */
			if ( fgv.old_cap == fgv.no_cap )
					NODE_CAPTION(fgv.w->node)[0]='\0';
	}

}

void stub_toggle_filenames( void ){
		opt.flg.draw_filename = !opt.flg.draw_filename;
		winwidget_rerender_all(0);
}
#ifdef HAVE_LIBEXIF
	void stub_toggle_exif( void ){
		opt.flg.draw_exif = !opt.flg.draw_exif;
		winwidget_rerender_all(0);
	}
#endif

void stub_toggle_fullscreen( void ){
#ifdef HAVE_LIBXINERAMA
		int curr_screen = 0;
		if (opt.flg.xinerama && fgv.xinerama_screens) {
			int i, rect[4];

			winwidget_get_geometry(fgv.w, rect);
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
#endif    /* HAVE_LIBXINERAMA */
		fgv.w->full_screen = !fgv.w->full_screen;
		winwidget_destroy_xwin(fgv.w);
		winwidget_create_window(fgv.w, fgv.w->im_w, fgv.w->im_h);
		winwidget_render_image(fgv.w, 1, 0, SANITIZE_NO);
		winwidget_show(fgv.w);
#ifdef HAVE_LIBXINERAMA
		/* if we have xinerama and we're using it, then full screen the window
		 * on the head that the window was active on */
		if (fgv.w->full_screen == TRUE && opt.flg.xinerama && fgv.xinerama_screens) {
			fgv.xinerama_screen = curr_screen;
			winwidget_move(fgv.w,
					fgv.xinerama_screens[curr_screen].x_org, fgv.xinerama_screens[curr_screen].y_org);
		}
#endif   /* HAVE_LIBXINERAMA */

}
void stub_toggle_info( void ){
	opt.flg.draw_info = !opt.flg.draw_info;
	winwidget_rerender_all(0);
}

void stub_toggle_menu( void ){ winwidget_show_menu(fgv.w);}

/*   void stub_toggle_move_mode( void ){}  **** ReMapped **** */

void stub_toggle_pause( void ){
	opt.flg.paused = !opt.flg.paused;
	winwidget_update_title(fgv.w, NULL);
}

void stub_toggle_pointer( void ){
	winwidget_set_pointer(fgv.w, opt.flg.hide_pointer);
	opt.flg.hide_pointer = !opt.flg.hide_pointer;
}

void stub_toggle_thumb_mode( void ) {
	/* dynamic toggle between slideshow and index/thumbnail/multi displays */
	if ( opt.flg.mode == MODE_SLIDESHOW  ){
		fgv.tdselected = NULL;    /* just in case */
		toggle_mode( MOVE_RESTORE_ORIGINAL);
	} else
		toggle_mode( MODE_SLIDESHOW);
}

static void toggle_mode( int requested_mode ){
		/* because toggling events call winwidget_destroy_all(), I have to
		 * protect FEP.dflt.mm_image and null out the FEP.dflt.w->im else
		 * the destroy() will decache that w->im.
		 */

	opt.flg.mode = (requested_mode == MODE_SLIDESHOW)
	             ? MODE_SLIDESHOW
	             : opt.flg.mode_original;

	if (FEP.dflt.w && FEP.dflt.w->im )
		FEP.dflt.w->im = NULL;
	FEP.dflt.w = NULL;          /* to trigger a re-winwidget_create() */
	fgv.doit_again = 1;
	winwidget_destroy_all();
}

void stub_zoom_default( void ){
	fgv.w->zoom = 1.0;
	/* --scale-down will revert our operation if old_zoom == 1.0 */
	if (opt.flg.scale_down)
		fgv.w->old_zoom = 1.001;
	winwidget_center_image(fgv.w);
	winwidget_render_image(fgv.w, 0, 0, SANITIZE_NO);
	/* --scale-down will also do weird stuff if zoom is 1.0 */
	if (opt.flg.scale_down)
		fgv.w->zoom = 1.001;
}

void stub_zoom_fit( void ){
	feh_calc_needed_zoom(&fgv.w->zoom, fgv.w->im_w, fgv.w->im_h, fgv.w->wide, fgv.w->high);
	winwidget_center_image(fgv.w);
	winwidget_render_image(fgv.w, 0, 0, SANITIZE_NO);
}

void stub_zoom_in(  void ){ zoom_it( 1.25 ); }
void stub_zoom_out( void ){ zoom_it( 0.80 ); }


/* ********************************************* *
 *   End of stub_<name'd> functions stubs.       *
 * ********************************************* */

static void zoom_it( double factor ){
	fgv.w->old_zoom = fgv.w->zoom;
	fgv.w->zoom = fgv.w->zoom * factor;

	if (fgv.w->zoom > ZOOM_MAX)
			fgv.w->zoom = ZOOM_MAX;

	if (fgv.w->zoom < ZOOM_MIN)
			fgv.w->zoom = ZOOM_MIN;

	fgv.w->im_x = (fgv.w->wide / 2) - (((fgv.w->wide / 2) - fgv.w->im_x) /
		fgv.w->old_zoom * fgv.w->zoom);
	fgv.w->im_y = (fgv.w->high / 2) - (((fgv.w->high / 2) - fgv.w->im_y) /
		fgv.w->old_zoom * fgv.w->zoom);
	winwidget_render_image(fgv.w, 0, 0, SANITIZE_YES);
}

static void remove_or_delete( enum misc_flags delete_flag ){
	/* called only by stub_remove() and _delete() */
	if (opt.flg.mode == MODE_INDEX ) return;
	if (fgv.w->type == WIN_TYPE_THUMBNAIL_VIEWER || WIN_TYPE_THUMBNAIL )
		feh_thumbnail_mark_removed(fgv.w->node, delete_flag );
	feh_move_node_to_remove_list(fgv.w, delete_flag, WIN_MAINT_DO);

}


static void feh_event_invoke_action( unsigned char action) {
		/* Called from all the stub_action<N> calls in newkeyev.c.   This is a
		* minimally hacked version of the original function of the same name .
		*/
	short int *flag;

	if (opt.actions[action]) {
		if (opt.flg.mode == MODE_SLIDESHOW) {
			feh_action_run(fgv.w->node, opt.actions[action]);
			winwidget_update_caption(fgv.w);

			if ( !(opt.actions[action][0] == ';') )         /* this is a "hold" action */
				slideshow_change_image(fgv.w, SLIDE_NEXT, RENDER_YES);

		} else if ((fgv.w->type == WIN_TYPE_SINGLE)
		        || (fgv.w->type == WIN_TYPE_THUMBNAIL_VIEWER)) {
			feh_action_run(fgv.w->node, opt.actions[action]);
			if ( !(opt.actions[action][0] == ';') )         /* this is a "hold" action */
				winwidget_destroy(fgv.w);
		} else if (fgv.w->type == WIN_TYPE_THUMBNAIL) {
			if ( fgv.tdselected ){
				feh_action_run(fgv.w->node, opt.actions[action]);
				if ( !(opt.actions[action][0] == ';') )       /* this is a "hold" action */
					fgv.tdselected = NULL;
			} else {
				fputs("No pic selected to 'action' on!\n", stdout);
			}
		}
	}
	/* regardless if an action was/wasnot called, update the list[0-9] flags */
	flag = (short int *) &fgv.w->node->nd;          /* for cn->nd   */
	*flag ^= (1<<(short int)action);                /* flip the bit */
	flag = (short int *) &fgv.w->md->rn->nd;        /* for rn->nd   */
	*flag |= (1<<(short int)action);                /* set the bit  */
	/* then set one flag to know if ANY of the list[0-9] were set   */
	fgv.w->md->rn->nd.tagged = fgv.w->node->nd.tagged = 1;
}


static StubPtr getPtr4actions( unsigned int symstate, enum bs_which which, Key2Action actions[] ){
		/* called by feh_event_handle_keypress() to resolve the funcptr that
		* mapped to this keypress (symstate) code.  Used for menu_ and move_.
		* Returns the function ptr to the action stub, or NULL.
		*/

	static int i;

	for (i=0;i<bs.acnt[which] && symstate >= actions[i].symstate ;i++ )
		if (symstate == actions[i].symstate )
			return actions[i].ptr ;

	/* fell off the end so symstate is not defined. */
	return (StubPtr) NULL;
}


#ifdef BTREE_SEARCH

StubPtr getPtr4feh( unsigned int symstate ){
		/* called by feh_event_handle_keypress() with the user's keypress.
		* Returns the function ptr to the action stub, or NULL.
		*/

	static int i , end ;

	/* if ((symstate < act_subset[0].symstate ) || (symstate > act_subset[bs.ss_last].symstate )) */
	if ( symstate > act_subset[bs.ss_last].symstate )
		return (StubPtr) NULL;      /* symstate is out of range*/

	/* act_subset[] holds 10% of the full feh_actions[], just for speedy lookup */
	for (i=0; symstate >= act_subset[i].symstate ;i++ )
		if (symstate == act_subset[i].symstate )
			return act_subset[i].ptr ;

	/* falls thru to searching the full array. Index i is now set to one PAST
	* the point to start looking in the full feh_actions[] array.
	* Search feh_actions[], starting ONE BEFORE the act_subset[i].which ptr.*/
	end=act_subset[i].which;
	for (i=act_subset[i-1].which+1;i<end && symstate >= feh_actions[i].symstate ;i++ ) {
		if (symstate == feh_actions[i].symstate )
			return feh_actions[i].ptr ;
	}

	return (StubPtr) NULL;             /* Not found */
}
#endif      /* end of BTREE_SEARCH */

#ifdef BSEARCH_SEARCH
StubPtr getPtr4feh( unsigned int symstate ){
		/* This version uses the C-lang bsearch() instead.
		* Called by feh_event_handle_keypress() with the user's keypress.
		* Returns the function ptr to the action stub, or NULL.
		*/

	static Key2Action *aptr=NULL;

	if ((symstate < act_subset[0].symstate ) || (symstate > act_subset[bs.ss_last].symstate ))
		return (StubPtr) NULL;      /* symstate is out of range*/

	aptr=bsearch(  &symstate, feh_actions ,bs.acnt[FEH_PTR],sizeof(feh_actions[0]),  cmp_symstate );

	return (StubPtr) aptr ? aptr->ptr : NULL;
}  /* end of bsearch_actions() */

#endif      /* end of BSEARCH_SEARCH */

static void edit_caption( KeySym keysym, int state ){
		/* All caption edits are done in a tmp buffer assigned at the time they
		* toggle_caption.  If they DON'T commit edit changes, then revert
		* back to the old caption saved in fgv.old_cap.
		*/
		static char *tmp;

		if ( keysym ==  XK_Return ) {
			if (state & ControlMask) {
				/* insert actual newline */
				strcat( NODE_CAPTION(fgv.w->node), "\n");
				winwidget_render_image_cached(fgv.w);
			} else {
				/* finished. Write to captions file */
				FILE *fp;
				char *cap_file = build_caption_filename(NODE_DATA(fgv.w->node), 1);
				fgv.w->caption_entry = 0;
				winwidget_render_image_cached(fgv.w);
				XFreePixmap(fgv.disp, fgv.w->bg_pmap_cache);
				fgv.w->bg_pmap_cache = 0;
				if ( NODE_CAPTION(fgv.w->node)[0]=='\0') {
						unlink(cap_file);           /* kill zerobyte captions*/
						NODE_CAPTION(fgv.w->node) = fgv.no_cap;
					return;
				}
				if ( ! (fp = fopen(cap_file, "w") ) ){
					eprintf("%swrite to captions file %s:",ERR_CANNOT, cap_file);
					return;
				}
				fprintf(fp, "%s", NODE_CAPTION(fgv.w->node));
				fclose(fp);
				if ( !( fgv.old_cap == fgv.no_cap || fgv.old_cap == NULL ) )
						free( fgv.old_cap );
				/* save this tmp buffer as the real caption */
				NODE_CAPTION(fgv.w->node) = estrdup(NODE_CAPTION(fgv.w->node));
			}
		} else if ( keysym ==  XK_Escape ) {
			/* cancel, revert caption */
			fgv.w->caption_entry = 0;
			NODE_CAPTION(fgv.w->node)= fgv.old_cap;
			winwidget_render_image_cached(fgv.w);
			XFreePixmap(fgv.disp, fgv.w->bg_pmap_cache);
			fgv.w->bg_pmap_cache = 0;
		} else if ( keysym ==  XK_BackSpace ) {
			/* kill last char (if any) */
			tmp = strchr( NODE_CAPTION(fgv.w->node),'\0');
			*(tmp-1) = '\0';
			state = 0;                        /* reuse state as a flag */
			if ( NODE_CAPTION(fgv.w->node)[0]=='\0' ) {
					strcat( NODE_CAPTION(fgv.w->node), fgv.no_cap );
					state = 1;                    /* flag to clean out no_cap text */
			}
			winwidget_render_image_cached(fgv.w);
			if ( state )
					NODE_CAPTION(fgv.w->node)[0]='\0';
		} else {
			if (isascii(keysym)) {            /* append to caption */
				tmp = strchr( NODE_CAPTION(fgv.w->node),'\0');
				tmp[0] = keysym;	tmp[1] = '\0';
				winwidget_render_image_cached(fgv.w);
			}
		}

}   /* end of edit_caption() */

void feh_event_handle_keypress(XEvent * ev) {
		/* registered in feh_event_init() as the default KeyPress event handler.
		* This is a hacked version of the stock feh function of the same name,
		* but modified to handle HRABAK sysmstate logic
		*/

	int state;
	static char kbuf[20];
	KeySym keysym;
	XKeyEvent *kev;
	StubPtr fptr;
	unsigned int symstate;


	fgv.w = winwidget_get_from_window(ev->xkey.window);

	/* nuke dupe events, unless we're typing text */
	if (fgv.w && !fgv.w->caption_entry) {
		while (XCheckTypedWindowEvent(fgv.disp, ev->xkey.window, KeyPress, ev));
	}

	kev = (XKeyEvent *) ev;
	XLookupString(&ev->xkey, (char *) kbuf, sizeof(kbuf), &keysym, NULL);
	state = kev->state & (ControlMask | ShiftMask | Mod1Mask | Mod4Mask);

	if (isascii(keysym))
		state &= ~ShiftMask;

	symstate=(keysym << 8) + state;    /* this is HRABAK's combo symstate code */

	/* if menus showing? then this is a menu control keypress */
	if (ev->xbutton.window == fgv.mnu.cover) {
		fgv.selected_item = feh_menu_find_selected_r(fgv.mnu.root, &fgv.selected_menu);
		if (  (fptr=getPtr4actions( symstate, MENU_PTR , menu_actions )) ) fptr();
		return;
	}

	if (fgv.w == NULL)
		return;

	if (fgv.w->caption_entry){
			edit_caption( keysym, state );
			return;
	}
	/* are we in move_mode? */
	if ( opt.flg.mode == MODE_MOVE ) {
			if ( (fptr=getPtr4actions( symstate, MOVE_PTR , move_actions )) ) fptr();
			return ;
	}

	/* anything that gets past this point is a general feh  keypress */
	if ( (fptr=getPtr4feh( symstate )) ) fptr();

}   /* end of feh_event_handle_keypress() */

void init_keyevents( void ) {
		/* called by main() to load all the keybindings.  See the feh_init.c for the
		* code that creates the feh_bindings.map binary file.  This init() loads that
		* pre-made key binding array.  This is a HRABAK modification to speed the
		* loading of key bindings by offloading the parsing chore to feh_init.
		* This is the same function name found in the originial keyevents.c module,
		* but completely different logic.
		*/

	if (read_map_from_disc( ) ){
			/* it failed.  So shell out to run the feh_init program */
			esystem( "./feh_init NoTest" );   /* FIXME.  kill './' for final version */
			if (read_map_from_disc( ) ){
					/* bad news.  feh cannot run without this map */
					eprintf("The pre-built '" FEH_BINDINGS_MAP "' could not be loaded.\n");
			}
	}

	/* that's it.  The entire keymap is loaded and ready to go. */

}  /* end of init_keyevents */

