INCLUDES = -I$(top_srcdir)/lib -I$(top_srcdir)/src
DEFS = -DPKGDATADIR=\"$(pkgdatadir)\" -DDATADIR=\"$(datadir)\" -DLIBDIR=\"$(libdir)\"

lib_LTLIBRARIES=libfcitx-config.la
libfcitx_config_la_SOURCES= \
						   cutils.c cutils.h \
						   sprintf.c sprintf.h \
						   fcitx-config.c fcitx-config.h \
						   profile.h \
						   configfile.h \
						   hotkey.c hotkey.h \
						   uthash.h \
						   xdg.c xdg.h
libfcitx_config_la_LDFLAGS= -version-number 4:0:0 -no-undefined
libfcitx_configincludedir= $(includedir)/fcitx-config
libfcitx_configinclude_HEADERS= \
								fcitx-config.h \
								configfile.h \
								profile.h \
								hotkey.h \
								xdg.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = fcitx-config.pc
EXTRA_DIST = \
	fcitx-config.pc.in
