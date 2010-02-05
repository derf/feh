/* imlib.c

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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <jpeglib.h>

#include "transupp.h"

Display *disp = NULL;
Visual *vis = NULL;
Screen *scr = NULL;
Colormap cm;
int depth;
Atom wmDeleteWindow;
XContext xid_context = 0;
Window root = 0;

/* Xinerama support */
#ifdef HAVE_LIBXINERAMA
XineramaScreenInfo *xinerama_screens = NULL;
int xinerama_screen;
int num_xinerama_screens;
#endif /* HAVE_LIBXINERAMA */

#ifdef HAVE_LIBXINERAMA
void
init_xinerama(void)
{
  if (opt.xinerama && XineramaIsActive(disp)) {
    int major, minor;
    xinerama_screen = 0;
    XineramaQueryVersion(disp, &major, &minor);
    xinerama_screens = XineramaQueryScreens(disp, &num_xinerama_screens);
  }
}
#endif /* HAVE_LIBXINERAMA */

void
init_x_and_imlib(void)
{
   D_ENTER(4);

   disp = XOpenDisplay(NULL);
   if (!disp)
      eprintf("Can't open X display. It *is* running, yeah?");
   vis = DefaultVisual(disp, DefaultScreen(disp));
   depth = DefaultDepth(disp, DefaultScreen(disp));
   cm = DefaultColormap(disp, DefaultScreen(disp));
   root = RootWindow(disp, DefaultScreen(disp));
   scr = ScreenOfDisplay(disp, DefaultScreen(disp));
   xid_context = XUniqueContext();

#ifdef HAVE_LIBXINERAMA
   init_xinerama();
#endif /* HAVE_LIBXINERAMA */

   imlib_context_set_display(disp);
   imlib_context_set_visual(vis);
   imlib_context_set_colormap(cm);
   imlib_context_set_color_modifier(NULL);
   imlib_context_set_progress_function(NULL);
   imlib_context_set_operation(IMLIB_OP_COPY);
   wmDeleteWindow = XInternAtom(disp, "WM_DELETE_WINDOW", False);

   /* Initialise random numbers */
   srand(getpid() * time(NULL) % ((unsigned int) -1));

   /* Set up the font stuff */
   imlib_add_path_to_font_path(".");
   imlib_add_path_to_font_path(PREFIX "/share/feh/fonts");
   imlib_add_path_to_font_path("./ttfonts");

   D_RETURN_(4);
}

int
feh_load_image_char(Imlib_Image * im, char *filename)
{
   feh_file *file;
   int i;

   D_ENTER(4);
   file = feh_file_new(filename);
   i = feh_load_image(im, file);
   feh_file_free(file);
   D_RETURN(4, i);
}

int
feh_load_image(Imlib_Image * im, feh_file * file)
{
   Imlib_Load_Error err;

   D_ENTER(4);
   D(3, ("filename is %s, image is %p\n", file->filename, im));

   if (!file || !file->filename)
      D_RETURN(4, 0);

   /* Handle URLs */
   if ((!strncmp(file->filename, "http://", 7)) ||
       (!strncmp(file->filename, "https://", 8)) ||
       (!strncmp(file->filename, "ftp://", 6)))
   {
      char *tmpname = NULL;
      char *tempcpy;

      tmpname = feh_http_load_image(file->filename);
      if (tmpname == NULL)
         D_RETURN(4, 0);
      *im = imlib_load_image_with_error_return(tmpname, &err);
      if (im)
      {
         /* load the info now, in case it's needed after we delete the
            temporary image file */
         tempcpy = file->filename;
         file->filename = tmpname;
         feh_file_info_load(file, *im);
         file->filename = tempcpy;
      }
      if ((opt.slideshow) && (opt.reload == 0))
      {
         /* Http, no reload, slideshow. Let's keep this image on hand... */
         free(file->filename);
         file->filename = estrdup(tmpname);
      }
      else
      {
         /* Don't cache the image if we're doing reload + http (webcams etc) */
         if (!opt.keep_http)
            unlink(tmpname);
      }
      if (!opt.keep_http)
         add_file_to_rm_filelist(tmpname);
      free(tmpname);
   }
   else
   {
      *im = imlib_load_image_with_error_return(file->filename, &err);
   }

   if ((err) || (!im))
   {
      if (opt.verbose && !opt.quiet)
      {
         fprintf(stdout, "\n");
         reset_output = 1;
      }
      /* Check error code */
      switch (err)
      {
        case IMLIB_LOAD_ERROR_FILE_DOES_NOT_EXIST:
           if (!opt.quiet)
              weprintf("%s - File does not exist", file->filename);
           break;
        case IMLIB_LOAD_ERROR_FILE_IS_DIRECTORY:
           if (!opt.quiet)
              weprintf("%s - Directory specified for image filename",
                       file->filename);
           break;
        case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_READ:
           if (!opt.quiet)
              weprintf("%s - No read access to directory", file->filename);
           break;
        case IMLIB_LOAD_ERROR_UNKNOWN:
        case IMLIB_LOAD_ERROR_NO_LOADER_FOR_FILE_FORMAT:
           if (!opt.quiet)
              weprintf("%s - No Imlib2 loader for that file format",
                       file->filename);
           break;
        case IMLIB_LOAD_ERROR_PATH_TOO_LONG:
           if (!opt.quiet)
              weprintf("%s - Path specified is too long", file->filename);
           break;
        case IMLIB_LOAD_ERROR_PATH_COMPONENT_NON_EXISTANT:
           if (!opt.quiet)
              weprintf("%s - Path component does not exist", file->filename);
           break;
        case IMLIB_LOAD_ERROR_PATH_COMPONENT_NOT_DIRECTORY:
           if (!opt.quiet)
              weprintf("%s - Path component is not a directory",
                       file->filename);
           break;
        case IMLIB_LOAD_ERROR_PATH_POINTS_OUTSIDE_ADDRESS_SPACE:
           if (!opt.quiet)
              weprintf("%s - Path points outside address space",
                       file->filename);
           break;
        case IMLIB_LOAD_ERROR_TOO_MANY_SYMBOLIC_LINKS:
           if (!opt.quiet)
              weprintf("%s - Too many levels of symbolic links",
                       file->filename);
           break;
        case IMLIB_LOAD_ERROR_OUT_OF_MEMORY:
           if (!opt.quiet)
              weprintf("While loading %s - Out of memory", file->filename);
           break;
        case IMLIB_LOAD_ERROR_OUT_OF_FILE_DESCRIPTORS:
           eprintf("While loading %s - Out of file descriptors",
                   file->filename);
           break;
        case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_WRITE:
           if (!opt.quiet)
              weprintf("%s - Cannot write to directory", file->filename);
           break;
        case IMLIB_LOAD_ERROR_OUT_OF_DISK_SPACE:
           if (!opt.quiet)
              weprintf("%s - Cannot write - out of disk space",
                       file->filename);
           break;
        default:
           if (!opt.quiet)
              weprintf
                 ("While loading %s - Unknown error (%d). Attempting to continue",
                  file->filename, err);
           break;
      }
      D(3, ("Load *failed*\n"));
      D_RETURN(4, 0);
   }

   D(3, ("Loaded ok\n"));
   D_RETURN(4, 1);
}

char *
feh_http_load_image(char *url)
{
   char *tmpname;
   char *tmpname_timestamper = NULL;
   char *basename;
   char *newurl = NULL;
   char randnum[20];
   int rnum;
   char *path = NULL;

   D_ENTER(4);

   if (opt.keep_http)
   {
      if (opt.output_dir)
         path = opt.output_dir;
      else
         path = "";
   }
   else
      path = "/tmp/";

   basename = strrchr(url, '/') + 1;
   tmpname = feh_unique_filename(path, basename);

   if (opt.wget_timestamp)
   {
      char cppid[10];
      pid_t ppid;

      ppid = getpid();
      snprintf(cppid, sizeof(cppid), "%06ld", (long)ppid);
      tmpname_timestamper =
         estrjoin("", "/tmp/feh_", cppid, "_", basename, NULL);
   }

   if (opt.wget_timestamp)
   {
      newurl = estrdup(url);
   }
   else
   {
      rnum = rand();
      snprintf(randnum, sizeof(randnum), "%d", rnum);
      newurl = estrjoin("?", url, randnum, NULL);
   }
   D(3, ("newurl: %s\n", newurl));

   if (opt.builtin_http)
   {
      /* state for HTTP header parser */
#define SAW_NONE    1
#define SAW_ONE_CM  2
#define SAW_ONE_CJ  3
#define SAW_TWO_CM  4
#define IN_BODY     5

#define OUR_BUF_SIZE 1024
#define EOL "\015\012"

      int sockno = 0;
      int size;
      int body = SAW_NONE;
      struct sockaddr_in addr;
      struct hostent *hptr;
      char *hostname;
      char *get_string;
      char *host_string;
      char *query_string;
      char *get_url;
      static char buf[OUR_BUF_SIZE];
      char ua_string[] = "User-Agent: feh image viewer";
      char accept_string[] = "Accept: image/*";
      FILE *fp;

      D(4, ("using builtin http collection\n"));
      fp = fopen(tmpname, "w");
      if (!fp)
      {
         weprintf("couldn't write to file %s:", tmpname);
         free(tmpname);
         D_RETURN(4, NULL);
      }

      hostname = feh_strip_hostname(newurl);
      if (!hostname)
      {
         weprintf("couldn't work out hostname from %s:", newurl);
         free(tmpname);
         free(newurl);
         D_RETURN(4, NULL);
      }

      D(4, ("trying hostname %s\n", hostname));

      if (!(hptr = feh_gethostbyname(hostname)))
      {
         weprintf("error resolving host %s:", hostname);
         free(hostname);
         free(tmpname);
         free(newurl);
         D_RETURN(4, NULL);
      }

      /* Copy the address of the host to socket description. */
      memcpy(&addr.sin_addr, hptr->h_addr, hptr->h_length);

      /* Set port and protocol */
      addr.sin_family = AF_INET;
      addr.sin_port = htons(80);

      if ((sockno = socket(PF_INET, SOCK_STREAM, 0)) == -1)
      {
         weprintf("error opening socket:");
         free(tmpname);
         free(hostname);
         free(newurl);
         D_RETURN(4, NULL);
      }
      if (connect(sockno, (struct sockaddr *) &addr, sizeof(addr)) == -1)
      {
         weprintf("error connecting socket:");
         free(tmpname);
         free(hostname);
         free(newurl);
         D_RETURN(4, NULL);
      }

      get_url = strchr(newurl, '/') + 2;
      get_url = strchr(get_url, '/');

      get_string = estrjoin(" ", "GET", get_url, "HTTP/1.0", NULL);
      host_string = estrjoin(" ", "Host:", hostname, NULL);
      query_string =
         estrjoin(EOL, get_string, host_string, accept_string, ua_string, "",
                  "", NULL);
      /* At this point query_string looks something like
         **
         **    GET /dir/foo.jpg?123456 HTTP/1.0^M^J
         **    Host: www.example.com^M^J
         **    Accept: image/ *^M^J
         **    User-Agent: feh image viewer^M^J
         **    ^M^J
         **
         ** Host: is required by HTTP/1.1 and very important for some sites,
         ** even with HTTP/1.0
         **
         ** -- BEG
       */
      if ((send(sockno, query_string, strlen(query_string), 0)) == -1)
      {
         free(get_string);
         free(host_string);
         free(query_string);
         free(tmpname);
         free(hostname);
         free(newurl);
         weprintf("error sending over socket:");
         D_RETURN(4, NULL);
      }
      free(get_string);
      free(host_string);
      free(query_string);
      free(hostname);
      free(newurl);

      while ((size = read(sockno, &buf, OUR_BUF_SIZE)))
      {
         if (body == IN_BODY)
         {
            fwrite(buf, 1, size, fp);
         }
         else
         {
            int i;

            for (i = 0; i < size; i++)
            {
               /* We are looking for ^M^J^M^J, but will accept
                  ** ^J^J from broken servers. Stray ^Ms will be
                  ** ignored.
                  **
                  ** TODO:
                  ** Checking the headers for a
                  **    Content-Type: image/ *
                  ** header would help detect problems with results.
                  ** Maybe look at the response code too? But there is
                  ** no fundamental reason why a 4xx or 5xx response
                  ** could not return an image, it is just the 3xx
                  ** series we need to worry about.
                  **
                  ** Also, grabbing the size from the Content-Length
                  ** header and killing the connection after that
                  ** many bytes where read would speed up closing the
                  ** socket.
                  ** -- BEG
                */

               switch (body)
               {

                 case IN_BODY:
                    fwrite(buf + i, 1, size - i, fp);
                    i = size;
                    break;

                 case SAW_ONE_CM:
                    if (buf[i] == '\012')
                    {
                       body = SAW_ONE_CJ;
                    }
                    else
                    {
                       body = SAW_NONE;
                    }
                    break;

                 case SAW_ONE_CJ:
                    if (buf[i] == '\015')
                    {
                       body = SAW_TWO_CM;
                    }
                    else
                    {
                       if (buf[i] == '\012')
                       {
                          body = IN_BODY;
                       }
                       else
                       {
                          body = SAW_NONE;
                       }
                    }
                    break;

                 case SAW_TWO_CM:
                    if (buf[i] == '\012')
                    {
                       body = IN_BODY;
                    }
                    else
                    {
                       body = SAW_NONE;
                    }
                    break;

                 case SAW_NONE:
                    if (buf[i] == '\015')
                    {
                       body = SAW_ONE_CM;
                    }
                    else
                    {
                       if (buf[i] == '\012')
                       {
                          body = SAW_ONE_CJ;
                       }
                    }
                    break;

               }                            /* switch */
            }                               /* for i */
         }
      }                                     /* while read */
      close(sockno);
      fclose(fp);
   }
   else
   {
      int pid;
      int status;

      if ((pid = fork()) < 0)
      {
         weprintf("open url: fork failed:");
         free(tmpname);
         free(newurl);
         D_RETURN(4, NULL);
      }
      else if (pid == 0)
      {
         char *quiet = NULL;

         if (!opt.verbose)
            quiet = estrdup("-q");

         if (opt.wget_timestamp)
         {
            execlp("wget", "wget", "-N", "-O", tmpname_timestamper, newurl,
                   quiet, (char*) NULL);
         }
         else
         {
            execlp("wget", "wget", "--cache", "0", newurl, "-O", tmpname,
                   quiet, (char*) NULL);
         }
         eprintf("url: exec failed: wget:");
      }
      else
      {
         waitpid(pid, &status, 0);

         if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
         {
            weprintf("url: wget failed to load URL %s\n", url);
            free(tmpname);
            free(newurl);
            D_RETURN(4, NULL);
         }
         if (opt.wget_timestamp)
         {
            char cmd[2048];

            snprintf(cmd, sizeof(cmd), "/bin/cp %s %s", tmpname_timestamper,
                     tmpname);
            system(cmd);
         }
      }
   }

   free(newurl);
   D_RETURN(4, tmpname);
}

struct hostent *
feh_gethostbyname(const char *name)
{
   struct hostent *hp;
   unsigned long addr;

   D_ENTER(3);
   addr = (unsigned long) inet_addr(name);
   if ((int) addr != -1)
      hp = gethostbyaddr((char *) &addr, sizeof(addr), AF_INET);
   else
      hp = gethostbyname(name);
   D_RETURN(3, hp);
}

char *
feh_strip_hostname(char *url)
{
   char *ret;
   char *start;
   char *finish;
   int len;

   D_ENTER(3);

   start = strchr(url, '/');
   if (!start)
      D_RETURN(3, NULL);

   start += 2;

   finish = strchr(start, '/');
   if (!finish)
      D_RETURN(3, NULL);

   len = finish - start;

   ret = emalloc(len + 1);
   strncpy(ret, start, len);
   ret[len] = '\0';
   D_RETURN(3, ret);
}

void
feh_draw_zoom(winwidget w)
{
   static Imlib_Font fn = NULL;
   int tw = 0, th = 0;
   Imlib_Image im = NULL;
   char buf[100];
   static DATA8 atab[256];

   D_ENTER(4);

   if (!w->im)
      D_RETURN_(4);

   if (!fn) {
      fn = gib_imlib_load_font(DEFAULT_FONT);
      memset(atab, 0, sizeof(atab));
   }

   if (!fn)
   {
      weprintf("Couldn't load font for zoom printing");
      D_RETURN_(4);
   }

   snprintf(buf, sizeof(buf), "%.0f%%, %dx%d", w->zoom * 100,
            (int) (w->im_w * w->zoom), (int) (w->im_h * w->zoom));

   /* Work out how high the font is */
   gib_imlib_get_text_size(fn, buf, NULL, &tw, &th, IMLIB_TEXT_TO_RIGHT);

   tw += 3;
   th += 3;
   im = imlib_create_image(tw, th);
   if (!im)
      eprintf("Couldn't create image. Out of memory?");

   gib_imlib_image_set_has_alpha(im, 1);
   gib_imlib_apply_color_modifier_to_rectangle(im, 0, 0, tw, th,
                                               NULL, NULL, NULL, atab);
   gib_imlib_image_fill_rectangle(im, 0, 0, tw, th, 0, 0, 0, 0);

   gib_imlib_text_draw(im, fn, NULL, 2, 2, buf, IMLIB_TEXT_TO_RIGHT,
                       0, 0, 0, 255);
   gib_imlib_text_draw(im, fn, NULL, 1, 1, buf, IMLIB_TEXT_TO_RIGHT,
                       255, 255, 255, 255);
   gib_imlib_render_image_on_drawable(w->bg_pmap, im, 0, w->h - th, 1, 1, 0);
   gib_imlib_free_image_and_decache(im);
   D_RETURN_(4);
}

void
feh_draw_filename(winwidget w)
{
   static Imlib_Font fn = NULL;
   int tw = 0, th = 0;
   Imlib_Image im = NULL;
   static DATA8 atab[256];
   char *s = NULL;
   int len = 0;

   D_ENTER(4);

   if ((!w->file) || (!FEH_FILE(w->file->data))
       || (!FEH_FILE(w->file->data)->filename))
      D_RETURN_(4);

   if (!fn)
   {
      memset(atab, 0, sizeof(atab));
      if (w->full_screen)
         fn = gib_imlib_load_font(DEFAULT_FONT_BIG);
      else
         fn = gib_imlib_load_font(DEFAULT_FONT);
   }

   if (!fn)
   {
      weprintf("Couldn't load font for filename printing");
      D_RETURN_(4);
   }

   /* Work out how high the font is */
   gib_imlib_get_text_size(fn, FEH_FILE(w->file->data)->filename, NULL, &tw, &th,
                           IMLIB_TEXT_TO_RIGHT);

   /* tw is no longer correct, if the filename is shorter than
    * the string "%d of %d" used below in fullscreen mode */
   tw += 3;
   th += 3;
   im = imlib_create_image(tw, 2*th);
   if (!im)
      eprintf("Couldn't create image. Out of memory?");

   gib_imlib_image_set_has_alpha(im, 1);
   gib_imlib_apply_color_modifier_to_rectangle(im, 0, 0, tw, 2*th,
                                               NULL, NULL, NULL, atab);
   gib_imlib_image_fill_rectangle(im, 0, 0, tw, 2*th, 0, 0, 0, 0);

   gib_imlib_text_draw(im, fn, NULL, 2, 2, FEH_FILE(w->file->data)->filename,
                       IMLIB_TEXT_TO_RIGHT, 0, 0, 0, 255);
   gib_imlib_text_draw(im, fn, NULL, 1, 1, FEH_FILE(w->file->data)->filename,
                       IMLIB_TEXT_TO_RIGHT, 255, 255, 255, 255);
   /* Print the position in the filelist, if we are in fullscreen and the
    * list has more than one element */
   if (w->full_screen && (gib_list_length(filelist)-1))
   {
      /* sic! */
      len = snprintf(NULL, 0, "%d of %d", gib_list_length(filelist), gib_list_length(filelist))+1;
      s = emalloc(len);
      snprintf(s, len, "%d of %d",
               gib_list_num(filelist, current_file) + 1,
               gib_list_length(filelist));
      /* This should somehow be right-aligned */
      gib_imlib_text_draw(im, fn, NULL, 2, th+1, s, IMLIB_TEXT_TO_RIGHT, 0, 0, 0, 255);
      gib_imlib_text_draw(im, fn, NULL, 1, th, s, IMLIB_TEXT_TO_RIGHT, 255, 255, 255, 255);
      free(s);
   }

   gib_imlib_render_image_on_drawable(w->bg_pmap, im, 0, 0, 1, 1, 0);

   gib_imlib_free_image_and_decache(im);
   D_RETURN_(4);
}

char *build_caption_filename(feh_file *file) {
  char *caption_filename;
  char *s, *dir;
  s = strrchr(file->filename, '/');
  if (s) {
    dir = estrdup(file->filename);
    s = strrchr(dir, '/');
    *s = '\0';
  } else {
    dir = estrdup(".");
  }
  caption_filename = estrjoin("",
                              dir,
                              "/",
                              opt.caption_path,
                              "/",
                              file->name,
                              ".txt",
                              NULL);
  free(dir);
  return caption_filename;
}

void
feh_draw_caption(winwidget w)
{
   static Imlib_Font fn = NULL;
   int tw = 0, th = 0, ww, hh;
   int x, y;
   Imlib_Image im = NULL;
   static DATA8 atab[256];
   char *p;
   gib_list *lines, *l;
   static gib_style *caption_style = NULL;
   feh_file *file;

   D_ENTER(4);

   if (!w->file) {
     D_RETURN_(4);
   }
   file = FEH_FILE(w->file->data);
   if (!file->filename) {
     D_RETURN_(4);
   }

   if (!file->caption) {
      char *caption_filename;
      caption_filename = build_caption_filename(file);
      /* read caption from file */
      file->caption = ereadfile(caption_filename);
      free(caption_filename);
   }

   if (file->caption == NULL) {
     /* caption file is not there, we want to cache that, otherwise we'll stat
      * the damn file every time we render the image. Reloading an image will
      * always cause the caption to be reread though so we're safe to do so.
      * (Before this bit was added, when zooming a captionless image with
      * captions enabled, the captions file would be stat()d like 30 times a
      * second) - don't forget this function is called from
      * winwidget_render_image().
      */
     file->caption = estrdup("");
     D_RETURN_(4);
   }

   if (file->caption == '\0') {
     D_RETURN_(4);
   }

   if (!caption_style) {
     caption_style = gib_style_new("caption");
     caption_style->bits = gib_list_add_front(caption_style->bits,
                                            gib_style_bit_new(0,0,0,0,0,0));
     caption_style->bits = gib_list_add_front(caption_style->bits, 
                                            gib_style_bit_new(1,1,0,0,0,255));
   }

   if (!fn)
   {
      memset(atab, 0, sizeof(atab));
      if (w->full_screen)
         fn = gib_imlib_load_font(DEFAULT_FONT_BIG);
      else
         fn = gib_imlib_load_font(DEFAULT_FONT);
   }

   if (!fn)
   {
      weprintf("Couldn't load font for caption printing");
      D_RETURN_(4);
   }

   lines = feh_wrap_string(file->caption, w->w, w->h, fn, NULL);
   if (!lines)
     D_RETURN_(4);

   /* Work out how high/wide the caption is */
   l = lines;
   while (l) {
     p = (char *) l->data;
     gib_imlib_get_text_size(fn, p, caption_style, &ww, &hh, IMLIB_TEXT_TO_RIGHT);
     if (ww > tw)
       tw = ww;
     th += hh;
     if (l->next)
       th += 1; /* line spacing */
     l = l->next;
   }
   
   /* we don't want the caption overlay larger than our window */
   if (th > w->h)
     th = w->h;
   if (tw > w->w)
     tw = w->w;

   im = imlib_create_image(tw, th);
   if (!im)
      eprintf("Couldn't create image. Out of memory?");

   gib_imlib_image_set_has_alpha(im, 1);
   gib_imlib_apply_color_modifier_to_rectangle(im, 0, 0, tw, th,
                                               NULL, NULL, NULL, atab);
   gib_imlib_image_fill_rectangle(im, 0, 0, tw, th, 0, 0, 0, 0);
   
   l = lines;
   x = 0;
   y = 0;
   while (l) {
     p = (char *) l->data;
     gib_imlib_get_text_size(fn, p, caption_style, &ww, &hh, IMLIB_TEXT_TO_RIGHT);
     x = (tw - ww) / 2;
     if (w->caption_entry) {
     gib_imlib_text_draw(im, fn, caption_style, x, y, p, IMLIB_TEXT_TO_RIGHT,
                         255, 255, 0, 255);
     } else {
     gib_imlib_text_draw(im, fn, caption_style, x, y, p, IMLIB_TEXT_TO_RIGHT,
                         255, 255, 255, 255);
     }
                
     y += hh + 1; /* line spacing */
     l = l->next;
   }
   
   gib_imlib_render_image_on_drawable(w->bg_pmap, im, (w->w - tw) / 2, w->h - th, 1, 1, 0);
   gib_imlib_free_image_and_decache(im);
   gib_list_free_and_data(lines);
   D_RETURN_(4);
}


unsigned char reset_output = 0;

void
feh_display_status(char stat)
{
   static int i = 0;
   static int init_len = 0;
   int j = 0;

   D_ENTER(5);

   D(5, ("filelist %p, filelist->next %p\n", filelist, filelist->next));

   if (!init_len)
      init_len = gib_list_length(filelist);

   if (i)
   {
      if (reset_output)
      {
         /* There's just been an error message. Unfortunate ;) */
         for (j = 0; j < (((i % 50) + ((i % 50) / 10)) + 7); j++)
            fprintf(stdout, " ");
      }

      if (!(i % 50))
      {
         int len;
         char buf[50];

         len = gib_list_length(filelist);
         snprintf(buf, sizeof(buf), " %5d/%d (%d)\n[%3d%%] ", i, init_len,
                  len, ((int) ((float) i / init_len * 100)));
         fprintf(stdout, buf);
      }
      else if ((!(i % 10)) && (!reset_output))
         fprintf(stdout, " ");

      reset_output = 0;
   }
   else
      fprintf(stdout, "[  0%%] ");

   fprintf(stdout, "%c", stat);
   fflush(stdout);
   i++;
   D_RETURN_(5);
}

void feh_edit_inplace_orient(winwidget w, int orientation) {
  int ret;
  Imlib_Image old;
  D_ENTER(4);
  if(!w->file
    || !w->file->data 
    || !FEH_FILE(w->file->data)->filename)
    D_RETURN_(4);

  if (!strcmp(gib_imlib_image_format(w->im), "jpeg")) {
    feh_edit_inplace_lossless_rotate(w, orientation);
    feh_reload_image(w, 1, 1);
    D_RETURN_(4);
  }

  ret = feh_load_image(&old, FEH_FILE(w->file->data));
  if(ret) {
    gib_imlib_image_orientate(old, orientation);
    gib_imlib_save_image(old, FEH_FILE(w->file->data)->filename);
    gib_imlib_free_image(old);
    feh_reload_image(w, 1, 1);
  } else {
    weprintf("failed to load image from disk to edit it in place\n");
  }
  
  D_RETURN_(4);
}


/* TODO max_height is ignored... Could use a function which generates a
 * transparent text overlay image, with wrapping and all. Would be useful */
gib_list *
feh_wrap_string(char *text, int wrap_width, int max_height, Imlib_Font fn, gib_style * style)
{
   gib_list *ll, *lines = NULL, *list = NULL, *words;
   gib_list *l = NULL;
   char delim[2] = { '\n', '\0' };
   int w, line_width;
   int tw, th;
   char *p, *pp;
   char *line = NULL;
   char *temp;
   int space_width = 0, m_width = 0, t_width = 0, new_width = 0;

   lines = gib_string_split(text, delim);

   if (wrap_width)
   {
      gib_imlib_get_text_size(fn, "M M", style, &t_width, NULL,
                              IMLIB_TEXT_TO_RIGHT);
      gib_imlib_get_text_size(fn, "M", style, &m_width, NULL,
                              IMLIB_TEXT_TO_RIGHT);
      space_width = t_width - (2 * m_width);
      w = wrap_width;
      l = lines;
      while (l)
      {
         line_width = 0;
         p = (char *) l->data;
         /* quick check to see if whole line fits okay */
         gib_imlib_get_text_size(fn, p, style, &tw, &th, IMLIB_TEXT_TO_RIGHT);
         if (tw <= w) {
            list = gib_list_add_end(list, estrdup(p));
         } else if (strlen(p) == 0) {
            list = gib_list_add_end(list, estrdup(""));
         } else if (!strcmp(p, " ")) {
            list = gib_list_add_end(list, estrdup(" "));
         } else {
            words = gib_string_split(p, " ");
            if (words) {
               ll = words;
               while (ll) {
                  pp = (char *) ll->data;
                  if (strcmp(pp, " ")) {
                     gib_imlib_get_text_size(fn, pp, style, &tw, &th,
                                             IMLIB_TEXT_TO_RIGHT);
                     if (line_width == 0)
                        new_width = tw;
                     else
                        new_width = line_width + space_width + tw;
                     if (new_width <= w) {
                        /* add word to line */
                        if (line) {
                           int len;

                           len = strlen(line) + strlen(pp) + 2;
                           temp = emalloc(len);
                           snprintf(temp, len, "%s %s", line, pp);
                           free(line);
                           line = temp;
                        } else {
                          line = estrdup(pp);
                        }
                        line_width = new_width;
                     } else if (line_width == 0) {
                        /* can't fit single word in :/
                           increase width limit to width of word
                           and jam the bastard in anyhow */
                        w = tw;
                        line = estrdup(pp);
                        line_width = new_width;
                     } else {
                        /* finish this line, start next and add word there */
                        if (line) {
                           list = gib_list_add_end(list, estrdup(line));
                           free(line);
                           line = NULL;
                        }
                        line = estrdup(pp);
                        line_width = tw;
                     }
                  }
                  ll = ll->next;
               }
               if (line) {
                  /* finish last line */
                  list = gib_list_add_end(list, estrdup(line));
                  free(line);
                  line = NULL;
                  line_width = 0;
               }
               gib_list_free_and_data(words);
            }
         }
         l = l->next;
      }
      gib_list_free_and_data(lines);
      lines = list;
   }
   return lines;
}

void feh_edit_inplace_lossless_rotate(winwidget w, int orientation) {
  FILE *input_file;
  FILE *output_file;
  struct jpeg_decompress_struct srcinfo;
  struct jpeg_compress_struct dstinfo;
  struct jpeg_error_mgr jsrcerr, jdsterr;
  jvirt_barray_ptr * src_coef_arrays;
  jvirt_barray_ptr * dst_coef_arrays;
  JCOPY_OPTION copyoption;
  jpeg_transform_info transformoption;
  int len;
  char *outfilename;
  char *infilename = FEH_FILE(w->file->data)->filename;

  copyoption = JCOPYOPT_ALL;
  transformoption.transform = JXFORM_NONE;
  transformoption.trim = FALSE;
  transformoption.force_grayscale = FALSE;

  if (orientation == 1) {
    transformoption.transform = JXFORM_ROT_90;
  } else if (orientation == 2) {
    transformoption.transform = JXFORM_ROT_180;
  } else {
    transformoption.transform = JXFORM_ROT_270;
  }
  
  if ((input_file = fopen(infilename, "rb")) == NULL) {
    weprintf("couldn't open file for reading: %s\n", infilename);
    D_RETURN_(4);
  }
  len = strlen(infilename) + sizeof(".tmp") + 1;
  outfilename = emalloc(len);
  snprintf(outfilename, len, "%s.tmp", infilename);

  if ((output_file = fopen(outfilename, "wb")) == NULL) {
    weprintf("couldn't open file for writing: %s\n", outfilename);
    free(outfilename);
    fclose(input_file);
    D_RETURN_(4);
  }

  /* Initialize the JPEG decompression object with default error handling. */
  srcinfo.err = jpeg_std_error(&jsrcerr);
  jpeg_create_decompress(&srcinfo);

  /* Initialize the JPEG compression object with default error handling. */
  dstinfo.err = jpeg_std_error(&jdsterr);
  jpeg_create_compress(&dstinfo);
  jsrcerr.trace_level = jdsterr.trace_level;

  /* Specify data source for decompression */
  jpeg_stdio_src(&srcinfo, input_file);

  /* Enable saving of extra markers that we want to copy */
  jcopy_markers_setup(&srcinfo, copyoption);

  /* Read file header */
  (void) jpeg_read_header(&srcinfo, TRUE);

  /* Any space needed by a transform option must be requested before
   * jpeg_read_coefficients so that memory allocation will be done right.
   */
  jtransform_request_workspace(&srcinfo, &transformoption);

  /* Read source file as DCT coefficients */
  src_coef_arrays = jpeg_read_coefficients(&srcinfo);

  /* Initialize destination compression parameters from source values */
  jpeg_copy_critical_parameters(&srcinfo, &dstinfo);

  /* Adjust destination parameters if required by transform options;
   * also find out which set of coefficient arrays will hold the output.
   */
  dst_coef_arrays = jtransform_adjust_parameters(&srcinfo, &dstinfo,
             src_coef_arrays,
             &transformoption);

  /* Specify data destination for compression */
  jpeg_stdio_dest(&dstinfo, output_file);

  /* Start compressor (note no image data is actually written here) */
  jpeg_write_coefficients(&dstinfo, dst_coef_arrays);

  /* Copy to the output file any extra markers that we want to preserve */
  jcopy_markers_execute(&srcinfo, &dstinfo, copyoption);

  /* Execute image transformation */
  jtransform_execute_transformation(&srcinfo, &dstinfo,
            src_coef_arrays,
            &transformoption);

  /* Finish compression and release memory */
  jpeg_finish_compress(&dstinfo);
  jpeg_destroy_compress(&dstinfo);

  (void) jpeg_finish_decompress(&srcinfo);
  jpeg_destroy_decompress(&srcinfo);

  fclose(input_file);
  fclose(output_file);

  /* TODO fix EXIF tags (orientation, width, height) */

  /* rename outfilename to infilename.. if it worked */
  if (jsrcerr.num_warnings > 0) {
    weprintf("got errors from libjpeg (%d), not replacing file\n", jsrcerr.num_warnings);
  } else {
    if (rename(outfilename, infilename)) {
      weprintf("failed to replace file %s with %s\n", infilename, outfilename);
    }
  }
  free(outfilename);
}

void
feh_draw_actions(winwidget w)
{
   static Imlib_Font fn = NULL;
   int tw = 0, th = 0;
   int max_tw = 0;
   int line_th = 0;
   Imlib_Image im = NULL;
   static DATA8 atab[256];
   int i = 0;
   int num_actions = 0;
   
   D_ENTER(4);

// count the number of defined actions
   for (num_actions=0;opt.actions[num_actions];num_actions++)
   	;
   if (num_actions == 0)
      return;
   
   if ((!w->file) || (!FEH_FILE(w->file->data))
       || (!FEH_FILE(w->file->data)->filename))
      D_RETURN_(4);

   if (!fn)
   {
      memset(atab, 0, sizeof(atab));
      if (w->full_screen)
         fn = gib_imlib_load_font(DEFAULT_FONT_BIG);
      else
         fn = gib_imlib_load_font(DEFAULT_FONT);
   }

   if (!fn)
   {
      weprintf("Couldn't load font for actions printing");
      D_RETURN_(4);
   }


   gib_imlib_get_text_size(fn, "defined actions:", NULL, &tw, &th,
                           IMLIB_TEXT_TO_RIGHT);
// Check for the widest line
   max_tw = tw; 
   for (i=0;opt.actions[i];i++) {
   gib_imlib_get_text_size(fn, opt.actions[i], NULL, &tw, &th,
                           IMLIB_TEXT_TO_RIGHT);
   	if (tw>max_tw) {
	   max_tw = tw;
	}
   }

   tw = max_tw;
   tw += 3;
   th += 3;
   line_th = th;
   th = (th*num_actions)+line_th;

   im = imlib_create_image(tw, th);
   if (!im)
      eprintf("Couldn't create image. Out of memory?");

   gib_imlib_image_set_has_alpha(im, 1);
   gib_imlib_apply_color_modifier_to_rectangle(im, 0, 0, tw, th,
                                               NULL, NULL, NULL, atab);
   gib_imlib_image_fill_rectangle(im, 0, 0, tw, th, 0, 0, 0, 0);
   
   gib_imlib_text_draw(im, fn, NULL, 1, 1, "defined actions:",
                       IMLIB_TEXT_TO_RIGHT, 0, 0, 0, 255);
   gib_imlib_text_draw(im, fn, NULL, 2, 2, "defined actions:",
                       IMLIB_TEXT_TO_RIGHT, 255, 255, 255, 255);

   for(i=0;i<num_actions;i++)
   {
//    compose a line containing an index, a colon followed by the
//    action.
      char index[1];
      char line[strlen(opt.actions[i])+5];
      sprintf(index, "%d", i);
      strcpy(line, index);
      strcat(line, ": ");
      strcat(line, opt.actions[i]);

      gib_imlib_text_draw(im, fn, NULL, 2, ((i+1)*line_th)+2, line,
                          IMLIB_TEXT_TO_RIGHT, 0, 0, 0, 255);
      gib_imlib_text_draw(im, fn, NULL, 1, ((i+1)*line_th)+1, line,
                          IMLIB_TEXT_TO_RIGHT, 255, 255, 255, 255);
      free(line);
   }

   gib_imlib_render_image_on_drawable(w->bg_pmap, im, 0, 0, 1, 1, 0);

   gib_imlib_free_image_and_decache(im);
   D_RETURN_(4);
}
