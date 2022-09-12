include config.mk

all: build-src build-man build-applications

build-src:
	@${MAKE} -C src

build-man:
	@${MAKE} -C man

build-applications:
	@${MAKE} -C share/applications

test: all
	@if ! uname -m | grep -q -e arm -e mips; then \
		PACKAGE=${PACKAGE} prove test/feh.t test/mandoc.t; \
	else \
		PACKAGE=${PACKAGE} prove test/feh.t test/mandoc.t || cat test/imlib2-bug-notice; \
	fi

test-x11: all
	test/run-interactive
	prove test/feh-bg-i.t

install: install-man install-doc install-bin install-font install-img
install: install-icon install-examples install-applications

install-man: man/feh.1
	@echo installing manuals to ${man_dir}
	@mkdir -p ${man_dir}/man1
	@cp man/feh.1 ${man_dir}/man1
	@chmod 644 ${man_dir}/man1/feh.1

install-doc: AUTHORS ChangeLog README.md TODO
	@echo installing docs to ${doc_dir}
	@mkdir -p ${doc_dir}
	@cp AUTHORS ChangeLog README.md TODO ${doc_dir}
	@chmod 644 ${doc_dir}/AUTHORS ${doc_dir}/ChangeLog ${doc_dir}/README.md \
		${doc_dir}/TODO

install-bin: src/feh
	@echo installing executables to ${bin_dir}
	@mkdir -p ${bin_dir}
	@cp src/feh ${bin_dir}/feh.tmp
	@mv ${bin_dir}/feh.tmp ${bin_dir}/feh
	@chmod 755 ${bin_dir}/feh

install-font:
	@echo installing fonts to ${font_dir}
	@mkdir -p ${font_dir}
	@chmod 755 ${font_dir}
	@cp share/fonts/* ${font_dir}
	@chmod 644 ${font_dir}/*

install-img:
	@echo installing images to ${image_dir}
	@mkdir -p ${image_dir}
	@chmod 755 ${image_dir}
	@cp share/images/* ${image_dir}
	@chmod 644 ${image_dir}/*

install-icon:
	@echo installing icon to ${48_icon_dir}
	@mkdir -p ${48_icon_dir}
	@cp share/images/feh.png ${48_icon_dir}
	@chmod 644 ${48_icon_dir}/feh.png
	@echo installing icon to ${scalable_icon_dir}
	@mkdir -p ${scalable_icon_dir}
	@cp share/images/feh.svg ${scalable_icon_dir}
	@chmod 644 ${scalable_icon_dir}/feh.svg
	@if test "${app}" = 1 && which gtk-update-icon-cache > /dev/null 2>&1; then \
		gtk-update-icon-cache ${icon_dir}; \
	fi

install-examples:
	@echo installing examples to ${example_dir}
	@mkdir -p ${example_dir}
	@cp examples/* ${example_dir}
	@chmod 644 ${example_dir}/*

install-applications: share/applications/feh.desktop
	@echo installing desktop file to ${desktop_dir}
	@mkdir -p ${desktop_dir}
	@cp share/applications/feh.desktop ${desktop_dir}
	@chmod 644 ${desktop_dir}/feh.desktop


uninstall:
	rm -f ${man_dir}/man1/feh.1
	rm -rf ${doc_dir}
	rm -f ${bin_dir}/feh
	rm -f ${desktop_dir}/feh.desktop
	rm -rf ${font_dir}
	rm -rf ${image_dir}
	@if test -e ${48_icon_dir}/feh.png; then \
		echo rm -f ${48_icon_dir}/feh.png; \
		rm -f ${48_icon_dir}/feh.png; \
	fi
	@if test -e ${scalable_icon_dir}/feh.svg; then \
		echo rm -f ${scalable_icon_dir}/feh.svg; \
		rm -f ${scalable_icon_dir}/feh.svg; \
	fi
	@if which gtk-update-icon-cache > /dev/null 2>&1; then \
		gtk-update-icon-cache ${icon_dir}; \
	fi

dist:
	mkdir /tmp/feh-${VERSION}
	git --work-tree=/tmp/feh-${VERSION} checkout -f
	cp src/deps.mk /tmp/feh-${VERSION}/src/deps.mk
	sed -i 's/^VERSION ?= .*$$/VERSION ?= ${VERSION}/' \
		/tmp/feh-${VERSION}/config.mk
	sed -i 's/^MAN_DATE ?= .*$$/MAN_DATE ?= ${MAN_DATE}/' \
		/tmp/feh-${VERSION}/config.mk
	tar -C /tmp -cjf ../feh-${VERSION}.tar.bz2 feh-${VERSION}
	rm -r /tmp/feh-${VERSION}

disttest: dist
	tar -C /tmp -xjf ../feh-${VERSION}.tar.bz2
	make -C /tmp/feh-${VERSION}
	make -C /tmp/feh-${VERSION} test
	make -C /tmp/feh-${VERSION} install DESTDIR=./install
	make -C /tmp/feh-${VERSION} uninstall DESTDIR=./install
	rm -r /tmp/feh-${VERSION}

clean:
	@${MAKE} -C src clean
	@${MAKE} -C man clean
	@${MAKE} -C share/applications clean

.PHONY: all test test-x11 install uninstall clean install-man install-doc \
	install-bin install-font install-img install-examples \
	install-applications dist
