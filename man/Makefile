include ../config.mk

SOURCES = ${shell echo *.pre}
TARGETS = ${SOURCES:.pre=.1}

all: ${TARGETS}

.pre.1:
	sed \
	-e 's/\$$VERSION\$$/${VERSION}/g' \
	-e 's/\$$DATE\$$/'"$$(date '+%B %d, %Y')"/g \
	< ${@:.1=.pre} > $@

clean:
	rm -f *.1

.SUFFIXES: .pre .1

.PHONY: clean
