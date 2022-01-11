include ../config.mk

TARGETS = \
	events.c \
	feh_png.c \
	filelist.c \
	gib_hash.c \
	gib_imlib.c \
	gib_list.c \
	gib_style.c \
	imlib.c \
	index.c \
	keyevents.c \
	list.c \
	main.c \
	md5.c \
	menu.c \
	multiwindow.c \
	options.c \
	signals.c \
	slideshow.c \
	thumbnail.c \
	timers.c \
	utils.c \
	wallpaper.c \
	winwidget.c

ifeq (${exif},1)
	TARGETS += \
		exif.c \
		exif_canon.c \
		exif_nikon.c
endif

ifneq (${verscmp},1)
	TARGETS += strverscmp.c
endif

OBJECTS = ${TARGETS:.c=.o}

I_SRCS = ${shell echo *.raw}
I_DSTS = ${I_SRCS:.raw=.inc}

feh: deps.mk ${OBJECTS} ${I_DSTS}
	${CC} ${LDFLAGS} ${CFLAGS} -o $@ ${OBJECTS} ${LDLIBS}

include deps.mk

.raw.inc:
	sed 's/^\(.*\)$$/"\1\\n"/' < ${@:.inc=.raw} > $@

fehrc.inc: fehrc.raw
help.inc: help.raw

# CFLAGS might contain include paths needed to resolve includes in headers
deps.mk: ${TARGETS} ${I_DSTS}
	${CC} ${CFLAGS} -MM ${TARGETS} > $@

clean:
	rm -f feh *.o *.inc

.PHONY: clean

.SUFFIXES: .inc .raw
