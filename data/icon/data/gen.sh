#!/bin/sh

a=`find scabable/status/ -type f -name \*.svg`
echo $a
size="24 48"
for f in $a
do
    ff=`basename $f`
    for s in $size
    do
        inkscape -e /home/saber/Develop/fcitx/data/icon/${s}x${s}/status/${ff/.svg/.png} -w $s -h $s -b "000000" -y 0.0 /home/saber/Develop/fcitx/data/icon/$f
    done
done

for f in fcitx-kbd-16.svg fcitx-vk-active-16.svg fcitx-vk-inactive-16.svg
do
inkscape -e /home/saber/Develop/fcitx/data/icon/16x16/status/${f/-16.svg/.png} -w 16 -h 16 -b "000000" -y 0.0 /home/saber/Develop/fcitx/data/icon/data/$f
done



for f in fcitx-fullwidth-active.svg fcitx-fullwidth-inactive.svg fcitx-punc-active.svg fcitx-punc-inactive.svg
do
inkscape -e /home/saber/Develop/fcitx/data/icon/16x16/status/${f/.svg/.png} -w 16 -h 16 -b "000000" -y 0.0 /home/saber/Develop/fcitx/data/icon/scabable/status/$f
done