/* winwidget.h

Copyright (C) 1999-2003 Tom Gilbert.

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

#ifndef WINWIDGET_H
#define WINWIDGET_H

/* This MWM stuff pinched from Eterm/src/command.h */

# include <X11/X.h>
# include <X11/Xproto.h>

/* Motif window hints */
#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define MWM_HINTS_INPUT_MODE    (1L << 2)
#define MWM_HINTS_STATUS        (1L << 3)
/* bit definitions for MwmHints.functions */
#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)
/* bit definitions for MwmHints.decorations */
#define MWM_DECOR_ALL           (1L << 0)
#define MWM_DECOR_BORDER        (1L << 1)
#define MWM_DECOR_RESIZEH       (1L << 2)
#define MWM_DECOR_TITLE         (1L << 3)
#define MWM_DECOR_MENU          (1L << 4)
#define MWM_DECOR_MINIMIZE      (1L << 5)
#define MWM_DECOR_MAXIMIZE      (1L << 6)
/* bit definitions for MwmHints.inputMode */
#define MWM_INPUT_MODELESS                  0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 1
#define MWM_INPUT_SYSTEM_MODAL              2
#define MWM_INPUT_FULL_APPLICATION_MODAL    3
#define PROP_MWM_HINTS_ELEMENTS             5

/* Motif window hints */
typedef struct _mwmhints
{
   CARD32 flags;
   CARD32 functions;
   CARD32 decorations;
   INT32 input_mode;
   CARD32 status;
}
MWMHints;

enum win_type
{
   WIN_TYPE_UNSET, WIN_TYPE_SLIDESHOW, WIN_TYPE_SINGLE, WIN_TYPE_ABOUT,
   WIN_TYPE_THUMBNAIL, WIN_TYPE_THUMBNAIL_VIEWER
};

struct __winwidget
{
   Window win;
   int x;
   int y;
   int w;
   int h;
   int im_w;
   int im_h;
   double im_angle;
   enum win_type type;
   unsigned char had_resize, full_screen;
   Imlib_Image im;
   GC gc;
   Pixmap bg_pmap;
   Pixmap bg_pmap_cache;
   char *name;
   gib_list *file;
   unsigned char visible;

   /* Stuff for zooming */
   unsigned char mode;

   unsigned char caption_entry; /* are we in caption entry mode? */

   /* New stuff */
   int im_x;                    /* image offset from window top left */
   int im_y;                    /* image offset from window top left */
   double zoom;                 /* From 0 (not visible) to 100 (actual size)
                                   all the way up to INT_MAX (ouch) */
   int click_offset_x;
   int click_offset_y;
   int im_click_offset_x;
   int im_click_offset_y;

   unsigned char has_rotated;
};

int winwidget_loadimage(winwidget winwid, feh_file * filename);
void winwidget_show(winwidget winwid);
void winwidget_show_menu(winwidget winwid);
void winwidget_hide(winwidget winwid);
void winwidget_destroy_all(void);
void winwidget_free_image(winwidget w);
void winwidget_render_image(winwidget winwid, int resize, int alias);
void winwidget_rotate_image(winwidget winid, double angle);
void winwidget_move(winwidget winwid, int x, int y);
void winwidget_resize(winwidget winwid, int w, int h);
void winwidget_setup_pixmaps(winwidget winwid);
void winwidget_update_title(winwidget ret);
void winwidget_rerender_all(int resize, int alias);
void winwidget_destroy_xwin(winwidget winwid);
int winwidget_count(void);

void winwidget_get_geometry(winwidget winwid, int *rect);
int winwidget_get_width(winwidget winwid);
int winwidget_get_height(winwidget winwid);
winwidget winwidget_get_from_window(Window win);
winwidget winwidget_create_from_file(gib_list * filename, char *name,

                                     char type);
winwidget winwidget_create_from_image(Imlib_Image im, char *name, char type);
void winwidget_rename(winwidget winwid, char *newname);
void winwidget_destroy(winwidget winwid);
void winwidget_create_window(winwidget ret, int w, int h);
void winwidget_clear_background(winwidget w);
Pixmap feh_create_checks(void);
double feh_calc_needed_zoom(double *zoom, int orig_w, int orig_h, int dest_w,

                            int dest_h);
void feh_debug_print_winwid(winwidget winwid);
winwidget winwidget_get_first_window_of_type(unsigned int type);
void winwidget_reset_image(winwidget winwid);
void winwidget_sanitise_offsets(winwidget winwid);
void winwidget_size_to_image(winwidget winwid);
void winwidget_render_image_cached(winwidget winwid);

extern int window_num;          /* For window list */
extern winwidget *windows;      /* List of windows to loop though */

#endif
