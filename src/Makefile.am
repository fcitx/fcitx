bin_PROGRAMS = fcitx
AM_CFLAGS = -O2 -fno-strength-reduce
INCLUDES = -I$(top_srcdir)/lib -I$(top_srcdir)/xpm
fcitx_SOURCES = \
  IC.c IC.h ime.c ime.h InputWindow.c InputWindow.h KeyList.c \
  KeyList.h main.c main.h MainWindow.c MainWindow.h \
  MyErrorsHandlers.c MyErrorsHandlers.h punc.c punc.h py.c PYFA.c \
  PYFA.h py.h pyMapTable.c pyMapTable.h pyParser.c pyParser.h \
  SetLocale.c SetLocale.h sp.c sp.h tools.c tools.h ui.c ui.h \
  version.h wbx.c wbx.h xim.c xim.h
fcitx_LDADD = ../lib/libXimd.a
