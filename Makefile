include config.mk

all: build-src build-man build-applications

build-src:
	@${MAKE} -C src

build-man:
	@${MAKE} -C man

build-applications:
	@${MAKE} -C share/applications

test: all
	@PACKAGE=${PACKAGE} prove test

test-x11: all
	test/run-interactive
	prove test/feh-bg.i

install: install-man install-doc install-bin install-font install-img
install: install-examples install-applications

install-man:
	@echo installing manuals to ${man_dir}
	@mkdir -p ${man_dir}/man1
	@cp man/*.1 ${man_dir}/man1
	@chmod 644 ${man_dir}/man1/feh.1 ${man_dir}/man1/feh-cam.1 \
		${man_dir}/man1/gen-cam-menu.1

install-doc:
	@echo installing docs to ${doc_dir}
	@mkdir -p ${doc_dir}
	@cp AUTHORS ChangeLog README TODO ${doc_dir}
	@chmod 644 ${doc_dir}/AUTHORS ${doc_dir}/ChangeLog ${doc_dir}/README \
		${doc_dir}/TODO

install-bin:
	@echo installing executables to ${bin_dir}
	@mkdir -p ${bin_dir}
	@cp src/feh cam/feh-cam cam/gen-cam-menu ${bin_dir}
	@chmod 755 ${bin_dir}/feh ${bin_dir}/feh-cam \
		${bin_dir}/gen-cam-menu

install-font:
	@echo installing fonts to ${font_dir}
	@mkdir -p ${font_dir}
	@cp share/fonts/* ${font_dir}
	@chmod 644 ${font_dir}/*

install-img:
	@echo installing images to ${image_dir}
	@mkdir -p ${image_dir}
	@cp share/images/* ${image_dir}
	@chmod 644 ${image_dir}/*

install-examples:
	@echo installing examples to ${example_dir}
	@mkdir -p ${example_dir}
	@cp examples/* ${example_dir}
	@chmod 644 ${example_dir}/*

install-applications:
	@echo installing desktop file to ${desktop_dir}
	@mkdir -p ${desktop_dir}
	@cp share/applications/feh.desktop ${desktop_dir}
	@chmod 644 ${desktop_dir}/feh.desktop


uninstall:
	rm -f ${man_dir}/man1/feh.1 ${man_dir}/man1/feh-cam.1
	rm -f ${man_dir}/man1/gen-cam-menu.1
	rm -rf ${doc_dir}
	rm -f ${bin_dir}/feh ${bin_dir}/feh-cam ${bin_dir}/gen-cam-menu
	rm -f ${desktop_dir}/feh.desktop
	rm -rf ${font_dir}
	rm -rf ${image_dir}

dist:
	mkdir /tmp/feh-${VERSION}
	git --work-tree=/tmp/feh-${VERSION} checkout -f
	cp src/deps.mk /tmp/feh-${VERSION}/src/deps.mk
	sed -i 's/^VERSION ?= .*$$/VERSION ?= ${VERSION}/' \
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
