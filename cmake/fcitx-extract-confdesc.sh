#!/bin/bash

action="$1"

shift 1 || exit 1

. "$(dirname ${BASH_SOURCE})/fcitx-write-po.sh"

extract_confdesc() {
    local in_file="$1"
    local out_file="$2"
    local line_num=0
    local line
    local lines
    mapfile -t lines < "${in_file}"
    for line in "${lines[@]}"; do
        ((line_num++))
        if [[ $line =~ ^\[(.*)/(.*)\]$ ]]; then
            fcitx_record_po_msg "${in_file}" "${BASH_REMATCH[1]}" "${line_num}"
            # fcitx_record_po_msg "${in_file}" "${BASH_REMATCH[2]}" "${line_num}"
        elif [[ $line =~ ^[\ \t]*(Description|LongDescription|Enum[0-9]+)=(.*)$ ]]; then
            fcitx_record_po_msg "${in_file}" "${BASH_REMATCH[2]}" "${line_num}"
        fi
    done
}

case "${action}" in
    -c)
        in_file="${1}"
        if fcitx_str_match "*.desc" "${in_file}"; then
            exit 0
        fi
        exit 1
        ;;
    -w)
        out_file="${1}"
        shift || exit 1
        fcitx_write_po_header "${out_file}"
        for rel_path in "$@"; do
            echo "Extracting po string from ${rel_path}"
            extract_confdesc "${rel_path}" "${out_file}"
        done
        fcitx_write_all_po "${out_file}"
        exit 0
        ;;
esac
