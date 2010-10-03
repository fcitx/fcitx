tabledir = $(pkgdatadir)/data/table

mb_data = cj.mb erbi.mb wanfeng.mb wbpy.mb qxm.mb db.mb wbx.mb zrm.mb
txt_data = cj.txt erbi.txt wanfeng.txt wbpy.txt qxm.txt db.txt wbx.txt zrm.txt
mb_conf = qxm.conf db.conf erbi.conf wanfeng.conf  wbpy.conf wbx.conf zrm.conf cangjie.conf disable-table.conf.template

table_DATA = $(mb_data) $(mb_conf)
EXTRA_DIST = $(mb_conf) $(txt_data)

%.mb: %.txt
	../../tools/txt2mb $< $@

tools: ../../tools/txt2mb
	$(MAKE) -C ../../tools

all: tools $(mb_data)

clean:
	rm -f $(mb_data)
