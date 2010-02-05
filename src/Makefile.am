# A list of all the files in the current directory which can be regenerated
MAINTAINERCLEANFILES = Makefile.in				

LDFLAGS           = -L/usr/X11R6/lib -L/usr/local/lib -lz -lpng
INCLUDES          = -I/usr/X11R6/include $(X_CFLAGS) \
                    -I$(prefix)/include -I$(includedir) \
                    -I. -I$(top_srcdir)/src \
                    -I/usr/local/include/libpng \
                    -DPREFIX=\""$(prefix)"\" \
                    @IMLIB_CFLAGS@ @GIBLIB_CFLAGS@
LIBOBJS = @LIBOBJS@

bin_PROGRAMS      = feh
feh_SOURCES       = main.c getopt.c getopt1.c getopt.h feh.h \
options.c options.h winwidget.c winwidget.h menu.c menu.h structs.h \
filelist.c filelist.h multiwindow.c imlib.c index.c slideshow.c \
utils.c utils.h keyevents.c timers.c timers.h list.c collage.c debug.h \
events.c events.h support.c support.h transupp.c transupp.h \
thumbnail.c thumbnail.h ipc.c ipc.h md5.c md5.h feh_png.c feh_png.h \
jpegint.h

feh_LDADD         = -lX11 -lz -lpng @IMLIB_LIBS@ @GIBLIB_LIBS@

images_DATA = about.png menubg_default.png menubg_sky.png \
menubg_chrome.png menubg_brushed.png \
menubg_pastel.png menubg_aluminium.png menubg_aqua.png \
menubg_wood.png menubg_britney.png menubg_black.png

imagesdir=$(prefix)/share/feh/images
EXTRA_DIST = $(images_DATA)

test:
	@for i in test[0-9]* ; do \
	  if test -x $$i ; then \
	    echo "Executing test script $$i..." ; \
	    ./$$i ; \
	  fi ; \
	done

testclean:
	-rm -f _test*.jpg

SUBDIRS = ttfonts
