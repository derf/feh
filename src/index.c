/* index.c

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

static char *create_index_dimension_string(int w, int h);
static char *create_index_size_string(char *file);
static char *create_index_title_string(int num, int w, int h);

/* TODO Break this up a bit ;) */
/* TODO s/bit/lot */
void
init_index_mode(void)
{
   Imlib_Image im_main;
   Imlib_Image im_temp;
   int w = 800, h = 600, ww = 0, hh = 0, www, hhh, xxx, yyy;
   int x = 0, y = 0;
   int bg_w = 0, bg_h = 0;
   winwidget winwid = NULL;
   Imlib_Image bg_im = NULL, im_thumb = NULL;
   int tot_thumb_h;
   int text_area_h = 50;
   int title_area_h = 0;
   Imlib_Font fn = NULL;
   Imlib_Font title_fn = NULL;
   int text_area_w = 0;
   int tw = 0, th = 0;
   int fw_name, fw_size, fw_dim, fw, fh;
   int vertical = 0;
   int max_column_w = 0;
   int thumbnailcount = 0;
   gib_list *l = NULL, *last = NULL;
   feh_file *file = NULL;
   int lines;
   unsigned char trans_bg = 0;
   int index_image_width, index_image_height;
   int x_offset_name = 0, x_offset_dim = 0, x_offset_size = 0;
   char *s;

   D_ENTER(3);

   if (opt.montage) {
     mode = "montage";
   } else {
     mode = "index";
   }

   fn = gib_imlib_load_font(opt.font);

   if (opt.title_font)
   {
      int fh, fw;

      title_fn = gib_imlib_load_font(opt.title_font);
      if (!fn)
         title_fn = gib_imlib_load_font(DEFAULT_FONT_TITLE);
      gib_imlib_get_text_size(title_fn, "W", NULL, &fw, &fh, IMLIB_TEXT_TO_RIGHT);
      title_area_h = fh + 4;
   }
   else
      title_fn = gib_imlib_load_font(DEFAULT_FONT_TITLE);

   if ((!fn) || (!title_fn))
      eprintf("Error loading fonts");

   /* Work out how tall the font is */
   gib_imlib_get_text_size(fn, "W", NULL, &tw, &th, IMLIB_TEXT_TO_RIGHT);
   /* For now, allow room for the right number of lines with small gaps */
   text_area_h =
      ((th + 2) * (opt.index_show_name + opt.index_show_size +
                   opt.index_show_dim)) + 5;

   /* This includes the text area for index data */
   tot_thumb_h = opt.thumb_h + text_area_h;

   /* Use bg image dimensions for default size */
   if (opt.bg && opt.bg_file)
   {
      if (!strcmp(opt.bg_file, "trans"))
         trans_bg = 1;
      else
      {
         D(3, ("Time to apply a background to blend onto\n"));
         if (feh_load_image_char(&bg_im, opt.bg_file) != 0)
         {
            bg_w = gib_imlib_image_get_width(bg_im);
            bg_h = gib_imlib_image_get_height(bg_im);
         }
      }
   }

   if (!opt.limit_w && !opt.limit_h)
   {
      if (bg_im)
      {
         if (opt.verbose)
            fprintf(stdout,
                    PACKAGE " - No size restriction specified for index.\n"
                    " You did specify a background however, so the\n"
                    " index size has defaulted to the size of the image\n");
         opt.limit_w = bg_w;
         opt.limit_h = bg_h;
      }
      else
      {
         if (opt.verbose)
            fprintf(stdout,
                    PACKAGE " - No size restriction specified for index.\n"
                    " Using defaults (width limited to 800)\n");
         opt.limit_w = 800;
      }
   }


   /* Here we need to whiz through the files, and look at the filenames and
      info in the selected font, work out how much space we need, and
      calculate the size of the image we will require */

   if (opt.limit_w && opt.limit_h)
   {
      int rec_h = 0;

      w = opt.limit_w;
      h = opt.limit_h;

      /* Work out if this is big enough, and give a warning if not */

      /* Pretend we are limiting width by that specified, loop through, and
         see it we fit in the height specified. If not, continue the loop,
         and recommend the final value instead. Carry on and make the index
         anyway. */

      for (l = filelist; l; l = l->next)
      {
         file = FEH_FILE(l->data);
         text_area_w = opt.thumb_w;
         if (opt.index_show_name)
         {
            gib_imlib_get_text_size(fn, file->name, NULL, &fw, &fh,
                                    IMLIB_TEXT_TO_RIGHT);
            if (fw > text_area_w)
               text_area_w = fw;
         }
         if (opt.index_show_dim)
         {
            gib_imlib_get_text_size(fn,
                                    create_index_dimension_string(1000, 1000),
                                    NULL, &fw, &fh, IMLIB_TEXT_TO_RIGHT);
            if (fw > text_area_w)
               text_area_w = fw;
         }
         if (opt.index_show_size)
         {
            gib_imlib_get_text_size(fn,
                                    create_index_size_string(file->filename),
                                    NULL, &fw, &fh, IMLIB_TEXT_TO_RIGHT);
            if (fw > text_area_w)
               text_area_w = fw;
         }
         if (text_area_w > opt.thumb_w)
            text_area_w += 5;

         if ((x > w - text_area_w))
         {
            x = 0;
            y += tot_thumb_h;
         }

         x += text_area_w;
      }
      rec_h = y + tot_thumb_h;

      if (h < rec_h)
      {
         weprintf("The image size you specified (%d by %d) is not large\n"
                  "enough to hold all the thumnails you specified (%d). To fit all\n"
                  "the thumnails, either decrease their size, choose a smaller font,\n"
                  "or use a larger image (may I recommend %d by %d?)",
                  opt.limit_w, opt.limit_h, filelist_len, opt.limit_w, rec_h);
      }
   }
   else if (opt.limit_h)
   {
      vertical = 1;
      h = opt.limit_h;
      /* calc w */
      for (l = filelist; l; l = l->next)
      {
         file = FEH_FILE(l->data);
         text_area_w = opt.thumb_w;
         /* Calc width of text */
         if (opt.index_show_name)
         {
            gib_imlib_get_text_size(fn, file->name, NULL, &fw, &fh,
                                    IMLIB_TEXT_TO_RIGHT);
            if (fw > text_area_w)
               text_area_w = fw;
         }
         if (opt.index_show_dim)
         {
            gib_imlib_get_text_size(fn,
                                    create_index_dimension_string(1000, 1000),
                                    NULL, &fw, &fh, IMLIB_TEXT_TO_RIGHT);
            if (fw > text_area_w)
               text_area_w = fw;
         }
         if (opt.index_show_size)
         {
            gib_imlib_get_text_size(fn,
                                    create_index_size_string(file->filename),
                                    NULL, &fw, &fh, IMLIB_TEXT_TO_RIGHT);
            if (fw > text_area_w)
               text_area_w = fw;
         }
         if (text_area_w > opt.thumb_w)
            text_area_w += 5;

         if (text_area_w > max_column_w)
            max_column_w = text_area_w;

         if ((y > h - tot_thumb_h))
         {
            y = 0;
            x += max_column_w;
            max_column_w = 0;
         }

         y += tot_thumb_h;
      }
      w = x + text_area_w;
      max_column_w = 0;
   }
   else if (opt.limit_w)
   {
      w = opt.limit_w;
      /* calc h */

      for (l = filelist; l; l = l->next)
      {
         file = FEH_FILE(l->data);
         text_area_w = opt.thumb_w;
         if (opt.index_show_name)
         {
            gib_imlib_get_text_size(fn, file->name, NULL, &fw, &fh,
                                    IMLIB_TEXT_TO_RIGHT);
            if (fw > text_area_w)
               text_area_w = fw;
         }
         if (opt.index_show_dim)
         {
            gib_imlib_get_text_size(fn,
                                    create_index_dimension_string(1000, 1000),
                                    NULL, &fw, &fh, IMLIB_TEXT_TO_RIGHT);
            if (fw > text_area_w)
               text_area_w = fw;
         }
         if (opt.index_show_size)
         {
            gib_imlib_get_text_size(fn,
                                    create_index_size_string(file->filename),
                                    NULL, &fw, &fh, IMLIB_TEXT_TO_RIGHT);
            if (fw > text_area_w)
               text_area_w = fw;
         }

         if (text_area_w > opt.thumb_w)
            text_area_w += 5;

         if ((x > w - text_area_w))
         {
            x = 0;
            y += tot_thumb_h;
         }

         x += text_area_w;
      }
      h = y + tot_thumb_h;
   }

   x = y = 0;

   index_image_width = w;
   index_image_height = h + title_area_h;
   im_main = imlib_create_image(index_image_width, index_image_height);

   if (!im_main)
      eprintf("Imlib error creating index image, are you low on RAM?");

   if (bg_im)
      gib_imlib_blend_image_onto_image(im_main, bg_im,
                                       gib_imlib_image_has_alpha(bg_im), 0, 0,
                                       bg_w, bg_h, 0, 0, w, h, 1, 0, 0);
   else if (trans_bg)
   {
      gib_imlib_image_fill_rectangle(im_main, 0, 0, w, h + title_area_h, 0, 0,
                                     0, 0);
      gib_imlib_image_set_has_alpha(im_main, 1);
   }
   else
   {
      /* Colour the background */
      gib_imlib_image_fill_rectangle(im_main, 0, 0, w, h + title_area_h, 0, 0,
                                     0, 255);
   }

   /* Create the window title at this point */

   if (!opt.title)
      s = estrdup(PACKAGE " [index mode]");
   else
      s = estrdup(feh_printf(opt.title, NULL));

   if (opt.display)
   {
      winwid = winwidget_create_from_image(im_main, s, WIN_TYPE_SINGLE);
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
      D(4, ("About to load image %s\n", file->filename));
      if (feh_load_image(&im_temp, file) != 0)
      {
         if (opt.verbose)
            feh_display_status('.');
         D(4, ("Successfully loaded %s\n", file->filename));
         www = opt.thumb_w;
         hhh = opt.thumb_h;
         ww = gib_imlib_image_get_width(im_temp);
         hh = gib_imlib_image_get_height(im_temp);
         thumbnailcount++;

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

         im_thumb =
            gib_imlib_create_cropped_scaled_image(im_temp, 0, 0, ww, hh, www,
                                                  hhh, 1);
         gib_imlib_free_image_and_decache(im_temp);

         if (opt.alpha)
         {
            DATA8 atab[256];

            D(3, ("Applying alpha options\n"));
            gib_imlib_image_set_has_alpha(im_thumb, 1);
            memset(atab, opt.alpha_level, sizeof(atab));
            gib_imlib_apply_color_modifier_to_rectangle(im_thumb, 0, 0, www,
                                                        hhh, NULL, NULL, NULL,
                                                        atab);
         }

         text_area_w = opt.thumb_w;
         /* Now draw on the info text */
         if (opt.index_show_name)
         {
            gib_imlib_get_text_size(fn, file->name, NULL, &fw_name, &fh,
                                    IMLIB_TEXT_TO_RIGHT);
            if (fw_name > text_area_w)
               text_area_w = fw_name;
         }
         if (opt.index_show_dim)
         {
            gib_imlib_get_text_size(fn, create_index_dimension_string(ww, hh),
                                    NULL, &fw_dim, &fh, IMLIB_TEXT_TO_RIGHT);
            if (fw_dim > text_area_w)
               text_area_w = fw_dim;
         }
         if (opt.index_show_size)
         {
            gib_imlib_get_text_size(fn,
                                    create_index_size_string(file->filename),
                                    NULL, &fw_size, &fh, IMLIB_TEXT_TO_RIGHT);
            if (fw_size > text_area_w)
               text_area_w = fw_size;
         }
         if (text_area_w > opt.thumb_w)
            text_area_w += 5;

         /* offsets for centering text */
         x_offset_name = (text_area_w - fw_name) / 2;
         x_offset_dim = (text_area_w - fw_dim) / 2;
         x_offset_size = (text_area_w - fw_size) / 2;

         if (vertical)
         {
            if (text_area_w > max_column_w)
               max_column_w = text_area_w;
            if (y > h - tot_thumb_h)
            {
               y = 0;
               x += max_column_w;
               max_column_w = 0;
            }
            if (x > w - text_area_w)
               break;
         }
         else
         {
            if (x > w - text_area_w)
            {
               x = 0;
               y += tot_thumb_h;
            }
            if (y > h - tot_thumb_h)
               break;
         }

         if (opt.aspect)
         {
            xxx = x + ((opt.thumb_w - www) / 2);
            yyy = y + ((opt.thumb_h - hhh) / 2);
         }
         else
         {
            /* Ignore the aspect ratio and squash the image in */
            xxx = x;
            yyy = y;
         }

         /* Draw now */
         gib_imlib_blend_image_onto_image(im_main, im_thumb,
                                          gib_imlib_image_has_alpha(im_thumb),
                                          0, 0, www, hhh, xxx, yyy, www, hhh,
                                          1,
                                          gib_imlib_image_has_alpha(im_thumb),
                                          0);

         gib_imlib_free_image_and_decache(im_thumb);

         lines = 0;
         if (opt.index_show_name)
            gib_imlib_text_draw(im_main, fn, NULL, x + x_offset_name,
                                y + opt.thumb_h + (lines++ * (th + 2)) + 2,
                                file->name, IMLIB_TEXT_TO_RIGHT, 255, 255,
                                255, 255);
         if (opt.index_show_dim)
            gib_imlib_text_draw(im_main, fn, NULL, x + x_offset_dim,
                                y + opt.thumb_h + (lines++ * (th + 2)) + 2,
                                create_index_dimension_string(ww, hh),
                                IMLIB_TEXT_TO_RIGHT, 255, 255, 255, 255);
         if (opt.index_show_size)
            gib_imlib_text_draw(im_main, fn, NULL, x + x_offset_size,
                                y + opt.thumb_h + (lines++ * (th + 2)) + 2,
                                create_index_size_string(file->filename),
                                IMLIB_TEXT_TO_RIGHT, 255, 255, 255, 255);

         if (vertical)
            y += tot_thumb_h;
         else
            x += text_area_w;

      }
      else
      {
         if (opt.verbose)
            feh_display_status('x');
         last = l;
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

   if (opt.title_font)
   {
      int fw, fh, fx, fy;
      char *s;

      s = create_index_title_string(thumbnailcount, w, h);
      gib_imlib_get_text_size(title_fn, s, NULL, &fw, &fh, IMLIB_TEXT_TO_RIGHT);
      fx = (index_image_width - fw) >> 1;
      fy = index_image_height - fh - 2;
      gib_imlib_text_draw(im_main, title_fn, NULL, fx, fy, s, IMLIB_TEXT_TO_RIGHT,
                          255, 255, 255, 255);
   }

   if (opt.output && opt.output_file)
   {
      char output_buf[1024];

      if (opt.output_dir)
         snprintf(output_buf, 1024, "%s/%s", opt.output_dir, opt.output_file);
      else
         strncpy(output_buf, opt.output_file, 1024);

      gib_imlib_save_image(im_main, output_buf);
      if (opt.verbose)
      {
         int tw, th;

         tw = gib_imlib_image_get_width(im_main);
         th = gib_imlib_image_get_height(im_main);
         fprintf(stdout, PACKAGE " - File saved as %s\n", output_buf);
         fprintf(stdout,
                 "    - Image is %dx%d pixels and contains %d thumbnails\n",
                 tw, th, thumbnailcount);
      }
   }

   if (!opt.display)
      gib_imlib_free_image_and_decache(im_main);

   free(s);
   D_RETURN_(3);
}


char *
chop_file_from_full_path(char *str)
{
   D_ENTER(4);
   D_RETURN(4, strrchr(str, '/') + 1);
}

static char *
create_index_size_string(char *file)
{
   static char str[50];
   int size = 0;
   double kbs = 0.0;
   struct stat st;

   D_ENTER(4);
   if (stat(file, &st))
      kbs = 0.0;
   else
   {
      size = st.st_size;
      kbs = (double) size / 1000;
   }

   snprintf(str, sizeof(str), "%.2fKb", kbs);
   D_RETURN(4, str);
}

static char *
create_index_dimension_string(int w, int h)
{
   static char str[50];

   D_ENTER(4);
   snprintf(str, sizeof(str), "%dx%d", w, h);
   D_RETURN(4, str);
}

static char *
create_index_title_string(int num, int w, int h)
{
   static char str[50];

   D_ENTER(4);
   snprintf(str, sizeof(str),
            PACKAGE " index - %d thumbnails, %d by %d pixels", num, w, h);
   D_RETURN(4, str);
}
