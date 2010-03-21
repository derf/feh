# Package name and version
package = feh
version = 1.4.1

# Prefix for all installed files
prefix = /usr/local

# Directories for manuals, executables, docs, data, etc.
man_dir = $(prefix)/share/man
bin_dir = $(prefix)/bin
doc_dir = $(prefix)/share/doc
image_dir = $(prefix)/share/feh/images
font_dir = $(prefix)/share/feh/fonts

# debug = 1 if you want debug mode
debug =

# default CFLAGS
CFLAGS = -g -Wall -Wextra -O2

# Comment these out if you don't have libxinerama
xinerama = -DHAVE_LIBXINERAMA
xinerama_ld = -lXinerama

# Put extra header directories here
extra_headers =

# Put extra include (-Lfoo) directories here
extra_libs =

dmalloc = -DWITH_DMALLOC
# Enable this to use dmalloc
#CFLAGS += $(dmalloc)


# You should not need to change anything below this line.

CFLAGS += $(extra_headers) $(xinerama) -DPREFIX=\"$(prefix)\" \
	-DPACKAGE=\"$(package)\" -DVERSION=\"$(version)\" $(debug)

LDFLAGS = -lz -lpng -lX11 -lImlib2 -lfreetype -lXext -ldl -lm -lgiblib \
	$(xinerama_ld) $(extra_includes)
