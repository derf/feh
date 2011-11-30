include ../config.mk

TARGETS = ${shell echo *.c}
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


deps.mk: ${TARGETS} ${I_DSTS}
	${CC} ${CPPFLAGS} -MM ${TARGETS} > $@

clean:
	rm -f feh *.o *.inc

.PHONY: clean

.SUFFIXES: .inc .raw
