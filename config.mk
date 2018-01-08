PACKAGE ?= feh
VERSION ?= ${shell git describe --dirty}

app ?= 0
cam ?= 0
curl ?= 1
debug ?= 0
help ?= 0
xinerama ?= 1
exif ?= 0

# Prefix for all installed files
PREFIX ?= /usr/local
ICON_PREFIX ?= ${DESTDIR}${PREFIX}/share/icons

# icons in /usr/share/local/icons (and other prefixes != /usr) are not
# generally supported. So ignore PREFIX and always install icons into
# /usr/share/icons if the user wants to install feh on their local machine.
ifeq (${app},1)
	ICON_PREFIX = /usr/share/icons
endif

# Directories for manuals, executables, docs, data, etc.
main_dir = ${DESTDIR}${PREFIX}
man_dir = ${main_dir}/share/man
bin_dir = ${main_dir}/bin
doc_dir = ${main_dir}/share/doc/feh
image_dir = ${main_dir}/share/feh/images
font_dir = ${main_dir}/share/feh/fonts
example_dir = ${main_dir}/share/doc/feh/examples
desktop_dir = ${main_dir}/share/applications
icon_dir = ${ICON_PREFIX}/hicolor
48_icon_dir = ${icon_dir}/48x48/apps
scalable_icon_dir = ${icon_dir}/scalable/apps

# default CFLAGS
CFLAGS ?= -g -O2
CFLAGS += -Wall -Wextra -pedantic

# Settings for glibc >= 2.19 - may need to be adjusted for other systems
CFLAGS += -std=c11 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700

ifeq (${curl},1)
	CFLAGS += -DHAVE_LIBCURL
	LDLIBS += -lcurl
	MAN_CURL = enabled
else
	MAN_CURL = disabled
endif

ifeq (${debug},1)
	CFLAGS += -DDEBUG -O0
	MAN_DEBUG = . This is a debug build.
else
	MAN_DEBUG =
endif

ifeq (${help},1)
	CFLAGS += -DINCLUDE_HELP
endif

ifeq (${stat64},1)
	CFLAGS += -D_FILE_OFFSET_BITS=64
endif

ifeq (${xinerama},1)
	CFLAGS += -DHAVE_LIBXINERAMA
	LDLIBS += -lXinerama
	MAN_XINERAMA = enabled
else
	MAN_XINERAMA = disabled
endif

ifeq (${exif},1)
	CFLAGS += -DHAVE_LIBEXIF
	LDLIBS += -lexif
	MAN_EXIF = enabled
else
	MAN_EXIF = disabled
endif

MAN_DATE ?= ${shell date '+%B %d, %Y'}

# Uncomment this to use dmalloc
#CFLAGS += -DWITH_DMALLOC

CFLAGS += -DPREFIX=\"${PREFIX}\" \
	-DPACKAGE=\"${PACKAGE}\" -DVERSION=\"${VERSION}\"

LDLIBS += -lm -lpng -lX11 -lImlib2
