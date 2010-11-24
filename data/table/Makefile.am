tabledir = $(pkgdatadir)/data/table

mb_data = \
		  @srcdir@/cj.mb \
		  @srcdir@/erbi.mb \
		  @srcdir@/wanfeng.mb \
		  @srcdir@/wbpy.mb \
		  @srcdir@/qxm.mb \
		  @srcdir@/db.mb \
		  @srcdir@/wbx.mb \
		  @srcdir@/zrm.mb \
		  $(NULL)
txt_data = \
		   @srcdir@/cj.txt \
		   @srcdir@/erbi.txt \
		   @srcdir@/wanfeng.txt \
		   @srcdir@/wbpy.txt \
		   @srcdir@/qxm.txt \
		   @srcdir@/db.txt \
		   @srcdir@/wbx.txt \
		   @srcdir@/zrm.txt \
		   $(NULL)
txt_m_data = \
		   @srcdir@/erbi.txt \
		   @srcdir@/wanfeng.txt \
		   @srcdir@/wbpy.txt \
		   @srcdir@/qxm.txt \
		   @srcdir@/db.txt \
		   @srcdir@/wbx.txt \
		   @srcdir@/zrm.txt \
		   $(NULL)

mb_conf = qxm.conf db.conf erbi.conf wanfeng.conf  wbpy.conf wbx.conf zrm.conf cangjie.conf disable-table.conf.template
table_url = http://fcitx.googlecode.com/files/table.tar.gz
table_tar = @srcdir@/table.tar.gz

table_DATA = $(mb_data) $(mb_conf)
EXTRA_DIST = $(mb_conf)

CLEANFILES = $(mb_data)

DISTCLEANFILES = $(txt_data)

@srcdir@/%.mb: @srcdir@/%.txt
	../../tools/txt2mb $< $@

tools: ../../tools/txt2mb
	$(MAKE) -C ../../tools

$(txt_m_data): @srcdir@/cj.txt ;

@srcdir@/cj.txt: $(table_tar)
	cd $(srcdir) && $(MD5SUM) -c $(table_tar).md5 || ( $(RM) $(table_tar); exit 1)
	cd $(srcdir) && $(TAR) xzmvf $(table_tar)

$(table_tar):
	$(WGET) -O $(table_tar) $(table_url)
	$(WGET) -O $(table_tar).md5 $(table_url).md5

all-local: tools $(txt_data) $(mb_data)

