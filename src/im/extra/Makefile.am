INCLUDES = \
		   -I$(top_srcdir)/lib \
		   -I$(top_srcdir)/src \
		   @CAIRO_XLIB_CFLAGS@ \
		   $(NULL)
DEFS = -DPKGDATADIR=\"$(pkgdatadir)\" -DLIBDIR=\"$(libdir)\"

noinst_LIBRARIES=libimextra.a
libimextra_a_SOURCES= extra.c extra.h

