/* structs.h

Copyright (C) 1999-2003 Tom Gilbert.
Copyright (C) 2010-2011 Daniel Friesel.

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

#ifndef STRUCTS_H
#define STRUCTS_H

typedef struct __fehtimer _fehtimer;
typedef _fehtimer *fehtimer;
typedef struct __feh_file feh_file;
typedef struct __feh_file_info feh_file_info;
typedef struct __winwidget _winwidget;
typedef _winwidget *winwidget;
typedef struct __fehoptions fehoptions;
typedef struct __fehkey fehkey;
typedef struct __fehkb fehkb;
typedef struct __fehbutton fehbutton;
typedef struct __fehbb fehbb;


/* May 2012 Stuff from HRABAK */

/* The new feh linkedList structure with a circular link back to the rootnode */
typedef struct __feh_node feh_node;

struct __feh_node  {

  void *data;                     /* holds the feh_file struct, plus feh_file_info .
                                   * For the 0th (root_node), data will point to a static
                                   * string that holds room for the text " 1 of 56" to
                                   * support the --draw-filename param
                                   */
  struct{
    unsigned flag01   : 1;            /*  user definable flags */
    unsigned flag02   : 1;
    unsigned list0    : 1;            /* list numbers 0-9 for the default       */
    unsigned list1    : 1;            /* action keys 0-9.  If no other actions  */
    unsigned list2    : 1;            /* are defined, then pressing the 0-9     */
    unsigned list3    : 1;            /* keys will add this pic to that list    */
    unsigned list4    : 1;            /* number.  Any lists tagged this way     */
    unsigned list5    : 1;            /* will be written at the end,            */
    unsigned list6    : 1;            /* regardless of the --nowrite-filelist   */
    unsigned list7    : 1;            /* option.                                */
    unsigned list8    : 1;
    unsigned list9    : 1;
    unsigned lchange  : 1;            /* has the LL list changed? if so, write it out */
    unsigned tagged   : 1;            /* has one pic been tagged for move?*/
    unsigned flipped  : 1;            /* is the move_list to be flipped, back-front?*/
    unsigned dirty    : 1;            /* do we need to recnt()?*/
    unsigned wide     : 16;           /* pic width in pixels */
    unsigned high     : 16;           /* pic height in pixels */
    unsigned cnt      : 16;           /* stores the (1 of 56) relative position cnts of each node*/
  } nd;                               /* nd stands for node_data */

  feh_node *prev;
  feh_node *next;
} ;

typedef struct __llMetaData LLMD;    /* LLMD stands for linked list MetaData*/

/* note:  don't need to store the file_list_length cause that is in the rn.cnt */
struct __llMetaData{
   feh_node * rn;             /* the root_node (ie the 0th node ) */
   feh_node * cn;             /* the current node (picture) we are on */
} ;


/* As of May 2012, HRABAK cut out ALL the gib_style stuff and replaced it
 * with an array of 5 styles (feh_style struct) inside __fehoptions.
 */
typedef struct _style feh_style;

struct _style {
  struct {
      short r,g,b,a;
      short x_off;
      short y_off;
  } bg;
  struct {
      short r,g,b,a;
      short x_off;
      short y_off;
  } fg;
  /* store the max/mins so we don't have to recalc EVERY time */
/*  short max_x_off;
  short min_x_off;
  short max_y_off;
  short min_y_off; */
} ;

/* enums to support the array of feh_style structs stored inside fehoptions */
enum style_type { STYLE_MENU =0, STYLE_CAPTION, STYLE_WHITE,
                   STYLE_RED, STYLE_YELLOW, STYLE_CNT };


#endif      /* STRUCTS_H */
