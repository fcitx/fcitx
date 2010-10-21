INCLUDES = \
		   -I$(top_srcdir)/lib \
		   -I$(top_srcdir)/src \
		   @CAIRO_XLIB_CFLAGS@ \
		   $(NULL)
DEFS = -DPKGDATADIR=\"$(pkgdatadir)\"

EXTRA_DIST = $(tools_DATA)

noinst_PROGRAMS = testpyfa
testpyfa_SOURCES = testpyfa.c

testpyfa_LDADD = \
                 ../src/im/pinyin/PYFA.o \
                 ../src/fcitx-config/libfcitx-config.la \
                 @INTLLIBS@ \
                 $(NULL)

all: test

test: testpyfa
	./testpyfa || ( echo "PYFA has bug" ;exit 1 )
