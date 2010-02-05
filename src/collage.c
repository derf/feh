/* collage.c

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
#include "winwidget.h"
#include "filelist.h"
#include "options.h"

void
init_collage_mode(void)
{
   Imlib_Image im_main;
   Imlib_Image im_temp;
   int ww, hh, www, hhh, xxx, yyy;
   int w = 800, h = 600;
   int bg_w = 0, bg_h = 0;
   winwidget winwid = NULL;
   Imlib_Image bg_im = NULL, im_thumb = NULL;
   feh_file *file = NULL;
   unsigned char trans_bg = 0;
   gib_list *l, *last = NULL;
   char *s;

   D_ENTER(4);

   mode = "collage";

   /* Use bg image dimensions for default size */
   if (opt.bg && opt.bg_file)
   {
      if (!strcmp(opt.bg_file, "trans"))
         trans_bg = 1;
      else
      {

         D(4,("Time to apply a background to blend onto\n"));
         if (feh_load_image_char(&bg_im, opt.bg_file) != 0)
         {
            bg_w = gib_imlib_image_get_width(bg_im);
            bg_h = gib_imlib_image_get_height(bg_im);
         }
      }
   }

   if (!opt.limit_w || !opt.limit_h)
   {
      if (bg_im)
      {
         if (opt.verbose)
            fprintf(stdout,
                    PACKAGE " - No size restriction specified for collage.\n"
                    " You did specify a background however, so the\n"
                    " collage size has defaulted to the size of the image\n");
         opt.limit_w = bg_w;
         opt.limit_h = bg_h;
      }
      else
      {
         if (opt.verbose)
            fprintf(stdout,
                    PACKAGE " - No size restriction specified for collage.\n"
                    " - For collage mode, you need to specify width and height.\n"
                    " Using defaults (width 800, height 600)\n");
         opt.limit_w = 800;
         opt.limit_h = 600;
      }
   }

   w = opt.limit_w;
   h = opt.limit_h;
   D(4,("Limiting width to %d and height to %d\n", w, h));

   im_main = imlib_create_image(w, h);

   if (!im_main)
      eprintf("Imlib error creating image");

   if (bg_im)
      gib_imlib_blend_image_onto_image(im_main, bg_im,
                                       gib_imlib_image_has_alpha(bg_im), 0, 0,
                                       bg_w, bg_h, 0, 0, w, h, 1, 0, 0);
   else if (trans_bg)
   {
      gib_imlib_image_fill_rectangle(im_main, 0, 0, w, h, 0, 0, 0, 0);
      gib_imlib_image_set_has_alpha(im_main, 1);
   }
   else
   {
      /* Colour the background */
      gib_imlib_image_fill_rectangle(im_main, 0, 0, w, h, 0, 0, 0, 255);
   }

   /* Create the title string */
   
   if (!opt.title)
	  s = estrdup(PACKAGE " [collage mode]");
   else
	  s = estrdup(feh_printf(opt.title, NULL));
   
   if (opt.display)
   {
      winwid =
         winwidget_create_from_image(im_main, s, WIN_TYPE_SINGLE);
      winwidget_show(winwid);
   }


   for (l = filelist; l; l = l->next)
   {
      file = FEH_FILE(l->data);
      if (last)
      {
         filelist = feh_file_remove_from_list(filelist, last);
         filelist_len--;
         last = NULL;
      }
      D(3,("About to load image %s\n", file->filename));
      if (feh_load_image(&im_temp, file) != 0)
      {
         D(3,("Successfully loaded %s\n", file->filename));
         if (opt.verbose)
            feh_display_status('.');
         www = opt.thumb_w;
         hhh = opt.thumb_h;
         ww = gib_imlib_image_get_width(im_temp);
         hh = gib_imlib_image_get_height(im_temp);

         if (opt.aspect)
         {
            double ratio = 0.0;

            /* Keep the aspect ratio for the thumbnail */
            ratio = ((double) ww / hh) / ((double) www / hhh);

            if (ratio > 1.0)
               hhh = opt.thumb_h / ratio;
            else if (ratio != 1.0)
               www = opt.thumb_w * ratio;
         }

         if ((!opt.stretch) && ((www > ww) || (hhh > hh)))
         {
            /* Don't make the image larger unless stretch is specified */
            www = ww;
            hhh = hh;
         }

         /* pick random coords for thumbnail */
         xxx = ((w - www) * ((double) rand() / RAND_MAX));
         yyy = ((h - hhh) * ((double) rand() / RAND_MAX));
         D(5,("image going on at x=%d, y=%d\n", xxx, yyy));

         im_thumb =
            gib_imlib_create_cropped_scaled_image(im_temp, 0, 0, ww, hh, www,
                                                  hhh, 1);
         gib_imlib_free_image_and_decache(im_temp);

         if (opt.alpha)
         {
            DATA8 atab[256];

            D(4,("Applying alpha options\n"));
            gib_imlib_image_set_has_alpha(im_thumb, 1);
            memset(atab, opt.alpha_level, sizeof(atab));
            gib_imlib_apply_color_modifier_to_rectangle(im_thumb, 0, 0, www,
                                                        hhh, NULL, NULL, NULL,
                                                        atab);
         }
         gib_imlib_blend_image_onto_image(im_main, im_thumb,
                                          gib_imlib_image_has_alpha(im_thumb),
                                          0, 0, www, hhh, xxx, yyy, www, hhh,
                                          1,
                                          gib_imlib_image_has_alpha(im_thumb),
                                          0);
         gib_imlib_free_image_and_decache(im_thumb);
      }
      else
      {
         last = l;
         if (opt.verbose)
            feh_display_status('x');
      }
      if (opt.display)
      {
         winwidget_render_image(winwid, 0, 0);
         if (!feh_main_iteration(0))
            exit(0);
      }
   }
   if (opt.verbose)
      fprintf(stdout, "\n");

   if (opt.output && opt.output_file)
   {
      char output_buf[1024];
      if (opt.output_dir)
        snprintf(output_buf,1024,"%s/%s", opt.output_dir, opt.output_file);
      else 
        strncpy(output_buf,opt.output_file, 1024);
      gib_imlib_save_image(im_main, output_buf);
      if (opt.verbose)
      {
         int tw, th;

         tw = gib_imlib_image_get_width(im_main);
         th = gib_imlib_image_get_height(im_main);
         fprintf(stdout, PACKAGE " - File saved as %s\n", output_buf);
         fprintf(stdout,
                 "    - Image is %dx%d pixels and contains %d thumbnails\n",
                 tw, th, (tw / opt.thumb_w) * (th / opt.thumb_h));
      }
   }

   if (!opt.display)
      gib_imlib_free_image_and_decache(im_main);
   free(s);
   D_RETURN_(4);
}
