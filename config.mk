PACKAGE ?= feh
VERSION ?= 1.4.2-git

# Prefix for all installed files
PREFIX ?= /usr/local

# Directories for manuals, executables, docs, data, etc.
main_dir = ${DESTDIR}${PREFIX}
man_dir = ${main_dir}/share/man
bin_dir = ${main_dir}/bin
doc_dir = ${main_dir}/share/doc
image_dir = ${main_dir}/share/feh/images
font_dir = ${main_dir}/share/feh/fonts

# default CFLAGS
CFLAGS ?= -g -O2
CFLAGS += -Wall -Wextra -pedantic

# Comment these out if you don't have libxinerama
xinerama = -DHAVE_LIBXINERAMA
xinerama_ld = -lXinerama

debug = -DDEBUG
# Uncomment this for debug mode
# (Use feh -+ <level> or feh --debug-level <level> to see debug output)
#CFLAGS += ${debug}

dmalloc = -DWITH_DMALLOC
# Uncomment this to use dmalloc
#CFLAGS += ${dmalloc}

CFLAGS += ${xinerama} -DPREFIX=\"${PREFIX}\" \
	-DPACKAGE=\"${PACKAGE}\" -DVERSION=\"${VERSION}\"

LDFLAGS += -lpng -lX11 -lImlib2 -lfreetype -lXext -lgiblib \
	${xinerama_ld}
