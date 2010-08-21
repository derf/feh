include ../config.mk

SOURCES = ${shell echo *.pre}
TARGETS = ${SOURCES:.pre=.1}

all: ${TARGETS}

.pre.1:
	sed 's/\$$VERSION\$$/${VERSION}/g' < ${@:.1=.pre} > $@

clean:
	rm *.1

.SUFFIXES: .pre .1

.PHONY: clean
