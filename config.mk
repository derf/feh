package = feh
version = 1.4.1-git

# Customize below:

# Prefix for all installed files
prefix = /usr/local

# Directories for manuals, executables, docs, data, etc.
man_dir = $(prefix)/share/man
bin_dir = $(prefix)/bin
doc_dir = $(prefix)/share/doc
image_dir = $(prefix)/share/feh/images
font_dir = $(prefix)/share/feh/fonts

# default CFLAGS
CFLAGS = -g -Wall -Wextra -pedantic -O2

# Comment these out if you don't have libxinerama
xinerama = -DHAVE_LIBXINERAMA
xinerama_ld = -lXinerama

# Put extra header (-I/some/directory) directories here
extra_headers =

# Put extra include (-L/some/directory) directories here
extra_libs =

debug = -DDEBUG
# Uncomment this for debug mode
# (Use feh -+ <level> to see debug output)
#CFLAGS += $(debug)

dmalloc = -DWITH_DMALLOC
# Uncomment this to use dmalloc
#CFLAGS += $(dmalloc)


# You should not need to change anything below this line.

CFLAGS += $(extra_headers) $(xinerama) -DPREFIX=\"$(prefix)\" \
	-DPACKAGE=\"$(package)\" -DVERSION=\"$(version)\"

LDFLAGS = -lz -lpng -lX11 -lImlib2 -lfreetype -lXext -ldl -lm -lgiblib \
	$(xinerama_ld) $(extra_includes)
