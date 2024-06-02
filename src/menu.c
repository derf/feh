/* menu.c

Copyright (C) 1999-2003 Tom Gilbert.
Copyright (C) 2010-2020 Birte Kristina Friesel.

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
#include "thumbnail.h"
#include "wallpaper.h"
#include "winwidget.h"
#include "filelist.h"
#include "options.h"

Window menu_cover = 0;
feh_menu *menu_root = NULL;
feh_menu *menu_main = NULL;
feh_menu *menu_single_win = NULL;
feh_menu *menu_thumbnail_viewer = NULL;
feh_menu *menu_thumbnail_win = NULL;
feh_menu *menu_bg = NULL;
static feh_menu_list *menus = NULL;
static int common_menus = 0;

static feh_menu *feh_menu_func_gen_info(feh_menu * m);
static void feh_menu_func_free_info(feh_menu * m);
static void feh_menu_func_free_options(feh_menu * m);
static feh_menu *feh_menu_func_gen_options(feh_menu * m);
void feh_menu_cb(feh_menu * m, feh_menu_item * i, int action, unsigned short data);
void feh_menu_cb_opt_fullscreen(feh_menu * m, feh_menu_item * i);

enum {
	CB_CLOSE = 1,
	CB_EXIT,
	CB_RELOAD,
	CB_REMOVE,
	CB_DELETE,
	CB_RESET,
	CB_REMOVE_THUMB,
	CB_DELETE_THUMB,
	CB_BG_TILED,
	CB_BG_SCALED,
	CB_BG_CENTERED,
	CB_BG_FILLED,
	CB_BG_TILED_NOFILE,
	CB_BG_SCALED_NOFILE,
	CB_BG_CENTERED_NOFILE,
	CB_BG_FILLED_NOFILE,
	CB_SORT_FILENAME,
	CB_SORT_IMAGENAME,
	CB_SORT_DIRNAME,
	CB_SORT_MTIME,
	CB_SORT_FILESIZE,
	CB_SORT_RANDOMIZE,
	CB_SAVE_IMAGE,
	CB_SAVE_FILELIST,
	CB_FIT,
	CB_OPT_DRAW_FILENAME,
	CB_OPT_DRAW_ACTIONS,
	CB_OPT_KEEP_HTTP,
	CB_OPT_FREEZE_WINDOW,
	CB_OPT_FULLSCREEN,
	CB_EDIT_ROTATE,
	CB_EDIT_MIRROR,
	CB_EDIT_FLIP,
	CB_OPT_AUTO_ZOOM,
	CB_OPT_KEEP_ZOOM_VP
};

feh_menu *feh_menu_new(void)
{
	feh_menu *m;
	XSetWindowAttributes attr;
	feh_menu_list *l;
	static Imlib_Image bg = NULL;
	static Imlib_Border border;

	m = (feh_menu *) emalloc(sizeof(feh_menu));

	attr.backing_store = NotUseful;
	attr.override_redirect = True;
	attr.colormap = cm;
	attr.border_pixel = 0;
	attr.background_pixmap = None;
	attr.save_under = False;
	attr.do_not_propagate_mask = True;

	m->win = XCreateWindow(
			disp, root, 1, 1, 1, 1, 0, depth, InputOutput, vis,
			CWOverrideRedirect | CWSaveUnder | CWBackingStore
			| CWColormap | CWBackPixmap | CWBorderPixel | CWDontPropagate, &attr);
	XSelectInput(disp, m->win,
			ButtonPressMask | ButtonReleaseMask | EnterWindowMask
			| LeaveWindowMask | PointerMotionMask | ButtonMotionMask);

	m->name = NULL;
	m->fehwin = NULL;
	m->pmap = 0;
	m->x = 0;
	m->y = 0;
	m->w = 0;
	m->h = 0;
	m->visible = 0;
	m->items = NULL;
	m->next = NULL;
	m->prev = NULL;
	m->updates = NULL;
	m->needs_redraw = 1;
	m->func_free = NULL;
	m->data = 0;
	m->calc = 0;
	m->bg = NULL;

	l = emalloc(sizeof(feh_menu_list));
	l->menu = m;
	l->next = menus;
	menus = l;

	if (!bg) {
		feh_load_image_char(&bg, PREFIX "/share/feh/images/menubg_default.png");
		if (bg) {
			border.left = border.right = border.top = border.bottom
				= 4;
			imlib_context_set_image(bg);
			imlib_image_set_border(&border);
		}
	}

	if (bg)
		m->bg = gib_imlib_clone_image(bg);

	return(m);
}

void feh_menu_free(feh_menu * m)
{
	feh_menu_item *i;
	feh_menu_list *l, *pl = NULL;

	if (m->name)
		free(m->name);
	XDestroyWindow(disp, m->win);
	if (m->pmap)
		XFreePixmap(disp, m->pmap);
	if (m->updates)
		imlib_updates_free(m->updates);
	for (i = m->items; i;) {
		feh_menu_item *ii;

		ii = i;
		i = i->next;
		if (ii->text)
			free(ii->text);
		if (ii->submenu)
			free(ii->submenu);
		free(ii);
	}

	for (l = menus; l; l = l->next) {
		if (l->menu == m) {
			if (pl)
				pl->next = l->next;
			else
				menus = l->next;
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
		} else if (i->submenu) {
			mm = feh_menu_find(i->submenu);
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
			if (i->action || i->submenu || i->func_gen_sub || i->text) {
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
				for (ii = selected_menu->items; ii->next; ii = ii->next);
				i = ii;
			}
			if (i->action || i->submenu || i->func_gen_sub || i->text) {
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
			if (i->submenu && !strcmp(i->submenu, selected_menu->name))
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
	if ((i) && (i->action)) {
		feh_menu_hide(menu_root, False);
		feh_main_iteration(0);
		feh_menu_cb(m, i, i->action, i->data);
		if (m->func_free)
			m->func_free(m);
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
	if (i->submenu) {
		feh_menu *mm;

		mm = feh_menu_find(i->submenu);
		if (mm)
			feh_menu_show_at_submenu(mm, m, i);
		else if (i->func_gen_sub)
			feh_menu_show_at_submenu(i->func_gen_sub(m), m, i);
	}
	return;
}

void feh_menu_show_at(feh_menu * m, int x, int y)
{
	if (m->calc)
		feh_menu_calc_size(m);
	if (!menu_cover) {
		XSetWindowAttributes attr;

		D(("creating menu cover window\n"));
		attr.override_redirect = True;
		attr.do_not_propagate_mask = True;
		menu_cover = XCreateWindow(
				disp, root, 0, 0, scr->width,
				scr->height, 0, 0, InputOnly, vis,
				CWOverrideRedirect | CWDontPropagate, &attr);
		XSelectInput(disp, menu_cover,
				KeyPressMask | ButtonPressMask |
				ButtonReleaseMask | EnterWindowMask |
				LeaveWindowMask | PointerMotionMask | ButtonMotionMask);

		XRaiseWindow(disp, menu_cover);
		XMapWindow(disp, menu_cover);
		menu_root = m;
		XUngrabPointer(disp, CurrentTime);
		XSetInputFocus(disp, menu_cover, RevertToPointerRoot, CurrentTime);
	}
	m->visible = 1;
	XMoveWindow(disp, m->win, x, y);
	m->x = x;
	m->y = y;
	XRaiseWindow(disp, m->win);
	feh_menu_redraw(m);
	XMapWindow(disp, m->win);
	return;
}

void feh_menu_show_at_xy(feh_menu * m, winwidget winwid, int x, int y)
{
	if (!m)
		return;

	if (m->calc)
		feh_menu_calc_size(m);
	m->fehwin = winwid;
	if ((x + m->w) > scr->width)
		x = scr->width - m->w;
	if ((y + m->h) > scr->height)
		y = scr->height - m->h;

#if 0
/* #ifdef HAVE_LIBXINERAMA */
/* this doesn't work correctly :( -- pabs */
	if (opt.xinerama && xinerama_screens) {
		if ((x + m->w) > xinerama_screens[xinerama_screen].width)
			x = xinerama_screens[xinerama_screen].width - m->w;
		if ((y + m->h) > xinerama_screens[xinerama_screen].height)
			y = xinerama_screens[xinerama_screen].height - m->h;

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

void feh_menu_show_at_submenu(feh_menu * m, feh_menu * parent_m, feh_menu_item * i)
{
	int mx, my;

	if (!m)
		return;

	if (m->calc)
		feh_menu_calc_size(m);
	mx = parent_m->x + parent_m->w;
	my = parent_m->y + i->y - FEH_MENU_PAD_TOP;
	m->fehwin = parent_m->fehwin;
	parent_m->next = m;
	m->prev = parent_m;
	feh_menu_move(m, mx, my);
	feh_menu_show(m);
	return;
}

void feh_menu_move(feh_menu * m, int x, int y)
{
	if (!m)
		return;

	if (m->visible)
		XMoveWindow(disp, m->win, x, y);

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
		for (m = menus; m; m = m->next) {
			if (m->menu->visible)
				feh_menu_move(m->menu, m->menu->x + stepx, m->menu->y + stepy);

		}
		XWarpPointer(disp, None, None, 0, 0, 0, 0, stepx, stepy);
	}
	return;
}

void feh_menu_hide(feh_menu * m, int func_free)
{
	if (!m->visible)
		return;
	if (m->next) {
		m->next->prev = NULL;
		feh_menu_hide(m->next, func_free);
		m->next = NULL;
	}
	if (m == menu_root) {
		if (menu_cover) {
			D(("DESTROYING menu cover\n"));
			XDestroyWindow(disp, menu_cover);
			menu_cover = 0;
		}
		menu_root = NULL;
	}
	m->visible = 0;
	XUnmapWindow(disp, m->win);
	if (func_free && m->func_free)
		m->func_free(m);
	else
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

feh_menu_item *feh_menu_add_toggle_entry(feh_menu * m, char *text,
		char *submenu, int action,
		unsigned short data, void (*func_free) (void *data), int setting)
{
	feh_menu_item *mi;

	mi = feh_menu_add_entry(m, text, submenu, action, data, func_free);
	mi->is_toggle = TRUE;
	MENU_ITEM_TOGGLE_SET(mi, setting);
	return(mi);
}

feh_menu_item *feh_menu_add_entry(feh_menu * m, char *text, char *submenu,
	int action, unsigned short data, void (*func_free) (void *data))
{
	feh_menu_item *mi, *ptr;


	mi = (feh_menu_item *) emalloc(sizeof(feh_menu_item));
	mi->state = MENU_ITEM_STATE_NORMAL;
	mi->is_toggle = FALSE;
	if (text)
		mi->text = estrdup(text);
	else
		mi->text = NULL;
	if (submenu)
		mi->submenu = estrdup(submenu);
	else
		mi->submenu = NULL;
	mi->action = action;
	mi->func_free = func_free;
	mi->data = data;
	mi->func_gen_sub = NULL;
	mi->next = NULL;
	mi->prev = NULL;

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
}

void feh_menu_entry_get_size(feh_menu_item * i, int *w, int *h)
{
	int tw, th;

	if (i->text) {
		gib_imlib_get_text_size(opt.menu_fn, i->text, NULL, &tw, &th, IMLIB_TEXT_TO_RIGHT);
		*w = tw + FEH_MENUITEM_PAD_LEFT + FEH_MENUITEM_PAD_RIGHT;
		*h = th + FEH_MENUITEM_PAD_TOP + FEH_MENUITEM_PAD_BOTTOM;
	} else {
		*w = FEH_MENUITEM_PAD_LEFT + FEH_MENUITEM_PAD_RIGHT;
		*h = FEH_MENUITEM_PAD_TOP + FEH_MENUITEM_PAD_BOTTOM;
	}

	return;
}

void feh_menu_calc_size(feh_menu * m)
{
	int prev_w, prev_h;
	feh_menu_item *i;
	int j = 0, count = 0, max_w = 0, max_h = 0, next_w = 0;
	int toggle_w = 1;

	prev_w = m->w;
	prev_h = m->h;
	m->calc = 0;

	for (i = m->items; i; i = i->next) {
		int w, h;

		feh_menu_entry_get_size(i, &w, &h);
		if (w > max_w)
			max_w = w;
		if (h > max_h)
			max_h = h;
		if (i->submenu) {
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

	m->h = FEH_MENU_PAD_TOP;
	for (i = m->items; i; i = i->next) {
		i->x = FEH_MENU_PAD_LEFT;
		i->y = m->h;
		i->w = max_w + toggle_w + next_w;
		i->toggle_x = 1;
		i->text_x = i->toggle_x + toggle_w;
		i->sub_x = i->text_x + max_w;
		if (i->text)
			i->h = max_h;
		else
			i->h = FEH_MENU_SEP_MAX_H;
		m->h += i->h;
		j++;
	}
	m->h += FEH_MENU_PAD_BOTTOM;
	m->w = next_w + toggle_w + max_w + FEH_MENU_PAD_LEFT + FEH_MENU_PAD_RIGHT;

	if ((prev_w != m->w) || (prev_h != m->h)) {
		if (m->pmap)
			XFreePixmap(disp, m->pmap);
		m->pmap = 0;
		m->needs_redraw = 1;
		XResizeWindow(disp, m->win, m->w, m->h);
		m->updates = imlib_update_append_rect(m->updates, 0, 0, m->w, m->h);
	}
	D(("menu size calculated. w=%d h=%d\n", m->w, m->h));

	/* Make sure bg is same size */
	if (m->bg) {
		int bg_w, bg_h;

		bg_w = gib_imlib_image_get_width(m->bg);
		bg_h = gib_imlib_image_get_height(m->bg);

		if (m->w != bg_w || m->h != bg_h) {
			Imlib_Image newim = imlib_create_image(m->w, m->h);

			D(("resizing bg to %dx%d\n", m->w, m->h));

			gib_imlib_blend_image_onto_image(newim, m->bg, 0, 0, 0, bg_w, bg_h, 0, 0, m->w, m->h, 0, 0, 1);
			gib_imlib_free_image_and_decache(m->bg);
			m->bg = newim;
		}
	}

	return;
}

void feh_menu_draw_item(feh_menu_item * i, Imlib_Image im, int ox, int oy)
{
	D(("drawing item %p (text %s)\n", i, i->text));

	if (i->text) {
		D(("text item\n"));
		if (MENU_ITEM_IS_SELECTED(i)) {
			D(("selected item\n"));
			/* draw selected image */
			feh_menu_item_draw_at(i->x, i->y, i->w, i->h, im, ox, oy, 1);
		} else {
			D(("unselected item\n"));
			/* draw unselected image */
			feh_menu_item_draw_at(i->x, i->y, i->w, i->h, im, ox, oy, 0);
		}

		/* draw text */
		gib_imlib_text_draw(im, opt.menu_fn, NULL,
				i->x - ox + i->text_x, i->y - oy + FEH_MENUITEM_PAD_TOP,
				i->text, IMLIB_TEXT_TO_RIGHT, 0, 0, 0, 255);
		if (i->submenu) {
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
	} else {
		D(("separator item\n"));
		feh_menu_draw_separator_at(i->x, i->y, i->w, i->h, im, ox, oy);
	}
	return;
}

void feh_menu_redraw(feh_menu * m)
{
	Imlib_Updates u, uu;

	if ((!m->needs_redraw) || (!m->visible) || (!m->updates))
		return;
	m->needs_redraw = 0;
	if (!m->pmap)
		m->pmap = XCreatePixmap(disp, m->win, m->w, m->h, depth);
	XSetWindowBackgroundPixmap(disp, m->win, m->pmap);

	u = imlib_updates_merge_for_rendering(m->updates, m->w, m->h);
	m->updates = NULL;
	if (u) {
		D(("I have updates to render\n"));
		for (uu = u; u; u = imlib_updates_get_next(u)) {
			int x, y, w, h;
			Imlib_Image im;

			imlib_updates_get_coordinates(u, &x, &y, &w, &h);
			D(("update coords %d,%d %d*%d\n", x, y, w, h));
			im = imlib_create_image(w, h);
			gib_imlib_image_fill_rectangle(im, 0, 0, w, h, 0, 0, 0, 0);
			if (im) {
				feh_menu_draw_to_buf(m, im, x, y);
				gib_imlib_render_image_on_drawable(m->pmap, im, x, y, 1, 0, 0);
				gib_imlib_free_image(im);
				XClearArea(disp, m->win, x, y, w, h, False);
			}
		}
		imlib_updates_free(uu);
	}
	return;
}

feh_menu *feh_menu_find(char *name)
{
	feh_menu_list *l;

	for (l = menus; l; l = l->next) {
		if ((l->menu->name) && (!strcmp(l->menu->name, name)))
			return(l->menu);
	}
	return(NULL);
}

void feh_menu_draw_to_buf(feh_menu * m, Imlib_Image im, int ox, int oy)
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

void feh_menu_draw_menu_bg(feh_menu * m, Imlib_Image im, int ox, int oy)
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

void feh_menu_draw_toggle_at(int x, int y, int w, int h, Imlib_Image dst, int ox, int oy, int on)
{
	x -= ox;
	y -= oy;
	if (on)
		gib_imlib_image_fill_rectangle(dst, x, y, w, h, 0, 0, 0, 255);
	else
		gib_imlib_image_draw_rectangle(dst, x, y, w, h, 0, 0, 0, 255);
	return;
}

void feh_menu_draw_submenu_at(int x, int y, Imlib_Image dst, int ox, int oy)
{
        // Draw filled triangle
        x -= ox;
	y -= oy;

	imlib_context_set_image(dst);
	imlib_context_set_color(0, 0, 0, 255);

	for (int i= 0; i <= 3; i++) {
	  imlib_image_draw_line(x+i, y+3+i, x+i, y+9-i, 0);
	}

	return;
}

void feh_menu_draw_separator_at(int x, int y, int w, int h, Imlib_Image dst, int ox, int oy)
{
	gib_imlib_image_fill_rectangle(dst, x - ox + 2, y - oy + 2, w - 4, h - 4, 0, 0, 0, 255);
	return;
}

void feh_menu_item_draw_at(int x, int y, int w, int h, Imlib_Image dst, int ox, int oy, int selected)
{
	imlib_context_set_image(dst);
	if (selected)
		gib_imlib_image_fill_rectangle(dst, x - ox, y - oy, w, h, 127, 127, 127, 178);
	return;
}

void feh_raise_all_menus(void)
{
	feh_menu_list *l;

	for (l = menus; l; l = l->next) {
		if (l->menu->visible)
			XRaiseWindow(disp, l->menu->win);
	}
	return;
}

void feh_redraw_menus(void)
{
	feh_menu_list *l;

	for (l = menus; l; l = l->next) {
		if (l->menu->needs_redraw)
			feh_menu_redraw(l->menu);
	}

	return;
}

feh_menu *feh_menu_get_from_window(Window win)
{
	feh_menu_list *l;

	for (l = menus; l; l = l->next)
		if (l->menu->win == win)
			return(l->menu);
	return(NULL);
}

void feh_menu_init_main(void)
{
	feh_menu *m;
	feh_menu_item *mi;

	if (!common_menus)
		feh_menu_init_common();

	menu_main = feh_menu_new();
	menu_main->name = estrdup("MAIN");

	feh_menu_add_entry(menu_main, "File", "FILE", 0, 0, NULL);
	if (opt.slideshow || opt.multiwindow) {
		feh_menu_add_entry(menu_main, "Sort List", "SORT", 0, 0, NULL);
		mi = feh_menu_add_entry(menu_main, "Image Info", "INFO", 0, 0, NULL);
		mi->func_gen_sub = feh_menu_func_gen_info;
		feh_menu_add_entry(menu_main, NULL, NULL, 0, 0, NULL);
	}
	mi = feh_menu_add_entry(menu_main, "Options", "OPTIONS", 0, 0, NULL);
	mi->func_gen_sub = feh_menu_func_gen_options;

	if (opt.multiwindow)
		feh_menu_add_entry(menu_main, "Close", NULL, CB_CLOSE, 0, NULL);
	feh_menu_add_entry(menu_main, "Exit", NULL, CB_EXIT, 0, NULL);

	m = feh_menu_new();
	m->name = estrdup("FILE");
	feh_menu_add_entry(m, "Reset", NULL, CB_RESET, 0, NULL);
	feh_menu_add_entry(m, "Resize Window", NULL, CB_FIT, 0, NULL);
	feh_menu_add_entry(m, "Reload", NULL, CB_RELOAD, 0, NULL);
	feh_menu_add_entry(m, "Save Image", NULL, CB_SAVE_IMAGE, 0, NULL);
	feh_menu_add_entry(m, "Save List", NULL, CB_SAVE_FILELIST, 0, NULL);
	if (opt.edit) {
		feh_menu_add_entry(m, "Edit in Place", "EDIT", 0, 0, NULL);
	}
	else {
		feh_menu_add_entry(m, "Change View", "EDIT", 0, 0, NULL);
	}
	feh_menu_add_entry(m, "Background", "BACKGROUND", 0, 0, NULL);
	feh_menu_add_entry(m, NULL, NULL, 0, 0, NULL);
	feh_menu_add_entry(m, "Hide", NULL, CB_REMOVE, 0, NULL);
	feh_menu_add_entry(m, "Delete", "CONFIRM", 0, 0, NULL);

	return;
}

void feh_menu_init_common(void)
{
	int num_desks, i;
	char buf[30];
	feh_menu *m;

	if (!opt.menu_fn) {
		opt.menu_fn = gib_imlib_load_font(opt.menu_font);
		if (!opt.menu_fn)
			eprintf
			    ("couldn't load menu font %s, did you make install?\nAre you specifying a nonexistent font?\nDid you tell feh where to find it with --fontpath?",
			     opt.menu_font);
	}

	m = feh_menu_new();
	m->name = estrdup("SORT");

	feh_menu_add_entry(m, "By File Name", NULL, CB_SORT_FILENAME, 0, NULL);
	feh_menu_add_entry(m, "By Image Name", NULL, CB_SORT_IMAGENAME, 0, NULL);
	feh_menu_add_entry(m, "By Directory Name", NULL, CB_SORT_DIRNAME, 0, NULL);
	feh_menu_add_entry(m, "By Modification Date", NULL, CB_SORT_MTIME, 0, NULL);
	if (opt.preload || (opt.sort > SORT_MTIME))
		feh_menu_add_entry(m, "By File Size", NULL, CB_SORT_FILESIZE, 0, NULL);
	feh_menu_add_entry(m, "Randomize", NULL, CB_SORT_RANDOMIZE, 0, NULL);

	m = feh_menu_new();
	m->name = estrdup("CONFIRM");
	feh_menu_add_entry(m, "Confirm", NULL, CB_DELETE, 0, NULL);

	m = feh_menu_new();
	m->name = estrdup("EDIT");
	feh_menu_add_entry(m, "Rotate 90 CW", NULL, CB_EDIT_ROTATE, 1, NULL);
	feh_menu_add_entry(m, "Rotate 180", NULL, CB_EDIT_ROTATE, 2, NULL);
	feh_menu_add_entry(m, "Rotate 90 CCW", NULL, CB_EDIT_ROTATE, 3, NULL);
	feh_menu_add_entry(m, "Mirror", NULL, CB_EDIT_MIRROR, 0, NULL);
	feh_menu_add_entry(m, "Flip", NULL, CB_EDIT_FLIP, 0, NULL);

	menu_bg = feh_menu_new();
	menu_bg->name = estrdup("BACKGROUND");

	num_desks = feh_wm_get_num_desks();
	if (num_desks > 1) {
		feh_menu_add_entry(menu_bg, "Set Tiled", "TILED", 0, 0, NULL);
		feh_menu_add_entry(menu_bg, "Set Scaled", "SCALED", 0, 0, NULL);
		feh_menu_add_entry(menu_bg, "Set Centered", "CENTERED", 0, 0, NULL);
		feh_menu_add_entry(menu_bg, "Set Filled", "FILLED", 0, 0, NULL);

		m = feh_menu_new();
		m->name = estrdup("TILED");
		for (i = 0; i < num_desks; i++) {
			snprintf(buf, sizeof(buf), "Desktop %d", i + 1);
			if (opt.slideshow || opt.multiwindow)
				feh_menu_add_entry(m, buf, NULL, CB_BG_TILED,
						i, NULL);
			else
				feh_menu_add_entry(m, buf, NULL, CB_BG_TILED_NOFILE,
						i, NULL);
		}

		m = feh_menu_new();
		m->name = estrdup("SCALED");
		for (i = 0; i < num_desks; i++) {
			snprintf(buf, sizeof(buf), "Desktop %d", i + 1);

			if (opt.slideshow || opt.multiwindow)
				feh_menu_add_entry(m, buf, NULL, CB_BG_SCALED,
						i, NULL);
			else
				feh_menu_add_entry(m, buf, NULL, CB_BG_SCALED_NOFILE,
						i, NULL);
		}

		m = feh_menu_new();
		m->name = estrdup("CENTERED");
		for (i = 0; i < num_desks; i++) {
			snprintf(buf, sizeof(buf), "Desktop %d", i + 1);
			if (opt.slideshow || opt.multiwindow)
				feh_menu_add_entry(m, buf, NULL,
						CB_BG_CENTERED, i, NULL);
			else
				feh_menu_add_entry(m, buf, NULL,
						CB_BG_CENTERED_NOFILE, i, NULL);
		}

		m = feh_menu_new();
		m->name = estrdup("FILLED");
		for (i = 0; i < num_desks; i++) {
			snprintf(buf, sizeof(buf), "Desktop %d", i + 1);
			if (opt.slideshow || opt.multiwindow)
				feh_menu_add_entry(m, buf, NULL,
						CB_BG_FILLED,
						i, NULL);
			else
				feh_menu_add_entry(m, buf, NULL,
						CB_BG_FILLED_NOFILE,
						i, NULL);
		}
	} else {
		if (opt.slideshow || opt.multiwindow) {
			feh_menu_add_entry(menu_bg, "Set Tiled",
					NULL, CB_BG_TILED, 0, NULL);
			feh_menu_add_entry(menu_bg, "Set Scaled",
					NULL, CB_BG_SCALED, 0, NULL);
			feh_menu_add_entry(menu_bg, "Set Centered",
					NULL, CB_BG_CENTERED, 0, NULL);
			feh_menu_add_entry(menu_bg, "Set Filled",
					NULL, CB_BG_FILLED, 0, NULL);
		} else {
			feh_menu_add_entry(menu_bg, "Set Tiled",
					NULL, CB_BG_TILED_NOFILE, 0, NULL);
			feh_menu_add_entry(menu_bg, "Set Scaled",
					NULL, CB_BG_SCALED_NOFILE, 0, NULL);
			feh_menu_add_entry(menu_bg, "Set Centered",
					NULL, CB_BG_CENTERED_NOFILE, 0, NULL);
			feh_menu_add_entry(menu_bg, "Set Filled",
					NULL, CB_BG_FILLED_NOFILE, 0, NULL);
		}
	}
	common_menus = 1;

	return;
}

void feh_menu_init_single_win(void)
{
	feh_menu *m;
	feh_menu_item *mi;

	if (!common_menus)
		feh_menu_init_common();

	menu_single_win = feh_menu_new();
	menu_single_win->name = estrdup("SINGLEWIN");

	feh_menu_add_entry(menu_single_win, "File", "SINGLEWIN_FILE", 0, 0, NULL);
	m = feh_menu_new();
	m->name = estrdup("SINGLEWIN_FILE");
	feh_menu_add_entry(m, "Reset", NULL, CB_RESET, 0, NULL);
	feh_menu_add_entry(m, "Resize Window", NULL, CB_FIT, 0, NULL);
	feh_menu_add_entry(m, "Reload", NULL, CB_RELOAD, 0, NULL);
	feh_menu_add_entry(m, "Save Image", NULL, CB_SAVE_IMAGE, 0, NULL);
	feh_menu_add_entry(m, "Save List", NULL, CB_SAVE_FILELIST, 0, NULL);
	feh_menu_add_entry(m, "Edit in Place", "EDIT", 0, 0, NULL);
	feh_menu_add_entry(m, "Background", "BACKGROUND", 0, 0, NULL);
	if (opt.multiwindow || opt.slideshow) {
		feh_menu_add_entry(m, NULL, NULL, 0, 0, NULL);
		feh_menu_add_entry(m, "Hide", NULL, CB_REMOVE, 0, NULL);
		feh_menu_add_entry(m, "Delete", "CONFIRM", 0, 0, NULL);
	}

	mi = feh_menu_add_entry(menu_single_win, "Image Info", "INFO", 0, 0, NULL);
	mi->func_gen_sub = feh_menu_func_gen_info;
	feh_menu_add_entry(menu_single_win, NULL, NULL, 0, 0, NULL);
	mi = feh_menu_add_entry(menu_single_win, "Options", "OPTIONS", 0, 0, NULL);
	mi->func_gen_sub = feh_menu_func_gen_options;
	feh_menu_add_entry(menu_single_win, "Close", NULL, CB_CLOSE, 0, NULL);
	feh_menu_add_entry(menu_single_win, "Exit", NULL, CB_EXIT, 0, NULL);

	return;
}

void feh_menu_init_thumbnail_win(void)
{
	feh_menu *m;
	feh_menu_item *mi;

	if (!common_menus)
		feh_menu_init_common();

	menu_thumbnail_win = feh_menu_new();
	menu_thumbnail_win->name = estrdup("THUMBWIN");

	feh_menu_add_entry(menu_thumbnail_win, "File", "THUMBWIN_FILE", 0, 0, NULL);
	m = feh_menu_new();
	m->name = estrdup("THUMBWIN_FILE");
	feh_menu_add_entry(m, "Reset", NULL, CB_RESET, 0, NULL);
	feh_menu_add_entry(m, "Resize Window", NULL, CB_FIT, 0, NULL);
	feh_menu_add_entry(m, "Save Image", NULL, CB_SAVE_IMAGE, 0, NULL);
	feh_menu_add_entry(m, "Save List", NULL, CB_SAVE_FILELIST, 0, NULL);
	feh_menu_add_entry(m, "Background", "BACKGROUND", 0, 0, NULL);
	feh_menu_add_entry(menu_thumbnail_win, NULL, NULL, 0, 0, NULL);
	mi = feh_menu_add_entry(menu_thumbnail_win, "Options", "OPTIONS", 0, 0, NULL);
	mi->func_gen_sub = feh_menu_func_gen_options;
	feh_menu_add_entry(menu_thumbnail_win, "Close", NULL, CB_CLOSE, 0, NULL);
	feh_menu_add_entry(menu_thumbnail_win, "Exit", NULL, CB_EXIT, 0, NULL);
	return;
}

void feh_menu_init_thumbnail_viewer(void)
{
	feh_menu *m;
	feh_menu_item *mi;

	if (!common_menus)
		feh_menu_init_common();

	menu_thumbnail_viewer = feh_menu_new();
	menu_thumbnail_viewer->name = estrdup("THUMBVIEW");

	feh_menu_add_entry(menu_thumbnail_viewer, "File", "THUMBVIEW_FILE",
			0, 0, NULL);
	m = feh_menu_new();
	m->name = estrdup("THUMBVIEW_FILE");
	feh_menu_add_entry(m, "Reset", NULL, CB_RESET, 0, NULL);
	feh_menu_add_entry(m, "Resize Window", NULL, CB_FIT, 0, NULL);
	feh_menu_add_entry(m, "Reload", NULL, CB_RELOAD, 0, NULL);
	feh_menu_add_entry(m, "Save Image", NULL, CB_SAVE_IMAGE, 0, NULL);
	feh_menu_add_entry(m, "Save List", NULL, CB_SAVE_FILELIST, 0, NULL);
	feh_menu_add_entry(m, "Edit in Place", "EDIT", 0, 0, NULL);
	feh_menu_add_entry(m, "Background", "BACKGROUND", 0, 0, NULL);
	feh_menu_add_entry(m, NULL, NULL, 0, 0, NULL);
	feh_menu_add_entry(m, "Hide", NULL, CB_REMOVE_THUMB, 0, NULL);
	feh_menu_add_entry(m, "Delete", "THUMBVIEW_CONFIRM", 0, 0, NULL);
	mi = feh_menu_add_entry(menu_thumbnail_viewer, "Image Info",
			"INFO", 0, 0, NULL);
	mi->func_gen_sub = feh_menu_func_gen_info;
	feh_menu_add_entry(menu_thumbnail_viewer, NULL, NULL, 0, 0, NULL);
	mi = feh_menu_add_entry(menu_thumbnail_viewer, "Options",
			"OPTIONS", 0, 0, NULL);
	mi->func_gen_sub = feh_menu_func_gen_options;
	feh_menu_add_entry(menu_thumbnail_viewer, "Close", NULL, CB_CLOSE, 0, NULL);
	feh_menu_add_entry(menu_thumbnail_viewer, "Exit", NULL, CB_EXIT, 0, NULL);
	m = feh_menu_new();
	m->name = estrdup("THUMBVIEW_CONFIRM");
	feh_menu_add_entry(m, "Confirm", NULL, CB_DELETE_THUMB, 0, NULL);
	return;
}

void feh_menu_cb_opt_fullscreen(feh_menu * m, feh_menu_item * i)
{
	int curr_screen = 0;

	MENU_ITEM_TOGGLE(i);
	if (MENU_ITEM_IS_ON(i))
		m->fehwin->full_screen = TRUE;
	else
		m->fehwin->full_screen = FALSE;

#ifdef HAVE_LIBXINERAMA
	if (opt.xinerama && xinerama_screens) {
		int i, rect[4];

		winwidget_get_geometry(m->fehwin, rect);
		for (i = 0; i < num_xinerama_screens; i++) {
			xinerama_screen = 0;
			if (XY_IN_RECT(rect[0], rect[1],
				       xinerama_screens[i].x_org,
				       xinerama_screens[i].y_org,
				       xinerama_screens[i].width, xinerama_screens[i].height)) {
				curr_screen = xinerama_screen = i;
				break;
			}

		}
		if (opt.xinerama_index >= 0)
			curr_screen = xinerama_screen = opt.xinerama_index;
	}
#endif				/* HAVE_LIBXINERAMA */

	winwidget_destroy_xwin(m->fehwin);
	winwidget_create_window(m->fehwin, m->fehwin->im_w, m->fehwin->im_h);

	winwidget_render_image(m->fehwin, 1, 0);
	winwidget_show(m->fehwin);

#ifdef HAVE_LIBXINERAMA
	/* if we have xinerama and we're using it, then full screen the window
	 * on the head that the window was active on */
	if (m->fehwin->full_screen == TRUE && opt.xinerama && xinerama_screens) {
		xinerama_screen = curr_screen;
		winwidget_move(m->fehwin, xinerama_screens[curr_screen].x_org, xinerama_screens[curr_screen].y_org);
	}
#endif				/* HAVE_LIBXINERAMA */
}

void feh_menu_cb(feh_menu * m, feh_menu_item * i, int action, unsigned short data)
{
	char *path;

	switch (action) {
		case CB_BG_TILED:
			path = FEH_FILE(m->fehwin->file->data)->filename;
			feh_wm_set_bg(path, m->fehwin->im, 0, 0, 0, data, 0);
			break;
		case CB_BG_SCALED:
			path = FEH_FILE(m->fehwin->file->data)->filename;
			feh_wm_set_bg(path, m->fehwin->im, 0, 1, 0, data, 0);
			break;
		case CB_BG_CENTERED:
			path = FEH_FILE(m->fehwin->file->data)->filename;
			feh_wm_set_bg(path, m->fehwin->im, 1, 0, 0, data, 0);
			break;
		case CB_BG_FILLED:
			path = FEH_FILE(m->fehwin->file->data)->filename;
			feh_wm_set_bg(path, m->fehwin->im, 0, 0, 1, data, 0);
			break;
		case CB_BG_TILED_NOFILE:
			feh_wm_set_bg(NULL, m->fehwin->im, 0, 0, 0, data, 0);
			break;
		case CB_BG_SCALED_NOFILE:
			feh_wm_set_bg(NULL, m->fehwin->im, 0, 1, 0, data, 0);
			break;
		case CB_BG_CENTERED_NOFILE:
			feh_wm_set_bg(NULL, m->fehwin->im, 1, 0, 0, data, 0);
			break;
		case CB_BG_FILLED_NOFILE:
			feh_wm_set_bg(NULL, m->fehwin->im, 0, 0, 1, data, 0);
			break;
		case CB_CLOSE:
			winwidget_destroy(m->fehwin);
			break;
		case CB_EXIT:
			winwidget_destroy_all();
			break;
		case CB_RESET:
			if (m->fehwin->has_rotated) {
				m->fehwin->im_w = gib_imlib_image_get_width(m->fehwin->im);
				m->fehwin->im_h = gib_imlib_image_get_height(m->fehwin->im);
				winwidget_resize(m->fehwin, m->fehwin->im_w, m->fehwin->im_h, 0);
			}
			winwidget_reset_image(m->fehwin);
			winwidget_render_image(m->fehwin, 1, 0);
			break;
		case CB_RELOAD:
			feh_reload_image(m->fehwin, 0, 1);
			break;
		case CB_REMOVE:
			feh_filelist_image_remove(m->fehwin, 0);
			break;
		case CB_DELETE:
			feh_filelist_image_remove(m->fehwin, 1);
			break;
		case CB_REMOVE_THUMB:
			feh_thumbnail_mark_removed(FEH_FILE(m->fehwin->file->data), 0);
			feh_filelist_image_remove(m->fehwin, 0);
			break;
		case CB_DELETE_THUMB:
			feh_thumbnail_mark_removed(FEH_FILE(m->fehwin->file->data), 1);
			feh_filelist_image_remove(m->fehwin, 1);
			break;
		case CB_SORT_FILENAME:
			filelist = gib_list_sort(filelist, feh_cmp_filename);
			if (opt.jump_on_resort) {
				slideshow_change_image(m->fehwin, SLIDE_FIRST, 1);
			}
			break;
		case CB_SORT_IMAGENAME:
			filelist = gib_list_sort(filelist, feh_cmp_name);
			if (opt.jump_on_resort) {
				slideshow_change_image(m->fehwin, SLIDE_FIRST, 1);
			}
			break;
		case CB_SORT_DIRNAME:
			filelist = gib_list_sort(filelist, feh_cmp_dirname);
			if (opt.jump_on_resort) {
				slideshow_change_image(m->fehwin, SLIDE_FIRST, 1);
			}
			break;
		case CB_SORT_MTIME:
			filelist = gib_list_sort(filelist, feh_cmp_mtime);
			if (opt.jump_on_resort) {
				slideshow_change_image(m->fehwin, SLIDE_FIRST, 1);
			}
			break;
		case CB_SORT_FILESIZE:
			filelist = gib_list_sort(filelist, feh_cmp_size);
			if (opt.jump_on_resort) {
				slideshow_change_image(m->fehwin, SLIDE_FIRST, 1);
			}
			break;
		case CB_SORT_RANDOMIZE:
			filelist = gib_list_randomize(filelist);
			if (opt.jump_on_resort) {
				slideshow_change_image(m->fehwin, SLIDE_FIRST, 1);
			}
			break;
		case CB_FIT:
			winwidget_size_to_image(m->fehwin);
			break;
		case CB_EDIT_ROTATE:
			feh_edit_inplace(m->fehwin, data);
			break;
		case CB_EDIT_MIRROR:
			feh_edit_inplace(m->fehwin, INPLACE_EDIT_MIRROR);
			break;
		case CB_EDIT_FLIP:
			feh_edit_inplace(m->fehwin, INPLACE_EDIT_FLIP);
			break;
		case CB_SAVE_IMAGE:
			slideshow_save_image(m->fehwin);
			break;
		case CB_SAVE_FILELIST:
			feh_save_filelist();
			break;
		case CB_OPT_DRAW_FILENAME:
			MENU_ITEM_TOGGLE(i);
			if (MENU_ITEM_IS_ON(i))
				opt.draw_filename = TRUE;
			else
				opt.draw_filename = FALSE;
			winwidget_rerender_all(0);
			break;
		case CB_OPT_DRAW_ACTIONS:
			MENU_ITEM_TOGGLE(i);
			if (MENU_ITEM_IS_ON(i))
				opt.draw_actions = TRUE;
			else
				opt.draw_actions = FALSE;
			winwidget_rerender_all(0);
			break;
		case CB_OPT_KEEP_HTTP:
			MENU_ITEM_TOGGLE(i);
			if (MENU_ITEM_IS_ON(i))
				opt.keep_http = TRUE;
			else
				opt.keep_http = FALSE;
			break;
		case CB_OPT_FREEZE_WINDOW:
			MENU_ITEM_TOGGLE(i);
			if (MENU_ITEM_IS_ON(i)) {
				opt.geom_flags = (WidthValue | HeightValue);
				opt.geom_w = m->fehwin->w;
				opt.geom_h = m->fehwin->h;
			} else {
				opt.geom_flags = 0;
			}
			break;
		case CB_OPT_FULLSCREEN:
			feh_menu_cb_opt_fullscreen(m, i);
			break;
		case CB_OPT_AUTO_ZOOM:
			MENU_ITEM_TOGGLE(i);
			if (MENU_ITEM_IS_ON(i))
				opt.zoom_mode = ZOOM_MODE_MAX;
			else
				opt.zoom_mode = 0;
			winwidget_rerender_all(1);
			break;
		case CB_OPT_KEEP_ZOOM_VP:
			MENU_ITEM_TOGGLE(i);
			if (MENU_ITEM_IS_ON(i))
				opt.keep_zoom_vp = 1;
			else
				opt.keep_zoom_vp = 0;
			break;
	}
	return;
}

static feh_menu *feh_menu_func_gen_info(feh_menu * m)
{
	Imlib_Image im;
	feh_menu *mm;
	feh_file *file;
	char buffer[400];

	if (!m->fehwin->file)
		return(NULL);
	file = FEH_FILE(m->fehwin->file->data);
	im = m->fehwin->im;
	if (!im)
		return(NULL);
	mm = feh_menu_new();
	mm->name = estrdup("INFO");
	snprintf(buffer, sizeof(buffer), "Filename: %s", file->name);
	feh_menu_add_entry(mm, buffer, NULL, 0, 0, NULL);
	if (!file->info)
		feh_file_info_load(file, im);
	if (file->info) {
		snprintf(buffer, sizeof(buffer), "Size: %dKb", file->size / 1024);
		feh_menu_add_entry(mm, buffer, NULL, 0, 0, NULL);
		snprintf(buffer, sizeof(buffer), "Dimensions: %dx%d", file->info->width, file->info->height);
		feh_menu_add_entry(mm, buffer, NULL, 0, 0, NULL);
		snprintf(buffer, sizeof(buffer), "Type: %s", file->info->format);
		feh_menu_add_entry(mm, buffer, NULL, 0, 0, NULL);
	}

	mm->func_free = feh_menu_func_free_info;
	return(mm);
}

static void feh_menu_func_free_info(feh_menu * m)
{
	feh_menu_free(m);
	return;
}

static feh_menu *feh_menu_func_gen_options(feh_menu * m)
{
	feh_menu *mm;

	mm = feh_menu_new();
	mm->name = estrdup("OPTIONS");
	mm->fehwin = m->fehwin;
	feh_menu_add_toggle_entry(mm, "Auto-Zoom", NULL, CB_OPT_AUTO_ZOOM,
				0, NULL, opt.zoom_mode);
	feh_menu_add_toggle_entry(mm, "Freeze Window Size", NULL,
				CB_OPT_FREEZE_WINDOW, 0, NULL, opt.geom_flags);
	feh_menu_add_toggle_entry(mm, "Fullscreen", NULL,
				CB_OPT_FULLSCREEN, 0, NULL, m->fehwin->full_screen);
	feh_menu_add_toggle_entry(mm, "Keep viewport zoom & pos", NULL,
				CB_OPT_KEEP_ZOOM_VP, 0, NULL, opt.keep_zoom_vp);

	feh_menu_add_entry(mm, NULL, NULL, 0, 0, NULL);

	feh_menu_add_toggle_entry(mm, "Draw Filename", NULL,
				CB_OPT_DRAW_FILENAME, 0, NULL, opt.draw_filename);
	feh_menu_add_toggle_entry(mm, "Draw Actions", NULL,
				CB_OPT_DRAW_ACTIONS, 0, NULL, opt.draw_actions);
	feh_menu_add_toggle_entry(mm, "Keep HTTP Files", NULL,
				CB_OPT_KEEP_HTTP, 0, NULL, opt.keep_http);
	mm->func_free = feh_menu_func_free_options;
	return(mm);
}

static void feh_menu_func_free_options(feh_menu * m)
{
	feh_menu_free(m);
	return;
}
