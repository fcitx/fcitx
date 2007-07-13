toolsdir = $(pkgdatadir)/tools
INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/lib

bin_PROGRAMS = mb2txt txt2mb createPYMB
mb2txt_SOURCES = mb2txt.c
txt2mb_SOURCES = txt2mb.c
createPYMB_SOURCES = createPYMB.c

all: 
	./txt2mb $(srcdir)/../data/cj.txt ../data/cj.mb
	./txt2mb $(srcdir)/../data/erbi.txt ../data/erbi.mb
	./txt2mb $(srcdir)/../data/wbx.txt ../data/wbx.mb
	./createPYMB $(srcdir)/../data/gbkpy.org $(srcdir)/../data/pyPhrase.org && mv *.mb ../data && rm -f pyERROR ; rm -f pyPhrase.ok
