DEFS = -DPKGDATADIR=\"$(pkgdatadir)\"

INCLUDES = -I$(top_srcdir)/lib -I$(top_srcdir)/xpm

AM_CFLAGS = -O2 -fno-strength-reduce


bin_PROGRAMS = fcitx

fcitx_SOURCES = erbi.c erbi.h \
                IC.c IC.h \
                ime.c ime.h \
                InputWindow.c InputWindow.h \
                KeyList.c KeyList.h \
                main.c main.h \
                MainWindow.c MainWindow.h \
                MyErrorsHandlers.c MyErrorsHandlers.h \
                punc.c punc.h \
                py.c py.h \
                PYFA.c PYFA.h \
                pyMapTable.c pyMapTable.h \
                pyParser.c pyParser.h \
                SetLocale.c SetLocale.h \
                sp.c sp.h \
                tools.c tools.h \
                ui.c ui.h \
                version.h \
                wbx.c wbx.h \
                xim.c xim.h
fcitx_LDADD = ../lib/libXimd.a
