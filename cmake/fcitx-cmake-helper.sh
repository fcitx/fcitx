#!/bin/sh
#   Copyright (C) 2012~2013 by Yichao Yu
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

add_source() {
    local file="$1"
    local type="$2"
    [ -z "${type}" ] && type="generic"
    echo "${type} ${file}" >> "${src_cache}"
}

download_wget() {
    local url="$1"
    local output="$2"
    wget -c -T 10 -O "${output}.part" "${url}" || return 1
    mv "${output}.part" "${output}"
}

download_curl() {
    local url="$1"
    local output="$2"
    curl -C - -L -m 10 -o "${output}.part" "${url}" || return 1
    mv "${output}.part" "${output}"
}

download_cmake() {
    local url="$1"
    local output="$2"
    __print_cmake_download_cmd() {
        echo "file(DOWNLOAD \"${url}\" \"${output}\" TIMEOUT 10 STATUS result)"
        echo 'list(GET result 0 code)'
        echo 'if(code)'
        echo '  list(GET result 1 message)'
        echo '  message(FATAL_ERROR "${message}")'
        echo 'endif()'
    }
    local stdin_file=''
    local f
    for f in /dev/stdin /dev/fd/0 /proc/self/fd/0; do
        if [ -e "$f" ]; then
            stdin_file="$f"
            break
        fi
    done
    if [ -z "${stdin_file}" ]; then
        tmpdir="${FCITX_CMAKE_CACHE_BASE}/cmake_download"
        "${FCITX_HELPER_CMAKE_CMD}" -E make_directory "${tmpdir}"
        fname="$(mktemp "${tmpdir}/tmp.XXXXXX")" || return 1
        __print_cmake_download_cmd > "${fname}"
        "${FCITX_HELPER_CMAKE_CMD}" -P "${fname}"
        res=$?
        rm "${fname}"
        return $res
    else
        __print_cmake_download_cmd | "${FCITX_HELPER_CMAKE_CMD}" \
            -P "${stdin_file}" || return 1
        return $?
    fi
}

download() {
    local url="$1"
    local output="$2"
    local res=1
    if type wget >/dev/null 2>&1; then
        download_wget "${url}" "${output}"
        res=$?
    elif type curl >/dev/null 2>&1; then
        download_curl "${url}" "${output}"
        res=$?
    elif type mktemp >/dev/null 2>&1 ||
        [ -e /dev/stdin ] ||
        [ -e /dev/fd/0 ] ||
        [ -e /proc/self/fd/0 ]; then
        download_cmake "${url}" "${output}"
        res=$?
    else
        echo "Cannot find a command to download." >&2
        return 1
    fi
    if [ $res = 0 ]; then
        "${FCITX_HELPER_CMAKE_CMD}" -E touch_nocreate "${output}"
    fi
    return $res
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
    --add-source)
        add_source "$@" || {
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
        handler="${1}"
        shift || return 1
        for t in generic $@; do
            echo "${t} ${handler}" >> "${handler_cache}"
        done
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
        "${FCITX_HELPER_CMAKE_CMD}" -E touch \
            "${src_cache}" "${po_cache}" "${handler_cache}"
        exit 0
        ;;
    --uninstall)
        while read file; do
            [ -z "${file}" ] && continue
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
