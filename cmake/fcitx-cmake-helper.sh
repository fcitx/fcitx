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

cache_base="$1"
pot_file="$2"
action="$3"

shift 3 || exit 1

. "${_FCITX_MACRO_CMAKE_DIR}/fcitx-parse-po.sh"

src_cache="${cache_base}/sources-cache.txt"
po_cache="${cache_base}/pos-cache.txt"
handler_cache="${cache_base}/handlers-cache.txt"
parse_cache="${cache_base}/parse_po_cache"

add_sources() {
    for f in "$@"; do
        echo "$@" || return 1
    done >> "${src_cache}"
}

case "${action}" in
    --check-apply-handler)
        # full_name may be invalid
        script="$1"
        in_file="$2"
        out_file="$3"
        "${script}" "${parse_cache}" "${po_cache}" -c "${in_file}" "${out_file}"
        exit $?
        ;;
    --apply-po-merge)
        # full_name may be invalid
        script="$1"
        in_file="$2"
        out_file="$3"
        "${script}" "${parse_cache}" "${po_cache}" -w "${in_file}" "${out_file}"
        exit $?
        ;;
    --parse-pos)
        # full_name may be invalid
        fcitx_parse_all_pos "${po_cache}" "${parse_cache}"
        exit 0
        ;;
    --add-sources)
        add_sources "$@" || {
            rm -f "${src_cache}"
            exit 1
        }
        exit 0
        ;;
    --add-po)
        echo "$1 $2" >> "${po_cache}"
        exit 0
        ;;
    --add-handler)
        echo "${1}" >> "${handler_cache}"
        exit 0
        ;;
    --pot)
        . "${_FCITX_MACRO_CMAKE_DIR}/fcitx-write-po.sh"
        bug_addr="${1}"
        fcitx_generate_pot "${bug_addr}"
        exit 0
        ;;
    --clean)
        rm -f "${src_cache}" "${po_cache}" "${handler_cache}"
        exit 0
        ;;
    --uninstall)
        while read file; do
            file="${DESTDIR}/${file}"
            [ -f "${file}" ] || [ -L "${file}" ] || {
                echo "File: ${file}, doesn't exist or is not a regular file."
                continue
            }
            rm -v "${file}" || exit 1
        done <<EOF
$(cat install_manifest.txt)
EOF
        exit 0
        ;;
    *)
        exit 1
        ;;
esac
