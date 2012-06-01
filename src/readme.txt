###############################################################################
##                                                                           ##
##  The changes Hrabak made can be summarized as follows.                    ##
##                                                                           ##
##  1)The old keyevents.c logic processed a runtime keypress by exhaustive-  ##
##    ly seaching to see if the keypress code was defined inside the         ##
##    __fehkb structure of 60 members and three keys defined for each.       ##
##    So for a keyevent that was found to match the 60th member, required    ##
##    60x3x2=360 tests.                                                      ##
##                                                                           ##
##  2)The Hrabak change makes an array of ptrs to the functions to call      ##
##    for each keypress, mapped to and sorted by the keycode that is the     ##
##    concatenation of the keysym and state as reported by xev routine.      ##
##                                                                           ##
##  3)Each runtime keypress is then bsearch()'d inside this array looking    ##
##    for the UNIQUE sequence of keysym+state.  So the stock keyevents       ##
##    that could define a max of 180 keysym+state combos would require       ##
##    a max of 8 tests to find the deepest code.  This new fmt no longer     ##
##    limits you to 3 keybindings per action.  Any number can be defined.    ##
##                                                                           ##
##  4)But that is only part of the benefit.  The largest benefit comes from  ##
##    the ease of adding more keypress codes and the functions that they     ##
##    invoke.  New functionality can be added by doing the following...      ##
##     - make up a name for your new action  (ex  "my_action" )              ##
##     - add that action and the keypress codes that activate it to this     ##
##       bindings.sh file.                                                   ##
##       (ex my_action:A-Return ) will call my_action for  <Alt>+Return      ##
##     - code the function to carry out this action and name that func       ##
##       like ...                                                            ##
##       int stub_my_action( winwidget winwid ){ ...}                        ##
##       where the name of your new function is just the name of the         ##
##       new action (my_action) prepended by the word "stub_"                ##
##     - compile and enjoy.  All the headers, function prototypes,           ##
##       structs and arrays are created automatically.                       ##
##                                                                           ##
##  Hrabak added new functionality to allow moving pics.      The original   ##
##  feh does not allow rearranging the list except to delete items.  This    ##
##  new version allows you to stop a slide show, change into move_mode and   ##
##  to rearrange pics in the list on screen, then to restart the slideshow.  ##
##  This adds in excess of 33 new key bindings.  To avoid any overlap with   ##
##  existing feh and menu bindings, Hrabak just created a new move_mode.     ##
##  Key bindings may be duplicated across the three modes, just not within   ##
##  a mode.  So the Left arrow key has one meaning when menus are displayed, ##
##  another when feh is displaying pics and a third meaning when rearranging ##
##  pics in the list.  This change requires that the developer be concious   ##
##  of the NAMING of the new key bindings, which will fall into one of three ##
##  sets depending on the binding prefix.  All menu related bindings must    ##
##  begin with "menu_".  All move_mode bindings begin with "move_".  All     ##
##  the rest can be named anything as the rest fall into the general         ##
##  feh_actions[] category.  The rest of the compile process is the same as  ##
##  outlined above in steps 1 thru 4.                                        ##
##                                                                           ##
##  Note:  Run this script with the -DHRABAK precompile directive to         ##
##  activate the new move_mode behavior.                                     ##
##                                                                           ##
###############################################################################
