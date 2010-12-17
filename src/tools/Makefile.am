INCLUDES = \
		   -I$(top_srcdir)/lib \
		   -I$(top_srcdir)/src \
		   @CAIRO_XLIB_CFLAGS@ \
		   @PANGOCAIRO_CFLAGS@
		   $(NULL)
DEFS = -DPKGDATADIR=\"$(pkgdatadir)\" -DDATADIR=\"$(datadir)\"

noinst_LIBRARIES=libtools.a
libtools_a_SOURCES= utf8.c utf8.h\
					utf8_in_gb18030.h \
					tools.c tools.h \
					utarray.h \
					configfile.c configfile.h \
					profile.c profile.h \
					$(NULL)

