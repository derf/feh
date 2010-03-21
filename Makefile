include config.mk

default:
	@$(MAKE) -C src

install: install-man install-doc install-bin install-font install-img

install-man:
	@echo installing manuals
	@mkdir -p $(man_dir)/man1
	@cp man/feh.1 man/feh-cam.1 $(man_dir)/man1
	@chmod 644 $(man_dir)/man1/feh.1 $(man_dir)/man1/feh-cam.1
	@ln -fs feh-cam.1 $(man_dir)/man1/gen-cam-menu.1

install-doc:
	@echo installing additional docs
	@mkdir -p $(doc_dir)/feh
	@cp AUTHORS ChangeLog README TODO $(doc_dir)/feh
	@chmod 644 $(doc_dir)/feh/AUTHORS $(doc_dir)/feh/ChangeLog \
		$(doc_dir)/feh/README  $(doc_dir)/feh/TODO

install-bin:
	@echo installing executables
	@mkdir -p $(bin_dir)
	@cp src/feh cam/feh-cam cam/gen-cam-menu $(bin_dir)
	@chmod 755 $(bin_dir)/feh $(bin_dir)/feh-cam \
		$(bin_dir)/gen-cam-menu

install-font:
	@echo installing fonts
	@mkdir -p $(font_dir)
	@cp src/ttfonts/black.style src/ttfonts/menu.style \
		src/ttfonts/yudit.ttf $(font_dir)
	@chmod 644 $(font_dir)/black.style $(font_dir)/menu.style \
		$(font_dir)/yudit.ttf

install-img:
	@echo installing images
	@mkdir -p $(image_dir)
	@cp src/about.png src/menubg_aluminium.png src/menubg_aqua.png \
		src/menubg_black.png src/menubg_brushed.png src/menubg_chrome.png \
		src/menubg_default.png src/menubg_pastel.png src/menubg_sky.png \
		src/menubg_wood.png $(image_dir)
	@chmod 644 $(image_dir)/about.png $(image_dir)/menubg_aluminium.png \
		$(image_dir)/menubg_aqua.png $(image_dir)/menubg_black.png \
		$(image_dir)/menubg_brushed.png $(image_dir)/menubg_chrome.png \
		$(image_dir)/menubg_default.png $(image_dir)/menubg_pastel.png \
		$(image_dir)/menubg_sky.png $(image_dir)/menubg_wood.png


uninstall:
	rm -f $(man_dir)/man1/feh.1 $(man_dir)/man1/feh-cam.1
	rm -f $(man_dir)/man1/gen-cam-menu.1
	rm -rf $(doc_dir)
	rm -f $(bin_dir)/feh $(bin_dir)/feh-cam $(bin_dir)/gen-cam-menu
	rm -rf $(font_dir)
	rm -rf $(image_dir)

clean:
	@$(MAKE) -C src clean

.PHONY: default install uninstall clean
