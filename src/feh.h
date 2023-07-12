/* feh.h

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

#ifndef FEH_H
#define FEH_H

/*
 * strverscmp(3) is a GNU extension. In most supporting C libraries it
 * requires _GNU_SOURCE to be defined.
 */
#ifdef HAVE_STRVERSCMP
#define _GNU_SOURCE
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <X11/keysym.h>
#include <X11/Xresource.h>
#ifdef HAVE_LIBXINERAMA
#include <X11/extensions/Xinerama.h>
#include <X11/X.h>
#endif				/* HAVE_LIBXINERAMA */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <dirent.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/wait.h>
#include <math.h>
#include <getopt.h>

#include <Imlib2.h>
#include "gib_hash.h"
#include "gib_imlib.h"
#include "gib_list.h"
#include "gib_style.h"

#include "structs.h"
#include "menu.h"

#include "utils.h"

#include "debug.h"

#define SLIDESHOW_RELOAD_MAX 4096

#ifndef TRUE
#define FALSE	0
#define TRUE	!FALSE
#endif

#ifndef __GNUC__
# define __attribute__(x)
#endif

#define XY_IN_RECT(x, y, rx, ry, rw, rh) \
(((x) >= (rx)) && ((y) >= (ry)) && ((x) < ((rx) + (rw))) && ((y) < ((ry) + (rh))))

#define DEFAULT_FONT "yudit/11"
#define DEFAULT_MENU_FONT "yudit/10"
#define DEFAULT_FONT_BIG "yudit/12"
#define DEFAULT_FONT_TITLE "yudit/14"

enum mode_type { MODE_NORMAL = 0, MODE_PAN, MODE_ZOOM, MODE_ROTATE, MODE_BLUR, MODE_NEXT
};

enum bgmode_type { BG_MODE_NONE = 0, BG_MODE_TILE, BG_MODE_CENTER,
	BG_MODE_SCALE, BG_MODE_FILL, BG_MODE_MAX
};

enum zoom_mode { ZOOM_MODE_FILL = 1, ZOOM_MODE_MAX };

enum text_bg { TEXT_BG_CLEAR = 0, TEXT_BG_TINTED };

enum slide_change { SLIDE_NEXT, SLIDE_PREV, SLIDE_RAND, SLIDE_FIRST, SLIDE_LAST,
	SLIDE_JUMP_FWD,
	SLIDE_JUMP_BACK,
	SLIDE_JUMP_NEXT_DIR,
	SLIDE_JUMP_PREV_DIR
};

enum feh_load_error {
	LOAD_ERROR_IMLIB = 0,
	LOAD_ERROR_IMAGEMAGICK,
	LOAD_ERROR_CURL,
	LOAD_ERROR_DCRAW,
	LOAD_ERROR_MAGICBYTES
};

#define INPLACE_EDIT_FLIP   -1
#define INPLACE_EDIT_MIRROR -2

#define ZOOM_MIN 0.002
#define ZOOM_MAX 2000

typedef void (*sighandler_t) (int);

int feh_main_iteration(int block);
void feh_handle_event(XEvent * ev);
void init_imlib_fonts(void);
void init_x_and_imlib(void);
#ifdef HAVE_LIBXINERAMA
void init_xinerama(void);
#endif				/* HAVE_LIBXINERAMA */
void init_multiwindow_mode(void);
void init_thumbnail_mode(void);
void init_index_mode(void);
void init_slideshow_mode(void);
void init_list_mode(void);
void init_loadables_mode(void);
void init_unloadables_mode(void);
#ifdef HAVE_LIBMAGIC
void uninit_magic(void);
void init_magic(void);
#endif
void feh_clean_exit(void);
int feh_should_ignore_image(Imlib_Image * im);
int feh_load_image(Imlib_Image * im, feh_file * file);
void show_mini_usage(void);
void slideshow_change_image(winwidget winwid, int change, int render);
void slideshow_pause_toggle(winwidget w);
void init_keyevents(void);
void init_buttonbindings(void);
void setup_stdin(void);
void restore_stdin(void);
void feh_event_handle_keypress(XEvent * ev);
void feh_event_handle_stdin(void);
void feh_event_handle_generic(winwidget winwid, unsigned int state, KeySym keysym, unsigned int button);
fehkey *feh_str_to_kb(char * action);
void feh_action_run(feh_file * file, char *action, winwidget winwid);
char *format_size(double size);
char *feh_printf(char *str, feh_file * file, winwidget winwid);
void im_weprintf(winwidget w, char *fmt, ...);
void feh_draw_zoom(winwidget w);
void feh_draw_checks(winwidget win);
void cb_slide_timer(void *data);
void cb_reload_timer(void *data);
int feh_load_image_char(Imlib_Image * im, char *filename);
void feh_draw_filename(winwidget w);
#ifdef HAVE_LIBEXIF
void feh_draw_exif(winwidget w);
#endif
void feh_draw_actions(winwidget w);
void feh_draw_caption(winwidget w);
void feh_draw_info(winwidget w);
void feh_draw_errstr(winwidget w);
void feh_display_status(char stat);
void real_loadables_mode(int loadable);
void feh_reload_image(winwidget w, int resize, int force_new);
void feh_filelist_image_remove(winwidget winwid, char do_delete);
void feh_print_load_error(char *file, winwidget w, Imlib_Load_Error err, enum feh_load_error feh_err);
void slideshow_save_image(winwidget win);
void feh_edit_inplace(winwidget w, int orientation);
void feh_edit_inplace_lossless(winwidget w, int orientation);
gib_list *feh_wrap_string(char *text, int wrap_width, Imlib_Font fn, gib_style * style);
char *build_caption_filename(feh_file * file, short create_dir);
gib_list *feh_list_jump(gib_list * root, gib_list * l, int direction, int num);
#ifdef HAVE_INOTIFY
void feh_event_handle_inotify(void);
#endif
#ifndef HAVE_STRVERSCMP
int strverscmp(const char *l0, const char *r0);
#endif

/* Imlib stuff */
extern Display *disp;
extern Visual *vis;
extern Colormap cm;
extern int depth;
extern Atom wmDeleteWindow;

#ifdef HAVE_LIBXINERAMA
extern int num_xinerama_screens;
extern XineramaScreenInfo *xinerama_screens;
extern int xinerama_screen;
#endif				/* HAVE_LIBXINERAMA */

/* Thumbnail sizes */
extern int cmdargc;
extern char **cmdargv;
extern Window root;
extern XContext xid_context;
extern Screen *scr;
extern unsigned char reset_output;
extern feh_menu *menu_main;
extern feh_menu *menu_close;
extern char *mode;		/* label for the current mode */

/* to terminate long-running children with SIGALRM */
extern int childpid;

extern unsigned char control_via_stdin;

#endif
