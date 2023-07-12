/* events.c

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
#include "filelist.h"
#include "winwidget.h"
#include "timers.h"
#include "options.h"
#include "events.h"
#include "thumbnail.h"

#define FEH_JITTER_OFFSET 2
#define FEH_JITTER_TIME 1

extern struct __fehkey keys[EVENT_LIST_END];
fehkey *feh_str_to_kb(char *action);

feh_event_handler *ev_handler[LASTEvent];

static void feh_event_handle_ButtonPress(XEvent * ev);
static void feh_event_handle_ButtonRelease(XEvent * ev);
static void feh_event_handle_LeaveNotify(XEvent * ev);
static void feh_event_handle_MotionNotify(XEvent * ev);
static void feh_event_handle_ClientMessage(XEvent * ev);

static void feh_set_bb(unsigned int bb_index, int modifier, char button)
{
	keys[bb_index].state  = modifier;
	keys[bb_index].button = button;
}

static void feh_set_parse_bb_partial(fehkey *button, char *binding)
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
				weprintf("buttons: invalid modifier %c in \"%s\"", cur[0], binding);
				break;
		}
		cur += 2;
	}

	button->button = atoi(cur);
	button->state  = mod;

	if (button->button == 0) {
		/*
		 * Mod3 is unused on today's keyboards. If Mod3 is unset and button==0,
		 * we are dealing with an uninitialized or unset binding. If Mod3 is set
		 * and button==0, it refers to mouse movement.
		 */
		button->state |= Mod3Mask;
	}
}

/*
 * Called after init_keyevents in keyevents.c
 * -> no need to memset
 */
void init_buttonbindings(void)
{
	char *home = NULL;
	char *confhome = NULL;
	char *confpath = NULL;
	char line[128];
	char action[32], button[8];
	struct __fehkey *cur_bb = NULL;
	FILE *conf = NULL;
	int read = 0;

	feh_set_bb(EVENT_pan,         0, 1);
	feh_set_bb(EVENT_zoom,        0, 2);
	feh_set_bb(EVENT_toggle_menu, 0, 3);
	feh_set_bb(EVENT_prev_img,    0, 4);
	feh_set_bb(EVENT_next_img,    0, 5);
	feh_set_bb(EVENT_blur,        4, 1);
	feh_set_bb(EVENT_rotate,      4, 2);

	home = getenv("HOME");
	confhome = getenv("XDG_CONFIG_HOME");

	if (confhome)
		confpath = estrjoin("/", confhome, "feh/buttons", NULL);
	else if (home)
		confpath = estrjoin("/", home, ".config/feh/buttons", NULL);
	else
		return;

	conf = fopen(confpath, "r");

	free(confpath);

	if (!conf && ((conf = fopen("/etc/feh/buttons", "r")) == NULL))
		return;

	while (fgets(line, sizeof(line), conf)) {
		*action = '\0';
		*button = '\0';
		cur_bb = NULL;

		read = sscanf(line, "%31s %7s\n", (char *) &action, (char *) &button);

		if ((read == EOF) || (read == 0) || (line[0] == '#'))
			continue;

		cur_bb = feh_str_to_kb(action);
		if (cur_bb == NULL) {
			if (!strcmp(action, "reload"))
				cur_bb = &keys[EVENT_reload_image];
			else if (!strcmp(action, "menu"))
				cur_bb = &keys[EVENT_toggle_menu];
			else if (!strcmp(action, "prev"))
				cur_bb = &keys[EVENT_prev_img];
			else if (!strcmp(action, "next"))
				cur_bb = &keys[EVENT_next_img];
		}
		if (cur_bb)
			feh_set_parse_bb_partial(cur_bb, button);
		else
			weprintf("buttons: Invalid action: %s", action);
	}
	fclose(conf);
}

static short feh_is_bb(unsigned int key_index, unsigned int button, unsigned int mod)
{
	if ((keys[key_index].state == mod) && (keys[key_index].button == button))
		return 1;
	return 0;
}


void feh_event_init(void)
{
	int i;

	for (i = 0; i < LASTEvent; i++)
		ev_handler[i] = NULL;

	ev_handler[KeyPress] = feh_event_handle_keypress;
	ev_handler[ButtonPress] = feh_event_handle_ButtonPress;
	ev_handler[ButtonRelease] = feh_event_handle_ButtonRelease;
	ev_handler[ConfigureNotify] = feh_event_handle_ConfigureNotify;
	ev_handler[LeaveNotify] = feh_event_handle_LeaveNotify;
	ev_handler[MotionNotify] = feh_event_handle_MotionNotify;
	ev_handler[ClientMessage] = feh_event_handle_ClientMessage;

	return;
}

static void feh_event_handle_ButtonPress(XEvent * ev)
{
	winwidget winwid = NULL;
	unsigned int state, button;

	/* get the heck out if it's a mouse-click on the
	   cover, we'll hide the menus on release */
	if (ev->xbutton.window == menu_cover) {
		return;
	}

	winwid = winwidget_get_from_window(ev->xbutton.window);
	if (winwid == NULL || winwid->caption_entry) {
		return;
	}

	state = ev->xbutton.state & (ControlMask | ShiftMask | Mod1Mask | Mod4Mask);
	button = ev->xbutton.button;

	if (!opt.no_menus && feh_is_bb(EVENT_toggle_menu, button, state)) {
		D(("Menu Button Press event\n"));
		winwidget_show_menu(winwid);

	} else if (feh_is_bb(EVENT_rotate, button, state)
		   && (winwid->type != WIN_TYPE_THUMBNAIL)) {
		opt.mode = MODE_ROTATE;
		winwid->mode = MODE_ROTATE;
		D(("rotate starting at %d, %d\n", ev->xbutton.x, ev->xbutton.y));

	} else if (feh_is_bb(EVENT_blur, button, state)
		   && (winwid->type != WIN_TYPE_THUMBNAIL)) {
		opt.mode = MODE_BLUR;
		winwid->mode = MODE_BLUR;
		D(("blur starting at %d, %d\n", ev->xbutton.x, ev->xbutton.y));

	} else if (feh_is_bb(EVENT_pan, button, state)) {
		D(("Next button, but could be pan mode\n"));
		opt.mode = MODE_NEXT;
		winwid->mode = MODE_NEXT;
		D(("click offset is %d,%d\n", ev->xbutton.x, ev->xbutton.y));
		winwid->click_offset_x = ev->xbutton.x - winwid->im_x;
		winwid->click_offset_y = ev->xbutton.y - winwid->im_y;
		winwid->click_start_time = time(NULL);

	} else if (feh_is_bb(EVENT_zoom, button, state)) {
		D(("Zoom Button Press event\n"));
		opt.mode = MODE_ZOOM;
		winwid->mode = MODE_ZOOM;
		D(("click offset is %d,%d\n", ev->xbutton.x, ev->xbutton.y));
		winwid->click_offset_x = ev->xbutton.x;
		winwid->click_offset_y = ev->xbutton.y;
		winwid->old_zoom = winwid->zoom;

		/* required to adjust the image position in zoom mode */
		winwid->im_click_offset_x = (winwid->click_offset_x
				- winwid->im_x) / winwid->old_zoom;
		winwid->im_click_offset_y = (winwid->click_offset_y
				- winwid->im_y) / winwid->old_zoom;

	} else if (feh_is_bb(EVENT_zoom_in, button, state)) {
		D(("Zoom_In Button Press event\n"));
		D(("click offset is %d,%d\n", ev->xbutton.x, ev->xbutton.y));
		winwid->click_offset_x = ev->xbutton.x;
		winwid->click_offset_y = ev->xbutton.y;
		winwid->old_zoom = winwid->zoom;

		/* required to adjust the image position in zoom mode */
		winwid->im_click_offset_x = (winwid->click_offset_x
				- winwid->im_x) / winwid->old_zoom;
		winwid->im_click_offset_y = (winwid->click_offset_y
				- winwid->im_y) / winwid->old_zoom;

		/* copied from zoom_in, keyevents.c */
		winwid->zoom = winwid->zoom * opt.zoom_rate;

		if (winwid->zoom > ZOOM_MAX)
			winwid->zoom = ZOOM_MAX;

		/* copied from below (ZOOM, feh_event_handle_MotionNotify) */
		winwid->im_x = winwid->click_offset_x
				- (winwid->im_click_offset_x * winwid->zoom);
		winwid->im_y = winwid->click_offset_y
				- (winwid->im_click_offset_y * winwid->zoom);

		winwidget_sanitise_offsets(winwid);
		winwidget_render_image(winwid, 0, 0);

	} else if (feh_is_bb(EVENT_zoom_out, button, state)) {
		D(("Zoom_Out Button Press event\n"));
		D(("click offset is %d,%d\n", ev->xbutton.x, ev->xbutton.y));
		winwid->click_offset_x = ev->xbutton.x;
		winwid->click_offset_y = ev->xbutton.y;
		winwid->old_zoom = winwid->zoom;

		/* required to adjust the image position in zoom mode */
		winwid->im_click_offset_x = (winwid->click_offset_x
				- winwid->im_x) / winwid->old_zoom;
		winwid->im_click_offset_y = (winwid->click_offset_y
				- winwid->im_y) / winwid->old_zoom;

		/* copied from zoom_out, keyevents.c */
		winwid->zoom = winwid->zoom / opt.zoom_rate;

		if (winwid->zoom < ZOOM_MIN)
			winwid->zoom = ZOOM_MIN;

		/* copied from below (ZOOM, feh_event_handle_MotionNotify) */
		winwid->im_x = winwid->click_offset_x
				- (winwid->im_click_offset_x * winwid->zoom);
		winwid->im_y = winwid->click_offset_y
				- (winwid->im_click_offset_y * winwid->zoom);

		winwidget_sanitise_offsets(winwid);
		winwidget_render_image(winwid, 0, 0);

	} else if (feh_is_bb(EVENT_reload_image, button, state)) {
		D(("Reload Button Press event\n"));
			feh_reload_image(winwid, 0, 1);

	} else if (feh_is_bb(EVENT_prev_img, button, state)) {
		D(("Prev Button Press event\n"));
		if (winwid->type == WIN_TYPE_SLIDESHOW)
			slideshow_change_image(winwid, SLIDE_PREV, 1);

	} else if (feh_is_bb(EVENT_next_img, button, state)) {
		D(("Next Button Press event\n"));
		if (winwid->type == WIN_TYPE_SLIDESHOW)
			slideshow_change_image(winwid, SLIDE_NEXT, 1);

	} else {
		D(("Received other ButtonPress event\n"));
		feh_event_handle_generic(winwid, state, NoSymbol, button);
	}
	return;
}

static void feh_event_handle_ButtonRelease(XEvent * ev)
{
	winwidget winwid = NULL;
	unsigned int state = ev->xbutton.state & (ControlMask | ShiftMask | Mod1Mask | Mod4Mask);
	unsigned int button = ev->xbutton.button;

	if (menu_root) {
		/* if menus are open, close them, and execute action if needed */

		if (ev->xbutton.window == menu_cover) {
			feh_menu_hide(menu_root, True);
		} else if (menu_root) {
			feh_menu *m;

			if ((m = feh_menu_get_from_window(ev->xbutton.window))) {
				feh_menu_item *i = NULL;

				i = feh_menu_find_selected(m);
				feh_menu_item_activate(m, i);
			}
		}
		return;
	}

	winwid = winwidget_get_from_window(ev->xbutton.window);
	if (winwid == NULL || winwid->caption_entry) {
		return;
	}

	if (feh_is_bb(EVENT_pan, button, state)) {
		if (opt.mode == MODE_PAN) {
			D(("Disabling pan mode\n"));
			opt.mode = MODE_NORMAL;
			winwid->mode = MODE_NORMAL;
			winwidget_sanitise_offsets(winwid);
			winwidget_render_image(winwid, 0, 0);
		} else if (opt.mode == MODE_NEXT) {
			opt.mode = MODE_NORMAL;
			winwid->mode = MODE_NORMAL;
			if (winwid->type == WIN_TYPE_SLIDESHOW)
				slideshow_change_image(winwid, SLIDE_NEXT, 1);
			else if (winwid->type == WIN_TYPE_THUMBNAIL) {
				feh_file *thumbfile;
				int x, y;

				x = ev->xbutton.x;
				y = ev->xbutton.y;
				x -= winwid->im_x;
				y -= winwid->im_y;
				x /= winwid->zoom;
				y /= winwid->zoom;
				thumbfile = feh_thumbnail_get_file_from_coords(x, y);
				if (thumbfile) {
					if (opt.actions[0]) {
						feh_action_run(thumbfile, opt.actions[0], winwid);
						if (!opt.hold_actions[0])
							feh_thumbnail_mark_removed(thumbfile, 0);
					} else {
						feh_thumbnail_show_fullsize(thumbfile);
					}
				}
			}
		} else {
			opt.mode = MODE_NORMAL;
			winwid->mode = MODE_NORMAL;
		}

	} else if (feh_is_bb(EVENT_rotate, button, state)
			|| feh_is_bb(EVENT_zoom, button, state)) {
		D(("Disabling mode\n"));
		opt.mode = MODE_NORMAL;
		winwid->mode = MODE_NORMAL;

		if ((feh_is_bb(EVENT_zoom, button, state))
				&& (ev->xbutton.x == winwid->click_offset_x)
				&& (ev->xbutton.y == winwid->click_offset_y)) {
			winwid->zoom = 1.0;
			winwidget_center_image(winwid);
		} else
			winwidget_sanitise_offsets(winwid);

		winwidget_render_image(winwid, 0, 0);

	} else if (feh_is_bb(EVENT_blur, button, state)) {
		D(("Disabling Blur mode\n"));
		opt.mode = MODE_NORMAL;
		winwid->mode = MODE_NORMAL;
	}
	return;
}

void feh_event_handle_ConfigureNotify(XEvent * ev)
{
	while (XCheckTypedWindowEvent(disp, ev->xconfigure.window, ConfigureNotify, ev));
	if (!menu_root) {
		winwidget w = winwidget_get_from_window(ev->xconfigure.window);

		if (w) {
			D(("configure size %dx%d\n", ev->xconfigure.width, ev->xconfigure.height));
			if ((w->w != ev->xconfigure.width)
					|| (w->h != ev->xconfigure.height)) {
				D(("assigning size and rerendering\n"));
				w->w = ev->xconfigure.width;
				w->h = ev->xconfigure.height;
				w->had_resize = 1;
				if (opt.geom_flags & WidthValue || opt.geom_flags & HeightValue) {
					opt.geom_w = w->w;
					opt.geom_h = w->h;
				}
				winwidget_render_image(w, 0, 0);
			}
		}
	}

	return;
}

static void feh_event_handle_LeaveNotify(XEvent * ev)
{
	if ((menu_root) && (ev->xcrossing.window == menu_root->win)) {
		feh_menu_item *ii;

		D(("It is for a menu\n"));
		for (ii = menu_root->items; ii; ii = ii->next) {
			if (MENU_ITEM_IS_SELECTED(ii)) {
				D(("Unselecting menu\n"));
				MENU_ITEM_SET_NORMAL(ii);
				menu_root->updates =
					imlib_update_append_rect(menu_root->updates, ii->x, ii->y, ii->w, ii->h);
				menu_root->needs_redraw = 1;
			}
		}
		feh_raise_all_menus();
	}

	return;
}

static void feh_event_handle_MotionNotify(XEvent * ev)
{
	winwidget winwid = NULL;
	int dx, dy;
	int scr_width, scr_height;

	scr_width = scr->width;
	scr_height = scr->height;
#ifdef HAVE_LIBXINERAMA
	if (opt.xinerama && xinerama_screens) {
		scr_width = xinerama_screens[xinerama_screen].width;
		scr_height = xinerama_screens[xinerama_screen].height;
	}
#endif				/* HAVE_LIBXINERAMA */

	if (menu_root) {
		feh_menu *m;
		feh_menu_item *selected_item, *mouseover_item;

		D(("motion notify with menus open\n"));
		while (XCheckTypedWindowEvent(disp, ev->xmotion.window, MotionNotify, ev));

		if (ev->xmotion.window == menu_cover) {
			return;
		} else if ((m = feh_menu_get_from_window(ev->xmotion.window))) {
			selected_item = feh_menu_find_selected(m);
			mouseover_item = feh_menu_find_at_xy(m, ev->xmotion.x, ev->xmotion.y);

			if (selected_item != mouseover_item) {
				D(("selecting a menu item\n"));
				if (selected_item)
					feh_menu_deselect_selected(m);
				if ((mouseover_item)
						&& ((mouseover_item->action)
							|| (mouseover_item->submenu)
							|| (mouseover_item->func_gen_sub)))
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
					&& ((scr->width - (ev->xmotion.x + m->x)) <
						m->w || (scr->height - (ev->xmotion.y + m->y)) < m->w)) {
				dx = scr_width - (m->x + m->w);
				dy = scr_height - (m->y + m->h);
				dx = dx < 0 ? dx : 0;
				dy = dy < 0 ? dy : 0;
				dx = m->x + dx < 0 ? -m->x : dx;
				dy = m->y + dy < 0 ? -m->y : dy;
				if (dx || dy)
					feh_menu_slide_all_menus_relative(dx, dy);
			}
			/* if a submenu is open we want to see that also */
			if (mouseover_item && m->next && ((scr->width - (ev->xmotion.x + m->next->x))
						< m->next->w
						|| (scr->height -
							(ev->xmotion.y + m->next->y)) < m->next->w)) {
				dx = scr->width - (m->next->x + m->next->w);
				dy = scr->height - (m->next->y + m->next->h);
				dx = dx < 0 ? dx : 0;
				dy = dy < 0 ? dy : 0;
				dx = m->x + dx < 0 ? -m->x : dx;
				dy = m->y + dy < 0 ? -m->y : dy;
				if (dx || dy)
					feh_menu_slide_all_menus_relative(dx, dy);
			}
		}
	} else if (opt.mode == MODE_ZOOM) {
		while (XCheckTypedWindowEvent(disp, ev->xmotion.window, MotionNotify, ev));

		winwid = winwidget_get_from_window(ev->xmotion.window);
		if (winwid) {
			if (ev->xmotion.x > winwid->click_offset_x)
				winwid->zoom = winwid->old_zoom + (
						((double) ev->xmotion.x - (double) winwid->click_offset_x)
						/ 128.0);
			else
				winwid->zoom = winwid->old_zoom - (
						((double) winwid->click_offset_x - (double) ev->xmotion.x)
						/ 128.0);

			if (winwid->zoom < ZOOM_MIN)
				winwid->zoom = ZOOM_MIN;
			else if (winwid->zoom > ZOOM_MAX)
				winwid->zoom = ZOOM_MAX;

			/* center around click_offset */
			winwid->im_x = winwid->click_offset_x
					- (winwid->im_click_offset_x * winwid->zoom);
			winwid->im_y = winwid->click_offset_y
					- (winwid->im_click_offset_y * winwid->zoom);

			winwidget_render_image(winwid, 0, 1);
		}
	} else if ((opt.mode == MODE_PAN) || (opt.mode == MODE_NEXT)) {
		int orig_x, orig_y;

		while (XCheckTypedWindowEvent(disp, ev->xmotion.window, MotionNotify, ev));
		winwid = winwidget_get_from_window(ev->xmotion.window);
		if (winwid) {
			if (opt.mode == MODE_NEXT) {
				if ((abs(winwid->click_offset_x - (ev->xmotion.x - winwid->im_x)) > FEH_JITTER_OFFSET)
						|| (abs(winwid->click_offset_y - (ev->xmotion.y - winwid->im_y)) > FEH_JITTER_OFFSET)
						|| (time(NULL) - winwid->click_start_time > FEH_JITTER_TIME)) {
					opt.mode = MODE_PAN;
					winwid->mode = MODE_PAN;
				}
				else
					return;
			}
			D(("Panning\n"));
			orig_x = winwid->im_x;
			orig_y = winwid->im_y;

			winwid->im_x = ev->xmotion.x - winwid->click_offset_x;
			winwid->im_y = ev->xmotion.y - winwid->click_offset_y;

			winwidget_sanitise_offsets(winwid);

			D(("im_x %d, im_w %d, off %d, mx %d, my %d\n", winwid->im_x,
				winwid->im_w, winwid->click_offset_x, ev->xmotion.x,
				ev->xmotion.y));

			/* XWarpPointer generates a MotionNotify event which we will
			 * parse. Since that event would undo the effect of the pointer
			 * warp, we need to change the click_offset to compensate this.
			 */
			if ((winwid->w - ev->xmotion.x <= 1) && (winwid->im_x < 0))
			{
				XWarpPointer(disp, None, winwid->win, 0, 0, 0, 0, 3,
					ev->xmotion.y);
				winwid->click_offset_x -= winwid->w - 4;
			}
			// TODO needlessly warps for certain zoom levels
			else if ((ev->xmotion.x <= 1) && (winwid->im_x >
					(winwid->w - winwid->im_w * winwid->zoom)))
			{
				XWarpPointer(disp, None, winwid->win, 0, 0, 0, 0,
					winwid->w - 4, ev->xmotion.y);
				winwid->click_offset_x += winwid->w - 4;
			}
			else if ((winwid->h - ev->xmotion.y <= 1)
					&& (winwid->im_y < 0))
			{
				XWarpPointer(disp, None, winwid->win, 0, 0, 0, 0,
					ev->xmotion.x, 3);
				winwid->click_offset_y -= winwid->h - 4;
			}
			// TODO needlessly warps for certain zoomlevels
			else if ((ev->xmotion.y <= 1) && (winwid->im_y >
					(winwid->h - winwid->im_h * winwid->zoom)))
			{
				XWarpPointer(disp, None, winwid->win, 0, 0, 0, 0,
					ev->xmotion.x, winwid->h - 4);
				winwid->click_offset_y += winwid->h - 4;
			}

			if ((winwid->im_x != orig_x)
					|| (winwid->im_y != orig_y))
				winwidget_render_image(winwid, 0, 1);
		}
	} else if (opt.mode == MODE_ROTATE) {
		while (XCheckTypedWindowEvent(disp, ev->xmotion.window, MotionNotify, ev));
		winwid = winwidget_get_from_window(ev->xmotion.window);
		if (winwid) {
			D(("Rotating\n"));
			if (!winwid->has_rotated) {
				Imlib_Image temp;

				temp = gib_imlib_create_rotated_image(winwid->im, 0.0);
				if (temp != NULL) {
					winwid->im_w = gib_imlib_image_get_width(temp);
					winwid->im_h = gib_imlib_image_get_height(temp);
					gib_imlib_free_image_and_decache(temp);
					if (!winwid->full_screen && !opt.geom_flags)
						winwidget_resize(winwid, winwid->im_w, winwid->im_h, 0);
					winwid->has_rotated = 1;
				}
			}
			winwid->im_angle = (ev->xmotion.x - winwid->w / 2) / ((double) winwid->w / 2) * 3.1415926535;
			D(("angle: %f\n", winwid->im_angle));
			winwidget_render_image(winwid, 0, 1);
		}
	} else if (opt.mode == MODE_BLUR) {
		while (XCheckTypedWindowEvent(disp, ev->xmotion.window, MotionNotify, ev));
		winwid = winwidget_get_from_window(ev->xmotion.window);
		if (winwid) {
			Imlib_Image temp, ptr;
			signed int blur_radius;

			D(("Blurring\n"));

			temp = gib_imlib_clone_image(winwid->im);
			if (temp != NULL) {
				blur_radius = (((double) ev->xmotion.x / winwid->w) * 20) - 10;
				D(("angle: %d\n", blur_radius));
				if (blur_radius > 0)
					gib_imlib_image_sharpen(temp, blur_radius);
				else
					gib_imlib_image_blur(temp, 0 - blur_radius);
				ptr = winwid->im;
				winwid->im = temp;
				winwidget_render_image(winwid, 0, 1);
				gib_imlib_free_image_and_decache(winwid->im);
				winwid->im = ptr;
			}
		}
	} else {
		while (XCheckTypedWindowEvent(disp, ev->xmotion.window, MotionNotify, ev));
		winwid = winwidget_get_from_window(ev->xmotion.window);
		if (winwid != NULL) {
			if (winwid->type == WIN_TYPE_THUMBNAIL) {
				feh_thumbnail *thumbnail;
				int x, y;

				x = (ev->xbutton.x - winwid->im_x) / winwid->zoom;
				y = (ev->xbutton.y - winwid->im_y) / winwid->zoom;
				thumbnail = feh_thumbnail_get_thumbnail_from_coords(x, y);
				feh_thumbnail_select(winwid, thumbnail);
			} else {
				feh_event_handle_generic(winwid, ev->xmotion.state | Mod3Mask, NoSymbol, 0);
			}
		}
	}
	return;
}

static void feh_event_handle_ClientMessage(XEvent * ev)
{
	winwidget winwid = NULL;

	if (ev->xclient.format == 32 && ev->xclient.data.l[0] == (signed) wmDeleteWindow) {
		winwid = winwidget_get_from_window(ev->xclient.window);
		if (winwid)
			winwidget_destroy(winwid);
	}

	return;
}
