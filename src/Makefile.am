bin_PROGRAMS = sxiv
sxiv_SOURCES = commands.c exif.c image.c main.c options.c thumbs.c util.c window.c scripting.c commands.h  config.def.h  config.h  exif.h  image.h  options.h  scripting.h  thumbs.h  types.h  util.h  window.h
sxiv_CFLAGS = $(AM_CFLAGS) $(GUILE_CFLAGS) -std=gnu99 -DGUILE_SCRIPT='"$(sysconfdir)/sxiv.scm"' -DINFO_SCRIPT='"$(sysconfdir)/sxiv-image-info"'
sxiv_LDADD = -lX11 -lImlib2 -lgif $(GUILE_LDFLAGS)
