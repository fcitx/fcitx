INCLUDES = \
		   -I$(top_srcdir)/lib \
		   -I$(top_srcdir)/src \
		   @CAIRO_XLIB_CFLAGS@ \
		   @DBUS_CFLAGS@ \
		   $(NULL)
DEFS = -DPKGDATADIR=\"$(pkgdatadir)\" -DDATADIR=\"$(datadir)\" -DLOCALEDIR=\"$(localedir)\"

bin_PROGRAMS=fcitx
fcitx_SOURCES= IC.c IC.h\
			   MyErrorsHandlers.c MyErrorsHandlers.h\
			   ime.c ime.h \
			   internalVersion.c \
			   xim.c xim.h \
			   addon.c addon.h \
			   main.c  \
			   fcitx.h im.h \
			   $(NULL)

fcitxincludedir= $(includedir)/fcitx
fcitxinclude_HEADERS= \
					  im.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = fcitx.pc
EXTRA_DIST = \
	fcitx.pc.in

fcitx_LDADD=../../lib/libXimd.a \
			../im/pinyin/libimpinyin.a \
			../im/table/libimtable.a \
			../im/qw/libimqw.a \
			../im/extra/libimextra.a \
			../im/special/libimspecial.a \
			../tools/libtools.a \
			../ui/libui.a \
			../interface/libinterface.a \
            ../fcitx-config/libfcitx-config.la \
			@CAIRO_XLIB_LIBS@ \
			@DBUS_LIBS@ \
			$(NULL)

fcitx_LDFLAGS = -ldl -lpthread
