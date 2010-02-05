/* winwidget.c

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

#include "feh.h"
#include "filelist.h"
#include "winwidget.h"
#include "options.h"

static void winwidget_unregister(winwidget win);
static void winwidget_register(winwidget win);
static winwidget winwidget_allocate(void);

static char bm_no_data[] = { 0,0,0,0, 0,0,0,0 };
int window_num = 0;             /* For window list */
winwidget *windows = NULL;      /* List of windows to loop though */

static winwidget
winwidget_allocate(void)
{
  winwidget ret = NULL;

  D_ENTER(4);
  ret = emalloc(sizeof(_winwidget));

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
  ret->type = WIN_TYPE_UNSET;
  ret->visible = 0;
  ret->caption_entry = 0;

  /* Zoom stuff */
  ret->mode = MODE_NORMAL;

  ret->gc = None;

  /* New stuff */
  ret->im_x = 0;
  ret->im_y = 0;
  ret->zoom = 1.0;

  ret->click_offset_x = 0;
  ret->click_offset_y = 0;
  ret->im_click_offset_x = 0;
  ret->im_click_offset_y = 0;
  ret->has_rotated = 0;

  D_RETURN(4, ret);
}

winwidget
winwidget_create_from_image(Imlib_Image im,
                            char *name,
                            char type)
{
  winwidget ret = NULL;

  D_ENTER(4);

  if (im == NULL)
    D_RETURN(4, NULL);

  ret = winwidget_allocate();
  ret->type = type;

  ret->im = im;
  ret->w = ret->im_w = gib_imlib_image_get_width(ret->im);
  ret->h = ret->im_h = gib_imlib_image_get_height(ret->im);

  if (name)
    ret->name = estrdup(name);
  else
    ret->name = estrdup(PACKAGE);

  if (opt.full_screen)
    ret->full_screen = True;
  winwidget_create_window(ret, ret->w, ret->h);
  winwidget_render_image(ret, 1, 1);

  D_RETURN(4, ret);
}

winwidget
winwidget_create_from_file(gib_list * list,
                           char *name,
                           char type)
{
  winwidget ret = NULL;
  feh_file *file = FEH_FILE(list->data);

  D_ENTER(4);

  if (!file || !file->filename)
    D_RETURN(4, NULL);

  ret = winwidget_allocate();
  ret->file = list;
  ret->type = type;
  if (name)
    ret->name = estrdup(name);
  else
    ret->name = estrdup(file->filename);

  if (winwidget_loadimage(ret, file) == 0) {
    winwidget_destroy(ret);
    D_RETURN(4, NULL);
  }

  if (!ret->win) {
    ret->w = ret->im_w = gib_imlib_image_get_width(ret->im);
    ret->h = ret->im_h = gib_imlib_image_get_height(ret->im);
    D(3,
      ("image is %dx%d pixels, format %s\n", ret->w, ret->h,
       gib_imlib_image_format(ret->im)));
    if (opt.full_screen)
      ret->full_screen = True;
    winwidget_create_window(ret, ret->w, ret->h);
    winwidget_render_image(ret, 1, 1);
  }

  D_RETURN(4, ret);
}

void
winwidget_create_window(winwidget ret,
                        int w,
                        int h)
{
  XSetWindowAttributes attr;
  XClassHint *xch;
  MWMHints mwmhints;
  Atom prop = None;
  int x = 0;
  int y = 0;

  D_ENTER(4);

  if (ret->full_screen) {
    w = scr->width;
    h = scr->height;

#ifdef HAVE_LIBXINERAMA
    if (opt.xinerama && xinerama_screens) {
      w = xinerama_screens[xinerama_screen].width;
      h = xinerama_screens[xinerama_screen].height;
    }
#endif /* HAVE_LIBXINERAMA */
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
#endif /* HAVE_LIBXINERAMA */
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
    PointerMotionMask | EnterWindowMask | LeaveWindowMask | KeyPressMask |
    KeyReleaseMask | ButtonMotionMask | ExposureMask | FocusChangeMask |
    PropertyChangeMask | VisibilityChangeMask;

  if (opt.borderless || ret->full_screen) {
    prop = XInternAtom(disp, "_MOTIF_WM_HINTS", True);
    if (prop == None) {
      weprintf("Window Manager does not support MWM hints. "
               "To get a borderless window I have to bypass your wm.");
      attr.override_redirect = True;
      mwmhints.flags = 0;
    } else {
      mwmhints.flags = MWM_HINTS_DECORATIONS;
      mwmhints.decorations = 0;
    }
  } else
    mwmhints.flags = 0;

  ret->win =
    XCreateWindow(disp, DefaultRootWindow(disp), x, y, w, h, 0, depth,
                  InputOutput, vis,
                  CWOverrideRedirect | CWSaveUnder | CWBackingStore |
                  CWColormap | CWBackPixel | CWBorderPixel | CWEventMask,
                  &attr);

  if (mwmhints.flags) {
    XChangeProperty(disp, ret->win, prop, prop, 32, PropModeReplace,
                    (unsigned char *) &mwmhints, PROP_MWM_HINTS_ELEMENTS);
  }

  XSetWMProtocols(disp, ret->win, &wmDeleteWindow, 1);
  winwidget_update_title(ret);
  xch = XAllocClassHint();
  xch->res_name = "feh";
  xch->res_class = "feh";
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
  if (ret->full_screen && opt.hide_pointer) {
    Cursor no_ptr;
    XColor black, dummy;
    Pixmap bm_no;
    bm_no = XCreateBitmapFromData(disp, ret->win, bm_no_data, 8, 8);
    XAllocNamedColor(disp, DefaultColormapOfScreen(DefaultScreenOfDisplay(disp)), "black", &black, &dummy);
    
    no_ptr = XCreatePixmapCursor(disp, bm_no, bm_no, &black, &black, 0, 0);
    XDefineCursor(disp, ret->win, no_ptr);
  }

  /* set the icon name property */
  XSetIconName(disp, ret->win, "feh");
  /* set the command hint */
  XSetCommand(disp, ret->win, cmdargv, cmdargc);

  winwidget_register(ret);
  D_RETURN_(4);
}

void
winwidget_update_title(winwidget ret)
{
  char *name;

  D_ENTER(4);
  D(4, ("winwid->name = %s\n", ret->name));
  name = ret->name ? ret->name : "feh";
  XStoreName(disp, ret->win, name);
  XSetIconName(disp, ret->win, name);
  /* XFlush(disp); */
  D_RETURN_(4);
}

void
winwidget_setup_pixmaps(winwidget winwid)
{
  D_ENTER(4);

  if (winwid->full_screen) {
    if (!(winwid->bg_pmap)) {
      if (winwid->gc == None) {
        XGCValues gcval;

        gcval.foreground = BlackPixel(disp, DefaultScreen(disp));
        winwid->gc = XCreateGC(disp, winwid->win, GCForeground, &gcval);
      }
      winwid->bg_pmap =
        XCreatePixmap(disp, winwid->win, scr->width, scr->height, depth);
    }
    XFillRectangle(disp, winwid->bg_pmap, winwid->gc, 0, 0, scr->width,
                   scr->height);
  } else {
    if (!winwid->bg_pmap || winwid->had_resize) {
      D(4, ("recreating background pixmap (%dx%d)\n", winwid->w, winwid->h));
      if (winwid->bg_pmap)
        XFreePixmap(disp, winwid->bg_pmap);

      if (winwid->w == 0)
        winwid->w = 1;
      if (winwid->h == 0)
        winwid->h = 1;
      winwid->bg_pmap =
        XCreatePixmap(disp, winwid->win, winwid->w, winwid->h, depth);
      winwid->had_resize = 0;
    }
  }
  D_RETURN_(4);
}

void
winwidget_render_image(winwidget winwid,
                       int resize,
                       int alias)
{
  int sx, sy, sw, sh, dx, dy, dw, dh;
  int calc_w, calc_h;

  D_ENTER(4);

  if (!winwid->full_screen && resize) {
    winwidget_resize(winwid, winwid->im_w, winwid->im_h);
    winwidget_reset_image(winwid);
  }

  /* bounds checks for panning */
  if (winwid->im_x > winwid->w)
    winwid->im_x = winwid->w;
  if (winwid->im_y > winwid->h)
    winwid->im_y = winwid->h;

  winwidget_setup_pixmaps(winwid);

  if (!winwid->full_screen
      && ((gib_imlib_image_has_alpha(winwid->im)) || (opt.geom_flags)
          || (winwid->im_x || winwid->im_y) || (winwid->zoom != 1.0)
          || (winwid->w > winwid->im_w || winwid->h > winwid->im_h)
          || (winwid->has_rotated)))
    feh_draw_checks(winwid);

  if (!winwid->full_screen && opt.scale_down
      && ((winwid->w < winwid->im_w) || (winwid->h < winwid->im_h))) {
    D(2, ("scaling down image\n"));

    feh_calc_needed_zoom(&(winwid->zoom), winwid->im_w, winwid->im_h,
                         winwid->w, winwid->h);
    winwidget_resize(winwid, winwid->im_w * winwid->zoom,
                     winwid->im_h * winwid->zoom);
  }

  if (resize && (winwid->full_screen || opt.geom_flags)) {
    int smaller;                /* Is the image smaller than screen? */
    int max_w, max_h;

    if (winwid->full_screen) {
      max_w = scr->width;
      max_h = scr->height;
#ifdef HAVE_LIBXINERAMA
      if (opt.xinerama && xinerama_screens) {
        max_w = xinerama_screens[xinerama_screen].width;
        max_h = xinerama_screens[xinerama_screen].height;
      }
#endif /* HAVE_LIBXINERAMA */
    } else {
      if (opt.geom_flags & WidthValue) {
        max_w = opt.geom_w;
      }
      if (opt.geom_flags & HeightValue) {
        max_h = opt.geom_h;
      }
    }

    D(4, ("Calculating for fullscreen/fixed geom render\n"));
    smaller = ((winwid->im_w < max_w) && (winwid->im_h < max_h));

    if (!smaller || opt.auto_zoom) {
      double ratio = 0.0;

      /* Image is larger than the screen (so wants shrinking), or it's
         smaller but wants expanding to fill it */
      ratio =
        feh_calc_needed_zoom(&(winwid->zoom), winwid->im_w, winwid->im_h,
                             max_w, max_h);

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
        double old_zoom = winwid->zoom;

        winwid->zoom = 0.01 * opt.default_zoom;
        if ((winwid->im_h * winwid->zoom) > max_h)
          winwid->zoom = old_zoom;
        if ((winwid->im_w * winwid->zoom) > max_w)
          winwid->zoom = old_zoom;

        winwid->im_x = ((int) (max_w - (winwid->im_w * winwid->zoom))) >> 1;
        winwid->im_y = ((int) (max_h - (winwid->im_h * winwid->zoom))) >> 1;
      } else {
        if (ratio > 1.0) {
          /* height is the factor */
          winwid->im_x = 0;
          winwid->im_y = ((int) (max_h - (winwid->im_h * winwid->zoom))) >> 1;
        } else {
          /* width is the factor */
          winwid->im_x = ((int) (max_w - (winwid->im_w * winwid->zoom))) >> 1;
          winwid->im_y = 0;
        }
      }
    } else {
      /* my modification to jens hack, allow --zoom without auto-zoom mode */
      if(opt.default_zoom) {
        winwid->zoom = 0.01 * opt.default_zoom;
      } else {
        winwid->zoom = 1.0;
      }
      /* Just center the image in the window */
      winwid->im_x = (int)(max_w - (winwid->im_w * winwid->zoom)) >> 1;
      winwid->im_y = (int)(max_h - (winwid->im_h * winwid->zoom)) >> 1;
    }
  }

  /* Now we ensure only to render the area we're looking at */
  dx = winwid->im_x;
  dy = winwid->im_y;
  if (dx < 0)
    dx = 0;
  if (dy < 0)
    dy = 0;

  if (winwid->im_x < 0)
    sx = 0 - (winwid->im_x / winwid->zoom);
  else
    sx = 0;

  if (winwid->im_y < 0)
    sy = 0 - (winwid->im_y / winwid->zoom);
  else
    sy = 0;
  
  calc_w = winwid->im_w * winwid->zoom;
  calc_h = winwid->im_h * winwid->zoom;
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

  sw = dw / winwid->zoom;
  sh = dh / winwid->zoom;

  D(5,
    ("sx: %d sy: %d sw: %d sh: %d dx: %d dy: %d dw: %d dh: %d zoom: %f\n", sx,
     sy, sw, sh, dx, dy, dw, dh, winwid->zoom));

  D(5, ("winwidget_render(): winwid->im_angle = %f\n", winwid->im_angle));
  if (winwid->has_rotated)
    gib_imlib_render_image_part_on_drawable_at_size_with_rotation(winwid->
                                                                  bg_pmap,
                                                                  winwid->im,
                                                                  sx, sy, sw,
                                                                  sh, dx, dy,
                                                                  dw, dh,
                                                                  winwid->
                                                                  im_angle, 1,
                                                                  1, alias);
  else
    gib_imlib_render_image_part_on_drawable_at_size(winwid->bg_pmap,
                                                    winwid->im, sx, sy, sw,
                                                    sh, dx, dy, dw, dh, 1,
                                                    gib_imlib_image_has_alpha
                                                    (winwid->im), alias);
  if (opt.caption_path) {
    /* cache bg pixmap. during caption entry, multiple redraws are done
     * because the caption overlay changes - the image doesn't though, so re-
     * rendering that is a waste of time */
    if (winwid->caption_entry) {
      GC gc;
      if (winwid->bg_pmap_cache)
        XFreePixmap(disp, winwid->bg_pmap_cache);
      winwid->bg_pmap_cache = XCreatePixmap(disp,
                                            winwid->win,
                                            winwid->w,
                                            winwid->h,
                                            depth);
      gc = XCreateGC(disp, winwid->win, 0, NULL);
      XCopyArea(disp, winwid->bg_pmap, winwid->bg_pmap_cache, gc, 0, 0, winwid->w, winwid->h, 0, 0);
      XFreeGC(disp, gc);
    }
    feh_draw_caption(winwid);
  }

  if (opt.draw_filename)
    feh_draw_filename(winwid);
  if (opt.draw_actions)
    feh_draw_actions(winwid);
  if ((opt.mode == MODE_ZOOM) && !alias)
    feh_draw_zoom(winwid);
  XSetWindowBackgroundPixmap(disp, winwid->win, winwid->bg_pmap);
  XClearWindow(disp, winwid->win);
  D_RETURN_(4);
}

void winwidget_render_image_cached(winwidget winwid) {
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
  XSetWindowBackgroundPixmap(disp, winwid->win, winwid->bg_pmap);
  XClearWindow(disp, winwid->win);
}

double
feh_calc_needed_zoom(double *zoom,
                     int orig_w,
                     int orig_h,
                     int dest_w,
                     int dest_h)
{
  double ratio = 0.0;

  D_ENTER(4);

  ratio = ((double) orig_w / orig_h) / ((double) dest_w / dest_h);

  if (ratio > 1.0)
    *zoom = ((double) dest_w / orig_w);
  else
    *zoom = ((double) dest_h / orig_h);

  D_RETURN(4, ratio);
}

Pixmap
feh_create_checks(void)
{
  static Pixmap checks_pmap = None;
  Imlib_Image checks = NULL;

  D_ENTER(4);
  if (checks_pmap == None) {
    int onoff, x, y;

    checks = imlib_create_image(16, 16);

    if (!checks)
      eprintf
        ("Unable to create a teeny weeny imlib image. I detect problems");

    for (y = 0; y < 16; y += 8) {
      onoff = (y / 8) & 0x1;
      for (x = 0; x < 16; x += 8) {
        if (onoff)
          gib_imlib_image_fill_rectangle(checks, x, y, 8, 8, 144, 144, 144,
                                         255);
        else
          gib_imlib_image_fill_rectangle(checks, x, y, 8, 8, 100, 100, 100,
                                         255);
        onoff++;
        if (onoff == 2)
          onoff = 0;
      }
    }
    checks_pmap = XCreatePixmap(disp, root, 16, 16, depth);
    gib_imlib_render_image_on_drawable(checks_pmap, checks, 0, 0, 1, 0, 0);
    gib_imlib_free_image_and_decache(checks);
  }
  D_RETURN(4, checks_pmap);
}

void
winwidget_clear_background(winwidget w)
{
  D_ENTER(4);
  XSetWindowBackgroundPixmap(disp, w->win, feh_create_checks());
  /* XClearWindow(disp, w->win); */
  D_RETURN_(4);
}

void
feh_draw_checks(winwidget win)
{
  static GC gc = None;
  XGCValues gcval;

  D_ENTER(4);
  if (gc == None) {
    gcval.tile = feh_create_checks();
    gcval.fill_style = FillTiled;
    gc = XCreateGC(disp, win->win, GCTile | GCFillStyle, &gcval);
  }
  XFillRectangle(disp, win->bg_pmap, gc, 0, 0, win->w, win->h);
  D_RETURN_(4);
}

void
winwidget_destroy_xwin(winwidget winwid)
{
  D_ENTER(4);
  if (winwid->win) {
    winwidget_unregister(winwid);
    XDestroyWindow(disp, winwid->win);
  }
  if (winwid->bg_pmap) {
    XFreePixmap(disp, winwid->bg_pmap);
    winwid->bg_pmap = None;
  }
  D_RETURN_(4);
}

void
winwidget_destroy(winwidget winwid)
{
  D_ENTER(4);
  winwidget_destroy_xwin(winwid);
  if (winwid->name)
    free(winwid->name);
  if ((winwid->type == WIN_TYPE_ABOUT) && winwid->file) {
    feh_file_free(FEH_FILE(winwid->file->data));
    free(winwid->file);
  }
  if (winwid->gc)
    XFreeGC(disp, winwid->gc);
  if (winwid->im)
    gib_imlib_free_image_and_decache(winwid->im);
  free(winwid);
  D_RETURN_(4);
}

void
winwidget_destroy_all(void)
{
  int i;

  D_ENTER(4);
  /* Have to DESCEND the list here, 'cos of the way _unregister works */
  for (i = window_num - 1; i >= 0; i--)
    winwidget_destroy(windows[i]);
  D_RETURN_(4);
}

void
winwidget_rerender_all(int resize,
                       int alias)
{
  int i;

  D_ENTER(4);
  /* Have to DESCEND the list here, 'cos of the way _unregister works */
  for (i = window_num - 1; i >= 0; i--)
    winwidget_render_image(windows[i], resize, alias);
  D_RETURN_(4);
}

winwidget
winwidget_get_first_window_of_type(unsigned int type)
{
  int i;

  D_ENTER(4);
  for (i = 0; i < window_num; i++)
    if (windows[i]->type == type)
      D_RETURN(4, windows[i]);
  D_RETURN(4, NULL);
}

int
winwidget_loadimage(winwidget winwid,
                    feh_file * file)
{
  D_ENTER(4);
  D(4, ("filename %s\n", file->filename));
  D_RETURN(4, feh_load_image(&(winwid->im), file));
}

void
winwidget_show(winwidget winwid)
{
  XEvent ev;

  D_ENTER(4);

  /* feh_debug_print_winwid(winwid); */
  if (!winwid->visible) {
    XMapWindow(disp, winwid->win);
    if (opt.full_screen)
      XMoveWindow(disp, winwid->win, 0, 0);
    /* wait for the window to map */
    D(4, ("Waiting for window to map\n"));
    XMaskEvent(disp, StructureNotifyMask, &ev);
    D(4, ("Window mapped\n"));
    winwid->visible = 1;
  }
  D_RETURN_(4);
}

int
winwidget_count(void)
{
  D_ENTER(4);
  D_RETURN(4, window_num);
}

void 
winwidget_move(winwidget winwid,
               int x,
               int y)
{
  D_ENTER(4);
  if (winwid && ((winwid->x != x) || (winwid->y != y))) {
    winwid->x = x;
    winwid->y = y;
    winwid->x = (x > scr->width) ? scr->width : x;
    winwid->y = (y > scr->height) ? scr->height : y;
    XMoveWindow(disp, winwid->win, winwid->x, winwid->y);
    XFlush(disp);
  } else {
    D(4, ("No move actually needed\n"));
  }
  D_RETURN_(4);
}

void
winwidget_resize(winwidget winwid,
                 int w,
                 int h)
{
  D_ENTER(4);
  if (opt.geom_flags) {
    winwid->had_resize = 1;
    return;
  }
  if (winwid && ((winwid->w != w) || (winwid->h != h))) {
    D(4, ("Really doing a resize\n"));
    /* winwidget_clear_background(winwid); */
    if (opt.screen_clip) {
      winwid->w = (w > scr->width) ? scr->width : w;
      winwid->h = (h > scr->height) ? scr->height : h;
    }
    XResizeWindow(disp, winwid->win, winwid->w, winwid->h);
    winwid->had_resize = 1;
    XFlush(disp);

#ifdef HAVE_LIBXINERAMA
    if (opt.xinerama && xinerama_screens) {
      int i;

      for (i = 0; i < num_xinerama_screens; i++) {
        xinerama_screen = 0;
        if (XY_IN_RECT(winwid->x, winwid->y,
                       xinerama_screens[i].x_org, xinerama_screens[i].y_org,
                       xinerama_screens[i].width, xinerama_screens[i].height)) {
          xinerama_screen = i;
          break;
        }
          
      }
    }
#endif /* HAVE_LIBXINERAMA */

  } else {
    D(4, ("No resize actually needed\n"));
  }

  D_RETURN_(4);
}

void
winwidget_hide(winwidget winwid)
{
  D_ENTER(4);
  XUnmapWindow(disp, winwid->win);
  winwid->visible = 0;
  D_RETURN_(4);
}

static void
winwidget_register(winwidget win)
{
  D_ENTER(4);
  D(5, ("window %p\n", win));
  window_num++;
  if (windows)
    windows = erealloc(windows, window_num * sizeof(winwidget));
  else
    windows = emalloc(window_num * sizeof(winwidget));
  windows[window_num - 1] = win;

  XSaveContext(disp, win->win, xid_context, (XPointer) win);
  D_RETURN_(4);
}

static void
winwidget_unregister(winwidget win)
{
  int i, j;

  D_ENTER(4);
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
  D_RETURN_(4);
}

winwidget
winwidget_get_from_window(Window win)
{
  winwidget ret = NULL;

  D_ENTER(4);
  if (XFindContext(disp, win, xid_context, (XPointer *) & ret) != XCNOENT)
    D_RETURN(4, ret);
  D_RETURN(4, NULL);
}

void
winwidget_rename(winwidget winwid,
                 char *newname)
{
  D_ENTER(4);
  if (winwid->name)
    free(winwid->name);
  winwid->name = estrdup(newname);
  winwidget_update_title(winwid);
  D_RETURN_(4);
}

void
winwidget_free_image(winwidget w)
{
  D_ENTER(4);
  if (w->im)
    gib_imlib_free_image_and_decache(w->im);
  w->im = NULL;
  w->im_w = 0;
  w->im_h = 0;
  D_RETURN_(4);
}

void
feh_debug_print_winwid(winwidget w)
{
  printf("winwid_debug:\n" "winwid = %p\n" "win = %ld\n" "w = %d\n" "h = %d\n"
         "im_w = %d\n" "im_h = %d\n" "im_angle = %f\n" "type = %d\n"
         "had_resize = %d\n" "im = %p\n" "GC = %p\n" "pixmap = %ld\n"
         "name = %s\n" "file = %p\n" "mode = %d\n" "im_x = %d\n" "im_y = %d\n"
         "zoom = %f\n" "click_offset_x = %d\n" "click_offset_y = %d\n"
         "im_click_offset_x = %d\n" "im_click_offset_y = %d\n"
         "has_rotated = %d\n", w, w->win, w->w, w->h, w->im_w, w->im_h,
         w->im_angle, w->type, w->had_resize, w->im, w->gc, w->bg_pmap,
         w->name, w->file, w->mode, w->im_x, w->im_y, w->zoom,
         w->click_offset_x, w->click_offset_y, w->im_click_offset_x,
         w->im_click_offset_y, w->has_rotated);
}

void
winwidget_reset_image(winwidget winwid)
{
  D_ENTER(4);
  winwid->zoom = 1.0;
  winwid->im_x = 0;
  winwid->im_y = 0;
  winwid->im_angle = 0.0;
  winwid->has_rotated = 0;
  D_RETURN_(4);
}

void
winwidget_sanitise_offsets(winwidget winwid)
{
  int far_left, far_top;
  int min_x, max_x, max_y, min_y;

  D_ENTER(4);

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

  D_RETURN_(4);
}


void
winwidget_size_to_image(winwidget winwid)
{
  D_ENTER(4);
  winwidget_resize(winwid, winwid->im_w * winwid->zoom,
                   winwid->im_h * winwid->zoom);
  winwid->im_x = winwid->im_y = 0;
  winwidget_render_image(winwid, 0, 1);
  D_RETURN_(4);
}

int winwidget_get_width(winwidget winwid) {
  int rect[4];
  D_ENTER(4);
  winwidget_get_geometry(winwid, rect);
  D_RETURN(4, rect[2]);
}

int winwidget_get_height(winwidget winwid) {
  int rect[4];
  D_ENTER(4);
  winwidget_get_geometry(winwid, rect);
  D_RETURN(4, rect[3]);
}

void winwidget_get_geometry(winwidget winwid, int *rect) {
  int bw, bp;
  D_ENTER(4);
  if (!rect)
    return;

  XGetGeometry(disp, winwid->win, &root, 
               &(rect[0]), &(rect[1]), &(rect[2]), &(rect[3]), &bw, &bp);

  /* update the window geometry (in case it's inaccurate) */
  winwid->x = rect[0];
  winwid->y = rect[1];
  winwid->w = rect[2];
  winwid->h = rect[3];
  D_RETURN_(4);
}

void winwidget_show_menu(winwidget winwid) {
  int x, y, b;
  unsigned int c;
  Window r;

  XQueryPointer(disp, winwid->win, &r, &r, &x, &y, &b, &b, &c);
  if (winwid->type == WIN_TYPE_ABOUT)
  {
    if (!menu_about_win)
        feh_menu_init_about_win();
    feh_menu_show_at_xy(menu_about_win, winwid, x, y);
  }
  else if (winwid->type == WIN_TYPE_SINGLE)
  {
    if (!menu_single_win)
        feh_menu_init_single_win();
    feh_menu_show_at_xy(menu_single_win, winwid, x, y);
  }
  else if (winwid->type == WIN_TYPE_THUMBNAIL)
  {
    if (!menu_thumbnail_win)
        feh_menu_init_thumbnail_win();
    feh_menu_show_at_xy(menu_thumbnail_win, winwid, x, y);
  }
  else if (winwid->type == WIN_TYPE_THUMBNAIL_VIEWER)
  {
    if (!menu_single_win)
        feh_menu_init_thumbnail_viewer();
    feh_menu_show_at_xy(menu_thumbnail_viewer, winwid, x, y);
  }
  else
  {
    if (!menu_main)
        feh_menu_init_main();
    feh_menu_show_at_xy(menu_main, winwid, x, y);
  }
}
