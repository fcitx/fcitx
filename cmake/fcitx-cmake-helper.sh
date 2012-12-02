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

cache_base="${FCITX_CMAKE_CACHE_BASE}"
pot_file="${_FCITX_TRANSLATION_TARGET_FILE}"
action="$1"

shift || exit 1

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

download() {
    local url="$1"
    local output="$2"
    wget -c -T 10 -O "${output}.part" "${url}" || return 1
    mv "${output}.part" "${output}"
}

get_md5sum() {
    local file
    local res
    file="$1"
    res=$("${FCITX_HELPER_CMAKE_CMD}" -E md5sum "${file}")
    echo -n "${res%% *}"
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
    --download)
        url="$1"
        output="$2"
        md5sum="$3"
        if [ -f "${output}" ]; then
            touch "${output}"
            if [ -z "${md5sum}" ]; then
                exit 0
            fi
            realmd5="$(get_md5sum "${output}")"
            if [ "${md5sum}" = "${realmd5}" ]; then
                exit 0
            fi
            rm -f "${output}"
        fi
        download "${url}" "${output}" || exit 1
        if [ -z "${md5sum}" ]; then
            exit 0
        fi
        realmd5="$(get_md5sum "${output}")"
        if [ "${md5sum}" = "${realmd5}" ]; then
            exit 0
        fi
        exit 1
        ;;
    --check-md5sum)
        file="$1"
        md5sum="$2"
        remove="$3"
        realmd5="$(get_md5sum "${file}")"
        if [ "${md5sum}" = "${realmd5}" ]; then
            exit 0
        fi
        if [ ! -z "${remove}" ]; then
            rm -f "${file}"
        fi
        exit 1
        ;;
    *)
        exit 1
        ;;
esac
