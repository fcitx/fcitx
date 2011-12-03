#!/bin/sh

a=`find scabable/status/ -type f -name \*.svg`
echo $a
size="24 32 48"
for f in $a
do
    ff=`basename $f`
    for s in $size
    do
        inkscape-0.48.2-1 -e /home/saber/Develop/fcitx/data/icon/${s}x${s}/status/${ff/.svg/.png} -w $s -h $s -b "000000" -y 0.0 /home/saber/Develop/fcitx/data/icon/$f
    done
done
