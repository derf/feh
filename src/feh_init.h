/* feh_init.h

Copyright (C) 2012      Christopher Hrabak

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

* June 2012: Hrabak rewrote the keyevents.c logic and this header file
* supports that rewrite.  The replacement is called newkeyev.c.  Jump to
* line 140 to skip all this babbel.
*
* This header file contains all the default keybinding assignments.  Edit
* this file to make changes to the DEFAULT bindings.  You still would edit
* the "keys" file to reassign runtime key bindings.  Note that the format
* of the external "keys" file has changed to match this (new) internal format.
*
* Key bindings fall into one of three groups: menu, move or feh.  All menu
* bindings begin with "menu_" ,  move bindings ( activated with the -DHRABAK
* compile directive) begin with "move_" and the rest are "normal" feh bindings.
* Keybindings are unique within each mode; menu_, move_ and feh_, but may be
* duplicated between modes.
* Caption editing bindings are NOT maintained here. They are hardcoded.
*
* The changes Hrabak made can be summarized as follows.
*
* 1)The old keyevents.c logic processed a runtime keypress by exhaustive-
*   ly seaching to see if the keypress code was defined inside the
*   __fehkb structure of 60 members and three keys defined for each.
*   So for a keyevent that was found to match the 60th member required
*   60x3x2=360 tests.
*
* 2)The Hrabak change makes an array of functions ptrs to call for
*   each keypress, mapped to and sorted by the keycode that is the
*   concatenation of the keysym and state as reported by xev.
*
* 3)Each runtime keypress is then bsearch()'d inside this array looking
*   for the UNIQUE sequence of keysym+state.  So the old keyevents that
*   defined up to 180 keysym+state combos would require only 8 tests
*   to reach deepest code.  This new fmt no longer limits you to 3
*   keybindings per action.  Any number can be defined.
*
* 4)But the major benefit of this change comes from the ease of adding new
*   keypress codes and the functions they invoke.  New functionality can be
*   added by doing the following...
*    - make up a name for your new action  (ex  "my_action" )
*    - add that action and the keypress codes that activate it to this
*      feh_init.h file.  (ex  "my_action"     ":A-Return" , )
*      will call my_action for  <Alt>+Return
*    - code the function to carry out this action and declare it as ...
*          void stub_my_action( void );
*      where the name of your new function is just the name of the
*      new action (my_action) prepended with the word "stub_"
*    - compile and enjoy.  All the headers, function prototypes,
*      structs and arrays are created automatically.
*
* Hrabak relegated the parsing of keybindings to a standalone executable
* called feh_init; the idea being that keybinding changes are infrequent.
* feh_init makes a feh_bindings.map (a binary file) that feh loads without
* any further checks.  Whenever the external "keys" file changes, feh shells
* out to rerun feh_init, then uses the updated .map file.  This way, feh is
* relieved of all that parsing code. feh_init also creates feh_bindings.htm;
* a help file, viewable within feh via the F1 keypress.  The help file
* reflects any key reassignments made to the "keys" file.
*
* The -DHRABAK compile directive, for both feh_init and feh, adds code
* to allow rearranging (moving - changing the order in the linkedList)
* pics while in slideshow mode.  Press C-m to enter move_mode.
* Over 33 new keybindings were added to support this change (hense the
* reason I rewrote the keypress logic).  To avoid any overlap with existing
* feh and menu bindings, Hrabak created a new move_mode.  Key bindings may
* be duplicated across the three modes, just not within a mode.  So the
* Left arrow key has one meaning when menus are displayed, another when feh
* is displaying pics and a third meaning when rearranging pics in the list.
* This change requires the developer to be conscious of the NAMING of the
* new key bindings.  All menu related bindings must begin with "menu_".
* All move_mode bindings begin with "move_".  All the rest can be named
* anything as they fall into the general feh_actions[] category.
*
* keybinding format is <action>         : <key>
* Example              menu_close       : Escape
*
* Where each <key> is an X11 keysym (see /usr/include/X11/keysymdef.h)
* with optional modifier.  The old keyevents.c used a Mod1 and Mod4Mask.
* Mod1Mask has been rechristened Alt in newkeyev.c.  Mod4Mask was dropped.
* What is a Mod4Mask anyway?
*
* For instance, C-x would be Ctrl+X.  Valid modifiers are A-C-S ,
*   C for <Control>, A for <Alt>, S for <Shift>.
* Combos work ... C-A-S-x for <Control>+<Alt>+<Shift>+x
*
* To disable a keybinding (in this file) just comment out the line.  But if
* the end user wants to disable one of these default bindings at runtime,
* she would edit the keys file and assign the binding code to a bogus action
* named either "menu_DISABLE_ME", "move_DISABLE_ME" or "feh_DISABLE_ME"
* because a keybinding can be duplicated across any of the three modes.
*
* For example:
*     rot_cw_slow      : S-KP_Add     # the default binding
*     feh_DISABLE_ME   : S-KP_Add     # would turn it off
*
* Note: _DISABLE_ME is not implemented (yet).
*
* For developers ONLY,  and not available in the external "keys" file.  You
* can ReMap an action to use a function ptr assigned to another action.
* For example.  The help4bindings action is assigned to the F1 function key
* which will only be "seen" in the feh_actions[] bindings.  I want that help
* to also be available in the move_mode.  So assign a new action like move_help
* (Which, because of its name will fall in the move_actions[] bindings) but
* add a ReMap name after it.
*
* So a simple binding format listed above would look like ...
* help4bindings    : F1               # shows ALL the current key bindings
* ... but to make that action available when inside the move_mode, do this ...
* move_help        : F1   : stub_help4bindings      # ReMaps to call the stub_help4bindings
* ... This defines a whole new action (could have a diff key binding)
* but avoids having to duplicate the stub_<name> code.
* To repeat:  You can do this here, but NOT in the external "keys" file.
*
*/


#ifndef FEH_INIT_H
#define FEH_INIT_H

/* the "tag" to insert the real keybindings (and their descriptions) into the help file */
#define TRIGGER_TO_INCLUDE_HELP "###INSERT_BINDINGS_HELP_HERE###"

 /* all keys are defined here in the dflt_kb[] array.
  * feh_init.c will break these into menu_ move_ and feh_ lists.
  * The load process will warn you if there are dupe keybindings.
  */

char * dflt_kb[] = {
 "action_0"                  ":Return"            ,
 "action_0"                  ":0"                 ,
 "action_0"                  ":KP_0"              ,
 "action_1"                  ":1"                 ,
 "action_1"                  ":KP_1"              ,
 "action_2"                  ":2"                 ,
 "action_2"                  ":KP_2"              ,
 "action_3"                  ":3"                 ,
 "action_3"                  ":KP_3"              ,
 "action_4"                  ":4"                 ,
 "action_4"                  ":KP_4"              ,
 "action_5"                  ":5"                 ,
 "action_5"                  ":KP_5"              ,
 "action_6"                  ":6"                 ,
 "action_6"                  ":KP_6"              ,
 "action_7"                  ":7"                 ,
 "action_7"                  ":KP_7"              ,
 "action_8"                  ":8"                 ,
 "action_8"                  ":KP_8"              ,
 "action_9"                  ":9"                 ,
 "action_9"                  ":KP_9"              ,
 "close"                     ":x"                 ,
 "delete"                    ":C-Delete"          ,
 "enter_move_mode"           ":C-m"               ,
 "flip"                      ":underscore"        ,
 "help4bindings"             ":F1"                ,  /* shows feh_keybindings.htm help file */
 "help4feh"                  ":C-F1"              ,  /* shows feh_man.htm help file         */
 "jump_back"                 ":KP_Page_Up"        ,
 "jump_back"                 ":Page_Up"           ,
 "jump_first"                ":Home"              ,
 "jump_first"                ":KP_Home"           ,
 "jump_fwd"                  ":KP_Page_Down"      ,
 "jump_fwd"                  ":Page_Down"         ,
 "jump_last"                 ":End"               ,
 "jump_last"                 ":KP_End"            ,
 /* I had a hard time finding these.  Look in /usr/include/X11/keysymdef.h:531 for the
  * the matching hex codes and then just drop the 'XK_' prefix.
  * So XK_dollar   0x0024  becomes just 'dollar' in this mapping.
  */
 "jump_to_10"                ":exclam"         ":stub_move_to_10"   ,
 "jump_to_20"                ":at"             ":stub_move_to_20"   ,
 "jump_to_30"                ":numbersign"     ":stub_move_to_30"   ,
 "jump_to_40"                ":dollar"         ":stub_move_to_40"   ,
 "jump_to_50"                ":percent"        ":stub_move_to_50"   ,
 "jump_to_60"                ":asciicircum"    ":stub_move_to_60"   ,
 "jump_to_70"                ":ampersand"      ":stub_move_to_70"   ,
 "jump_to_80"                ":asterisk"       ":stub_move_to_80"   ,
 "jump_to_90"                ":parenleft"      ":stub_move_to_90"   ,
 "jump_to_100"               ":parenright"     ":stub_move_to_end"  ,
 "jump_random"               ":z"                 ,

/* all menu bindings begin with "menu_" */
 "menu_child"                ":Right"             ,
 "menu_close"                ":Escape"            ,
 "menu_down"                 ":Down"              ,
 "menu_parent"               ":Left"              ,
 "menu_select"               ":Return"            ,
 "menu_select"               ":space"             ,
 "menu_up"                   ":Up"                ,

/* back to regular feh bindings */
 "mirror"                    ":bar"               ,
 "next_img"                  ":Right"             ,
 "next_img"                  ":n"                 ,
 "next_img"                  ":space"             ,
 "orient_1"                  ":greater"           ,
 "orient_3"                  ":less"              ,
 "prev_img"                  ":BackSpace"         ,
 "prev_img"                  ":Left"              ,
 "prev_img"                  ":p"                 ,
 "quit"                      ":Escape"            ,
 "quit"                      ":q"                 ,
 "reload_image"              ":r"                 ,
 "reload_plus"               ":plus"              ,
 "remove"                    ":Delete"            ,
 "save_filelist"             ":f"                 ,
 "save_image"                ":s"                 ,
 "scroll_down"               ":C-Down"            ,
 "scroll_down"               ":KP_Down"           ,
 "scroll_down_page"          ":A-Down"            ,
 "scroll_left"               ":C-Left"            ,
 "scroll_left"               ":KP_Left"           ,
 "scroll_left_page"          ":A-Left"            ,
 "scroll_right"              ":C-Right"           ,
 "scroll_right"              ":KP_Right"          ,
 "scroll_right_page"         ":A-Right"           ,
 "scroll_up"                 ":C-Up"              ,
 "scroll_up"                 ":KP_Up"             ,
 "scroll_up_page"            ":A-Up"              ,
 "size_to_image"             ":w"                 ,
 "toggle_actions"            ":a"                 ,
 "toggle_aliasing"           ":A"                 ,
 "toggle_caption"            ":c"                 ,
#ifdef HAVE_LIBEXIF
	 "toggle_exif"             ":e"                 ,
#endif
 "toggle_filenames"          ":d"                 ,
 "toggle_fullscreen"         ":v"                 ,
 "toggle_info"               ":i"                 ,
 "toggle_menu"               ":m"                 ,
 "toggle_pause"              ":h"                 ,
 "toggle_pointer"            ":o"                 ,
 "toggle_thumb_mode"         ":C-t"               ,
 "zoom_default"              ":KP_Multiply"       ,
 "zoom_fit"                  ":KP_Divide"         ,
 "zoom_in"                   ":KP_Add"            ,
 "zoom_in"                   ":Up"                ,
 "zoom_out"                  ":Down"              ,
 "zoom_out"                  ":KP_Subtract"       ,

/* all the move_mode behavior is controled with the -DHRABAK directive.
 * Rearrangings pics in the linkedList involves tagging one or more pics in
 * slideshow mode, pressing C-m to change to a move_mode screen that displays
 * 5pics.  The one pic you are moving (or first in the list of pics to move)
 * is center screen.  Two pics to the left, and right are shown either side
 * of center.  The arrow keys slide those 4 pics past the center (stationary)
 * pic.  Press <Return> to drop that pic in between.
 */

#ifdef HRABAK
   "add2move_list"           ":C-space"           ,
   "render"                  ":KP_Begin"          ,

/* In regular mode, C-spacebar key adds/removes a pic from the move_list.
 * In move_mode, the Return key drops the pic into its new location in the list.
 * So after dropping a pic, press C-Return to return to the place you were
 * when you initiated the move.  You will appreciate this in really long lists.
 */

   "ret2pos"                 ":C-Return"     ":stub_move_ret2pos"  ,
   "ret2posFwd"              ":S-C-Return"        ,  /* same as ret2pos, only going Fwd in stack */
   "rot_ccw_fast"            ":A-KP_Subtract"     ,  /* rotations NOT using jpegtran */
   "rot_ccw_slow"            ":S-KP_Subtract"     ,
   "rot_cw_fast"             ":A-KP_Add"          ,
   "rot_cw_slow"             ":S-KP_Add"          ,

/* I am scamming onto the reload logic to change delay time inside a running
 * slideshow.  I assume that the reload logic only applies while you are in
 * a web-cam setting, so slideshow and web-camming are mutually exclusive.
 */

   "reload_minus"            ":equal"             ,
   "move_cancel"             ":Escape"            ,
   "move_dropit"             ":Return"            ,
   "move_flip_list"          ":C-f"               ,  /* process the mov_md LL in reverse order */
   "move_help"               ":F1"           ":stub_help4bindings"   ,  /* reMapped  */
   "move_one_bak"            ":KP_Left"           ,
   "move_one_bak"            ":Left"              ,
   "move_one_fwd"            ":KP_Right"          ,
   "move_one_fwd"            ":Right"             ,
   "move_ret2pos"            ":C-Return"          ,
   "move_to_10"              ":1"                 ,
   "move_to_10"              ":KP_1"              ,
   "move_to_20"              ":2"                 ,
   "move_to_20"              ":KP_2"              ,
   "move_to_30"              ":3"                 ,
   "move_to_30"              ":KP_3"              ,
   "move_to_40"              ":4"                 ,
   "move_to_40"              ":KP_4"              ,
   "move_to_50"              ":5"                 ,
   "move_to_50"              ":KP_5"              ,
   "move_to_60"              ":6"                 ,
   "move_to_60"              ":KP_6"              ,
   "move_to_70"              ":7"                 ,
   "move_to_70"              ":KP_7"              ,
   "move_to_80"              ":8"                 ,
   "move_to_80"              ":KP_8"              ,
   "move_to_90"              ":9"                 ,
   "move_to_90"              ":KP_9"              ,
   "move_to_beg"             ":Home"              ,
   "move_to_beg"             ":KP_Home"           ,
   "move_to_end"             ":0"                 ,
   "move_to_end"             ":End"               ,
   "move_to_end"             ":KP_0"              ,
   "move_to_end"             ":KP_End"            ,

	/* don't bother to add a pgUp and pgDn move_to (as exists in the slideshow
	 * mode ) cause the 10% thru 90% jumps are more controlled */

#else         /* not HRABAK */
   "reload_minus"            ":minus"             ,
#endif        /* HRABAK */
};     /* end of dflt_kb string */



/* array of action name strings (char*).  Parallels funcptr[] */
typedef struct {
  char * action;
  int len;
} SimplePtrmap;


/* ************************************************************* *
 * Global hlp variable,  used by feh_init.c and feh_initpre.c    *
 * ************************************************************* */

/* save the actual keybinding (text) for help display.
 * Allow for double the number of default bindings.
 */

#define DFLT_KB_CNT  (sizeof( dflt_kb )/ sizeof(char*))
struct {
  int   dflt_cnt;         /* total # of dflt_kb bindings                          */
  int   cnt;              /* number in list                                       */
  char *list[ DFLT_KB_CNT *2 ];
  char *ptr;              /* follows the trailing '\0' in names                   */
  char *names;            /* action'\0'key pairs, malloc'd in make_feh_bindings() */
  int   rm_cnt;           /* number of remapped function ptrs                     */
  char *(*rmlist)[];      /* ptr to an array of ptrs to each remapped func        */
  char *rm_ptr;           /* follows the trailing '\0' in remaps                  */
  char *remaps;           /* action'\0'reMapFunc, malloc'd in -DFIRST_PASS        */
} hlp = {.dflt_cnt=DFLT_KB_CNT };



/* Function prototypes ... */

int cmp_fname( const void *left, const void *right );
void make_subset( void ) ;
int cmp_symstate( const void *left, const void *right ) ;
StubPtr find_stubptr( char * action ) ;
int parse_bindings( char *action, char *ks ) ;
int make_feh_help( void );
void test_all_funcptr( void );


#endif    /* FEH_INIT_H */

