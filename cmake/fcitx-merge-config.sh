#!/bin/bash

cache_base="$1"
po_cache="$2"
action="$3"
in_file="$4"
out_file="$5"

shift 5 || exit 1

filter_src="$(dirname ${BASH_SOURCE})/fcitx-filter-po.sh"
. "$(dirname ${BASH_SOURCE})/fcitx-parse-po.sh"

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
