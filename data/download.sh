#!/bin/sh
WGET=$1
URL=$2
OUTPUT=$3
if [ ! -f $OUTPUT ]; then
    ${WGET} -c -T 10 -O $OUTPUT.part $URL
    mv $OUTPUT.part $OUTPUT
fi
