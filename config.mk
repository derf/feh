PACKAGE ?= feh
VERSION ?= ${shell git describe}

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

# Comment these out if you don't have libxinerama
xinerama = -DHAVE_LIBXINERAMA
xinerama_ld = -lXinerama

# Uncomment this for debug mode
# (Use feh -+ or feh --debug to see debug output)
#CFLAGS += -DDEBUG

# Uncomment this to use dmalloc
#CFLAGS += -DWITH_DMALLOC

CFLAGS += ${xinerama} -DPREFIX=\"${PREFIX}\" \
	-DPACKAGE=\"${PACKAGE}\" -DVERSION=\"${VERSION}\"

LDLIBS += -lm -lpng -lX11 -lImlib2 -lgiblib -lcurl ${xinerama_ld}
