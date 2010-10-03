INCLUDES = \
		   -I$(top_srcdir)/lib \
		   -I$(top_srcdir)/src \
		   @CAIRO_XLIB_CFLAGS@ \
		   $(NULL)
DEFS = -DPKGDATADIR=\"$(pkgdatadir)\" -DDATADIR=\"$(datadir)\"

noinst_LIBRARIES=libimpinyin.a
libimpinyin_a_SOURCES= py.c py.h \
					  PYFA.c PYFA.h \
					  pyMapTable.c pyMapTable.h \
					  pyParser.c pyParser.h \
					  sp.c sp.h

