/* winwidget.c

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

static void winwidget_unregister(winwidget w);
static void winwidget_register(winwidget w);


winwidget winwidget_allocate(void)
{
	winwidget ret = NULL;

	ret = ecalloc(sizeof(_winwidget));    /* calloc() zeros out all members */

	ret->type   = WIN_TYPE_UNSET;
	ret->gc = None;

	/* New stuff */
	ret->zoom = ret->old_zoom = 1.0;

	return(ret);
}

winwidget winwidget_create_from_image(LLMD *md, Imlib_Image im, char *name, char type)
{
	winwidget ret = NULL;

	if (im == NULL)
		return(NULL);

	ret = winwidget_allocate();
	ret->md   = md;
	ret->type = type;

	ret->im = im;
	ret->wide = ret->im_w = gib_imlib_image_get_width(ret->im);
	ret->high = ret->im_h = gib_imlib_image_get_height(ret->im);

	if (name)
		ret->name = estrdup(name);
	else
		ret->name = estrdup(PACKAGE);

	if (opt.flg.full_screen
		  && (type != WIN_TYPE_THUMBNAIL)
		  && (type != WIN_TYPE_MOVE_MODE))
		ret->full_screen = True;
	winwidget_create_window(ret, ret->wide, ret->high);
	winwidget_render_image(ret, 1, 0, SANITIZE_NO);

	return(ret);
}

winwidget winwidget_create_from_file( LLMD * md, feh_node * node, char *name, char type)
{
	winwidget ret = NULL;

	if (!NODE_DATA(node) || !NODE_FILENAME(node) )
		return(NULL);

	ret = winwidget_allocate();
	ret->md   = md;
	ret->node = node;
	ret->type = type;
	if (name)
		ret->name = estrdup(name);
	else
		ret->name = estrdup(NODE_FILENAME(node));

	if (winwidget_loadimage(ret, NODE_DATA(node)) == 0) {
		winwidget_destroy(ret);
		return(NULL);
	}

	if (!ret->win) {
		ret->wide = ret->im_w = gib_imlib_image_get_width(ret->im);
		ret->high = ret->im_h = gib_imlib_image_get_height(ret->im);
		D(("image is %dx%d pixels, format %s\n", ret->wide, ret->high, gib_imlib_image_format(ret->im)));
		if (opt.flg.full_screen)
			ret->full_screen = True;
		winwidget_create_window(ret, ret->wide, ret->high);
		winwidget_render_image(ret, 1, 0, SANITIZE_NO);
	}

	return(ret);
}

void winwidget_create_window(winwidget w, int wide, int high)
{
	XSetWindowAttributes attr;
	XEvent ev;
	XClassHint *xch;
	MWMHints mwmhints;
	Atom prop = None;
	int x = 0;
	int y = 0;

	D(("winwidget_create_window %dx%d\n", wide, high));

	if (w->full_screen) {
		wide = fgv.scr->width;
		high = fgv.scr->height;

#ifdef HAVE_LIBXINERAMA
		if (opt.flg.xinerama && fgv.xinerama_screens) {
			wide = fgv.xinerama_screens[fgv.xinerama_screen].width;
			high = fgv.xinerama_screens[fgv.xinerama_screen].height;
			x = fgv.xinerama_screens[fgv.xinerama_screen].x_org;
			y = fgv.xinerama_screens[fgv.xinerama_screen].y_org;
		}
#endif				/* HAVE_LIBXINERAMA */
	} else if (opt.geom_flags) {
		if (opt.geom_flags & WidthValue) {
			wide = opt.geom_w;
		}
		if (opt.geom_flags & HeightValue) {
			high = opt.geom_h;
		}
		if (opt.geom_flags & XValue) {
			if (opt.geom_flags & XNegative) {
				x = fgv.scr->width - opt.geom_x;
			} else {
				x = opt.geom_x;
			}
		}
		if (opt.geom_flags & YValue) {
			if (opt.geom_flags & YNegative) {
				y = fgv.scr->height - opt.geom_y;
			} else {
				y = opt.geom_y;
			}
		}
	} else if (opt.flg.screen_clip) {
		if (wide > fgv.scr->width)
			wide = fgv.scr->width;
		if (high > fgv.scr->height)
			high = fgv.scr->height;

#ifdef HAVE_LIBXINERAMA
		if (opt.flg.xinerama && fgv.xinerama_screens) {
			if (wide > fgv.xinerama_screens[fgv.xinerama_screen].width)
				wide = fgv.xinerama_screens[fgv.xinerama_screen].width;
			if (high > fgv.xinerama_screens[fgv.xinerama_screen].height)
				high = fgv.xinerama_screens[fgv.xinerama_screen].height;
		}
#endif				/* HAVE_LIBXINERAMA */
	}

	w->x       = x;
	w->y       = y;
	w->wide    = wide;
	w->high    = high;
	w->visible = False;

	attr.backing_store = NotUseful;
	attr.override_redirect = False;
	attr.colormap = fgv.cm;
	attr.border_pixel = 0;
	attr.background_pixel = 0;
	attr.save_under = False;
	attr.event_mask =
	    StructureNotifyMask | ButtonPressMask | ButtonReleaseMask |
	    PointerMotionMask | EnterWindowMask | LeaveWindowMask |
	    KeyPressMask | KeyReleaseMask | ButtonMotionMask | ExposureMask
	    | FocusChangeMask | PropertyChangeMask | VisibilityChangeMask;

	if (opt.flg.borderless || w->full_screen) {
		prop = XInternAtom(fgv.disp, "_MOTIF_WM_HINTS", True);
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
	} else
		mwmhints.flags = 0;

	w->win =
	    XCreateWindow(fgv.disp, DefaultRootWindow(fgv.disp), x, y, wide, high, 0,
			  fgv.depth, InputOutput, fgv.vis,
			  CWOverrideRedirect | CWSaveUnder | CWBackingStore
			  | CWColormap | CWBackPixel | CWBorderPixel | CWEventMask, &attr);

	if (mwmhints.flags) {
		XChangeProperty(fgv.disp, w->win, prop, prop, 32,
				PropModeReplace, (unsigned char *) &mwmhints, PROP_MWM_HINTS_ELEMENTS);
	}
	if (w->full_screen) {
		Atom prop_fs = XInternAtom(fgv.disp, "_NET_WM_STATE_FULLSCREEN", False);
		Atom prop_state = XInternAtom(fgv.disp, "_NET_WM_STATE", False);

		memset(&ev, 0, sizeof(ev));
		ev.xclient.type         = ClientMessage;
		ev.xclient.message_type = prop_state;
		ev.xclient.display      = fgv.disp;
		ev.xclient.window       = w->win;
		ev.xclient.format       = 32;
		ev.xclient.data.l[0]    = 1;
		ev.xclient.data.l[1]    = prop_fs;

		XChangeProperty(fgv.disp, w->win, prop_state, XA_ATOM, 32,
				PropModeReplace, (unsigned char *) &prop_fs, 1);
	}

	XSetWMProtocols(fgv.disp, w->win, &fgv.wmDeleteWindow, 1);
	winwidget_update_title(w, NULL);
	xch = XAllocClassHint();
	xch->res_name = xch->res_class = "feh";
	XSetClassHint(fgv.disp, w->win, xch);
	XFree(xch);

	/* Size hints */
	if (w->full_screen || opt.geom_flags) {
		XSizeHints xsz;

		xsz.flags = USPosition;
		xsz.x = x;
		xsz.y = y;
		XSetWMNormalHints(fgv.disp, w->win, &xsz);
		XMoveWindow(fgv.disp, w->win, x, y);
	}
	if (opt.flg.hide_pointer)
		winwidget_set_pointer(w, 0);

	/* set the icon name property */
	XSetIconName(fgv.disp, w->win, "feh");
	/* set the command hint */
	XSetCommand(fgv.disp, w->win, fgv.cmdargv, fgv.cmdargc);

	winwidget_register(w);
	return;
}

void winwidget_update_title(winwidget w, char *newname ){
		/* simplified and combined with old winwidget_rename() */

	char *name = mobs(2);

	Atom prop_name = XInternAtom(fgv.disp, "_NET_WM_NAME", False);
	Atom prop_icon = XInternAtom(fgv.disp, "_NET_WM_ICON_NAME", False);
	Atom prop_utf8 = XInternAtom(fgv.disp, "UTF8_STRING", False);

	if ( newname ){
		free(w->name);
		w->name = estrdup( newname);
	}

	D(("w->name = %s\n", w->name));
	if ( w->errstr ) {               /* cheat.  Put the warning FIRST */
		STRCAT_2ITEMS( name, " Hey! ", w->errstr);
		w->errstr = NULL;
	}
	strcat( name, w->name ? w->name : " feh");
	strcat( name, opt.flg.paused ? " [Paused]":"");
	XStoreName(fgv.disp, w->win, name);
	XSetIconName(fgv.disp, w->win, name);

	XChangeProperty(fgv.disp, w->win, prop_name, prop_utf8, 8,
			PropModeReplace, (const unsigned char *)name, strlen(name));

	XChangeProperty(fgv.disp, w->win, prop_icon, prop_utf8, 8,
			PropModeReplace, (const unsigned char *)name, strlen(name));

	return;
}

void winwidget_update_caption(winwidget w)
{
	if (opt.caption_path) {
		/* TODO: Does someone understand the caching here. Is this the right
		 * approach now that I have broken this out into a separate function. -lsmith */

		/* cache bg pixmap. during caption entry, multiple redraws are done
		 * because the caption overlay changes - the image doesn't though, so re-
		 * rendering that is a waste of time */
		if (w->caption_entry) {
			GC gc;
			if (w->bg_pmap_cache)
				XFreePixmap(fgv.disp, w->bg_pmap_cache);
			w->bg_pmap_cache = XCreatePixmap(fgv.disp, w->win, w->wide, w->high, fgv.depth);
			gc = XCreateGC(fgv.disp, w->win, 0, NULL);
			XCopyArea(fgv.disp, w->bg_pmap, w->bg_pmap_cache, gc, 0, 0, w->wide, w->high, 0, 0);
			XFreeGC(fgv.disp, gc);
		}
		feh_draw_caption(w);
	}
	return;
}

void winwidget_setup_pixmaps(winwidget w)
{
	if (w->full_screen) {
		if (!(w->bg_pmap)) {
			if (w->gc == None) {
				XGCValues gcval;

				if (opt.flg.image_bg == IMAGE_BG_WHITE)
					gcval.foreground = WhitePixel(fgv.disp, DefaultScreen(fgv.disp));
				else
					gcval.foreground = BlackPixel(fgv.disp, DefaultScreen(fgv.disp));
				w->gc = XCreateGC(fgv.disp, w->win, GCForeground, &gcval);
			}
			w->bg_pmap = XCreatePixmap(fgv.disp, w->win, fgv.scr->width, fgv.scr->height, fgv.depth);
		}
		XFillRectangle(fgv.disp, w->bg_pmap, w->gc, 0, 0, fgv.scr->width, fgv.scr->height);
	} else {
		if (!w->bg_pmap || w->had_resize) {
			D(("recreating background pixmap (%dx%d)\n", w->wide, w->high));
			if (w->bg_pmap)
				XFreePixmap(fgv.disp, w->bg_pmap);

			if (w->wide == 0)
				w->wide = 1;
			if (w->high == 0)
				w->high = 1;
			w->bg_pmap = XCreatePixmap(fgv.disp, w->win, w->wide, w->high, fgv.depth);
			w->had_resize = 0;
		}
	}
	return;
}

void winwidget_render_image(winwidget w, int resize, int force_alias, int sanitize)
{
	int sx, sy, sw, sh, dx, dy, dw, dh;
	int calc_w, calc_h;
	int antialias = 0;
	int need_center = w->had_resize;

	if ( sanitize )
			winwidget_sanitise_offsets(w);

	if (!w->full_screen && resize) {
		winwidget_resize(w, w->im_w, w->im_h);
		winwidget_reset_image(w);
	}

	/* bounds checks for panning */
	if (w->im_x > w->wide)
		w->im_x = w->wide;
	if (w->im_y > w->high)
		w->im_y = w->high;

	D(("winwidget_render_image resize %d force_alias %d im %dx%d\n",
			resize, force_alias, w->im_w, w->im_h));

	winwidget_setup_pixmaps(w);

	if (!w->full_screen && opt.flg.scale_down && ((w->wide < w->im_w)
						       || (w->high < w->im_h)) &&
						       (w->old_zoom == 1.0)) {
		D(("scaling down image %dx%d\n", w->wide, w->high));

		feh_calc_needed_zoom(&(w->zoom), w->im_w, w->im_h, w->wide, w->high);
		if (resize)
			 winwidget_resize(w, w->im_w * w->zoom, w->im_h * w->zoom);
		D(("after scaling down image %dx%d\n", w->wide, w->high));
	}

	if (!w->full_screen && ((gib_imlib_image_has_alpha(w->im))
		      || (opt.geom_flags & (WidthValue | HeightValue))
		      || (w->im_x || w->im_y) || (w->zoom != 1.0)
		      || (w->wide > w->im_w || w->high > w->im_h)
		      || (w->has_rotated)))
		feh_draw_checks(w);

	if (!w->full_screen && opt.zoom_mode
				     && (w->zoom == 1.0) && ! (opt.geom_flags & (WidthValue | HeightValue))
				     && (w->wide > w->im_w) && (w->high > w->im_h))
		feh_calc_needed_zoom(&(w->zoom), w->im_w, w->im_h, w->wide, w->high);

	if (resize && (w->full_screen || (opt.geom_flags & (WidthValue | HeightValue)))) {
		int smaller;    /* Is the image smaller than screen? */
		int max_w = 0, max_h = 0;

		if (w->full_screen) {
			max_w = fgv.scr->width;
			max_h = fgv.scr->height;
#ifdef HAVE_LIBXINERAMA
			if (opt.flg.xinerama && fgv.xinerama_screens) {
				max_w = fgv.xinerama_screens[fgv.xinerama_screen].width;
				max_h = fgv.xinerama_screens[fgv.xinerama_screen].height;
			}
#endif				/* HAVE_LIBXINERAMA */
		} else {
			if (opt.geom_flags & WidthValue) {
				max_w = opt.geom_w;
			}
			if (opt.geom_flags & HeightValue) {
				max_h = opt.geom_h;
			}
		}

		D(("Calculating for fullscreen/fixed geom render\n"));
		smaller = ((w->im_w < max_w)
			   && (w->im_h < max_h));

		if (!smaller || opt.zoom_mode) {
			double ratio = 0.0;

			/* Image is larger than the screen (so wants shrinking), or it's
			   smaller but wants expanding to fill it */
			ratio = feh_calc_needed_zoom(&(w->zoom), w->im_w, w->im_h, max_w, max_h);

			/* contributed by Jens Laas <jens.laas@data.slu.se>
			 * What it does:
			 * zooms images by a fixed amount but never larger than the screen.
			 *
			 * Why:
			 * This is nice if you got a collection of images where some
			 * are small and can stand a small zoom. Large images are unaffected.
			 *
			 * When does it work, and how?
			 * You have to be in fullscreen mode _and_ have auto-zoom turned on.
			 *   "feh -FZ --zoom 130 imagefile" will do the trick.
			 *        -zoom percent - the new switch.
			 *                        100 = orignal size,
			 *                        130 is 30% larger.
			 */
			if (opt.default_zoom) {
				double old_zoom = w->zoom;

				w->zoom = 0.01 * opt.default_zoom;
				if (w->zoom != 1.0) {
					if ((w->im_h * w->zoom) > max_h)
						w->zoom = old_zoom;
					else if ((w->im_w * w->zoom) > max_w)
						w->zoom = old_zoom;
				}

				w->im_x = ((int)
						(max_w - (w->im_w * w->zoom))) >> 1;
				w->im_y = ((int)
						(max_h - (w->im_h * w->zoom))) >> 1;
			} else {
				if (ratio > 1.0) {
					/* height is the factor */
					w->im_x = 0;
					w->im_y = ((int)
							(max_h - (w->im_h * w->zoom))) >> 1;
				} else {
					/* width is the factor */
					w->im_x = ((int)
							(max_w - (w->im_w * w->zoom))) >> 1;
					w->im_y = 0;
				}
			}
		} else {
			/* my modification to jens hack, allow --zoom without auto-zoom mode */
			if (opt.default_zoom) {
				w->zoom = 0.01 * opt.default_zoom;
			} else {
				w->zoom = 1.0;
			}
			/* Just center the image in the window */
			w->im_x = (int) (max_w - (w->im_w * w->zoom)) >> 1;
			w->im_y = (int) (max_h - (w->im_h * w->zoom)) >> 1;
		}
	}
	else if (need_center && !w->full_screen && opt.flg.scale_down) {
		w->im_x = (int) (w->wide - (w->im_w * w->zoom)) >> 1;
		w->im_y = (int) (w->high - (w->im_h * w->zoom)) >> 1;
	}

	/* Now we ensure only to render the area we're looking at */
	dx = w->im_x;
	dy = w->im_y;
	if (dx < 0)
		dx = 0;
	if (dy < 0)
		dy = 0;

	if (w->im_x < 0)
		sx = 0 - lround(w->im_x / w->zoom);
	else
		sx = 0;

	if (w->im_y < 0)
		sy = 0 - lround(w->im_y / w->zoom);
	else
		sy = 0;

	calc_w = lround(w->im_w * w->zoom);
	calc_h = lround(w->im_h * w->zoom);
	dw = (w->wide - w->im_x);
	dh = (w->high - w->im_y);
	if (calc_w < dw)
		dw = calc_w;
	if (calc_h < dh)
		dh = calc_h;
	if (dw > w->wide)
		dw = w->wide;
	if (dh > w->high)
		dh = w->high;

	sw = lround(dw / w->zoom);
	sh = lround(dh / w->zoom);

	D(("sx: %d sy: %d sw: %d sh: %d dx: %d dy: %d dw: %d dh: %d zoom: %f\n",
	   sx, sy, sw, sh, dx, dy, dw, dh, w->zoom));

	if ((w->zoom != 1.0) && !force_alias && !opt.force_aliasing)
		antialias = 1;

	D(("winwidget_render(): w->im_angle = %f\n", w->im_angle));
	if (w->has_rotated)
		gib_imlib_render_image_part_on_drawable_at_size_with_rotation
			(w->bg_pmap, w->im, sx, sy, sw, sh, dx, dy, dw, dh,
			w->im_angle, 1, 1, antialias);
	else
		gib_imlib_render_image_part_on_drawable_at_size(w->bg_pmap,
				      w->im,
				      sx, sy, sw,
				      sh, dx, dy,
				      dw, dh, 1,
				      gib_imlib_image_has_alpha(w->im),
				      antialias);

	if (opt.flg.state == STATE_NORMAL) {
		if (opt.caption_path)
			winwidget_update_caption(w);
		if (opt.flg.draw_filename)
			feh_draw_filename(w);
#ifdef HAVE_LIBEXIF
		if (opt.flg.draw_exif)
			feh_draw_exif(w);
#endif
		if (opt.flg.draw_actions)
			feh_draw_actions(w);
		if (opt.flg.draw_info && opt.info_cmd)
			feh_draw_info(w);
		if (w->errstr)
			feh_draw_errstr(w);
	} else if ((opt.flg.state == STATE_ZOOM) && !antialias)
			feh_draw_zoom(w);

	XSetWindowBackgroundPixmap(fgv.disp, w->win, w->bg_pmap);
	XClearWindow(fgv.disp, w->win);
	return;
}

void winwidget_render_image_cached(winwidget w)
{
	static GC gc = None;

	if (gc == None) {
		gc = XCreateGC(fgv.disp, w->win, 0, NULL);
	}
	XCopyArea(fgv.disp, w->bg_pmap_cache, w->bg_pmap, gc, 0, 0, w->wide, w->high, 0, 0);

	if (opt.caption_path)
		feh_draw_caption(w);
	if (opt.flg.draw_filename)
		feh_draw_filename(w);
	if (opt.flg.draw_actions)
		feh_draw_actions(w);
	if (opt.flg.draw_info && opt.info_cmd)
		feh_draw_info(w);
	XSetWindowBackgroundPixmap(fgv.disp, w->win, w->bg_pmap);
	XClearWindow(fgv.disp, w->win);
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
		checks = feh_imlib_image_make_n_fill_text_bg( 16,16,0 );

		if (opt.flg.image_bg == IMAGE_BG_WHITE)
			gib_imlib_image_fill_rectangle(checks, 0, 0, 16, 16, 255, 255, 255, 255);
		else if (opt.flg.image_bg == IMAGE_BG_BLACK)
			gib_imlib_image_fill_rectangle(checks, 0, 0, 16, 16, 0, 0, 0, 255);
		else {
			gib_imlib_image_fill_rectangle(checks, 0, 0, 16, 16, 144, 144, 144, 255);
			gib_imlib_image_fill_rectangle(checks, 0, 0,  8,  8, 100, 100, 100, 255);
			gib_imlib_image_fill_rectangle(checks, 8, 8,  8,  8, 100, 100, 100, 255);
		}

		checks_pmap = XCreatePixmap(fgv.disp, fgv.root, 16, 16, fgv.depth);
		gib_imlib_render_image_on_drawable(checks_pmap, checks, 0, 0, 1, 0, 0);
		gib_imlib_free_image_and_decache(checks);
	}
	return(checks_pmap);
}

#if 0
void winwidget_clear_background(winwidget w)
{
	XSetWindowBackgroundPixmap(fgv.disp, w->win, feh_create_checks());
	/* XClearWindow(disp, w->win); */
	return;
}
#endif

void feh_draw_checks(winwidget w)
{
	static GC gc = None;
	XGCValues gcval;

	if (gc == None) {
		gcval.tile = feh_create_checks();
		gcval.fill_style = FillTiled;
		gc = XCreateGC(fgv.disp, w->win, GCTile | GCFillStyle, &gcval);
	}
	XFillRectangle(fgv.disp, w->bg_pmap, gc, 0, 0, w->wide, w->high);
	return;
}

void winwidget_destroy_xwin(winwidget w)
{
	if (w->win) {
		winwidget_unregister(w);
		XDestroyWindow(fgv.disp, w->win);
	}
	if (w->bg_pmap) {
		XFreePixmap(fgv.disp, w->bg_pmap);
		w->bg_pmap = None;
	}
	return;
}

void winwidget_destroy(winwidget w)
{
	winwidget_destroy_xwin(w);
	if (w->name)
		free(w->name);
	if (w->gc)
		XFreeGC(fgv.disp, w->gc);
	if (w->im)
		gib_imlib_free_image_and_decache(w->im);
	free(w);
	return;
}

void winwidget_destroy_all(void)
{
	int i;

	/* Have to DESCEND the list here, 'cos of the way _unregister works */
	for (i = fgv.window_num - 1; i >= 0; i--)
		winwidget_destroy(fgv.windows[i]);
	return;
}

void winwidget_rerender_all(int resize)
{
	int i;

	/* Have to DESCEND the list here, 'cos of the way _unregister works */
	for (i = fgv.window_num - 1; i >= 0; i--)
		winwidget_render_image(fgv.windows[i], resize, 0, SANITIZE_NO);
	return;
}

winwidget winwidget_get_first_window_of_type(unsigned int type)
{
	int i;

	for (i = 0; i < fgv.window_num; i++)
		if (fgv.windows[i]->type == type)
			return(fgv.windows[i]);
	return(NULL);
}

int winwidget_loadimage(winwidget w, feh_data * data)
{
	D(("filename %s\n", data->filename));
	return(feh_load_image(&(w->im), data));
}

void winwidget_show(winwidget w)
{
	XEvent ev;

#ifdef DEBUG
	 feh_debug_print_winwid(w);
#endif
	if (!w->visible) {
		XMapWindow(fgv.disp, w->win);
		if (opt.flg.full_screen)
			XMoveWindow(fgv.disp, w->win, 0, 0);
		D(("Waiting for window to map\n"));
		XMaskEvent(fgv.disp, StructureNotifyMask, &ev);
		D(("Window mapped\n"));
		w->visible = 1;
	}
	return;
}

void winwidget_move(winwidget w, int x, int y)
{
	if (w && ((w->x != x) || (w->y != y))) {
		w->x = x;
		w->y = y;
		w->x = (x > fgv.scr->width) ? fgv.scr->width : x;
		w->y = (y > fgv.scr->height) ? fgv.scr->height : y;
		XMoveWindow(fgv.disp, w->win, w->x, w->y);
		XFlush(fgv.disp);
	} else {
		D(("No move actually needed\n"));
	}
	return;
}

void winwidget_resize(winwidget w, int wide, int high)
{
	Window ignored_window;
	XWindowAttributes attributes;
	int tc_x, tc_y;
	int scr_width = fgv.scr->width;
	int scr_height = fgv.scr->height;

	XGetWindowAttributes(fgv.disp, w->win, &attributes);

#ifdef HAVE_LIBXINERAMA
	if (opt.flg.xinerama && fgv.xinerama_screens) {
		int i;
		fgv.xinerama_screen = 0;
		for (i = 0; i < fgv.num_xinerama_screens; i++) {
			if (XY_IN_RECT(attributes.x, attributes.y,
					   fgv.xinerama_screens[i].x_org,
					   fgv.xinerama_screens[i].y_org,
					   fgv.xinerama_screens[i].width,
					   fgv.xinerama_screens[i].height)) {
				fgv.xinerama_screen = i;
				break;
			}

		}
		if (getenv("XINERAMA_SCREEN"))
			fgv.xinerama_screen = atoi(getenv("XINERAMA_SCREEN"));

		scr_width  = fgv.xinerama_screens[fgv.xinerama_screen].width;
		scr_height = fgv.xinerama_screens[fgv.xinerama_screen].height;
	}
#endif


	D(("   x %d y %d w %d h %d\n",attributes.x,attributes.y,w->wide,w->high));

	if (opt.geom_flags & (WidthValue | HeightValue)) {
		w->had_resize = 1;
		return;
	}
	if (w && ((w->wide != wide) || (w->high != high))) {
		/* winwidget_clear_background(w); */
		if (opt.flg.screen_clip) {
			w->wide = (wide > scr_width) ? scr_width : wide;
			w->high = (high > scr_height) ? scr_height : high;
		}
		if (w->full_screen) {
			XTranslateCoordinates(fgv.disp, w->win, attributes.root,
					    -attributes.border_width - attributes.x,
					    -attributes.border_width - attributes.y,
					    &tc_x, &tc_y, &ignored_window);
			w->x = tc_x;
			w->y = tc_y;
			XMoveResizeWindow(fgv.disp, w->win, tc_x, tc_y, w->wide, w->high);
		} else
			XResizeWindow(fgv.disp, w->win, w->wide, w->high);

		w->had_resize = 1;
		XFlush(fgv.disp);

		D(("-> x %d y %d w %d h %d\n", w->x, w->y, w->wide,w->high));

	} else {
		D(("No resize actually needed\n"));
	}

	return;
}

void winwidget_hide(winwidget w)
{
	XUnmapWindow(fgv.disp, w->win);
	w->visible = 0;
	return;
}

static void winwidget_register(winwidget w)
{
	D(("window %p\n", w));
	fgv.window_num++;
	if (fgv.windows)
		fgv.windows = erealloc(fgv.windows, fgv.window_num * sizeof(winwidget));
	else
		fgv.windows = emalloc(fgv.window_num * sizeof(winwidget));
	fgv.windows[fgv.window_num - 1] = w;

	XSaveContext(fgv.disp, w->win, fgv.xid_context, (XPointer) w);
	return;
}

static void winwidget_unregister(winwidget w)
{
	int i, j;

	for (i = 0; i < fgv.window_num; i++) {
		if (fgv.windows[i] == w) {
			for (j = i; j < fgv.window_num - 1; j++)
				fgv.windows[j] = fgv.windows[j + 1];
			fgv.window_num--;
			if (fgv.window_num > 0)
				fgv.windows = erealloc(fgv.windows, fgv.window_num * sizeof(winwidget));
			else {
				free(fgv.windows);
				fgv.windows = NULL;
			}
		}
	}
	XDeleteContext(fgv.disp, w->win, fgv.xid_context);
	return;
}

winwidget winwidget_get_from_window(Window win){
		/* fgv.w was added when HRABAK changed the keypress logic to use
		 * function ptrs.  menus need that winwidget w too, so assign fgv.w
		 * here as the common spot to capture ALL events.
		 */
	winwidget ret = NULL;

	if (XFindContext(fgv.disp, win, fgv.xid_context, (XPointer *) & ret) != XCNOENT){
		fgv.w = ret;
		return(ret);
	}
	return(NULL);
}

void winwidget_free_image(winwidget w)
{
	if (w->im)
		gib_imlib_free_image_and_decache(w->im);
	w->im = NULL;
	w->im_w = 0;
	w->im_h = 0;
	return;
}

#ifdef DEBUG
void feh_debug_print_winwid(winwidget w)
{
	printf("winwid_debug:\n" "w = %p\n" "win = %ld\n" "w = %d\n"
	       "h = %d\n" "im_w = %d\n" "im_h = %d\n" "im_angle = %f\n"
	       "type = %d\n" "had_resize = %d\n" "im = %p\n" "GC = %p\n"
	       "pixmap = %ld\n" "name = %s\n" "node = %p\n" "opt.flg.state = %d\n"
	       "im_x = %d\n" "im_y = %d\n" "zoom = %f\n" "old_zoom = %f\n"
	       "click_offset_x = %d\n" "click_offset_y = %d\n"
	       "has_rotated = %d\n", (void *)w, w->win, w->wide, w->high, w->im_w,
	       w->im_h, w->im_angle, w->type, w->had_resize, w->im, (void *)w->gc,
	       w->bg_pmap, w->name, (void *)w->node, opt.flg.state, w->im_x, w->im_y,
	       w->zoom, w->old_zoom, w->click_offset_x, w->click_offset_y,
	       w->has_rotated);
}
#endif    /* DEBUG */

void winwidget_reset_image(winwidget w)
{
	w->zoom = 1.0;
	w->old_zoom = 1.0;
	w->im_x = 0;
	w->im_y = 0;
	w->im_angle = 0.0;
	w->has_rotated = 0;
	return;
}

void winwidget_center_image(winwidget w)
{
	int scr_width, scr_height;

	scr_width  = fgv.scr->width;
	scr_height = fgv.scr->height;

#ifdef HAVE_LIBXINERAMA
	if (opt.flg.xinerama && fgv.xinerama_screens) {
		scr_width  = fgv.xinerama_screens[fgv.xinerama_screen].width;
		scr_height = fgv.xinerama_screens[fgv.xinerama_screen].height;
	}
#endif				/* HAVE_LIBXINERAMA */

	if (w->full_screen) {
		w->im_x = (scr_width - lround(w->im_w * w->zoom)) >> 1;
		w->im_y = (scr_height - lround(w->im_h * w->zoom)) >> 1;
	} else {
		if (opt.geom_flags & WidthValue)
			w->im_x = ((int)opt.geom_w - lround(w->im_w * w->zoom)) >> 1;
		else
			w->im_x = 0;
		if (opt.geom_flags & HeightValue)
			w->im_y = ((int)opt.geom_h - lround(w->im_h * w->zoom)) >> 1;
		else
			w->im_y = 0;
	}
}

void winwidget_sanitise_offsets(winwidget w)
{
	int far_left, far_top;
	int min_x, max_x, max_y, min_y;

	far_left = w->wide - (w->im_w * w->zoom);
	far_top = w->high - (w->im_h * w->zoom);

	if ((w->im_w * w->zoom) > w->wide) {
		min_x = far_left;
		max_x = 0;
	} else {
		min_x = 0;
		max_x = far_left;
	}
	if ((w->im_h * w->zoom) > w->high) {
		min_y = far_top;
		max_y = 0;
	} else {
		min_y = 0;
		max_y = far_top;
	}
	if (w->im_x > max_x)
		w->im_x = max_x;
	if (w->im_x < min_x)
		w->im_x = min_x;
	if (w->im_y > max_y)
		w->im_y = max_y;
	if (w->im_y < min_y)
		w->im_y = min_y;

	return;
}

void winwidget_size_to_image(winwidget w)
{
	winwidget_resize(w, w->im_w * w->zoom, w->im_h * w->zoom);
	w->im_x = w->im_y = 0;
	winwidget_render_image(w, 0, 0, SANITIZE_NO);
	return;
}

void winwidget_set_pointer(winwidget w, int visible)
{
	Cursor no_ptr;
	XColor black, dummy;
	Pixmap bm_no;
	static char bm_no_data[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

	if (visible)
		XUndefineCursor(fgv.disp, w->win);
	else {
		bm_no = XCreateBitmapFromData(fgv.disp, w->win, bm_no_data, 8, 8);
		XAllocNamedColor(fgv.disp, DefaultColormapOfScreen(DefaultScreenOfDisplay(fgv.disp)), "black", &black, &dummy);

		no_ptr = XCreatePixmapCursor(fgv.disp, bm_no, bm_no, &black, &black, 0, 0);
		XDefineCursor(fgv.disp, w->win, no_ptr);
	}
}

int winwidget_get_width(winwidget w)
{
	int rect[4];
	winwidget_get_geometry(w, rect);
	return(rect[2]);
}

int winwidget_get_height(winwidget w)
{
	int rect[4];
	winwidget_get_geometry(w, rect);
	return(rect[3]);
}

void winwidget_get_geometry(winwidget w, int *rect)
{
	unsigned int bw, bp;
	Window child;
	if (!rect)
		return;

	XGetGeometry(fgv.disp, w->win, &fgv.root, &(rect[0]), &(rect[1]), (unsigned
				int *)&(rect[2]), (unsigned int *)&(rect[3]), &bw, &bp);

	XTranslateCoordinates(fgv.disp, w->win, fgv.root, 0, 0, &(rect[0]), &(rect[1]), &child);

	/* update the window geometry (in case it's inaccurate) */
	w->x    = rect[0];
	w->y    = rect[1];
	w->wide = rect[2];
	w->high = rect[3];
	return;
}

void winwidget_show_menu(winwidget w){
		/* this is the entrance for generating any/all menus.
		 * Set the fn_menu ONCE here and restore it during
		 * feh_menu_hide()
		 */

	int x, y, b;
	unsigned int c;
	Window r;

	/* future plan to show mov_md LL items as a menu list in mm */
	if (opt.flg.mode == MODE_MOVE )
			return;

	XQueryPointer(fgv.disp, w->win, &r, &r, &x, &y, &b, &b, &c);

	opt.fn_ptr = opt.flg.big_menus ? &opt.fn_fulls : &opt.fn_menu;

	if ( feh_menu_init_from_mnu_defs( MENU_MAIN, 1 ) == 1 )
			eprintf("%s menu load for sub_code %d !",ERR_FAILED, MENU_MAIN );
	feh_menu_show_at_xy(fgv.mnu.root, w, x, y);

}
