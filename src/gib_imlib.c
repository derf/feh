/* gib_imlib.c

Copyright (C) 1999,2000 Tom Gilbert.

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

#include "gib_imlib.h"
#include "utils.h"
#include "debug.h"

/*
int
gib_imlib_load_image(Imlib_Image * im, char *filename)
{
   Imlib_Load_Error err;

   imlib_context_set_progress_function(NULL);
   if (!filename)
      return (0);
   *im = imlib_load_image_with_error_return(filename, &err);
   if ((err) || (!im))
   {
      switch (err)
      {
        case IMLIB_LOAD_ERROR_FILE_DOES_NOT_EXIST:
           weprintf("%s - File does not exist", filename);
           break;
        case IMLIB_LOAD_ERROR_FILE_IS_DIRECTORY:
           weprintf("%s - Directory specified for image filename", filename);
           break;
        case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_READ:
           weprintf("%s - No read access to directory", filename);
           break;
        case IMLIB_LOAD_ERROR_NO_LOADER_FOR_FILE_FORMAT:
           weprintf("%s - No Imlib2 loader for that file format", filename);
           break;
        case IMLIB_LOAD_ERROR_PATH_TOO_LONG:
           weprintf("%s - Path specified is too long", filename);
           break;
        case IMLIB_LOAD_ERROR_PATH_COMPONENT_NON_EXISTANT:
           weprintf("%s - Path component does not exist", filename);
           break;
        case IMLIB_LOAD_ERROR_PATH_COMPONENT_NOT_DIRECTORY:
           weprintf("%s - Path component is not a directory", filename);
           break;
        case IMLIB_LOAD_ERROR_PATH_POINTS_OUTSIDE_ADDRESS_SPACE:
           weprintf("%s - Path points outside address space", filename);
           break;
        case IMLIB_LOAD_ERROR_TOO_MANY_SYMBOLIC_LINKS:
           weprintf("%s - Too many levels of symbolic links", filename);
           break;
        case IMLIB_LOAD_ERROR_OUT_OF_MEMORY:
           eprintf("While loading %s - Out of memory", filename);
           break;
        case IMLIB_LOAD_ERROR_OUT_OF_FILE_DESCRIPTORS:
           eprintf("While loading %s - Out of file descriptors", filename);
           break;
        case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_WRITE:
           weprintf("%s - Cannot write to directory", filename);
           break;
        case IMLIB_LOAD_ERROR_OUT_OF_DISK_SPACE:
           weprintf("%s - Cannot write - out of disk space", filename);
           break;
        case IMLIB_LOAD_ERROR_UNKNOWN:
        default:
           weprintf
              ("While loading %s - Unknown error. Attempting to continue",
               filename);
           break;
      }
      return (0);
   }
   return (1);
}
*/

int
gib_imlib_image_get_width(Imlib_Image im)
{
   imlib_context_set_image(im);
   return imlib_image_get_width();
}

int
gib_imlib_image_get_height(Imlib_Image im)
{
   imlib_context_set_image(im);
   return imlib_image_get_height();
}

int
gib_imlib_image_has_alpha(Imlib_Image im)
{
   imlib_context_set_image(im);
   return imlib_image_has_alpha();
}

void
gib_imlib_free_image_and_decache(Imlib_Image im)
{
   imlib_context_set_image(im);
   imlib_free_image_and_decache();
}

void
gib_imlib_free_image(Imlib_Image im)
{
   imlib_context_set_image(im);
   imlib_free_image();
}

const char *
gib_imlib_image_get_filename(Imlib_Image im)
{
   if (im)
   {
      imlib_context_set_image(im);
      return imlib_image_get_filename();
   }
   else
      return NULL;
}

void
gib_imlib_render_image_on_drawable(Drawable d, Imlib_Image im, int x, int y,
                                   char dither, char blend, char alias)
{
   imlib_context_set_image(im);
   imlib_context_set_drawable(d);
   imlib_context_set_anti_alias(alias);
   imlib_context_set_dither(dither);
   imlib_context_set_blend(blend);
   imlib_context_set_angle(0);
   imlib_render_image_on_drawable(x, y);
}

/*
void
gib_imlib_render_image_on_drawable_with_rotation(Drawable d, Imlib_Image im,
                                                 int x, int y, double angle,
                                                 char dither, char blend,
                                                 char alias)
{
   Imlib_Image new_im;

   imlib_context_set_image(im);
   imlib_context_set_anti_alias(alias);
   imlib_context_set_dither(dither);
   imlib_context_set_blend(blend);
   imlib_context_set_angle(angle);
   imlib_context_set_drawable(d);
   new_im = imlib_create_rotated_image(angle);
   imlib_context_set_image(new_im);
   imlib_render_image_on_drawable(x, y);
   imlib_free_image();
}
*/

void
gib_imlib_render_image_on_drawable_at_size(Drawable d, Imlib_Image im, int x,
                                           int y, int w, int h, char dither,
                                           char blend, char alias)
{
   imlib_context_set_image(im);
   imlib_context_set_drawable(d);
   imlib_context_set_anti_alias(alias);
   imlib_context_set_dither(dither);
   imlib_context_set_blend(blend);
   imlib_context_set_angle(0);
   imlib_render_image_on_drawable_at_size(x, y, w, h);
}


/*
void
gib_imlib_render_image_on_drawable_at_size_with_rotation(Drawable d,
                                                         Imlib_Image im,
                                                         int x, int y, int w,
                                                         int h, double angle,
                                                         char dither,
                                                         char blend,
                                                         char alias)
{
   Imlib_Image new_im;

   imlib_context_set_image(im);
   imlib_context_set_drawable(d);
   imlib_context_set_anti_alias(alias);
   imlib_context_set_dither(dither);
   imlib_context_set_blend(blend);
   imlib_context_set_angle(angle);
   new_im = imlib_create_rotated_image(angle);
   imlib_context_set_image(new_im);
   imlib_render_image_on_drawable_at_size(x, y, w, h);
   imlib_free_image_and_decache();
}
*/


void
gib_imlib_render_image_part_on_drawable_at_size(Drawable d, Imlib_Image im,
                                                int sx, int sy, int sw,
                                                int sh, int dx, int dy,
                                                int dw, int dh, char dither,
                                                char blend, char alias)
{
   imlib_context_set_image(im);
   imlib_context_set_drawable(d);
   imlib_context_set_anti_alias(alias);
   imlib_context_set_dither(dither);
   imlib_context_set_blend(blend);
   imlib_context_set_angle(0);
   imlib_render_image_part_on_drawable_at_size(sx, sy, sw, sh, dx, dy, dw,
                                               dh);
}


void
gib_imlib_render_image_part_on_drawable_at_size_with_rotation(Drawable d,
                                                              Imlib_Image im,
                                                              int sx, int sy,
                                                              int sw, int sh,
                                                              int dx, int dy,
                                                              int dw, int dh,
                                                              double angle,
                                                              char dither,
                                                              char blend,
                                                              char alias)
{
   Imlib_Image new_im;

   imlib_context_set_image(im);
   imlib_context_set_drawable(d);
   imlib_context_set_anti_alias(alias);
   imlib_context_set_dither(dither);
   imlib_context_set_angle(angle);
   imlib_context_set_blend(blend);
   new_im = imlib_create_rotated_image(angle);
   imlib_context_set_image(new_im);
   imlib_render_image_part_on_drawable_at_size(sx, sy, sw, sh, dx, dy, dw,
                                               dh);
   imlib_free_image_and_decache();
}


void
gib_imlib_image_fill_rectangle(Imlib_Image im, int x, int y, int w, int h,
                               int r, int g, int b, int a)
{
   imlib_context_set_image(im);
   imlib_context_set_color(r, g, b, a);
   imlib_image_fill_rectangle(x, y, w, h);
}

void
gib_imlib_image_fill_polygon(Imlib_Image im, ImlibPolygon poly, int r, int g,
                             int b, int a, unsigned char alias, int cx,
                             int cy, int cw, int ch)
{
   imlib_context_set_image(im);
   imlib_context_set_color(r, g, b, a);
   imlib_context_set_anti_alias(alias);
   imlib_context_set_cliprect(cx, cy, cw, ch);
   imlib_image_fill_polygon(poly);
   imlib_context_set_cliprect(0, 0, 0, 0);
}

void
gib_imlib_image_draw_polygon(Imlib_Image im, ImlibPolygon poly, int r, int g,
                             int b, int a, unsigned char closed,
                             unsigned char alias, int cx, int cy, int cw,
                             int ch)
{
   imlib_context_set_image(im);
   imlib_context_set_color(r, g, b, a);
   imlib_context_set_anti_alias(alias);
   imlib_context_set_cliprect(cx, cy, cw, ch);
   imlib_image_draw_polygon(poly, closed);
   imlib_context_set_cliprect(0, 0, 0, 0);
}


void
gib_imlib_image_draw_rectangle(Imlib_Image im, int x, int y, int w, int h,
                               int r, int g, int b, int a)
{
   imlib_context_set_image(im);
   imlib_context_set_color(r, g, b, a);
   imlib_image_draw_rectangle(x, y, w, h);
}


void
gib_imlib_text_draw(Imlib_Image im, Imlib_Font fn, gib_style * s, int x,
                    int y, char *text, Imlib_Text_Direction dir, int r, int g,
                    int b, int a)
{
   imlib_context_set_image(im);
   imlib_context_set_font(fn);
   imlib_context_set_direction(dir);
   if (s)
   {
      int min_x = 0, min_y = 0;
      gib_style_bit *bb;
      gib_list *l;

      /* here we shift the draw to accommodate bits with negative offsets,
       * which would be drawn at negative coords otherwise */
      l = s->bits;
      while (l)
      {
         bb = (gib_style_bit *) l->data;
         if (bb)
         {
            if (bb->x_offset < min_x)
               min_x = bb->x_offset;
            if (bb->y_offset < min_y)
               min_y = bb->y_offset;
         }
         l = l->next;
      }
      x -= min_x;
      y -= min_y;

      /* Now draw the bits */
      l = s->bits;
      while (l)
      {
         bb = (gib_style_bit *) l->data;
         if (bb)
         {
            if ((bb->r + bb->g + bb->b + bb->a) == 0)
               imlib_context_set_color(r, g, b, a);
            else
               imlib_context_set_color(bb->r, bb->g, bb->b, bb->a);
            imlib_text_draw(x + bb->x_offset, y + bb->y_offset, text);
         }
         l = l->next;
      }
   }
   else
   {
      imlib_context_set_color(r, g, b, a);
      imlib_text_draw(x, y, text);
   }
}

char **
gib_imlib_list_fonts(int *num)
{
   return imlib_list_fonts(num);
}


void
gib_imlib_get_text_size(Imlib_Font fn, char *text, gib_style * s, int *w,
                        int *h, Imlib_Text_Direction dir)
{

   imlib_context_set_font(fn);
   imlib_context_set_direction(dir);
   imlib_get_text_size(text, w, h);
   if (s)
   {
      gib_style_bit *b;
      int max_x_off = 0, min_x_off = 0, max_y_off = 0, min_y_off = 0;
      gib_list *l;

      l = s->bits;
      while (l)
      {
         b = (gib_style_bit *) l->data;
         if (b)
         {
            if (b->x_offset > max_x_off)
               max_x_off = b->x_offset;
            else if (b->x_offset < min_x_off)
               min_x_off = b->x_offset;
            if (b->y_offset > max_y_off)
               max_y_off = b->y_offset;
            else if (b->y_offset < min_y_off)
               min_y_off = b->y_offset;
         }
         l = l->next;
      }
      if (h)
      {
         *h += max_y_off;
         *h -= min_y_off;
      }
      if (w)
      {
         *w += max_x_off;
         *w -= min_x_off;
      }
   }
}

Imlib_Image gib_imlib_clone_image(Imlib_Image im)
{
   imlib_context_set_image(im);
   return imlib_clone_image();
}

char *
gib_imlib_image_format(Imlib_Image im)
{
   imlib_context_set_image(im);
   return imlib_image_format();
}


void
gib_imlib_blend_image_onto_image(Imlib_Image dest_image,
                                 Imlib_Image source_image, char merge_alpha,
                                 int sx, int sy, int sw, int sh, int dx,
                                 int dy, int dw, int dh, char dither,
                                 char blend, char alias)
{
   imlib_context_set_image(dest_image);
   imlib_context_set_anti_alias(alias);
   imlib_context_set_dither(dither);
   imlib_context_set_blend(blend);
   imlib_context_set_angle(0);
   imlib_blend_image_onto_image(source_image, merge_alpha, sx, sy, sw, sh, dx,
                                dy, dw, dh);
}


/*
void
gib_imlib_blend_image_onto_image_with_rotation(Imlib_Image dest_image,
                                               Imlib_Image source_image,
                                               char merge_alpha, int sx,
                                               int sy, int sw, int sh, int dx,
                                               int dy, int dw, int dh,
                                               double angle, char dither,
                                               char blend, char alias)
{
   imlib_context_set_image(dest_image);
   imlib_context_set_anti_alias(alias);
   imlib_context_set_dither(dither);
   imlib_context_set_blend(blend);
   imlib_context_set_angle(angle);
   imlib_blend_image_onto_image_at_angle(source_image, merge_alpha, sx, sy,
                                         sw, sh, dx, dy, (int) angle,
                                         (int) angle);
   return;
   dw = 0;
   dh = 0;
}
*/

Imlib_Image gib_imlib_create_cropped_scaled_image(Imlib_Image im, int sx,
                                                  int sy, int sw, int sh,
                                                  int dw, int dh, char alias)
{
   imlib_context_set_image(im);
   imlib_context_set_anti_alias(alias);
   return imlib_create_cropped_scaled_image(sx, sy, sw, sh, dw, dh);
}

void
gib_imlib_apply_color_modifier_to_rectangle(Imlib_Image im, int x, int y,
                                            int w, int h, DATA8 * rtab,
                                            DATA8 * gtab, DATA8 * btab,
                                            DATA8 * atab)
{
   Imlib_Color_Modifier cm;

   imlib_context_set_image(im);
   cm = imlib_create_color_modifier();
   imlib_context_set_color_modifier(cm);
   imlib_set_color_modifier_tables(rtab, gtab, btab, atab);
   imlib_apply_color_modifier_to_rectangle(x, y, w, h);
   imlib_free_color_modifier();
}

void
gib_imlib_image_set_has_alpha(Imlib_Image im, int alpha)
{
   imlib_context_set_image(im);
   imlib_image_set_has_alpha(alpha);
}

void
gib_imlib_save_image(Imlib_Image im, char *file)
{
   char *tmp;

   imlib_context_set_image(im);
   tmp = strrchr(file, '.');
   if (tmp)
   {
     char *p, *pp;
     p = strdup(tmp + 1);
     pp = p;
     while(*pp) {
       *pp = tolower(*pp);
       pp++;
     }
     imlib_image_set_format(p);
     free(p);
   }
   imlib_save_image(file);
}

void
gib_imlib_save_image_with_error_return(Imlib_Image im, char *file,
                                       Imlib_Load_Error * error_return)
{
    char *tmp;

    imlib_context_set_image(im);
    tmp = strrchr(file, '.');
    if (tmp) {
        char *p, *pp;
        p = estrdup(tmp + 1);
        pp = p;
        while(*pp) {
            *pp = tolower(*pp);
            pp++;
        }
        imlib_image_set_format(p);
        free(p);
    }
    imlib_save_image_with_error_return(file, error_return);
}

void
gib_imlib_free_font(Imlib_Font fn)
{
   imlib_context_set_font(fn);
   imlib_free_font();
}

void
gib_imlib_image_draw_line(Imlib_Image im, int x1, int y1, int x2, int y2,
                          char make_updates, int r, int g, int b, int a)
{
   imlib_context_set_image(im);
   imlib_context_set_color(r, g, b, a);
   imlib_image_draw_line(x1, y1, x2, y2, make_updates);
}

Imlib_Image gib_imlib_create_rotated_image(Imlib_Image im, double angle)
{
   imlib_context_set_image(im);
   return (imlib_create_rotated_image(angle));
}

void
gib_imlib_image_tile(Imlib_Image im)
{
   imlib_context_set_image(im);
   imlib_image_tile();
}

void
gib_imlib_image_blur(Imlib_Image im, int radius)
{
   imlib_context_set_image(im);
   imlib_image_blur(radius);
}

void
gib_imlib_image_sharpen(Imlib_Image im, int radius)
{
   imlib_context_set_image(im);
   imlib_image_sharpen(radius);
}

void
gib_imlib_line_clip_and_draw(Imlib_Image dest, int x0, int y0, int x1, int y1,
                             int cx, int cy, int cw, int ch, int r, int g,
                             int b, int a)
{
   imlib_context_set_cliprect(cx, cy, cw, ch);
   gib_imlib_image_draw_line(dest, x0, y0, x1, y1, 0, r, g, b, a);
   imlib_context_set_cliprect(0, 0, 0, 0);
}

Imlib_Image
gib_imlib_create_image_from_drawable(Drawable d, Pixmap mask, int x, int y,
                                     int width, int height,
                                     char need_to_grab_x)
{
   imlib_context_set_drawable(d);
   return imlib_create_image_from_drawable(mask, x, y, width, height,
                                           need_to_grab_x);
}

void gib_imlib_parse_color(char *col, int *r, int *g, int *b, int *a)
   {
   gib_list *ll;
   unsigned long cc, rr, gg, bb, aa;
   int len;

   if (col[0] == '#')
   {
      /* #RRGGBBAA style */
      /* skip the '#' */
      col++;
      len = strlen(col);
      if (len == 8)
      {
         cc = (unsigned long) strtoul(col, NULL, 16);
         rr = (cc & 0xff000000) >> 24;
         gg = (cc & 0x00ff0000) >> 16;
         bb = (cc & 0x0000ff00) >> 8;
         aa = (cc & 0x000000ff);
      }
      else if (len == 6)
      {
         cc = (unsigned long) strtoul(col, NULL, 16);
         rr = (cc & 0xff0000) >> 16;
         gg = (cc & 0x00ff00) >> 8;
         bb = (cc & 0x0000ff);
         aa = 255;
      }
      else
      {
         weprintf("unable to parse color %s\n", col);
         return;
      }
   }
   else
   {
      /* r,g,b,a style */
      ll = gib_string_split(col, ",");
      if (!ll)
      {
         weprintf("unable to parse color %s\n", col);
         return;
      }
      len = gib_list_length(ll);
      if (len == 3)
      {
         rr = atoi(ll->data);
         gg = atoi(ll->next->data);
         bb = atoi(ll->next->next->data);
         aa = 255;
      }
      else if (len == 4)
      {
         rr = atoi(ll->data);
         gg = atoi(ll->next->data);
         bb = atoi(ll->next->next->data);
         aa = atoi(ll->next->next->next->data);
      }
      else
      {
         weprintf("unable to parse color %s\n", col);
         return;
      }
   }
   *r = rr;
   *g = gg;
   *b = bb;
   *a = aa;
   }

void
gib_imlib_parse_fontpath(char *path)
{
   gib_list *l, *ll;

   if (!path)
      return;

   l = gib_string_split(path, ":");
   if (!l)
      return;
   ll = l;
   while (ll)
   {
      imlib_add_path_to_font_path((char *) ll->data);
      ll = ll->next;
   }
   gib_list_free_and_data(l);
}

Imlib_Font
gib_imlib_load_font(char *name)
{
   Imlib_Font fn;

   if ((fn = imlib_load_font(name)))
      return fn;
   weprintf("couldn't load font %s, attempting to fall back to fixed.", name);
   if ((fn = imlib_load_font("fixed")))
      return fn;
   weprintf("failed to even load fixed! Attempting to find any font.");
   return imlib_load_font("*");
}

void gib_imlib_image_orientate(Imlib_Image im, int orientation)
{
  imlib_context_set_image(im);
  imlib_image_orientate(orientation);
}

void gib_imlib_image_flip_horizontal(Imlib_Image im)
{
  imlib_context_set_image(im);
  imlib_image_flip_horizontal();
}

void gib_imlib_image_flip_vertical(Imlib_Image im)
{
  imlib_context_set_image(im);
  imlib_image_flip_vertical();
}
