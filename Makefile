include config.mk

all:
	@${MAKE} -C src

test: all
	@PACKAGE=${PACKAGE} VERSION=${VERSION} prove test

test-interactive: all
	@if [ "$$(whoami)" != derf ]; then \
		echo "Note: This will do stuff with your X and take a while"; \
		echo "If you don't know what's going on, hit ^C now"; \
		sleep 5; \
		echo "Okay, running test"; \
	fi
	@if [ "$$(whoami)" = derf ]; then setxkbmap us; fi
	@prove test/*.i
	@if [ "$$(whoami)" = derf ]; then setxkbmap greedy; fi

install: install-man install-doc install-bin install-font install-img

install-man:
	@echo installing manuals to ${man_dir}
	@mkdir -p ${man_dir}/man1
	@cp man/* ${man_dir}/man1
	@chmod 644 ${man_dir}/man1/feh.1 ${man_dir}/man1/feh-cam.1 \
		${man_dir}/man1/gen-cam-menu.1

install-doc:
	@echo installing docs to ${doc_dir}
	@mkdir -p ${doc_dir}
	@cp AUTHORS ChangeLog README TODO ${doc_dir}
	@chmod 644 ${doc_dir}/*

install-bin:
	@echo installing executables to ${bin_dir}
	@mkdir -p ${bin_dir}
	@cp src/feh cam/feh-cam cam/gen-cam-menu ${bin_dir}
	@chmod 755 ${bin_dir}/feh ${bin_dir}/feh-cam \
		${bin_dir}/gen-cam-menu

install-font:
	@echo installing fonts to ${font_dir}
	@mkdir -p ${font_dir}
	@cp data/fonts/* ${font_dir}
	@chmod 644 ${font_dir}/*

install-img:
	@echo installing images to ${image_dir}
	@mkdir -p ${image_dir}
	@cp data/images/* ${image_dir}
	@chmod 644 ${image_dir}/*


uninstall:
	rm -f ${man_dir}/man1/feh.1 ${man_dir}/man1/feh-cam.1
	rm -f ${man_dir}/man1/gen-cam-menu.1
	rm -rf ${doc_dir}
	rm -f ${bin_dir}/feh ${bin_dir}/feh-cam ${bin_dir}/gen-cam-menu
	rm -rf ${font_dir}
	rm -rf ${image_dir}

clean:
	@${MAKE} -C src clean

.PHONY: all test install uninstall clean install-man install-doc install-bin \
	install-font install-img
