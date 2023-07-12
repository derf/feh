/* keyevents.c

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
#include "filelist.h"
#include "winwidget.h"
#include "options.h"
#include <termios.h>

struct __fehkey keys[EVENT_LIST_END];
struct termios old_term_settings;
unsigned char control_via_stdin = 0;

void setup_stdin(void) {
	struct termios ctrl;

	control_via_stdin = 1;

	if (tcgetattr(STDIN_FILENO, &old_term_settings) == -1)
		eprintf("tcgetattr failed");
	if (tcgetattr(STDIN_FILENO, &ctrl) == -1)
		eprintf("tcgetattr failed");

	ctrl.c_iflag &= ~(PARMRK | ISTRIP
			| INLCR | IGNCR | IXON);
	ctrl.c_lflag &= ~(ECHO | ICANON | IEXTEN);
	ctrl.c_cflag &= ~(CSIZE | PARENB);
	ctrl.c_cflag |= CS8;

	if (tcsetattr(STDIN_FILENO, TCSANOW, &ctrl) == -1)
		eprintf("tcsetattr failed");
}

void restore_stdin(void) {
	if (tcsetattr(STDIN_FILENO, TCSANOW, &old_term_settings) == -1)
		eprintf("tcsetattr failed");
}

static void feh_set_kb(char *name, unsigned int s0, unsigned int y0,
		unsigned int s1, unsigned int y1, unsigned int s2, unsigned int y2) {
	static int key_index = 0;
	fehkey *key = &keys[key_index];
	key->keystates[0] = s0;
	key->keystates[1] = s1;
	key->keystates[2] = s2;
	key->keysyms[0] = y0;
	key->keysyms[1] = y1;
	key->keysyms[2] = y2;
	key->state = 0;
	key->button = 0;
	key->name = name;
	key_index++;
}

static inline int ignore_space(int keysym) {
	/*
	 * Passing values which do not fit inside a signed 8bit char to isprint,
	 * isspace and the likes is undefined behaviour... which glibc (for some
	 * values) implements as a segmentation fault. So let's not do that.
	 */
	return ((keysym <= 127) && (keysym >= -128) && isprint(keysym) && !isspace(keysym));
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
	if (ignore_space(key->keysyms[index]))
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

	/*
	 * The feh_set_kb statements must have the same order as the key_action
	 * enum.
	 */

	feh_set_kb("menu_close" , 0, XK_Escape    , 0, 0            , 0, 0);
	feh_set_kb("menu_parent", 0, XK_Left      , 0, 0            , 0, 0);
	feh_set_kb("menu_down", 0, XK_Down      , 0, 0            , 0, 0);
	feh_set_kb("menu_up", 0, XK_Up        , 0, 0            , 0, 0);
	feh_set_kb("menu_child", 0, XK_Right     , 0, 0            , 0, 0);
	feh_set_kb("menu_select", 0, XK_Return    , 0, XK_space     , 0, 0);
	feh_set_kb("scroll_left",0, XK_KP_Left   , 4, XK_Left      , 0, 0);
	feh_set_kb("scroll_right", 0,XK_KP_Right  , 4, XK_Right     , 0, 0);
	feh_set_kb("scroll_down",0, XK_KP_Down   , 4, XK_Down      , 0, 0);
	feh_set_kb("scroll_up",  0, XK_KP_Up     , 4, XK_Up        , 0, 0);
	feh_set_kb("scroll_left_page" , 8, XK_Left , 0, 0          , 0, 0);
	feh_set_kb("scroll_right_page", 8, XK_Right, 0, 0          , 0, 0);
	feh_set_kb("scroll_down_page" , 8, XK_Down , 0, 0          , 0, 0);
	feh_set_kb("scroll_up_page" , 8, XK_Up   , 0, 0          , 0, 0);
	feh_set_kb("prev_img"  , 0, XK_Left      , 0, XK_p         , 0, XK_BackSpace);
	feh_set_kb("next_img"  , 0, XK_Right     , 0, XK_n         , 0, XK_space);
	feh_set_kb("jump_back" , 0, XK_Page_Up   , 0, XK_KP_Page_Up, 0, 0);
	feh_set_kb("jump_fwd"  , 0, XK_Page_Down , 0, XK_KP_Page_Down,0,0);
	feh_set_kb("prev_dir"  , 0, XK_bracketleft, 0, 0           , 0, 0);
	feh_set_kb("next_dir"  , 0, XK_bracketright, 0, 0          , 0, 0);
	feh_set_kb("jump_random" ,0, XK_z         , 0, 0            , 0, 0);
	feh_set_kb("quit"      , 0, XK_Escape    , 0, XK_q         , 0, 0);
	feh_set_kb("close"     , 0, XK_x         , 0, 0            , 0, 0);
	feh_set_kb("remove"    , 0, XK_Delete    , 0, 0            , 0, 0);
	feh_set_kb("delete"    , 4, XK_Delete    , 0, 0            , 0, 0);
	feh_set_kb("jump_first" , 0, XK_Home      , 0, XK_KP_Home   , 0, 0);
	feh_set_kb("jump_last" , 0, XK_End       , 0, XK_KP_End    , 0, 0);
	feh_set_kb("action_0"  , 0, XK_Return    , 0, XK_0         , 0, XK_KP_0);
	feh_set_kb("action_1"  , 0, XK_1         , 0, XK_KP_1      , 0, 0);
	feh_set_kb("action_2"  , 0, XK_2         , 0, XK_KP_2      , 0, 0);
	feh_set_kb("action_3"  , 0, XK_3         , 0, XK_KP_3      , 0, 0);
	feh_set_kb("action_4"  , 0, XK_4         , 0, XK_KP_4      , 0, 0);
	feh_set_kb("action_5"  , 0, XK_5         , 0, XK_KP_5      , 0, 0);
	feh_set_kb("action_6"  , 0, XK_6         , 0, XK_KP_6      , 0, 0);
	feh_set_kb("action_7"  , 0, XK_7         , 0, XK_KP_7      , 0, 0);
	feh_set_kb("action_8"  , 0, XK_8         , 0, XK_KP_8      , 0, 0);
	feh_set_kb("action_9"  , 0, XK_9         , 0, XK_KP_9      , 0, 0);
	feh_set_kb("zoom_in"   , 0, XK_Up        , 0, XK_KP_Add    , 0, 0);
	feh_set_kb("zoom_out"  , 0, XK_Down      , 0, XK_KP_Subtract,0, 0);
	feh_set_kb("zoom_default" , 0, XK_KP_Multiply, 0, XK_asterisk,0, 0);
	feh_set_kb("zoom_fit"  , 0, XK_KP_Divide , 0, XK_slash     , 0, 0);
	feh_set_kb("zoom_fill" , 0, XK_exclam    , 0, 0            , 0, 0);
	feh_set_kb("size_to_image" , 0, XK_w      , 0, 0            , 0, 0);
	feh_set_kb("render"    , 0, XK_KP_Begin  , 0, XK_R         , 0, 0);
	feh_set_kb("toggle_actions" , 0, XK_a, 0, 0, 0, 0);
	feh_set_kb("toggle_aliasing" , 0, XK_A, 0, 0, 0, 0);
	feh_set_kb("toggle_auto_zoom" , 0, XK_Z, 0, 0, 0, 0);
#ifdef HAVE_LIBEXIF
	feh_set_kb("toggle_exif" , 0, XK_e, 0, 0, 0, 0);
#endif
	feh_set_kb("toggle_filenames" , 0, XK_d, 0, 0, 0, 0);
	feh_set_kb("toggle_info" , 0, XK_i, 0, 0, 0, 0);
	feh_set_kb("toggle_pointer" , 0, XK_o, 0, 0, 0, 0);
	feh_set_kb("toggle_caption" , 0, XK_c, 0, 0, 0, 0);
	feh_set_kb("toggle_pause" , 0, XK_h, 0, 0, 0, 0);
	feh_set_kb("toggle_menu" , 0, XK_m, 0, 0, 0, 0);
	feh_set_kb("toggle_fullscreen" , 0, XK_f, 0, 0, 0, 0);
	feh_set_kb("reload_image" , 0, XK_r, 0, 0, 0, 0);
	feh_set_kb("save_image" , 0, XK_s, 0, 0, 0, 0);
	feh_set_kb("save_filelist" , 0, XK_L, 0, 0, 0, 0);
	feh_set_kb("orient_1" , 0, XK_greater, 0, 0, 0, 0);
	feh_set_kb("orient_3" , 0, XK_less, 0, 0, 0, 0);
	feh_set_kb("flip" , 0, XK_underscore, 0, 0, 0, 0);
	feh_set_kb("mirror" , 0, XK_bar, 0, 0, 0, 0);
	feh_set_kb("reload_minus" , 0, XK_minus, 0, 0, 0, 0);
	feh_set_kb("reload_plus" , 0, XK_plus, 0, 0, 0, 0);
	feh_set_kb("toggle_keep_vp" , 0, XK_k, 0, 0, 0, 0);
	feh_set_kb("toggle_fixed_geometry" , 0, XK_g, 0, 0, 0, 0);
	feh_set_kb("pan" , 0, 0, 0, 0, 0, 0);
	feh_set_kb("zoom" , 0, 0, 0, 0, 0, 0);
	feh_set_kb("blur" , 0, 0, 0, 0, 0, 0);
	feh_set_kb("rotate" , 0, 0, 0, 0, 0, 0);

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

static short feh_is_kp(unsigned int key_index, unsigned int state,
		unsigned int sym, unsigned int button) {
	int i;

	if (sym != NoSymbol) {
		for (i = 0; i < 3; i++) {
			if (
					(keys[key_index].keysyms[i] == sym) &&
					(keys[key_index].keystates[i] == state))
				return 1;
			else if (keys[key_index].keysyms[i] == 0)
				return 0;
		}
		return 0;
	}
	if ((keys[key_index].state == state)
			&& (keys[key_index].button == button)) {
		return 1;
	}
	return 0;
}

void feh_event_invoke_action(winwidget winwid, unsigned char action)
{
	struct stat st;
	if (opt.actions[action]) {
		if (opt.slideshow) {
			feh_action_run(FEH_FILE(winwid->file->data), opt.actions[action], winwid);

			if (opt.hold_actions[action])
				feh_reload_image(winwid, 1, 1);
			else if (stat(FEH_FILE(winwid->file->data)->filename, &st) == -1)
				feh_filelist_image_remove(winwid, 0);
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

void feh_event_handle_stdin(void)
{
	char stdin_buf[2];
	static char is_esc = 0;
	KeySym keysym = NoSymbol;
	if (read(STDIN_FILENO, &stdin_buf, 1) <= 0) {
		control_via_stdin = 0;
		if (isatty(STDIN_FILENO) && getpgrp() == (tcgetpgrp(STDIN_FILENO))) {
			weprintf("reading a command from stdin failed - disabling control via stdin");
			restore_stdin();
		}
		return;
	}
	stdin_buf[1] = '\0';

	// escape?
	if (stdin_buf[0] == 0x1b) {
		is_esc = 1;
		return;
	}
	if ((is_esc == 1) && (stdin_buf[0] == '[')) {
		is_esc = 2;
		return;
	}

	if (stdin_buf[0] == ' ')
		keysym = XK_space;
	else if (stdin_buf[0] == '\n')
		keysym = XK_Return;
	else if ((stdin_buf[0] == '\b') || (stdin_buf[0] == 127))
		keysym = XK_BackSpace;
	else if (is_esc == 2) {
		if (stdin_buf[0] == 'A')
			keysym = XK_Up;
		else if (stdin_buf[0] == 'B')
			keysym = XK_Down;
		else if (stdin_buf[0] == 'C')
			keysym = XK_Right;
		else if (stdin_buf[0] == 'D')
			keysym = XK_Left;
		is_esc = 0;
	}
	else
		keysym = XStringToKeysym(stdin_buf);

	if (window_num && keysym)
		feh_event_handle_generic(windows[0], is_esc * Mod1Mask, keysym, 0);

	is_esc = 0;
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

	if (ignore_space(keysym))
		state &= ~ShiftMask;

	/* menus are showing, so this is a menu control keypress */
	if (ev->xbutton.window == menu_cover) {
		selected_item = feh_menu_find_selected_r(menu_root, &selected_menu);
		if (feh_is_kp(EVENT_menu_close, state, keysym, 0))
			feh_menu_hide(menu_root, True);
		else if (feh_is_kp(EVENT_menu_parent, state, keysym, 0))
			feh_menu_select_parent(selected_menu);
		else if (feh_is_kp(EVENT_menu_down, state, keysym, 0))
			feh_menu_select_next(selected_menu, selected_item);
		else if (feh_is_kp(EVENT_menu_up, state, keysym, 0))
			feh_menu_select_prev(selected_menu, selected_item);
		else if (feh_is_kp(EVENT_menu_child, state, keysym, 0))
			feh_menu_select_submenu(selected_menu);
		else if (feh_is_kp(EVENT_menu_select, state, keysym, 0))
			feh_menu_item_activate(selected_menu, selected_item);
		return;
	}

	if (winwid == NULL)
		return;

	feh_event_handle_generic(winwid, state, keysym, 0);
}

fehkey *feh_str_to_kb(char *action)
{
	for (unsigned int i = 0; i < EVENT_LIST_END; i++) {
		if (!strcmp(action, keys[i].name)) {
			return &keys[i];
		}
	}
	return NULL;
}

void feh_event_handle_generic(winwidget winwid, unsigned int state, KeySym keysym, unsigned int button) {
	int curr_screen = 0;

	if (winwid->caption_entry && (keysym != NoSymbol)) {
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

	if (feh_is_kp(EVENT_next_img, state, keysym, button)) {
		if (opt.slideshow)
			slideshow_change_image(winwid, SLIDE_NEXT, 1);
		else if (winwid->type == WIN_TYPE_THUMBNAIL)
			feh_thumbnail_select_next(winwid, 1);
	}
	else if (feh_is_kp(EVENT_prev_img, state, keysym, button)) {
		if (opt.slideshow)
			slideshow_change_image(winwid, SLIDE_PREV, 1);
		else if (winwid->type == WIN_TYPE_THUMBNAIL)
			feh_thumbnail_select_prev(winwid, 1);
	}
	else if (feh_is_kp(EVENT_scroll_right, state, keysym, button)) {
		winwid->im_x -= opt.scroll_step;;
		winwidget_sanitise_offsets(winwid);
		winwidget_render_image(winwid, 0, 1);
	}
	else if (feh_is_kp(EVENT_scroll_left, state, keysym, button)) {
		winwid->im_x += opt.scroll_step;
		winwidget_sanitise_offsets(winwid);
		winwidget_render_image(winwid, 0, 1);
	}
	else if (feh_is_kp(EVENT_scroll_down, state, keysym, button)) {
		winwid->im_y -= opt.scroll_step;
		winwidget_sanitise_offsets(winwid);
		winwidget_render_image(winwid, 0, 1);
	}
	else if (feh_is_kp(EVENT_scroll_up, state, keysym, button)) {
		winwid->im_y += opt.scroll_step;
		winwidget_sanitise_offsets(winwid);
		winwidget_render_image(winwid, 0, 1);
	}
	else if (feh_is_kp(EVENT_scroll_right_page, state, keysym, button)) {
		winwid->im_x -= winwid->w;
		winwidget_sanitise_offsets(winwid);
		winwidget_render_image(winwid, 0, 0);
	}
	else if (feh_is_kp(EVENT_scroll_left_page, state, keysym, button)) {
		winwid->im_x += winwid->w;
		winwidget_sanitise_offsets(winwid);
		winwidget_render_image(winwid, 0, 0);
	}
	else if (feh_is_kp(EVENT_scroll_down_page, state, keysym, button)) {
		winwid->im_y -= winwid->h;
		winwidget_sanitise_offsets(winwid);
		winwidget_render_image(winwid, 0, 0);
	}
	else if (feh_is_kp(EVENT_scroll_up_page, state, keysym, button)) {
		winwid->im_y += winwid->h;
		winwidget_sanitise_offsets(winwid);
		winwidget_render_image(winwid, 0, 0);
	}
	else if (feh_is_kp(EVENT_jump_back, state, keysym, button)) {
		if (opt.slideshow)
			slideshow_change_image(winwid, SLIDE_JUMP_BACK, 1);
		else if (winwid->type == WIN_TYPE_THUMBNAIL)
			feh_thumbnail_select_prev(winwid, 10);
	}
	else if (feh_is_kp(EVENT_jump_fwd, state, keysym, button)) {
		if (opt.slideshow)
			slideshow_change_image(winwid, SLIDE_JUMP_FWD, 1);
		else if (winwid->type == WIN_TYPE_THUMBNAIL)
			feh_thumbnail_select_next(winwid, 10);
	}
	else if (feh_is_kp(EVENT_next_dir, state, keysym, button)) {
		if (opt.slideshow)
			slideshow_change_image(winwid, SLIDE_JUMP_NEXT_DIR, 1);
	}
	else if (feh_is_kp(EVENT_prev_dir, state, keysym, button)) {
		if (opt.slideshow)
			slideshow_change_image(winwid, SLIDE_JUMP_PREV_DIR, 1);
	}
	else if (feh_is_kp(EVENT_quit, state, keysym, button)) {
		winwidget_destroy_all();
	}
	else if (feh_is_kp(EVENT_delete, state, keysym, button)) {
		if (winwid->type == WIN_TYPE_THUMBNAIL_VIEWER)
			feh_thumbnail_mark_removed(FEH_FILE(winwid->file->data), 1);
		feh_filelist_image_remove(winwid, 1);
	}
	else if (feh_is_kp(EVENT_remove, state, keysym, button)) {
		if (winwid->type == WIN_TYPE_THUMBNAIL_VIEWER)
			feh_thumbnail_mark_removed(FEH_FILE(winwid->file->data), 0);
		feh_filelist_image_remove(winwid, 0);
	}
	else if (feh_is_kp(EVENT_jump_first, state, keysym, button)) {
		if (opt.slideshow)
			slideshow_change_image(winwid, SLIDE_FIRST, 1);
	}
	else if (feh_is_kp(EVENT_jump_last, state, keysym, button)) {
		if (opt.slideshow)
			slideshow_change_image(winwid, SLIDE_LAST, 1);
	}
	else if (feh_is_kp(EVENT_action_0, state, keysym, button)) {
		feh_event_invoke_action(winwid, 0);
	}
	else if (feh_is_kp(EVENT_action_1, state, keysym, button)) {
		feh_event_invoke_action(winwid, 1);
	}
	else if (feh_is_kp(EVENT_action_2, state, keysym, button)) {
		feh_event_invoke_action(winwid, 2);
	}
	else if (feh_is_kp(EVENT_action_3, state, keysym, button)) {
		feh_event_invoke_action(winwid, 3);
	}
	else if (feh_is_kp(EVENT_action_4, state, keysym, button)) {
		feh_event_invoke_action(winwid, 4);
	}
	else if (feh_is_kp(EVENT_action_5, state, keysym, button)) {
		feh_event_invoke_action(winwid, 5);
	}
	else if (feh_is_kp(EVENT_action_6, state, keysym, button)) {
		feh_event_invoke_action(winwid, 6);
	}
	else if (feh_is_kp(EVENT_action_7, state, keysym, button)) {
		feh_event_invoke_action(winwid, 7);
	}
	else if (feh_is_kp(EVENT_action_8, state, keysym, button)) {
		feh_event_invoke_action(winwid, 8);
	}
	else if (feh_is_kp(EVENT_action_9, state, keysym, button)) {
		feh_event_invoke_action(winwid, 9);
	}
	else if (feh_is_kp(EVENT_zoom_in, state, keysym, button)) {
		winwid->old_zoom = winwid->zoom;
		winwid->zoom = winwid->zoom * opt.zoom_rate;

		if (winwid->zoom > ZOOM_MAX)
			winwid->zoom = ZOOM_MAX;

		winwid->im_x = (winwid->w / 2) - (((winwid->w / 2) - winwid->im_x) /
			winwid->old_zoom * winwid->zoom);
		winwid->im_y = (winwid->h / 2) - (((winwid->h / 2) - winwid->im_y) /
			winwid->old_zoom * winwid->zoom);
		winwidget_sanitise_offsets(winwid);
		winwidget_render_image(winwid, 0, 0);
	}
	else if (feh_is_kp(EVENT_zoom_out, state, keysym, button)) {
		winwid->old_zoom = winwid->zoom;
		winwid->zoom = winwid->zoom / opt.zoom_rate;

		if (winwid->zoom < ZOOM_MIN)
			winwid->zoom = ZOOM_MIN;

		winwid->im_x = (winwid->w / 2) - (((winwid->w / 2) - winwid->im_x) /
			winwid->old_zoom * winwid->zoom);
		winwid->im_y = (winwid->h / 2) - (((winwid->h / 2) - winwid->im_y) /
			winwid->old_zoom * winwid->zoom);
		winwidget_sanitise_offsets(winwid);
		winwidget_render_image(winwid, 0, 0);
	}
	else if (feh_is_kp(EVENT_zoom_default, state, keysym, button)) {
		winwid->zoom = 1.0;
		winwidget_center_image(winwid);
		winwidget_render_image(winwid, 0, 0);
	}
	else if (feh_is_kp(EVENT_zoom_fit, state, keysym, button)) {
		feh_calc_needed_zoom(&winwid->zoom, winwid->im_w, winwid->im_h, winwid->w, winwid->h);
		winwidget_center_image(winwid);
		winwidget_render_image(winwid, 0, 0);
	}
	else if (feh_is_kp(EVENT_zoom_fill, state, keysym, button)) {
		int save_zoom = opt.zoom_mode;
		opt.zoom_mode = ZOOM_MODE_FILL;
		feh_calc_needed_zoom(&winwid->zoom, winwid->im_w, winwid->im_h, winwid->w, winwid->h);
		winwidget_center_image(winwid);
		winwidget_render_image(winwid, 0, 0);
		opt.zoom_mode = save_zoom;
	}
	else if (feh_is_kp(EVENT_render, state, keysym, button)) {
		if (winwid->type == WIN_TYPE_THUMBNAIL)
			feh_thumbnail_show_selected();
		else
			winwidget_render_image(winwid, 0, 0);
	}
	else if (feh_is_kp(EVENT_toggle_actions, state, keysym, button)) {
		opt.draw_actions = !opt.draw_actions;
		winwidget_rerender_all(0);
	}
	else if (feh_is_kp(EVENT_toggle_aliasing, state, keysym, button)) {
		opt.force_aliasing = !opt.force_aliasing;
		winwid->force_aliasing = !winwid->force_aliasing;
		winwidget_render_image(winwid, 0, 0);
	}
	else if (feh_is_kp(EVENT_toggle_auto_zoom, state, keysym, button)) {
		opt.zoom_mode = (opt.zoom_mode == 0 ? ZOOM_MODE_MAX : 0);
		winwidget_rerender_all(1);
	}
	else if (feh_is_kp(EVENT_toggle_filenames, state, keysym, button)) {
		opt.draw_filename = !opt.draw_filename;
		winwidget_rerender_all(0);
	}
#ifdef HAVE_LIBEXIF
	else if (feh_is_kp(EVENT_toggle_exif, state, keysym, button)) {
		opt.draw_exif = !opt.draw_exif;
		winwidget_rerender_all(0);
	}
#endif
	else if (feh_is_kp(EVENT_toggle_info, state, keysym, button)) {
		opt.draw_info = !opt.draw_info;
		winwidget_rerender_all(0);
	}
	else if (feh_is_kp(EVENT_toggle_pointer, state, keysym, button)) {
		winwidget_set_pointer(winwid, opt.hide_pointer);
		opt.hide_pointer = !opt.hide_pointer;
	}
	else if (feh_is_kp(EVENT_jump_random, state, keysym, button)) {
		if (winwid->type == WIN_TYPE_THUMBNAIL)
			feh_thumbnail_select_next(winwid, random() % (filelist_len - 1));
		else
			slideshow_change_image(winwid, SLIDE_RAND, 1);
	}
	else if (feh_is_kp(EVENT_toggle_caption, state, keysym, button)) {
		if (opt.caption_path && path_is_url(FEH_FILE(winwid->file->data)->filename)) {
			im_weprintf(winwid, "Caption entry is not supported on URLs");
		}
		else if (opt.caption_path) {
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
	else if (feh_is_kp(EVENT_reload_image, state, keysym, button)) {
		feh_reload_image(winwid, 0, 0);
	}
	else if (feh_is_kp(EVENT_toggle_pause, state, keysym, button)) {
		slideshow_pause_toggle(winwid);
		/* We need to re-render the image to update the info string immediately. */
		winwidget_render_image(winwid, 0, 0);
	}
	else if (feh_is_kp(EVENT_save_image, state, keysym, button)) {
		slideshow_save_image(winwid);
	}
	else if (feh_is_kp(EVENT_save_filelist, state, keysym, button)) {
		if ((winwid->type == WIN_TYPE_THUMBNAIL)
				|| (winwid->type == WIN_TYPE_THUMBNAIL_VIEWER))
			weprintf("Filelist saving is not supported in thumbnail mode");
		else
			feh_save_filelist();
	}
	else if (feh_is_kp(EVENT_size_to_image, state, keysym, button)) {
		winwidget_size_to_image(winwid);
	}
	else if (!opt.no_menus && feh_is_kp(EVENT_toggle_menu, state, keysym, button)) {
		winwidget_show_menu(winwid);
	}
	else if (feh_is_kp(EVENT_close, state, keysym, button)) {
		winwidget_destroy(winwid);
	}
	else if (feh_is_kp(EVENT_orient_1, state, keysym, button)) {
		feh_edit_inplace(winwid, 1);
	}
	else if (feh_is_kp(EVENT_orient_3, state, keysym, button)) {
		feh_edit_inplace(winwid, 3);
	}
	else if (feh_is_kp(EVENT_flip, state, keysym, button)) {
		feh_edit_inplace(winwid, INPLACE_EDIT_FLIP);
	}
	else if (feh_is_kp(EVENT_mirror, state, keysym, button)) {
		feh_edit_inplace(winwid, INPLACE_EDIT_MIRROR);
	}
	else if (feh_is_kp(EVENT_toggle_fullscreen, state, keysym, button)) {
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
	else if (feh_is_kp(EVENT_reload_plus, state, keysym, button)){
		if (opt.reload < SLIDESHOW_RELOAD_MAX)
			opt.reload++;
		else if (opt.verbose)
			weprintf("Cannot set RELOAD higher than %f seconds.", opt.reload);
	}
	else if (feh_is_kp(EVENT_reload_minus, state, keysym, button)) {
		if (opt.reload > 1)
			opt.reload--;
		else if (opt.verbose)
			weprintf("Cannot set RELOAD lower than 1 second.");
	}
	else if (feh_is_kp(EVENT_toggle_keep_vp, state, keysym, button)) {
		opt.keep_zoom_vp = !opt.keep_zoom_vp;
	}
	else if (feh_is_kp(EVENT_toggle_fixed_geometry, state, keysym, button)) {
		if (opt.geom_flags & ((WidthValue | HeightValue))) {
			opt.geom_flags &= ~(WidthValue | HeightValue);
		} else {
			opt.geom_flags |= (WidthValue | HeightValue);
			opt.geom_w = winwid->w;
			opt.geom_h = winwid->h;
		}
		winwidget_render_image(winwid, 1, 0);
	}
	return;
}
