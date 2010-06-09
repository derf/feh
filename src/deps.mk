collage.o: collage.c feh.h structs.h menu.h ipc.h utils.h getopt.h \
  debug.h winwidget.h filelist.h options.h
events.o: events.c feh.h structs.h menu.h ipc.h utils.h getopt.h debug.h \
  filelist.h winwidget.h timers.h options.h events.h thumbnail.h
feh_png.o: feh_png.c feh_png.h feh.h structs.h menu.h ipc.h utils.h \
  getopt.h debug.h
filelist.o: filelist.c feh.h structs.h menu.h ipc.h utils.h getopt.h \
  debug.h filelist.h options.h
getopt.o: getopt.c
getopt1.o: getopt1.c getopt.h
imlib.o: imlib.c feh.h structs.h menu.h ipc.h utils.h getopt.h debug.h \
  filelist.h winwidget.h options.h
index.o: index.c feh.h structs.h menu.h ipc.h utils.h getopt.h debug.h \
  filelist.h winwidget.h options.h
ipc.o: ipc.c feh.h structs.h menu.h ipc.h utils.h getopt.h debug.h \
  options.h
keyevents.o: keyevents.c feh.h structs.h menu.h ipc.h utils.h getopt.h \
  debug.h thumbnail.h filelist.h winwidget.h options.h
list.o: list.c feh.h structs.h menu.h ipc.h utils.h getopt.h debug.h \
  filelist.h options.h
main.o: main.c feh.h structs.h menu.h ipc.h utils.h getopt.h debug.h \
  filelist.h winwidget.h timers.h options.h events.h support.h
md5.o: md5.c md5.h
menu.o: menu.c feh.h structs.h menu.h ipc.h utils.h getopt.h debug.h \
  support.h thumbnail.h filelist.h winwidget.h options.h
multiwindow.o: multiwindow.c feh.h structs.h menu.h ipc.h utils.h \
  getopt.h debug.h winwidget.h timers.h filelist.h options.h
options.o: options.c feh.h structs.h menu.h ipc.h utils.h getopt.h \
  debug.h filelist.h options.h
signals.o: signals.c feh.h structs.h menu.h ipc.h utils.h getopt.h \
  debug.h winwidget.h
slideshow.o: slideshow.c feh.h structs.h menu.h ipc.h utils.h getopt.h \
  debug.h filelist.h timers.h winwidget.h options.h signals.h
support.o: support.c feh.h structs.h menu.h ipc.h utils.h getopt.h \
  debug.h filelist.h options.h support.h
thumbnail.o: thumbnail.c feh.h structs.h menu.h ipc.h utils.h getopt.h \
  debug.h filelist.h winwidget.h options.h thumbnail.h md5.h feh_png.h
timers.o: timers.c feh.h structs.h menu.h ipc.h utils.h getopt.h debug.h \
  options.h timers.h
utils.o: utils.c feh.h structs.h menu.h ipc.h utils.h getopt.h debug.h \
  options.h
winwidget.o: winwidget.c feh.h structs.h menu.h ipc.h utils.h getopt.h \
  debug.h filelist.h winwidget.h options.h
