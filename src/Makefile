include ../config.mk

TARGETS = \
	display.c \
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

ifeq (${wayland},1)
	WAYLAND_NAMES = xdg-shell cursor-shape-v1 tablet-unstable-v2 \
		viewporter fractional-scale-v1
	TARGETS += wayland.c ${WAYLAND_NAMES:%=%-protocol.c}
	WAYLAND_PROTOCOLS = ${shell ${PKG_CONFIG} --variable=pkgdatadir wayland-protocols}
	WAYLAND_HEADERS = ${WAYLAND_NAMES:%=%-client-protocol.h}
	vpath %.xml ${WAYLAND_PROTOCOLS}/stable/xdg-shell \
		${WAYLAND_PROTOCOLS}/staging/cursor-shape \
		${WAYLAND_PROTOCOLS}/unstable/tablet \
		${WAYLAND_PROTOCOLS}/stable/viewporter \
		${WAYLAND_PROTOCOLS}/staging/fractional-scale
endif

OBJECTS = ${TARGETS:.c=.o}

I_SRCS = ${shell echo *.raw}
I_DSTS = ${I_SRCS:.raw=.inc}

.DEFAULT_GOAL := feh

%-client-protocol.h: %.xml
	${WAYLAND_SCANNER} client-header $< $@

%-protocol.c: %.xml
	${WAYLAND_SCANNER} private-code $< $@

wayland.o: ${WAYLAND_HEADERS}

feh: deps.mk ${OBJECTS} ${I_DSTS}
	${CC} ${LDFLAGS} ${CFLAGS} -o $@ ${OBJECTS} ${LDLIBS}

ifneq (${MAKECMDGOALS},clean)
include deps.mk
endif

.raw.inc:
	sed 's/^\(.*\)$$/"\1\\n"/' < ${@:.inc=.raw} > $@

fehrc.inc: fehrc.raw
help.inc: help.raw

# CFLAGS might contain include paths needed to resolve includes in headers
deps.mk: ${TARGETS} ${I_DSTS} ${WAYLAND_HEADERS}
	${CC} ${CFLAGS} -MM ${TARGETS} > $@

clean:
	rm -f feh *.o *.inc deps.mk *-client-protocol.h *-protocol.c

.PHONY: clean

.SUFFIXES: .inc .raw
