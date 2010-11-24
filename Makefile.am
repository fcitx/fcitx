AUTOMAKE_OPTIONS = foreign
SUBDIRS = doc skin lib src tools test data png po
EXTRA_DIST = autogen.sh fcitx.spec

MAINTAINERCLEANFILES = \
        aclocal.m4 \
        config.guess \
        config.sub \
        configure \
        INSTALL \
        missing \
        mkinstalldirs \
        Makefile.in

install-exec-local:
	@echo
	@echo Fcitx Install Finished.
	@echo You may need to run ldconfig to make shared library work.
	@echo
