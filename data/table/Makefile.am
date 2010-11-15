tabledir = $(pkgdatadir)/data/table

mb_data = cj.mb erbi.mb wanfeng.mb wbpy.mb qxm.mb db.mb wbx.mb zrm.mb
txt_data = cj.txt erbi.txt wanfeng.txt wbpy.txt qxm.txt db.txt wbx.txt zrm.txt
txt_m_data = erbi.txt wanfeng.txt wbpy.txt qxm.txt db.txt wbx.txt zrm.txt
mb_conf = qxm.conf db.conf erbi.conf wanfeng.conf  wbpy.conf wbx.conf zrm.conf cangjie.conf disable-table.conf.template
table_url = http://fcitx.googlecode.com/files/table.tar.gz
table_tar = table.tar.gz

table_DATA = $(mb_data) $(mb_conf)
EXTRA_DIST = $(mb_conf)

CLEANFILES = $(mb_data)

DISTCLEANFILES = $(txt_data) $(table_tar)

%.mb: %.txt
	../../tools/txt2mb $< $@

tools: ../../tools/txt2mb
	$(MAKE) -C ../../tools

$(txt_m_data): cj.txt ;

cj.txt: $(table_tar) md5check
	$(TAR) xzvf $(table_tar)

$(table_tar):
	$(WGET) -O $(table_tar) $(table_url)
	$(WGET) -O $(table_tar).md5 $(table_url).md5

md5check:
	$(MD5SUM) -c $(table_tar).md5 || ( $(RM) $(table_tar); exit 1)

all-local: tools $(txt_data) $(mb_data)

