/* support.c

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
#include "options.h"
#include "support.h"
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

void
feh_wm_set_bg_file(char *file, unsigned char bgmode)
{
   Imlib_Image im;
   feh_file *fil;

   fil = feh_file_new(file);
   if (fil)
   {
      if (feh_load_image(&im, fil) == 0)
         eprintf("Couldn't load image in order to set bg");
      switch (bgmode)
      {
        case BG_MODE_SEAMLESS:
           gib_imlib_image_tile(im);
           feh_wm_set_bg(NULL, im, 0, 0, 0, 1);
           break;
        case BG_MODE_TILE:
           feh_wm_set_bg(file, im, 0, 0, 0, 1);
           break;
        case BG_MODE_SCALE:
           feh_wm_set_bg(file, im, 0, 1, 0, 1);
           break;
        default:
           feh_wm_set_bg(file, im, 1, 0, 0, 1);
           break;
      }
      gib_imlib_free_image_and_decache(im);
      feh_file_free(fil);
   }
}

void
feh_wm_set_bg(char *fil, Imlib_Image im, int centered, int scaled,
              int desktop, int set)
{
   char bgname[20];
   int num = (int) rand();
   char bgfil[4096];
   char sendbuf[4096];

   D_ENTER(4);

   snprintf(bgname, sizeof(bgname), "FEHBG_%d", num);

   if (!fil)
   {
      snprintf(bgfil, sizeof(bgfil), "%s/.%s.png", getenv("HOME"), bgname);
      imlib_context_set_image(im);
      imlib_image_set_format("png");
      gib_imlib_save_image(im, bgfil);
      D(3, ("bg saved as %s\n", bgfil));
      fil = bgfil;
   }
   D(3, ("Setting bg %s\n", fil));

   if (feh_wm_get_wm_is_e() && (enl_ipc_get_win() != None))
   {
      snprintf(sendbuf, sizeof(sendbuf), "background %s bg.file %s", bgname,
               fil);
      enl_ipc_send(sendbuf);

      if (scaled)
      {
         snprintf(sendbuf, sizeof(sendbuf), "background %s bg.solid 0 0 0",
                  bgname);
         enl_ipc_send(sendbuf);
         snprintf(sendbuf, sizeof(sendbuf), "background %s bg.tile 0",
                  bgname);
         enl_ipc_send(sendbuf);
         snprintf(sendbuf, sizeof(sendbuf), "background %s bg.xjust 512",
                  bgname);
         enl_ipc_send(sendbuf);
         snprintf(sendbuf, sizeof(sendbuf), "background %s bg.yjust 512",
                  bgname);
         enl_ipc_send(sendbuf);
         snprintf(sendbuf, sizeof(sendbuf), "background %s bg.xperc 1024",
                  bgname);
         enl_ipc_send(sendbuf);
         snprintf(sendbuf, sizeof(sendbuf), "background %s bg.yperc 1024",
                  bgname);
         enl_ipc_send(sendbuf);
      }
      else if (centered)
      {
         snprintf(sendbuf, sizeof(sendbuf), "background %s bg.solid 0 0 0",
                  bgname);
         enl_ipc_send(sendbuf);
         snprintf(sendbuf, sizeof(sendbuf), "background %s bg.tile 0",
                  bgname);
         enl_ipc_send(sendbuf);
         snprintf(sendbuf, sizeof(sendbuf), "background %s bg.xjust 512",
                  bgname);
         enl_ipc_send(sendbuf);
         snprintf(sendbuf, sizeof(sendbuf), "background %s bg.yjust 512",
                  bgname);
         enl_ipc_send(sendbuf);
      }
      else
      {
         snprintf(sendbuf, sizeof(sendbuf), "background %s bg.tile 1",
                  bgname);
         enl_ipc_send(sendbuf);
      }

      if (set)
      {
         snprintf(sendbuf, sizeof(sendbuf), "use_bg %s %d", bgname, desktop);
         enl_ipc_send(sendbuf);
      }
      enl_ipc_sync();
   }
   else
   {
      Atom prop_root, prop_esetroot, type;
      int format;
      unsigned long length, after;
      unsigned char *data_root, *data_esetroot;
      Pixmap pmap_d1, pmap_d2;
      /* string for sticking in ~/.fehbg */
      char *fehbg = NULL;
      char *home;

      /* local display to set closedownmode on */
      Display *disp2;
      Window root2;
      int depth2;
      XGCValues gcvalues;
      GC gc;
      int w, h;

      D(3, ("Falling back to XSetRootWindowPixmap\n"));
      
      if (scaled) {
         w = scr->width;
         h = scr->height;

/* disable xinerama check for setting background */
#if 0
/* #ifdef HAVE_LIBXINERAMA */
         if (opt.xinerama && xinerama_screens) {
           w = xinerama_screens[xinerama_screen].width;
           h = xinerama_screens[xinerama_screen].height;
         }
#endif /* HAVE_LIBXINERAMA */

         pmap_d1 = XCreatePixmap(disp, root, w, h, depth);
         gib_imlib_render_image_on_drawable_at_size(pmap_d1, im, 0, 0, w, h,
                                                    1, 0, 1);
         fehbg = estrjoin(" ", "feh --bg-scale", fil, NULL);
      } else if (centered) {
         XGCValues gcval;
         GC gc;
         int x, y;

         D(3, ("centering\n"));
         w = scr->width;
         h = scr->height;

/* disable xinerama check for setting background */
#if 0
/* #ifdef HAVE_LIBXINERAMA */
         if (opt.xinerama && xinerama_screens) {
           w = xinerama_screens[xinerama_screen].width;
           h = xinerama_screens[xinerama_screen].height;
         }
#endif /* HAVE_LIBXINERAMA */

         pmap_d1 = XCreatePixmap(disp, root, w, h, depth);
         gcval.foreground = BlackPixel(disp, DefaultScreen(disp));
         gc = XCreateGC(disp, root, GCForeground, &gcval);
         XFillRectangle(disp, pmap_d1, gc, 0, 0, w, h);
         x = (w - gib_imlib_image_get_width(im)) >> 1;
         y = (h - gib_imlib_image_get_height(im)) >> 1;
         gib_imlib_render_image_on_drawable(pmap_d1, im, x, y, 1, 0, 0);
         XFreeGC(disp, gc);
         fehbg = estrjoin(" ", "feh --bg-center", fil, NULL);
      } else {
         w = gib_imlib_image_get_width(im);
         h = gib_imlib_image_get_height(im);
         pmap_d1 =
            XCreatePixmap(disp, root, w, h, depth);
         gib_imlib_render_image_on_drawable(pmap_d1, im, 0, 0, 1, 0, 0);
         fehbg = estrjoin(" ", "feh --bg-tile", fil, NULL);
      }

      if (fehbg) {
        home = getenv("HOME");
        if(home) {
          FILE *fp;
          char *path;
          path = estrjoin("/", home, ".fehbg", NULL);
          if ((fp = fopen(path, "w")) == NULL) {
	     wprintf("Can't open %s for write", path);
          } else {
             fprintf(fp, "%s\n", fehbg);
             fclose(fp);
          }
          free(path);
        }
        free(fehbg);
      }
      
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
      gc = XCreateGC(disp2, pmap_d2, GCFillStyle|GCTile, &gcvalues);
      XFillRectangle(disp2, pmap_d2, gc, 0, 0, scr->width, scr->height);
      XFreeGC(disp2, gc);
      XSync(disp2, False);
      XSync(disp, False);
      XFreePixmap(disp, pmap_d1);
      
      prop_root = XInternAtom(disp2, "_XROOTPMAP_ID", True);
      prop_esetroot = XInternAtom(disp2, "ESETROOT_PMAP_ID", True);

      if (prop_root != None && prop_esetroot != None)
      {
         XGetWindowProperty(disp2, root2, prop_root, 0L, 1L, False,
                            AnyPropertyType, &type, &format, &length, &after,
                            &data_root);
         if (type == XA_PIXMAP)
         {
            XGetWindowProperty(disp2, root2, prop_esetroot, 0L, 1L, False,
                               AnyPropertyType, &type, &format, &length,
                               &after, &data_esetroot);
            if (data_root && data_esetroot)
            {
               if (type == XA_PIXMAP
                   && *((Pixmap *) data_root) == *((Pixmap *) data_esetroot))
               {
                  XKillClient(disp2, *((Pixmap *) data_root));
               }
            }
         }
      }
      /* This will locate the property, creating it if it doesn't exist */
      prop_root = XInternAtom(disp2, "_XROOTPMAP_ID", False);
      prop_esetroot = XInternAtom(disp2, "ESETROOT_PMAP_ID", False);

      if (prop_root == None || prop_esetroot == None)
         weprintf("creation of pixmap property failed.");

      XChangeProperty(disp2, root2, prop_root, XA_PIXMAP, 32, PropModeReplace,
                      (unsigned char *) &pmap_d2, 1);
      XChangeProperty(disp2, root2, prop_esetroot, XA_PIXMAP, 32,
                      PropModeReplace, (unsigned char *) &pmap_d2, 1);

      XSetWindowBackgroundPixmap(disp2, root2, pmap_d2);
      XClearWindow(disp2, root2);
      XFlush(disp2);
      XSetCloseDownMode(disp2, RetainPermanent);
      XCloseDisplay(disp2);
   }
   D_RETURN_(4);
}

signed char
feh_wm_get_wm_is_e(void)
{
   static signed char e = -1;

   D_ENTER(4);

   /* check if E is actually running */
   if (e == -1)
   {
      /* XXX: This only covers E17 prior to 6/22/05 */
      if ((XInternAtom(disp, "ENLIGHTENMENT_COMMS", True) != None) &&
          (XInternAtom(disp, "ENLIGHTENMENT_VERSION", True) != None))
      {
         D(3, ("Enlightenment detected.\n"));
         e = 1;
      }
      else
      {
         D(3, ("Enlightenment not detected.\n"));
         e = 0;
      }
   }
   D_RETURN(4, e);
}

int
feh_wm_get_num_desks(void)
{
   char *buf, *ptr;
   int desks;

   D_ENTER(4);

   if (!feh_wm_get_wm_is_e())
      D_RETURN(4, -1);

   buf = enl_send_and_wait("num_desks ?");
   if (buf == IPC_FAKE)         /* Fake E17 IPC */
       D_RETURN(4, -1);
   D(3, ("Got from E IPC: %s\n", buf));
   ptr = buf;
   while (ptr && !isdigit(*ptr))
      ptr++;
   desks = atoi(ptr);

D_RETURN(4, desks)}

Window
enl_ipc_get_win(void)
{

   unsigned char *str = NULL;
   Atom prop, prop2, ever;
   unsigned long num, after;
   int format;
   Window dummy_win;
   int dummy_int;
   unsigned int dummy_uint;

   D_ENTER(4);

   D(3, ("Searching for IPC window.\n"));

   /*
    * Shortcircuit this entire func
    * if we already know it's an e17 fake
    */
   if (e17_fake_ipc)
     D_RETURN(4, ipc_win)
   
   prop = XInternAtom(disp, "ENLIGHTENMENT_COMMS", True);
   if (prop == None)
   {
      D(3, ("Enlightenment is not running.\n"));
      D_RETURN(4, None);
   } else {
     /* XXX: This will only work with E17 prior to 6/22/2005 */
     ever = XInternAtom(disp, "ENLIGHTENMENT_VERSION", True);
     if (ever == None) {
       /* This is an E without ENLIGHTENMENT_VERSION */
       D(3, ("E16 IPC Protocol not supported"));
       D_RETURN(4, None);
     }
   }
   XGetWindowProperty(disp, root, prop, 0, 14, False, AnyPropertyType, &prop2,
                      &format, &num, &after, &str);
   if (str)
   {
      sscanf((char *) str, "%*s %x", (unsigned int *) &ipc_win);
      XFree(str);
   }
   if (ipc_win != None)
   {
      if (!XGetGeometry
          (disp, ipc_win, &dummy_win, &dummy_int, &dummy_int, &dummy_uint,
           &dummy_uint, &dummy_uint, &dummy_uint))
      {
         D(3,
           (" -> IPC Window property is valid, but the window doesn't exist.\n"));
         ipc_win = None;
      }
      str = NULL;
      if (ipc_win != None)
      {
         XGetWindowProperty(disp, ipc_win, prop, 0, 14, False,
                            AnyPropertyType, &prop2, &format, &num, &after,
                            &str);
         if (str)
         {
            XFree(str);
         }
         else
         {
            D(3,
              (" -> IPC Window lacks the proper atom.  I can't talk to fake IPC windows....\n"));
            ipc_win = None;
         }
      }
   }
   if (ipc_win != None)
   {

      XGetWindowProperty(disp, ipc_win, ever, 0, 14, False, AnyPropertyType,
                        &prop2, &format, &num, &after, &str);
      if (str) {
        /*
         * This is E17's way of telling us it's only pretending
         * as a workaround for a bug related to the way java handles
         * Window Managers.
         * (Only valid after date of this comment)
         * -- richlowe 2005-06-22
         */
        XFree(str);
        D(3,(" -> Found a fake E17 IPC window, ignoring"));
        ipc_win = None;
        e17_fake_ipc = 1;
        D_RETURN(4, ipc_win);
      }
       
      D(3,
        (" -> IPC Window found and verified as 0x%08x.  Registering feh as an IPC client.\n",
         (int) ipc_win));
      XSelectInput(disp, ipc_win,
                   StructureNotifyMask | SubstructureNotifyMask);
      enl_ipc_send("set clientname " PACKAGE);
      enl_ipc_send("set version " VERSION);
      enl_ipc_send("set email tom@linuxbrit.co.uk");
      enl_ipc_send("set web http://www.linuxbrit.co.uk");
      enl_ipc_send("set info Feh - be pr0n or be dead");
   }
   if (my_ipc_win == None)
   {
      my_ipc_win = XCreateSimpleWindow(disp, root, -2, -2, 1, 1, 0, 0, 0);
   }
   D_RETURN(4, ipc_win);
}

void
enl_ipc_send(char *str)
{

   static char *last_msg = NULL;
   char buff[21];
   register unsigned short i;
   register unsigned char j;
   unsigned short len;
   XEvent ev;

   D_ENTER(4);
   if (str == NULL)
   {
      if (last_msg == NULL)
         eprintf("eeek");
      str = last_msg;
      D(4, ("Resending last message \"%s\" to Enlightenment.\n", str));
   }
   else
   {
      if (last_msg != NULL)
      {
         free(last_msg);
      }
      last_msg = estrdup(str);
      D(4, ("Sending \"%s\" to Enlightenment.\n", str));
   }
   if (ipc_win == None)
   {
      if ((ipc_win = enl_ipc_get_win()) == None)
      {
         D(3,
           ("Hrm. Enlightenment doesn't seem to be running. No IPC window, no IPC.\n"));
         D_RETURN_(4);
      }
   }
   len = strlen(str);
   ipc_atom = XInternAtom(disp, "ENL_MSG", False);
   if (ipc_atom == None)
   {
      D(3, ("IPC error:  Unable to find/create ENL_MSG atom.\n"));
      D_RETURN_(4);
   }
   for (; XCheckTypedWindowEvent(disp, my_ipc_win, ClientMessage, &ev););	/* Discard any out-of-sync messages */
   ev.xclient.type = ClientMessage;
   ev.xclient.serial = 0;
   ev.xclient.send_event = True;
   ev.xclient.window = ipc_win;
   ev.xclient.message_type = ipc_atom;
   ev.xclient.format = 8;

   for (i = 0; i < len + 1; i += 12)
   {
      sprintf(buff, "%8x", (int) my_ipc_win);
      for (j = 0; j < 12; j++)
      {
         buff[8 + j] = str[i + j];
         if (!str[i + j])
         {
            break;
         }
      }
      buff[20] = 0;
      for (j = 0; j < 20; j++)
      {
         ev.xclient.data.b[j] = buff[j];
      }
      XSendEvent(disp, ipc_win, False, 0, (XEvent *) & ev);
   }
   D_RETURN_(4);
}

static sighandler_t *
enl_ipc_timeout(int sig)
{
   timeout = 1;
   D_RETURN(4, (sighandler_t *) sig);
   sig = 0;
}

char *
enl_wait_for_reply(void)
{

   XEvent ev;
   static char msg_buffer[20];
   register unsigned char i;

   D_ENTER(4);

   alarm(2);
   for (;
        !XCheckTypedWindowEvent(disp, my_ipc_win, ClientMessage, &ev)
        && !timeout;);
   alarm(0);
   if (ev.xany.type != ClientMessage)
   {
      D_RETURN(4, IPC_TIMEOUT);
   }
   for (i = 0; i < 20; i++)
   {
      msg_buffer[i] = ev.xclient.data.b[i];
   }
   D_RETURN(4, msg_buffer + 8);
}

char *
enl_ipc_get(const char *msg_data)
{

   static char *message = NULL;
   static unsigned short len = 0;
   char buff[13], *ret_msg = NULL;
   register unsigned char i;
   unsigned char blen;

   D_ENTER(4);

   if (msg_data == IPC_TIMEOUT)
   {
      D_RETURN(4, IPC_TIMEOUT);
   }
   for (i = 0; i < 12; i++)
   {
      buff[i] = msg_data[i];
   }
   buff[12] = 0;
   blen = strlen(buff);
   if (message != NULL)
   {
      len += blen;
      message = (char *) erealloc(message, len + 1);
      strcat(message, buff);
   }
   else
   {
      len = blen;
      message = (char *) emalloc(len + 1);
      strcpy(message, buff);
   }
   if (blen < 12)
   {
      ret_msg = message;
      message = NULL;
      D(4, ("Received complete reply:  \"%s\"\n", ret_msg));
   }
   D_RETURN(4, ret_msg);
}

char *
enl_send_and_wait(char *msg)
{
   char *reply = IPC_TIMEOUT;
   sighandler_t old_alrm;

   D_ENTER(4);

   /*
    * Shortcut this func and return IPC_FAKE
    * If the IPC Window is the E17 fake
    */
   if (e17_fake_ipc)
     return IPC_FAKE;
   
   if (ipc_win == None)
   {
      /* The IPC window is missing.  Wait for it to return or feh to be killed. */
      /* Only called once in the E17 case */
      for (; enl_ipc_get_win() == None;)
      {
         if (e17_fake_ipc)      
           return IPC_FAKE;
         else
           sleep(1);
      }
   }
   old_alrm = (sighandler_t) signal(SIGALRM, (sighandler_t) enl_ipc_timeout);
   for (; reply == IPC_TIMEOUT;)
   {
      timeout = 0;
      enl_ipc_send(msg);
      for (; !(reply = enl_ipc_get(enl_wait_for_reply())););
      if (reply == IPC_TIMEOUT)
      {
         /* We timed out.  The IPC window must be AWOL.  Reset and resend message. */
         D(3, ("IPC timed out.  IPC window has gone. Clearing ipc_win.\n"));
         XSelectInput(disp, ipc_win, None);
         ipc_win = None;
      }
   }
   signal(SIGALRM, old_alrm);
   D_RETURN(4, reply);
}
