/* menu.h

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

#ifndef MENU_H
#define MENU_H

/* moved to structs.h
 * typedef struct _feh_menu feh_menu;
 * typedef struct _feh_menu_item feh_menu_item;
 * typedef struct _feh_menu_list feh_menu_list;
 */

#define MENU_ITEM_STATE_NORMAL   0x00
#define MENU_ITEM_STATE_SELECTED 0x01
#define MENU_ITEM_STATE_ON       0x02

#define MENU_ITEM_IS_SELECTED(item) \
((item)->state & MENU_ITEM_STATE_SELECTED)
#define MENU_ITEM_SET_NORMAL(item) \
(item)->state = (item)->state & (~MENU_ITEM_STATE_SELECTED)
#define MENU_ITEM_SET_SELECTED(item) \
(item)->state = (item)->state | MENU_ITEM_STATE_SELECTED

#define MENU_ITEM_IS_ON(item) \
((item)->state & MENU_ITEM_STATE_ON)
#define MENU_ITEM_TOGGLE_ON(item) \
((item)->state |= MENU_ITEM_STATE_ON)
#define MENU_ITEM_TOGGLE_OFF(item) \
((item)->state &= ~(MENU_ITEM_STATE_ON))
#define MENU_ITEM_TOGGLE_SET(item, setting) \
((setting) ? MENU_ITEM_TOGGLE_ON(item) : MENU_ITEM_TOGGLE_OFF(item))
#define MENU_ITEM_TOGGLE(item) \
(((item)->state & MENU_ITEM_STATE_ON) ? MENU_ITEM_TOGGLE_OFF(item) : MENU_ITEM_TOGGLE_ON(item))

#define RECTS_INTERSECT(x, y, w, h, xx, yy, ww, hh) \
((SPANS_COMMON((x), (w), (xx), (ww))) && (SPANS_COMMON((y), (h), (yy), (hh))))
#define SPANS_COMMON(x1, w1, x2, w2) \
(!((((x2) + (w2)) <= (x1)) || ((x2) >= ((x1) + (w1)))))

/* all of these were just #defines in the old menu.h  */
enum mnu_offsets {
				FEH_MENUITEM_PAD_TOP    = 1,
				FEH_MENUITEM_PAD_BOTTOM = FEH_MENUITEM_PAD_TOP ,
				FEH_MENUITEM_PAD_LEFT   = 2,
				FEH_MENUITEM_PAD_RIGHT  = FEH_MENUITEM_PAD_LEFT,

				FEH_MENU_PAD_LEFT   = 3,
				FEH_MENU_PAD_RIGHT  = FEH_MENU_PAD_LEFT,
				FEH_MENU_PAD_TOP    = FEH_MENU_PAD_LEFT,
				FEH_MENU_PAD_BOTTOM = FEH_MENU_PAD_LEFT,
				FEH_MENU_TOGGLE_PAD = FEH_MENU_PAD_LEFT,

				FEH_MENU_SEP_MAX_H  = 5,

				FEH_MENU_TOGGLE_H   = 7,
				FEH_MENU_TOGGLE_W   = FEH_MENU_TOGGLE_H,

				FEH_MENU_SUBMENU_W  = 9,
				FEH_MENU_SUBMENU_H  = 14,
};


/* moved to structs.h */
/* typedef feh_menu *(*menuitem_func_gen) (feh_menu * m); */

/* *********************************************************************
 * mnu_lvl is a single list of enums broken into three logical groups.
 * MNU_TOP_LVL holds the old feh_menu *menu_main type global menu names.
 * SUB_MNU_LVL holds ALL the submenu entries.
 * CB_LVL      holds the old CB_-type enums.
 *
 * Note1:  It is ONLY the CB_-type codes that have actions associated
 * with them.  All the other codes (above these) just serve to drill
 * down to these actionable items.
 *
 * Note2:  Every one of these mnu_lvl codes are used as the INDEX inside
 * the mnu_text[] array which holds the actual text string to display
 * for each level.
 *
 * Note3:  You may add new entries inside of any of the three groups.
 * Just be sure to then update the mnu_txt[] text assignments and
 * obviously add the new mnu_defs[] entries.
 *
 ************************************************************************* */

enum mnu_lvl {
	IS_CB_ACTION,               /* so gdb shows a name for ZERO */

	SUB_MNU_LVL_BEG,            /* begin "names" of all sub menus */
	IS_SUB_MENU = SUB_MNU_LVL_BEG,
	SUB_DELETE, SUB_EDIT_IN_PLACE, SUB_FILE, SUB_INFO, SUB_OPTIONS,
	SUB_SORT, SUB_BACKGROUND, SUB_BG_DESK, SUB_BG_TILED,
	SUB_BG_SCALED, SUB_BG_CENTERED, SUB_BG_FILLED, SUB_MM_ESC,
	SUB_HELP,
	SUB_MNU_LVL_END,            /* end of all sub menu "names"    */

	CB_LVL_BEG,                 /* these are the old CB_ enums    */
	CB_CLOSE, CB_EXIT, CB_RELOAD, CB_REMOVE, CB_DEL_OK, CB_RESET,
	CB_SORT_FILENAME, CB_SORT_IMAGENAME, CB_SORT_FILESIZE,
	CB_SORT_RANDOMIZE, CB_SORT_REVERSE, CB_SAVE_IMAGE, CB_SAVE_FILELIST,
	CB_FIT, CB_OPT_DRAW_FILENAME, CB_OPT_DRAW_ACTIONS, CB_OPT_KEEP_HTTP,
	CB_OPT_FREEZE_WINDOW, CB_OPT_FULLSCREEN, CB_OPT_BIG_MENUS,
	CB_ROTATE90CW, CB_ROTATE180, CB_ROTATE90CCW, CB_OPT_AUTO_ZOOM,
	CB_IMAGE_FILE, CB_IMAGE_SIZE, CB_IMAGE_DIMS ,
	CB_IMAGE_TYPE, CB_IMAGE_DATE, CB_LIST0_9FLAGS,
	CB_BG_TILED, CB_BG_SCALED, CB_BG_CENTERED,
	CB_BG_FILLED, CB_BG_MAX,    /* MAX setting only set on command line */
	/* these four are for move_mode */
	CB_MM_ESC_IGNORE, CB_MM_ESC_KEEP, CB_MM_DROPIT, CB_MM_BACK,
	CB_HELP_KEYS, CB_HELP_FEH,
	CB_SEPARATOR,               /* no text associated with this one     */
	CB_LVL_END,                 /* used to define mnu_txt[ CB_LVL_END ] */

	MNU_TOP_LVL_BEG,            /* the rest are  top level menu "names" */
	MENU_MAIN, MENU_MM_ESC_OK ,
	MNU_TOP_LVL_END,            /* end of  top level menu names */

};


struct _feh_menu_list {
	feh_menu *menu;
	feh_menu_list *next;
};

/* for setting multiple desktops, I store the "Desktop 1" type text in
 * the mnu_txt[] array at an offest BEYOND the last real element.  This
 * is that offset.
 */
#define MNU_TXT_DESKTOP_OFFSET    ( MNU_TOP_LVL_END - CB_BG_TILED +2 )

/* see menu.c top comments for the changes in struct feh_menu_item  */
struct _feh_menu_item {
	feh_menu_item *next;
	feh_menu_item *prev;
	int state;
	int text_x, icon_x, sub_x, toggle_x;
	int x, y, w, h;

	enum mnu_lvl code;         /* the "text" is stored in mnu_txt[ code ]  */
	enum mnu_lvl sub_code;     /* holds either a submenu or CB_action code */
	enum mnu_lvl flag;         /* either sub_code is  submenu or CB_action */
	unsigned char data;        /* max 0 thru 10 for desktops               */
	unsigned char is_toggle;   /* on/off only for 6 opt.flg toggles        */
	enum mnu_lvl o_sub_code;   /* orig CB_BG_xxx B4 Desktop 0-9 bogosity   */
};

struct _feh_menu {
	enum mnu_lvl code;        /* no longer a text string, just an enum code */
	winwidget w;
	Window win;
	Pixmap pmap;
	int x, y, wide, high;
	int item_w, item_h;
	int visible;
	feh_menu_item *items;
	feh_menu *next;
	feh_menu *prev;
	Imlib_Updates updates;
	Imlib_Image bg;
	int needs_redraw;
	void *data;
	int calc;
};

/* these are the non-static menu functions.  All
 * static funcs are defined inside menu.c
 */
feh_menu_item *feh_menu_find_selected(feh_menu * m);
feh_menu_item *feh_menu_find_at_xy(feh_menu * m, int x, int y);
void feh_menu_deselect_selected(feh_menu * m);
void feh_menu_select(feh_menu * m, feh_menu_item * i);
void feh_menu_show_at(feh_menu * m, int x, int y);
void feh_menu_show_at_xy(feh_menu * m, winwidget winwin, int x, int y);
void feh_menu_hide(feh_menu * m, int free_menu);
void feh_menu_show(feh_menu * m);
void feh_menu_slide_all_menus_relative(int dx, int dy);
void feh_redraw_menus(void);
feh_menu *feh_menu_find(enum mnu_lvl mnu_code );
feh_menu *feh_menu_get_from_window(Window win);
void feh_raise_all_menus(void);
feh_menu_item *feh_menu_find_selected_r(feh_menu * m, feh_menu ** parent);
void feh_menu_select_prev(feh_menu * selected_menu, feh_menu_item * selected_item);
void feh_menu_select_next(feh_menu * selected_menu, feh_menu_item * selected_item);
void feh_menu_item_activate(feh_menu * selected_menu, feh_menu_item * selected_item);
void feh_menu_select_parent(feh_menu * selected_menu);
void feh_menu_select_submenu(feh_menu * selected_menu);
void feh_menu_init_mnu_txt( void );
int feh_menu_init_from_mnu_defs( enum mnu_lvl mnu_code , int first_time );
void move_mode_exit( enum mnu_lvl flag );           /* for move_mode exit control */
void help4feh( const char *help_name);              /* moved from newkeyev.c      */

#endif
