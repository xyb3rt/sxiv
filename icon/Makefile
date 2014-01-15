PREFIX = /usr/local
ICONS  = 16x16.png 32x32.png 48x48.png 64x64.png 128x128.png

all:

install:
	for f in $(ICONS); do \
		dir="$(DESTDIR)$(PREFIX)/share/icons/hicolor/$${f%.png}/apps"; \
		mkdir -p "$$dir"; \
		cp "$$f" "$$dir/sxiv.png"; \
		chmod 644 "$$dir/sxiv.png"; \
	done
