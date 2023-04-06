/* gib_imlib.h

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

#ifndef GIB_IMLIB_H
#define GIB_IMLIB_H

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <Imlib2.h>
#include <stdarg.h>
#include <ctype.h>
#include "gib_style.h"

#define GIBCLIP(x, y, w, h, xx, yy, ww, hh) \
{ \
   if ((yy) > y) { h -= (yy) - y; y = (yy); } \
   if ((yy) + hh < y + h) { h -= y + h - ((yy) + (hh)); } \
   if ((xx) > x) { w -= (xx) - x; x = (xx); } \
   if ((xx) + (ww) < x + w) { w -= x + w - ((xx) + (ww)); } \
}

#ifdef __cplusplus
extern "C"
{
#endif

/*
int gib_imlib_load_image(Imlib_Image * im, char *filename);
*/
int gib_imlib_image_get_width(Imlib_Image im);
int gib_imlib_image_get_height(Imlib_Image im);
int gib_imlib_image_has_alpha(Imlib_Image im);
const char *gib_imlib_image_get_filename(Imlib_Image im);
void gib_imlib_free_image_and_decache(Imlib_Image im);
void gib_imlib_render_image_on_drawable(Drawable d, Imlib_Image im, int x,
                                        int y, char dither, char blend,

                                        char alias);
/*
void gib_imlib_render_image_on_drawable_with_rotation(Drawable d,
                                                      Imlib_Image im, int x,
                                                      int y, double angle,

                                                      char dither, char blend,
                                                      char alias);
*/
void gib_imlib_render_image_part_on_drawable_at_size(Drawable d,
                                                     Imlib_Image im, int sx,
                                                     int sy, int sw, int sh,
                                                     int dx, int dy, int dw,
                                                     int dh, char dither,

                                                     char blend, char alias);

void gib_imlib_render_image_part_on_drawable_at_size_with_rotation(Drawable d,
                                                                   Imlib_Image

                                                                   im, int sx,
                                                                   int sy,
                                                                   int sw,
                                                                   int sh,
                                                                   int dx,
                                                                   int dy,
                                                                   int dw,
                                                                   int dh,
                                                                   double
                                                                   angle,
                                                                   char
                                                                   dither,
                                                                   char blend,
                                                                   char
                                                                   alias);

void gib_imlib_image_fill_rectangle(Imlib_Image im, int x, int y, int w,
                                    int h, int r, int g, int b, int a);
void gib_imlib_text_draw(Imlib_Image im, Imlib_Font fn, gib_style * s, int x,
                         int y, char *text, Imlib_Text_Direction dir, int r,
                         int g, int b, int a);
void gib_imlib_get_text_size(Imlib_Font fn, char *text, gib_style * s, int *w,
                             int *h, Imlib_Text_Direction dir);
Imlib_Image gib_imlib_clone_image(Imlib_Image im);
char *gib_imlib_image_format(Imlib_Image im);
char **gib_imlib_list_fonts(int *num);
void gib_imlib_render_image_on_drawable_at_size(Drawable d, Imlib_Image im,
                                                int x, int y, int w, int h,
                                                char dither, char blend,

                                                char alias);
/*
void gib_imlib_render_image_on_drawable_at_size_with_rotation(Drawable d,
                                                              Imlib_Image im,
                                                              int x, int y,

                                                              int w, int h,
                                                              double angle,
                                                              char dither,
                                                              char blend,
                                                              char alias);
*/

void gib_imlib_blend_image_onto_image(Imlib_Image dest_image,
                                      Imlib_Image source_image,
                                      char merge_alpha, int sx, int sy,
                                      int sw, int sh, int dx, int dy, int dw,
                                      int dh, char dither, char blend,

                                      char alias);

/*
void gib_imlib_blend_image_onto_image_with_rotation(Imlib_Image dest_image,
                                                    Imlib_Image source_image,
                                                    char merge_alpha, int sx,
                                                    int sy, int sw, int sh,
                                                    int dx, int dy, int dw,
                                                    int dh, double angle,

                                                    char dither, char blend,
                                                    char alias);
*/
Imlib_Image gib_imlib_create_cropped_scaled_image(Imlib_Image im, int sx,
                                                  int sy, int sw, int sh,
                                                  int dw, int dh, char alias);
void gib_imlib_apply_color_modifier_to_rectangle(Imlib_Image im, int x, int y,
                                                 int w, int h, DATA8 * rtab,
                                                 DATA8 * gtab, DATA8 * btab,
                                                 DATA8 * atab);
void gib_imlib_image_set_has_alpha(Imlib_Image im, int alpha);
void gib_imlib_save_image(Imlib_Image im, char *file);
void gib_imlib_save_image_with_error_return(Imlib_Image im, char *file,
                                            Imlib_Load_Error * error_return);
void gib_imlib_free_font(Imlib_Font fn);
void gib_imlib_free_image(Imlib_Image im);
void gib_imlib_image_draw_line(Imlib_Image im, int x1, int y1, int x2, int y2,
                               char make_updates, int r, int g, int b, int a);
void gib_imlib_image_set_has_alpha(Imlib_Image im, int alpha);
void gib_imlib_free_font(Imlib_Font fn);
Imlib_Image gib_imlib_create_rotated_image(Imlib_Image im, double angle);
void gib_imlib_image_tile(Imlib_Image im);
void gib_imlib_image_blur(Imlib_Image im, int radius);
void gib_imlib_image_sharpen(Imlib_Image im, int radius);
void gib_imlib_image_draw_rectangle(Imlib_Image im, int x, int y, int w,
                                    int h, int r, int g, int b, int a);
void gib_imlib_line_clip_and_draw(Imlib_Image dest, int x0, int y0, int x1,
                                  int y1, int cx, int cy, int cw, int ch,
                                  int r, int g, int b, int a);
void gib_imlib_image_fill_polygon(Imlib_Image im, ImlibPolygon poly, int r,
                                  int g, int b, int a, unsigned char alias,
                                  int cx, int cy, int cw, int ch);
void gib_imlib_image_draw_polygon(Imlib_Image im, ImlibPolygon poly, int r,
                                  int g, int b, int a, unsigned char closed,
                                  unsigned char alias, int cx, int cy, int cw,

                                  int ch);
Imlib_Image gib_imlib_create_image_from_drawable(Drawable d, Pixmap mask,
                                                 int x, int y, int width,

                                                 int height,
                                                 char need_to_grab_x);
void gib_imlib_parse_color(char *col, int *r, int *g, int *b, int *a);
void gib_imlib_parse_fontpath(char *path);
Imlib_Font gib_imlib_load_font(char *name);
void gib_imlib_image_orientate(Imlib_Image im, int orientation);
void gib_imlib_image_flip_horizontal(Imlib_Image im);
void gib_imlib_image_flip_vertical(Imlib_Image im);

#ifdef __cplusplus
}
#endif


#endif
