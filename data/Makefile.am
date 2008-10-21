tabledir = $(pkgdatadir)/data
table_DATA = cj.mb erbi.mb punc.mb pybase.mb pyphrase.mb pySym.mb sp.dat \
	     wbx.mb wanfeng.mb wbpy.mb qxm.mb zrm.mb db.mb \
	     tables.conf vk.conf gbks2t.tab config AutoEng.dat
EXTRA_DIST = $(table_DATA)

mb_data = cj.mb erbi.mb wanfeng.mb wbpy.mb qxm.mb db.mb wbx.mb zrm.mb

%.mb: %.txt
	@top_srcdir@/tools/txt2mb $< $@

py_data = pybase.mb pyphrase.mb
$(py_data): pyPhrase.org gbkpy.org
	@top_srcdir@/tools/createPYMB gbkpy.org pyPhrase.org

tools: @top_srcdir@/tools/createPYMB @top_srcdir@/tools/txt2mb
	$(MAKE) -C ../tools

all: tools $(mb_data) $(py_data)
	rm -f pyERROR
	rm -f pyPhrase.ok
	
