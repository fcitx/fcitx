#!/bin/sh

parse_cache="$1"
po_cache="$2"
action="$3"
in_file="$4"
out_file="$5"

shift 5 || exit 1

. "$(dirname "$(realpath -es $0)")/fcitx-parse-po.sh"

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
