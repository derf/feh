include ../config.mk

TARGETS = ${shell echo *.c}
OBJECTS = ${TARGETS:.c=.o}

I_SRCS = ${shell echo *.raw}
I_DSTS = ${I_SRCS:.raw=.inc}

feh_init_list=feh_initpre.o feh_init.o stubptr.o
FILTERED=$(filter-out $(feh_init_list),${OBJECTS})


# swap ${FILTERED} with ${OBJECTS} to get back to stock feh build
feh: deps.mk ${FILTERED} ${I_DSTS}
	${CC} ${LDFLAGS} ${CFLAGS} -o $@ ${FILTERED} ${LDLIBS}

include deps.mk

# feh_init depends on feh_initpre.  Compile feh_initpre as feh_init (executable)
# to make stubptr.c that gets linked back into feh_init.c to make the final
# feh_init executable.  So when it finishes, only the final feh_init is left
# standing.  FIXME Is there a cleaner way to do this?
#  NOTE!  You run feh_init twice.  First time is a pre version.  2nd time is real
feh_init: feh_initpre.c feh_init.c feh_init.h stubptr.c
	${CC} ${LDFLAGS} ${CFLAGS} -DHRABAK -o $@ feh_initpre.c
	./feh_init
	${CC} ${LDFLAGS} ${CFLAGS} -DHRABAK -o $@ feh_init.c stubptr.c utils.c ${LDLIBS}
	./feh_init

.raw.inc:
	sed 's/^\(.*\)$$/"\1\\n"/' < ${@:.inc=.raw} > $@

fehrc.inc: fehrc.raw
help.inc: help.raw


deps.mk: ${TARGETS} ${I_DSTS}
	${CC} ${CPPFLAGS} -MM ${TARGETS} > $@

clean:
	rm -f feh feh_init feh_initpre *.o *.inc

.PHONY: clean

.SUFFIXES: .inc .raw
