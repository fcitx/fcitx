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

extract_subconfig() {
    local IFS=','
    local sub_config="$1"
    local in_file="$2"
    local line_num="$3"
    for section in ${sub_config}; do
        [[ "${section}" =~ ^([^:]+):(.*)$ ]] || continue
        [[ "${BASH_REMATCH[2]}" == "domain" ]] && continue
        msgid="${BASH_REMATCH[1]}"
        fcitx_record_po_msg "${in_file}" "${msgid}" "${line_num}"
    done
}

extract_desktop() {
    local in_file="$1"
    local out_file="$2"
    local line_num=0
    local line
    while read line; do
        : $((line_num++))
        case "${line}" in
            _*=*)
                line="${line#_}"
                key="${line%%=*}"
                msgid="${line#*=}"
                fcitx_record_po_msg "${in_file}" "${msgid}" "${line_num}"
                ;;
            SubConfig=*)
                key="${line%%=*}"
                sub_config="${line#*=}"
                extract_subconfig "${sub_config}" "${in_file}" "${line_num}"
                ;;
        esac
    done <<EOF
$(cat "${in_file}")
EOF
}

case "${action}" in
    -c)
        in_file="${1}"
        fcitx_exts_match "${in_file}" conf.in desktop.in && exit 0
        exit 1
        ;;
    -w)
        out_file="${1}"
        shift || exit 1
        fcitx_write_po_header "${out_file}"
        for rel_path in "$@"; do
            echo "Extracting po string from ${rel_path}"
            extract_desktop "${rel_path}" "${out_file}"
        done
        fcitx_write_all_po "${out_file}"
        exit 0
        ;;
esac
