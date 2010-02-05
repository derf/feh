/* thumbnail.c

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
#include "thumbnail.h"
#include "md5.h"
#include "feh_png.h"

static char *create_index_dimension_string(int w, int h);
static char *create_index_size_string(char *file);
static char *create_index_title_string(int num, int w, int h);
static gib_list *thumbnails = NULL;

static thumbmode_data td;

/* TODO Break this up a bit ;) */
/* TODO s/bit/lot */
void
init_thumbnail_mode(void)
{
	/* moved to thumbnail_data:
		 Imlib_Image im_main;
		 Imlib_Image bg_im = NULL;
		 Imlib_Font fn = NULL;
		 Imlib_Font title_fn = NULL;

		 int w = 800, h = 600;
		 int bg_w = 0, bg_h = 0;

		 int text_area_w, text_area_h;
		 int max_column_w = 0;
	*/


   Imlib_Image im_temp;
   int ww = 0, hh = 0, www, hhh, xxx, yyy;
   int x = 0, y = 0;
   winwidget winwid = NULL;
   Imlib_Image im_thumb = NULL;
   unsigned char trans_bg = 0;
   int title_area_h = 0;
   int tw = 0, th = 0;
   int fw_name, fw_size, fw_dim, fh;
   int thumbnailcount = 0;
   feh_file *file = NULL;
   gib_list *l, *last = NULL;
   int lines;
   int index_image_width, index_image_height;
   int x_offset_name = 0, x_offset_dim = 0, x_offset_size = 0;
   char *s;

	 /* initialize thumbnail mode data */
	 td.im_main = NULL;
	 td.im_bg = NULL;
	 td.font_main = NULL;
	 td.font_title = NULL;

	 td.w = 640;
	 td.h = 480;
	 td.bg_w = 0;
	 td.bg_h = 0;
	 td.thumb_tot_h = 0;
	 td.text_area_w = 0;
	 td.text_area_h = 0;

	 td.vertical = 0;
	 td.max_column_w = 0;

   D_ENTER(3);

   mode = "thumbnail";

   td.font_main = gib_imlib_load_font(opt.font);

   if (opt.title_font)
   {
      int fh, fw;

      td.font_title = gib_imlib_load_font(opt.title_font);
      gib_imlib_get_text_size(td.font_title, "W", NULL, &fw, &fh, IMLIB_TEXT_TO_RIGHT);
      title_area_h = fh + 4;
   }
   else
      td.font_title = imlib_load_font(DEFAULT_FONT_TITLE);

   if ((!td.font_main) || (!td.font_title))
      eprintf("Error loading fonts");

   /* Work out how tall the font is */
   gib_imlib_get_text_size(td.font_main, "W", NULL, &tw, &th, IMLIB_TEXT_TO_RIGHT);
   /* For now, allow room for the right number of lines with small gaps */
   td.text_area_h =
      ((th + 2) * (opt.index_show_name + opt.index_show_size +
                   opt.index_show_dim)) + 5;

   /* This includes the text area for index data */
   td.thumb_tot_h = opt.thumb_h + td.text_area_h;

   /* Use bg image dimensions for default size */
   if (opt.bg && opt.bg_file)
   {
      if (!strcmp(opt.bg_file, "trans"))
         trans_bg = 1;
      else
      {

         D(3, ("Time to apply a background to blend onto\n"));
         if (feh_load_image_char(&td.im_bg, opt.bg_file) != 0)
         {
            td.bg_w = gib_imlib_image_get_width(td.im_bg);
            td.bg_h = gib_imlib_image_get_height(td.im_bg);
         }
      }
   }

	 /* figure out geometry for the main window and entries */
	 feh_thumbnail_calculate_geometry();

   index_image_width = td.w;
   index_image_height = td.h + title_area_h;
   td.im_main = imlib_create_image(index_image_width, index_image_height);

   if (!td.im_main)
      eprintf("Imlib error creating index image, are you low on RAM?");

   if (td.im_bg)
      gib_imlib_blend_image_onto_image(td.im_main, td.im_bg,
                                       gib_imlib_image_has_alpha(td.im_bg), 0, 0,
                                       td.bg_w, td.bg_h, 0, 0, td.w, td.h, 1, 0, 0);
   else if (trans_bg)
   {
      gib_imlib_image_fill_rectangle(td.im_main, 0, 0, td.w, td.h + title_area_h, 0, 0,
                                     0, 0);
      gib_imlib_image_set_has_alpha(td.im_main, 1);
   }
   else
   {
      /* Colour the background */
      gib_imlib_image_fill_rectangle(td.im_main, 0, 0, td.w, td.h + title_area_h, 0, 0,
                                     0, 255);
   }

   /* Create title now */

   if (!opt.title)
      s = estrdup(PACKAGE " [thumbnail mode]");
   else
      s = estrdup(feh_printf(opt.title, NULL));

   if (opt.display)
   {
      winwid = winwidget_create_from_image(td.im_main, s, WIN_TYPE_THUMBNAIL);
      winwidget_show(winwid);
   }

	 /* make sure we have an ~/.thumbnails/normal directory for storing
			permanent thumbnails */
	 td.cache_thumbnails = feh_thumbnail_setup_thumbnail_dir();
   td.cache_thumbnails = opt.cache_thumbnails;

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
			/*      if (feh_load_image(&im_temp, file) != 0) */
			if (feh_thumbnail_get_thumbnail(&im_temp, file) != 0)
			{
         if (opt.verbose)
            feh_display_status('.');
         D(4, ("Successfully loaded %s\n", file->filename));
         www = opt.thumb_w;
         hhh = opt.thumb_h;
         ww = gib_imlib_image_get_width(im_temp);
         hh = gib_imlib_image_get_height(im_temp);
         thumbnailcount++;
         if (gib_imlib_image_has_alpha(im_temp))
            imlib_context_set_blend(1);
         else
            imlib_context_set_blend(0);

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

         td.text_area_w = opt.thumb_w;
         /* Now draw on the info text */
         if (opt.index_show_name)
         {
            gib_imlib_get_text_size(td.font_main, file->name, NULL, &fw_name, &fh,
                                    IMLIB_TEXT_TO_RIGHT);
            if (fw_name > td.text_area_w)
               td.text_area_w = fw_name;
         }
         if (opt.index_show_dim)
         {
            gib_imlib_get_text_size(td.font_main, create_index_dimension_string(ww, hh),
                                    NULL, &fw_dim, &fh, IMLIB_TEXT_TO_RIGHT);
            if (fw_dim > td.text_area_w)
               td.text_area_w = fw_dim;
         }
         if (opt.index_show_size)
         {
            gib_imlib_get_text_size(td.font_main,
                                    create_index_size_string(file->filename),
                                    NULL, &fw_size, &fh, IMLIB_TEXT_TO_RIGHT);
            if (fw_size > td.text_area_w)
               td.text_area_w = fw_size;
         }
         if (td.text_area_w > opt.thumb_w)
            td.text_area_w += 5;

         /* offsets for centering text */
         x_offset_name = (td.text_area_w - fw_name) / 2;
         x_offset_dim = (td.text_area_w - fw_dim) / 2;
         x_offset_size = (td.text_area_w - fw_size) / 2;

         if (td.vertical)
         {
					 if (td.text_area_w > td.max_column_w)
               td.max_column_w = td.text_area_w;
            if (y > td.h - td.thumb_tot_h)
            {
               y = 0;
               x += td.max_column_w;
               td.max_column_w = 0;
            }
            if (x > td.w - td.text_area_w)
               break;
         }
         else
         {
            if (x > td.w - td.text_area_w)
            {
               x = 0;
               y += td.thumb_tot_h;
            }
            if (y > td.h - td.thumb_tot_h)
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
         gib_imlib_blend_image_onto_image(td.im_main, im_thumb,
                                          gib_imlib_image_has_alpha(im_thumb),
                                          0, 0, www, hhh, xxx, yyy, www, hhh,
                                          1,
                                          gib_imlib_image_has_alpha(im_thumb),
                                          0);

         thumbnails =
            gib_list_add_front(thumbnails,
                               feh_thumbnail_new(file, xxx, yyy, www, hhh));

         gib_imlib_free_image_and_decache(im_thumb);

         lines = 0;
         if (opt.index_show_name)
            gib_imlib_text_draw(td.im_main, td.font_main, NULL, x + x_offset_name,
                                y + opt.thumb_h + (lines++ * (th + 2)) + 2,
                                file->name, IMLIB_TEXT_TO_RIGHT, 255, 255,
                                255, 255);
         if (opt.index_show_dim)
            gib_imlib_text_draw(td.im_main, td.font_main, NULL, x + x_offset_dim,
                                y + opt.thumb_h + (lines++ * (th + 2)) + 2,
                                create_index_dimension_string(ww, hh),
                                IMLIB_TEXT_TO_RIGHT, 255, 255, 255, 255);
         if (opt.index_show_size)
            gib_imlib_text_draw(td.im_main, td.font_main, NULL, x + x_offset_size,
                                y + opt.thumb_h + (lines++ * (th + 2)) + 2,
                                create_index_size_string(file->filename),
                                IMLIB_TEXT_TO_RIGHT, 255, 255, 255, 255);

         if (td.vertical)
            y += td.thumb_tot_h;
         else
            x += td.text_area_w;
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

      s = create_index_title_string(thumbnailcount, td.w, td.h);
      gib_imlib_get_text_size(td.font_title, s, NULL, &fw, &fh, IMLIB_TEXT_TO_RIGHT);
      fx = (index_image_width - fw) >> 1;
      fy = index_image_height - fh - 2;
      gib_imlib_text_draw(td.im_main, td.font_title, NULL, fx, fy, s, IMLIB_TEXT_TO_RIGHT,
                          255, 255, 255, 255);
   }

   if (opt.output && opt.output_file)
   {
      char output_buf[1024];

      if (opt.output_dir)
         snprintf(output_buf, 1024, "%s/%s", opt.output_dir, opt.output_file);
      else
         strncpy(output_buf, opt.output_file, 1024);
      gib_imlib_save_image(td.im_main, output_buf);
      if (opt.verbose)
      {
         int tw, th;

         tw = gib_imlib_image_get_width(td.im_main);
         th = gib_imlib_image_get_height(td.im_main);
         fprintf(stdout, PACKAGE " - File saved as %s\n", output_buf);
         fprintf(stdout,
                 "    - Image is %dx%d pixels and contains %d thumbnails\n",
                 tw, th, thumbnailcount);
      }
   }

   if (!opt.display)
      gib_imlib_free_image_and_decache(td.im_main);

   free(s);
   D_RETURN_(3);
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

feh_thumbnail *
feh_thumbnail_new(feh_file * file, int x, int y, int w, int h)
{
   feh_thumbnail *thumb;

   D_ENTER(4);

   thumb = (feh_thumbnail *) emalloc(sizeof(feh_thumbnail));
   thumb->x = x;
   thumb->y = y;
   thumb->w = w;
   thumb->h = h;
   thumb->file = file;
   thumb->exists = 1;

   D_RETURN(4, thumb);
}

feh_file *
feh_thumbnail_get_file_from_coords(int x, int y)
{
   gib_list *l;
   feh_thumbnail *thumb;

   D_ENTER(4);

   for (l = thumbnails; l; l = l->next)
   {
      thumb = FEH_THUMB(l->data);
      if (XY_IN_RECT(x, y, thumb->x, thumb->y, thumb->w, thumb->h))
      {
         if (thumb->exists)
         {
            D_RETURN(4, thumb->file);
         }
      }
   }
   D(4, ("No matching %d %d\n", x, y));
   D_RETURN(4, NULL);
}

feh_thumbnail *
feh_thumbnail_get_thumbnail_from_coords(int x, int y)
{
   gib_list *l;
   feh_thumbnail *thumb;

   D_ENTER(4);

   for (l = thumbnails; l; l = l->next)
   {
      thumb = FEH_THUMB(l->data);
      if (XY_IN_RECT(x, y, thumb->x, thumb->y, thumb->w, thumb->h))
      {
         if (thumb->exists)
         {
            D_RETURN(4, thumb);
         }
      }
   }
   D(4, ("No matching %d %d\n", x, y));
   D_RETURN(4, NULL);
}

feh_thumbnail *
feh_thumbnail_get_from_file(feh_file * file)
{
   gib_list *l;
   feh_thumbnail *thumb;

   D_ENTER(4);

   for (l = thumbnails; l; l = l->next)
   {
      thumb = FEH_THUMB(l->data);
      if (thumb->file == file)
      {
         if (thumb->exists)
         {
            D_RETURN(4, thumb);
         }
      }
   }
   D(4, ("No match\n"));
   D_RETURN(4, NULL);
}


void
feh_thumbnail_mark_removed(feh_file * file, int deleted)
{
   feh_thumbnail *thumb;
   winwidget w;

   D_ENTER(4);

   thumb = feh_thumbnail_get_from_file(file);
   if (thumb)
   {
      w = winwidget_get_first_window_of_type(WIN_TYPE_THUMBNAIL);
      if (w)
      {
         td.font_main = imlib_load_font(DEFAULT_FONT_TITLE);
         if (deleted)
            gib_imlib_image_fill_rectangle(w->im, thumb->x, thumb->y,
                                           thumb->w, thumb->h, 255, 0, 0,
                                           150);
         else
            gib_imlib_image_fill_rectangle(w->im, thumb->x, thumb->y,
                                           thumb->w, thumb->h, 0, 0, 255,
                                           150);
         if (td.font_main)
         {
            int tw, th;

            gib_imlib_get_text_size(td.font_main, "X", NULL, &tw, &th, IMLIB_TEXT_TO_RIGHT);
            gib_imlib_text_draw(w->im, td.font_main, NULL, thumb->x + ((thumb->w - tw) / 2),
                                thumb->y + ((thumb->h - th) / 2), "X",
                                IMLIB_TEXT_TO_RIGHT, 205, 205, 50, 255);
         }
         else
            weprintf(DEFAULT_FONT_TITLE);
         winwidget_render_image(w, 0, 1);
      }
      thumb->exists = 0;
   }
   D_RETURN_(4);
}

void
feh_thumbnail_calculate_geometry(void)
{
	gib_list *l;
	feh_file *file;

	int x = 0, y = 0;
	int fw, fh;

   if (!opt.limit_w && !opt.limit_h)
   {
      if (td.im_bg)
      {
         if (opt.verbose)
            fprintf(stdout,
                    PACKAGE " - No size restriction specified for index.\n"
                    " You did specify a background however, so the\n"
                    " index size has defaulted to the size of the image\n");
         opt.limit_w = td.bg_w;
         opt.limit_h = td.bg_h;
      }
      else
      {
         if (opt.verbose)
            fprintf(stdout,
                    PACKAGE " - No size restriction specified for index.\n"
                    " Using defaults (width limited to 640)\n");
         opt.limit_w = 640;
      }
   }


   /* Here we need to whiz through the files, and look at the filenames and
      info in the selected font, work out how much space we need, and
      calculate the size of the image we will require */

   if (opt.limit_w && opt.limit_h)
   {
      int rec_h = 0;

      td.w = opt.limit_w;
      td.h = opt.limit_h;

      /* Work out if this is big enough, and give a warning if not */

      /* Pretend we are limiting width by that specified, loop through, and
         see it we fit in the height specified. If not, continue the loop,
         and recommend the final value instead. Carry on and make the index
         anyway. */

      for (l = filelist; l; l = l->next)
      {
         file = FEH_FILE(l->data);
         td.text_area_w = opt.thumb_w;
         if (opt.index_show_name)
         {
            gib_imlib_get_text_size(td.font_main, file->name, NULL, &fw, &fh,
                                    IMLIB_TEXT_TO_RIGHT);
            if (fw > td.text_area_w)
               td.text_area_w = fw;
         }
         if (opt.index_show_dim)
         {
            gib_imlib_get_text_size(td.font_main,
                                    create_index_dimension_string(1000, 1000),
                                    NULL, &fw, &fh, IMLIB_TEXT_TO_RIGHT);
            if (fw > td.text_area_w)
               td.text_area_w = fw;
         }
         if (opt.index_show_size)
         {
            gib_imlib_get_text_size(td.font_main,
                                    create_index_size_string(file->filename),
                                    NULL, &fw, &fh, IMLIB_TEXT_TO_RIGHT);
            if (fw > td.text_area_w)
               td.text_area_w = fw;
         }
         if (td.text_area_w > opt.thumb_w)
            td.text_area_w += 5;

         if ((x > td.w - td.text_area_w))
         {
            x = 0;
            y += td.thumb_tot_h;
         }

         x += td.text_area_w;
      }
      rec_h = y + td.thumb_tot_h;

      if (td.h < rec_h)
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
      td.vertical = 1;
      td.h = opt.limit_h;
      /* calc w */
      for (l = filelist; l; l = l->next)
      {
         file = FEH_FILE(l->data);
         td.text_area_w = opt.thumb_w;
         /* Calc width of text */
         if (opt.index_show_name)
         {
            gib_imlib_get_text_size(td.font_main, file->name, NULL, &fw, &fh,
                                    IMLIB_TEXT_TO_RIGHT);
            if (fw > td.text_area_w)
               td.text_area_w = fw;
         }
         if (opt.index_show_dim)
         {
            gib_imlib_get_text_size(td.font_main,
                                    create_index_dimension_string(1000, 1000),
                                    NULL, &fw, &fh, IMLIB_TEXT_TO_RIGHT);
            if (fw > td.text_area_w)
               td.text_area_w = fw;
         }
         if (opt.index_show_size)
         {
            gib_imlib_get_text_size(td.font_main,
                                    create_index_size_string(file->filename),
                                    NULL, &fw, &fh, IMLIB_TEXT_TO_RIGHT);
            if (fw > td.text_area_w)
               td.text_area_w = fw;
         }
         if (td.text_area_w > opt.thumb_w)
            td.text_area_w += 5;

         if (td.text_area_w > td.max_column_w)
            td.max_column_w = td.text_area_w;

         if ((y > td.h - td.thumb_tot_h))
         {
            y = 0;
            x += td.max_column_w;
            td.max_column_w = 0;
         }

         y += td.thumb_tot_h;
      }
      td.w = x + td.text_area_w;
      td.max_column_w = 0;
   }
   else if (opt.limit_w)
   {
      td.w = opt.limit_w;
      /* calc h */

      for (l = filelist; l; l = l->next)
      {
         file = FEH_FILE(l->data);
         td.text_area_w = opt.thumb_w;
         if (opt.index_show_name)
         {
            gib_imlib_get_text_size(td.font_main, file->name, NULL, &fw, &fh,
                                    IMLIB_TEXT_TO_RIGHT);
            if (fw > td.text_area_w)
               td.text_area_w = fw;
         }
         if (opt.index_show_dim)
         {
            gib_imlib_get_text_size(td.font_main,
                                    create_index_dimension_string(1000, 1000),
                                    NULL, &fw, &fh, IMLIB_TEXT_TO_RIGHT);
            if (fw > td.text_area_w)
               td.text_area_w = fw;
         }
         if (opt.index_show_size)
         {
            gib_imlib_get_text_size(td.font_main,
                                    create_index_size_string(file->filename),
                                    NULL, &fw, &fh, IMLIB_TEXT_TO_RIGHT);
            if (fw > td.text_area_w)
               td.text_area_w = fw;
         }

         if (td.text_area_w > opt.thumb_w)
            td.text_area_w += 5;

         if ((x > td.w - td.text_area_w))
         {
            x = 0;
            y += td.thumb_tot_h;
         }

         x += td.text_area_w;
      }
      td.h = y + td.thumb_tot_h;
   }
}

int
feh_thumbnail_get_thumbnail(Imlib_Image *image, feh_file *file)
{
	int status = 0;
	char *thumb_file = NULL, *uri = NULL;

	if (!file || !file->filename)
		return(0);

	if (td.cache_thumbnails)
	{
		uri = feh_thumbnail_get_name_uri(file->filename);
		thumb_file = feh_thumbnail_get_name(uri);
		status = feh_thumbnail_get_generated(image, file, thumb_file, uri);

		if (!status)
			status = feh_thumbnail_generate(image, file, thumb_file, uri);
		
    printf("uri is %s, thumb_file is %s\n", uri, thumb_file);
		free(uri);
		free(thumb_file);
	}
	else
		status = feh_load_image(image, file);

	return status;
}

char*
feh_thumbnail_get_name(char *uri)
{
	char *home = NULL, *thumb_file = NULL, *md5_name = NULL;

	/* FIXME: make sure original file isn't under ~/.thumbnails */



	md5_name = feh_thumbnail_get_name_md5(uri);

	home = getenv("HOME");
	if (home)
	{
		thumb_file = estrjoin("/", home, ".thumbnails/normal", md5_name, NULL);
	}

	free(md5_name);

	return thumb_file;
}

char*
feh_thumbnail_get_name_uri(char *name)
{
	char *cwd, *uri = NULL;

	/* FIXME: what happends with http, https, and ftp? MTime etc */
	if ((strncmp(name, "http://", 7) != 0) &&
			(strncmp(name, "https://", 8) != 0) &&
			(strncmp(name, "ftp://", 6) != 0) &&
			(strncmp(name, "file://", 7) != 0))
	{

		/* make sure it's an absoulte path */
		/* FIXME: add support for ~, need to investigate if it's expanded
			 somewhere else before adding (unecessary) code */
		if (name[0] != '/') {
			cwd = getcwd(NULL, 0);
			uri = estrjoin("/", "file:/", cwd, name, NULL);
			free(cwd);
		} else {
			uri = estrjoin(NULL, "file://", name, NULL);
		}
	}
	else
		uri = estrdup(name);
	
	return uri;
}

char*
feh_thumbnail_get_name_md5(char *uri)
{
	int i;
	char *pos, *md5_name;
	md5_state_t pms;
	md5_byte_t digest[16];

	/* generate the md5 sum */
	md5_init(&pms);
	md5_append(&pms, uri, strlen(uri));
	md5_finish(&pms, digest);

	/* print the md5 as hex to a string */
	md5_name = emalloc(32 + 4 + 1 * sizeof(char)); /* md5 + .png + '\0' */
	for (i = 0, pos = md5_name; i < 16; i++, pos += 2)
	{
		sprintf(pos, "%02x", digest[i]);
	}
	sprintf(pos, ".png");

	return md5_name;
}

int
feh_thumbnail_generate(Imlib_Image *image, feh_file *file, char *thumb_file, char *uri)
{
	int w, h, thumb_w, thumb_h;
	char *c_mtime;
	Imlib_Image im_temp;
	struct stat sb;

	if (feh_load_image(&im_temp, file) != 0)
	{
		w = gib_imlib_image_get_width(im_temp);
		h = gib_imlib_image_get_height(im_temp);
		thumb_w = 128;
		thumb_h = 128;

		if ((w > 128) || (h > 128))
		{
			double ratio = (double) w / h;
			if (ratio > 1.0)
				thumb_h = 128 / ratio;
			else if (ratio != 1.0)
				thumb_w = 128 * ratio;
		}

		*image = gib_imlib_create_cropped_scaled_image(im_temp, 0, 0, w, h,
																									 thumb_w, thumb_h, 1);

		if (!stat(file->filename, &sb))
		{
      char c_mtime[256];
			sprintf(c_mtime, "%d", sb.st_mtime);
			feh_png_write_png(*image, thumb_file,
												"Thumb::URI", uri, "Thumb::MTime", c_mtime);
		}

		gib_imlib_free_image_and_decache(im_temp);

		return(1);
	}

	return(0);
}

int
feh_thumbnail_get_generated(Imlib_Image *image, feh_file *file, char *thumb_file, char *uri)
{
	struct stat sb;
	char *c_mtime;
	time_t mtime = 0;
	gib_hash *hash;

	if (!stat(file->filename, &sb))
	{
		hash = feh_png_read_comments(thumb_file);
		if (hash != NULL)
		{
			c_mtime = (char*) gib_hash_get(hash, "Thumb::MTime");
			if (c_mtime != NULL)
				mtime = (time_t) strtol(c_mtime, NULL,10);
			gib_hash_free_and_data(hash);
		}

		/* FIXME: should we bother about Thumb::URI? */
		if (mtime == sb.st_mtime)
		{
      feh_load_image_char(image, thumb_file);

			return(1);
		}
	}

	return(0);
}

int
feh_thumbnail_setup_thumbnail_dir(void)
{
	int status = 0;
	struct stat sb;
	char *dir, *dir_thumbnails, *home;

	home = getenv("HOME");
	if (home != NULL)
	{
		dir = estrjoin("/", home, ".thumbnails/normal", NULL);

		if (!stat(dir, &sb)) {
			if (S_ISDIR(sb.st_mode))
				status = 1;				
			else
				weprintf("%s should be a directory", dir);
		} else {
			dir_thumbnails = estrjoin("/", home, ".thumbnails", NULL);

			if (stat(dir_thumbnails, &sb) != 0)
			{
				if (mkdir(dir_thumbnails, 0700) == -1)
					weprintf("unable to create %s directory", dir_thumbnails);
			}

			if (mkdir(dir, 0700) == -1)
				weprintf("unable to create %s directory", dir);
			else
				status = 1;
		}
	}

	return status;
}
