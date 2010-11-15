SUBDIRS=table

generaldir = $(pkgdatadir)/data

configdescfile = config.desc profile.desc table.desc skin.desc addon.desc

general_DATA = pybase.mb pyphrase.mb pySym.mb sp.dat punc.mb \
	     vk.conf gbks2t.tab AutoEng.dat \
		 $(configdescfile)

pyorg_data = pyPhrase.org
py_data = pybase.mb pyphrase.mb
py_tar = pinyin.tar.gz
py_url = http://fcitx.googlecode.com/files/pinyin.tar.gz

EXTRA_DIST = pySym.mb sp.dat punc.mb vk.conf gbks2t.tab AutoEng.dat gbkpy.org getdescpo fcitx-config $(configdescfile)
CLEANFILES = $(py_data)
DISTCLEANFILES = $(pyorg_data)
bin_SCRIPTS = fcitx-config

pybase.mb: pyphrase.mb ;

pyphrase.mb: $(pyorg_data) gbkpy.org
	../tools/createPYMB gbkpy.org $(pyorg_data)

tools: ../tools/createPYMB
	$(MAKE) -C ../tools

$(pyorg_data): $(py_tar) md5check
	$(TAR) xzvf $(py_tar)

$(py_tar):
	$(WGET) -O $(py_tar) $(py_url)
	$(WGET) -O $(py_tar).md5 $(py_url).md5

md5check:
	$(MD5SUM) -c $(py_tar).md5 || ( $(RM) $(py_tar); exit 1)

all-local: tools  $(py_data) desc.po
	rm -f pyERROR
	rm -f pyPhrase.ok

desc.po: *.desc
	./getdescpo

