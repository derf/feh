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
#include "options.h"
#include "wallpaper.h"
#include <limits.h>
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

static void feh_wm_load_next(Imlib_Image *im)
{
	static feh_node *node = NULL;

	if (node == NULL)
		node = feh_md->rn->next;

	if (feh_load_image(im, NODE_DATA(node)) == 0)
		eprintf("Unable to load image %s", NODE_FILENAME(node) );
	if (node->next != feh_md->rn )
		node = node->next;

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

	offset_x = (w - gib_imlib_image_get_width(im)) >> 1;
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

	if (use_filelist)
		feh_wm_load_next(&im);

	img_w = gib_imlib_image_get_width(im);
	img_h = gib_imlib_image_get_height(im);

	border_x = (((img_w * h) > (img_h * w)) ? 0 : 1);

	render_w = (  border_x ? ((img_w * h) / img_h) : w);
	render_h = ( !border_x ? ((img_h * w) / img_w) : h);

	render_x = x + (  border_x ? ((w - render_w) >> 1) : 0);
	render_y = y + ( !border_x ? ((h - render_h) >> 1) : 0);

	gib_imlib_render_image_on_drawable_at_size(pmap, im,
		render_x, render_y,
		render_w, render_h,
		1, 0, !opt.force_aliasing);

	if (use_filelist)
		gib_imlib_free_image_and_decache(im);

	return;
}

void feh_wm_set_bg(char *fil, Imlib_Image im, enum mnu_lvl bgmode,
		               int desktop, int use_filelist)
{
	char bgname[20];
	int num = (int) rand();
	char bgfil[4096];
	char sendbuf[4096];
	int centered, scaled,filled;

	if ( bgmode == CB_BG_TILED )
		 ( centered = scaled = filled = 0);
	else if ( bgmode == CB_BG_SCALED )
		 ( centered = 0, scaled = 1, filled = 0);
	else if ( bgmode == CB_BG_FILLED )
		 ( centered = scaled = 0, filled = 1);
	else if ( bgmode == CB_BG_MAX )
		 ( centered = scaled = 0, filled = 2);
	else    /* CB_BG_CENTERED is the default */
		 ( centered = 1, scaled = filled = 0);

	snprintf(bgname, sizeof(bgname), "FEHBG_%d", num);

	if (!fil && im) {
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
			fil = NODE_FILENAME(feh_md->cn);
		}
		snprintf(sendbuf, sizeof(sendbuf), "%s %s bg.file %s", ERR_BACKGROUND,bgname, fil);
		enl_ipc_send(sendbuf);

		if (scaled) {
			snprintf(sendbuf, sizeof(sendbuf), "%s %s bg.solid 0 0 0", ERR_BACKGROUND, bgname);
			enl_ipc_send(sendbuf);
			snprintf(sendbuf, sizeof(sendbuf), "%s %s bg.tile 0",ERR_BACKGROUND, bgname);
			enl_ipc_send(sendbuf);
			snprintf(sendbuf, sizeof(sendbuf), "%s %s bg.xjust 512",ERR_BACKGROUND, bgname);
			enl_ipc_send(sendbuf);
			snprintf(sendbuf, sizeof(sendbuf), "%s %s bg.yjust 512",ERR_BACKGROUND, bgname);
			enl_ipc_send(sendbuf);
			snprintf(sendbuf, sizeof(sendbuf), "%s %s bg.xperc 1024",ERR_BACKGROUND, bgname);
			enl_ipc_send(sendbuf);
			snprintf(sendbuf, sizeof(sendbuf), "%s %s bg.yperc 1024",ERR_BACKGROUND, bgname);
			enl_ipc_send(sendbuf);
		} else if (centered) {
			snprintf(sendbuf, sizeof(sendbuf), "%s %s bg.solid 0 0 0", ERR_BACKGROUND, bgname);
			enl_ipc_send(sendbuf);
			snprintf(sendbuf, sizeof(sendbuf), "%s%s bg.tile 0",ERR_BACKGROUND, bgname);
			enl_ipc_send(sendbuf);
			snprintf(sendbuf, sizeof(sendbuf), "%s %s bg.xjust 512", ERR_BACKGROUND, bgname);
			enl_ipc_send(sendbuf);
			snprintf(sendbuf, sizeof(sendbuf), "%s %s bg.yjust 512",ERR_BACKGROUND, bgname);
			enl_ipc_send(sendbuf);
		} else {
			snprintf(sendbuf, sizeof(sendbuf), "%s %s bg.tile 1", ERR_BACKGROUND, bgname);
			enl_ipc_send(sendbuf);
		}

		snprintf(sendbuf, sizeof(sendbuf), "use_bg %s %d", bgname, desktop);
		enl_ipc_send(sendbuf);
		enl_ipc_sync();
	} else {
		Atom prop_root, prop_esetroot, type;
		int format, i;
		unsigned long length, after;
		unsigned char *data_root, *data_esetroot;
		Pixmap pmap_d1, pmap_d2;

		/* string for sticking in ~/.fehbg */
		char *fehbg = mobs(3);
		char *home;
		char filbuf[4096];
		char fehbg_xinerama[] = "--no-xinerama";

		/* local display to set closedownmode on */
		Display *disp2;
		Window root2;
		int depth2;
		XGCValues gcvalues;
		GC gc;
		int in, out, w, h;

		if (opt.flg.xinerama)
			fehbg_xinerama[0] = '\0';

		D(("Falling back to XSetRootWindowPixmap\n"));

		/* Put the filename in filbuf between ' and escape ' in the filename */
		out = 0;

		if (fil && !use_filelist) {
			filbuf[out++] = '\'';

			for (in = 0; fil[in] && out < 4092; in++) {

				if (fil[in] == '\'')
					filbuf[out++] = '\\';
				filbuf[out++] = fil[in];
			}
			filbuf[out++] = '\'';

		} else {
      for (feh_md->cn = feh_md->rn->next ;
                       (feh_md->cn != feh_md->rn) && out < 4092;
                        feh_md->cn = feh_md->cn->next) {
				filbuf[out++] = '\'';

				fil = NODE_FILENAME(feh_md->cn);

				for (in = 0; fil[in] && out < 4092; in++) {

					if (fil[in] == '\'')
						filbuf[out++] = '\\';
					filbuf[out++] = fil[in];
				}
				filbuf[out++] = '\'';
				filbuf[out++] = ' ';
			}
		}


		filbuf[out++] = 0;

		if (scaled) {
			pmap_d1 = XCreatePixmap(fgv.disp, fgv.root, fgv.scr->width, fgv.scr->height, fgv.depth);

#ifdef HAVE_LIBXINERAMA
			if (opt.flg.xinerama && fgv.xinerama_screens)
				for (i = 0; i < fgv.num_xinerama_screens; i++)
					feh_wm_set_bg_scaled(pmap_d1, im, use_filelist,
						fgv.xinerama_screens[i].x_org, fgv.xinerama_screens[i].y_org,
						fgv.xinerama_screens[i].width, fgv.xinerama_screens[i].height);
			else
#endif			/* HAVE_LIBXINERAMA */
				feh_wm_set_bg_scaled(pmap_d1, im, use_filelist,
                              0, 0, fgv.scr->width, fgv.scr->height);
			STRCAT_4ITEMS(fehbg,"feh ", fehbg_xinerama, " --bg-scale ", filbuf);
		} else if (centered) {
			XGCValues gcval;
			GC gc;

			D(("centering\n"));

			pmap_d1 = XCreatePixmap(fgv.disp, fgv.root, fgv.scr->width, fgv.scr->height, fgv.depth);
			gcval.foreground = BlackPixel(fgv.disp, DefaultScreen(fgv.disp));
			gc = XCreateGC(fgv.disp, fgv.root, GCForeground, &gcval);
			XFillRectangle(fgv.disp, pmap_d1, gc, 0, 0, fgv.scr->width, fgv.scr->height);

#ifdef HAVE_LIBXINERAMA
			if (opt.flg.xinerama && fgv.xinerama_screens)
				for (i = 0; i < fgv.num_xinerama_screens; i++)
					feh_wm_set_bg_centered(pmap_d1, im, use_filelist,
						fgv.xinerama_screens[i].x_org, fgv.xinerama_screens[i].y_org,
						fgv.xinerama_screens[i].width, fgv.xinerama_screens[i].height);
			else
#endif				/* HAVE_LIBXINERAMA */
				feh_wm_set_bg_centered(pmap_d1, im, use_filelist,
					0, 0, fgv.scr->width, fgv.scr->height);

			XFreeGC(fgv.disp, gc);

			STRCAT_4ITEMS(fehbg,"feh ", fehbg_xinerama, " --bg-center ", filbuf);

		} else if (filled == 1) {

			pmap_d1 = XCreatePixmap(fgv.disp, fgv.root, fgv.scr->width, fgv.scr->height, fgv.depth);

#ifdef HAVE_LIBXINERAMA
			if (opt.flg.xinerama && fgv.xinerama_screens)
				for (i = 0; i < fgv.num_xinerama_screens; i++)
					feh_wm_set_bg_filled(pmap_d1, im, use_filelist,
						fgv.xinerama_screens[i].x_org, fgv.xinerama_screens[i].y_org,
						fgv.xinerama_screens[i].width, fgv.xinerama_screens[i].height);
			else
#endif				/* HAVE_LIBXINERAMA */
				feh_wm_set_bg_filled(pmap_d1, im, use_filelist
					, 0, 0, fgv.scr->width, fgv.scr->height);

			STRCAT_4ITEMS(fehbg,"feh ", fehbg_xinerama, " --bg-fill ", filbuf);

		} else if (filled == 2) {
			XGCValues gcval;

			pmap_d1 = XCreatePixmap(fgv.disp, fgv.root, fgv.scr->width, fgv.scr->height, fgv.depth);
			gcval.foreground = BlackPixel(fgv.disp, DefaultScreen(fgv.disp));
			gc = XCreateGC(fgv.disp, fgv.root, GCForeground, &gcval);
			XFillRectangle(fgv.disp, pmap_d1, gc, 0, 0, fgv.scr->width, fgv.scr->height);

#ifdef HAVE_LIBXINERAMA
			if (opt.flg.xinerama && fgv.xinerama_screens)
				for (i = 0; i < fgv.num_xinerama_screens; i++)
					feh_wm_set_bg_maxed(pmap_d1, im, use_filelist,
						fgv.xinerama_screens[i].x_org, fgv.xinerama_screens[i].y_org,
						fgv.xinerama_screens[i].width, fgv.xinerama_screens[i].height);
			else
#endif				/* HAVE_LIBXINERAMA */
				feh_wm_set_bg_maxed(pmap_d1, im, use_filelist,
					0, 0, fgv.scr->width, fgv.scr->height);

			XFreeGC(fgv.disp, gc);

			STRCAT_4ITEMS(fehbg,"feh ", fehbg_xinerama, " --bg-max ", filbuf);

		} else {
			if (use_filelist)
				feh_wm_load_next(&im);
			w = gib_imlib_image_get_width(im);
			h = gib_imlib_image_get_height(im);
			pmap_d1 = XCreatePixmap(fgv.disp, fgv.root, w, h, fgv.depth);
			gib_imlib_render_image_on_drawable(pmap_d1, im, 0, 0, 1, 0, 0);
			STRCAT_2ITEMS(fehbg,"feh --bg-tile ", filbuf);
		}

		if (fehbg[0] && !opt.flg.no_fehbg) {
			home = getenv("HOME");
			if (home) {
				FILE *fp;
				char *path = mobs(2);
				STRCAT_2ITEMS(path, home, "/.fehbg");
				if ((fp = fopen(path, "w")) == NULL) {
					weprintf("%swrite to %s",ERR_CANNOT, path);
				} else {
					fprintf(fp, "%s\n", fehbg);
					fclose(fp);
				}
			}
		}

		/* create new display, copy pixmap to new display */
		disp2 = XOpenDisplay(NULL);
		if (!disp2)
			eprintf("%sreopen X display.", ERR_CANNOT);
		root2 = RootWindow(disp2, DefaultScreen(disp2));
		depth2 = DefaultDepth(disp2, DefaultScreen(disp2));
		XSync(fgv.disp, False);
		pmap_d2 = XCreatePixmap(disp2, root2, fgv.scr->width, fgv.scr->height, depth2);
		gcvalues.fill_style = FillTiled;
		gcvalues.tile = pmap_d1;
		gc = XCreateGC(disp2, pmap_d2, GCFillStyle | GCTile, &gcvalues);
		XFillRectangle(disp2, pmap_d2, gc, 0, 0, fgv.scr->width, fgv.scr->height);
		XFreeGC(disp2, gc);
		XSync(disp2, False);
		XSync(fgv.disp, False);
		XFreePixmap(fgv.disp, pmap_d1);

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
		/* This will locate the property, creating it if it doesn't exist */
		prop_root = XInternAtom(disp2, "_XROOTPMAP_ID", False);
		prop_esetroot = XInternAtom(disp2, "ESETROOT_PMAP_ID", False);

		if (prop_root == None || prop_esetroot == None)
			eprintf("creation of pixmap property %s.",ERR_FAILED);

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
		if ((XInternAtom(fgv.disp, "ENLIGHTENMENT_COMMS", True) != None)
		    && (XInternAtom(fgv.disp, "ENLIGHTENMENT_VERSION", True) != None)) {
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

	prop = XInternAtom(fgv.disp, "ENLIGHTENMENT_COMMS", True);
	if (prop == None) {
		D(("Enlightenment is not running.\n"));
		return(None);
	} else {
		/* XXX: This will only work with E17 prior to 6/22/2005 */
		ever = XInternAtom(fgv.disp, "ENLIGHTENMENT_VERSION", True);
		if (ever == None) {
			/* This is an E without ENLIGHTENMENT_VERSION */
			D(("E16 IPC Protocol not supported"));
			return(None);
		}
	}
	XGetWindowProperty(fgv.disp, fgv.root, prop, 0, 14, False, AnyPropertyType, &prop2, &format, &num, &after, &str);
	if (str) {
		sscanf((char *) str, "%*s %x", (unsigned int *) &ipc_win);
		XFree(str);
	}
	if (ipc_win != None) {
		if (!XGetGeometry
		    (fgv.disp, ipc_win, &dummy_win, &dummy_int, &dummy_int,
		     &dummy_uint, &dummy_uint, &dummy_uint, &dummy_uint)) {
			D((" -> IPC Window property is valid, but the window doesn't exist.\n"));
			ipc_win = None;
		}
		str = NULL;
		if (ipc_win != None) {
			XGetWindowProperty(fgv.disp, ipc_win, prop, 0, 14,
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

		XGetWindowProperty(fgv.disp, ipc_win, ever, 0, 14, False,
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
		XSelectInput(fgv.disp, ipc_win, StructureNotifyMask | SubstructureNotifyMask);
		enl_ipc_send("set clientname " PACKAGE);
		enl_ipc_send("set version " VERSION);
		enl_ipc_send("set email tom@linuxbrit.co.uk");
		enl_ipc_send("set web http://www.linuxbrit.co.uk");
		enl_ipc_send("set info Feh - be pr0n or be dead");
	}
	if (my_ipc_win == None) {
		my_ipc_win = XCreateSimpleWindow(fgv.disp, fgv.root, -2, -2, 1, 1, 0, 0, 0);
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
	ipc_atom = XInternAtom(fgv.disp, "ENL_MSG", False);
	if (ipc_atom == None) {
		D(("IPC error:  Unable to find/create ENL_MSG atom.\n"));
		return;
	}
	for (; XCheckTypedWindowEvent(fgv.disp, my_ipc_win, ClientMessage, &ev););	/* Discard any out-of-sync messages */
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
		XSendEvent(fgv.disp, ipc_win, False, 0, (XEvent *) & ev);
	}
	return;
}

static sighandler_t *enl_ipc_timeout(int sig)
{
	timeout = 1;
	return((sighandler_t *) sig);
	sig = 0;
}

char *enl_wait_for_reply(void)
{

	XEvent ev;
	static char msg_buffer[20];
	register unsigned char i;

	alarm(2);
	for (; !XCheckTypedWindowEvent(fgv.disp, my_ipc_win, ClientMessage, &ev)
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
			XSelectInput(fgv.disp, ipc_win, None);
			ipc_win = None;
		}
	}
	signal(SIGALRM, old_alrm);
	return(reply);
}
