/* display.c

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

#include "feh.h"
#include "options.h"
#include <locale.h>

#ifdef HAVE_WAYLAND
int feh_using_wayland;
#endif

void feh_display_init(void)
{
	if (opt.backend == FEH_BACKEND_WAYLAND && (opt.bgmode || opt.x11_windowid))
		eprintf("Wallpaper setting and --window-id require the X11 backend");
#ifdef HAVE_WAYLAND
	if (opt.backend == FEH_BACKEND_WAYLAND
			|| (opt.backend == FEH_BACKEND_AUTO && !opt.bgmode && !opt.x11_windowid
				&& (getenv("WAYLAND_DISPLAY") || getenv("WAYLAND_SOCKET")))) {
		setlocale(LC_ALL, "");
		if (feh_wl_init()) {
			feh_using_wayland = 1;
			imlib_context_set_color_modifier(NULL);
			imlib_context_set_progress_function(NULL);
			imlib_context_set_operation(IMLIB_OP_COPY);
			imlib_set_cache_size(opt.cache_size * 1024 * 1024);
			return;
		}
		if (opt.backend == FEH_BACKEND_WAYLAND)
			eprintf("Can't connect to a Wayland display");
	}
#else
	if (opt.backend == FEH_BACKEND_WAYLAND)
		eprintf("Wayland support was disabled at build time (wayland=0)");
#endif
	init_x_and_imlib();
	return;
}

int feh_screen_width(void)
{
#ifdef HAVE_WAYLAND
	if (feh_is_wayland()) {
		int width, height;

		feh_wl_screen_size(&width, &height);
		return(width);
	}
#endif
	return(scr->width);
}

int feh_screen_height(void)
{
#ifdef HAVE_WAYLAND
	if (feh_is_wayland()) {
		int width, height;

		feh_wl_screen_size(&width, &height);
		return(height);
	}
#endif
	return(scr->height);
}
