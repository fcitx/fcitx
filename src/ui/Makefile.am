INCLUDES = \
		   -I$(top_srcdir)/lib \
		   -I$(top_srcdir)/src \
		   @CAIRO_XLIB_CFLAGS@ \
		   @DBUS_CFLAGS@ \
		   $(NULL)
DEFS = -DPKGDATADIR=\"$(pkgdatadir)\" -DDATADIR=\"$(datadir)\" -DBINDIR=\"$(bindir)\"

noinst_LIBRARIES=libui.a
libui_a_SOURCES= InputWindow.c InputWindow.h\
				 MainWindow.c MainWindow.h \
				 TrayWindow.c TrayWindow.h \
				 MessageWindow.c MessageWindow.h \
				 tray.c tray.h \
				 AboutWindow.c AboutWindow.h \
				 ui.c ui.h \
				 skin.c skin.h \
				 skinconfig.c \
				 MenuWindow.c MenuWindow.h \
				 font.c font.h
