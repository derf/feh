/* events.c

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

#include "feh.h"
#include "winwidget.h"
#include "options.h"
#include "thumbnail.h"

fehbb buttons;

feh_event_handler *ev_handler[LASTEvent];

static void feh_event_handle_ButtonPress(XEvent * ev);
static void feh_event_handle_ButtonRelease(XEvent * ev);
static void feh_event_handle_ConfigureNotify(XEvent * ev);
static void feh_event_handle_LeaveNotify(XEvent * ev);
static void feh_event_handle_MotionNotify(XEvent * ev);
static void feh_event_handle_ClientMessage(XEvent * ev);

static void feh_set_bb(fehbutton *bb, int modifier, char button)
{
	bb->modifier = modifier;
	bb->button = button;
}

static void feh_set_parse_bb_partial(fehbutton *button, char *binding)
{
	char *cur = binding;
	int mod = 0;

	if (!*binding) {
		button->button = 0;
		return;
	}

	while (cur[1] == '-') {
		switch (cur[0]) {
			case 'C':
				mod |= ControlMask;
				break;
			case 'S':
				mod |= ShiftMask;
				break;
			case '1':
				mod |= Mod1Mask;
				break;
			case '4':
				mod |= Mod4Mask;
				break;
			default:
				weprintf("%s modifier %c in \"%s\"",ERR_BUTTONS_INVALID, cur[0], binding);
				break;
		}
		cur += 2;
	}

	button->button = atoi(cur);
	button->modifier = mod;
}

void init_buttonbindings(void)
{
	char *home = NULL;
	char *confhome = NULL;
	char *confpath = mobs(2);
	char line[128];
	char action[32], button[8];
	struct __fehbutton *cur_bb = NULL;
	FILE *conf = NULL;
	int read = 0;

	memset(&buttons, 0, sizeof(buttons));

	feh_set_bb(&buttons.reload, 0, 0);
	feh_set_bb(&buttons.pan,    0, 1);
	feh_set_bb(&buttons.zoom,   0, 2);
	feh_set_bb(&buttons.menu,   0, 3);
	feh_set_bb(&buttons.prev,   0, 4);
	feh_set_bb(&buttons.next,   0, 5);
	feh_set_bb(&buttons.blur,   4, 1);
	feh_set_bb(&buttons.rotate, 4, 2);

	home = getenv("HOME");
	if (!home)
		eprintf("No HOME in environment\n");

	confhome = getenv("XDG_CONFIG_HOME");

	if (confhome)
		STRCAT_2ITEMS(confpath,confhome,"/feh/buttons");
	else
		STRCAT_2ITEMS(confpath,home,"/.config/feh/buttons");

	conf = fopen(confpath, "r");

	if (!conf && ((conf = fopen("/etc/feh/buttons", "r")) == NULL))
		return;

	while (fgets(line, sizeof(line), conf)) {
		*action = '\0';
		*button = '\0';

		read = sscanf(line, "%31s %7s\n", (char *) &action, (char *) &button);

		if ((read == EOF) || (read == 0) || (line[0] == '#'))
			continue;

		if (!strcmp(action, "reload"))
			cur_bb = &buttons.reload;
		else if (!strcmp(action, "pan"))
			cur_bb = &buttons.pan;
		else if (!strcmp(action, "zoom"))
			cur_bb = &buttons.zoom;
		else if (!strcmp(action, "menu"))
			cur_bb = &buttons.menu;
		else if (!strcmp(action, "prev"))
			cur_bb = &buttons.prev;
		else if (!strcmp(action, "next"))
			cur_bb = &buttons.next;
		else if (!strcmp(action, "blur"))
			cur_bb = &buttons.blur;
		else if (!strcmp(action, "rotate"))
			cur_bb = &buttons.rotate;
		else
			weprintf("%s action: %s", ERR_BUTTONS_INVALID, action);

		if (cur_bb)
			feh_set_parse_bb_partial(cur_bb, button);
	}
	fclose(conf);
}

static short feh_is_bb(fehbutton *bb, int button, int mod)
{
	if ((bb->modifier == mod) && (bb->button == button))
		return 1;
	return 0;
}


void feh_event_init(void)
{
	int i;

	for (i = 0; i < LASTEvent; i++)
		ev_handler[i] = NULL;

	ev_handler[KeyPress]        = feh_event_handle_keypress;
	ev_handler[ButtonPress]     = feh_event_handle_ButtonPress;
	ev_handler[ButtonRelease]   = feh_event_handle_ButtonRelease;
	ev_handler[ConfigureNotify] = feh_event_handle_ConfigureNotify;
	ev_handler[LeaveNotify]     = feh_event_handle_LeaveNotify;
	ev_handler[MotionNotify]    = feh_event_handle_MotionNotify;
	ev_handler[ClientMessage]   = feh_event_handle_ClientMessage;

	return;
}

static void feh_event_handle_ButtonPress(XEvent * ev)
{
	winwidget w = NULL;
	int state, button;

	/* get the heck out if it's a mouse-click on the
	   cover, we'll hide the menus on release */
	if (ev->xbutton.window == fgv.mnu.cover) {
		return;
	}

	w = winwidget_get_from_window(ev->xbutton.window);
	if (w == NULL || w->caption_entry)
		return;

	state = ev->xbutton.state & (ControlMask | ShiftMask | Mod1Mask | Mod4Mask);
	button = ev->xbutton.button;

	if (!opt.flg.no_menus && feh_is_bb(&buttons.menu, button, state)) {
		D(("Menu Button Press event\n"));
		winwidget_show_menu(w);

	} else if (feh_is_bb(&buttons.rotate, button, state)
		   && (w->type != WIN_TYPE_THUMBNAIL)) {
		opt.flg.state = STATE_ROTATE;
		D(("rotate starting at %d, %d\n", ev->xbutton.x, ev->xbutton.y));

	} else if (feh_is_bb(&buttons.blur, button, state)
		   && (w->type != WIN_TYPE_THUMBNAIL)) {
		opt.flg.state = STATE_BLUR;
		D(("blur starting at %d, %d\n", ev->xbutton.x, ev->xbutton.y));

	} else if (feh_is_bb(&buttons.pan, button, state)) {
		D(("Next button, but could be pan mode\n"));
		opt.flg.state = STATE_NEXT;
		D(("click offset is %d,%d\n", ev->xbutton.x, ev->xbutton.y));
		w->click_offset_x = ev->xbutton.x - w->im_x;
		w->click_offset_y = ev->xbutton.y - w->im_y;

	} else if (feh_is_bb(&buttons.zoom, button, state)) {
		D(("Zoom Button Press event\n"));
		opt.flg.state = STATE_ZOOM;
		D(("click offset is %d,%d\n", ev->xbutton.x, ev->xbutton.y));
		w->click_offset_x = ev->xbutton.x;
		w->click_offset_y = ev->xbutton.y;
		w->old_zoom = w->zoom;

		/* required to adjust the image position in zoom mode */
		w->im_click_offset_x = (w->click_offset_x
				- w->im_x) / w->old_zoom;
		w->im_click_offset_y = (w->click_offset_y
				- w->im_y) / w->old_zoom;

	} else if (feh_is_bb(&buttons.reload, button, state)) {
		D(("Reload Button Press event\n"));
			feh_reload_image(w,  RESIZE_NO, FORCE_NEW_YES );

	} else if (feh_is_bb(&buttons.prev, button, state)) {
		D(("Prev Button Press event\n"));
		if (w->type == WIN_TYPE_SLIDESHOW)
			slideshow_change_image(w, SLIDE_PREV, RENDER_YES);

	} else if (feh_is_bb(&buttons.next, button, state)) {
		D(("Next Button Press event\n"));
		if (w->type == WIN_TYPE_SLIDESHOW)
			slideshow_change_image(w, SLIDE_NEXT, RENDER_YES);

	} else {
		D(("Received other ButtonPress event\n"));
	}
	return;
}

static void feh_event_handle_ButtonRelease(XEvent * ev)
{
	winwidget w = NULL;
	int state = ev->xbutton.state & (ControlMask | ShiftMask | Mod1Mask | Mod4Mask);
	int button = ev->xbutton.button;
	int sanitize = SANITIZE_NO;

	if (fgv.mnu.root) {
		/* if menus are open, close them, and execute action if needed */

		if (ev->xbutton.window == fgv.mnu.cover) {
			feh_menu_hide(fgv.mnu.root, True);
		} else if (fgv.mnu.root) {
			feh_menu *m;

			if ((m = feh_menu_get_from_window(ev->xbutton.window))) {
				feh_menu_item *i = NULL;

				i = feh_menu_find_selected(m);
				feh_menu_item_activate(m, i);
			}
		}
		return;
	}

	w = winwidget_get_from_window(ev->xbutton.window);
	if (w == NULL || w->caption_entry) {
		return;
	}

	if (feh_is_bb(&buttons.pan, button, state)) {
		if (opt.flg.state == STATE_PAN) {
			D(("Disabling pan mode\n"));
			opt.flg.state = STATE_NORMAL;
			winwidget_render_image(w, 0, 0, SANITIZE_YES);
		} else if (opt.flg.state == STATE_NEXT) {
				opt.flg.state = STATE_NORMAL;
			if (w->type == WIN_TYPE_SLIDESHOW)
				slideshow_change_image(w, SLIDE_NEXT, RENDER_YES);
			else if (w->type == WIN_TYPE_THUMBNAIL) {
				int x, y;

				x = ev->xbutton.x;
				y = ev->xbutton.y;
				x -= w->im_x;
				y -= w->im_y;
				x /= w->zoom;
				y /= w->zoom;
				if ( feh_thumbnail_get_file_from_coords( w->md, x, y) == 0 )
						feh_thumbnail_show_fullsize( w->md ,w->md->cn);
			}
		} else {
			opt.flg.state = STATE_NORMAL;
		}

	} else if (feh_is_bb(&buttons.rotate, button, state)
		      || feh_is_bb(&buttons.zoom, button, state)) {
		D(("Disabling mode\n"));
		opt.flg.state = STATE_NORMAL;

		if ((feh_is_bb(&buttons.zoom, button, state))
				&& (ev->xbutton.x == w->click_offset_x)
				&& (ev->xbutton.y == w->click_offset_y)) {
			w->zoom = 1.0;
			winwidget_center_image(w);
		} else
				sanitize = SANITIZE_YES;

		winwidget_render_image(w, 0, 0, sanitize );

	} else if (feh_is_bb(&buttons.blur, button, state)) {
		D(("Disabling Blur mode\n"));
		opt.flg.state = STATE_NORMAL;
	}
	return;
}

static void feh_event_handle_ConfigureNotify(XEvent * ev)
{
	while (XCheckTypedWindowEvent(fgv.disp, ev->xconfigure.window, ConfigureNotify, ev));
	if (!fgv.mnu.root) {
		winwidget w = winwidget_get_from_window(ev->xconfigure.window);

		if (w) {
			D(("configure size %dx%d\n", ev->xconfigure.width, ev->xconfigure.height));
			if ((w->wide != ev->xconfigure.width)
					|| (w->high != ev->xconfigure.height)) {
				D(("assigning size and rerendering\n"));
				w->wide = ev->xconfigure.width;
				w->high = ev->xconfigure.height;
				w->had_resize = 1;
				if (opt.geom_flags & WidthValue || opt.geom_flags & HeightValue) {
					opt.geom_w = w->wide;
					opt.geom_h = w->high;
				}
				winwidget_render_image(w, 0, 0, SANITIZE_NO);
			}
		}
	}

	return;
}

static void feh_event_handle_LeaveNotify(XEvent * ev)
{
	if ((fgv.mnu.root) && (ev->xcrossing.window == fgv.mnu.root->win)) {
		feh_menu_item *ii;

		D(("It is for a menu\n"));
		for (ii = fgv.mnu.root->items; ii; ii = ii->next) {
			if (MENU_ITEM_IS_SELECTED(ii)) {
				D(("Unselecting menu\n"));
				MENU_ITEM_SET_NORMAL(ii);
				fgv.mnu.root->updates =
					imlib_update_append_rect(fgv.mnu.root->updates, ii->x, ii->y, ii->w, ii->h);
				fgv.mnu.root->needs_redraw = 1;
			}
		}
		feh_raise_all_menus();
	}

	return;
}

static void feh_event_handle_MotionNotify(XEvent * ev)
{
	winwidget w = NULL;
	int dx, dy;
	int scr_width, scr_height;

	scr_width  = fgv.scr->width;
	scr_height = fgv.scr->height;
#ifdef HAVE_LIBXINERAMA
	if (opt.flg.xinerama && fgv.xinerama_screens) {
		scr_width = fgv.xinerama_screens[fgv.xinerama_screen].width;
		scr_height = fgv.xinerama_screens[fgv.xinerama_screen].height;
	}
#endif     /* HAVE_LIBXINERAMA */

	if (fgv.mnu.root) {
		feh_menu *m;
		feh_menu_item *selected_item, *mouseover_item;

		D(("motion notify with menus open\n"));
		while (XCheckTypedWindowEvent(fgv.disp, ev->xmotion.window, MotionNotify, ev));

		if (ev->xmotion.window == fgv.mnu.cover) {
			return;
		} else if ((m = feh_menu_get_from_window(ev->xmotion.window))) {
			selected_item = feh_menu_find_selected(m);
			mouseover_item = feh_menu_find_at_xy(m, ev->xmotion.x, ev->xmotion.y);

			if (selected_item != mouseover_item) {
				D(("selecting a menu item\n"));
				if (selected_item)
					feh_menu_deselect_selected(m);
				if ((mouseover_item) && (mouseover_item->sub_code) )
					feh_menu_select(m, mouseover_item);
			}
			/* check if we are close to the right and/or the bottom edge of the
			 * screen. If so, and if the menu we are currently over is partially
			 * hidden, slide the menu to the left and/or up until it is
			 * fully visible */

			/* FIXME: get this working nicely with xinerama screen edges --
			 * at the moment it does really funky stuff with
			 * scr_{width,height} instead of scr->{width,height} -- pabs*/
			if (mouseover_item
					&& ((fgv.scr->width - (ev->xmotion.x + m->x)) <
						m->wide || (fgv.scr->height - (ev->xmotion.y + m->y)) < m->wide)) {
				dx = scr_width  - (m->x + m->wide);
				dy = scr_height - (m->y + m->high);
				dx = dx < 0 ? dx : 0;
				dy = dy < 0 ? dy : 0;
				if (dx || dy)
					feh_menu_slide_all_menus_relative(dx, dy);
			}
			/* if a submenu is open we want to see that also */
			if (mouseover_item && m->next && ((fgv.scr->width - (ev->xmotion.x + m->next->x))
						< m->next->wide
						|| (fgv.scr->height -
							(ev->xmotion.y + m->next->y)) < m->next->wide)) {
				dx = fgv.scr->width  - (m->next->x + m->next->wide);
				dy = fgv.scr->height - (m->next->y + m->next->high);
				dx = dx < 0 ? dx : 0;
				dy = dy < 0 ? dy : 0;
				if (dx || dy)
					feh_menu_slide_all_menus_relative(dx, dy);
			}
		}
	} else if (opt.flg.state == STATE_ZOOM) {
		while (XCheckTypedWindowEvent(fgv.disp, ev->xmotion.window, MotionNotify, ev));

		w = winwidget_get_from_window(ev->xmotion.window);
		if (w) {
			if (ev->xmotion.x > w->click_offset_x)
				w->zoom = w->old_zoom + (
						((double) ev->xmotion.x - (double) w->click_offset_x)
						/ 128.0);
			else
				w->zoom = w->old_zoom - (
						((double) w->click_offset_x - (double) ev->xmotion.x)
						/ 128.0);

			if (w->zoom < ZOOM_MIN)
				w->zoom = ZOOM_MIN;
			else if (w->zoom > ZOOM_MAX)
				w->zoom = ZOOM_MAX;

			/* center around click_offset */
			w->im_x = w->click_offset_x
					- (w->im_click_offset_x * w->zoom);
			w->im_y = w->click_offset_y
					- (w->im_click_offset_y * w->zoom);

			winwidget_render_image(w, 0, 1, SANITIZE_NO);
		}
	} else if ((opt.flg.state == STATE_PAN) || (opt.flg.state == STATE_NEXT)) {
		int orig_x, orig_y;

		while (XCheckTypedWindowEvent(fgv.disp, ev->xmotion.window, MotionNotify, ev));
		w = winwidget_get_from_window(ev->xmotion.window);
		if (w) {
			if (opt.flg.state == STATE_NEXT) {
				opt.flg.state = STATE_PAN;
			}
			D(("Panning\n"));
			orig_x = w->im_x;
			orig_y = w->im_y;

			w->im_x = ev->xmotion.x - w->click_offset_x;
			w->im_y = ev->xmotion.y - w->click_offset_y;

			winwidget_sanitise_offsets(w);

			D(("im_x %d, im_w %d, off %d, mx %d\n", w->im_x,
				w->im_w, w->click_offset_x, ev->xmotion.x));

			/* XWarpPointer generates a MotionNotify event which we will
			 * parse. Since that event would undo the effect of the pointer
			 * warp, we need to change the click_offset to compensate this.
			 */
			if ((w->wide - ev->xmotion.x <= 1)
					&& (w->click_offset_x >= w->wide - 4))
			{
				XWarpPointer(fgv.disp, None, w->win, 0, 0, 0, 0, 3,
					ev->xmotion.y);
				w->click_offset_x -= w->wide - 4;
			}
			else if ((ev->xmotion.x <= 1) && (w->click_offset_x
					<= (w->im_w * w->zoom) - w->wide - 3))
			{
				XWarpPointer(fgv.disp, None, w->win, 0, 0, 0, 0,
					w->wide - 4, ev->xmotion.y);
				w->click_offset_x += w->wide - 4;
			}
			else if ((w->high - ev->xmotion.y <= 1)
					&& (w->click_offset_y >= w->high - 4))
			{
				XWarpPointer(fgv.disp, None, w->win, 0, 0, 0, 0,
					ev->xmotion.x, 3);
				w->click_offset_y -= w->high - 4;
			}
			else if ((ev->xmotion.y <= 1) && (w->click_offset_y
					<= (w->im_h * w->zoom) - w->high - 3))
			{
				XWarpPointer(fgv.disp, None, w->win, 0, 0, 0, 0,
					ev->xmotion.x, w->high - 4);
				w->click_offset_y += w->high - 4;
			}

			if ((w->im_x != orig_x)
					|| (w->im_y != orig_y))
				winwidget_render_image(w, 0, 1, SANITIZE_NO);
		}
	} else if (opt.flg.state == STATE_ROTATE) {
		while (XCheckTypedWindowEvent(fgv.disp, ev->xmotion.window, MotionNotify, ev));
		w = winwidget_get_from_window(ev->xmotion.window);
		if (w) {
			D(("Rotating\n"));
			if (!w->has_rotated) {
				Imlib_Image temp;

				temp = gib_imlib_create_rotated_image(w->im, 0.0);
				w->im_w = gib_imlib_image_get_width(temp);
				w->im_h = gib_imlib_image_get_height(temp);
				gib_imlib_free_image_and_decache(temp);
				if (!w->full_screen && !opt.geom_flags)
					winwidget_resize(w, w->im_w, w->im_h);
				w->has_rotated = 1;
			}
			w->im_angle = (ev->xmotion.x - w->wide / 2) / ((double) w->wide / 2) * 3.1415926535;
			D(("angle: %f\n", w->im_angle));
			winwidget_render_image(w, 0, 1, SANITIZE_NO);
		}
	} else if (opt.flg.state == STATE_BLUR) {
		while (XCheckTypedWindowEvent(fgv.disp, ev->xmotion.window, MotionNotify, ev));
		w = winwidget_get_from_window(ev->xmotion.window);
		if (w) {
			Imlib_Image temp, ptr;
			signed int blur_radius;

			D(("Blurring\n"));

			temp = gib_imlib_clone_image(w->im);
			blur_radius = (((double) ev->xmotion.x / w->wide) * 20) - 10;
			D(("angle: %d\n", blur_radius));
			if (blur_radius > 0)
				gib_imlib_image_sharpen(temp, blur_radius);
			else
				gib_imlib_image_blur(temp, 0 - blur_radius);
			ptr = w->im;
			w->im = temp;
			winwidget_render_image(w, 0, 1, SANITIZE_NO);
			gib_imlib_free_image_and_decache(w->im);
			w->im = ptr;
		}
	} else {
		while (XCheckTypedWindowEvent(fgv.disp, ev->xmotion.window, MotionNotify, ev));
		w = winwidget_get_from_window(ev->xmotion.window);
		if ((w != NULL) && (w->type == WIN_TYPE_THUMBNAIL)) {
			int x, y;

			x = (ev->xbutton.x - w->im_x) / w->zoom;
			y = (ev->xbutton.y - w->im_y) / w->zoom;
			if ( feh_thumbnail_get_file_from_coords( feh_md, x, y) == 0 )
				   feh_thumbnail_select(w, SLIDE_NO_JUMP);

		}
	}
	return;
}

static void feh_event_handle_ClientMessage(XEvent * ev)
{
	winwidget w = NULL;

	if (ev->xclient.format == 32 && ev->xclient.data.l[0] == (signed) fgv.wmDeleteWindow) {
		w = winwidget_get_from_window(ev->xclient.window);
		if (w)
			winwidget_destroy(w);
	}

	return;
}
