#!/bin/sh
if test -f /etc/debian_version; then
  # in my debian/unstable, i use these:
  aclocal-1.6 || exit 1
  autoconf2.50 || exit 2
  automake-1.6 --add-missing || exit 3
else
  aclocal || exit 1
  autoconf || exit 2
  automake --add-missing || exit 3
fi

if [ -z "$1" ] ; then
  echo
  echo FCITX sources are now prepared. To build here, run:
  echo " ./configure && make"
else
  ./configure $* || exit 4
  make || exit 5
fi

echo
echo 'Build and Install on system:'
echo '  ./configure && make install   # run as root'

echo 'Build and Install on user home:'
echo '  ./configure --prefix="$HOME" && make install'

echo 'Build without xft:'
echo '  ./configure --enable-xft=no && make'

echo
echo 'Build RPM Package like these:'
echo '  ./configure && make dist'
echo '  rpmbuild -ts fcitx-*.tar.gz   # build source package'
echo '  rpmbuild -tb fcitx-*.tar.gz   # build binary package'

echo
echo 'Build DEBIAN Package like these:'
echo '  ./configure'
echo '  vim ./debian/changelog        # add change log'
echo '  fakeroot dpkg-buildpackage'
