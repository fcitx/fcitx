SUBDIRS=table

generaldir = $(pkgdatadir)/data

configdescfile = config.desc profile.desc table.desc skin.desc addon.desc

general_DATA = pybase.mb pyphrase.mb pySym.mb sp.dat punc.mb \
	     vk.conf gbks2t.tab AutoEng.dat \
		 $(configdescfile)
EXTRA_DIST = pySym.mb sp.dat punc.mb vk.conf gbks2t.tab AutoEng.dat pyPhrase.org gbkpy.org getdescpo fcitx-config $(configdescfile)

bin_SCRIPTS = fcitx-config

py_data = pybase.mb pyphrase.mb

$(py_data): @top_srcdir@/data/pyPhrase.org @top_srcdir@/data/gbkpy.org
	../tools/createPYMB @top_srcdir@/data/gbkpy.org @top_srcdir@/data/pyPhrase.org

tools: ../tools/createPYMB
	$(MAKE) -C ../tools

all: tools $(py_data) desc.po
	rm -f pyERROR
	rm -f pyPhrase.ok

desc.po: *.desc
	./getdescpo

clean:
	rm -f $(py_data)
