#!/usr/bin/env bash
#   Copyright (C) 2012~2012 by Yichao Yu
#   yyc1992@gmail.com
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

action="$1"

shift 1 || exit 1

. "${_FCITX_MACRO_CMAKE_DIR}/fcitx-write-po.sh"

case "${action}" in
    -c)
        in_file="${1}"
        fcitx_exts_match "${in_file}" qml qs && exit 0
        exit 1
        ;;
    -w)
        out_file="${1}"
        shift || exit 1
        [[ -z "$*" ]] && exit 1
        echo "Extracting po string from qt sources."
        # need to touch source dir here since lupdate will otherwise include
        # absolute path (or wrong relative path) in po files afaik.
        tempfile="$(mktemp tmp.XXXXXXXX_fcitx_qt_$$.ts)"
        rm "${tempfile}"
        lupdate -locations relative -no-obsolete "$@" -ts "${tempfile}"
        lconvert -of po -o "${out_file}" -if ts -i "${tempfile}" \
            --drop-translations
        rm "${tempfile}"
        exit 0
        ;;
esac
