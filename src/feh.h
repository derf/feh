/* feh.h

Copyright (C) 1999-2003 Tom Gilbert.
Copyright (C) 2010-2011 Daniel Friesel.
Copyright (C) 2013      Christopher Hrabak

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

* As of June 2012, HRABAK moved most feh structures to struct.h, and
* combined the function prototypes of these header into feh.h.
* debug.h,events.h,feh_png.h,filelist.h,index.h,signals.h,timers.h,utils.h
* The old <giblib/giblib.h> was renamed imlib.h  and trimmed just for
* feh needs.
*
*/

#ifndef FEH_H
#define FEH_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <X11/keysym.h>
#include <X11/Xresource.h>

#ifdef HAVE_LIBXINERAMA
	#include <X11/extensions/Xinerama.h>
	#include <X11/X.h>
#endif     /* HAVE_LIBXINERAMA */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <dirent.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/wait.h>
#include <math.h>

#include <Imlib2.h>

#include "structs.h"
#include "imlib.h"
#include "menu.h"

#include "getopt.h"

#include "debug.h"

#ifdef HAVE_LIBEXIF
	#include <libexif/exif-data.h>
#endif


/* **********************************************
 *           function typedefs                  *
 ************************************************
 */

typedef void (*sighandler_t) (int);
typedef void (feh_event_handler) (XEvent * ev);  /* from events.h */



/* **********************************************
 *           Global variables                   *
 ************************************************
 */
extern fehoptions      opt;
extern feh_global_vars fgv;          /* 'fgv' stands for FehGlobalVars */

extern feh_event_handler *ev_handler[];       /* from events.h */
extern fehtimer           first_timer;         /* from timers.h */

/* 'LLMD' stands for linkedListMetaData.  'md' stands for metaData.
 *  'ofi' stands for original_file_items, 'rm' was for the old removelist
 */
extern LLMD *feh_md, *rm_md;

/* **********************************************
 *           Global #defines                    *
 ************************************************
 */

#define SLIDESHOW_RELOAD_MAX 4096

#ifndef TRUE
	#define FALSE    0
	#define TRUE     !FALSE
#endif

#ifndef __GNUC__
	# define __attribute__(x)
#endif

#define XY_IN_RECT(x, y, rx, ry, rw, rh) \
(((x) >= (rx)) && ((y) >= (ry)) && ((x) < ((rx) + (rw))) && ((y) < ((ry) + (rh))))

/* All the feh_default fonts are in init_feh_fonts() */

#define INPLACE_EDIT_FLIP   -1
#define INPLACE_EDIT_MIRROR -2

#define ZOOM_MIN 0.002
#define ZOOM_MAX 2000

/* macros for accessing the feh (circular) linkedList (LL) structures LLMD *md */
#define LL_RN_ND(md)              (( md->rn->nd ))
#define LL_CNT(md)                (( md->rn->nd.cnt ))
#define LL_FIRST(md)              (( md->rn->next ))
#define LL_LAST(md)               (( md->rn->prev ))
#define LL_DIRTY(md)              (( md->rn->nd.dirty ))
#define LL_REMOVE(md)             (( md->rn->nd.remove ))
#define LL_CHANGED(md)            (( md->rn->nd.lchange ))
#define INC_RN_CNT(md)            (( md->rn->nd.cnt++ ))
#define DEC_RN_CNT(md)            (( md->rn->nd.cnt-- ))

#define CN_ND(md)                 (( md->cn->nd ))
#define CN_CNT(md)                (( md->cn->nd.cnt ))
#define CN_REMOVE(md)             (( md->cn->nd.remove ))
#define CN_NEXT(md)               (( md->cn->next ))
#define CN_PREV(md)               (( md->cn->prev ))
#define INC_CN_CNT(md)            (( md->cn->nd.cnt++ ))
#define DEC_CN_CNT(md)            (( md->cn->nd.cnt-- ))
/* these access the data member which holds filename, caption, name ext */
#define CN_DATA(md)               (( md->cn->data ))
#define CN_FILENAME(md)           ( ((feh_data *)( md->cn->data))->filename )
#define CN_CAPTION(md)            ( ((feh_data *)( md->cn->data))->caption )
#define CN_NAME(md)               ( ((feh_data *)( md->cn->data))->name )
#define CN_EXT(md)                ( ((feh_data *)( md->cn->data))->ext )
/* and the same things when the "node" is given rather than the LLMD *md */
#define NODE_DATA(n)              ( ((feh_data *)( n->data ) ))
#define NODE_FILENAME(n)          ( ((feh_data *)( n->data))->filename )
#define NODE_CAPTION(n)           ( ((feh_data *)( n->data))->caption )
#define NODE_NAME(n)              ( ((feh_data *)( n->data))->name )
#define NODE_EXT(n)               ( ((feh_data *)( n->data))->ext )
#define NODE_INFO(n)              ( ((feh_data *)( n->data))->info )

#define MAX_ACTIONS  10           /* for opt.actions[] */

/* loop-unwinding replacements for estrjoin() logic
 * Returns a ptr to tail (ie the trailing '\0')
 * WARNING!  s1 is strcpy()'d into b, not strcat()'d.
 * The rest (s2-s7) are (as if they were) strcat()'d.
 */
#define STRCAT_2ITEMS(b,s1,s2)            stpcpy(stpcpy(b,s1),s2)
#define STRCAT_3ITEMS(b,s1,s2,s3)         stpcpy(stpcpy(stpcpy(b,s1),s2),s3)
#define STRCAT_4ITEMS(b,s1,s2,s3,s4)      stpcpy(stpcpy(stpcpy(stpcpy(b,s1),s2),s3),s4)
#define STRCAT_5ITEMS(b,s1,s2,s3,s4,s5)   stpcpy(stpcpy(stpcpy(stpcpy(stpcpy(b,s1),s2),s3),s4),s5)
#define STRCAT_6ITEMS(b,s1,s2,s3,s4,s5,s6)   stpcpy(stpcpy(stpcpy(stpcpy(stpcpy(stpcpy(b,s1),s2),s3),s4),s5),s6)
#define STRCAT_7ITEMS(b,s1,s2,s3,s4,s5,s6,s7)   strcpy(stpcpy(stpcpy(stpcpy(stpcpy(stpcpy(stpcpy(b,s1),s2),s3),s4),s5),s6),s7)

/* to support the newkeyev.c rewrite of the keypress logic */
#define FEH_EXTERNAL_KEYS_FILE      "keys"
#define FEH_GENERAL_HELP            "feh_help.htm"
#define FEH_BINDINGS_MAP            "feh_bindings.map"
#define FEH_BINDINGS_HELP           "feh_bindings.htm"
#define FEH_BINDINGS_HELP_ORIG      "feh_bindings.htm.orig"
#define FEH_BINDINGS_DESC           "feh_bindings.desc"
#define ACT_SUBSET_SIZE   10     /* for BTREE_SEARCH */

#define AltMask    (1<<3)        /* 0x8, see X.h where Mod1Mask==(1<<3)*/

#define ZERO_OUT( s )            memset( s , 0 , sizeof( s) )

	/* a test to see how much HRABAK can reduce the duplication of static strings
	 * stored inside feh executable.  These are all for error msgs.  The #define
	 * name is chosen to match to actual text it stands for.  Just doing this the
	 * text section inside feh executatble was shrunk by 500bytes.
	 */
#define ERR_FAILED             "Failed"
#define ERR_BACKGROUND         "Background"
#define ERR_OPEN_URL           "open url: "
#define ERR_BUTTONS_INVALID    "buttons: Invalid "
#define ERR_CANNOT             "Cannot "
#define ERR_COMBINE            "combine --"
#define ERR_OPEN               "open "
#define ERR_TO_SET_UP_SIGNAL   " to set up signal "

/* **********************************************
 *           function prototypes                *
 ************************************************
 */
#ifdef HAVE_LIBXINERAMA
	void init_xinerama(void);
#endif     /* HAVE_LIBXINERAMA */

#ifdef HAVE_LIBEXIF
	void feh_draw_exif(winwidget w);
#endif


int feh_main_iteration(int block);
void feh_handle_event(XEvent * ev);
void init_all_fgv(void);
void init_feh_fonts(void);
void init_x_and_imlib(void);
void init_multiwindow_mode( LLMD *md );
void init_thumbnail_mode( LLMD *md, int mode );
void init_slideshow_mode(LLMD *md);
void init_list_mode(LLMD *md);
void init_loadables_mode(void);
void init_unloadables_mode(void);
void feh_clean_exit(void);
int  feh_load_image(Imlib_Image * im, feh_data * data);
void show_mini_usage(void);
enum misc_flags test_first_last( LLMD *md , enum misc_flags jmp_code);
void slideshow_change_image(winwidget w,
                            enum misc_flags jmp_code,
                            enum misc_flags render);
char *slideshow_create_name(LLMD *md);
void init_keyevents(void);
void init_buttonbindings(void);
void feh_event_handle_keypress(XEvent * ev);
void feh_action_run( feh_node *node, char *action);
char *format_size(int size);
void shell_escape(char *input, char *ret);
char *feh_printf(char *str, feh_data * data);
void im_weprintf(winwidget w, char *fmt, ...);
void feh_draw_zoom(winwidget w);
void feh_draw_checks(winwidget w);
void cb_slide_timer(void *data);
void cb_reload_timer(void);
void reload_logic( int argc, char **argv, int optind );
int  feh_load_image_char(Imlib_Image * im, char *filename);
void feh_draw_filename(winwidget w);
void feh_draw_actions(winwidget w);
void feh_draw_caption(winwidget w);
void feh_draw_info(winwidget w);
void feh_draw_errstr(winwidget w);
void feh_display_status(char stat);
void real_loadables_mode(LLMD *md);
void feh_reload_image(winwidget w, int resize, int force_new);
void slideshow_save_image(winwidget w);
void feh_edit_inplace(winwidget w, int orientation);
void feh_edit_inplace_lossless(winwidget w, int orientation);
char *build_caption_filename(feh_data * data, short create_dir);
void  feh_list_jump( LLMD *md , enum misc_flags jmp_code );

/*  HRABAK rewrite, robbed from gib_style.c */
ld * feh_wrap_string(char *text, feh_style *s,  int w );
void feh_event_init(void);    /* events.c */

/* from feh_png.c */
int feh_png_read_comments( char *file, bhs_node **aHash);
int feh_png_write_png(Imlib_Image image, char *file, ...);
int feh_png_file_is_png(FILE * fp);

/* from filelist.h */
LLMD * init_LLMD( void );
feh_data *feh_ll_new_data(char *filename);
void feh_ll_free_data(feh_data * data);
void feh_ll_free_md( LLMD *md );
void feh_ll_free_data_info(feh_data_info * info);
int  file_selector_all(const struct dirent *unused);
void add_file_to_filelist_recursively(char *origpath, unsigned char level);
int  feh_ll_load_data_info(feh_data * data, Imlib_Image im);
int  feh_prepare_filelist( LLMD *md );
void feh_write_filelist( LLMD * md, char *list_name, unsigned short int bit_pos );
void feh_read_filelist( LLMD *md , char *filelistfile);
char *feh_absolute_path(char *path);
void stub_save_filelist( void );        /* replaces feh_save_filelist() */

/* void feh_ll_remove_node_from_list( LLMD *md , enum misc_flags delete_flag ); */
/* feh_data_info *feh_ll_new_data_info(void); */

int feh_cmp_name(const void *file1, const void *file2);
int feh_cmp_filename(const void *file1, const void *file2);
int feh_cmp_width(const void *file1, const void *file2);
int feh_cmp_height(const void *file1, const void *file2);
int feh_cmp_pixels(const void *file1, const void *file2);
int feh_cmp_size(const void *file1, const void *file2);
int feh_cmp_format(const void *file1, const void *file2);

/* encapsulate old delete_rm_files() logic here too by adding the delete_flag */
void feh_move_node_to_remove_list(winwidget w,
                                  enum misc_flags delete_flag ,
                                  enum misc_flags w_maint_flag );
/*
 ***********************************************************
 * May 5, 2012 HRABAK combined list.c into feh_ll.c module *
 ***********************************************************
 */

/* from index.h */

void create_index_title_string(winwidget w, thumbmode_data td);
void get_index_string_dim( feh_data *data, int *w, int *h);
void index_calculate_height( LLMD *md, int w, int *h, int *tot_thumb_h);
void index_calculate_width(  LLMD *md, int *w, int h, int *tot_thumb_h);

/* from signals.h */
void setup_signal_handlers();
void feh_handle_timer(void);

/* from timers.h */
double feh_get_time(void);
void feh_remove_timer(char *name);
void feh_add_timer(void (*func) (void *data), void *data, double in, char *name);
void feh_add_unique_timer(void (*func) ,  double in);

/* from utils.h */

void eprintf(char *fmt, ...) __attribute__ ((noreturn));
void weprintf(char *fmt, ...);
char *_estrdup(char *s);
void *_emalloc(size_t n);
void *_ecalloc(size_t n);
void *_erealloc(void *ptr, size_t n);
char *feh_unique_filename(char *path, char *basename);
char *ereadfile(char *path);
void esystem( char *cmd );


bhs_node ** bhs_hash_init(  int size  );
int bhs_hash_get( bhs_node **a , char *key, char *data, int addit );
void bhs_hash_rpt( bhs_node **a , int addit, int ret_code );

wd * feh_string_split( char *s);
char * get_next_awp_line( wd awp[] , int linenum, int i_start, int *i_end );
void feh_style_new_from_ascii(char *file, feh_style *s);
int feh_is_filelist(  char *filename );

#define  SIZE_128        128
#define  SIZE_256        256
#define  SIZE_1024       1024

#define MOBS_ROW_SIZE    SIZE_128
char * mobs( int rows );

/* a list for tmp (mobs) buffer sizing */
#define  MOBS_NSIZE(n)   ( MOBS_ROW_SIZE * (n) )


/* function prototypes for the read_map.c module; used in feh_init and newkeyev */
int read_map_from_disc( void );
int find_config_files( const char * name, char *buf , struct stat *st );

#endif    /* end of #ifndef FEH_H */
