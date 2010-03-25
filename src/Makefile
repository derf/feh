include ../config.mk

TARGETS = $(shell echo *.c)
OBJECTS = $(TARGETS:.c=.o)

feh: deps.mk $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

deps.mk: $(TARGETS)
	$(CC) -MM $(TARGETS) > $@

include deps.mk

clean:
	rm -f feh *.o

.PHONY: clean
