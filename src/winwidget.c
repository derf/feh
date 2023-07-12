/* winwidget.c

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
#include "options.h"
#include "events.h"
#include "timers.h"

#ifdef HAVE_INOTIFY
#include <sys/inotify.h>
#endif

static void winwidget_unregister(winwidget win);
static void winwidget_register(winwidget win);
static winwidget winwidget_allocate(void);


int window_num = 0;		/* For window list */
winwidget *windows = NULL;	/* List of windows to loop though */

static winwidget winwidget_allocate(void)
{
	winwidget ret = NULL;

	ret = emalloc(sizeof(_winwidget));
	memset(ret, 0, sizeof(_winwidget));

	ret->win = 0;
	ret->w = 0;
	ret->h = 0;
	ret->full_screen = 0;
	ret->im_w = 0;
	ret->im_h = 0;
	ret->im_angle = 0;
	ret->bg_pmap = 0;
	ret->bg_pmap_cache = 0;
	ret->im = NULL;
	ret->name = NULL;
	ret->file = NULL;
	ret->errstr = NULL;
	ret->type = WIN_TYPE_UNSET;
	ret->visible = 0;
	ret->caption_entry = 0;
	ret->force_aliasing = opt.force_aliasing;

	/* Zoom stuff */
	ret->mode = MODE_NORMAL;

	ret->gc = None;

	/* New stuff */
	ret->im_x = 0;
	ret->im_y = 0;
	ret->zoom = 1.0;
	ret->old_zoom = 1.0;

	ret->click_offset_x = 0;
	ret->click_offset_y = 0;
	ret->has_rotated = 0;

#ifdef HAVE_INOTIFY
    ret->inotify_wd = -1;
#endif

	return(ret);
}

winwidget winwidget_create_from_image(Imlib_Image im, char type)
{
	winwidget ret = NULL;

	if (im == NULL)
		return(NULL);

	ret = winwidget_allocate();
	ret->type = type;

	ret->im = im;
	ret->w = ret->im_w = gib_imlib_image_get_width(ret->im);
	ret->h = ret->im_h = gib_imlib_image_get_height(ret->im);

	if (opt.full_screen && (type != WIN_TYPE_THUMBNAIL)) {
		ret->full_screen = True;
	} else if (opt.default_zoom) {
		ret->zoom = 0.01 * opt.default_zoom;
		ret->w *= ret->zoom;
		ret->h *= ret->zoom;
	}
	winwidget_create_window(ret, ret->w, ret->h);
	winwidget_render_image(ret, 1, 0);

	return(ret);
}

winwidget winwidget_create_from_file(gib_list * list, char type)
{
	winwidget ret = NULL;
	feh_file *file = FEH_FILE(list->data);

	if (!file || !file->filename)
		return(NULL);

	ret = winwidget_allocate();
	ret->file = list;
	ret->type = type;

	if ((winwidget_loadimage(ret, file) == 0) || feh_should_ignore_image(ret->im)) {
		winwidget_destroy(ret);
		return(NULL);
	}

	if (!ret->win) {
		ret->w = ret->im_w = gib_imlib_image_get_width(ret->im);
		ret->h = ret->im_h = gib_imlib_image_get_height(ret->im);
		D(("image is %dx%d pixels, format %s\n", ret->w, ret->h, gib_imlib_image_format(ret->im)));
		if (opt.full_screen) {
			ret->full_screen = True;
		} else if (opt.default_zoom) {
			ret->zoom = 0.01 * opt.default_zoom;
			ret->w *= ret->zoom;
			ret->h *= ret->zoom;
		}
		winwidget_create_window(ret, ret->w, ret->h);
		winwidget_render_image(ret, 1, 0);
	}

	return(ret);
}

void winwidget_create_window(winwidget ret, int w, int h)
{
	XWindowAttributes window_attr;
	XSetWindowAttributes attr;
	XEvent ev;
	XClassHint *xch;
	MWMHints mwmhints;
	Atom prop = None;
	pid_t pid;
	int x = 0;
	int y = 0;
	char *tmpname;
#ifdef HOST_NAME_MAX
	char hostname[HOST_NAME_MAX];
#else /* ! HOST_NAME_MAX */
	long host_name_max;
	char *hostname;
#endif /* HOST_NAME_MAX */

	D(("winwidget_create_window %dx%d\n", w, h));

	if (ret->full_screen) {
		w = scr->width;
		h = scr->height;

#ifdef HAVE_LIBXINERAMA
		if (opt.xinerama && xinerama_screens) {
			w = xinerama_screens[xinerama_screen].width;
			h = xinerama_screens[xinerama_screen].height;
			x = xinerama_screens[xinerama_screen].x_org;
			y = xinerama_screens[xinerama_screen].y_org;
		}
#endif				/* HAVE_LIBXINERAMA */
	} else if (opt.geom_flags) {
		if (opt.geom_flags & WidthValue) {
			w = opt.geom_w;
		}
		if (opt.geom_flags & HeightValue) {
			h = opt.geom_h;
		}
		if (opt.geom_flags & XValue) {
			if (opt.geom_flags & XNegative) {
				x = scr->width - opt.geom_x;
			} else {
				x = opt.geom_x;
			}
		}
		if (opt.geom_flags & YValue) {
			if (opt.geom_flags & YNegative) {
				y = scr->height - opt.geom_y;
			} else {
				y = opt.geom_y;
			}
		}
	} else if (opt.screen_clip) {
		if (w > scr->width)
			w = scr->width;
		if (h > scr->height)
			h = scr->height;

#ifdef HAVE_LIBXINERAMA
		if (opt.xinerama && xinerama_screens) {
			if (w > xinerama_screens[xinerama_screen].width)
				w = xinerama_screens[xinerama_screen].width;
			if (h > xinerama_screens[xinerama_screen].height)
				h = xinerama_screens[xinerama_screen].height;
		}
#endif				/* HAVE_LIBXINERAMA */
	}

	if (opt.paused) {
		tmpname = estrjoin(" ", ret->name, "[Paused]", NULL);
		free(ret->name);
		ret->name = tmpname;
	}

	ret->x = x;
	ret->y = y;
	ret->w = w;
	ret->h = h;
	ret->visible = False;

	attr.backing_store = NotUseful;
	attr.override_redirect = False;
	attr.colormap = cm;
	attr.border_pixel = 0;
	attr.background_pixel = 0;
	attr.save_under = False;
	attr.event_mask =
	    StructureNotifyMask | ButtonPressMask | ButtonReleaseMask |
	    PointerMotionMask | EnterWindowMask | LeaveWindowMask |
	    KeyPressMask | KeyReleaseMask | ButtonMotionMask | ExposureMask
	    | FocusChangeMask | PropertyChangeMask | VisibilityChangeMask;

	memset(&mwmhints, 0, sizeof(mwmhints));
	if (opt.borderless || ret->full_screen) {
		prop = XInternAtom(disp, "_MOTIF_WM_HINTS", True);
		if (prop == None) {
			weprintf
			    ("Window Manager does not support MWM hints. "
			     "To get a borderless window I have to bypass your wm.");
			attr.override_redirect = True;
			mwmhints.flags = 0;
		} else {
			mwmhints.flags = MWM_HINTS_DECORATIONS;
			mwmhints.decorations = 0;
		}
	}

	if (opt.x11_windowid == 0) {
		ret->win =
		  XCreateWindow(disp, DefaultRootWindow(disp), x, y, w, h, 0,
		                depth, InputOutput, vis,
		                CWOverrideRedirect | CWSaveUnder | CWBackingStore
		                | CWColormap | CWBackPixel | CWBorderPixel | CWEventMask,
		                &attr);
	} else {
		/* x11_windowid is a pointer to the window */
		ret->win = (Window) opt.x11_windowid;

		/* set our attributes on the window */
		XChangeWindowAttributes(disp, ret->win,
                            CWOverrideRedirect | CWSaveUnder | CWBackingStore
                            | CWColormap | CWBackPixel | CWBorderPixel
                            | CWEventMask, &attr);

		/* determine the size and visibility of the window */
		XGetWindowAttributes(disp, ret->win, &window_attr);
		ret->visible = (window_attr.map_state == IsViewable);
		ret->x = window_attr.x;
		ret->y = window_attr.y;
		ret->w = window_attr.width;
		ret->h = window_attr.height;
	}

	if (mwmhints.flags) {
		XChangeProperty(disp, ret->win, prop, prop, 32,
				PropModeReplace, (unsigned char *) &mwmhints, PROP_MWM_HINTS_ELEMENTS);
	}
	if (ret->full_screen) {
		Atom prop_fs = XInternAtom(disp, "_NET_WM_STATE_FULLSCREEN", False);
		Atom prop_state = XInternAtom(disp, "_NET_WM_STATE", False);

		memset(&ev, 0, sizeof(ev));
		ev.xclient.type = ClientMessage;
		ev.xclient.message_type = prop_state;
		ev.xclient.display = disp;
		ev.xclient.window = ret->win;
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = 1;
		ev.xclient.data.l[1] = prop_fs;

		XChangeProperty(disp, ret->win, prop_state, XA_ATOM, 32,
				PropModeReplace, (unsigned char *) &prop_fs, 1);
	}

	pid = getpid();
	prop = XInternAtom(disp, "_NET_WM_PID", False);
	XChangeProperty(disp, ret->win, prop, XA_CARDINAL, sizeof(pid_t) * 8,
			PropModeReplace, (const unsigned char *)&pid, 1);

#ifdef HOST_NAME_MAX
	if (gethostname(hostname, HOST_NAME_MAX) == 0) {
		hostname[HOST_NAME_MAX-1] = '\0';
		prop = XInternAtom(disp, "WM_CLIENT_MACHINE", False);
		XChangeProperty(disp, ret->win, prop, XA_STRING, sizeof(char) * 8,
				PropModeReplace, (unsigned char *)hostname, strlen(hostname));
	}
#else /* ! HOST_NAME_MAX */
	if ((host_name_max = sysconf(_SC_HOST_NAME_MAX)) != -1 ) {
		if ((hostname = calloc(1, host_name_max + 1)) != NULL ) {
			if (gethostname(hostname, host_name_max) == 0) {
				prop = XInternAtom(disp, "WM_CLIENT_MACHINE", False);
				XChangeProperty(disp, ret->win, prop, XA_STRING, sizeof(char) * 8,
						PropModeReplace, (unsigned char *)hostname, strlen(hostname));
			}
			free(hostname);
		}
	}
#endif /* HOST_NAME_MAX */

	XSetWMProtocols(disp, ret->win, &wmDeleteWindow, 1);
	winwidget_update_title(ret);
	xch = XAllocClassHint();
	xch->res_name = "feh";
	xch->res_class = opt.x11_class ? opt.x11_class : "feh";
	XSetClassHint(disp, ret->win, xch);
	XFree(xch);

	/* Size hints */
	if (ret->full_screen || opt.geom_flags) {
		XSizeHints xsz;

		xsz.flags = USPosition;
		xsz.x = x;
		xsz.y = y;
		XSetWMNormalHints(disp, ret->win, &xsz);
		XMoveWindow(disp, ret->win, x, y);
	}
	if (opt.hide_pointer)
		winwidget_set_pointer(ret, 0);

	/* set the icon name property */
	XSetIconName(disp, ret->win, "feh");
	/* set the command hint */
	XSetCommand(disp, ret->win, cmdargv, cmdargc);

	winwidget_register(ret);

	/* do not scale down a thumbnail list window, only those created from it */
	if (opt.geom_enabled && (ret->type != WIN_TYPE_THUMBNAIL)) {
		opt.geom_w = w;
		opt.geom_h = h;
		opt.geom_flags |= WidthValue | HeightValue;
	}

	return;
}

void winwidget_update_title(winwidget ret)
{
	char *name;
	Atom prop_name = XInternAtom(disp, "_NET_WM_NAME", False);
	Atom prop_icon = XInternAtom(disp, "_NET_WM_ICON_NAME", False);
	Atom prop_utf8 = XInternAtom(disp, "UTF8_STRING", False);

	D(("winwid->name = %s\n", ret->name));
	name = ret->name ? ret->name : "feh";
	XStoreName(disp, ret->win, name);
	XSetIconName(disp, ret->win, name);

	XChangeProperty(disp, ret->win, prop_name, prop_utf8, 8,
			PropModeReplace, (const unsigned char *)name, strlen(name));

	XChangeProperty(disp, ret->win, prop_icon, prop_utf8, 8,
			PropModeReplace, (const unsigned char *)name, strlen(name));

	return;
}

void winwidget_update_caption(winwidget winwid)
{
	if (opt.caption_path) {
		/* TODO: Does someone understand the caching here. Is this the right
		 * approach now that I have broken this out into a separate function. -lsmith */

		/* cache bg pixmap. during caption entry, multiple redraws are done
		 * because the caption overlay changes - the image doesn't though, so re-
		 * rendering that is a waste of time */
		if (winwid->caption_entry) {
			GC gc;
			if (winwid->bg_pmap_cache)
				XFreePixmap(disp, winwid->bg_pmap_cache);
			winwid->bg_pmap_cache = XCreatePixmap(disp, winwid->win, winwid->w, winwid->h, depth);
			gc = XCreateGC(disp, winwid->win, 0, NULL);
			XCopyArea(disp, winwid->bg_pmap, winwid->bg_pmap_cache, gc, 0, 0, winwid->w, winwid->h, 0, 0);
			XFreeGC(disp, gc);
		}
		feh_draw_caption(winwid);
	}
	return;
}

void winwidget_setup_pixmaps(winwidget winwid)
{
	if (winwid->full_screen) {
		if (!(winwid->bg_pmap)) {
			if (winwid->gc == None) {
				XGCValues gcval;

				if (!opt.image_bg || !strcmp(opt.image_bg, "default")) {
					gcval.foreground = BlackPixel(disp, DefaultScreen(disp));
					winwid->gc = XCreateGC(disp, winwid->win, GCForeground, &gcval);
				} else if (!strcmp(opt.image_bg, "checks")) {
					gcval.tile = feh_create_checks();
					gcval.fill_style = FillTiled;
					winwid->gc = XCreateGC(disp, winwid->win, GCTile | GCFillStyle, &gcval);
				} else {
					XColor color;
					Colormap cmap = DefaultColormap(disp, DefaultScreen(disp));
					XAllocNamedColor(disp, cmap, (char*) opt.image_bg, &color, &color);
					gcval.foreground = color.pixel;
					winwid->gc = XCreateGC(disp, winwid->win, GCForeground, &gcval);
				}
			}
			winwid->bg_pmap = XCreatePixmap(disp, winwid->win, scr->width, scr->height, depth);
		}
		XFillRectangle(disp, winwid->bg_pmap, winwid->gc, 0, 0, scr->width, scr->height);
	} else {
		if (!winwid->bg_pmap || winwid->had_resize) {
			D(("recreating background pixmap (%dx%d)\n", winwid->w, winwid->h));
			if (winwid->bg_pmap)
				XFreePixmap(disp, winwid->bg_pmap);

			if (winwid->w == 0)
				winwid->w = 1;
			if (winwid->h == 0)
				winwid->h = 1;
			winwid->bg_pmap = XCreatePixmap(disp, winwid->win, winwid->w, winwid->h, depth);
			winwid->had_resize = 0;
		}
	}
	return;
}

void winwidget_render_image(winwidget winwid, int resize, int force_alias)
{
	int sx, sy, sw, sh, dx, dy, dw, dh;
	int calc_w, calc_h;
	int antialias = 0;

	if (!winwid->full_screen && resize) {
		if (opt.default_zoom) {
			winwid->zoom = 0.01 * opt.default_zoom;
			winwidget_resize(winwid, winwid->im_w * winwid->zoom, winwid->im_h * winwid->zoom, 0);
		} else {
			winwidget_resize(winwid, winwid->im_w, winwid->im_h, 0);
		}
		winwidget_reset_image(winwid);
	}

	D(("winwidget_render_image resize %d force_alias %d im %dx%d\n",
	      resize, force_alias, winwid->im_w, winwid->im_h));

	/* winwidget_setup_pixmaps(winwid) resets the winwid->had_resize flag */
	int had_resize = winwid->had_resize || resize;

	winwidget_setup_pixmaps(winwid);

	if (had_resize && !opt.keep_zoom_vp && (winwid->type != WIN_TYPE_THUMBNAIL)) {
		double required_zoom = 1.0;
		feh_calc_needed_zoom(&required_zoom, winwid->im_w, winwid->im_h, winwid->w, winwid->h);

		winwid->zoom = opt.default_zoom ? (0.01 * opt.default_zoom) : 1.0;

		if ((opt.scale_down || (winwid->full_screen && !opt.default_zoom))
				&& winwid->zoom > required_zoom)
			winwid->zoom = required_zoom;
		else if ((opt.zoom_mode && required_zoom > 1)
				&& (!opt.default_zoom || required_zoom < winwid->zoom))
			winwid->zoom = required_zoom;

		if (opt.offset_flags & XValue) {
			if (opt.offset_flags & XNegative) {
				winwid->im_x = winwid->w - (winwid->im_w * winwid->zoom) - opt.offset_x;
			} else {
				winwid->im_x = - opt.offset_x * winwid->zoom;
			}
		} else {
			winwid->im_x = (int) (winwid->w - (winwid->im_w * winwid->zoom)) >> 1;
		}
		if (opt.offset_flags & YValue) {
			if (opt.offset_flags & YNegative) {
				winwid->im_y = winwid->h - (winwid->im_h * winwid->zoom) - opt.offset_y;
			} else {
				winwid->im_y = - opt.offset_y * winwid->zoom;
			}
		} else {
			winwid->im_y = (int) (winwid->h - (winwid->im_h * winwid->zoom)) >> 1;
		}
	}

	winwid->had_resize = 0;

	if (opt.keep_zoom_vp)
		winwidget_sanitise_offsets(winwid);

	if (!winwid->full_screen && ((gib_imlib_image_has_alpha(winwid->im))
				     || (opt.geom_flags & (WidthValue | HeightValue))
				     || (winwid->im_x || winwid->im_y)
				     || (winwid->w > winwid->im_w * winwid->zoom)
				     || (winwid->h > winwid->im_h * winwid->zoom)
				     || (winwid->has_rotated)))
		feh_draw_checks(winwid);

	/* Now we ensure only to render the area we're looking at */
	dx = winwid->im_x;
	dy = winwid->im_y;
	if (dx < 0)
		dx = 0;
	if (dy < 0)
		dy = 0;

	if (winwid->im_x < 0)
		sx = 0 - lround(winwid->im_x / winwid->zoom);
	else
		sx = 0;

	if (winwid->im_y < 0)
		sy = 0 - lround(winwid->im_y / winwid->zoom);
	else
		sy = 0;

	calc_w = lround(winwid->im_w * winwid->zoom);
	calc_h = lround(winwid->im_h * winwid->zoom);
	dw = (winwid->w - winwid->im_x);
	dh = (winwid->h - winwid->im_y);
	if (calc_w < dw)
		dw = calc_w;
	if (calc_h < dh)
		dh = calc_h;
	if (dw > winwid->w)
		dw = winwid->w;
	if (dh > winwid->h)
		dh = winwid->h;

	sw = lround(dw / winwid->zoom);
	sh = lround(dh / winwid->zoom);

	D(("sx: %d sy: %d sw: %d sh: %d dx: %d dy: %d dw: %d dh: %d zoom: %f\n",
	   sx, sy, sw, sh, dx, dy, dw, dh, winwid->zoom));

	if ((winwid->zoom != 1.0 || winwid->has_rotated) && !force_alias && !winwid->force_aliasing)
		antialias = 1;

	D(("winwidget_render(): winwid->im_angle = %f\n", winwid->im_angle));
	if (winwid->has_rotated)
		gib_imlib_render_image_part_on_drawable_at_size_with_rotation
			(winwid->bg_pmap, winwid->im, sx, sy, sw, sh, dx, dy, dw, dh,
			winwid->im_angle, 1, 1, antialias);
	else
		gib_imlib_render_image_part_on_drawable_at_size(winwid->bg_pmap,
								winwid->im,
								sx, sy, sw,
								sh, dx, dy,
								dw, dh, 1,
								gib_imlib_image_has_alpha(winwid->im),
								antialias);

	if (opt.mode == MODE_NORMAL) {
		if (opt.caption_path)
			winwidget_update_caption(winwid);
		if (opt.draw_filename)
			feh_draw_filename(winwid);
#ifdef HAVE_LIBEXIF
		if (opt.draw_exif)
			feh_draw_exif(winwid);
#endif
		if (opt.draw_actions)
			feh_draw_actions(winwid);
		if (opt.draw_info && opt.info_cmd)
			feh_draw_info(winwid);
		if (winwid->errstr)
			feh_draw_errstr(winwid);
		if (winwid->file != NULL) {
			if (opt.title && winwid->type != WIN_TYPE_THUMBNAIL_VIEWER) {
				winwidget_rename(winwid, feh_printf(opt.title, FEH_FILE(winwid->file->data), winwid));
			} else if (opt.thumb_title && winwid->type == WIN_TYPE_THUMBNAIL_VIEWER) {
				winwidget_rename(winwid, feh_printf(opt.thumb_title, FEH_FILE(winwid->file->data), winwid));
			}
		}
	} else if ((opt.mode == MODE_ZOOM) && !antialias)
		feh_draw_zoom(winwid);

	XSetWindowBackgroundPixmap(disp, winwid->win, winwid->bg_pmap);
	XClearWindow(disp, winwid->win);
	return;
}

void winwidget_render_image_cached(winwidget winwid)
{
	static GC gc = None;

	if (gc == None) {
		gc = XCreateGC(disp, winwid->win, 0, NULL);
	}
	XCopyArea(disp, winwid->bg_pmap_cache, winwid->bg_pmap, gc, 0, 0, winwid->w, winwid->h, 0, 0);

	if (opt.caption_path)
		feh_draw_caption(winwid);
	if (opt.draw_filename)
		feh_draw_filename(winwid);
	if (opt.draw_actions)
		feh_draw_actions(winwid);
	if (opt.draw_info && opt.info_cmd)
		feh_draw_info(winwid);
	XSetWindowBackgroundPixmap(disp, winwid->win, winwid->bg_pmap);
	XClearWindow(disp, winwid->win);
}

double feh_calc_needed_zoom(double *zoom, int orig_w, int orig_h, int dest_w, int dest_h)
{
	double ratio = 0.0;

	ratio = ((double) orig_w / orig_h) / ((double) dest_w / dest_h);

	if (opt.zoom_mode == ZOOM_MODE_FILL)
		ratio = 1.0 / ratio;

	if (ratio > 1.0)
		*zoom = ((double) dest_w / orig_w);
	else
		*zoom = ((double) dest_h / orig_h);

	return(ratio);
}

Pixmap feh_create_checks(void)
{
	static Pixmap checks_pmap = None;
	Imlib_Image checks = NULL;

	if (checks_pmap == None) {
		checks = imlib_create_image(16, 16);

		if (!checks)
			eprintf("Unable to create a teeny weeny imlib image. I detect problems");

		if (!opt.image_bg || !strcmp(opt.image_bg, "default") || !strcmp(opt.image_bg, "checks")) {
			gib_imlib_image_fill_rectangle(checks, 0, 0, 16, 16, 144, 144, 144, 255);
			gib_imlib_image_fill_rectangle(checks, 0, 0,  8,  8, 100, 100, 100, 255);
			gib_imlib_image_fill_rectangle(checks, 8, 8,  8,  8, 100, 100, 100, 255);
		} else {
			XColor color;
			Colormap cmap = DefaultColormap(disp, DefaultScreen(disp));
			XAllocNamedColor(disp, cmap, (char*) opt.image_bg, &color, &color);
			gib_imlib_image_fill_rectangle(checks, 0, 0, 16, 16, color.red, color.green, color.blue, 255);
		}

		checks_pmap = XCreatePixmap(disp, root, 16, 16, depth);
		gib_imlib_render_image_on_drawable(checks_pmap, checks, 0, 0, 1, 0, 0);
		gib_imlib_free_image_and_decache(checks);
	}
	return(checks_pmap);
}

void feh_draw_checks(winwidget win)
{
	static GC gc = None;
	XGCValues gcval;

	if (gc == None) {
		gcval.tile = feh_create_checks();
		gcval.fill_style = FillTiled;
		gc = XCreateGC(disp, win->win, GCTile | GCFillStyle, &gcval);
	}
	XFillRectangle(disp, win->bg_pmap, gc, 0, 0, win->w, win->h);
	return;
}

void winwidget_destroy_xwin(winwidget winwid)
{
	if (winwid->win) {
		winwidget_unregister(winwid);
		XDestroyWindow(disp, winwid->win);
	}
	if (winwid->bg_pmap) {
		XFreePixmap(disp, winwid->bg_pmap);
		winwid->bg_pmap = None;
	}
	return;
}

void winwidget_destroy(winwidget winwid)
{
#ifdef HAVE_INOTIFY
    winwidget_inotify_remove(winwid);
#endif
	if (opt.reload > 0 && opt.multiwindow) {
		feh_remove_timer_by_data(winwid);
	}
	winwidget_destroy_xwin(winwid);
	if (winwid->name)
		free(winwid->name);
	if (winwid->gc)
		XFreeGC(disp, winwid->gc);
	if (winwid->im)
		gib_imlib_free_image_and_decache(winwid->im);
	free(winwid);
	return;
}

#ifdef HAVE_INOTIFY
void winwidget_inotify_remove(winwidget winwid)
{
    if (winwid->inotify_wd >= 0) {
        D(("Removing inotify watch\n"));
        if (inotify_rm_watch(opt.inotify_fd, winwid->inotify_wd))
            weprintf("inotify_rm_watch failed:");
        winwid->inotify_wd = -1;
    }
}
#endif

#ifdef HAVE_INOTIFY
void winwidget_inotify_add(winwidget winwid, feh_file * file)
{
    if (opt.auto_reload && !path_is_url(file->filename)) {
        D(("Adding inotify watch for %s\n", file->filename));
        char dir[PATH_MAX];
        feh_file_dirname(dir, file, PATH_MAX);

        /*
         * Handle files without directory part, e.g. "feh somefile.jpg".
         * These always reside in the current directory.
         */
        if (dir[0] == '\0') {
            dir[0] = '.';
            dir[1] = '\0';
        }
        winwid->inotify_wd = inotify_add_watch(opt.inotify_fd, dir, IN_CLOSE_WRITE | IN_MOVED_TO);
        if (winwid->inotify_wd < 0)
            weprintf("inotify_add_watch failed:");
    }
}
#endif

#ifdef HAVE_INOTIFY
#define INOTIFY_BUFFER_LEN (1024 * (sizeof (struct inotify_event)) + 16)
void feh_event_handle_inotify(void)
{
    D(("Received inotify events\n"));
    char buf[INOTIFY_BUFFER_LEN];
    int i = 0;
    int len = read (opt.inotify_fd, buf, INOTIFY_BUFFER_LEN);
    if (len < 0) {
        if (errno != EINTR)
            eprintf("inotify event read failed");
    } else if (!len)
        eprintf("inotify event read failed");
    while (i < len) {
        struct inotify_event *event;
        event = (struct inotify_event *) &buf[i];
        for (int j = 0; j < window_num; j++) {
            if(windows[j]->inotify_wd == event->wd) {
                if (event->mask & IN_IGNORED) {
                    D(("inotify watch was implicitly removed\n"));
                    windows[j]->inotify_wd = -1;
                } else if (event->mask & (IN_CLOSE_WRITE | IN_MOVED_TO)) {
                    if (strcmp(event->name, FEH_FILE(windows[j]->file->data)->name) == 0) {
                        D(("inotify says file changed\n"));
                        feh_reload_image(windows[j], 0, 0);
                    }
                }
                break;
            }
        }
        i += sizeof(struct inotify_event) + event->len;
    }
}
#endif

void winwidget_destroy_all(void)
{
	int i;

	/* Have to DESCEND the list here, 'cos of the way _unregister works */
	for (i = window_num - 1; i >= 0; i--)
		winwidget_destroy(windows[i]);
	return;
}

void winwidget_rerender_all(int resize)
{
	int i;

	/* Have to DESCEND the list here, 'cos of the way _unregister works */
	for (i = window_num - 1; i >= 0; i--)
		winwidget_render_image(windows[i], resize, 0);
	return;
}

winwidget winwidget_get_first_window_of_type(unsigned int type)
{
	int i;

	for (i = 0; i < window_num; i++)
		if (windows[i]->type == type)
			return(windows[i]);
	return(NULL);
}

int winwidget_loadimage(winwidget winwid, feh_file * file)
{
	D(("filename %s\n", file->filename));
#ifdef HAVE_INOTIFY
    winwidget_inotify_remove(winwid);
#endif
    int res = feh_load_image(&(winwid->im), file);
#ifdef HAVE_INOTIFY
    if (res) {
        winwidget_inotify_add(winwid, file);
    }
#endif
	return(res);
}

void winwidget_show(winwidget winwid)
{
	XEvent ev;

	/* feh_debug_print_winwid(winwid); */
	if (!winwid->visible) {
		XMapWindow(disp, winwid->win);
		if (opt.full_screen)
			XMoveWindow(disp, winwid->win, 0, 0);
		/* wait for the window to map */
		D(("Waiting for window to map\n"));
		XMaskEvent(disp, StructureNotifyMask, &ev);
		winwidget_get_geometry(winwid, NULL);

		/* Unfortunately, StructureNotifyMask does not only mask
		 * the events of type MapNotify (which we want to mask here)
		 * but also such of type ConfigureNotify (and others, see
		 * https://tronche.com/gui/x/xlib/events/processing-overview.html),
		 * which should be handled, especially on tiling wm's. To
		 * remedy this, the handler is executed explicitly:
		 */
		if (ev.type == ConfigureNotify)
			feh_event_handle_ConfigureNotify(&ev);
		D(("Window mapped\n"));
		winwid->visible = 1;
	}
	return;
}

void winwidget_move(winwidget winwid, int x, int y)
{
	if (winwid && ((winwid->x != x) || (winwid->y != y))) {
		winwid->x = (x > scr->width) ? scr->width : x;
		winwid->y = (y > scr->height) ? scr->height : y;
		XMoveWindow(disp, winwid->win, winwid->x, winwid->y);
		XFlush(disp);
	} else {
		D(("No move actually needed\n"));
	}
	return;
}

void winwidget_resize(winwidget winwid, int w, int h, int force_resize)
{
	XWindowAttributes attributes;
	int tc_x, tc_y, px, py;
	int scr_width = scr->width;
	int scr_height = scr->height;

	/* discarded */
	Window dw;
	int di, i;
	unsigned int du;

	XGetWindowAttributes(disp, winwid->win, &attributes);

#ifdef HAVE_LIBXINERAMA
	if (opt.xinerama && xinerama_screens) {
		xinerama_screen = 0;
		XQueryPointer(disp, root, &dw, &dw, &px, &py, &di, &di, &du);
		for (i = 0; i < num_xinerama_screens; i++) {
			if (XY_IN_RECT(px, py,
						xinerama_screens[i].x_org,
						xinerama_screens[i].y_org,
						xinerama_screens[i].width,
						xinerama_screens[i].height)) {
				xinerama_screen = i;
				break;
			}

		}
		if (opt.xinerama_index >= 0)
			xinerama_screen = opt.xinerama_index;

		scr_width = xinerama_screens[xinerama_screen].width;
		scr_height = xinerama_screens[xinerama_screen].height;
	}
#endif


	D(("   x %d y %d w %d h %d\n", attributes.x, attributes.y, winwid->w,
		winwid->h));

	if ((opt.geom_flags & (WidthValue | HeightValue)) && !force_resize) {
		winwid->had_resize = 1;
		return;
	}
	if (winwid && ((winwid->w != w) || (winwid->h != h))) {
		if (opt.screen_clip) {
			double required_zoom = winwid->zoom;
			if (opt.scale_down && !opt.keep_zoom_vp) {
				int max_w = (w > scr_width) ? scr_width : w;
				int max_h = (h > scr_height) ? scr_height : h;
				feh_calc_needed_zoom(&required_zoom, winwid->im_w, winwid->im_h, max_w, max_h);
			}
			int desired_w = winwid->im_w * required_zoom;
			int desired_h = winwid->im_h * required_zoom;
			winwid->w = (desired_w > scr_width) ? scr_width : desired_w;
			winwid->h = (desired_h > scr_height) ? scr_height : desired_h;
		}
		if (winwid->full_screen) {
			XTranslateCoordinates(disp, winwid->win, attributes.root,
						-attributes.border_width - attributes.x,
						-attributes.border_width - attributes.y, &tc_x, &tc_y, &dw);
			winwid->x = tc_x;
			winwid->y = tc_y;
			XMoveResizeWindow(disp, winwid->win, tc_x, tc_y, winwid->w, winwid->h);
		} else
			XResizeWindow(disp, winwid->win, winwid->w, winwid->h);

		winwid->had_resize = 1;
		XFlush(disp);

		winwidget_get_geometry(winwid, NULL);

		if (force_resize && (opt.geom_flags & (WidthValue | HeightValue))
				&& (winwid->type != WIN_TYPE_THUMBNAIL)) {
			opt.geom_w = winwid->w;
			opt.geom_h = winwid->h;
		}

		D(("-> x %d y %d w %d h %d\n", winwid->x, winwid->y, winwid->w,
			winwid->h));

	} else {
		D(("No resize actually needed\n"));
	}

	return;
}

void winwidget_hide(winwidget winwid)
{
	XUnmapWindow(disp, winwid->win);
	winwid->visible = 0;
	return;
}

static void winwidget_register(winwidget win)
{
	D(("window %p\n", win));
	window_num++;
	if (windows)
		windows = erealloc(windows, window_num * sizeof(winwidget));
	else
		windows = emalloc(window_num * sizeof(winwidget));
	windows[window_num - 1] = win;

	XSaveContext(disp, win->win, xid_context, (XPointer) win);
	return;
}

static void winwidget_unregister(winwidget win)
{
	int i, j;

	for (i = 0; i < window_num; i++) {
		if (windows[i] == win) {
			for (j = i; j < window_num - 1; j++)
				windows[j] = windows[j + 1];
			window_num--;
			if (window_num > 0)
				windows = erealloc(windows, window_num * sizeof(winwidget));
			else {
				free(windows);
				windows = NULL;
			}
		}
	}
	XDeleteContext(disp, win->win, xid_context);
	return;
}

winwidget winwidget_get_from_window(Window win)
{
	winwidget ret = NULL;

	if (XFindContext(disp, win, xid_context, (XPointer *) & ret) != XCNOENT)
		return(ret);
	return(NULL);
}

void winwidget_rename(winwidget winwid, char *newname)
{
	/* newname == NULL -> update current title */
	char *p_str;

	if (newname == NULL)
		newname = estrdup(winwid->name ? winwid->name : "");
	if (winwid->name)
		free(winwid->name);

	winwid->name = emalloc(strlen(newname) + 10);
	strcpy(winwid->name, newname);

	if (strlen(winwid->name) > 9)
		p_str = winwid->name + strlen(winwid->name) - 9;
	else
		p_str = winwid->name;

	if (opt.paused && strcmp(p_str, " [Paused]") != 0)
		strcat(winwid->name, " [Paused]");
	else if (!opt.paused && strcmp(p_str, " [Paused]") == 0)
		*p_str = '\0';

	winwidget_update_title(winwid);
	return;
}

void winwidget_free_image(winwidget w)
{
	if (w->im) {
		gib_imlib_free_image(w->im);
	}
	w->im = NULL;
	w->im_w = 0;
	w->im_h = 0;
	return;
}

void feh_debug_print_winwid(winwidget w)
{
	printf("winwid_debug:\n" "winwid = %p\n" "win = %ld\n" "w = %d\n"
	       "h = %d\n" "im_w = %d\n" "im_h = %d\n" "im_angle = %f\n"
	       "type = %d\n" "had_resize = %d\n" "im = %p\n" "GC = %p\n"
	       "pixmap = %ld\n" "name = %s\n" "file = %p\n" "mode = %d\n"
	       "im_x = %d\n" "im_y = %d\n" "zoom = %f\n" "old_zoom = %f\n"
	       "click_offset_x = %d\n" "click_offset_y = %d\n"
	       "has_rotated = %d\n", (void *)w, w->win, w->w, w->h, w->im_w,
	       w->im_h, w->im_angle, w->type, w->had_resize, w->im, (void *)w->gc,
	       w->bg_pmap, w->name, (void *)w->file, w->mode, w->im_x, w->im_y,
	       w->zoom, w->old_zoom, w->click_offset_x, w->click_offset_y,
	       w->has_rotated);
}

void winwidget_reset_image(winwidget winwid)
{
	if (!opt.keep_zoom_vp) {
		winwid->zoom = 1.0;
		winwid->old_zoom = 1.0;
		winwid->im_x = 0;
		winwid->im_y = 0;
	}
	winwid->im_angle = 0.0;
	winwid->has_rotated = 0;
	return;
}

void winwidget_center_image(winwidget winwid)
{
	int scr_width, scr_height;

	scr_width = scr->width;
	scr_height = scr->height;

#ifdef HAVE_LIBXINERAMA
	if (opt.xinerama && xinerama_screens) {
		scr_width = xinerama_screens[xinerama_screen].width;
		scr_height = xinerama_screens[xinerama_screen].height;
	}
#endif				/* HAVE_LIBXINERAMA */

	if (winwid->full_screen) {
		winwid->im_x = (scr_width - lround(winwid->im_w * winwid->zoom)) >> 1;
		winwid->im_y = (scr_height - lround(winwid->im_h * winwid->zoom)) >> 1;
	} else {
		if (opt.geom_flags & WidthValue)
			winwid->im_x = ((int)opt.geom_w - lround(winwid->im_w * winwid->zoom)) >> 1;
		else
			winwid->im_x = 0;
		if (opt.geom_flags & HeightValue)
			winwid->im_y = ((int)opt.geom_h - lround(winwid->im_h * winwid->zoom)) >> 1;
		else
			winwid->im_y = 0;
	}
}

void winwidget_sanitise_offsets(winwidget winwid)
{
	int far_left, far_top;
	int min_x, max_x, max_y, min_y;

	far_left = winwid->w - (winwid->im_w * winwid->zoom);
	far_top = winwid->h - (winwid->im_h * winwid->zoom);

	if ((winwid->im_w * winwid->zoom) > winwid->w) {
		min_x = far_left;
		max_x = 0;
	} else {
		min_x = 0;
		max_x = far_left;
	}
	if ((winwid->im_h * winwid->zoom) > winwid->h) {
		min_y = far_top;
		max_y = 0;
	} else {
		min_y = 0;
		max_y = far_top;
	}
	if (winwid->im_x > max_x)
		winwid->im_x = max_x;
	if (winwid->im_x < min_x)
		winwid->im_x = min_x;
	if (winwid->im_y > max_y)
		winwid->im_y = max_y;
	if (winwid->im_y < min_y)
		winwid->im_y = min_y;

	return;
}

void winwidget_size_to_image(winwidget winwid)
{
	winwidget_resize(winwid, winwid->im_w * winwid->zoom, winwid->im_h * winwid->zoom, 1);
	winwid->im_x = winwid->im_y = 0;
	winwidget_render_image(winwid, 0, 0);
	return;
}

void winwidget_set_pointer(winwidget winwid, int visible)
{
	Cursor no_ptr;
	XColor black, dummy;
	Pixmap bm_no;
	static char bm_no_data[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

	if (visible)
		XUndefineCursor(disp, winwid->win);
	else {
		bm_no = XCreateBitmapFromData(disp, winwid->win, bm_no_data, 8, 8);
		XAllocNamedColor(disp, DefaultColormapOfScreen(DefaultScreenOfDisplay(disp)), "black", &black, &dummy);

		no_ptr = XCreatePixmapCursor(disp, bm_no, bm_no, &black, &black, 0, 0);
		XDefineCursor(disp, winwid->win, no_ptr);
	}
}

int winwidget_get_width(winwidget winwid)
{
	int rect[4];
	winwidget_get_geometry(winwid, rect);
	return(rect[2]);
}

int winwidget_get_height(winwidget winwid)
{
	int rect[4];
	winwidget_get_geometry(winwid, rect);
	return(rect[3]);
}

void winwidget_get_geometry(winwidget winwid, int *rect)
{
	unsigned int bw, bp;
	Window child;

	int inner_rect[4];

	if (!rect)
		rect = inner_rect;

	XGetGeometry(disp, winwid->win, &root, &(rect[0]), &(rect[1]), (unsigned
				int *)&(rect[2]), (unsigned int *)&(rect[3]), &bw, &bp);

	XTranslateCoordinates(disp, winwid->win, root, 0, 0, &(rect[0]), &(rect[1]), &child);

	/* update the window geometry (in case it's inaccurate) */
	winwid->x = rect[0];
	winwid->y = rect[1];
	winwid->w = rect[2];
	winwid->h = rect[3];
	return;
}

void winwidget_show_menu(winwidget winwid)
{
	int x, y, b;
	unsigned int c;
	Window r;

	XQueryPointer(disp, winwid->win, &r, &r, &x, &y, &b, &b, &c);
	if (winwid->type == WIN_TYPE_SINGLE) {
		if (!menu_single_win)
			feh_menu_init_single_win();
		feh_menu_show_at_xy(menu_single_win, winwid, x, y);
	} else if (winwid->type == WIN_TYPE_THUMBNAIL) {
		if (!menu_thumbnail_win)
			feh_menu_init_thumbnail_win();
		feh_menu_show_at_xy(menu_thumbnail_win, winwid, x, y);
	} else if (winwid->type == WIN_TYPE_THUMBNAIL_VIEWER) {
		if (!menu_single_win)
			feh_menu_init_thumbnail_viewer();
		feh_menu_show_at_xy(menu_thumbnail_viewer, winwid, x, y);
	} else {
		if (!menu_main)
			feh_menu_init_main();
		feh_menu_show_at_xy(menu_main, winwid, x, y);
	}
}
