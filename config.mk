PACKAGE ?= feh
VERSION ?= ${shell git describe --dirty}

# Prefix for all installed files
PREFIX ?= /usr/local

# Directories for manuals, executables, docs, data, etc.
main_dir = ${DESTDIR}${PREFIX}
man_dir = ${main_dir}/share/man
bin_dir = ${main_dir}/bin
doc_dir = ${main_dir}/share/doc/feh
image_dir = ${main_dir}/share/feh/images
font_dir = ${main_dir}/share/feh/fonts
example_dir = ${main_dir}/share/doc/feh/examples

# default CFLAGS
CFLAGS ?= -g -O2
CFLAGS += -Wall -Wextra -pedantic

curl ?= 1
debug ?= 0
help ?= 0
xinerama ?= 1
exif ?= 0

ifeq (${curl},1)
	CFLAGS += -DHAVE_LIBCURL
	LDLIBS += -lcurl
	MAN_CURL = enabled
else
	MAN_CURL = disabled
endif

ifeq (${debug},1)
	CFLAGS += -DDEBUG
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


# Uncomment this to use dmalloc
#CFLAGS += -DWITH_DMALLOC

CFLAGS += -DPREFIX=\"${PREFIX}\" \
	-DPACKAGE=\"${PACKAGE}\" -DVERSION=\"${VERSION}\"

LDLIBS += -lm -lpng -lX11 -lImlib2
