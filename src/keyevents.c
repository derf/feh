/* keyevents.c

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
#include "thumbnail.h"
#include "filelist.h"
#include "winwidget.h"
#include "options.h"

fehkb keys;

static void feh_set_kb(fehkey *key, unsigned int s0, unsigned int y0, unsigned
		int s1, unsigned int y1, unsigned int s2, unsigned int y2) {
	key->keystates[0] = s0;
	key->keystates[1] = s1;
	key->keystates[2] = s2;
	key->keysyms[0] = y0;
	key->keysyms[1] = y1;
	key->keysyms[2] = y2;
}

static void feh_set_parse_kb_partial(fehkey *key, int index, char *ks) {
	char *cur = ks;
	int mod = 0;

	if (!*ks) {
		key->keysyms[index] = 0;
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
				weprintf("keys: invalid modifier %c in \"%s\"", cur[0], ks);
				break;
		}
		cur += 2;
	}

	key->keysyms[index] = XStringToKeysym(cur);
	if (isascii(key->keysyms[index]))
		mod &= ~ShiftMask;
	key->keystates[index] = mod;

	if (key->keysyms[index] == NoSymbol)
		weprintf("keys: Invalid keysym: %s", cur);
}

void init_keyevents(void) {
	char *home = NULL;
	char *confhome = NULL;
	char *confpath = NULL;
	char line[128];
	char action[32], k1[32], k2[32], k3[32];
	fehkey *cur_kb = NULL;
	FILE *conf = NULL;
	int read = 0;

	memset(&keys, 0, sizeof(keys));

	feh_set_kb(&keys.menu_close, 0, XK_Escape    , 0, 0            , 0, 0);
	feh_set_kb(&keys.menu_parent,0, XK_Left      , 0, 0            , 0, 0);
	feh_set_kb(&keys.menu_down , 0, XK_Down      , 0, 0            , 0, 0);
	feh_set_kb(&keys.menu_up   , 0, XK_Up        , 0, 0            , 0, 0);
	feh_set_kb(&keys.menu_child, 0, XK_Right     , 0, 0            , 0, 0);
	feh_set_kb(&keys.menu_select,0, XK_Return    , 0, XK_space     , 0, 0);
	feh_set_kb(&keys.scroll_left,0, XK_KP_Left   , 4, XK_Left      , 0, 0);
	feh_set_kb(&keys.scroll_right,0,XK_KP_Right  , 4, XK_Right     , 0, 0);
	feh_set_kb(&keys.scroll_down,0, XK_KP_Down   , 4, XK_Down      , 0, 0);
	feh_set_kb(&keys.scroll_up , 0, XK_KP_Up     , 4, XK_Up        , 0, 0);
	feh_set_kb(&keys.scroll_left_page , 8, XK_Left , 0, 0          , 0, 0);
	feh_set_kb(&keys.scroll_right_page, 8, XK_Right, 0, 0          , 0, 0);
	feh_set_kb(&keys.scroll_down_page , 8, XK_Down , 0, 0          , 0, 0);
	feh_set_kb(&keys.scroll_up_page   , 8, XK_Up   , 0, 0          , 0, 0);
	feh_set_kb(&keys.prev_img  , 0, XK_Left      , 0, XK_p         , 0, XK_BackSpace);
	feh_set_kb(&keys.next_img  , 0, XK_Right     , 0, XK_n         , 0, XK_space);
	feh_set_kb(&keys.jump_back , 0, XK_Page_Up   , 0, XK_KP_Page_Up, 0, 0);
	feh_set_kb(&keys.jump_fwd  , 0, XK_Page_Down , 0, XK_KP_Page_Down,0,0);
	feh_set_kb(&keys.prev_dir  , 0, XK_bracketleft, 0, 0           , 0, 0);
	feh_set_kb(&keys.next_dir  , 0, XK_bracketright, 0, 0          , 0, 0);
	feh_set_kb(&keys.jump_random,0, XK_z         , 0, 0            , 0, 0);
	feh_set_kb(&keys.quit      , 0, XK_Escape    , 0, XK_q         , 0, 0);
	feh_set_kb(&keys.close     , 0, XK_x         , 0, 0            , 0, 0);
	feh_set_kb(&keys.remove    , 0, XK_Delete    , 0, 0            , 0, 0);
	feh_set_kb(&keys.delete    , 4, XK_Delete    , 0, 0            , 0, 0);
	feh_set_kb(&keys.jump_first, 0, XK_Home      , 0, XK_KP_Home   , 0, 0);
	feh_set_kb(&keys.jump_last , 0, XK_End       , 0, XK_KP_End    , 0, 0);
	feh_set_kb(&keys.action_0  , 0, XK_Return    , 0, XK_0         , 0, XK_KP_0);
	feh_set_kb(&keys.action_1  , 0, XK_1         , 0, XK_KP_1      , 0, 0);
	feh_set_kb(&keys.action_2  , 0, XK_2         , 0, XK_KP_2      , 0, 0);
	feh_set_kb(&keys.action_3  , 0, XK_3         , 0, XK_KP_3      , 0, 0);
	feh_set_kb(&keys.action_4  , 0, XK_4         , 0, XK_KP_4      , 0, 0);
	feh_set_kb(&keys.action_5  , 0, XK_5         , 0, XK_KP_5      , 0, 0);
	feh_set_kb(&keys.action_6  , 0, XK_6         , 0, XK_KP_6      , 0, 0);
	feh_set_kb(&keys.action_7  , 0, XK_7         , 0, XK_KP_7      , 0, 0);
	feh_set_kb(&keys.action_8  , 0, XK_8         , 0, XK_KP_8      , 0, 0);
	feh_set_kb(&keys.action_9  , 0, XK_9         , 0, XK_KP_9      , 0, 0);
	feh_set_kb(&keys.zoom_in   , 0, XK_Up        , 0, XK_KP_Add    , 0, 0);
	feh_set_kb(&keys.zoom_out  , 0, XK_Down      , 0, XK_KP_Subtract,0, 0);
	feh_set_kb(&keys.zoom_default, 0, XK_KP_Multiply, 0, XK_asterisk,0, 0);
	feh_set_kb(&keys.zoom_fit  , 0, XK_KP_Divide , 0, XK_slash     , 0, 0);
	feh_set_kb(&keys.zoom_fill , 0, XK_exclam    , 0, 0            , 0, 0);
	feh_set_kb(&keys.size_to_image, 0, XK_w      , 0, 0            , 0, 0);
	feh_set_kb(&keys.render    , 0, XK_KP_Begin  , 0, XK_R         , 0, 0);
	feh_set_kb(&keys.toggle_actions, 0, XK_a, 0, 0, 0, 0);
	feh_set_kb(&keys.toggle_aliasing, 0, XK_A, 0, 0, 0, 0);
	feh_set_kb(&keys.toggle_filenames, 0, XK_d, 0, 0, 0, 0);
#ifdef HAVE_LIBEXIF
	feh_set_kb(&keys.toggle_exif, 0, XK_e, 0, 0, 0, 0);
#endif
	feh_set_kb(&keys.toggle_info, 0, XK_i, 0, 0, 0, 0);
	feh_set_kb(&keys.toggle_pointer, 0, XK_o, 0, 0, 0, 0);
	feh_set_kb(&keys.toggle_caption, 0, XK_c, 0, 0, 0, 0);
	feh_set_kb(&keys.toggle_pause, 0, XK_h, 0, 0, 0, 0);
	feh_set_kb(&keys.toggle_menu, 0, XK_m, 0, 0, 0, 0);
	feh_set_kb(&keys.toggle_fullscreen, 0, XK_v, 0, 0, 0, 0);
	feh_set_kb(&keys.reload_image, 0, XK_r, 0, 0, 0, 0);
	feh_set_kb(&keys.save_image, 0, XK_s, 0, 0, 0, 0);
	feh_set_kb(&keys.save_filelist, 0, XK_f, 0, 0, 0, 0);
	feh_set_kb(&keys.orient_1, 0, XK_greater, 0, 0, 0, 0);
	feh_set_kb(&keys.orient_3, 0, XK_less, 0, 0, 0, 0);
	feh_set_kb(&keys.flip, 0, XK_underscore, 0, 0, 0, 0);
	feh_set_kb(&keys.mirror, 0, XK_bar, 0, 0, 0, 0);
	feh_set_kb(&keys.reload_minus, 0, XK_minus, 0, 0, 0, 0);
	feh_set_kb(&keys.reload_plus, 0, XK_plus, 0, 0, 0, 0);
	feh_set_kb(&keys.toggle_keep_vp, 0, XK_k, 0, 0, 0, 0);

	home = getenv("HOME");
	confhome = getenv("XDG_CONFIG_HOME");

	if (confhome)
		confpath = estrjoin("/", confhome, "feh/keys", NULL);
	else if (home)
		confpath = estrjoin("/", home, ".config/feh/keys", NULL);
	else
		return;

	conf = fopen(confpath, "r");

	free(confpath);

	if (!conf && ((conf = fopen("/etc/feh/keys", "r")) == NULL))
		return;

	while (fgets(line, sizeof(line), conf)) {
		*action = '\0';
		*k1 = '\0';
		*k2 = '\0';
		*k3 = '\0';
		cur_kb = NULL;

		read = sscanf(line, "%31s %31s %31s %31s\n",
			(char *) &action, (char *) &k1, (char* ) &k2, (char *) &k3);

		if ((read == EOF) || (read == 0) || (line[0] == '#'))
			continue;

		cur_kb = feh_str_to_kb(action);

		if (cur_kb) {
			feh_set_parse_kb_partial(cur_kb, 0, k1);
			feh_set_parse_kb_partial(cur_kb, 1, k2);
			feh_set_parse_kb_partial(cur_kb, 2, k3);
		} else {
			weprintf("keys: Invalid action: %s", action);
		}
	}
	fclose(conf);
}

static short feh_is_kp(fehkey *key, unsigned int state, unsigned int sym, unsigned int button) {
	int i;

	if (sym != NoSymbol) {
		for (i = 0; i < 3; i++) {
			if (
					(key->keysyms[i] == sym) &&
					(key->keystates[i] == state))
				return 1;
			else if (key->keysyms[i] == 0)
				return 0;
		}
		return 0;
	}
	if ((key->state == state) && (key->button == button)) {
		return 1;
	}
	return 0;
}

void feh_event_invoke_action(winwidget winwid, unsigned char action)
{
	if (opt.actions[action]) {
		if (opt.slideshow) {
			feh_action_run(FEH_FILE(winwid->file->data), opt.actions[action], winwid);

			if (opt.hold_actions[action])
				feh_reload_image(winwid, 1, 1);
			else
				slideshow_change_image(winwid, SLIDE_NEXT, 1);

		} else if ((winwid->type == WIN_TYPE_SINGLE)
				|| (winwid->type == WIN_TYPE_THUMBNAIL_VIEWER)) {
			feh_action_run(FEH_FILE(winwid->file->data), opt.actions[action], winwid);

			if (opt.hold_actions[action])
				feh_reload_image(winwid, 1, 1);
			else
				winwidget_destroy(winwid);
		} else if (winwid->type == WIN_TYPE_THUMBNAIL) {
			feh_file *thumbfile;
			thumbfile = feh_thumbnail_get_selected_file();

			if (thumbfile) {
				feh_action_run(thumbfile, opt.actions[action], winwid);

				if (!opt.hold_actions[action])
					feh_thumbnail_mark_removed(thumbfile, 0);
			}
		}
	}
	return;
}

void feh_event_handle_keypress(XEvent * ev)
{
	int state;
	char kbuf[20];
	KeySym keysym;
	XKeyEvent *kev;
	winwidget winwid = NULL;
	feh_menu_item *selected_item;
	feh_menu *selected_menu;

	winwid = winwidget_get_from_window(ev->xkey.window);

	/* nuke dupe events, unless we're typing text */
	if (winwid && !winwid->caption_entry) {
		while (XCheckTypedWindowEvent(disp, ev->xkey.window, KeyPress, ev));
	}

	kev = (XKeyEvent *) ev;
	XLookupString(&ev->xkey, (char *) kbuf, sizeof(kbuf), &keysym, NULL);
	state = kev->state & (ControlMask | ShiftMask | Mod1Mask | Mod4Mask);

	if (isascii(keysym))
		state &= ~ShiftMask;

	/* menus are showing, so this is a menu control keypress */
	if (ev->xbutton.window == menu_cover) {
		selected_item = feh_menu_find_selected_r(menu_root, &selected_menu);
		if (feh_is_kp(&keys.menu_close, state, keysym, 0))
			feh_menu_hide(menu_root, True);
		else if (feh_is_kp(&keys.menu_parent, state, keysym, 0))
			feh_menu_select_parent(selected_menu);
		else if (feh_is_kp(&keys.menu_down, state, keysym, 0))
			feh_menu_select_next(selected_menu, selected_item);
		else if (feh_is_kp(&keys.menu_up, state, keysym, 0))
			feh_menu_select_prev(selected_menu, selected_item);
		else if (feh_is_kp(&keys.menu_child, state, keysym, 0))
			feh_menu_select_submenu(selected_menu);
		else if (feh_is_kp(&keys.menu_select, state, keysym, 0))
			feh_menu_item_activate(selected_menu, selected_item);
		return;
	}

	if (winwid == NULL)
		return;

	if (winwid->caption_entry) {
		switch (keysym) {
		case XK_Return:
			if (state & ControlMask) {
				/* insert actual newline */
				ESTRAPPEND(FEH_FILE(winwid->file->data)->caption, "\n");
				winwidget_render_image_cached(winwid);
			} else {
				/* finish caption entry, write to captions file */
				FILE *fp;
				char *caption_filename;
				caption_filename =
					build_caption_filename(FEH_FILE(winwid->file->data), 1);
				winwid->caption_entry = 0;
				winwidget_render_image_cached(winwid);
				XFreePixmap(disp, winwid->bg_pmap_cache);
				winwid->bg_pmap_cache = 0;
				fp = fopen(caption_filename, "w");
				if (!fp) {
					eprintf("couldn't write to captions file %s:", caption_filename);
					return;
				}
				fprintf(fp, "%s", FEH_FILE(winwid->file->data)->caption);
				free(caption_filename);
				fclose(fp);
			}
			break;
		case XK_Escape:
			/* cancel, revert caption */
			winwid->caption_entry = 0;
			free(FEH_FILE(winwid->file->data)->caption);
			FEH_FILE(winwid->file->data)->caption = NULL;
			winwidget_render_image_cached(winwid);
			XFreePixmap(disp, winwid->bg_pmap_cache);
			winwid->bg_pmap_cache = 0;
			break;
		case XK_BackSpace:
			/* backspace */
			ESTRTRUNC(FEH_FILE(winwid->file->data)->caption, 1);
			winwidget_render_image_cached(winwid);
			break;
		default:
			if (isascii(keysym)) {
				/* append to caption */
				ESTRAPPEND_CHAR(FEH_FILE(winwid->file->data)->caption, keysym);
				winwidget_render_image_cached(winwid);
			}
			break;
		}
		return;
	}
	feh_event_handle_generic(winwid, state, keysym, 0);
}

fehkey *feh_str_to_kb(char *action)
{
	if (!strcmp(action, "menu_close"))
		return &keys.menu_close;
	else if (!strcmp(action, "menu_parent"))
		return &keys.menu_parent;
	else if (!strcmp(action, "menu_down"))
		return &keys.menu_down;
	else if (!strcmp(action, "menu_up"))
		return &keys.menu_up;
	else if (!strcmp(action, "menu_child"))
		return &keys.menu_child;
	else if (!strcmp(action, "menu_select"))
		return &keys.menu_select;
	else if (!strcmp(action, "scroll_right"))
		return &keys.scroll_right;
	else if (!strcmp(action, "scroll_left"))
		return &keys.scroll_left;
	else if (!strcmp(action, "scroll_up"))
		return &keys.scroll_up;
	else if (!strcmp(action, "scroll_down"))
		return &keys.scroll_down;
	else if (!strcmp(action, "scroll_right_page"))
		return &keys.scroll_right_page;
	else if (!strcmp(action, "scroll_left_page"))
		return &keys.scroll_left_page;
	else if (!strcmp(action, "scroll_up_page"))
		return &keys.scroll_up_page;
	else if (!strcmp(action, "scroll_down_page"))
		return &keys.scroll_down_page;
	else if (!strcmp(action, "prev_img"))
		return &keys.prev_img;
	else if (!strcmp(action, "next_img"))
		return &keys.next_img;
	else if (!strcmp(action, "jump_back"))
		return &keys.jump_back;
	else if (!strcmp(action, "jump_fwd"))
		return &keys.jump_fwd;
	else if (!strcmp(action, "prev_dir"))
		return &keys.prev_dir;
	else if (!strcmp(action, "next_dir"))
		return &keys.next_dir;
	else if (!strcmp(action, "jump_random"))
		return &keys.jump_random;
	else if (!strcmp(action, "quit"))
		return &keys.quit;
	else if (!strcmp(action, "close"))
		return &keys.close;
	else if (!strcmp(action, "remove"))
		return &keys.remove;
	else if (!strcmp(action, "delete"))
		return &keys.delete;
	else if (!strcmp(action, "jump_first"))
		return &keys.jump_first;
	else if (!strcmp(action, "jump_last"))
		return &keys.jump_last;
	else if (!strcmp(action, "action_0"))
		return &keys.action_0;
	else if (!strcmp(action, "action_1"))
		return &keys.action_1;
	else if (!strcmp(action, "action_2"))
		return &keys.action_2;
	else if (!strcmp(action, "action_3"))
		return &keys.action_3;
	else if (!strcmp(action, "action_4"))
		return &keys.action_4;
	else if (!strcmp(action, "action_5"))
		return &keys.action_5;
	else if (!strcmp(action, "action_6"))
		return &keys.action_6;
	else if (!strcmp(action, "action_7"))
		return &keys.action_7;
	else if (!strcmp(action, "action_8"))
		return &keys.action_8;
	else if (!strcmp(action, "action_9"))
		return &keys.action_9;
	else if (!strcmp(action, "zoom_in"))
		return &keys.zoom_in;
	else if (!strcmp(action, "zoom_out"))
		return &keys.zoom_out;
	else if (!strcmp(action, "zoom_default"))
		return &keys.zoom_default;
	else if (!strcmp(action, "zoom_fit"))
		return &keys.zoom_fit;
	else if (!strcmp(action, "zoom_fill"))
		return &keys.zoom_fill;
	else if (!strcmp(action, "size_to_image"))
		return &keys.size_to_image;
	else if (!strcmp(action, "render"))
		return &keys.render;
	else if (!strcmp(action, "toggle_actions"))
		return &keys.toggle_actions;
	else if (!strcmp(action, "toggle_aliasing"))
		return &keys.toggle_aliasing;
	else if (!strcmp(action, "toggle_filenames"))
		return &keys.toggle_filenames;
#ifdef HAVE_LIBEXIF
	else if (!strcmp(action, "toggle_exif"))
		return &keys.toggle_exif;
#endif
	else if (!strcmp(action, "toggle_info"))
		return &keys.toggle_info;
	else if (!strcmp(action, "toggle_pointer"))
		return &keys.toggle_pointer;
	else if (!strcmp(action, "toggle_caption"))
		return &keys.toggle_caption;
	else if (!strcmp(action, "toggle_pause"))
		return &keys.toggle_pause;
	else if (!strcmp(action, "toggle_menu"))
		return &keys.toggle_menu;
	else if (!strcmp(action, "toggle_fullscreen"))
		return &keys.toggle_fullscreen;
	else if (!strcmp(action, "reload_image"))
		return &keys.reload_image;
	else if (!strcmp(action, "save_image"))
		return &keys.save_image;
	else if (!strcmp(action, "save_filelist"))
		return &keys.save_filelist;
	else if (!strcmp(action, "orient_1"))
		return &keys.orient_1;
	else if (!strcmp(action, "orient_3"))
		return &keys.orient_3;
	else if (!strcmp(action, "flip"))
		return &keys.flip;
	else if (!strcmp(action, "mirror"))
		return &keys.mirror;
	else if (!strcmp(action, "reload_minus"))
		return &keys.reload_minus;
	else if (!strcmp(action, "reload_plus"))
		return &keys.reload_plus;
	else if (!strcmp(action, "toggle_keep_vp"))
		return &keys.toggle_keep_vp;

	return NULL;
}

void feh_event_handle_generic(winwidget winwid, unsigned int state, KeySym keysym, unsigned int button) {
	int curr_screen = 0;

	if (feh_is_kp(&keys.next_img, state, keysym, button)) {
		if (opt.slideshow)
			slideshow_change_image(winwid, SLIDE_NEXT, 1);
		else if (winwid->type == WIN_TYPE_THUMBNAIL)
			feh_thumbnail_select_next(winwid, 1);
	}
	else if (feh_is_kp(&keys.prev_img, state, keysym, button)) {
		if (opt.slideshow)
			slideshow_change_image(winwid, SLIDE_PREV, 1);
		else if (winwid->type == WIN_TYPE_THUMBNAIL)
			feh_thumbnail_select_prev(winwid, 1);
	}
	else if (feh_is_kp(&keys.scroll_right, state, keysym, button)) {
		winwid->im_x -= opt.scroll_step;;
		winwidget_sanitise_offsets(winwid);
		winwidget_render_image(winwid, 0, 1);
	}
	else if (feh_is_kp(&keys.scroll_left, state, keysym, button)) {
		winwid->im_x += opt.scroll_step;
		winwidget_sanitise_offsets(winwid);
		winwidget_render_image(winwid, 0, 1);
	}
	else if (feh_is_kp(&keys.scroll_down, state, keysym, button)) {
		winwid->im_y -= opt.scroll_step;
		winwidget_sanitise_offsets(winwid);
		winwidget_render_image(winwid, 0, 1);
	}
	else if (feh_is_kp(&keys.scroll_up, state, keysym, button)) {
		winwid->im_y += opt.scroll_step;
		winwidget_sanitise_offsets(winwid);
		winwidget_render_image(winwid, 0, 1);
	}
	else if (feh_is_kp(&keys.scroll_right_page, state, keysym, button)) {
		winwid->im_x -= winwid->w;
		winwidget_sanitise_offsets(winwid);
		winwidget_render_image(winwid, 0, 0);
	}
	else if (feh_is_kp(&keys.scroll_left_page, state, keysym, button)) {
		winwid->im_x += winwid->w;
		winwidget_sanitise_offsets(winwid);
		winwidget_render_image(winwid, 0, 0);
	}
	else if (feh_is_kp(&keys.scroll_down_page, state, keysym, button)) {
		winwid->im_y -= winwid->h;
		winwidget_sanitise_offsets(winwid);
		winwidget_render_image(winwid, 0, 0);
	}
	else if (feh_is_kp(&keys.scroll_up_page, state, keysym, button)) {
		winwid->im_y += winwid->h;
		winwidget_sanitise_offsets(winwid);
		winwidget_render_image(winwid, 0, 0);
	}
	else if (feh_is_kp(&keys.jump_back, state, keysym, button)) {
		if (opt.slideshow)
			slideshow_change_image(winwid, SLIDE_JUMP_BACK, 1);
		else if (winwid->type == WIN_TYPE_THUMBNAIL)
			feh_thumbnail_select_prev(winwid, 10);
	}
	else if (feh_is_kp(&keys.jump_fwd, state, keysym, button)) {
		if (opt.slideshow)
			slideshow_change_image(winwid, SLIDE_JUMP_FWD, 1);
		else if (winwid->type == WIN_TYPE_THUMBNAIL)
			feh_thumbnail_select_next(winwid, 10);
	}
	else if (feh_is_kp(&keys.next_dir, state, keysym, button)) {
		if (opt.slideshow)
			slideshow_change_image(winwid, SLIDE_JUMP_NEXT_DIR, 1);
	}
	else if (feh_is_kp(&keys.prev_dir, state, keysym, button)) {
		if (opt.slideshow)
			slideshow_change_image(winwid, SLIDE_JUMP_PREV_DIR, 1);
	}
	else if (feh_is_kp(&keys.quit, state, keysym, button)) {
		winwidget_destroy_all();
	}
	else if (feh_is_kp(&keys.delete, state, keysym, button)) {
		if (winwid->type == WIN_TYPE_THUMBNAIL_VIEWER)
			feh_thumbnail_mark_removed(FEH_FILE(winwid->file->data), 1);
		feh_filelist_image_remove(winwid, 1);
	}
	else if (feh_is_kp(&keys.remove, state, keysym, button)) {
		if (winwid->type == WIN_TYPE_THUMBNAIL_VIEWER)
			feh_thumbnail_mark_removed(FEH_FILE(winwid->file->data), 0);
		feh_filelist_image_remove(winwid, 0);
	}
	else if (feh_is_kp(&keys.jump_first, state, keysym, button)) {
		if (opt.slideshow)
			slideshow_change_image(winwid, SLIDE_FIRST, 1);
	}
	else if (feh_is_kp(&keys.jump_last, state, keysym, button)) {
		if (opt.slideshow)
			slideshow_change_image(winwid, SLIDE_LAST, 1);
	}
	else if (feh_is_kp(&keys.action_0, state, keysym, button)) {
		feh_event_invoke_action(winwid, 0);
	}
	else if (feh_is_kp(&keys.action_1, state, keysym, button)) {
		feh_event_invoke_action(winwid, 1);
	}
	else if (feh_is_kp(&keys.action_2, state, keysym, button)) {
		feh_event_invoke_action(winwid, 2);
	}
	else if (feh_is_kp(&keys.action_3, state, keysym, button)) {
		feh_event_invoke_action(winwid, 3);
	}
	else if (feh_is_kp(&keys.action_4, state, keysym, button)) {
		feh_event_invoke_action(winwid, 4);
	}
	else if (feh_is_kp(&keys.action_5, state, keysym, button)) {
		feh_event_invoke_action(winwid, 5);
	}
	else if (feh_is_kp(&keys.action_6, state, keysym, button)) {
		feh_event_invoke_action(winwid, 6);
	}
	else if (feh_is_kp(&keys.action_7, state, keysym, button)) {
		feh_event_invoke_action(winwid, 7);
	}
	else if (feh_is_kp(&keys.action_8, state, keysym, button)) {
		feh_event_invoke_action(winwid, 8);
	}
	else if (feh_is_kp(&keys.action_9, state, keysym, button)) {
		feh_event_invoke_action(winwid, 9);
	}
	else if (feh_is_kp(&keys.zoom_in, state, keysym, button)) {
		winwid->old_zoom = winwid->zoom;
		winwid->zoom = winwid->zoom * 1.25;

		if (winwid->zoom > ZOOM_MAX)
			winwid->zoom = ZOOM_MAX;

		winwid->im_x = (winwid->w / 2) - (((winwid->w / 2) - winwid->im_x) /
			winwid->old_zoom * winwid->zoom);
		winwid->im_y = (winwid->h / 2) - (((winwid->h / 2) - winwid->im_y) /
			winwid->old_zoom * winwid->zoom);
		winwidget_sanitise_offsets(winwid);
		winwidget_render_image(winwid, 0, 0);
	}
	else if (feh_is_kp(&keys.zoom_out, state, keysym, button)) {
		winwid->old_zoom = winwid->zoom;
		winwid->zoom = winwid->zoom * 0.80;

		if (winwid->zoom < ZOOM_MIN)
			winwid->zoom = ZOOM_MIN;

		winwid->im_x = (winwid->w / 2) - (((winwid->w / 2) - winwid->im_x) /
			winwid->old_zoom * winwid->zoom);
		winwid->im_y = (winwid->h / 2) - (((winwid->h / 2) - winwid->im_y) /
			winwid->old_zoom * winwid->zoom);
		winwidget_sanitise_offsets(winwid);
		winwidget_render_image(winwid, 0, 0);
	}
	else if (feh_is_kp(&keys.zoom_default, state, keysym, button)) {
		winwid->zoom = 1.0;
		winwidget_center_image(winwid);
		winwidget_render_image(winwid, 0, 0);
	}
	else if (feh_is_kp(&keys.zoom_fit, state, keysym, button)) {
		feh_calc_needed_zoom(&winwid->zoom, winwid->im_w, winwid->im_h, winwid->w, winwid->h);
		winwidget_center_image(winwid);
		winwidget_render_image(winwid, 0, 0);
	}
	else if (feh_is_kp(&keys.zoom_fill, state, keysym, button)) {
		int save_zoom = opt.zoom_mode;
		opt.zoom_mode = ZOOM_MODE_FILL;
		feh_calc_needed_zoom(&winwid->zoom, winwid->im_w, winwid->im_h, winwid->w, winwid->h);
		winwidget_center_image(winwid);
		winwidget_render_image(winwid, 0, 0);
		opt.zoom_mode = save_zoom;
	}
	else if (feh_is_kp(&keys.render, state, keysym, button)) {
		if (winwid->type == WIN_TYPE_THUMBNAIL)
			feh_thumbnail_show_selected();
		else
			winwidget_render_image(winwid, 0, 0);
	}
	else if (feh_is_kp(&keys.toggle_actions, state, keysym, button)) {
		opt.draw_actions = !opt.draw_actions;
		winwidget_rerender_all(0);
	}
	else if (feh_is_kp(&keys.toggle_aliasing, state, keysym, button)) {
		opt.force_aliasing = !opt.force_aliasing;
		winwid->force_aliasing = !winwid->force_aliasing;
		winwidget_render_image(winwid, 0, 0);
	}
	else if (feh_is_kp(&keys.toggle_filenames, state, keysym, button)) {
		opt.draw_filename = !opt.draw_filename;
		winwidget_rerender_all(0);
	}
#ifdef HAVE_LIBEXIF
	else if (feh_is_kp(&keys.toggle_exif, state, keysym, button)) {
		opt.draw_exif = !opt.draw_exif;
		winwidget_rerender_all(0);
	}
#endif
	else if (feh_is_kp(&keys.toggle_info, state, keysym, button)) {
		opt.draw_info = !opt.draw_info;
		winwidget_rerender_all(0);
	}
	else if (feh_is_kp(&keys.toggle_pointer, state, keysym, button)) {
		winwidget_set_pointer(winwid, opt.hide_pointer);
		opt.hide_pointer = !opt.hide_pointer;
	}
	else if (feh_is_kp(&keys.jump_random, state, keysym, button)) {
		if (winwid->type == WIN_TYPE_THUMBNAIL)
			feh_thumbnail_select_next(winwid, rand() % (filelist_len - 1));
		else
			slideshow_change_image(winwid, SLIDE_RAND, 1);
	}
	else if (feh_is_kp(&keys.toggle_caption, state, keysym, button)) {
		if (opt.caption_path) {
			/*
			 * editing captions in slideshow mode does not make any sense
			 * at all; this is just in case someone accidentally does it...
			 */
			if (opt.slideshow_delay)
				opt.paused = 1;
			winwid->caption_entry = 1;
		}
		winwidget_render_image(winwid, 0, 0);
	}
	else if (feh_is_kp(&keys.reload_image, state, keysym, button)) {
		feh_reload_image(winwid, 0, 0);
	}
	else if (feh_is_kp(&keys.toggle_pause, state, keysym, button)) {
		slideshow_pause_toggle(winwid);
	}
	else if (feh_is_kp(&keys.save_image, state, keysym, button)) {
		slideshow_save_image(winwid);
	}
	else if (feh_is_kp(&keys.save_filelist, state, keysym, button)) {
		if ((winwid->type == WIN_TYPE_THUMBNAIL)
				|| (winwid->type == WIN_TYPE_THUMBNAIL_VIEWER))
			weprintf("Filelist saving is not supported in thumbnail mode");
		else
			feh_save_filelist();
	}
	else if (feh_is_kp(&keys.size_to_image, state, keysym, button)) {
		winwidget_size_to_image(winwid);
	}
	else if (feh_is_kp(&keys.toggle_menu, state, keysym, button)) {
		winwidget_show_menu(winwid);
	}
	else if (feh_is_kp(&keys.close, state, keysym, button)) {
		winwidget_destroy(winwid);
	}
	else if (feh_is_kp(&keys.orient_1, state, keysym, button)) {
		feh_edit_inplace(winwid, 1);
	}
	else if (feh_is_kp(&keys.orient_3, state, keysym, button)) {
		feh_edit_inplace(winwid, 3);
	}
	else if (feh_is_kp(&keys.flip, state, keysym, button)) {
		feh_edit_inplace(winwid, INPLACE_EDIT_FLIP);
	}
	else if (feh_is_kp(&keys.mirror, state, keysym, button)) {
		feh_edit_inplace(winwid, INPLACE_EDIT_MIRROR);
	}
	else if (feh_is_kp(&keys.toggle_fullscreen, state, keysym, button)) {
#ifdef HAVE_LIBXINERAMA
		if (opt.xinerama && xinerama_screens) {
			int i, rect[4];

			winwidget_get_geometry(winwid, rect);
			for (i = 0; i < num_xinerama_screens; i++) {
				xinerama_screen = 0;
				if (XY_IN_RECT(rect[0], rect[1],
							xinerama_screens[i].x_org,
							xinerama_screens[i].y_org,
							xinerama_screens[i].width,
							xinerama_screens[i].height)) {
					curr_screen = xinerama_screen = i;
					break;
				}
			}
			if (opt.xinerama_index >= 0)
				curr_screen = xinerama_screen = opt.xinerama_index;
		}
#endif				/* HAVE_LIBXINERAMA */
		winwid->full_screen = !winwid->full_screen;
		winwidget_destroy_xwin(winwid);
		winwidget_create_window(winwid, winwid->im_w, winwid->im_h);
		winwidget_render_image(winwid, 1, 0);
		winwidget_show(winwid);
#ifdef HAVE_LIBXINERAMA
		/* if we have xinerama and we're using it, then full screen the window
		 * on the head that the window was active on */
		if (winwid->full_screen == TRUE && opt.xinerama && xinerama_screens) {
			xinerama_screen = curr_screen;
			winwidget_move(winwid,
					xinerama_screens[curr_screen].x_org, xinerama_screens[curr_screen].y_org);
		}
#endif				/* HAVE_LIBXINERAMA */
	}
	else if (feh_is_kp(&keys.reload_plus, state, keysym, button)){
		if (opt.reload < SLIDESHOW_RELOAD_MAX)
			opt.reload++;
		else if (opt.verbose)
			weprintf("Cannot set RELOAD higher than %f seconds.", opt.reload);
	}
	else if (feh_is_kp(&keys.reload_minus, state, keysym, button)) {
		if (opt.reload > 1)
			opt.reload--;
		else if (opt.verbose)
			weprintf("Cannot set RELOAD lower than 1 second.");
	}
	else if (feh_is_kp(&keys.toggle_keep_vp, state, keysym, button)) {
		opt.keep_zoom_vp = !opt.keep_zoom_vp;
	}
	return;
}
