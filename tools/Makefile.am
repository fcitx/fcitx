toolsdir = $(pkgdatadir)/tools
INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/lib

bin_PROGRAMS = mb2txt txt2mb createPYMB
mb2txt_SOURCES = mb2txt.c
txt2mb_SOURCES = txt2mb.c
createPYMB_SOURCES = createPYMB.c

all: 
	./txt2mb ../data/cj.txt ../data/cj.mb
	./txt2mb ../data/erbi.txt ../data/erbi.mb
	./txt2mb ../data/wbx.txt ../data/wbx.mb
	./createPYMB ../data/gbkpy.org ../data/pyPhrase.org && mv *.mb ../data && rm -f pyERROR ; rm -f pyPhrase.ok
	