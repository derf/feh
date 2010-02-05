/* keyevents.c

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
#include "thumbnail.h"
#include "filelist.h"
#include "winwidget.h"
#include "options.h"

void
feh_event_invoke_action(winwidget winwid, char *action)
{
  D_ENTER(4);
  D(4, ("action is '%s'\n", action));
  D(4, ("winwid is '%p'\n", winwid));
  if (action)
  {
     if (opt.slideshow)
     {
        feh_action_run(FEH_FILE(winwid->file->data),action);
        slideshow_change_image(winwid, SLIDE_NEXT);
     }
     else if ((winwid->type == WIN_TYPE_SINGLE)
              || (winwid->type == WIN_TYPE_THUMBNAIL_VIEWER))
     {
        feh_action_run(FEH_FILE(winwid->file->data),action);
        winwidget_destroy(winwid);
     }
     else if (winwid->type == WIN_TYPE_THUMBNAIL)
     {
       printf("actions from the main thumb window aren't currentl supported!\n");
       printf("For now, open the image to perform the action on it.\n");
     }
  }
  D_RETURN_(4);
}

void
feh_event_handle_keypress(XEvent * ev)
{
   int len;
   char kbuf[20];
   KeySym keysym;
   XKeyEvent *kev;
   winwidget winwid = NULL;
   int curr_screen = 0;
   feh_menu_item *selected_item;
   feh_menu *selected_menu;

   D_ENTER(4);

   winwid = winwidget_get_from_window(ev->xkey.window);
   
   /* nuke dupe events, unless we're typing text */
   if (winwid && !winwid->caption_entry) {
     while (XCheckTypedWindowEvent(disp, ev->xkey.window, KeyPress, ev));
   }

   kev = (XKeyEvent *) ev;
   len = XLookupString(&ev->xkey, (char *) kbuf, sizeof(kbuf), &keysym, NULL);

   /* menus are showing, so this is a menu control keypress */
   if (ev->xbutton.window == menu_cover) {
     selected_item = feh_menu_find_selected_r(menu_root, &selected_menu);
     switch (keysym) {
       case XK_Escape:
         feh_menu_hide(menu_root, True);
         break;
       case XK_Left:
         feh_menu_select_parent(selected_menu, selected_item);
         break;
       case XK_Down:
         feh_menu_select_next(selected_menu, selected_item);
         break;
       case XK_Up:
         feh_menu_select_prev(selected_menu, selected_item);
         break;
       case XK_Right:
         feh_menu_select_submenu(selected_menu, selected_item);
         break;
       case XK_Return:
         feh_menu_item_activate(selected_menu, selected_item);
         break;
       default:
         break;
     }
     if (len <= 0 || len > (int) sizeof(kbuf))
        D_RETURN_(4);
     kbuf[len] = '\0';

     switch (*kbuf)
     {
       case 'h':
         feh_menu_select_parent(selected_menu, selected_item);
         break;
       case 'j':
         feh_menu_select_next(selected_menu, selected_item);
         break;
       case 'k':
         feh_menu_select_prev(selected_menu, selected_item);
         break;
       case 'l':
         feh_menu_select_submenu(selected_menu, selected_item);
         break;
       case ' ':
         feh_menu_item_activate(selected_menu, selected_item);
         break;
       default:
         break;
     }
     
     D_RETURN_(4);
   }

   if (winwid == NULL)
      D_RETURN_(4);

   if (winwid->caption_entry) {
     switch(keysym) {
       case XK_Return:
         if (kev->state & ControlMask) {
           /* insert actual newline */
           ESTRAPPEND(FEH_FILE(winwid->file->data)->caption, "\n");
           winwidget_render_image_cached(winwid);
         } else {
           /* finish caption entry, write to captions file */
           FILE *fp;
           char *caption_filename;
           caption_filename = build_caption_filename(FEH_FILE(winwid->file->data));
           winwid->caption_entry = 0;
           winwidget_render_image_cached(winwid);
           XFreePixmap(disp, winwid->bg_pmap_cache);
           winwid->bg_pmap_cache = 0;
           fp = fopen(caption_filename, "w");
           if (!fp) {
             weprintf("couldn't write to captions file %s:", caption_filename);
             D_RETURN_(4);
           }
           fprintf(fp, "%s", FEH_FILE(winwid->file->data)->caption);
           free(caption_filename);
           fclose(fp);
         }
         break;
       case XK_Escape:
         /* cancel, revert caption */
         winwid->caption_entry = 0;
         free(FEH_FILE(winwid->file->data)->caption);
         FEH_FILE(winwid->file->data)->caption = NULL;
         winwidget_render_image_cached(winwid);
         XFreePixmap(disp, winwid->bg_pmap_cache);
         winwid->bg_pmap_cache = 0;
         break;
       case XK_BackSpace:
         /* backspace */
         ESTRTRUNC(FEH_FILE(winwid->file->data)->caption, 1);
         winwidget_render_image_cached(winwid);
         break;
       default:
         if(isascii(keysym)) {
           /* append to caption */
           ESTRAPPEND_CHAR(FEH_FILE(winwid->file->data)->caption, keysym);
           winwidget_render_image_cached(winwid);
         }
         break;
     }
     D_RETURN_(4);
   }
   
   
   switch (keysym)
   {
     case XK_Left:
        if (opt.slideshow)
           slideshow_change_image(winwid, SLIDE_PREV);
        break;
     case XK_Right:
        if (opt.slideshow)
           slideshow_change_image(winwid, SLIDE_NEXT);
        break;
     case XK_Page_Up:
        if (opt.slideshow)
           slideshow_change_image(winwid, SLIDE_JUMP_BACK);
        break;
     case XK_Escape:
        winwidget_destroy_all();
        break;
     case XK_Page_Down:
        if (opt.slideshow)
           slideshow_change_image(winwid, SLIDE_JUMP_FWD);
        break;
     case XK_Delete:
        /* Holding ctrl gets you a filesystem deletion and removal from the * 
           filelist. Just DEL gets you filelist removal only. */
        if (kev->state & ControlMask)
        {
           if (winwid->type == WIN_TYPE_THUMBNAIL_VIEWER)
              feh_thumbnail_mark_removed(FEH_FILE(winwid->file->data), 1);
           feh_filelist_image_remove(winwid, 1);
        }
        else
        {
           if (winwid->type == WIN_TYPE_THUMBNAIL_VIEWER)
              feh_thumbnail_mark_removed(FEH_FILE(winwid->file->data), 0);
           feh_filelist_image_remove(winwid, 0);
        }
        break;
     case XK_Home:
     case XK_KP_Home:
        if (opt.slideshow)
           slideshow_change_image(winwid, SLIDE_FIRST);
        break;
     case XK_End:
     case XK_KP_End:
        if (opt.slideshow)
           slideshow_change_image(winwid, SLIDE_LAST);
        break;
     case XK_Tab:
	if (opt.draw_actions) 
        {
	   opt.draw_actions = 0;
	   winwidget_rerender_all(0, 1);
        }
	else
        {
	   opt.draw_actions = 1;
	   winwidget_rerender_all(0, 1);
        }
	break;
     case XK_Return:
         feh_event_invoke_action(winwid,opt.actions[0]);
         break;
     case XK_0:
         feh_event_invoke_action(winwid,opt.actions[0]);
         break;
     case XK_1:
         feh_event_invoke_action(winwid,opt.actions[1]);
         break;
     case XK_2:
         feh_event_invoke_action(winwid,opt.actions[2]);
         break;
     case XK_3:
         feh_event_invoke_action(winwid,opt.actions[3]);
         break;
     case XK_4:
         feh_event_invoke_action(winwid,opt.actions[4]);
         break;
     case XK_5:
         feh_event_invoke_action(winwid,opt.actions[5]);
         break;
     case XK_6:
         feh_event_invoke_action(winwid,opt.actions[6]);
         break;
     case XK_7:
         feh_event_invoke_action(winwid,opt.actions[7]);
         break;
     case XK_8:
         feh_event_invoke_action(winwid,opt.actions[8]);
         break;
     case XK_9:
         feh_event_invoke_action(winwid,opt.actions[9]);
         break;
     case XK_KP_Left:
        winwid->im_x = winwid->im_x - 10;
        winwidget_render_image(winwid, 0, 0);
        break;
     case XK_KP_Right:
        winwid->im_x = winwid->im_x + 10;
        winwidget_render_image(winwid, 0, 0);
        break;
     case XK_KP_Up:
        winwid->im_y = winwid->im_y - 10;
        winwidget_render_image(winwid, 0, 0);
        break;
     case XK_KP_Down:
        winwid->im_y = winwid->im_y + 10;
        winwidget_render_image(winwid, 0, 0);
        break;
     case XK_KP_Add:
        winwid->zoom = winwid->zoom * 1.25;
        winwidget_render_image(winwid, 0, 0);
        break;
     case XK_KP_Subtract:
        winwid->zoom = winwid->zoom * 0.75;
        winwidget_render_image(winwid, 0, 0);
	break;
     case XK_KP_Multiply:
        winwid->zoom = 1;
        winwidget_render_image(winwid, 0, 0);
	break;
     case XK_KP_Divide:
        feh_calc_needed_zoom(&winwid->zoom, winwid->im_w, winwid->im_h, winwid->w, winwid->h);
        winwidget_render_image(winwid, 0, 0);
	break;
     default:
        break;
   }

   if (len <= 0 || len > (int) sizeof(kbuf))
      D_RETURN_(4);
   kbuf[len] = '\0';

   switch (*kbuf)
   {
     case 'n':
     case 'N':
     case ' ':
        if (opt.slideshow)
           slideshow_change_image(winwid, SLIDE_NEXT);
        break;
     case 'p':
     case 'P':
     case '\b':
        if (opt.slideshow)
           slideshow_change_image(winwid, SLIDE_PREV);
        break;
     case 'q':
     case 'Q':
        winwidget_destroy_all();
        break;
     case 'c':
     case 'C':
        if (opt.caption_path)
          winwid->caption_entry = 1;
          winwidget_render_image(winwid, 0, 0);
        break;
     case 'r':
     case 'R':
        feh_reload_image(winwid, 0, 0);
        break;
     case 'h':
     case 'H':
        slideshow_pause_toggle(winwid);
        break;
     case 's':
     case 'S':
        slideshow_save_image(winwid);
        break;
     case 'f':
     case 'F':
        feh_save_filelist();
        break;
     case 'w':
     case 'W':
        winwidget_size_to_image(winwid);
        break;
     case 'm':
     case 'M':
        winwidget_show_menu(winwid);
        break;
     case 'x':
     case 'X':
        winwidget_destroy(winwid);
        break;
     case '>':
        feh_edit_inplace_orient(winwid, 1);
        break;
     case '<':
        feh_edit_inplace_orient(winwid, 3);
        break;
     case 'v':
     case 'V':
#ifdef HAVE_LIBXINERAMA
        if (opt.xinerama && xinerama_screens) {
          int i, rect[4];

          /* FIXME: this doesn't do what it should;  XGetGeometry always
           * returns x,y == 0,0.  I think that's due to the hints being passed
           * (or more specifically, a missing hint) to X in winwidget_create
           */
          winwidget_get_geometry(winwid, rect);
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
        winwid->full_screen = !winwid->full_screen;
        winwidget_destroy_xwin(winwid);
        winwidget_create_window(winwid, winwid->im_w, winwid->im_h);
        winwidget_render_image(winwid, 1, 1);
        winwidget_show(winwid);
#ifdef HAVE_LIBXINERAMA
        /* if we have xinerama and we're using it, then full screen the window
         * on the head that the window was active on */
        if (winwid->full_screen == TRUE &&
            opt.xinerama && xinerama_screens) {
          xinerama_screen = curr_screen;
          winwidget_move(winwid,
                         xinerama_screens[curr_screen].x_org,
                         xinerama_screens[curr_screen].y_org);
        }
#endif /* HAVE_LIBXINERAMA */
	   case '=':
     case '+':
        if (opt.reload < SLIDESHOW_RELOAD_MAX)
           opt.reload++;
        else if (opt.verbose)
           weprintf("Cannot set RELOAD higher than %d seconds.", opt.reload);
        break;
     case '-':
     case '_':
        if (opt.reload > 1)
           opt.reload--;
        else if (opt.verbose)
           weprintf("Cannot set RELOAD lower than 1 second.");
        break;
     default:
        break;
   }
   D_RETURN_(4);
}
