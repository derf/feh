include ../../config.mk

SOURCES = ${shell echo *.pre}
TARGETS = ${SOURCES:.pre=.desktop}

all: ${TARGETS}

.pre.desktop:
	sed \
	-e 's/\$$VERSION\$$/${VERSION}/g' \
	-e 's/\$$DATE\$$/'"$$(date '+%B %d, %Y')"/g \
	-e 's/\$$MAN_CURL\$$/${MAN_CURL}/' \
	-e 's/\$$MAN_DEBUG\$$/${MAN_DEBUG}/' \
	-e 's/\$$MAN_EXIF\$$/${MAN_EXIF}/' \
	-e 's/\$$MAN_XINERAMA\$$/${MAN_XINERAMA}/' \
	-e 's:\$$IMAGEDIR\$$:${image_dir}:' \
	< ${@:.desktop=.pre} > $@

clean:
	rm -f *.desktop

.SUFFIXES: .pre .desktop

.PHONY: clean
