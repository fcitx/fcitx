INCLUDES = \
		   -I$(top_srcdir)/lib \
		   -I$(top_srcdir)/src \
		   @CAIRO_XLIB_CFLAGS@ \
		   @DBUS_CFLAGS@ \
		   $(NULL)
DEFS = -DPKGDATADIR=\"$(pkgdatadir)\"

noinst_LIBRARIES=libimspecial.a
libimspecial_a_SOURCES= AutoEng.c AutoEng.h \
					  punc.c punc.h \
					  QuickPhrase.c QuickPhrase.h \
					  vk.c vk.h

