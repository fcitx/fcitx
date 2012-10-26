#!/bin/sh

function download_file()
{
    if [ "x$3" != "xf" ]; then
        if [ -f $1 ]; then
            return
        fi
    fi
    rm -f $1
    wget $2
}

XKEYSYM=http://cgit.freedesktop.org/xorg/proto/xproto/plain/keysymdef.h
KEYSYMGEN_HEADER=keysymgen.h

download_file keysymdef.h $XKEYSYM

cat > $KEYSYMGEN_HEADER <<EOF
/***************************************************************************
 *   Copyright (C) 2012~2012 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#ifndef FCITX_UTILS_KEYSYMGEN_H
#define FCITX_UTILS_KEYSYMGEN_H
#ifdef __cplusplus
extern "C" {
#endif

typedef enum _FcitxKeySym
{
FcitxKey_None = 0x0,
EOF

grep '^#define' keysymdef.h | sed 's|^#define XK_\([a-zA-Z_0-9]\+\) \+0x\([0-9A-Fa-f]\+\)\(.*\)$|FcitxKey_\1 = 0x\2,\3|g' >> $KEYSYMGEN_HEADER
cat >> $KEYSYMGEN_HEADER <<EOF
} FcitxKeySym;

#ifdef __cplusplus
}
#endif

#endif
EOF
