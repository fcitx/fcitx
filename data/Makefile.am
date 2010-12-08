if ENABLE_TABLE_DATA
TABLE=table
endif

SUBDIRS= \
		 $(TABLE) \
		 $(NULL)

generaldir = $(pkgdatadir)/data

configdescfile = config.desc profile.desc table.desc skin.desc addon.desc

general_DATA = pybase.mb pyphrase.mb pySym.mb sp.dat punc.mb \
	     vk.conf gbks2t.tab AutoEng.dat \
		 $(configdescfile)

pyorg_data = @srcdir@/pyPhrase.org
gbkorg_data = @srcdir@/gbkpy.org
py_data = @srcdir@/pybase.mb @srcdir@/pyphrase.mb
py_tar = @srcdir@/pinyin.tar.gz
py_url = http://fcitx.googlecode.com/files/pinyin.tar.gz

bin_SCRIPTS = fcitx-config

desktopdir = $(datadir)/applications
desktop_in_files = fcitx.desktop.in fcitx-configtool.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

EXTRA_DIST = pySym.mb sp.dat punc.mb vk.conf gbks2t.tab AutoEng.dat gbkpy.org getdescpo fcitx-config $(configdescfile) $(desktop_in_files)
CLEANFILES = $(py_data)
DISTCLEANFILES = $(pyorg_data) $(desktop_DATA) 

@INTLTOOL_DESKTOP_RULE@

@srcdir@/pybase.mb: @srcdir@/pyphrase.mb ;

@srcdir@/pyphrase.mb: $(pyorg_data) $(gbkorg_data)
	cd $(srcdir) && @abs_top_builddir@/tools/createPYMB $(gbkorg_data) $(pyorg_data)

tools: 
	$(MAKE) -C ../tools

$(pyorg_data): $(py_tar)
	cd $(srcdir) && $(MD5SUM) -c $(py_tar).md5 || ( $(RM) $(py_tar); exit 1 )
	cd $(srcdir) && $(TAR) xzmvf $(py_tar)

$(py_tar):
	$(WGET) -O $(py_tar) $(py_url)
	$(WGET) -O $(py_tar).md5 $(py_url).md5

all-local: tools  $(py_data) @srcdir@/desc.po
	rm -f pyERROR
	rm -f pyPhrase.ok

@srcdir@/desc.po: @srcdir@/*.desc
	@srcdir@/getdescpo @abs_srcdir@

