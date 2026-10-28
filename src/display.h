/* display.h

Copyright (C) 2026 Jason A. Donenfeld <Jason@zx2c4.com>. All Rights Reserved.

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

#ifndef FEH_DISPLAY_H
#define FEH_DISPLAY_H

void feh_display_init(void);
int feh_screen_width(void);
int feh_screen_height(void);

#ifdef HAVE_WAYLAND
extern int feh_using_wayland;
#define feh_is_wayland() (feh_using_wayland)
int feh_wl_init(void);
void feh_wl_shutdown(void);
int feh_wl_iteration(int block);
void feh_wl_screen_size(int *width, int *height);
void feh_wl_create_window(winwidget w, int width, int height);
void feh_wl_destroy_window(winwidget w);
void feh_wl_title(winwidget w);
void feh_wl_show(winwidget w);
void feh_wl_hide(winwidget w);
void feh_wl_resize(winwidget w, int width, int height, int force);
void feh_wl_fullscreen(winwidget w, int fullscreen);
void feh_wl_prepare_image(winwidget w);
void feh_wl_present(winwidget w);
void feh_wl_set_pointer(winwidget w, int visible);
void feh_wl_pointer_position(winwidget w, int *x, int *y);
void feh_wl_menu_show(feh_menu *m);
void feh_wl_menu_hide(feh_menu *m);
void feh_wl_menu_redraw(feh_menu *m);
#else
#define feh_is_wayland() 0
#define feh_wl_shutdown() ((void)0)
#define feh_wl_iteration(block) 0
#define feh_wl_create_window(w, width, height) ((void)0)
#define feh_wl_destroy_window(w) ((void)0)
#define feh_wl_title(w) ((void)0)
#define feh_wl_show(w) ((void)0)
#define feh_wl_hide(w) ((void)0)
#define feh_wl_resize(w, width, height, force) ((void)0)
#define feh_wl_fullscreen(w, fullscreen) ((void)0)
#define feh_wl_prepare_image(w) ((void)0)
#define feh_wl_present(w) ((void)0)
#define feh_wl_set_pointer(w, visible) ((void)0)
#define feh_wl_pointer_position(w, x, y) (*(x) = *(y) = 0)
#define feh_wl_menu_show(m) ((void)0)
#define feh_wl_menu_hide(m) ((void)0)
#define feh_wl_menu_redraw(m) ((void)0)
#endif
#endif
