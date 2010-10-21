INCLUDES = \
		   -I$(top_srcdir)/lib \
		   -I$(top_srcdir)/src \
		   @CAIRO_XLIB_CFLAGS@ \
		   $(NULL)
DEFS = -DPKGDATADIR=\"$(pkgdatadir)\"

EXTRA_DIST = $(tools_DATA)

bin_PROGRAMS = mb2txt txt2mb createPYMB readPYMB readPYBase mb2org fcitx-remote scel2org
scel2org_SOURCES = scel2org.c
readPYMB_SOURCES = pyTools.c pyTools.h readPYMB.c
readPYBase_SOURCES = pyTools.c pyTools.h readPYBase.c
fcitx_remote_SOURCES = fcitx-remote.c

scel2org_LDADD = \
                 @LIBICONV@ \
                 $(NULL)

fcitx_remote_LDADD = \
					 @CAIRO_XLIB_LIBS@ \
					 $(NULL)

readPYBase_LDADD = \
				   ../src/fcitx-config/libfcitx-config.la \
                                   @INTLLIBS@ \
				   $(NULL)

readPYMB_LDADD = \
				 ../src/fcitx-config/libfcitx-config.la \
                                 @INTLLIBS@ \
				 $(NULL)

createPYMB_LDADD = \
				   ../src/im/pinyin/pyParser.o \
				   ../src/im/pinyin/pyMapTable.o \
				   ../src/tools/utf8.o \
				   ../src/im/pinyin/PYFA.o \
				   ../src/im/pinyin/sp.o \
				   ../src/fcitx-config/libfcitx-config.la \
                                   @INTLLIBS@ \
				   $(NULL)
mb2org_LDADD = \
			   ../src/im/pinyin/pyParser.o \
			   ../src/im/pinyin/pyMapTable.o \
			   ../src/im/pinyin/PYFA.o \
			   ../src/im/pinyin/sp.o \
			   pyTools.o \
			   ../src/fcitx-config/libfcitx-config.la \
                           @INTLLIBS@ \
			   $(NULL)

txt2mb_LDADD = ../src/tools/utf8.o
