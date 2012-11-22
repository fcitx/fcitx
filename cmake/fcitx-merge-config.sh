#!/bin/sh
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

parse_cache="$1"
po_cache="$2"
action="$3"
in_file="$4"
out_file="$5"

shift 5 || exit 1

. "${_FCITX_MACRO_CMAKE_DIR}/fcitx-parse-po.sh"

merge_config() {
    local in_file="${1}"
    local out_file="${2}"
    local key
    local msgid
    local msgstr
    local lang
    local line
    while read line; do
        case "${line}" in
            _*=*)
                line="${line#_}"
                echo "${line}"
                key="${line%%=*}"
                msgid="${line#*=}"
                for lang in $all_po_langs; do
                    msgstr="$(fcitx_find_str_for_lang "${lang}" "${msgid}")"
                    if [ -z "${msgstr}" ]; then
                        continue
                    fi
                    echo "${key}[${lang}]=${msgstr}"
                done
                ;;
            *)
                echo "${line}"
                ;;
        esac
    done > "${out_file}" <<EOF
$(cat "${in_file}")
EOF
}

case "${action}" in
    -c)
        if fcitx_str_match "*.conf.in" "${in_file}" &&
            fcitx_str_match "*.conf" "${out_file}"; then
            exit 0
        elif fcitx_str_match "*.desktop.in" "${in_file}" &&
            fcitx_str_match "*.desktop" "${out_file}"; then
            exit 0
        fi
        exit 1
        ;;
    -w)
        fcitx_load_all_pos "${po_cache}" "${parse_cache}"
        merge_config "${in_file}" "${out_file}"
        exit 0
        ;;
esac
