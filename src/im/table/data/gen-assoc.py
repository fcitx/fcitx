#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# This script generates associative phrases for each word sorted by decending
# frequency.
#
# Traditional Chinese phrases are extracted from "tsi.src", provided by
# libchewing (chewing.im).
#
# Traditional Chinese phrases are extracted from "count.out", provided by
# iBDLE (nlp.blcu.edu.cn).
#
#############################################################################
# Copyright (C) 2016 Timothy Lee <timothy.ty.lee@gmail.com>
#
# This script is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This script is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public License
# along with this library; see the file COPYING.LIB.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.
#############################################################################


import os
import io
import csv
import operator
import urllib.request
import zipfile


# Maximum number of phrases per word
MAX_PHRASE_COUNT = 100


# Generator that parses 'tsi.src' into a list of records
def parse_tsi_src():
    with open('tsi.src', newline='') as data:
        for fields in csv.reader(data, delimiter=' '):
            ulen = len(fields[0])
            if len(fields) > 1 and ulen > 1:
                word = fields[0][0]
                if ord(word) > 0xff:
                    # Each record contains: phrase, word, length, frequency
                    yield [fields[0], word, ulen, int(fields[1])]


# Generator that parses 'count.out' into a list of records
def parse_count_out():
    with open('count.out', encoding='cp936', newline='') as data:
        for fields in csv.reader(data, delimiter=' '):
            ulen = len(fields[0])
            if len(fields) > 1 and ulen > 1:
                word = fields[0][0]
                if ord(word) > 0xff:
                    # Each record contains: phrase, word, length, frequency
                    yield [fields[0], word, ulen, int(fields[1])]


def unique_phrase(parsed):
    seen   = set([])
    totals = {}
    # Sort three times to order records by word, length, then frequency
    data = sorted(parsed, key=operator.itemgetter(3), reverse=True)
    data = sorted(data, key=operator.itemgetter(2))
    for cols in sorted(data, key=operator.itemgetter(1)):
        phrase = cols[0]
        word   = cols[1]
        # Ignore duplicate phrases
        if phrase in seen:
            continue
        # Limit the number of phrases provided for each word
        count = totals.get(word, 0)
        if count < MAX_PHRASE_COUNT:
            seen.add(phrase)
            totals[word] = count + 1
            yield phrase


# Load "tsi.src" from libchewing repository if it does not exist
if not os.path.isfile('tsi.src'):
    print('Downloading tsi.src......')
    urllib.request.urlretrieve('https://github.com/chewing/libchewing/raw/master/data/tsi.src', 'tsi.src')

# Load "count.zip" from iBDLE if it does not exist
if not os.path.isfile('count.out'):
    print('Downloading count.out......')
    data = urllib.request.urlopen('http://bcc.blcu.edu.cn/downloads/resources/%E6%B1%89%E8%AF%AD%E5%B8%B8%E7%94%A8%E8%AF%8D%E8%AF%8D%E9%A2%91%E8%A1%A8.zip')
    with zipfile.ZipFile(io.BytesIO(data.read())) as zfile:
        zfile.extract('count.out')

# Generate Traditional Chinese associative phrases
print('Generating assoc-cht.mb.new......')
with open('assoc-cht.mb.new', 'w') as out:
    for phrase in unique_phrase(parse_tsi_src()):
        out.write(phrase + '\n')

# Generate Simplified Chinese associative phrases
print('Generating assoc-chs.mb.new......')
with open('assoc-chs.mb.new', 'w') as out:
    for phrase in unique_phrase(parse_count_out()):
        out.write(phrase + '\n')

# Report success
print('Done.')
