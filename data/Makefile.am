tabledir = $(pkgdatadir)/data
table_DATA = cj.mb erbi.mb punc.mb pybase.mb pyphrase.mb pySym.mb sp.dat \
	     wbx.mb wanfeng.mb wbpy.mb qxm.mb zrm.mb db.mb \
	     tables.conf vk.conf gbks2t.tab config AutoEng.dat
EXTRA_DIST = $(table_DATA)

mb_data = cj.mb erbi.mb wanfeng.mb wbpy.mb qxm.mb db.mb wbx.mb zrm.mb

%.mb: %.txt
	../tools/txt2mb $< $@

py_data = pybase.mb pyphrase.mb
$(py_data): @top_srcdir@/data/pyPhrase.org @top_srcdir@/data/gbkpy.org
	../tools/createPYMB @top_srcdir@/data/gbkpy.org @top_srcdir@/data/pyPhrase.org

tools: ../tools/createPYMB ../tools/txt2mb
	$(MAKE) -C ../tools

all: tools $(mb_data) $(py_data)
	rm -f pyERROR
	rm -f pyPhrase.ok
	
