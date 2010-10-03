INCLUDES = \
		   -I$(top_srcdir)/lib \
		   -I$(top_srcdir)/src \
		   @CAIRO_XLIB_CFLAGS@ \
		   @DBUS_CFLAGS@
		   $(NULL)
DEFS = -DPKGDATADIR=\"$(pkgdatadir)\"

noinst_LIBRARIES=libinterface.a
libinterface_a_SOURCES= DBus.c DBus.h \
						ImeRemote.c

