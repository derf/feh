#!/bin/bash

######################################################################
# gen_cam_menu.sh 0.1                                                #
# by Paul Duncan <pabs@pablotron.org>                                #
#                                                                    #
# This script will generate a menu of cam bookmarks for              #
# Enlightenment DR0.16.x.  TYou can safely run this script more than #
# once; it won't add an another entry to the left-click menu if it's #
# already been run once.  It doesn't delete any existing menu        #
# entries, and it backs up your existing menu files as well. (just   #
# in case I screwed up.. hehe).  THe two variables below allow you   #
# rename the left-click menuitem, and the menu title.                #
#                                                                    #
#                                                                    #
MENU_ITEM="Webcams";                                                 #
MENU_TITLE="Webcam List";                                            #
BMARKS=$HOME"/.cam_bookmarks";                                       #
#                                                                    #  
######################################################################


C_MENUFILE="webcam.menu";
F_MENUFILE="file.menu";
C_MENU=$HOME"/.enlightenment/"$C_MENUFILE;
F_MENU=$HOME"/.enlightenment/"$F_MENUFILE;

# make backups, just in case
cp -f $C_MENU $C_MENU"-cam_menu.backup"
cp -f $F_MENU $F_MENU"-cam_menu.backup"

# generate cam menu
echo "Generating \""$C_MENU"\".";
echo "\"$TITLE\"" > "$C_MENU";
cat $BMARKS | perl -e "while (<>) { /(.*?)=/; \$keys{\$1}=\"1\"; } foreach(sort keys %keys) { /(.)(.*$)/; print \"\\\"\".uc(\$1).\"\$2\\\" NULL exec \\\"cam \$1\$2\\\"\\n\"; }">> $C_MENU;

# add entry to file menu if there isn't one
echo "Generating \""$F_MENU"\".";
perl -i -e "\$already_there=0; while (<>) { \$already_there++ if (/$MENU_ITEM/); print \"\\\"$MENU_ITEM\\\" NULL menu \\\"$C_MENUFILE\\\"\\n\" if (!\$already_there&&/Restart/); print; }" $F_MENU;

echo "Done.";
