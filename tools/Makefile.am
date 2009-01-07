DEFS = -DPKGDATADIR=\"$(pkgdatadir)\"

toolsdir = $(pkgdatadir)/tools
INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/lib -I$(top_srcdir)/tools

bin_PROGRAMS = mb2txt txt2mb createPYMB jd2fcitx pyjj2fcitx win2fcitx readPYMB readPYBase mb2org
bin_SOURCES = createPYMB.c jd2fcitx.c mb2org.c mb2txt.c pyjj2fcitx.c pyTools.c \
	pyTools.h readPYBase.c readPYMB.c txt2mb.c win2fcitx.c winmb2fcitx.py

createPYMB_LDADD = ../src/pyParser.o ../src/pyMapTable.o ../src/PYFA.o ../src/sp.o
pyjj2fcitx_LDADD = ../src/pyParser.o ../src/pyMapTable.o ../src/PYFA.o ../src/sp.o
readPYMB_LDADD = pyTools.o
readPYBase_LDADD = pyTools.o
mb2org_LDADD = ../src/pyParser.o ../src/pyMapTable.o ../src/PYFA.o ../src/sp.o pyTools.o
