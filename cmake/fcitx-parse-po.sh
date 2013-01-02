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

fcitx_str_match() {
    local pattern="$1"
    local string="$2"
    case "${string}" in
        $pattern)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

fcitx_exts_match() {
    local in_file="$1"
    local ext
    shift
    for ext in "${@}"; do
        fcitx_str_match "*.${ext}" "${in_file}" && return 0
    done
    return 1
}

fcitx_msgid_to_varname() {
    local prefix="${1}"
    local msgid="${2}"
    echo -n "${prefix}___"
    echo -n "${msgid}" | "${_FCITX_PO_PARSER_EXECUTABLE}" --encode
}

fcitx_parse_po_file() {
    local po_lang="${1}"
    local pofile="${2}"
    local parse_res="${3}"
    "${_FCITX_PO_PARSER_EXECUTABLE}" --parse-po "${po_lang}" \
        "${pofile}" > "${parse_res}"
}

fcitx_find_str() {
    local prefix="${1}"
    local msgid="${2}"
    local varname="$(fcitx_msgid_to_varname "${prefix}" "${msgid}")"
    eval "echo -n \"\${${varname}}\"" | \
        "${_FCITX_PO_PARSER_EXECUTABLE}" --decode
}

all_po_langs=''

fcitx_lang_to_prefix() {
    local lang="${1}"
    echo -n "${lang}" | "${_FCITX_PO_PARSER_EXECUTABLE}" --encode
}

fcitx_parse_all_pos() {
    local po_cache="$1"
    local po_parse_cache_dir="$2"
    mkdir -p "${po_parse_cache_dir}"
    local po_lang
    local po_file
    local prefix
    local po_parse_cache_file
    local line
    while read line; do
        [ -z "${line}" ] && continue
        po_lang="${line%% *}"
        po_file="${line#* }"
        prefix="$(fcitx_lang_to_prefix "${po_lang}")"
        po_parse_cache_file="${po_parse_cache_dir}/${prefix}.fxpo"
        if [ "${po_parse_cache_file}" -ot "${po_file}" ] ||
            [ "${po_parse_cache_file}" -ot "${_FCITX_PO_PARSER_EXECUTABLE}" ] ||
            ! [ -f "${po_parse_cache_file}" ]; then
            echo "Parsing po file: ${po_file}"
            local unique_fname="${po_parse_cache_file}.$$"
            fcitx_parse_po_file "${po_lang}" "${po_file}" "${unique_fname}"
            mv "${unique_fname}" "${po_parse_cache_file}"
            echo "Finished parsing po file: ${po_file}"
        fi
    done <<EOF
$(cat "${po_cache}")
EOF
}

fcitx_load_all_pos() {
    local po_cache="$1"
    local po_parse_cache_dir="$2"
    local po_lang
    local po_file
    local prefix
    local po_parse_cache_file
    local line
    while read line; do
        [ -z "${line}" ] && continue
        po_lang="${line%% *}"
        po_file="${line#* }"
        prefix="$(fcitx_lang_to_prefix "${po_lang}")"
        po_parse_cache_file="${po_parse_cache_dir}/${prefix}.fxpo"
        . "${po_parse_cache_file}"
        all_po_langs="${all_po_langs} ${po_lang}"
    done <<EOF
$(cat "${po_cache}")
EOF
}

fcitx_find_str_for_lang() {
    local lang="${1}"
    local msgid="${2}"
    local prefix="$(fcitx_lang_to_prefix "${lang}")"
    fcitx_find_str "${prefix}" "${msgid}"
}
