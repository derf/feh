/* menu.c

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
#include "support.h"
#include "thumbnail.h"
#include "winwidget.h"
#include "filelist.h"
#include "options.h"

Window menu_cover = 0;
feh_menu *menu_root = NULL;
feh_menu *menu_main = NULL;
feh_menu *menu_single_win = NULL;
feh_menu *menu_about_win = NULL;
feh_menu *menu_thumbnail_viewer = NULL;
feh_menu *menu_thumbnail_win = NULL;
feh_menu *menu_bg = NULL;
static feh_menu_list *menus = NULL;
static int common_menus = 0;

static void feh_menu_cb_about(feh_menu * m,
                              feh_menu_item * i,
                              void *data);
static void feh_menu_cb_close(feh_menu * m,
                              feh_menu_item * i,
                              void *data);
static void feh_menu_cb_exit(feh_menu * m,
                             feh_menu_item * i,
                             void *data);
static void feh_menu_cb_reload(feh_menu * m,
                               feh_menu_item * i,
                               void *data);
static void feh_menu_cb_remove(feh_menu * m,
                               feh_menu_item * i,
                               void *data);
static void feh_menu_cb_delete(feh_menu * m,
                               feh_menu_item * i,
                               void *data);
static void feh_menu_cb_reset(feh_menu * m,
                              feh_menu_item * i,
                              void *data);

static void feh_menu_cb_remove_thumb(feh_menu * m,
                                     feh_menu_item * i,
                                     void *data);
static void feh_menu_cb_delete_thumb(feh_menu * m,
                                     feh_menu_item * i,
                                     void *data);
static void feh_menu_cb_background_set_tiled(feh_menu * m,
                                             feh_menu_item * i,
                                             void *data);
static void feh_menu_cb_background_set_scaled(feh_menu * m,
                                              feh_menu_item * i,
                                              void *data);
static void feh_menu_cb_background_set_seamless(feh_menu * m,
                                                feh_menu_item * i,
                                                void *data);
static void feh_menu_cb_background_set_centered(feh_menu * m,
                                                feh_menu_item * i,
                                                void *data);
static void feh_menu_cb_background_set_tiled_no_file(feh_menu * m,
                                                     feh_menu_item * i,
                                                     void *data);
static void feh_menu_cb_background_set_scaled_no_file(feh_menu * m,
                                                      feh_menu_item * i,
                                                      void *data);
static void feh_menu_cb_background_set_centered_no_file(feh_menu * m,
                                                        feh_menu_item * i,
                                                        void *data);

static void feh_menu_cb_sort_filename(feh_menu * m,
                                      feh_menu_item * i,
                                      void *data);
static void feh_menu_cb_sort_imagename(feh_menu * m,
                                       feh_menu_item * i,
                                       void *data);
static void feh_menu_cb_sort_filesize(feh_menu * m,
                                      feh_menu_item * i,
                                      void *data);
static void feh_menu_cb_sort_randomize(feh_menu * m,
                                       feh_menu_item * i,
                                       void *data);
static void feh_menu_cb_jump_to(feh_menu * m,
                                feh_menu_item * i,
                                void *data);
static feh_menu *feh_menu_func_gen_jump(feh_menu * m,
                                        feh_menu_item * i,
                                        void *data);
static feh_menu *feh_menu_func_gen_info(feh_menu * m,
                                        feh_menu_item * i,
                                        void *data);
static void feh_menu_func_free_info(feh_menu * m,
                                    void *data);
static void feh_menu_cb_save_image(feh_menu * m,
                                   feh_menu_item * i,
                                   void *data);
static void feh_menu_cb_save_filelist(feh_menu * m,
                                      feh_menu_item * i,
                                      void *data);
static void feh_menu_cb_fit(feh_menu * m,
                            feh_menu_item * i,
                            void *data);
static void feh_menu_cb_opt_draw_filename(feh_menu * m,
                                          feh_menu_item * i,
                                          void *data);
static void feh_menu_cb_opt_keep_http(feh_menu * m,
                                      feh_menu_item * i,
                                      void *data);
static void feh_menu_cb_opt_freeze_window(feh_menu * m,
                                          feh_menu_item * i,
                                          void *data);
static void feh_menu_cb_opt_fullscreen(feh_menu * m,
                                       feh_menu_item * i,
                                       void *data);
static void feh_menu_func_free_options(feh_menu * m,
                                       void *data);
static feh_menu *feh_menu_func_gen_options(feh_menu * m,
                                           feh_menu_item * i,
                                           void *data);
static void feh_menu_cb_edit_rotate(feh_menu * m,
                                    feh_menu_item * i,
                                    void *data);
static void feh_menu_cb_opt_auto_zoom(feh_menu * m,
                                      feh_menu_item * i,
                                      void *data);
#ifdef HAVE_LIBXINERAMA
static void feh_menu_cb_opt_xinerama(feh_menu * m,
                                     feh_menu_item * i,
                                     void *data);
#endif /* HAVE_LIBXINERAMA */


feh_menu *
feh_menu_new(void)
{
  feh_menu *m;
  XSetWindowAttributes attr;
  feh_menu_list *l;
  static Imlib_Image bg = NULL;
  static Imlib_Border border;

  D_ENTER(4);

  m = (feh_menu *) emalloc(sizeof(feh_menu));

  attr.backing_store = NotUseful;
  attr.override_redirect = True;
  attr.colormap = cm;
  attr.border_pixel = 0;
  attr.background_pixmap = None;
  attr.save_under = False;
  attr.do_not_propagate_mask = True;

  m->win =
    XCreateWindow(disp, root, 1, 1, 1, 1, 0, depth, InputOutput, vis,
                  CWOverrideRedirect | CWSaveUnder | CWBackingStore |
                  CWColormap | CWBackPixmap | CWBorderPixel | CWDontPropagate,
                  &attr);
  XSelectInput(disp, m->win,
               ButtonPressMask | ButtonReleaseMask | EnterWindowMask |
               LeaveWindowMask | PointerMotionMask | ButtonMotionMask);

  m->name = NULL;
  m->fehwin = NULL;
  m->pmap = 0;
  m->x = 0;
  m->y = 0;
  m->w = 0;
  m->h = 0;
  m->visible = 0;
  m->items = NULL;
  m->next = NULL;
  m->prev = NULL;
  m->updates = NULL;
  m->needs_redraw = 1;
  m->func_free = NULL;
  m->data = NULL;
  m->calc = 0;
  m->bg = NULL;

  l = emalloc(sizeof(feh_menu_list));
  l->menu = m;
  l->next = menus;
  menus = l;

  if (!bg) {
    feh_load_image_char(&bg, opt.menu_bg);
    if (bg) {
      border.left = opt.menu_border;
      border.right = opt.menu_border;
      border.top = opt.menu_border;
      border.bottom = opt.menu_border;
      imlib_context_set_image(bg);
      imlib_image_set_border(&border);
    }
  }

  if (bg)
    m->bg = gib_imlib_clone_image(bg);

  D_RETURN(4, m);
}

void
feh_menu_free(feh_menu * m)
{
  feh_menu_item *i;
  feh_menu_list *l, *pl = NULL;

  D_ENTER(4);

  if (m->name)
    free(m->name);
  XDestroyWindow(disp, m->win);
  if (m->pmap)
    XFreePixmap(disp, m->pmap);
  if (m->updates)
    imlib_updates_free(m->updates);
  for (i = m->items; i;) {
    feh_menu_item *ii;

    ii = i;
    i = i->next;
    if (ii->func_free)
      (ii->func_free) (ii->data);
    if (ii->text)
      free(ii->text);
    if (ii->submenu)
      free(ii->submenu);
    free(ii);
  }

  for (l = menus; l; l = l->next) {
    if (l->menu == m) {
      if (pl)
        pl->next = l->next;
      else
        menus = l->next;
      free(l);
      break;
    }
    pl = l;
  }
  free(m);

  D_RETURN_(4);
}

feh_menu_item *
feh_menu_find_selected(feh_menu * m)
{
  feh_menu_item *i;

  D_ENTER(4);

  D(5, ("menu %p\n", m));

  for (i = m->items; i; i = i->next) {
    if (MENU_ITEM_IS_SELECTED(i))
      D_RETURN(4, i);
  }
  D_RETURN(4, NULL);
}

feh_menu_item *
feh_menu_find_selected_r(feh_menu * m, feh_menu **parent)
{
  feh_menu_item *i, *ii;
  feh_menu *mm;

  D_ENTER(4);

  D(5, ("menu %p\n", m));

  for (i = m->items; i; i = i->next) {
    if (MENU_ITEM_IS_SELECTED(i)) {
      if (parent)
        *parent = m;
      D_RETURN(4, i);
    } else if (i->submenu) {
      mm = feh_menu_find(i->submenu);
      if (mm) {
        ii = feh_menu_find_selected_r(mm, parent);
      	if (ii)
       		D_RETURN(4, ii);
      }
    }
  }
  if (parent)
    *parent = m;
  D_RETURN(4, NULL);
}

void
feh_menu_select_next(feh_menu *selected_menu, feh_menu_item *selected_item) {
  feh_menu_item *i;
  if (!selected_item) {
    /* jump to first item, select it */
    feh_menu_select(selected_menu, selected_menu->items);
  } else {
    i = selected_item;
    while (1) {
      i = i->next;
      if (!i)
        i = selected_menu->items;
      if (i->func || i->submenu || i->func_gen_sub || i->text) {
        break;
      }
    }
    feh_menu_deselect_selected(selected_menu);
    feh_menu_select(selected_menu, i);
  }
}

void
feh_menu_select_prev(feh_menu *selected_menu, feh_menu_item *selected_item) {
  feh_menu_item *i, *ii;
  if (!selected_item) {
    /* jump to last item, select it */
    for (i = selected_menu->items; i->next; i = i->next);
    feh_menu_select(selected_menu, i);
  } else {
    i = selected_item;
    while (1) {
      i = i->prev;
      if (!i) {
        i = selected_menu->items;
        for (ii = selected_menu->items; ii->next; ii = ii->next);
        i = ii;
      }
      if (i->func || i->submenu || i->func_gen_sub || i->text) {
        break;
      }
    }
    feh_menu_deselect_selected(selected_menu);
    feh_menu_select(selected_menu, i);
  }
}

void
feh_menu_select_parent(feh_menu *selected_menu, feh_menu_item *selected_item) {
  feh_menu *m;
  feh_menu_item *i;
  /* find the parent menu's item which refers to this menu's name */
  if (selected_menu->prev) {
    m = selected_menu->prev;
    for (i = m->items; i; i = i->next) {
      if(i->submenu && !strcmp(i->submenu, selected_menu->name))
        break;
    }
    /* shouldn't ever happen */
    if (i == NULL)
      i = m->items;
    feh_menu_deselect_selected(selected_menu);
    feh_menu_select(m, i);
  }
}

void
feh_menu_select_submenu(feh_menu *selected_menu, feh_menu_item *selected_item) {
  if (selected_menu->next) {
    feh_menu_deselect_selected(selected_menu);
    feh_menu_select(selected_menu->next, selected_menu->next->items);
  }
}

void feh_menu_item_activate(feh_menu *m, 
                            feh_menu_item *i) {
  /* watch out for this. I put it this way around so the menu
      goes away *before* we perform the action, if we start
      freeing menus on hiding, it will break ;-) */
  if ((i) && (i->func)) {
    feh_menu_hide(menu_root, False);
    feh_main_iteration(0);
    (i->func) (m, i, i->data);
    if(m->func_free)
        m->func_free(m, m->data);
  }
}

feh_menu_item *
feh_menu_find_at_xy(feh_menu * m,
                    int x,
                    int y)
{
  feh_menu_item *i;

  D_ENTER(4);
  D(4, ("looking for menu item at %d,%d\n", x, y));
  for (i = m->items; i; i = i->next) {
    if (XY_IN_RECT(x, y, i->x, i->y, i->w, i->h)) {
      D(4, ("Found an item\n"));
      D_RETURN(4, i);
    }
  }
  D(4, ("didn't find an item\n"));
  D_RETURN(4, NULL);
}

void
feh_menu_deselect_selected(feh_menu * m)
{
  feh_menu_item *i;

  D_ENTER(4);

  if (!m)
    D_RETURN_(4);

  i = feh_menu_find_selected(m);
  if (i) {
    D(4, ("found a selected menu, deselecting it\n"));
    MENU_ITEM_SET_NORMAL(i);
    m->updates = imlib_update_append_rect(m->updates, i->x, i->y, i->w, i->h);
    m->needs_redraw = 1;
  }
  D_RETURN_(4);
}

void
feh_menu_select(feh_menu * m,
                feh_menu_item * i)
{
  D_ENTER(4);
  MENU_ITEM_SET_SELECTED(i);
  m->updates = imlib_update_append_rect(m->updates, i->x, i->y, i->w, i->h);
  m->needs_redraw = 1;
  if (m->next) {
    m->next->prev = NULL;
    feh_menu_hide(m->next, TRUE);
    m->next = NULL;
  }
  if (i->submenu) {
    feh_menu *mm;

    mm = feh_menu_find(i->submenu);
    if (mm)
      feh_menu_show_at_submenu(mm, m, i);
    else if (i->func_gen_sub)
      feh_menu_show_at_submenu(i->func_gen_sub(m, i, i->data), m, i);
  }
  D_RETURN_(4);
}

void
feh_menu_show_at(feh_menu * m, int x, int y)
{
  D_ENTER(4);

  if (m->calc)
    feh_menu_calc_size(m);
  if (!menu_cover) {
    XSetWindowAttributes attr;

    D(4, ("creating menu cover window\n"));
    attr.override_redirect = True;
    attr.do_not_propagate_mask = True;
    menu_cover =
      XCreateWindow(disp, root, 0, 0, scr->width, scr->height, 0, 0,
                    InputOnly, vis, CWOverrideRedirect | CWDontPropagate,
                    &attr);
    XSelectInput(disp, menu_cover,
      KeyPressMask | ButtonPressMask | ButtonReleaseMask | 
      EnterWindowMask | LeaveWindowMask | PointerMotionMask | 
      ButtonMotionMask);

    XRaiseWindow(disp, menu_cover);
    XMapWindow(disp, menu_cover);
    menu_root = m;
    XUngrabPointer(disp, CurrentTime);
    XSetInputFocus(disp, menu_cover, RevertToPointerRoot, CurrentTime);
  }
  m->visible = 1;
  XMoveWindow(disp, m->win, x, y);
  m->x = x;
  m->y = y;
  XRaiseWindow(disp, m->win);
  feh_menu_redraw(m);
  XMapWindow(disp, m->win);
  D_RETURN_(4);
}

void
feh_menu_show_at_xy(feh_menu * m,
                    winwidget winwid,
                    int x,
                    int y)
{
  D_ENTER(4);

  if (!m)
    D_RETURN_(4);

  if (m->calc)
    feh_menu_calc_size(m);
  m->fehwin = winwid;
  if ((x + m->w) > scr->width)
    x = scr->width - m->w;
  if ((y + m->h) > scr->height)
    y = scr->height - m->h;

#if 0 
/* #ifdef HAVE_LIBXINERAMA */
/* this doesn't work correctly :( -- pabs */
  if (opt.xinerama && xinerama_screens) {
    if ((x + m->w) > xinerama_screens[xinerama_screen].width)
      x = xinerama_screens[xinerama_screen].width - m->w;
    if ((y + m->h) > xinerama_screens[xinerama_screen].height)
      y = xinerama_screens[xinerama_screen].height - m->h;

  }
#endif /* HAVE_LIBXINERAMA */

  if (x < 0)
    x = 0;
  if (y < 0)
    y = 0;
  feh_menu_move(m, x, y);
  feh_menu_show(m);
  D_RETURN_(4);
}

void
feh_menu_show_at_submenu(feh_menu * m,
                         feh_menu * parent_m,
                         feh_menu_item * i)
{
  int mx, my;

  D_ENTER(4);

  if (!m)
    D_RETURN_(4);

  if (m->calc)
    feh_menu_calc_size(m);
  mx = parent_m->x + parent_m->w;
  my = parent_m->y + i->y - FEH_MENU_PAD_TOP;
  m->fehwin = parent_m->fehwin;
  parent_m->next = m;
  m->prev = parent_m;
  feh_menu_move(m, mx, my);
  feh_menu_show(m);
  D_RETURN_(4);
}

void
feh_menu_move(feh_menu * m,
              int x,
              int y)
{
  int dx, dy;

  D_ENTER(4);

  if (!m)
    D_RETURN_(4);
  dx = x - m->x;
  dy = y - m->y;
  if (m->visible)
    XMoveWindow(disp, m->win, x, y);
  m->x = x;
  m->y = y;
  D_RETURN_(4);
}

void
feh_menu_slide_all_menus_relative(int dx,
                                  int dy)
{
  int i;
  feh_menu_list *m;
  double vector_len = 0;
  int stepx = 0;
  int stepy = 0;

  D_ENTER(4);
  vector_len = sqrt(dx * dx + dy * dy);
  if (vector_len) {
    if (dx)
      stepx = rint(dx / vector_len);

    if (dy)
      stepy = rint(dy / vector_len);

  }
  for (i = 0; i < vector_len; i++) {
    for (m = menus; m; m = m->next) {
      if (m->menu->visible)
        feh_menu_move(m->menu, m->menu->x + stepx, m->menu->y + stepy);

    }
    XWarpPointer(disp, None, None, 0, 0, 0, 0, stepx, stepy);
  }
  D_RETURN_(4);
}

void
feh_menu_hide(feh_menu * m,
              int func_free)
{
  D_ENTER(4);

  if (!m->visible)
    D_RETURN_(4);
  if (m->next) {
    m->next->prev = NULL;
    feh_menu_hide(m->next, func_free);
    m->next = NULL;
  }
  if (m == menu_root) {
    if (menu_cover) {
      D(4, ("DESTROYING menu cover\n"));
      XDestroyWindow(disp, menu_cover);
      menu_cover = 0;
    }
    menu_root = NULL;
  }
  m->visible = 0;
  XUnmapWindow(disp, m->win);
  if (func_free && m->func_free)
    m->func_free(m, m->data);
  else
    feh_menu_deselect_selected(m);
  D_RETURN_(4);
}

void
feh_menu_show(feh_menu * m)
{
  D_ENTER(4);
  if (!m)
    D_RETURN_(4);
  feh_menu_show_at(m, m->x, m->y);
  D_RETURN_(4);
}

feh_menu_item *
feh_menu_add_toggle_entry(feh_menu * m,
                          char *text,
                          Imlib_Image icon,
                          char *submenu,
                          menu_func func,
                          void *data,
                          void (*func_free) (void *data),
                          int setting)
{
  feh_menu_item *mi;

  D_ENTER(4);
  mi = feh_menu_add_entry(m, text, icon, submenu, func, data, func_free);
  mi->is_toggle = TRUE;
  MENU_ITEM_TOGGLE_SET(mi, setting);
  D_RETURN(4, mi);
}

feh_menu_item *
feh_menu_add_entry(feh_menu * m,
                   char *text,
                   Imlib_Image icon,
                   char *submenu,
                   menu_func func,
                   void *data,
                   void (*func_free) (void *data))
{
  feh_menu_item *mi, *ptr;

  D_ENTER(4);

  mi = (feh_menu_item *) emalloc(sizeof(feh_menu_item));
  mi->state = MENU_ITEM_STATE_NORMAL;
  mi->icon = icon;
  mi->is_toggle = FALSE;
  if (text)
    mi->text = estrdup(text);
  else
    mi->text = NULL;
  if (submenu)
    mi->submenu = estrdup(submenu);
  else
    mi->submenu = NULL;
  mi->func = func;
  mi->func_free = func_free;
  mi->data = data;
  mi->func_gen_sub = NULL;
  mi->next = NULL;
  mi->prev = NULL;

  if (!m->items)
    m->items = mi;
  else {
    for (ptr = m->items; ptr; ptr = ptr->next) {
      if (!ptr->next) {
        ptr->next = mi;
        mi->prev = ptr;
        break;
      }
    }
  }
  m->calc = 1;
  D_RETURN(4, mi);
}


void
feh_menu_entry_get_size(feh_menu * m,
                        feh_menu_item * i,
                        int *w,
                        int *h)
{
  int tw, th;

  D_ENTER(4);

  if (i->text) {
    gib_imlib_get_text_size(opt.menu_fn, i->text, opt.menu_style_l, &tw, &th,
                            IMLIB_TEXT_TO_RIGHT);
    *w =
      tw + FEH_MENUITEM_PAD_LEFT + FEH_MENUITEM_PAD_RIGHT;
    *h =
      th + FEH_MENUITEM_PAD_TOP + FEH_MENUITEM_PAD_BOTTOM;
  } else {
    *w = FEH_MENUITEM_PAD_LEFT + FEH_MENUITEM_PAD_RIGHT;
    *h = FEH_MENUITEM_PAD_TOP + FEH_MENUITEM_PAD_BOTTOM;
  }

  D_RETURN_(4);
  m = NULL;
}

void
feh_menu_calc_size(feh_menu * m)
{
  int prev_w, prev_h;
  feh_menu_item *i;
  int j = 0, count = 0, max_w = 0, max_h = 0, icon_w = 0, next_w = 0;
  int toggle_w = 0;

  D_ENTER(4);

  prev_w = m->w;
  prev_h = m->h;
  m->calc = 0;

  for (i = m->items; i; i = i->next) {
    int w, h;

    feh_menu_entry_get_size(m, i, &w, &h);
    if (w > max_w)
      max_w = w;
    if (h > max_h)
      max_h = h;
    if (i->submenu) {
      next_w = FEH_MENU_SUBMENU_W;
      if (FEH_MENU_SUBMENU_H > max_h)
        max_h = FEH_MENU_SUBMENU_H;
    }
    if (i->is_toggle) {
      toggle_w = FEH_MENU_TOGGLE_W + FEH_MENU_TOGGLE_PAD;
      if (FEH_MENU_TOGGLE_H > max_h)
        max_h = FEH_MENU_TOGGLE_H;
    }
    count++;
  }

  for (i = m->items; i; i = i->next) {
    if (i->icon) {
      Imlib_Image im;

      im = i->icon;
      if (im) {
        int iw, ih, ow, oh;

        iw = gib_imlib_image_get_width(im);
        ih = gib_imlib_image_get_height(im);
        if (ih <= max_h) {
          ow = iw;
          oh = ih;
        } else {
          ow = (iw * max_h) / ih;
          oh = max_h;
        }
        if (ow > icon_w)
          icon_w = ow;
      }
    }
  }
  m->h = FEH_MENU_PAD_TOP;
  for (i = m->items; i; i = i->next) {
    i->x = FEH_MENU_PAD_LEFT;
    i->y = m->h;
    i->w = max_w + icon_w + toggle_w + next_w;
    i->icon_x = FEH_MENUITEM_PAD_LEFT;
    i->toggle_x = i->icon_x + icon_w;
    i->text_x = i->toggle_x + toggle_w;
    i->sub_x = i->text_x + max_w;
    if (i->text)
      i->h = max_h;
    else
      i->h = FEH_MENU_SEP_MAX_H;
    m->h += i->h;
    j++;
  }
  m->h += FEH_MENU_PAD_BOTTOM;
  m->w =
    next_w + toggle_w + icon_w + max_w + FEH_MENU_PAD_LEFT +
    FEH_MENU_PAD_RIGHT;

  if ((prev_w != m->w) || (prev_h != m->h)) {
    if (m->pmap)
      XFreePixmap(disp, m->pmap);
    m->pmap = 0;
    m->needs_redraw = 1;
    XResizeWindow(disp, m->win, m->w, m->h);
    m->updates = imlib_update_append_rect(m->updates, 0, 0, m->w, m->h);
  }
  D(4, ("menu size calculated. w=%d h=%d\n", m->w, m->h));

  /* Make sure bg is same size */
  if (m->bg) {
    int bg_w, bg_h;

    bg_w = gib_imlib_image_get_width(m->bg);
    bg_h = gib_imlib_image_get_height(m->bg);

    if (m->w != bg_w || m->h != bg_h) {
      Imlib_Image newim = imlib_create_image(m->w, m->h);

      D(3, ("resizing bg to %dx%d\n", m->w, m->h));

      gib_imlib_blend_image_onto_image(newim, m->bg, 0, 0, 0, bg_w, bg_h, 0,
                                       0, m->w, m->h, 0, 0, 1);
      gib_imlib_free_image_and_decache(m->bg);
      m->bg = newim;
    }
  }

  D_RETURN_(4);
}

void
feh_menu_draw_item(feh_menu * m,
                   feh_menu_item * i,
                   Imlib_Image im,
                   int ox,
                   int oy)
{
  D_ENTER(5);

  D(5,
    ("drawing item %p (text %s) on menu %p (name %s)\n", i, i->text, m,
     m->name));

  if (i->text) {
    D(5, ("text item\n"));
    if (MENU_ITEM_IS_SELECTED(i)) {
      D(5, ("selected item\n"));
      /* draw selected image */
      feh_menu_item_draw_at(i->x, i->y, i->w, i->h, im, ox, oy, 1);
    } else {
      D(5, ("unselected item\n"));
      /* draw unselected image */
      feh_menu_item_draw_at(i->x, i->y, i->w, i->h, im, ox, oy, 0);
    }

    /* draw text */
    gib_imlib_text_draw(im, opt.menu_fn, opt.menu_style_l,
                        i->x - ox + i->text_x,
                        i->y - oy + FEH_MENUITEM_PAD_TOP, i->text,
                        IMLIB_TEXT_TO_RIGHT, 0, 0, 0, 255);
    if (i->icon) {
      Imlib_Image im2;

      D(5, ("icon item\n"));

      im2 = i->icon;
      if (im2) {
        int iw, ih, ow, oh;

        iw = gib_imlib_image_get_width(im2);
        ih = gib_imlib_image_get_height(im2);
        if (ih <= (i->h - FEH_MENUITEM_PAD_TOP - FEH_MENUITEM_PAD_BOTTOM)) {
          ow = iw;
          oh = ih;
        } else {
          ow =
            (iw * (i->h - FEH_MENUITEM_PAD_TOP - FEH_MENUITEM_PAD_BOTTOM)) /
            ih;
          oh = i->h - FEH_MENUITEM_PAD_TOP - FEH_MENUITEM_PAD_BOTTOM;
        }
        gib_imlib_blend_image_onto_image(im, im2, 0, 0, 0, iw, ih,
                                         i->x + i->icon_x - ox,
                                         i->y + FEH_MENUITEM_PAD_TOP +
                                         (((i->h - FEH_MENUITEM_PAD_TOP -
                                            FEH_MENUITEM_PAD_BOTTOM) -
                                           oh) / 2) - oy, ow, oh, 1, 1, 1);
        gib_imlib_free_image(im2);
      }
    }
    if (i->submenu) {
      D(5, ("submenu item\n"));
      feh_menu_draw_submenu_at(i->x + i->sub_x,
                               i->y + FEH_MENUITEM_PAD_TOP +
                               ((i->h - FEH_MENUITEM_PAD_TOP -
                                 FEH_MENUITEM_PAD_BOTTOM -
                                 FEH_MENU_SUBMENU_H) / 2), FEH_MENU_SUBMENU_W,
                               FEH_MENU_SUBMENU_H, im, ox, oy,
                               MENU_ITEM_IS_SELECTED(i));
    }
    if (i->is_toggle) {
      D(5, ("toggleable item\n"));
      feh_menu_draw_toggle_at(i->x + i->toggle_x,
                              i->y + FEH_MENUITEM_PAD_TOP +
                              ((i->h - FEH_MENUITEM_PAD_TOP -
                                FEH_MENUITEM_PAD_BOTTOM -
                                FEH_MENU_TOGGLE_H) / 2), FEH_MENU_TOGGLE_W,
                              FEH_MENU_TOGGLE_H, im, ox, oy,
                              MENU_ITEM_IS_ON(i));
    }
  } else {
    D(5, ("separator item\n"));
    feh_menu_draw_separator_at(i->x, i->y, i->w, i->h, im, ox, oy);
  }
  D_RETURN_(5);
  m = NULL;
}

void
feh_menu_redraw(feh_menu * m)
{
  Imlib_Updates u, uu;

  D_ENTER(5);

  if ((!m->needs_redraw) || (!m->visible) || (!m->updates))
    D_RETURN_(5);
  m->needs_redraw = 0;
  if (!m->pmap)
    m->pmap = XCreatePixmap(disp, m->win, m->w, m->h, depth);
  XSetWindowBackgroundPixmap(disp, m->win, m->pmap);

  u = imlib_updates_merge_for_rendering(m->updates, m->w, m->h);
  m->updates = NULL;
  if (u) {
    D(5, ("I have updates to render\n"));
    for (uu = u; u; u = imlib_updates_get_next(u)) {
      int x, y, w, h;
      Imlib_Image im;

      imlib_updates_get_coordinates(u, &x, &y, &w, &h);
      D(5, ("update coords %d,%d %d*%d\n", x, y, w, h));
      im = imlib_create_image(w, h);
      gib_imlib_image_fill_rectangle(im, 0, 0, w, h, 0, 0, 0, 0);
      if (im) {
        feh_menu_draw_to_buf(m, im, x, y);
        gib_imlib_render_image_on_drawable(m->pmap, im, x, y, 1, 0, 0);
        gib_imlib_free_image(im);
        XClearArea(disp, m->win, x, y, w, h, False);
      }
    }
    imlib_updates_free(uu);
  }
  D_RETURN_(5);
}

feh_menu *
feh_menu_find(char *name)
{
  feh_menu_list *l;

  D_ENTER(4);
  for (l = menus; l; l = l->next) {
    if ((l->menu->name) && (!strcmp(l->menu->name, name)))
      D_RETURN(4, l->menu);
  }
  D_RETURN(4, NULL);
}

void
feh_menu_draw_to_buf(feh_menu * m,
                     Imlib_Image im,
                     int ox,
                     int oy)
{
  feh_menu_item *i;
  int w, h;

  D_ENTER(5);
  w = gib_imlib_image_get_width(im);
  h = gib_imlib_image_get_height(im);

  feh_menu_draw_menu_bg(m, im, ox, oy);

  for (i = m->items; i; i = i->next) {
    if (RECTS_INTERSECT(i->x, i->y, i->w, i->h, ox, oy, w, h))
      feh_menu_draw_item(m, i, im, ox, oy);
  }
  D_RETURN_(5);
}

void
feh_menu_draw_menu_bg(feh_menu * m,
                      Imlib_Image im,
                      int ox,
                      int oy)
{
  int w, h;

  D_ENTER(5);

  w = gib_imlib_image_get_width(im);
  h = gib_imlib_image_get_height(im);

  if (m->bg)
    gib_imlib_blend_image_onto_image(im, m->bg, 0, ox, oy, w, h, 0, 0, w, h,
                                     0, 0, 0);
  else
    gib_imlib_image_fill_rectangle(im, 0, 0, w, h, 205, 203, 176, 255);

  D_RETURN_(5);
}

void
feh_menu_draw_toggle_at(int x,
                        int y,
                        int w,
                        int h,
                        Imlib_Image dst,
                        int ox,
                        int oy,
                        int on)
{
  D_ENTER(5);
  x -= ox;
  y -= oy;
  if (on)
    gib_imlib_image_fill_rectangle(dst, x, y, w, h, 0, 0, 0, 255);
  else
    gib_imlib_image_draw_rectangle(dst, x, y, w, h, 0, 0, 0, 255);
  D_RETURN_(5);
}

void
feh_menu_draw_submenu_at(int x,
                         int y,
                         int w,
                         int h,
                         Imlib_Image dst,
                         int ox,
                         int oy,
                         int selected)
{
  ImlibPolygon poly;

  D_ENTER(5);

  x -= ox;
  y -= oy;

  imlib_context_set_image(dst);
  poly = imlib_polygon_new();
  imlib_polygon_add_point(poly, x + 2, y + 5);
  imlib_polygon_add_point(poly, x + 5, y + 7);
  imlib_polygon_add_point(poly, x + 2, y + 11);
  imlib_context_set_color(0, 0, 0, 60);
  imlib_image_fill_polygon(poly);
  imlib_polygon_free(poly);

  poly = imlib_polygon_new();
  imlib_polygon_add_point(poly, x, y + 3);
  imlib_polygon_add_point(poly, x + 3, y + 6);
  imlib_polygon_add_point(poly, x, y + 9);
  imlib_context_set_color(0, 0, 0, 255);
  imlib_image_fill_polygon(poly);
  imlib_polygon_free(poly);

  D_RETURN_(5);
  selected = 0;
}


void
feh_menu_draw_separator_at(int x,
                           int y,
                           int w,
                           int h,
                           Imlib_Image dst,
                           int ox,
                           int oy)
{
  D_ENTER(5);
  gib_imlib_image_fill_rectangle(dst, x - ox + 2, y - oy + 2, w - 4, h - 4, 0,
                                 0, 0, 255);
  D_RETURN_(5);
}

void
feh_menu_item_draw_at(int x,
                      int y,
                      int w,
                      int h,
                      Imlib_Image dst,
                      int ox,
                      int oy,
                      int selected)
{
  D_ENTER(5);
  imlib_context_set_image(dst);
  if (selected)
    gib_imlib_image_fill_rectangle(dst, x - ox, y - oy, w, h, 255, 255, 255,
                                   178);
  D_RETURN_(5);
}


void
feh_raise_all_menus(void)
{
  feh_menu_list *l;

  D_ENTER(5);

  for (l = menus; l; l = l->next) {
    if (l->menu->visible)
      XRaiseWindow(disp, l->menu->win);
  }
  D_RETURN_(5);
}

void
feh_redraw_menus(void)
{
  feh_menu_list *l;

  D_ENTER(5);

  for (l = menus; l; l = l->next) {
    if (l->menu->needs_redraw)
      feh_menu_redraw(l->menu);
  }

  D_RETURN_(5);
}

feh_menu *
feh_menu_get_from_window(Window win)
{
  feh_menu_list *l;

  D_ENTER(5);
  for (l = menus; l; l = l->next)
    if (l->menu->win == win)
      D_RETURN(5, l->menu);
  D_RETURN(5, NULL);
}

void
feh_menu_init_main(void)
{
  feh_menu *m;
  feh_menu_item *mi;

  D_ENTER(4);
  if (!common_menus)
    feh_menu_init_common();

  menu_main = feh_menu_new();
  menu_main->name = estrdup("MAIN");

  feh_menu_add_entry(menu_main, "File", NULL, "FILE", NULL, NULL, NULL);
  if (opt.slideshow || opt.multiwindow) {
#if 0
    feh_menu_item *mi;

    mi =
      feh_menu_add_entry(menu_main, "Jump to", NULL, "JUMP", NULL, NULL,
                         NULL);
    mi->func_gen_sub = feh_menu_func_gen_jump;
#endif

    feh_menu_add_entry(menu_main, "Sort List", NULL, "SORT", NULL, NULL,
                       NULL);
    mi =
      feh_menu_add_entry(menu_main, "Image Info", NULL, "INFO", NULL, NULL,
                         NULL);
    mi->func_gen_sub = feh_menu_func_gen_info;
    feh_menu_add_entry(menu_main, NULL, NULL, NULL, NULL, NULL, NULL);
  }
  mi =
    feh_menu_add_entry(menu_main, "Options", NULL, "OPTIONS", NULL, NULL,
                       NULL);
  mi->func_gen_sub = feh_menu_func_gen_options;

  if (!opt.full_screen)
    feh_menu_add_entry(menu_main, "About " PACKAGE, NULL, NULL,
                       feh_menu_cb_about, NULL, NULL);
  if (opt.multiwindow)
    feh_menu_add_entry(menu_main, "Close", NULL, NULL, feh_menu_cb_close,
                       NULL, NULL);
  feh_menu_add_entry(menu_main, "Exit", NULL, NULL, feh_menu_cb_exit, NULL,
                     NULL);

  m = feh_menu_new();
  m->name = estrdup("FILE");
  feh_menu_add_entry(m, "Reset", NULL, NULL, feh_menu_cb_reset, NULL, NULL);
  feh_menu_add_entry(m, "Resize Window", NULL, NULL, feh_menu_cb_fit, NULL,
                     NULL);
  feh_menu_add_entry(m, "Reload", NULL, NULL, feh_menu_cb_reload, NULL, NULL);
  feh_menu_add_entry(m, "Save Image", NULL, NULL, feh_menu_cb_save_image,
                     NULL, NULL);
  feh_menu_add_entry(m, "Save List", NULL, NULL,
                     feh_menu_cb_save_filelist, NULL, NULL);
  feh_menu_add_entry(m, "Edit in Place", NULL, "EDIT", NULL, NULL, NULL);
  feh_menu_add_entry(m, "Background", NULL, "BACKGROUND", NULL, NULL, NULL);
  feh_menu_add_entry(m, NULL, NULL, NULL, NULL, NULL, NULL);
  feh_menu_add_entry(m, "Hide", NULL, NULL, feh_menu_cb_remove, NULL, NULL);
  feh_menu_add_entry(m, "Delete", NULL, "CONFIRM", NULL, NULL, NULL);

  D_RETURN_(4);
}


void
feh_menu_init_common()
{
  int num_desks, i;
  char buf[30];
  feh_menu *m;

  D_ENTER(4);

  if (!opt.menu_fn) {
    opt.menu_fn = gib_imlib_load_font(opt.menu_font);
    if (!opt.menu_fn)
      eprintf
        ("couldn't load menu font %s, did you make install?\nAre you specifying a nonexistant font?\nDid you tell feh where to find it with --fontpath?",
         opt.menu_font);
  }
  if (!opt.menu_style_l) {
    opt.menu_style_l = gib_style_new_from_ascii(opt.menu_style); 
    if (!opt.menu_style_l) {
      weprintf
        ("couldn't load style file for menu fonts, (%s).\nDid you make install? Menus will look boring without the style file.",
         opt.menu_style);
    }
  }

  m = feh_menu_new();
  m->name = estrdup("SORT");

  feh_menu_add_entry(m, "By File Name", NULL, NULL, feh_menu_cb_sort_filename,
                     NULL, NULL);
  feh_menu_add_entry(m, "By Image Name", NULL, NULL,
                     feh_menu_cb_sort_imagename, NULL, NULL);
  if (opt.preload || (opt.sort > SORT_FILENAME))
    feh_menu_add_entry(m, "By File Size", NULL, NULL,
                       feh_menu_cb_sort_filesize, NULL, NULL);
  feh_menu_add_entry(m, "Randomize", NULL, NULL, feh_menu_cb_sort_randomize,
                     NULL, NULL);

  m = feh_menu_new();
  m->name = estrdup("CONFIRM");
  feh_menu_add_entry(m, "Confirm", NULL, NULL, feh_menu_cb_delete, NULL,
                     NULL);

  m = feh_menu_new();
  m->name = estrdup("EDIT");
  feh_menu_add_entry(m, "Rotate 90 CW", NULL, NULL, feh_menu_cb_edit_rotate,
                     (void *) 1, NULL);
  feh_menu_add_entry(m, "Rotate 180", NULL, NULL, feh_menu_cb_edit_rotate,
                     (void *) 2, NULL);
  feh_menu_add_entry(m, "Rotate 90 CCW", NULL, NULL, feh_menu_cb_edit_rotate,
                     (void *) 3, NULL);

  menu_bg = feh_menu_new();
  menu_bg->name = estrdup("BACKGROUND");

  num_desks = feh_wm_get_num_desks();
  if (num_desks > 1) {
    feh_menu_add_entry(menu_bg, "Set Tiled", NULL, "TILED", NULL, NULL, NULL);
    feh_menu_add_entry(menu_bg, "Set Seamless", NULL, "SEAMLESS", NULL, NULL,
                       NULL);
    feh_menu_add_entry(menu_bg, "Set Scaled", NULL, "SCALED", NULL, NULL,
                       NULL);
    feh_menu_add_entry(menu_bg, "Set Centered", NULL, "CENTERED", NULL, NULL,
                       NULL);

    m = feh_menu_new();
    m->name = estrdup("TILED");
    for (i = 0; i < num_desks; i++) {
      snprintf(buf, sizeof(buf), "Desktop %d", i + 1);
      if (opt.slideshow || opt.multiwindow)
        feh_menu_add_entry(m, buf, NULL, NULL,
                           feh_menu_cb_background_set_tiled, (void *) i,
                           NULL);
      else
        feh_menu_add_entry(m, buf, NULL, NULL,
                           feh_menu_cb_background_set_tiled_no_file,
                           (void *) i, NULL);
    }

    m = feh_menu_new();
    m->name = estrdup("SEAMLESS");
    for (i = 0; i < num_desks; i++) {
      snprintf(buf, sizeof(buf), "Desktop %d", i + 1);
      feh_menu_add_entry(m, buf, NULL, NULL,
                         feh_menu_cb_background_set_seamless, (void *) i,
                         NULL);
    }


    m = feh_menu_new();
    m->name = estrdup("SCALED");
    for (i = 0; i < num_desks; i++) {
      snprintf(buf, sizeof(buf), "Desktop %d", i + 1);

      if (opt.slideshow || opt.multiwindow)
        feh_menu_add_entry(m, buf, NULL, NULL,
                           feh_menu_cb_background_set_scaled, (void *) i,
                           NULL);
      else
        feh_menu_add_entry(m, buf, NULL, NULL,
                           feh_menu_cb_background_set_scaled_no_file,
                           (void *) i, NULL);
    }

    m = feh_menu_new();
    m->name = estrdup("CENTERED");
    for (i = 0; i < num_desks; i++) {
      snprintf(buf, sizeof(buf), "Desktop %d", i + 1);
      if (opt.slideshow || opt.multiwindow)
        feh_menu_add_entry(m, buf, NULL, NULL,
                           feh_menu_cb_background_set_centered, (void *) i,
                           NULL);
      else
        feh_menu_add_entry(m, buf, NULL, NULL,
                           feh_menu_cb_background_set_centered_no_file,
                           (void *) i, NULL);
    }
  } else {
    if (opt.slideshow || opt.multiwindow) {
      feh_menu_add_entry(menu_bg, "Set Tiled", NULL, NULL,
                         feh_menu_cb_background_set_tiled, NULL, NULL);
      feh_menu_add_entry(menu_bg, "Set Seamless", NULL, NULL,
                         feh_menu_cb_background_set_seamless, NULL, NULL);
      feh_menu_add_entry(menu_bg, "Set Scaled", NULL, NULL,
                         feh_menu_cb_background_set_scaled, NULL, NULL);
      feh_menu_add_entry(menu_bg, "Set Centered", NULL, NULL,
                         feh_menu_cb_background_set_centered, NULL, NULL);
    } else {
      feh_menu_add_entry(menu_bg, "Set Tiled", NULL, NULL,
                         feh_menu_cb_background_set_tiled_no_file, NULL,
                         NULL);
      feh_menu_add_entry(menu_bg, "Set Seamless", NULL, NULL,
                         feh_menu_cb_background_set_seamless, NULL, NULL);
      feh_menu_add_entry(menu_bg, "Set Scaled", NULL, NULL,
                         feh_menu_cb_background_set_scaled_no_file, NULL,
                         NULL);
      feh_menu_add_entry(menu_bg, "Set Centered", NULL, NULL,
                         feh_menu_cb_background_set_centered_no_file, NULL,
                         NULL);
    }
  }
  common_menus = 1;

  D_RETURN_(4);
}

void
feh_menu_init_about_win(void)
{
  D_ENTER(4);

  menu_about_win = feh_menu_new();
  menu_about_win->name = estrdup("ABOUTWIN");

  feh_menu_add_entry(menu_about_win, "Close", NULL, NULL, feh_menu_cb_close,
                     NULL, NULL);
  feh_menu_add_entry(menu_about_win, "Exit", NULL, NULL, feh_menu_cb_exit,
                     NULL, NULL);

  D_RETURN_(4);
}

void
feh_menu_init_single_win(void)
{
  feh_menu *m;
  feh_menu_item *mi;

  D_ENTER(4);
  if (!common_menus)
    feh_menu_init_common();

  menu_single_win = feh_menu_new();
  menu_single_win->name = estrdup("SINGLEWIN");

  feh_menu_add_entry(menu_single_win, "File", NULL, "SINGLEWIN_FILE", NULL,
                     NULL, NULL);
  m = feh_menu_new();
  m->name = estrdup("SINGLEWIN_FILE");
  feh_menu_add_entry(m, "Reset", NULL, NULL, feh_menu_cb_reset, NULL, NULL);
  feh_menu_add_entry(m, "Resize Window", NULL, NULL, feh_menu_cb_fit, NULL,
                     NULL);
  feh_menu_add_entry(m, "Reload", NULL, NULL, feh_menu_cb_reload, NULL, NULL);
  feh_menu_add_entry(m, "Save Image", NULL, NULL, feh_menu_cb_save_image,
                     NULL, NULL);
  feh_menu_add_entry(m, "Save List", NULL, NULL,
                     feh_menu_cb_save_filelist, NULL, NULL);
  feh_menu_add_entry(m, "Edit in Place", NULL, "EDIT", NULL, NULL, NULL);
  feh_menu_add_entry(m, "Background", NULL, "BACKGROUND", NULL, NULL, NULL);
  if (opt.multiwindow || opt.slideshow) {
    feh_menu_add_entry(m, NULL, NULL, NULL, NULL, NULL, NULL);
    feh_menu_add_entry(m, "Hide", NULL, NULL, feh_menu_cb_remove, NULL, NULL);
    feh_menu_add_entry(m, "Delete", NULL, "CONFIRM", NULL, NULL, NULL);
  }

  mi =
    feh_menu_add_entry(menu_single_win, "Image Info", NULL, "INFO", NULL,
                       NULL, NULL);
  mi->func_gen_sub = feh_menu_func_gen_info;
  feh_menu_add_entry(menu_single_win, NULL, NULL, NULL, NULL, NULL, NULL);
  mi =
    feh_menu_add_entry(menu_single_win, "Options", NULL, "OPTIONS", NULL,
                       NULL, NULL);
  mi->func_gen_sub = feh_menu_func_gen_options;
  feh_menu_add_entry(menu_single_win, "About " PACKAGE, NULL, NULL,
                     feh_menu_cb_about, NULL, NULL);
  feh_menu_add_entry(menu_single_win, "Close", NULL, NULL, feh_menu_cb_close,
                     NULL, NULL);
  feh_menu_add_entry(menu_single_win, "Exit", NULL, NULL, feh_menu_cb_exit,
                     NULL, NULL);

  D_RETURN_(4);
}

void
feh_menu_init_thumbnail_win(void)
{
  feh_menu *m;
  feh_menu_item *mi;

  D_ENTER(4);
  if (!common_menus)
    feh_menu_init_common();

  menu_thumbnail_win = feh_menu_new();
  menu_thumbnail_win->name = estrdup("THUMBWIN");

  feh_menu_add_entry(menu_thumbnail_win, "File", NULL, "THUMBWIN_FILE", NULL,
                     NULL, NULL);
  m = feh_menu_new();
  m->name = estrdup("THUMBWIN_FILE");
  feh_menu_add_entry(m, "Reset", NULL, NULL, feh_menu_cb_reset, NULL, NULL);
  feh_menu_add_entry(m, "Resize Window", NULL, NULL, feh_menu_cb_fit, NULL,
                     NULL);
  feh_menu_add_entry(m, "Save Image", NULL, NULL, feh_menu_cb_save_image,
                     NULL, NULL);
  feh_menu_add_entry(m, "Save List", NULL, NULL,
                     feh_menu_cb_save_filelist, NULL, NULL);
  feh_menu_add_entry(m, "Background", NULL, "BACKGROUND", NULL, NULL, NULL);
  feh_menu_add_entry(menu_thumbnail_win, NULL, NULL, NULL, NULL, NULL, NULL);
  mi =
    feh_menu_add_entry(menu_thumbnail_win, "Options", NULL, "OPTIONS", NULL,
                       NULL, NULL);
  mi->func_gen_sub = feh_menu_func_gen_options;
  feh_menu_add_entry(menu_thumbnail_win, "About " PACKAGE, NULL, NULL,
                     feh_menu_cb_about, NULL, NULL);
  feh_menu_add_entry(menu_thumbnail_win, "Close", NULL, NULL,
                     feh_menu_cb_close, NULL, NULL);
  feh_menu_add_entry(menu_thumbnail_win, "Exit", NULL, NULL, feh_menu_cb_exit,
                     NULL, NULL);
  D_RETURN_(4);
}


void
feh_menu_init_thumbnail_viewer(void)
{
  feh_menu *m;
  feh_menu_item *mi;

  D_ENTER(4);
  if (!common_menus)
    feh_menu_init_common();

  menu_thumbnail_viewer = feh_menu_new();
  menu_thumbnail_viewer->name = estrdup("THUMBVIEW");

  feh_menu_add_entry(menu_thumbnail_viewer, "File", NULL, "THUMBVIEW_FILE",
                     NULL, NULL, NULL);
  m = feh_menu_new();
  m->name = estrdup("THUMBVIEW_FILE");
  feh_menu_add_entry(m, "Reset", NULL, NULL, feh_menu_cb_reset, NULL, NULL);
  feh_menu_add_entry(m, "Resize Window", NULL, NULL, feh_menu_cb_fit, NULL,
                     NULL);
  feh_menu_add_entry(m, "Reload", NULL, NULL, feh_menu_cb_reload, NULL, NULL);
  feh_menu_add_entry(m, "Save Image", NULL, NULL, feh_menu_cb_save_image,
                     NULL, NULL);
  feh_menu_add_entry(m, "Save List", NULL, NULL,
                     feh_menu_cb_save_filelist, NULL, NULL);
  feh_menu_add_entry(m, "Edit in Place", NULL, "EDIT", NULL, NULL, NULL);
  feh_menu_add_entry(m, "Background", NULL, "BACKGROUND", NULL, NULL, NULL);
  feh_menu_add_entry(m, NULL, NULL, NULL, NULL, NULL, NULL);
  feh_menu_add_entry(m, "Hide", NULL, NULL, feh_menu_cb_remove_thumb, NULL,
                     NULL);
  feh_menu_add_entry(m, "Delete", NULL, "THUMBVIEW_CONFIRM", NULL, NULL,
                     NULL);
  mi =
    feh_menu_add_entry(menu_thumbnail_viewer, "Image Info", NULL, "INFO",
                       NULL, NULL, NULL);
  mi->func_gen_sub = feh_menu_func_gen_info;
  feh_menu_add_entry(menu_thumbnail_viewer, NULL, NULL, NULL, NULL, NULL,
                     NULL);
  mi =
    feh_menu_add_entry(menu_thumbnail_viewer, "Options", NULL, "OPTIONS",
                       NULL, NULL, NULL);
  mi->func_gen_sub = feh_menu_func_gen_options;
  feh_menu_add_entry(menu_thumbnail_viewer, "About " PACKAGE, NULL, NULL,
                     feh_menu_cb_about, NULL, NULL);
  feh_menu_add_entry(menu_thumbnail_viewer, "Close", NULL, NULL,
                     feh_menu_cb_close, NULL, NULL);
  feh_menu_add_entry(menu_thumbnail_viewer, "Exit", NULL, NULL,
                     feh_menu_cb_exit, NULL, NULL);
  m = feh_menu_new();
  m->name = estrdup("THUMBVIEW_CONFIRM");
  feh_menu_add_entry(m, "Confirm", NULL, NULL, feh_menu_cb_delete_thumb, NULL,
                     NULL);
  D_RETURN_(4);
}

static void
feh_menu_cb_background_set_tiled(feh_menu * m,
                                 feh_menu_item * i,
                                 void *data)
{
  char *path;

  D_ENTER(4);
  path = feh_absolute_path(FEH_FILE(m->fehwin->file->data)->filename);
  feh_wm_set_bg(path, m->fehwin->im, 0, 0, (int) data, 1);
  free(path);
  D_RETURN_(4);
  i = NULL;
}

static void
feh_menu_cb_background_set_seamless(feh_menu * m,
                                    feh_menu_item * i,
                                    void *data)
{
  Imlib_Image im;

  D_ENTER(4);
  im = gib_imlib_clone_image(m->fehwin->im);
  gib_imlib_image_tile(im);
  feh_wm_set_bg(NULL, im, 0, 0, (int) data, 1);
  gib_imlib_free_image_and_decache(im);
  D_RETURN_(4);
  i = NULL;
}

static void
feh_menu_cb_background_set_scaled(feh_menu * m,
                                  feh_menu_item * i,
                                  void *data)
{
  char *path;

  D_ENTER(4);
  path = feh_absolute_path(FEH_FILE(m->fehwin->file->data)->filename);
  feh_wm_set_bg(path, m->fehwin->im, 0, 1, (int) data, 1);
  free(path);
  D_RETURN_(4);
  i = NULL;
}

static void
feh_menu_cb_background_set_centered(feh_menu * m,
                                    feh_menu_item * i,
                                    void *data)
{
  char *path;

  D_ENTER(4);
  path = feh_absolute_path(FEH_FILE(m->fehwin->file->data)->filename);
  feh_wm_set_bg(path, m->fehwin->im, 1, 0, (int) data, 1);
  free(path);
  D_RETURN_(4);
  i = NULL;
}

static void
feh_menu_cb_background_set_tiled_no_file(feh_menu * m,
                                         feh_menu_item * i,
                                         void *data)
{
  D_ENTER(4);
  feh_wm_set_bg(NULL, m->fehwin->im, 0, 0, (int) data, 1);
  D_RETURN_(4);
  i = NULL;
}

static void
feh_menu_cb_background_set_scaled_no_file(feh_menu * m,
                                          feh_menu_item * i,
                                          void *data)
{
  D_ENTER(4);
  feh_wm_set_bg(NULL, m->fehwin->im, 0, 1, (int) data, 1);
  D_RETURN_(4);
  i = NULL;
}

static void
feh_menu_cb_background_set_centered_no_file(feh_menu * m,
                                            feh_menu_item * i,
                                            void *data)
{
  D_ENTER(4);
  feh_wm_set_bg(NULL, m->fehwin->im, 1, 0, (int) data, 1);
  D_RETURN_(4);
  i = NULL;
}

static void
feh_menu_cb_about(feh_menu * m,
                  feh_menu_item * i,
                  void *data)
{
  Imlib_Image im;
  winwidget winwid;

  D_ENTER(4);
  if (feh_load_image_char(&im, PREFIX "/share/feh/images/about.png") != 0) {
    winwid =
      winwidget_create_from_image(im, "About " PACKAGE, WIN_TYPE_ABOUT);
    winwid->file =
      gib_list_add_front(NULL,
                         feh_file_new(PREFIX "/share/feh/images/about.png"));
    winwidget_show(winwid);
  }
  D_RETURN_(4);
  m = NULL;
  i = NULL;
  data = NULL;
}

static void
feh_menu_cb_close(feh_menu * m,
                  feh_menu_item * i,
                  void *data)
{
  D_ENTER(4);
  winwidget_destroy(m->fehwin);
  D_RETURN_(4);
  i = NULL;
  data = NULL;
}

static void
feh_menu_cb_exit(feh_menu * m,
                 feh_menu_item * i,
                 void *data)
{
  D_ENTER(4);
  winwidget_destroy_all();
  D_RETURN_(4);
  m = NULL;
  i = NULL;
  data = NULL;
}

static void
feh_menu_cb_reset(feh_menu * m,
                  feh_menu_item * i,
                  void *data)
{
  D_ENTER(4);
  if (m->fehwin->has_rotated) {
    m->fehwin->im_w = gib_imlib_image_get_width(m->fehwin->im);
    m->fehwin->im_h = gib_imlib_image_get_height(m->fehwin->im);
    winwidget_resize(m->fehwin, m->fehwin->im_w, m->fehwin->im_h);
  }
  winwidget_reset_image(m->fehwin);
  winwidget_render_image(m->fehwin, 1, 1);
  D_RETURN_(4);
  i = NULL;
  data = NULL;
}


static void
feh_menu_cb_reload(feh_menu * m,
                   feh_menu_item * i,
                   void *data)
{
  D_ENTER(4);
  feh_reload_image(m->fehwin, 0, 0);
  D_RETURN_(4);
  i = NULL;
  data = NULL;
}

static void
feh_menu_cb_remove(feh_menu * m,
                   feh_menu_item * i,
                   void *data)
{
  D_ENTER(4);
  feh_filelist_image_remove(m->fehwin, 0);
  D_RETURN_(4);
  i = NULL;
  data = NULL;
}

static void
feh_menu_cb_delete(feh_menu * m,
                   feh_menu_item * i,
                   void *data)
{
  D_ENTER(4);
  feh_filelist_image_remove(m->fehwin, 1);
  D_RETURN_(4);
  i = NULL;
  data = NULL;
}

static void
feh_menu_cb_remove_thumb(feh_menu * m,
                         feh_menu_item * i,
                         void *data)
{
  D_ENTER(4);
  feh_thumbnail_mark_removed(FEH_FILE(m->fehwin->file->data), 0);
  feh_filelist_image_remove(m->fehwin, 0);
  D_RETURN_(4);
  i = NULL;
  data = NULL;
}

static void
feh_menu_cb_delete_thumb(feh_menu * m,
                         feh_menu_item * i,
                         void *data)
{
  D_ENTER(4);
  feh_thumbnail_mark_removed(FEH_FILE(m->fehwin->file->data), 1);
  feh_filelist_image_remove(m->fehwin, 1);
  D_RETURN_(4);
  i = NULL;
  data = NULL;
}


static void
feh_menu_cb_sort_filename(feh_menu * m,
                          feh_menu_item * i,
                          void *data)
{
  D_ENTER(4);
  filelist = gib_list_sort(filelist, feh_cmp_filename);
  if(!opt.no_jump_on_resort){
    slideshow_change_image(m->fehwin, SLIDE_FIRST);
  };
  D_RETURN_(4);
  i = NULL;
  data = NULL;
}

static void
feh_menu_cb_sort_imagename(feh_menu * m,
                           feh_menu_item * i,
                           void *data)
{
  D_ENTER(4);
  filelist = gib_list_sort(filelist, feh_cmp_name);
  if(!opt.no_jump_on_resort){
    slideshow_change_image(m->fehwin, SLIDE_FIRST);
  };
  D_RETURN_(4);
  i = NULL;
  data = NULL;
}

static void
feh_menu_cb_sort_filesize(feh_menu * m,
                          feh_menu_item * i,
                          void *data)
{
  D_ENTER(4);
  filelist = gib_list_sort(filelist, feh_cmp_size);
  if(!opt.no_jump_on_resort){
    slideshow_change_image(m->fehwin, SLIDE_FIRST);
  };
  D_RETURN_(4);
  i = NULL;
  data = NULL;
}

static void
feh_menu_cb_sort_randomize(feh_menu * m,
                           feh_menu_item * i,
                           void *data)
{
  D_ENTER(4);
  filelist = gib_list_randomize(filelist);
  if(!opt.no_jump_on_resort){
    slideshow_change_image(m->fehwin, SLIDE_FIRST);
  };
  D_RETURN_(4);
  i = NULL;
  data = NULL;
}

static feh_menu *
feh_menu_func_gen_jump(feh_menu * m,
                       feh_menu_item * i,
                       void *data)
{
  feh_menu *mm;
  gib_list *l;

  D_ENTER(4);
  mm = feh_menu_new();
  mm->name = estrdup("JUMP");
  for (l = filelist; l; l = l->next) {
    feh_menu_add_entry(mm, FEH_FILE(l->data)->name, NULL, NULL,
                       feh_menu_cb_jump_to, l, NULL);
  }
  D_RETURN(4, mm);
  m = NULL;
  i = NULL;
  data = NULL;
}

static feh_menu *
feh_menu_func_gen_info(feh_menu * m,
                       feh_menu_item * i,
                       void *data)
{
  Imlib_Image im;
  feh_menu *mm;
  feh_file *file;
  char buffer[400];

  D_ENTER(4);
  if (!m->fehwin->file)
    D_RETURN(4, NULL);
  file = FEH_FILE(m->fehwin->file->data);
  im = m->fehwin->im;
  if (!im)
    D_RETURN(4, NULL);
  mm = feh_menu_new();
  mm->name = estrdup("INFO");
  snprintf(buffer, sizeof(buffer), "Filename: %s", file->name);
  feh_menu_add_entry(mm, buffer, NULL, NULL, NULL, NULL, NULL);
  if (!file->info)
    feh_file_info_load(file, im);
  if (file->info) {
    snprintf(buffer, sizeof(buffer), "Size: %dKb", file->info->size / 1024);
    feh_menu_add_entry(mm, buffer, NULL, NULL, NULL, NULL, NULL);
    snprintf(buffer, sizeof(buffer), "Dimensions: %dx%d", file->info->width,
             file->info->height);
    feh_menu_add_entry(mm, buffer, NULL, NULL, NULL, NULL, NULL);
    snprintf(buffer, sizeof(buffer), "Type: %s", file->info->format);
    feh_menu_add_entry(mm, buffer, NULL, NULL, NULL, NULL, NULL);
  }

  mm->func_free = feh_menu_func_free_info;
  D_RETURN(4, mm);
  i = NULL;
  data = NULL;
}

static void
feh_menu_func_free_info(feh_menu * m,
                        void *data)
{
  D_ENTER(4);
  feh_menu_free(m);
  D_RETURN_(4);
  data = NULL;
}


static feh_menu *
feh_menu_func_gen_options(feh_menu * m,
                          feh_menu_item * i,
                          void *data)
{
  feh_menu *mm;

  D_ENTER(4);
  mm = feh_menu_new();
  mm->name = estrdup("OPTIONS");
  mm->fehwin = m->fehwin;
  feh_menu_add_toggle_entry(mm, "Auto-Zoom", NULL, NULL,
                            feh_menu_cb_opt_auto_zoom, NULL, NULL,
                            opt.auto_zoom);
  feh_menu_add_toggle_entry(mm, "Freeze Window Size", NULL, NULL,
                            feh_menu_cb_opt_freeze_window, NULL, NULL,
                            opt.geom_flags);
  feh_menu_add_toggle_entry(mm, "Fullscreen", NULL, NULL,
                            feh_menu_cb_opt_fullscreen, NULL, NULL,
                            m->fehwin->full_screen);
#ifdef HAVE_LIBXINERAMA
  feh_menu_add_toggle_entry(mm, "Use Xinerama", NULL, NULL,
                            feh_menu_cb_opt_xinerama, NULL, NULL,
                            opt.xinerama);
#endif /* HAVE_LIBXINERAMA */
  feh_menu_add_entry(mm, NULL, NULL, NULL, NULL, NULL, NULL);
  feh_menu_add_toggle_entry(mm, "Draw Filename", NULL, NULL,
                            feh_menu_cb_opt_draw_filename, NULL, NULL,
                            opt.draw_filename);
  feh_menu_add_toggle_entry(mm, "Keep HTTP Files", NULL, NULL,
                            feh_menu_cb_opt_keep_http, NULL, NULL,
                            opt.keep_http);
  mm->func_free = feh_menu_func_free_options;
  D_RETURN(4, mm);
  i = NULL;
  data = NULL;
}

static void
feh_menu_func_free_options(feh_menu * m,
                           void *data)
{
  D_ENTER(4);
  feh_menu_free(m);
  D_RETURN_(4);
  data = NULL;
}

static void
feh_menu_cb_jump_to(feh_menu * m,
                    feh_menu_item * i,
                    void *data)
{
  gib_list *l;

  D_ENTER(4);
  l = (gib_list *) data;
  if (l->prev) {
    current_file = l->prev;
    slideshow_change_image(m->fehwin, SLIDE_NEXT);
  } else if (l->next) {
    current_file = l->next;
    slideshow_change_image(m->fehwin, SLIDE_PREV);
  }

  D_RETURN_(4);
  i = NULL;
  m = NULL;
}

static void
feh_menu_cb_fit(feh_menu * m,
                feh_menu_item * i,
                void *data)
{
  D_ENTER(4);
  winwidget_size_to_image(m->fehwin);
  D_RETURN_(4);
  data = NULL;
}

static void
feh_menu_cb_edit_rotate(feh_menu * m,
                        feh_menu_item * i,
                        void *data)
{
  D_ENTER(4);
  feh_edit_inplace_orient(m->fehwin, (int) data);
  D_RETURN_(4);
}

static void
feh_menu_cb_save_image(feh_menu * m,
                       feh_menu_item * i,
                       void *data)
{
  slideshow_save_image(m->fehwin);
}

static void
feh_menu_cb_save_filelist(feh_menu * m,
                          feh_menu_item * i,
                          void *data)
{
  feh_save_filelist();
}

static void
feh_menu_cb_opt_draw_filename(feh_menu * m,
                              feh_menu_item * i,
                              void *data)
{
  MENU_ITEM_TOGGLE(i);
  if (MENU_ITEM_IS_ON(i))
    opt.draw_filename = TRUE;
  else
    opt.draw_filename = FALSE;
  winwidget_rerender_all(0, 1);
}

static void
feh_menu_cb_opt_keep_http(feh_menu * m,
                          feh_menu_item * i,
                          void *data)
{
  MENU_ITEM_TOGGLE(i);
  if (MENU_ITEM_IS_ON(i))
    opt.keep_http = TRUE;
  else
    opt.keep_http = FALSE;
}

static void
feh_menu_cb_opt_freeze_window(feh_menu * m,
                              feh_menu_item * i,
                              void *data)
{
  MENU_ITEM_TOGGLE(i);
  if (MENU_ITEM_IS_ON(i)) {
    opt.geom_flags = (WidthValue | HeightValue);
    opt.geom_w = m->fehwin->w;
    opt.geom_h = m->fehwin->h;
  } else {
    opt.geom_flags = 0;
  }
}

static void
feh_menu_cb_opt_fullscreen(feh_menu * m,
                           feh_menu_item * i,
                           void *data)
{
  int curr_screen = 0;

  MENU_ITEM_TOGGLE(i);
  if (MENU_ITEM_IS_ON(i))
    m->fehwin->full_screen = TRUE;
  else
    m->fehwin->full_screen = FALSE;

#ifdef HAVE_LIBXINERAMA
  if (opt.xinerama && xinerama_screens) {
    int i, rect[4];

    /* FIXME: this doesn't do what it should;  XGetGeometry always
     * returns x,y == 0,0.  I think that's due to the hints being passed
     * (or more specifically, a missing hint) to X in winwidget_create
     */
    winwidget_get_geometry(m->fehwin, rect);
    /* printf("window: (%d, %d)\n", rect[0], rect[1]);
    printf("found %d screens.\n", num_xinerama_screens); */
    for (i = 0; i < num_xinerama_screens; i++) {
      xinerama_screen = 0;
      /* printf("%d: [%d, %d, %d, %d] (%d, %d)\n",
             i,
             xinerama_screens[i].x_org, xinerama_screens[i].y_org,
             xinerama_screens[i].width, xinerama_screens[i].height,
             rect[0], rect[1]);*/
      if (XY_IN_RECT(rect[0], rect[1],
            xinerama_screens[i].x_org, xinerama_screens[i].y_org,
            xinerama_screens[i].width, xinerama_screens[i].height)) {
        curr_screen = xinerama_screen = i;
        break;
      }

    }
  }
#endif /* HAVE_LIBXINERAMA */

  winwidget_destroy_xwin(m->fehwin);
  winwidget_create_window(m->fehwin, m->fehwin->im_w, m->fehwin->im_h);

  winwidget_render_image(m->fehwin, 1, 1);
  winwidget_show(m->fehwin);

#ifdef HAVE_LIBXINERAMA
  /* if we have xinerama and we're using it, then full screen the window
   * on the head that the window was active on */
  if (m->fehwin->full_screen == TRUE && opt.xinerama && xinerama_screens) {
    xinerama_screen = curr_screen;
    winwidget_move(m->fehwin,
                   xinerama_screens[curr_screen].x_org,
                   xinerama_screens[curr_screen].y_org);
  }
#endif /* HAVE_LIBXINERAMA */
}

static void
feh_menu_cb_opt_auto_zoom(feh_menu * m,
                          feh_menu_item * i,
                          void *data)
{
  MENU_ITEM_TOGGLE(i);
  opt.auto_zoom = MENU_ITEM_IS_ON(i) ? 1 : 0;
  winwidget_rerender_all(1, 1);
}

#ifdef HAVE_LIBXINERAMA
static void
feh_menu_cb_opt_xinerama(feh_menu * m,
                         feh_menu_item * i,
                         void *data)
{
  MENU_ITEM_TOGGLE(i);
  opt.xinerama = MENU_ITEM_IS_ON(i) ? 1 : 0;

  if (opt.xinerama) {
    init_xinerama();
  } else {
    XFree(xinerama_screens);
    xinerama_screens = NULL;
  }
  winwidget_rerender_all(1, 1);
}
#endif /* HAVE_LIBXINERAMA */
