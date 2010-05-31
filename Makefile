include config.mk

all:
	@${MAKE} -C src

test: all
	@PACKAGE=${PACKAGE} VERSION=${VERSION} prove test

install: install-man install-doc install-bin install-font install-img

install-man:
	@echo installing manuals to ${man_dir}
	@mkdir -p ${man_dir}/man1
	@cp man/feh.1 man/feh-cam.1 man/gen-cam-menu.1 ${man_dir}/man1
	@chmod 644 ${man_dir}/man1/feh.1 ${man_dir}/man1/feh-cam.1 \
		${man_dir}/man1/gen-cam-menu.1

install-doc:
	@echo installing docs to ${doc_dir}
	@mkdir -p ${doc_dir}
	@cp AUTHORS ChangeLog README TODO ${doc_dir}
	@chmod 644 ${doc_dir}/AUTHORS ${doc_dir}/ChangeLog \
		${doc_dir}/README ${doc_dir}/TODO

install-bin:
	@echo installing executables to ${bin_dir}
	@mkdir -p ${bin_dir}
	@cp src/feh cam/feh-cam cam/gen-cam-menu ${bin_dir}
	@chmod 755 ${bin_dir}/feh ${bin_dir}/feh-cam \
		${bin_dir}/gen-cam-menu

install-font:
	@echo installing fonts to ${font_dir}
	@mkdir -p ${font_dir}
	@cp src/ttfonts/black.style src/ttfonts/menu.style \
		src/ttfonts/yudit.ttf ${font_dir}
	@chmod 644 ${font_dir}/black.style ${font_dir}/menu.style \
		${font_dir}/yudit.ttf

install-img:
	@echo installing images to ${image_dir}
	@mkdir -p ${image_dir}
	@cp src/about.png src/menubg_aluminium.png src/menubg_aqua.png \
		src/menubg_black.png src/menubg_brushed.png src/menubg_chrome.png \
		src/menubg_default.png src/menubg_pastel.png src/menubg_sky.png \
		src/menubg_wood.png ${image_dir}
	@chmod 644 ${image_dir}/about.png ${image_dir}/menubg_aluminium.png \
		${image_dir}/menubg_aqua.png ${image_dir}/menubg_black.png \
		${image_dir}/menubg_brushed.png ${image_dir}/menubg_chrome.png \
		${image_dir}/menubg_default.png ${image_dir}/menubg_pastel.png \
		${image_dir}/menubg_sky.png ${image_dir}/menubg_wood.png


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
