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
