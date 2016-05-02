/* wallpaper.c

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
#include "filelist.h"
#include "options.h"
#include "wallpaper.h"
#include <limits.h>
#include <sys/stat.h>
Window ipc_win = None;
Window my_ipc_win = None;
Atom ipc_atom = None;
static unsigned char timeout = 0;

/*
 * This is a boolean indicating
 * That while we seem to see E16 IPC
 * it's actually E17 faking it
 * -- richlowe 2005-06-22
 */
static char e17_fake_ipc = 0;

void feh_wm_set_bg_filelist(unsigned char bgmode)
{
	if (filelist_len == 0)
		eprintf("No files specified for background setting");

	switch (bgmode) {
		case BG_MODE_TILE:
			feh_wm_set_bg(NULL, NULL, 0, 0, 0, 0, 1);
			break;
		case BG_MODE_SCALE:
			feh_wm_set_bg(NULL, NULL, 0, 1, 0, 0, 1);
			break;
		case BG_MODE_FILL:
			feh_wm_set_bg(NULL, NULL, 0, 0, 1, 0, 1);
			break;
		case BG_MODE_MAX:
			feh_wm_set_bg(NULL, NULL, 0, 0, 2, 0, 1);
			break;
		default:
			feh_wm_set_bg(NULL, NULL, 1, 0, 0, 0, 1);
			break;
	}
}

static void feh_wm_load_next(Imlib_Image *im)
{
	static gib_list *wpfile = NULL;

	if (wpfile == NULL)
		wpfile = filelist;

	if (feh_load_image(im, FEH_FILE(wpfile->data)) == 0)
		eprintf("Unable to load image %s", FEH_FILE(wpfile->data)->filename);
	if (wpfile->next)
		wpfile = wpfile->next;

	return;
}

static void feh_wm_set_bg_scaled(Pixmap pmap, Imlib_Image im, int use_filelist,
		int x, int y, int w, int h)
{
	if (use_filelist)
		feh_wm_load_next(&im);

	gib_imlib_render_image_on_drawable_at_size(pmap, im, x, y, w, h,
			1, 0, !opt.force_aliasing);

	if (use_filelist)
		gib_imlib_free_image_and_decache(im);

	return;
}

static void feh_wm_set_bg_centered(Pixmap pmap, Imlib_Image im, int use_filelist,
		int x, int y, int w, int h)
{
	int offset_x, offset_y;

	if (use_filelist)
		feh_wm_load_next(&im);

	if(opt.geom_flags & XValue)
		if(opt.geom_flags & XNegative)
			offset_x = (w - gib_imlib_image_get_width(im)) + opt.geom_x;
		else
			offset_x = opt.geom_x;
	else
		offset_x = (w - gib_imlib_image_get_width(im)) >> 1;

	if(opt.geom_flags & YValue)
		if(opt.geom_flags & YNegative)
			offset_y = (h - gib_imlib_image_get_height(im)) + opt.geom_y;
		else
			offset_y = opt.geom_y;
	else
		offset_y = (h - gib_imlib_image_get_height(im)) >> 1;

	gib_imlib_render_image_part_on_drawable_at_size(pmap, im,
		((offset_x < 0) ? -offset_x : 0),
		((offset_y < 0) ? -offset_y : 0),
		w,
		h,
		x + ((offset_x > 0) ? offset_x : 0),
		y + ((offset_y > 0) ? offset_y : 0),
		w,
		h,
		1, 0, 0);

	if (use_filelist)
		gib_imlib_free_image_and_decache(im);

	return;
}

static void feh_wm_set_bg_filled(Pixmap pmap, Imlib_Image im, int use_filelist,
		int x, int y, int w, int h)
{
	int img_w, img_h, cut_x;
	int render_w, render_h, render_x, render_y;

	if (use_filelist)
		feh_wm_load_next(&im);

	img_w = gib_imlib_image_get_width(im);
	img_h = gib_imlib_image_get_height(im);

	cut_x = (((img_w * h) > (img_h * w)) ? 1 : 0);

	render_w = (  cut_x ? ((img_h * w) / h) : img_w);
	render_h = ( !cut_x ? ((img_w * h) / w) : img_h);

	render_x = (  cut_x ? ((img_w - render_w) >> 1) : 0);
	render_y = ( !cut_x ? ((img_h - render_h) >> 1) : 0);

	gib_imlib_render_image_part_on_drawable_at_size(pmap, im,
		render_x, render_y,
		render_w, render_h,
		x, y, w, h,
		1, 0, !opt.force_aliasing);

	if (use_filelist)
		gib_imlib_free_image_and_decache(im);

	return;
}

static void feh_wm_set_bg_maxed(Pixmap pmap, Imlib_Image im, int use_filelist,
		int x, int y, int w, int h)
{
	int img_w, img_h, border_x;
	int render_w, render_h, render_x, render_y;
	int margin_x, margin_y;

	if (use_filelist)
		feh_wm_load_next(&im);

	img_w = gib_imlib_image_get_width(im);
	img_h = gib_imlib_image_get_height(im);

	border_x = (((img_w * h) > (img_h * w)) ? 0 : 1);

	render_w = (  border_x ? ((img_w * h) / img_h) : w);
	render_h = ( !border_x ? ((img_h * w) / img_w) : h);

	if(opt.geom_flags & XValue)
		if(opt.geom_flags & XNegative)
			margin_x = (w - render_w) + opt.geom_x;
		else
			margin_x = opt.geom_x;
	else
		margin_x = (w - render_w) >> 1;

	if(opt.geom_flags & YValue)
		if(opt.geom_flags & YNegative)
			margin_y = (h - render_h) + opt.geom_y;
		else
			margin_y = opt.geom_y;
	else
		margin_y = (h - render_h) >> 1;

	render_x = x + (  border_x ? margin_x : 0);
	render_y = y + ( !border_x ? margin_y : 0);

	gib_imlib_render_image_on_drawable_at_size(pmap, im,
		render_x, render_y,
		render_w, render_h,
		1, 0, !opt.force_aliasing);

	if (use_filelist)
		gib_imlib_free_image_and_decache(im);

	return;
}

void feh_wm_set_bg(char *fil, Imlib_Image im, int centered, int scaled,
		int filled, int desktop, int use_filelist)
{
	XGCValues gcvalues;
	XGCValues gcval;
	GC gc;
	char bgname[20];
	int num = (int) rand();
	char bgfil[4096];
	char sendbuf[4096];

	/*
	 * TODO this re-implements mkstemp (badly). However, it is only needed
	 * for non-file images and enlightenment. Might be easier to just remove
	 * it.
	 */

	snprintf(bgname, sizeof(bgname), "FEHBG_%d", num);

	if (!fil && im) {
		if (getenv("HOME") == NULL) {
			weprintf("Cannot save wallpaper to temporary file: You have no HOME");
			return;
		}
		snprintf(bgfil, sizeof(bgfil), "%s/.%s.png", getenv("HOME"), bgname);
		imlib_context_set_image(im);
		imlib_image_set_format("png");
		gib_imlib_save_image(im, bgfil);
		D(("bg saved as %s\n", bgfil));
		fil = bgfil;
	}

	if (feh_wm_get_wm_is_e() && (enl_ipc_get_win() != None)) {
		if (use_filelist) {
			feh_wm_load_next(&im);
			fil = FEH_FILE(filelist->data)->filename;
		}
		snprintf(sendbuf, sizeof(sendbuf), "background %s bg.file %s", bgname, fil);
		enl_ipc_send(sendbuf);

		if (scaled) {
			snprintf(sendbuf, sizeof(sendbuf), "background %s bg.solid 0 0 0", bgname);
			enl_ipc_send(sendbuf);
			snprintf(sendbuf, sizeof(sendbuf), "background %s bg.tile 0", bgname);
			enl_ipc_send(sendbuf);
			snprintf(sendbuf, sizeof(sendbuf), "background %s bg.xjust 512", bgname);
			enl_ipc_send(sendbuf);
			snprintf(sendbuf, sizeof(sendbuf), "background %s bg.yjust 512", bgname);
			enl_ipc_send(sendbuf);
			snprintf(sendbuf, sizeof(sendbuf), "background %s bg.xperc 1024", bgname);
			enl_ipc_send(sendbuf);
			snprintf(sendbuf, sizeof(sendbuf), "background %s bg.yperc 1024", bgname);
			enl_ipc_send(sendbuf);
		} else if (centered) {
			snprintf(sendbuf, sizeof(sendbuf), "background %s bg.solid 0 0 0", bgname);
			enl_ipc_send(sendbuf);
			snprintf(sendbuf, sizeof(sendbuf), "background %s bg.tile 0", bgname);
			enl_ipc_send(sendbuf);
			snprintf(sendbuf, sizeof(sendbuf), "background %s bg.xjust 512", bgname);
			enl_ipc_send(sendbuf);
			snprintf(sendbuf, sizeof(sendbuf), "background %s bg.yjust 512", bgname);
			enl_ipc_send(sendbuf);
		} else {
			snprintf(sendbuf, sizeof(sendbuf), "background %s bg.tile 1", bgname);
			enl_ipc_send(sendbuf);
		}

		snprintf(sendbuf, sizeof(sendbuf), "use_bg %s %d", bgname, desktop);
		enl_ipc_send(sendbuf);
		enl_ipc_sync();
	} else {
		Atom prop_root, prop_esetroot, type;
		int format, i;
		unsigned long length, after;
		unsigned char *data_root = NULL, *data_esetroot = NULL;
		Pixmap pmap_d1, pmap_d2;
		gib_list *l;

		/* string for sticking in ~/.fehbg */
		char *fehbg = NULL;
		char fehbg_args[512];
		fehbg_args[0] = '\0';
		char *home;
		char filbuf[4096];
		char *bgfill = NULL;
		bgfill = opt.image_bg == IMAGE_BG_WHITE ?  "--image-bg white" : "--image-bg black" ;

#ifdef HAVE_LIBXINERAMA
		if (opt.xinerama) {
			if (opt.xinerama_index >= 0) {
				snprintf(fehbg_args, sizeof(fehbg_args),
					"--xinerama-index %d", opt.xinerama_index);
			}
		}
		else
			snprintf(fehbg_args, sizeof(fehbg_args), "--no-xinerama");
#endif			/* HAVE_LIBXINERAMA */

		/* local display to set closedownmode on */
		Display *disp2;
		Window root2;
		int depth2;
		int in, out, w, h;

		D(("Falling back to XSetRootWindowPixmap\n"));

		/* Put the filename in filbuf between ' and escape ' in the filename */
		out = 0;

		if (fil && !use_filelist) {
			filbuf[out++] = '\'';

			fil = feh_absolute_path(fil);

			for (in = 0; fil[in] && out < 4092; in++) {

				if (fil[in] == '\'')
					filbuf[out++] = '\\';
				filbuf[out++] = fil[in];
			}
			filbuf[out++] = '\'';
			free(fil);

		} else {
			for (l = filelist; l && out < 4092; l = l->next) {
				filbuf[out++] = '\'';

				fil = feh_absolute_path(FEH_FILE(l->data)->filename);

				for (in = 0; fil[in] && out < 4092; in++) {

					if (fil[in] == '\'')
						filbuf[out++] = '\\';
					filbuf[out++] = fil[in];
				}
				filbuf[out++] = '\'';
				filbuf[out++] = ' ';
				free(fil);
			}
		}


		filbuf[out++] = 0;

		if (scaled) {
			pmap_d1 = XCreatePixmap(disp, root, scr->width, scr->height, depth);

#ifdef HAVE_LIBXINERAMA
			if (opt.xinerama_index >= 0) {
				if (opt.image_bg == IMAGE_BG_WHITE)
					gcval.foreground = WhitePixel(disp, DefaultScreen(disp));
				else
					gcval.foreground = BlackPixel(disp, DefaultScreen(disp));
				gc = XCreateGC(disp, root, GCForeground, &gcval);
				XFillRectangle(disp, pmap_d1, gc, 0, 0, scr->width, scr->height);
				XFreeGC(disp, gc);
			}

			if (opt.xinerama && xinerama_screens) {
				for (i = 0; i < num_xinerama_screens; i++) {
					if (opt.xinerama_index < 0 || opt.xinerama_index == i) {
						feh_wm_set_bg_scaled(pmap_d1, im, use_filelist,
							xinerama_screens[i].x_org, xinerama_screens[i].y_org,
							xinerama_screens[i].width, xinerama_screens[i].height);
					}
				}
			}
			else
#endif			/* HAVE_LIBXINERAMA */
				feh_wm_set_bg_scaled(pmap_d1, im, use_filelist,
					0, 0, scr->width, scr->height);
			fehbg = estrjoin(" ", "feh", fehbg_args, "--bg-scale", filbuf, NULL);
		} else if (centered) {

			D(("centering\n"));

			pmap_d1 = XCreatePixmap(disp, root, scr->width, scr->height, depth);
			if (opt.image_bg == IMAGE_BG_WHITE)
				gcval.foreground = WhitePixel(disp, DefaultScreen(disp));
			else
				gcval.foreground = BlackPixel(disp, DefaultScreen(disp));
			gc = XCreateGC(disp, root, GCForeground, &gcval);
			XFillRectangle(disp, pmap_d1, gc, 0, 0, scr->width, scr->height);

#ifdef HAVE_LIBXINERAMA
			if (opt.xinerama && xinerama_screens) {
				for (i = 0; i < num_xinerama_screens; i++) {
					if (opt.xinerama_index < 0 || opt.xinerama_index == i) {
						feh_wm_set_bg_centered(pmap_d1, im, use_filelist,
							xinerama_screens[i].x_org, xinerama_screens[i].y_org,
							xinerama_screens[i].width, xinerama_screens[i].height);
					}
				}
			}
			else
#endif				/* HAVE_LIBXINERAMA */
				feh_wm_set_bg_centered(pmap_d1, im, use_filelist,
					0, 0, scr->width, scr->height);

			XFreeGC(disp, gc);

			fehbg = estrjoin(" ", "feh", fehbg_args, bgfill, "--bg-center", filbuf, NULL);

		} else if (filled == 1) {

			pmap_d1 = XCreatePixmap(disp, root, scr->width, scr->height, depth);

#ifdef HAVE_LIBXINERAMA
			if (opt.xinerama_index >= 0) {
				if (opt.image_bg == IMAGE_BG_WHITE)
					gcval.foreground = WhitePixel(disp, DefaultScreen(disp));
				else
					gcval.foreground = BlackPixel(disp, DefaultScreen(disp));
				gc = XCreateGC(disp, root, GCForeground, &gcval);
				XFillRectangle(disp, pmap_d1, gc, 0, 0, scr->width, scr->height);
				XFreeGC(disp, gc);
			}

			if (opt.xinerama && xinerama_screens) {
				for (i = 0; i < num_xinerama_screens; i++) {
					if (opt.xinerama_index < 0 || opt.xinerama_index == i) {
						feh_wm_set_bg_filled(pmap_d1, im, use_filelist,
							xinerama_screens[i].x_org, xinerama_screens[i].y_org,
							xinerama_screens[i].width, xinerama_screens[i].height);
					}
				}
			}
			else
#endif				/* HAVE_LIBXINERAMA */
				feh_wm_set_bg_filled(pmap_d1, im, use_filelist
					, 0, 0, scr->width, scr->height);

			fehbg = estrjoin(" ", "feh", fehbg_args, "--bg-fill", filbuf, NULL);

		} else if (filled == 2) {

			pmap_d1 = XCreatePixmap(disp, root, scr->width, scr->height, depth);
			if (opt.image_bg == IMAGE_BG_WHITE)
				gcval.foreground = WhitePixel(disp, DefaultScreen(disp));
			else
				gcval.foreground = BlackPixel(disp, DefaultScreen(disp));
			gc = XCreateGC(disp, root, GCForeground, &gcval);
			XFillRectangle(disp, pmap_d1, gc, 0, 0, scr->width, scr->height);

#ifdef HAVE_LIBXINERAMA
			if (opt.xinerama && xinerama_screens) {
				for (i = 0; i < num_xinerama_screens; i++) {
					if (opt.xinerama_index < 0 || opt.xinerama_index == i) {
						feh_wm_set_bg_maxed(pmap_d1, im, use_filelist,
							xinerama_screens[i].x_org, xinerama_screens[i].y_org,
							xinerama_screens[i].width, xinerama_screens[i].height);
					}
				}
			}
			else
#endif				/* HAVE_LIBXINERAMA */
				feh_wm_set_bg_maxed(pmap_d1, im, use_filelist,
					0, 0, scr->width, scr->height);

			XFreeGC(disp, gc);

			fehbg = estrjoin(" ", "feh", fehbg_args, bgfill, "--bg-max", filbuf, NULL);

		} else {
			if (use_filelist)
				feh_wm_load_next(&im);
			w = gib_imlib_image_get_width(im);
			h = gib_imlib_image_get_height(im);
			pmap_d1 = XCreatePixmap(disp, root, w, h, depth);
			gib_imlib_render_image_on_drawable(pmap_d1, im, 0, 0, 1, 0, 0);
			fehbg = estrjoin(" ", "feh --bg-tile", filbuf, NULL);
		}

		if (fehbg && !opt.no_fehbg) {
			home = getenv("HOME");
			if (home) {
				FILE *fp;
				char *path;
				struct stat s;
				path = estrjoin("/", home, ".fehbg", NULL);
				if ((fp = fopen(path, "w")) == NULL) {
					weprintf("Can't write to %s", path);
				} else {
					fprintf(fp, "#!/bin/sh\n%s\n", fehbg);
					fclose(fp);
					stat(path, &s);
					if (chmod(path, s.st_mode | S_IXUSR | S_IXGRP) != 0) {
						weprintf("Can't set %s as executable", path);
					}
				}
				free(path);
			}
		}
		
		free(fehbg);

		/* create new display, copy pixmap to new display */
		disp2 = XOpenDisplay(NULL);
		if (!disp2)
			eprintf("Can't reopen X display.");
		root2 = RootWindow(disp2, DefaultScreen(disp2));
		depth2 = DefaultDepth(disp2, DefaultScreen(disp2));
		XSync(disp, False);
		pmap_d2 = XCreatePixmap(disp2, root2, scr->width, scr->height, depth2);
		gcvalues.fill_style = FillTiled;
		gcvalues.tile = pmap_d1;
		gc = XCreateGC(disp2, pmap_d2, GCFillStyle | GCTile, &gcvalues);
		XFillRectangle(disp2, pmap_d2, gc, 0, 0, scr->width, scr->height);
		XFreeGC(disp2, gc);
		XSync(disp2, False);
		XSync(disp, False);
		XFreePixmap(disp, pmap_d1);

		prop_root = XInternAtom(disp2, "_XROOTPMAP_ID", True);
		prop_esetroot = XInternAtom(disp2, "ESETROOT_PMAP_ID", True);

		if (prop_root != None && prop_esetroot != None) {
			XGetWindowProperty(disp2, root2, prop_root, 0L, 1L,
					   False, AnyPropertyType, &type, &format, &length, &after, &data_root);
			if (type == XA_PIXMAP) {
				XGetWindowProperty(disp2, root2,
						   prop_esetroot, 0L, 1L,
						   False, AnyPropertyType,
						   &type, &format, &length, &after, &data_esetroot);
				if (data_root && data_esetroot) {
					if (type == XA_PIXMAP && *((Pixmap *) data_root) == *((Pixmap *) data_esetroot)) {
						XKillClient(disp2, *((Pixmap *)
								     data_root));
					}
				}
			}
		}

		if (data_root)
			XFree(data_root);
	
		if (data_esetroot)
			XFree(data_esetroot);

		/* This will locate the property, creating it if it doesn't exist */
		prop_root = XInternAtom(disp2, "_XROOTPMAP_ID", False);
		prop_esetroot = XInternAtom(disp2, "ESETROOT_PMAP_ID", False);

		if (prop_root == None || prop_esetroot == None)
			eprintf("creation of pixmap property failed.");

		XChangeProperty(disp2, root2, prop_root, XA_PIXMAP, 32, PropModeReplace, (unsigned char *) &pmap_d2, 1);
		XChangeProperty(disp2, root2, prop_esetroot, XA_PIXMAP, 32,
				PropModeReplace, (unsigned char *) &pmap_d2, 1);

		XSetWindowBackgroundPixmap(disp2, root2, pmap_d2);
		XClearWindow(disp2, root2);
		XFlush(disp2);
		XSetCloseDownMode(disp2, RetainPermanent);
		XCloseDisplay(disp2);
	}
	return;
}

signed char feh_wm_get_wm_is_e(void)
{
	static signed char e = -1;

	/* check if E is actually running */
	if (e == -1) {
		/* XXX: This only covers E17 prior to 6/22/05 */
		if ((XInternAtom(disp, "ENLIGHTENMENT_COMMS", True) != None)
		    && (XInternAtom(disp, "ENLIGHTENMENT_VERSION", True) != None)) {
			D(("Enlightenment detected.\n"));
			e = 1;
		} else {
			D(("Enlightenment not detected.\n"));
			e = 0;
		}
	}
	return(e);
}

int feh_wm_get_num_desks(void)
{
	char *buf, *ptr;
	int desks;

	if (!feh_wm_get_wm_is_e())
		return(-1);

	buf = enl_send_and_wait("num_desks ?");
	if (buf == IPC_FAKE)	/* Fake E17 IPC */
		return(-1);
	D(("Got from E IPC: %s\n", buf));
	ptr = buf;
	while (ptr && !isdigit(*ptr))
		ptr++;
	desks = atoi(ptr);

	return(desks);
}

Window enl_ipc_get_win(void)
{

	unsigned char *str = NULL;
	Atom prop, prop2, ever;
	unsigned long num, after;
	int format;
	Window dummy_win;
	int dummy_int;
	unsigned int dummy_uint;

	D(("Searching for IPC window.\n"));

	/*
	 * Shortcircuit this entire func
	 * if we already know it's an e17 fake
	 */
	if (e17_fake_ipc)
		return(ipc_win);

		    prop = XInternAtom(disp, "ENLIGHTENMENT_COMMS", True);
	if (prop == None) {
		D(("Enlightenment is not running.\n"));
		return(None);
	} else {
		/* XXX: This will only work with E17 prior to 6/22/2005 */
		ever = XInternAtom(disp, "ENLIGHTENMENT_VERSION", True);
		if (ever == None) {
			/* This is an E without ENLIGHTENMENT_VERSION */
			D(("E16 IPC Protocol not supported"));
			return(None);
		}
	}
	XGetWindowProperty(disp, root, prop, 0, 14, False, AnyPropertyType, &prop2, &format, &num, &after, &str);
	if (str) {
		sscanf((char *) str, "%*s %x", (unsigned int *) &ipc_win);
		XFree(str);
	}
	if (ipc_win != None) {
		if (!XGetGeometry
		    (disp, ipc_win, &dummy_win, &dummy_int, &dummy_int,
		     &dummy_uint, &dummy_uint, &dummy_uint, &dummy_uint)) {
			D((" -> IPC Window property is valid, but the window doesn't exist.\n"));
			ipc_win = None;
		}
		str = NULL;
		if (ipc_win != None) {
			XGetWindowProperty(disp, ipc_win, prop, 0, 14,
					   False, AnyPropertyType, &prop2, &format, &num, &after, &str);
			if (str) {
				XFree(str);
			} else {
				D((" -> IPC Window lacks the proper atom.  I can't talk to fake IPC windows....\n"));
				ipc_win = None;
			}
		}
	}
	if (ipc_win != None) {

		XGetWindowProperty(disp, ipc_win, ever, 0, 14, False,
				   AnyPropertyType, &prop2, &format, &num, &after, &str);
		if (str) {
			/*
			 * This is E17's way of telling us it's only pretending
			 * as a workaround for a bug related to the way java handles
			 * Window Managers.
			 * (Only valid after date of this comment)
			 * -- richlowe 2005-06-22
			 */
			XFree(str);
			D((" -> Found a fake E17 IPC window, ignoring"));
			ipc_win = None;
			e17_fake_ipc = 1;
			return(ipc_win);
		}

		D((" -> IPC Window found and verified as 0x%08x.  Registering feh as an IPC client.\n", (int) ipc_win));
		XSelectInput(disp, ipc_win, StructureNotifyMask | SubstructureNotifyMask);
		enl_ipc_send("set clientname " PACKAGE);
		enl_ipc_send("set version " VERSION);
		enl_ipc_send("set email tom@linuxbrit.co.uk");
		enl_ipc_send("set web http://www.linuxbrit.co.uk");
		enl_ipc_send("set info Feh - be pr0n or be dead");
	}
	if (my_ipc_win == None) {
		my_ipc_win = XCreateSimpleWindow(disp, root, -2, -2, 1, 1, 0, 0, 0);
	}
	return(ipc_win);
}

void enl_ipc_send(char *str)
{

	static char *last_msg = NULL;
	char buff[21];
	register unsigned short i;
	register unsigned char j;
	unsigned short len;
	XEvent ev;

	if (str == NULL) {
		if (last_msg == NULL)
			eprintf("eeek");
		str = last_msg;
		D(("Resending last message \"%s\" to Enlightenment.\n", str));
	} else {
		if (last_msg != NULL) {
			free(last_msg);
		}
		last_msg = estrdup(str);
		D(("Sending \"%s\" to Enlightenment.\n", str));
	}
	if (ipc_win == None) {
		if ((ipc_win = enl_ipc_get_win()) == None) {
			D(("Hrm. Enlightenment doesn't seem to be running. No IPC window, no IPC.\n"));
			return;
		}
	}
	len = strlen(str);
	ipc_atom = XInternAtom(disp, "ENL_MSG", False);
	if (ipc_atom == None) {
		D(("IPC error:  Unable to find/create ENL_MSG atom.\n"));
		return;
	}
	for (; XCheckTypedWindowEvent(disp, my_ipc_win, ClientMessage, &ev););	/* Discard any out-of-sync messages */
	ev.xclient.type = ClientMessage;
	ev.xclient.serial = 0;
	ev.xclient.send_event = True;
	ev.xclient.window = ipc_win;
	ev.xclient.message_type = ipc_atom;
	ev.xclient.format = 8;

	for (i = 0; i < len + 1; i += 12) {
		sprintf(buff, "%8x", (int) my_ipc_win);
		for (j = 0; j < 12; j++) {
			buff[8 + j] = str[i + j];
			if (!str[i + j]) {
				break;
			}
		}
		buff[20] = 0;
		for (j = 0; j < 20; j++) {
			ev.xclient.data.b[j] = buff[j];
		}
		XSendEvent(disp, ipc_win, False, 0, (XEvent *) & ev);
	}
	return;
}

static sighandler_t *enl_ipc_timeout(int sig)
{
	timeout = 1;
	return((sighandler_t *) sig);
}

char *enl_wait_for_reply(void)
{

	XEvent ev;
	static char msg_buffer[20];
	register unsigned char i;

	alarm(2);
	for (; !XCheckTypedWindowEvent(disp, my_ipc_win, ClientMessage, &ev)
	     && !timeout;);
	alarm(0);
	if (ev.xany.type != ClientMessage) {
		return(IPC_TIMEOUT);
	}
	for (i = 0; i < 20; i++) {
		msg_buffer[i] = ev.xclient.data.b[i];
	}
	return(msg_buffer + 8);
}

char *enl_ipc_get(const char *msg_data)
{

	static char *message = NULL;
	static unsigned short len = 0;
	char buff[13], *ret_msg = NULL;
	register unsigned char i;
	unsigned char blen;

	if (msg_data == IPC_TIMEOUT) {
		return(IPC_TIMEOUT);
	}
	for (i = 0; i < 12; i++) {
		buff[i] = msg_data[i];
	}
	buff[12] = 0;
	blen = strlen(buff);
	if (message != NULL) {
		len += blen;
		message = (char *) erealloc(message, len + 1);
		strcat(message, buff);
	} else {
		len = blen;
		message = (char *) emalloc(len + 1);
		strcpy(message, buff);
	}
	if (blen < 12) {
		ret_msg = message;
		message = NULL;
		D(("Received complete reply:  \"%s\"\n", ret_msg));
	}
	return(ret_msg);
}

char *enl_send_and_wait(char *msg)
{
	char *reply = IPC_TIMEOUT;
	sighandler_t old_alrm;

	/*
	 * Shortcut this func and return IPC_FAKE
	 * If the IPC Window is the E17 fake
	 */
	if (e17_fake_ipc)
		return IPC_FAKE;

	if (ipc_win == None) {
		/* The IPC window is missing.  Wait for it to return or feh to be killed. */
		/* Only called once in the E17 case */
		for (; enl_ipc_get_win() == None;) {
			if (e17_fake_ipc)
				return IPC_FAKE;
			else
				sleep(1);
		}
	}
	old_alrm = (sighandler_t) signal(SIGALRM, (sighandler_t) enl_ipc_timeout);
	for (; reply == IPC_TIMEOUT;) {
		timeout = 0;
		enl_ipc_send(msg);
		for (; !(reply = enl_ipc_get(enl_wait_for_reply())););
		if (reply == IPC_TIMEOUT) {
			/* We timed out.  The IPC window must be AWOL.  Reset and resend message. */
			D(("IPC timed out.  IPC window has gone. Clearing ipc_win.\n"));
			XSelectInput(disp, ipc_win, None);
			ipc_win = None;
		}
	}
	signal(SIGALRM, old_alrm);
	return(reply);
}
