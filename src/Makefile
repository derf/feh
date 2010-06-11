include ../config.mk

TARGETS = ${shell echo *.c}
OBJECTS = ${TARGETS:.c=.o}

feh: deps.mk ${OBJECTS}
	${CC} ${LDFLAGS} ${CFLAGS} -o $@ ${OBJECTS} ${LDLIBS}

deps.mk: ${TARGETS}
	${CC} -MM ${TARGETS} > $@

include deps.mk

clean:
	rm -f feh *.o

.PHONY: clean
