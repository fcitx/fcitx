#!/bin/bash

cache_base="$1"
po_cache="$2"
action="$3"
in_file="$4"
out_file="$5"

dirname="$(dirname ${BASH_SOURCE})"
filter_src="${dirname}/fcitx-filter-po.sh"

shift 5 || exit 1

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
    echo -n "${prefix}_$(echo -n "${msgid}" | base64 -w0 | sed -e 's:=:_1:g' -e 's:\+:_2:g' -e 's:/:_3:g')"
}

parse_po_file() {
    local prefix="${1}"
    local pofile="${2}"
    local parse_res="${3}"
    msgexec -i "${pofile}" "${filter_src}" "${prefix}" > "${parse_res}"
}

find_str() {
    local prefix="${1}"
    local msgid="${2}"
    local varname="$(msgid_to_varname "${prefix}" "${msgid}")"
    echo -n "${!varname}"
}

all_po_langs=()

lang_to_prefix() {
    local lang="${1}"
    echo -n "${lang}" | base64 -w0 | sed -e 's:_:_0:g' \
        -e 's:=:_1:g' -e 's:\+:_2:g' -e 's:/:_3:g'
}

load_all_pos() {
    local po_cache="${1}"
    local po_lang
    local po_file
    local prefix
    local po_parse_cache_file
    local po_parse_cache_dir="${cache_base}/fcitx_lang_parse"
    mkdir -p "${po_parse_cache_dir}"
    while read line; do
        po_lang="${line%% *}"
        po_file="${line#* }"
        prefix="$(lang_to_prefix "${po_lang}")"
        po_parse_cache_file="${po_parse_cache_dir}/${prefix}"
        if [[ "${po_parse_cache_file}" -ot "${po_file}" ]]; then
            echo "Parsing po file: ${po_file}"
            parse_po_file "${prefix}" "${po_file}" "${po_parse_cache_file}"
        fi
        . "${po_parse_cache_file}"
        all_po_langs=("${all_po_langs[@]}" "${po_lang}")
    done < "${po_cache}"
}

find_str_for_lang() {
    local lang="${1}"
    local msgid="${2}"
    local prefix="$(lang_to_prefix "${lang}")"
    find_str "${prefix}" "${msgid}"
}

merge_config() {
    local in_file="${1}"
    local out_file="${2}"
    local line
    local key
    local msgid
    local msgstr
    local lang
    while read line; do
        case "${line}" in
            _*=*)
                line="${line#_}"
                echo "${line}"
                key="${line%%=*}"
                msgid="${line#*=}"
                for lang in "${all_po_langs[@]}"; do
                    msgstr="$(find_str_for_lang "${lang}" "${msgid}")"
                    if [[ -z ${msgstr} ]]; then
                        continue
                    fi
                    echo "${key}[${lang}]=${msgstr}"
                done
                ;;
            *)
                echo "${line}"
                ;;
        esac
    done < "${in_file}" > "${out_file}"
}

case "${action}" in
    -c)
        if str_match "*.conf.in" "${in_file}" &&
            str_match "*.conf" "${out_file}"; then
            exit 0
        elif str_match "*.desktop.in" "${in_file}" &&
            str_match "*.desktop" "${out_file}"; then
            exit 0
        fi
        exit 1
        ;;
    -w)
        load_all_pos "${po_cache}"
        merge_config "${in_file}" "${out_file}"
        exit 0
        ;;
esac
