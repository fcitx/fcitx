DEFS = -DPKGDATADIR=\"$(pkgdatadir)\"

toolsdir = $(pkgdatadir)/tools
INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/lib

bin_PROGRAMS = mb2txt txt2mb createPYMB jd2fcitx pyjj2fcitx win2fcitx readPYMB readPYBase mb2org

createPYMB_LDADD = ../src/pyParser.o ../src/pyMapTable.o ../src/PYFA.o ../src/sp.o
pyjj2fcitx_LDADD = ../src/pyParser.o ../src/pyMapTable.o ../src/PYFA.o ../src/sp.o
readPYMB_LDADD = pyTools.o
readPYBase_LDADD = pyTools.o
mb2org_LDADD = ../src/pyParser.o ../src/pyMapTable.o ../src/PYFA.o ../src/sp.o pyTools.o

all: 
	./txt2mb $(srcdir)/../data/cj.txt ../data/cj.mb
	./txt2mb $(srcdir)/../data/erbi.txt ../data/erbi.mb
	./txt2mb $(srcdir)/../data/wbx.txt ../data/wbx.mb
	./createPYMB $(srcdir)/../data/gbkpy.org $(srcdir)/../data/pyPhrase.org
	mv *.mb ../data
	rm -f pyERROR
	rm -f pyPhrase.ok

