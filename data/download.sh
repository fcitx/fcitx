#!/bin/sh
WGET=$1
URL=$2
OUTPUT=$3
if [ ! -f $OUTPUT ]; then
    ${WGET} -O $OUTPUT $URL
fi