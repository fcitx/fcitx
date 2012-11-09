#!/bin/bash

filter_src="$(dirname ${BASH_SOURCE})/fcitx-filter-po.sh"

quote () {
    local quoted=${1//\'/\'\\\'\'};
    printf "'%s'" "$quoted"
}

str_match() {
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

msgid_to_varname() {
    local prefix="${1}"
    local msgid="${2}"
    echo -n "${prefix}___$(echo -n "${msgid}" | base64 -w0 | sed -e 's:=:_1:g' -e 's:\+:_2:g' -e 's:/:_3:g')"
}

parse_po_file() {
    local prefix="${1}"
    local pofile="${2}"
    local parse_res="${3}"
    msgattrib -o- --translated --no-fuzzy --no-obsolete --force-po "${pofile}" |
    msgconv -tutf-8 --force-po |
    msgexec -i- "${filter_src}" "${prefix}" > "${parse_res}"
}

find_str() {
    local prefix="${1}"
    local msgid="${2}"
    local varname="$(msgid_to_varname "${prefix}" "${msgid}")"
    echo -n "${!varname}" | base64 -d
}

all_po_langs=()

lang_to_prefix() {
    local lang="${1}"
    echo -n "${lang}" | base64 -w0 | sed -e 's:_:_0:g' \
        -e 's:=:_1:g' -e 's:\+:_2:g' -e 's:/:_3:g' -e 's:\([A-Z]\):_\L\1:g'
}

load_all_pos() {
    local po_cache="$1"
    local cache_base="$2"
    local po_lang
    local po_file
    local prefix
    local po_parse_cache_file
    local po_parse_cache_dir="${cache_base}/fcitx_lang_parse"
    mkdir -p "${po_parse_cache_dir}"
    local line
    local lines
    mapfile -t lines < "${po_cache}"
    for line in "${lines[@]}"; do
        po_lang="${line%% *}"
        po_file="${line#* }"
        prefix="$(lang_to_prefix "${po_lang}")"
        po_parse_cache_file="${po_parse_cache_dir}/${prefix}"
        if [[ "${po_parse_cache_file}" -ot "${po_file}" ]]; then
            echo "Parsing po file: ${po_file}"
            local unique_fname="${po_parse_cache_file}.$$"
            parse_po_file "${prefix}" "${po_file}" "${unique_fname}"
            . "${unique_fname}"
            mv "${unique_fname}" "${po_parse_cache_file}"
        else
            . "${po_parse_cache_file}"
        fi
        all_po_langs=("${all_po_langs[@]}" "${po_lang}")
    done
}

find_str_for_lang() {
    local lang="${1}"
    local msgid="${2}"
    local prefix="$(lang_to_prefix "${lang}")"
    find_str "${prefix}" "${msgid}"
}
